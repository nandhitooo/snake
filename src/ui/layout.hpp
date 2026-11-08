#pragma once

#include "game/config.hpp"
#include "game/highscore.hpp"
#include "game/skins.hpp"

#include "raylib.h"

namespace snake::ui {

// --- debug layout guard ---------------------------------------------------
// validateLayout() is compiled in only when NDEBUG is not defined: it checks every
// interactive rect against the 960x540 canvas and reports overlaps per screen.

inline bool rectInsideCanvas(const Rectangle& r) {
    return r.width > 0.0f && r.height > 0.0f && r.x >= 0.0f && r.y >= 0.0f &&
           r.x + r.width <= static_cast<float>(kWinW) &&
           r.y + r.height <= static_cast<float>(kWinH);
}

inline bool rectsOverlap(const Rectangle& a, const Rectangle& b) {
    return a.x < b.x + b.width && b.x < a.x + a.width && a.y < b.y + b.height &&
           b.y < a.y + a.height;
}

inline void checkScreenRects(const char* screen, const Rectangle* rects, int count) {
    for (int i = 0; i < count; ++i) {
        if (!rectInsideCanvas(rects[i])) {
            TraceLog(LOG_WARNING, "layout(%s): rect %d is outside 960x540", screen, i);
        }
        for (int j = i + 1; j < count; ++j) {
            if (rectsOverlap(rects[i], rects[j])) {
                TraceLog(LOG_WARNING, "layout(%s): rects %d and %d overlap", screen, i, j);
            }
        }
    }
}

inline Rectangle moveLeftRect() {
    return Rectangle{static_cast<float>(kArenaX + 16), static_cast<float>(kBarY + 8), 42.0f, 44.0f};
}
inline Rectangle moveUpRect() {
    return Rectangle{static_cast<float>(kArenaX + 64), static_cast<float>(kBarY + 8), 42.0f, 44.0f};
}
inline Rectangle moveDownRect() {
    return Rectangle{static_cast<float>(kArenaX + 112), static_cast<float>(kBarY + 8), 42.0f,
                     44.0f};
}
inline Rectangle moveRightRect() {
    return Rectangle{static_cast<float>(kArenaX + 160), static_cast<float>(kBarY + 8), 42.0f,
                     44.0f};
}
inline Rectangle boostRect() {
    return Rectangle{static_cast<float>(kArenaX + 210), static_cast<float>(kBarY + 8), 84.0f,
                     44.0f};
}
inline Rectangle pauseRect() {
    return Rectangle{static_cast<float>(kArenaX + 302), static_cast<float>(kBarY + 8), 84.0f,
                     44.0f};
}
inline Rectangle restartRect() {
    return Rectangle{static_cast<float>(kArenaX + 394), static_cast<float>(kBarY + 8), 110.0f,
                     44.0f};
}
inline Rectangle quitRect() {
    return Rectangle{static_cast<float>(kArenaX + 512), static_cast<float>(kBarY + 8), 112.0f,
                     44.0f};
}

// Main menu: six 40 px rows between the snake motif (its lowest point is ~y=197)
// and the hint line at y=505, leaving 4 px of air between rows.
inline Rectangle menuPlayRect() { return Rectangle{340.0f, 208.0f, 280.0f, 40.0f}; }
inline Rectangle menuSkinsRect() { return Rectangle{340.0f, 252.0f, 280.0f, 40.0f}; }
inline Rectangle menuHighScoresRect() { return Rectangle{340.0f, 296.0f, 280.0f, 40.0f}; }
inline Rectangle menuSettingsRect() { return Rectangle{340.0f, 340.0f, 280.0f, 40.0f}; }
inline Rectangle menuFullscreenRect() { return Rectangle{340.0f, 384.0f, 280.0f, 40.0f}; }
inline Rectangle menuQuitRect() { return Rectangle{340.0f, 428.0f, 280.0f, 40.0f}; }

// Settings screen: three tabs, then up to 14 rows (GRAPHICS) and the back button.
inline Rectangle settingsTabRect(int index) {
    return Rectangle{200.0f + static_cast<float>(index) * 190.0f, 92.0f, 180.0f, 28.0f};
}
inline Rectangle settingsRowRect(int index) {
    return Rectangle{200.0f, 136.0f + static_cast<float>(index) * 21.0f, 560.0f, 20.0f};
}
inline Rectangle settingsBackRect() { return Rectangle{380.0f, 452.0f, 200.0f, 40.0f}; }

// Skins screen: six cards in a row plus a back button.
inline Rectangle skinCardRect(int index) {
    return Rectangle{66.0f + static_cast<float>(index) * 140.0f, 348.0f, 128.0f, 74.0f};
}
inline Rectangle skinsBackRect() { return Rectangle{400.0f, 442.0f, 160.0f, 38.0f}; }

// High Scores screen: ten table rows, BACK and the two-step CLEAR.
inline Rectangle highScoreRowRect(int index) {
    return Rectangle{160.0f, 128.0f + static_cast<float>(index) * 27.0f, 640.0f, 25.0f};
}
inline Rectangle highScoresBackRect() { return Rectangle{300.0f, 452.0f, 160.0f, 36.0f}; }
inline Rectangle highScoresClearRect() { return Rectangle{500.0f, 452.0f, 160.0f, 36.0f}; }

// Pause card (shared by renderer and input so hit boxes match the drawing).
inline Rectangle pauseCardRect() {
    return Rectangle{static_cast<float>(kArenaX + (kArenaW - 320) / 2),
                     static_cast<float>(kArenaY + (kArenaH - 200) / 2), 320.0f, 200.0f};
}
inline Rectangle pauseSettingsRect() {
    const Rectangle card = pauseCardRect();
    return Rectangle{card.x + 90.0f, card.y + 116.0f, 140.0f, 34.0f};
}

// Debug-only sanity check: called once at startup by Game::Game().
inline void validateLayout() {
#ifndef NDEBUG
    const Rectangle menuRects[] = {menuPlayRect(),       menuSkinsRect(),      menuHighScoresRect(),
                                   menuSettingsRect(),   menuFullscreenRect(), menuQuitRect()};
    checkScreenRects("menu", menuRects, 6);

    const Rectangle tabRects[kSettingTabCount] = {settingsTabRect(0), settingsTabRect(1),
                                                  settingsTabRect(2)};
    checkScreenRects("settings-tabs", tabRects, kSettingTabCount);

    Rectangle settingsRects[kMaxSettingsRowsPerTab + 1];
    for (int i = 0; i < kMaxSettingsRowsPerTab; ++i) {
        settingsRects[i] = settingsRowRect(i);
    }
    settingsRects[kMaxSettingsRowsPerTab] = settingsBackRect();
    checkScreenRects("settings", settingsRects, kMaxSettingsRowsPerTab + 1);

    Rectangle skinRects[kSkinCount + 1];
    for (int i = 0; i < kSkinCount; ++i) {
        skinRects[i] = skinCardRect(i);
    }
    skinRects[kSkinCount] = skinsBackRect();
    checkScreenRects("skins", skinRects, kSkinCount + 1);

    Rectangle highScoreRects[kMaxHighScores + 2];
    for (int i = 0; i < kMaxHighScores; ++i) {
        highScoreRects[i] = highScoreRowRect(i);
    }
    highScoreRects[kMaxHighScores] = highScoresBackRect();
    highScoreRects[kMaxHighScores + 1] = highScoresClearRect();
    checkScreenRects("highscores", highScoreRects, kMaxHighScores + 2);
#endif
}

} // namespace snake::ui
