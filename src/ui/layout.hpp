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

inline Rectangle menuPlayRect() { return Rectangle{340.0f, 254.0f, 280.0f, 56.0f}; }
inline Rectangle menuFullscreenRect() { return Rectangle{340.0f, 322.0f, 280.0f, 56.0f}; }
inline Rectangle menuQuitRect() { return Rectangle{340.0f, 390.0f, 280.0f, 56.0f}; }

} // namespace snake::ui
