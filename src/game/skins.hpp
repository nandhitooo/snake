#pragma once

#include "game/config.hpp"

#include <cmath>

namespace snake {

enum class SegmentShape { Cube, Sphere, Rounded };
enum class BodyPattern { Solid, Stripes, Spots, Checker, Pulse, Scales };
enum class Accessory { None, Crown, Halo, Horns, CatEars, Antenna, Flower };
enum class TrailKind {
    None,
    Embers,
    Sparkle,
    Glow,
    Bubbles,
    Petals,
    Stars,
    Smoke,
    Pixels,
    Lightning,
    Snow,
    Hearts
};
enum class EatBurst { Default, Hearts, Stars, Confetti, Sparks, Ripples };

// One row per skin: adding a skin means appending here (and bumping kSkinCount).
// The trailing fields have defaults, so a new skin only writes what it needs.
struct SkinDef {
    const char* name;
    Color head;
    Color body;
    Color tail;
    Color eye;
    Color accent;
    SegmentShape shape;
    TrailKind trail;
    Accessory accessory = Accessory::None;
    bool rainbow = false;
    BodyPattern pattern = BodyPattern::Solid;
    EatBurst burst = EatBurst::Default;
    bool translucent = false; // body drawn through translucent.hpp
    float glowAmount = 0.0f;  // aura strength (0 = none)
};

inline constexpr int kSkinCount = 16;

inline constexpr SkinDef kSkins[kSkinCount] = {
    // --- the original six ---------------------------------------------------
    {"Classic Green", {130, 225, 140, 255}, {92, 190, 120, 255}, {36, 120, 96, 255},
     {18, 26, 22, 255}, {190, 255, 200, 255}, SegmentShape::Rounded, TrailKind::None,
     Accessory::None},
    {"Neon Cyber", {80, 255, 235, 255}, {60, 200, 255, 255}, {190, 80, 255, 255},
     {10, 20, 30, 255}, {80, 255, 235, 255}, SegmentShape::Cube, TrailKind::Glow,
     Accessory::None, false, BodyPattern::Solid, EatBurst::Default, false, 0.8f},
    {"Lava", {255, 170, 60, 255}, {240, 90, 40, 255}, {120, 20, 15, 255}, {40, 10, 5, 255},
     {255, 220, 120, 255}, SegmentShape::Rounded, TrailKind::Embers, Accessory::None, false,
     BodyPattern::Solid, EatBurst::Sparks, false, 0.4f},
    {"Ice", {220, 245, 255, 255}, {150, 205, 245, 255}, {70, 130, 200, 255}, {20, 40, 70, 255},
     {200, 240, 255, 255}, SegmentShape::Sphere, TrailKind::Sparkle, Accessory::None, false,
     BodyPattern::Solid, EatBurst::Default, false, 0.3f},
    {"Gold Royal", {255, 215, 90, 255}, {230, 175, 45, 255}, {150, 105, 20, 255},
     {60, 40, 5, 255}, {255, 245, 190, 255}, SegmentShape::Rounded, TrailKind::Sparkle,
     Accessory::Crown, false, BodyPattern::Solid, EatBurst::Confetti, false, 0.5f},
    {"Rainbow", {255, 255, 255, 255}, {255, 120, 120, 255}, {120, 120, 255, 255},
     {20, 20, 30, 255}, {255, 255, 255, 255}, SegmentShape::Sphere, TrailKind::Glow,
     Accessory::None, true, BodyPattern::Solid, EatBurst::Default, false, 0.6f},

    // --- Phase I: ten more ---------------------------------------------------
    {"Sakura", {255, 185, 210, 255}, {255, 150, 185, 255}, {205, 110, 145, 255},
     {70, 40, 55, 255}, {255, 228, 238, 255}, SegmentShape::Rounded, TrailKind::Petals,
     Accessory::Flower, false, BodyPattern::Spots, EatBurst::Hearts, false, 0.2f},
    {"Galaxy", {155, 125, 255, 255}, {95, 95, 225, 255}, {42, 32, 115, 255},
     {228, 228, 255, 255}, {185, 165, 255, 255}, SegmentShape::Sphere, TrailKind::Stars,
     Accessory::None, false, BodyPattern::Pulse, EatBurst::Stars, false, 1.0f},
    {"Toxic Slime", {175, 255, 95, 255}, {120, 220, 60, 255}, {60, 140, 30, 255},
     {30, 45, 15, 255}, {235, 255, 120, 255}, SegmentShape::Rounded, TrailKind::Bubbles,
     Accessory::None, false, BodyPattern::Spots, EatBurst::Sparks, false, 0.5f},
    {"Ocean", {110, 230, 215, 255}, {60, 170, 200, 255}, {30, 90, 150, 255},
     {15, 40, 60, 255}, {190, 245, 255, 255}, SegmentShape::Rounded, TrailKind::Bubbles,
     Accessory::None, false, BodyPattern::Stripes, EatBurst::Ripples, false, 0.3f},
    {"Candy", {255, 165, 205, 255}, {255, 125, 175, 255}, {230, 90, 145, 255},
     {70, 30, 50, 255}, {255, 255, 255, 255}, SegmentShape::Rounded, TrailKind::Hearts,
     Accessory::CatEars, false, BodyPattern::Stripes, EatBurst::Confetti, false, 0.2f},
    {"Ninja Shadow", {72, 76, 90, 255}, {52, 55, 68, 255}, {30, 32, 42, 255},
     {240, 60, 70, 255}, {200, 50, 60, 255}, SegmentShape::Rounded, TrailKind::Smoke,
     Accessory::None, false, BodyPattern::Solid, EatBurst::Default, false, 0.1f},
    {"Ghost", {228, 242, 255, 255}, {205, 228, 252, 255}, {172, 200, 236, 255},
     {80, 100, 130, 255}, {235, 245, 255, 255}, SegmentShape::Sphere, TrailKind::Smoke,
     Accessory::Halo, false, BodyPattern::Solid, EatBurst::Ripples, true, 0.5f},
    {"Cyber Glitch", {32, 32, 40, 255}, {46, 22, 62, 255}, {20, 20, 30, 255},
     {0, 255, 220, 255}, {255, 45, 200, 255}, SegmentShape::Cube, TrailKind::Pixels,
     Accessory::None, false, BodyPattern::Checker, EatBurst::Sparks, false, 0.6f},
    {"Autumn", {235, 152, 72, 255}, {200, 110, 50, 255}, {120, 60, 30, 255},
     {50, 30, 15, 255}, {255, 205, 125, 255}, SegmentShape::Rounded, TrailKind::Petals,
     Accessory::None, false, BodyPattern::Scales, EatBurst::Confetti, false, 0.2f},
    {"Thunder", {255, 236, 112, 255}, {232, 202, 72, 255}, {150, 130, 40, 255},
     {40, 40, 60, 255}, {200, 232, 255, 255}, SegmentShape::Cube, TrailKind::Lightning,
     Accessory::Horns, false, BodyPattern::Pulse, EatBurst::Sparks, false, 0.8f},
};

// Cheap hue cycle used by the Rainbow skin (no assets, no shaders).
inline Color rainbowColor(float t) {
    const auto wave = [](float phase) {
        return static_cast<unsigned char>(125.0f + 125.0f * std::sin(phase));
    };
    return Color{wave(t), wave(t + 2.0943952f), wave(t + 4.1887902f), 255};
}

} // namespace snake
