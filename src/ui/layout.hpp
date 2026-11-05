#pragma once

#include "game/config.hpp"

namespace snake::ui {

inline Rectangle moveLeftRect() {
    return Rectangle{16.0f, static_cast<float>(kBarY + 16), 52.0f, 48.0f};
}
inline Rectangle moveUpRect() {
    return Rectangle{76.0f, static_cast<float>(kBarY + 16), 52.0f, 48.0f};
}
inline Rectangle moveDownRect() {
    return Rectangle{136.0f, static_cast<float>(kBarY + 16), 52.0f, 48.0f};
}
inline Rectangle moveRightRect() {
    return Rectangle{196.0f, static_cast<float>(kBarY + 16), 52.0f, 48.0f};
}
inline Rectangle boostRect() {
    return Rectangle{268.0f, static_cast<float>(kBarY + 16), 120.0f, 48.0f};
}
inline Rectangle restartRect() {
    return Rectangle{404.0f, static_cast<float>(kBarY + 16), 120.0f, 48.0f};
}
inline Rectangle quitRect() {
    return Rectangle{540.0f, static_cast<float>(kBarY + 16), 100.0f, 48.0f};
}

inline Rectangle menuPlayRect() { return Rectangle{220.0f, 330.0f, 280.0f, 56.0f}; }
inline Rectangle menuFullscreenRect() { return Rectangle{220.0f, 398.0f, 280.0f, 56.0f}; }
inline Rectangle menuQuitRect() { return Rectangle{220.0f, 466.0f, 280.0f, 56.0f}; }

} // namespace snake::ui
