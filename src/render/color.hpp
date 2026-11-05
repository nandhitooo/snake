#pragma once

#include "game/config.hpp"

namespace snake::render {

inline Color lerpColor(Color a, Color b, float t) {
    const float k = (t < 0.0f) ? 0.0f : (t > 1.0f ? 1.0f : t);
    const auto mix = [k](unsigned char x, unsigned char y) {
        return static_cast<unsigned char>(static_cast<float>(x) +
                                          (static_cast<float>(y) - static_cast<float>(x)) * k);
    };
    return Color{mix(a.r, b.r), mix(a.g, b.g), mix(a.b, b.b), 255};
}

} // namespace snake::render
