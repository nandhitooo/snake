#pragma once

#include "game/config.hpp"

namespace snake::ui {

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

// Main menu: five 50 px slots between the motif (y~188) and the hint line.
inline Rectangle menuPlayRect() { return Rectangle{340.0f, 220.0f, 280.0f, 50.0f}; }
inline Rectangle menuSkinsRect() { return Rectangle{340.0f, 274.0f, 280.0f, 50.0f}; }
inline Rectangle menuSettingsRect() { return Rectangle{340.0f, 328.0f, 280.0f, 50.0f}; }
inline Rectangle menuFullscreenRect() { return Rectangle{340.0f, 382.0f, 280.0f, 50.0f}; }
inline Rectangle menuQuitRect() { return Rectangle{340.0f, 436.0f, 280.0f, 50.0f}; }

// Settings screen rows (10) and its back button.
inline Rectangle settingsRowRect(int index) {
    return Rectangle{200.0f, 132.0f + static_cast<float>(index) * 30.0f, 560.0f, 26.0f};
}
inline Rectangle settingsBackRect() { return Rectangle{380.0f, 452.0f, 200.0f, 40.0f}; }

// Skins screen: six cards in a row plus a back button.
inline Rectangle skinCardRect(int index) {
    return Rectangle{66.0f + static_cast<float>(index) * 140.0f, 348.0f, 128.0f, 74.0f};
}
inline Rectangle skinsBackRect() { return Rectangle{400.0f, 442.0f, 160.0f, 38.0f}; }

// Pause card (shared by renderer and input so hit boxes match the drawing).
inline Rectangle pauseCardRect() {
    return Rectangle{static_cast<float>(kArenaX + (kArenaW - 320) / 2),
                     static_cast<float>(kArenaY + (kArenaH - 200) / 2), 320.0f, 200.0f};
}
inline Rectangle pauseSettingsRect() {
    const Rectangle card = pauseCardRect();
    return Rectangle{card.x + 90.0f, card.y + 116.0f, 140.0f, 34.0f};
}

} // namespace snake::ui
