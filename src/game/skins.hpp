#pragma once

#include "game/config.hpp"

#include <cmath>

namespace snake {

enum class SegmentShape { Cube, Sphere, Rounded };
enum class TrailKind { None, Embers, Sparkle, Glow };

// One row per skin: adding a skin means appending here (and bumping kSkinCount).
struct SkinDef {
    const char* name;
    Color head;
    Color body;
    Color tail;
    Color eye;
    Color accent;
    SegmentShape shape;
    TrailKind trail;
    bool crown;
    bool rainbow;
};

inline constexpr int kSkinCount = 6;

inline constexpr SkinDef kSkins[kSkinCount] = {
    {"Classic Green", {130, 225, 140, 255}, {92, 190, 120, 255}, {36, 120, 96, 255},
     {18, 26, 22, 255}, {190, 255, 200, 255}, SegmentShape::Rounded, TrailKind::None, false,
     false},
    {"Neon Cyber", {80, 255, 235, 255}, {60, 200, 255, 255}, {190, 80, 255, 255},
     {10, 20, 30, 255}, {80, 255, 235, 255}, SegmentShape::Cube, TrailKind::Glow, false, false},
    {"Lava", {255, 170, 60, 255}, {240, 90, 40, 255}, {120, 20, 15, 255}, {40, 10, 5, 255},
     {255, 220, 120, 255}, SegmentShape::Rounded, TrailKind::Embers, false, false},
    {"Ice", {220, 245, 255, 255}, {150, 205, 245, 255}, {70, 130, 200, 255}, {20, 40, 70, 255},
     {200, 240, 255, 255}, SegmentShape::Sphere, TrailKind::Sparkle, false, false},
    {"Gold Royal", {255, 215, 90, 255}, {230, 175, 45, 255}, {150, 105, 20, 255},
     {60, 40, 5, 255}, {255, 245, 190, 255}, SegmentShape::Rounded, TrailKind::Sparkle, true,
     false},
    {"Rainbow", {255, 255, 255, 255}, {255, 120, 120, 255}, {120, 120, 255, 255},
     {20, 20, 30, 255}, {255, 255, 255, 255}, SegmentShape::Sphere, TrailKind::Glow, false, true},
};

// Cheap hue cycle used by the Rainbow skin (no assets, no shaders).
inline Color rainbowColor(float t) {
    const auto wave = [](float phase) {
        return static_cast<unsigned char>(125.0f + 125.0f * std::sin(phase));
    };
    return Color{wave(t), wave(t + 2.0943952f), wave(t + 4.1887902f), 255};
}

} // namespace snake
