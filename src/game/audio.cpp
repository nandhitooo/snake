#include "game/audio.hpp"
#include "game/assets.hpp"

#include "raylib.h"

#include <algorithm>
#include <random>
#include <string>
#include <vector>

namespace snake {

Audio audio;

namespace {

// Short repeats (eat, clicks) get a small pool so the same sample can overlap
// itself instead of cutting the previous one off.
constexpr int kVoices = 3;

struct SfxDef {
    const char* file;
    float pitchJitter; // +/- fraction, 0 = no variation
};

// Indexed by SfxId; keep the order in sync with the enum.
constexpr SfxDef kSfxDefs[static_cast<int>(SfxId::Count)] = {
    {"assets/audio/sfx/menu_hover.wav", 0.0f},
    {"assets/audio/sfx/menu_click.wav", 0.0f},
    {"assets/audio/sfx/menu_back.wav", 0.0f},
    {"assets/audio/sfx/eat_food.mp3", 0.05f},
    {"assets/audio/sfx/special_good.mp3", 0.02f},
    {"assets/audio/sfx/special_bad.mp3", 0.0f},
    {"assets/audio/sfx/boost_start.mp3", 0.0f},
    {"assets/audio/sfx/boost_end.mp3", 0.0f},
    {"assets/audio/sfx/obstacle_warning.wav", 0.0f},
    {"assets/audio/sfx/obstacle_spawn.mp3", 0.02f},
    {"assets/audio/sfx/ai_spawn.mp3", 0.0f},
    {"assets/audio/sfx/eat_ai_snake.mp3", 0.0f},
    {"assets/audio/sfx/ai_die.mp3", 0.03f},
    {"assets/audio/sfx/game_over.mp3", 0.0f},
    {"assets/audio/sfx/pause.wav", 0.0f},
    {"assets/audio/sfx/resume.wav", 0.0f},
};

struct SfxSlot {
    Sound base{};
    std::vector<Sound> voices;
    size_t next = 0;
    bool loaded = false;
};

SfxSlot g_slots[static_cast<int>(SfxId::Count)];
Music g_music[static_cast<int>(MusicTrack::Count)];
bool g_musicLoaded[static_cast<int>(MusicTrack::Count)] = {false, false, false};
std::mt19937 g_rng{std::random_device{}()};

// Files are looked up next to the executable too (see game/assets.hpp).
std::string resolveAsset(const char* relative) { return resolveAssetPath(relative); }

void loadSounds() {
    for (int i = 0; i < static_cast<int>(SfxId::Count); ++i) {
        SfxSlot& slot = g_slots[i];
        const std::string path = resolveAsset(kSfxDefs[i].file);
        if (path.empty()) {
            continue; // missing file: that sound simply stays silent
        }
        const Sound base = LoadSound(path.c_str());
        if (!IsSoundValid(base)) {
            continue;
        }
        slot.base = base;
        slot.loaded = true;
        slot.voices.push_back(base);
        for (int v = 1; v < kVoices; ++v) {
            const Sound alias = LoadSoundAlias(base);
            if (IsSoundValid(alias)) {
                slot.voices.push_back(alias);
            }
        }
    }
}

void loadMusic() {
    const char* files[static_cast<int>(MusicTrack::Count)] = {
        nullptr,
        "assets/audio/music/menu.ogg",
        "assets/audio/music/gameplay.ogg",
    };
    for (int i = 1; i < static_cast<int>(MusicTrack::Count); ++i) {
        const std::string path = resolveAsset(files[i]);
        if (path.empty()) {
            continue;
        }
        Music m = LoadMusicStream(path.c_str());
        if (!IsMusicValid(m)) {
            continue;
        }
        m.looping = true;
        g_music[i] = m;
        g_musicLoaded[i] = true;
    }
}

} // namespace

void Audio::init() {
    if (ready) {
        return;
    }
    if (!IsAudioDeviceReady()) {
        InitAudioDevice();
    }
    if (!IsAudioDeviceReady()) {
        ready = false; // no sound card / no audio server: keep the game silent
        return;
    }
    ready = true;
    loadSounds();
    loadMusic();
    applyVolumes(static_cast<int>(masterVol * 100.0f), static_cast<int>(musicVol * 100.0f),
                 static_cast<int>(sfxVol * 100.0f), muted);
}

void Audio::shutdown() {
    if (!ready) {
        return;
    }
    for (SfxSlot& slot : g_slots) {
        if (!slot.loaded) {
            continue;
        }
        for (size_t v = 1; v < slot.voices.size(); ++v) {
            UnloadSoundAlias(slot.voices[v]);
        }
        UnloadSound(slot.base);
        slot.voices.clear();
        slot.loaded = false;
    }
    for (int i = 1; i < static_cast<int>(MusicTrack::Count); ++i) {
        if (g_musicLoaded[i]) {
            StopMusicStream(g_music[i]);
            UnloadMusicStream(g_music[i]);
            g_musicLoaded[i] = false;
        }
    }
    CloseAudioDevice();
    ready = false;
    track = MusicTrack::None;
}

void Audio::applyVolumes(int masterPct, int musicPct, int sfxPct, bool mute) {
    masterVol = static_cast<float>(std::clamp(masterPct, 0, 100)) / 100.0f;
    musicVol = static_cast<float>(std::clamp(musicPct, 0, 100)) / 100.0f;
    sfxVol = static_cast<float>(std::clamp(sfxPct, 0, 100)) / 100.0f;
    muted = mute;
    if (!ready) {
        return;
    }
    SetMasterVolume(muted ? 0.0f : masterVol);
    if (track != MusicTrack::None && g_musicLoaded[static_cast<int>(track)]) {
        SetMusicVolume(g_music[static_cast<int>(track)], musicVol * fade);
    }
}

void Audio::play(SfxId id) {
    if (!ready || muted) {
        return;
    }
    const int index = static_cast<int>(id);
    if (index < 0 || index >= static_cast<int>(SfxId::Count)) {
        return;
    }
    SfxSlot& slot = g_slots[index];
    if (!slot.loaded || slot.voices.empty()) {
        return;
    }
    Sound& voice = slot.voices[slot.next % slot.voices.size()];
    ++slot.next;

    SetSoundVolume(voice, sfxVol);
    const float jitter = kSfxDefs[index].pitchJitter;
    if (jitter > 0.0f) {
        std::uniform_real_distribution<float> dist(1.0f - jitter, 1.0f + jitter);
        SetSoundPitch(voice, dist(g_rng));
    }
    PlaySound(voice);
}

void Audio::update(MusicTrack wanted, float duck, float dt) {
    if (!ready) {
        return;
    }
    if (wanted != track) {
        if (track != MusicTrack::None && g_musicLoaded[static_cast<int>(track)]) {
            StopMusicStream(g_music[static_cast<int>(track)]);
        }
        track = wanted;
        fade = 0.0f;
        if (track != MusicTrack::None && g_musicLoaded[static_cast<int>(track)]) {
            PlayMusicStream(g_music[static_cast<int>(track)]);
        }
    }
    if (track == MusicTrack::None || !g_musicLoaded[static_cast<int>(track)]) {
        return;
    }
    fade = std::min(1.0f, fade + dt / 0.5f); // ~0.5 s fade-in on every switch
    Music& m = g_music[static_cast<int>(track)];
    SetMusicVolume(m, musicVol * fade * std::clamp(duck, 0.0f, 1.0f));
    UpdateMusicStream(m); // must run every frame or the stream starves
}

} // namespace snake
