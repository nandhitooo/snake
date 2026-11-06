#pragma once

#include "game/config.hpp"

namespace snake::render {

inline Color lerpColor(Color a, Color b, float t) {
    const float k = (t < 0.0f) ? 0.0f : (t > 1.0f ? 1.0f : t);
    const auto mix = [k](unsigned char x, unsigned char y) {
        return static_cast<unsigned char>(static_cast<float>(x) +
                                          (static_cast<float>(y) - static_cast<float>(x)) * k);
    };
    return Color{mix(a.r, b.r), mix(a.g, b.g), mix(a.b, b.b), mix(a.a, b.a)};
}

inline Color withAlpha(Color c, unsigned char a) {
    return Color{c.r, c.g, c.b, a};
}

inline Color tint(Color c, float factor) {
    const auto clampByte = [](float v) {
        if (v < 0.0f) return 0;
        if (v > 255.0f) return 255;
        return static_cast<int>(v);
    };
    return Color{static_cast<unsigned char>(clampByte(static_cast<float>(c.r) * factor)),
                 static_cast<unsigned char>(clampByte(static_cast<float>(c.g) * factor)),
                 static_cast<unsigned char>(clampByte(static_cast<float>(c.b) * factor)),
                 c.a};
}

} // namespace snake::render
