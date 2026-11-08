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
    int renderQuality = 1; // 0=low (1.0x), 1=medium (1.5x), 2=high (2.0x) supersampling
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
    std::string playerName = "PLAYER"; // last name submitted to the high-score table

    // Phase H: graphics options. Every field is a plain int so a broken
    // settings.cfg is clamped instead of crashing.
    int fxaa = 1;      // 0/1, independent of Render Quality now
    int bloom = 0;     // 0=off, 1=low, 2=high
    int vignette = 1;  // 0/1
    int shadows = 1;   // 0/1, contact shadows under apples / rivals / the snake
    int fovOffset = 2; // index into kFovOffsets (default 0 degrees)
    int showFps = 0;   // 0/1
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

// Phase H: three settings tabs. settingsIndex addresses a slot inside the active
// tab, so every tab can fit on one screen without moving rows off the canvas.
enum SettingTab { TabGraphics = 0, TabGameplay, TabAudio, kSettingTabCount };

enum SettingRow {
    RowPreset = 0,
    RowResolution,
    RowDisplayMode,
    RowVSync,
    RowFps,
    RowRenderQuality,
    RowFxaa,
    RowBloom,
    RowVignette,
    RowShadows,
    RowParticles,
    RowFov,
    RowShowFps,
    RowSpeed,
    RowCamera,
    RowAnimations,
    RowShake,
    RowMasterVol,
    RowMusicVol,
    RowSfxVol,
    RowMute,
    RowReset,
    kSettingRowCount
};

// The widest tab (GRAPHICS) has 14 rows: 136 + 13*21 + 20 = 429 px.
inline constexpr int kMaxSettingsRowsPerTab = 14;

// Camera FOV offset choices in degrees, added on top of every camera mode.
inline constexpr int kFovOffsetCount = 6;
inline constexpr int kFovOffsets[kFovOffsetCount] = {-10, -5, 0, 5, 10, 15};
inline constexpr int kDefaultFovOffset = 2;

// Graphics presets: 0 Low, 1 Medium, 2 High, 3 Ultra, 4 Custom (derived only).
inline constexpr int kPresetLow = 0;
inline constexpr int kPresetMedium = 1;
inline constexpr int kPresetHigh = 2;
inline constexpr int kPresetUltra = 3;
inline constexpr int kPresetCustom = 4;
inline constexpr int kPresetCount = 5;

Settings loadSettings(const std::string& path = "settings.cfg");
void saveSettings(const Settings& s, const std::string& path = "settings.cfg");

std::string settingLabel(const Settings& s, int row);
void adjustSetting(Settings& s, int row, int delta);

// Per-tab row lists (the GRAPHICS/GAMEPLAY/AUDIO tabs).
int settingsRowCount(int tab);
int settingsRowAt(int tab, int index);
const char* settingsTabName(int tab);

// Preset helpers: applyPreset writes the group, detectPreset derives the row value
// (Custom whenever the individual graphics rows no longer match a preset).
void applyPreset(Settings& s, int preset);
int detectPreset(const Settings& s);
float fovOffsetDegrees(int index);

int fpsTarget(int index);
float speedTickScale(int speed);
float particleScale(int particles);
float renderQualityScale(int quality); // supersampling factor for the canvas

} // namespace snake
