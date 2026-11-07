#pragma once

#include <string>

namespace snake {

// Persisted, user-facing options. All fields are plain ints so a broken
// settings.cfg can be clamped instead of crashing the game.
struct Settings {
    int resolution = 1; // index into kResolutions
    int fullscreen = 0; // 0/1
    int vsync = 1;      // 0/1
    int fpsLimit = 1;   // 0=30, 1=60, 2=120, 3=unlimited
    int speed = 1;      // 0=slow, 1=normal, 2=fast
    int camera = 0;     // 0=iso, 1=top, 2=third person
    int animations = 1; // 0/1
    int particles = 1;  // 0=low, 1=normal, 2=high
    int shake = 1;      // 0/1
    int skin = 0;       // index into kSkins (Phase 5)
    int masterVol = 80; // percent
    int musicVol = 50;  // percent
    int sfxVol = 80;    // percent
    int mute = 0;       // 0/1
};

struct Resolution {
    int w;
    int h;
    const char* label;
};

inline constexpr int kResolutionCount = 4;
inline constexpr Resolution kResolutions[kResolutionCount] = {
    {960, 540, "960 x 540"},
    {1280, 720, "1280 x 720"},
    {1600, 900, "1600 x 900"},
    {1920, 1080, "1920 x 1080"},
};

// Settings rows, in the order the Settings screen lists them.
enum SettingRow {
    RowResolution = 0,
    RowFullscreen,
    RowVSync,
    RowFps,
    RowSpeed,
    RowCamera,
    RowAnimations,
    RowParticles,
    RowShake,
    RowMasterVol,
    RowMusicVol,
    RowSfxVol,
    RowMute,
    RowReset,
    kSettingRowCount
};

Settings loadSettings(const std::string& path = "settings.cfg");
void saveSettings(const Settings& s, const std::string& path = "settings.cfg");

std::string settingLabel(const Settings& s, int row);
void adjustSetting(Settings& s, int row, int delta);

int fpsTarget(int index);
float speedTickScale(int speed);
float particleScale(int particles);

} // namespace snake
