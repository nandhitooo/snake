#pragma once

#include "game/config.hpp"

namespace snake::ui {

inline Rectangle moveLeftRect() {
    return Rectangle{static_cast<float>(kArenaX + 20), static_cast<float>(kBarY + 8), 44.0f, 44.0f};
}
inline Rectangle moveUpRect() {
    return Rectangle{static_cast<float>(kArenaX + 70), static_cast<float>(kBarY + 8), 44.0f, 44.0f};
}
inline Rectangle moveDownRect() {
    return Rectangle{static_cast<float>(kArenaX + 120), static_cast<float>(kBarY + 8), 44.0f,
                     44.0f};
}
inline Rectangle moveRightRect() {
    return Rectangle{static_cast<float>(kArenaX + 170), static_cast<float>(kBarY + 8), 44.0f,
                     44.0f};
}
inline Rectangle boostRect() {
    return Rectangle{static_cast<float>(kArenaX + 222), static_cast<float>(kBarY + 8), 96.0f,
                     44.0f};
}
inline Rectangle restartRect() {
    return Rectangle{static_cast<float>(kArenaX + 330), static_cast<float>(kBarY + 8), 108.0f,
                     44.0f};
}
inline Rectangle quitRect() {
    return Rectangle{static_cast<float>(kArenaX + 450), static_cast<float>(kBarY + 8), 108.0f,
                     44.0f};
}

inline Rectangle menuPlayRect() { return Rectangle{340.0f, 254.0f, 280.0f, 56.0f}; }
inline Rectangle menuFullscreenRect() { return Rectangle{340.0f, 322.0f, 280.0f, 56.0f}; }
inline Rectangle menuQuitRect() { return Rectangle{340.0f, 390.0f, 280.0f, 56.0f}; }

} // namespace snake::ui
