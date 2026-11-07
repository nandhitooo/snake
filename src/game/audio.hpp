#pragma once

// Audio service: one global instance so gameplay (game.cpp), the AI module and
// the UI can fire sounds without threading a handle through every call.
//
// Everything degrades to silence: if the audio device is missing or the
// assets/audio folder is not there, init()/play() become no-ops.

namespace snake {

enum class SfxId {
    MenuHover,
    MenuClick,
    MenuBack,
    EatFood,
    SpecialGood,
    SpecialBad,
    BoostStart,
    BoostEnd,
    ObstacleWarning,
    ObstacleSpawn,
    AiSpawn,
    EatAiSnake,
    AiDie,
    GameOver,
    Pause,
    Resume,
    Count
};

enum class MusicTrack { None = 0, Menu, Gameplay, Count };

struct Audio {
    bool ready = false;   // audio device up and at least the music loaded
    bool muted = false;
    MusicTrack track = MusicTrack::None;
    float fade = 0.0f;    // 0..1 fade-in of the current track
    float masterVol = 0.8f;
    float musicVol = 0.5f;
    float sfxVol = 0.8f;

    void init();   // call once after InitWindow()
    void shutdown();

    // Percentages 0..100 come straight from Settings (already clamped there too).
    void applyVolumes(int masterPct, int musicPct, int sfxPct, bool mute);

    void play(SfxId id);

    // Per frame: switches tracks, applies the fade and streams the music.
    // duck 1.0 = normal, 0.3 while paused, 0.4 on game over.
    void update(MusicTrack wanted, float duck, float dt);
};

extern Audio audio;

} // namespace snake
