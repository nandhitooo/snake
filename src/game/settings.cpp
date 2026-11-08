#include "game/settings.hpp"

#include "game/config.hpp"
#include "game/highscore.hpp"
#include "game/skins.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <string>

namespace snake {
namespace {

int clampInt(int v, int lo, int hi) { return std::max(lo, std::min(hi, v)); }

std::string trim(const std::string& in) {
    size_t begin = 0;
    size_t end = in.size();
    while (begin < end && std::isspace(static_cast<unsigned char>(in[begin])) != 0) {
        ++begin;
    }
    while (end > begin && std::isspace(static_cast<unsigned char>(in[end - 1])) != 0) {
        --end;
    }
    return in.substr(begin, end - begin);
}

const char* presetName(int preset) {
    switch (preset) {
    case kPresetLow:
        return "LOW";
    case kPresetMedium:
        return "MEDIUM";
    case kPresetHigh:
        return "HIGH";
    case kPresetUltra:
        return "ULTRA";
    case kPresetCustom:
    default:
        return "CUSTOM";
    }
}

int readInt(const std::string& value, int fallback) {
    try {
        size_t used = 0;
        const int parsed = std::stoi(value, &used);
        if (used == 0) {
            return fallback;
        }
        return parsed;
    } catch (...) {
        return fallback; // malformed value: keep the default
    }
}

} // namespace

Settings loadSettings(const std::string& path) {
    Settings s; // defaults double as the fallback for missing keys
    std::ifstream in(path);
    if (!in) {
        return s;
    }

    std::string line;
    while (std::getline(in, line)) {
        const std::string text = trim(line);
        if (text.empty() || text[0] == '#') {
            continue;
        }
        const size_t eq = text.find('=');
        if (eq == std::string::npos) {
            continue;
        }
        const std::string key = trim(text.substr(0, eq));
        const std::string raw = trim(text.substr(eq + 1));

        // A malformed number keeps the field's current value (its default), so a
        // corrupt settings.cfg degrades to sane options instead of resetting to 0.
        if (key == "resolution") {
            s.resolution = clampInt(readInt(raw, s.resolution), 0, kResolutionCount - 1);
        } else if (key == "fullscreen") {
            s.fullscreen = clampInt(readInt(raw, s.fullscreen), 0, 1);
        } else if (key == "vsync") {
            s.vsync = clampInt(readInt(raw, s.vsync), 0, 1);
        } else if (key == "fps") {
            s.fpsLimit = clampInt(readInt(raw, s.fpsLimit), 0, 3);
        } else if (key == "render_quality") {
            s.renderQuality = clampInt(readInt(raw, s.renderQuality), 0, 2);
        } else if (key == "speed") {
            s.speed = clampInt(readInt(raw, s.speed), 0, 2);
        } else if (key == "camera") {
            s.camera = clampInt(readInt(raw, s.camera), 0, 2);
        } else if (key == "animations") {
            s.animations = clampInt(readInt(raw, s.animations), 0, 1);
        } else if (key == "particles") {
            s.particles = clampInt(readInt(raw, s.particles), 0, 2);
        } else if (key == "shake") {
            s.shake = clampInt(readInt(raw, s.shake), 0, 1);
        } else if (key == "skin") {
            s.skin = clampInt(readInt(raw, s.skin), 0, kSkinCount - 1);
        } else if (key == "fxaa") {
            s.fxaa = clampInt(readInt(raw, s.fxaa), 0, 1);
        } else if (key == "bloom") {
            s.bloom = clampInt(readInt(raw, s.bloom), 0, 2);
        } else if (key == "vignette") {
            s.vignette = clampInt(readInt(raw, s.vignette), 0, 1);
        } else if (key == "shadows") {
            s.shadows = clampInt(readInt(raw, s.shadows), 0, 1);
        } else if (key == "fov_offset") {
            s.fovOffset = clampInt(readInt(raw, s.fovOffset), 0, kFovOffsetCount - 1);
        } else if (key == "show_fps") {
            s.showFps = clampInt(readInt(raw, s.showFps), 0, 1);
        } else if (key == "master_vol") {
            s.masterVol = clampInt(readInt(raw, s.masterVol), 0, 100);
        } else if (key == "music_vol") {
            s.musicVol = clampInt(readInt(raw, s.musicVol), 0, 100);
        } else if (key == "sfx_vol") {
            s.sfxVol = clampInt(readInt(raw, s.sfxVol), 0, 100);
        } else if (key == "mute") {
            s.mute = clampInt(readInt(raw, s.mute), 0, 1);
        } else if (key == "player_name") {
            s.playerName = sanitizePlayerName(raw);
        }
    }
    return s;
}

void saveSettings(const Settings& s, const std::string& path) {
    std::ofstream out(path, std::ios::trunc);
    if (!out) {
        return; // read-only working directory: keep playing with in-memory settings
    }
    out << "# snake settings (key=value)\n";
    out << "resolution=" << s.resolution << '\n';
    out << "fullscreen=" << s.fullscreen << '\n';
    out << "vsync=" << s.vsync << '\n';
    out << "fps=" << s.fpsLimit << '\n';
    out << "render_quality=" << s.renderQuality << '\n';
    out << "speed=" << s.speed << '\n';
    out << "camera=" << s.camera << '\n';
    out << "animations=" << s.animations << '\n';
    out << "particles=" << s.particles << '\n';
    out << "shake=" << s.shake << '\n';
    out << "skin=" << s.skin << '\n';
    out << "fxaa=" << s.fxaa << '\n';
    out << "bloom=" << s.bloom << '\n';
    out << "vignette=" << s.vignette << '\n';
    out << "shadows=" << s.shadows << '\n';
    out << "fov_offset=" << s.fovOffset << '\n';
    out << "show_fps=" << s.showFps << '\n';
    out << "master_vol=" << s.masterVol << '\n';
    out << "music_vol=" << s.musicVol << '\n';
    out << "sfx_vol=" << s.sfxVol << '\n';
    out << "mute=" << s.mute << '\n';
    out << "player_name=" << s.playerName << '\n';
}

int fpsTarget(int index) {
    switch (clampInt(index, 0, 3)) {
    case 0:
        return 30;
    case 2:
        return 120;
    case 3:
        return 0; // unlimited
    case 1:
    default:
        return 60;
    }
}

float renderQualityScale(int quality) {
    switch (clampInt(quality, 0, 2)) {
    case 0:
        return kRenderScaleLow;
    case 2:
        return kRenderScaleHigh;
    case 1:
    default:
        return kRenderScaleMedium;
    }
}

float speedTickScale(int speed) {
    switch (clampInt(speed, 0, 2)) {
    case 0:
        return 1.25f; // slow: longer interval
    case 2:
        return 0.8f; // fast: shorter interval
    case 1:
    default:
        return 1.0f;
    }
}

float particleScale(int particles) {
    switch (clampInt(particles, 0, 2)) {
    case 0:
        return 0.45f;
    case 2:
        return 1.7f;
    case 1:
    default:
        return 1.0f;
    }
}

std::string settingLabel(const Settings& s, int row) {
    switch (row) {
    case RowPreset:
        return presetName(detectPreset(s));
    case RowResolution:
        return kResolutions[clampInt(s.resolution, 0, kResolutionCount - 1)].label;
    case RowDisplayMode:
        return s.fullscreen != 0 ? "FULLSCREEN" : "WINDOWED";
    case RowVSync:
        return s.vsync != 0 ? "ON" : "OFF";
    case RowFps:
        switch (clampInt(s.fpsLimit, 0, 3)) {
        case 0:
            return "30";
        case 2:
            return "120";
        case 3:
            return "UNLIMITED";
        case 1:
        default:
            return "60";
        }
    case RowRenderQuality:
        switch (clampInt(s.renderQuality, 0, 2)) {
        case 0:
            return "LOW";
        case 2:
            return "HIGH";
        case 1:
        default:
            return "MEDIUM";
        }
    case RowSpeed:
        switch (clampInt(s.speed, 0, 2)) {
        case 0:
            return "SLOW";
        case 2:
            return "FAST";
        case 1:
        default:
            return "NORMAL";
        }
    case RowCamera:
        switch (clampInt(s.camera, 0, 2)) {
        case 1:
            return "TOP-DOWN";
        case 2:
            return "THIRD-PERSON";
        case 0:
        default:
            return "ISOMETRIC";
        }
    case RowFxaa:
        return s.fxaa != 0 ? "ON" : "OFF";
    case RowBloom:
        switch (clampInt(s.bloom, 0, 2)) {
        case 0:
            return "OFF";
        case 2:
            return "HIGH";
        case 1:
        default:
            return "LOW";
        }
    case RowVignette:
        return s.vignette != 0 ? "ON" : "OFF";
    case RowShadows:
        return s.shadows != 0 ? "ON" : "OFF";
    case RowFov: {
        const int deg = kFovOffsets[clampInt(s.fovOffset, 0, kFovOffsetCount - 1)];
        return deg > 0 ? ("+" + std::to_string(deg) + " DEG") : (std::to_string(deg) + " DEG");
    }
    case RowShowFps:
        return s.showFps != 0 ? "ON" : "OFF";
    case RowAnimations:
        return s.animations != 0 ? "ON" : "OFF";
    case RowParticles:
        switch (clampInt(s.particles, 0, 2)) {
        case 0:
            return "LOW";
        case 2:
            return "HIGH";
        case 1:
        default:
            return "NORMAL";
        }
    case RowShake:
        return s.shake != 0 ? "ON" : "OFF";
    case RowMasterVol:
        return std::to_string(clampInt(s.masterVol, 0, 100)) + "%";
    case RowMusicVol:
        return std::to_string(clampInt(s.musicVol, 0, 100)) + "%";
    case RowSfxVol:
        return std::to_string(clampInt(s.sfxVol, 0, 100)) + "%";
    case RowMute:
        return s.mute != 0 ? "ON" : "OFF";
    case RowReset:
        return "PRESS ENTER";
    default:
        return "";
    }
}

void adjustSetting(Settings& s, int row, int delta) {
    const int step = (delta >= 0) ? 1 : -1;
    switch (row) {
    case RowPreset: {
        const int current = detectPreset(s);
        const int base =
            (current == kPresetCustom) ? (step > 0 ? kPresetLow : kPresetHigh) : current;
        applyPreset(s, (base + step + 4) % 4);
        break;
    }
    case RowResolution:
        s.resolution = clampInt(s.resolution + step, 0, kResolutionCount - 1);
        break;
    case RowDisplayMode:
        s.fullscreen = (s.fullscreen != 0) ? 0 : 1;
        break;
    case RowVSync:
        s.vsync = (s.vsync != 0) ? 0 : 1;
        break;
    case RowFps:
        s.fpsLimit = clampInt(s.fpsLimit + step, 0, 3);
        break;
    case RowRenderQuality:
        s.renderQuality = clampInt(s.renderQuality + step, 0, 2);
        break;
    case RowSpeed:
        s.speed = clampInt(s.speed + step, 0, 2);
        break;
    case RowCamera:
        s.camera = clampInt(s.camera + step, 0, 2);
        break;
    case RowFxaa:
        s.fxaa = (s.fxaa != 0) ? 0 : 1;
        break;
    case RowBloom:
        s.bloom = clampInt(s.bloom + step, 0, 2);
        break;
    case RowVignette:
        s.vignette = (s.vignette != 0) ? 0 : 1;
        break;
    case RowShadows:
        s.shadows = (s.shadows != 0) ? 0 : 1;
        break;
    case RowFov:
        s.fovOffset = clampInt(s.fovOffset + step, 0, kFovOffsetCount - 1);
        break;
    case RowShowFps:
        s.showFps = (s.showFps != 0) ? 0 : 1;
        break;
    case RowAnimations:
        s.animations = (s.animations != 0) ? 0 : 1;
        break;
    case RowParticles:
        s.particles = clampInt(s.particles + step, 0, 2);
        break;
    case RowShake:
        s.shake = (s.shake != 0) ? 0 : 1;
        break;
    case RowMasterVol:
        s.masterVol = clampInt(s.masterVol + step * 5, 0, 100);
        break;
    case RowMusicVol:
        s.musicVol = clampInt(s.musicVol + step * 5, 0, 100);
        break;
    case RowSfxVol:
        s.sfxVol = clampInt(s.sfxVol + step * 5, 0, 100);
        break;
    case RowMute:
        s.mute = (s.mute != 0) ? 0 : 1;
        break;
    case RowReset:
    default:
        break;
    }
}

int settingsRowCount(int tab) {
    static const int sizes[kSettingTabCount] = {14, 5, 5};
    return sizes[clampInt(tab, 0, kSettingTabCount - 1)];
}

int settingsRowAt(int tab, int index) {
    static const int graphics[kMaxSettingsRowsPerTab] = {
        RowPreset,  RowResolution, RowDisplayMode, RowVSync,    RowFps,     RowRenderQuality, RowFxaa,
        RowBloom,   RowVignette,   RowShadows,     RowParticles, RowFov,   RowShowFps,       RowReset};
    static const int gameplay[] = {RowSpeed, RowCamera, RowAnimations, RowShake, RowReset};
    static const int audio[] = {RowMasterVol, RowMusicVol, RowSfxVol, RowMute, RowReset};
    const int t = clampInt(tab, 0, kSettingTabCount - 1);
    const int count = settingsRowCount(t);
    const int i = ((index % count) + count) % count;
    switch (t) {
    case TabGameplay:
        return gameplay[i];
    case TabAudio:
        return audio[i];
    case TabGraphics:
    default:
        return graphics[i];
    }
}

const char* settingsTabName(int tab) {
    switch (tab) {
    case TabGameplay:
        return "GAMEPLAY";
    case TabAudio:
        return "AUDIO";
    case TabGraphics:
    default:
        return "GRAPHICS";
    }
}

void applyPreset(Settings& s, int preset) {
    switch (preset) {
    case kPresetLow:
        s.renderQuality = 0;
        s.fxaa = 1;
        s.bloom = 0;
        s.vignette = 0;
        s.shadows = 0;
        s.particles = 0;
        break;
    case kPresetMedium:
        s.renderQuality = 1;
        s.fxaa = 1;
        s.bloom = 0;
        s.vignette = 1;
        s.shadows = 1;
        s.particles = 1;
        break;
    case kPresetHigh:
        s.renderQuality = 2;
        s.fxaa = 0;
        s.bloom = 1;
        s.vignette = 1;
        s.shadows = 1;
        s.particles = 1;
        break;
    case kPresetUltra:
        s.renderQuality = 2;
        s.fxaa = 0;
        s.bloom = 2;
        s.vignette = 1;
        s.shadows = 1;
        s.particles = 2;
        break;
    case kPresetCustom:
    default:
        break; // Custom is whatever the individual graphics rows already say
    }
}

int detectPreset(const Settings& s) {
    for (int preset = 0; preset < kPresetCustom; ++preset) {
        Settings probe = s;
        applyPreset(probe, preset);
        if (probe.renderQuality == s.renderQuality && probe.fxaa == s.fxaa &&
            probe.bloom == s.bloom && probe.vignette == s.vignette &&
            probe.shadows == s.shadows && probe.particles == s.particles) {
            return preset;
        }
    }
    return kPresetCustom;
}

float fovOffsetDegrees(int index) {
    return static_cast<float>(kFovOffsets[clampInt(index, 0, kFovOffsetCount - 1)]);
}

} // namespace snake
