#include "game/game.hpp"
#include "render/color.hpp"
#include "render/snake_head.hpp"
#include "ui/layout.hpp"

#include "rlgl.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace snake {
namespace {

Vector3 vadd(Vector3 a, Vector3 b) { return Vector3{a.x + b.x, a.y + b.y, a.z + b.z}; }

Vector3 vmul(Vector3 a, float s) { return Vector3{a.x * s, a.y * s, a.z * s}; }

// Same blink curve as the player, driven by a free-running clock so each rival
// can be phase-shifted instead of sharing the player's timer.
float blinkAt(float t, float period) {
    const float c = std::fmod(t, period);
    if (c < 0.0f || c >= kBlinkDuration) {
        return 0.0f;
    }
    if (c < kBlinkCloseTime) {
        return c / kBlinkCloseTime;
    }
    return 1.0f - (c - kBlinkCloseTime) / std::max(1e-4f, kBlinkDuration - kBlinkCloseTime);
}

Vector3 vnorm(Vector3 v) {
    const float len = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    if (len < 1e-6f) {
        return Vector3{1.0f, 0.0f, 0.0f};
    }
    return Vector3{v.x / len, v.y / len, v.z / len};
}

// World position of segment i, interpolated between the previous and current tick.
// The maths lives in Game so the camera can follow the very same head.
Vector3 segmentWorld(const Game& g, size_t i, float y) { return g.visualSegmentWorld(i, y); }

void drawKeycap(float x, float y, const char* key, const char* action) {
    const int keyW = MeasureText(key, 11) + 10;
    const Rectangle kr{x, y, static_cast<float>(keyW), 16.0f};
    DrawRectangleRounded(kr, 0.35f, 4, Color{36, 42, 56, 255});
    DrawRectangleRoundedLinesEx(kr, 0.35f, 4, 1.0f, Color{75, 85, 112, 255});
    DrawText(key, static_cast<int>(x + 5), static_cast<int>(y + 3), 11, Color{235, 240, 250, 255});
    DrawText(action, static_cast<int>(x + keyW + 5), static_cast<int>(y + 3), 11,
             Color{150, 156, 172, 255});
}

const char* cameraLabel(CameraMode mode) {
    switch (mode) {
    case CameraMode::Top:
        return "TOP-DOWN";
    case CameraMode::Third:
        return "THIRD-PERSON";
    case CameraMode::Iso:
    default:
        return "ISOMETRIC";
    }
}

// --- 3D scene pieces -------------------------------------------------------

void drawArenaFloor() {
    // Slab under the floor gives the arena some thickness in perspective.
    DrawCubeV(Vector3{0.0f, -0.35f, 0.0f}, Vector3{33.0f, 0.7f, 19.0f}, Color{13, 15, 22, 255});

    DrawPlane(Vector3{0.0f, 0.0f, 0.0f},
              Vector2{static_cast<float>(kCols), static_cast<float>(kRows)}, kArenaBg);

    const Color grid{34, 38, 52, 255};
    const float halfW = static_cast<float>(kCols) * 0.5f;
    const float halfH = static_cast<float>(kRows) * 0.5f;
    for (int x = 0; x <= kCols; ++x) {
        const float wx = static_cast<float>(x) - halfW;
        DrawLine3D(Vector3{wx, 0.012f, -halfH}, Vector3{wx, 0.012f, halfH}, grid);
    }
    for (int y = 0; y <= kRows; ++y) {
        const float wz = static_cast<float>(y) - halfH;
        DrawLine3D(Vector3{-halfW, 0.012f, wz}, Vector3{halfW, 0.012f, wz}, grid);
    }

    // Low border walls around the arena.
    const Color wall{88, 94, 116, 255};
    DrawCubeV(Vector3{0.0f, 0.22f, -halfH - 0.18f}, Vector3{static_cast<float>(kCols) + 0.7f, 0.44f, 0.36f}, wall);
    DrawCubeV(Vector3{0.0f, 0.22f, halfH + 0.18f}, Vector3{static_cast<float>(kCols) + 0.7f, 0.44f, 0.36f}, wall);
    DrawCubeV(Vector3{-halfW - 0.18f, 0.22f, 0.0f}, Vector3{0.36f, 0.44f, static_cast<float>(kRows) + 0.7f}, wall);
    DrawCubeV(Vector3{halfW + 0.18f, 0.22f, 0.0f}, Vector3{0.36f, 0.44f, static_cast<float>(kRows) + 0.7f}, wall);
}

void drawApple(const Game& g) {
    const Vector3 base = cellToWorld(g.food, 0.0f);
    const float bob = g.animations ? 0.06f * std::sin(g.animTime * 4.5f) : 0.0f;

    // Pop-in: quick overshoot that settles in ~0.25 s.
    float pop = 1.0f;
    if (g.animations) {
        const float x = std::clamp(g.foodAge / 0.25f, 0.0f, 1.0f);
        pop = 1.0f + 0.45f * std::sin(x * 3.141593f) * (1.0f - x);
    }
    const Vector3 c{base.x, 0.55f * pop + bob, base.z};

    // Contact shadow keeps the apple visually grounded.
    DrawPlane(Vector3{base.x, 0.015f, base.z}, Vector2{0.7f, 0.7f}, Color{0, 0, 0, 70});

    DrawSphereEx(c, 0.4f * pop, 10, 12, kFood);
    DrawSphereEx(vadd(c, vmul(Vector3{-0.13f, 0.14f, 0.12f}, pop)), 0.085f * pop, 6, 8,
                 Color{255, 168, 168, 230});
    DrawCylinder(Vector3{c.x, c.y + 0.34f * pop, c.z}, 0.035f, 0.05f, 0.22f * pop, 6, kFoodStem);

    const float ang = g.animations ? g.animTime * 1.9f : 0.0f;
    const Vector3 leaf{c.x + std::cos(ang) * 0.17f, c.y + 0.53f * pop,
                       c.z + std::sin(ang) * 0.17f};
    DrawCubeV(leaf, Vector3{0.24f * pop, 0.045f, 0.12f * pop}, kFoodLeaf);
}

void drawSpecialFood(const Game& g) {
    const Vector3 base = cellToWorld(g.specialFood, 0.0f);
    const float pulse = g.animations ? 0.5f + 0.5f * std::sin(g.animTime * 6.0f) : 0.5f;
    float pop = 1.0f;
    if (g.animations) {
        const float x = std::clamp(g.specialAge / 0.3f, 0.0f, 1.0f);
        pop = 1.0f + 0.5f * std::sin(x * 3.141593f) * (1.0f - x);
    }
    const Vector3 c{base.x, (0.68f + (g.animations ? 0.05f * std::sin(g.animTime * 3.4f) : 0.0f)) * 1.0f,
                    base.z};

    DrawPlane(Vector3{base.x, 0.015f, base.z}, Vector2{0.9f, 0.9f}, Color{0, 0, 0, 80});

    // Halo shell + orbiting gold motes.
    DrawSphereEx(c, (0.6f + 0.06f * pulse) * pop, 8, 10,
                 Color{kSpecialHalo.r, kSpecialHalo.g, kSpecialHalo.b, 40});
    for (int i = 0; i < 8; ++i) {
        const float ang = g.animTime * 1.7f + static_cast<float>(i) * 0.785398f;
        const Vector3 mote{c.x + std::cos(ang) * 0.58f * pop, c.y + 0.16f * std::sin(ang * 2.0f),
                           c.z + std::sin(ang) * 0.58f * pop};
        DrawSphereEx(mote, 0.07f * pop, 6, 8, kSpecialHalo);
    }
    DrawSphereEx(c, 0.34f * pop, 10, 12, kSpecial);
    DrawSphereEx(vadd(c, vmul(Vector3{-0.09f, 0.11f, 0.09f}, pop)), 0.075f * pop, 6, 8,
                 Color{245, 215, 255, 220});
}

void drawWarnings(const Game& g) {
    for (const Warning& w : g.warnings) {
        const Vector3 base = cellToWorld(w.cell, 0.0f);
        const float urgency = 1.0f - std::max(0.0f, w.timer / kWarningTime);
        const float blink = 0.5f + 0.5f * std::sin(g.animTime * (12.0f + urgency * 20.0f));

        DrawPlane(Vector3{base.x, 0.02f, base.z}, Vector2{0.82f, 0.82f},
                  Color{kWarning.r, kWarning.g, kWarning.b,
                        static_cast<unsigned char>(45.0f + 110.0f * blink)});

        const float h = 0.18f + 0.5f * urgency;
        DrawCubeWiresV(Vector3{base.x, h * 0.5f, base.z}, Vector3{0.7f, h, 0.7f},
                       Color{kWarning.r, kWarning.g, kWarning.b,
                             static_cast<unsigned char>(150.0f + 100.0f * blink)});
    }
}

void drawObstacles(const Game& g) {
    for (const Obstacle& o : g.obstacles) {
        const float age = kObstacleLife - o.life;
        const float rise = std::clamp(age / 0.3f, 0.0f, 1.0f);
        float fade = 1.0f;
        if (o.life < kObstacleFade) {
            fade = std::max(0.0f, o.life / kObstacleFade);
        }
        const unsigned char a = static_cast<unsigned char>(255.0f * fade);
        const unsigned char edgeA = static_cast<unsigned char>(200.0f * fade);
        // Shrink into the floor during the fade-out window.
        const float sink = (o.life < kObstacleFade) ? std::max(0.15f, o.life / kObstacleFade) : 1.0f;
        const float h = 0.95f * rise * sink;
        const Vector3 base = cellToWorld(o.cell, 0.0f);
        const Vector3 center{base.x, h * 0.5f, base.z};

        DrawPlane(Vector3{base.x, 0.014f, base.z}, Vector2{0.9f, 0.9f}, Color{0, 0, 0, 70});
        DrawCubeV(center, Vector3{0.86f, h, 0.86f}, Color{kObstacle.r, kObstacle.g, kObstacle.b, a});
        DrawCubeWiresV(center, Vector3{0.86f, h, 0.86f},
                       Color{60, 68, 88, edgeA});

        // Hazard cross on the top face.
        DrawLine3D(Vector3{base.x - 0.24f, h + 0.01f, base.z - 0.24f},
                   Vector3{base.x + 0.24f, h + 0.01f, base.z + 0.24f},
                   Color{150, 160, 185, edgeA});
        DrawLine3D(Vector3{base.x - 0.24f, h + 0.01f, base.z + 0.24f},
                   Vector3{base.x + 0.24f, h + 0.01f, base.z - 0.24f},
                   Color{150, 160, 185, edgeA});
    }
}

// --- Shared snake head ----------------------------------------------------
//
// The in-game snake and the skins-preview turntable both render through
// drawSnakeHead(), so the skull, mouth, eyes and crown exist in one place only.
// Every size below is expressed at kHeadRefRadius and scaled by `s`.

// Jaw, teeth, throat and tongue. Primitive-only (no assets) and nothing at all
// while the mouth is shut.
void drawMouthGeometry(const HeadPose& p, float headRadius) {
    const float open = std::clamp(p.mouthOpen, 0.0f, 1.0f);
    if (!p.animate || open <= 0.01f) {
        return;
    }
    const float s = headRadius / kHeadRefRadius;
    const Vector3 fwd = p.fwd;
    const Vector3 right{-fwd.z, 0.0f, fwd.x};
    const Vector3 up{0.0f, 1.0f, 0.0f};
    const Vector3 head = p.pos;

    const float angle = kMouthMaxAngle * open;
    const float ca = std::cos(angle);
    const float sa = std::sin(angle);
    const Color jaw = render::tint(p.head, 0.78f);

    // Throat: dark red, visible only through the open jaw.
    const Vector3 inner =
        vadd(head, vadd(vmul(fwd, 0.16f * s), Vector3{0.0f, -0.06f * s, 0.0f}));
    DrawSphereEx(inner, (0.24f * open + 0.05f) * s, 8, 10, kMouthInner);

    // Lower jaw hinges behind and below the head, then rotates down by `angle`.
    const Vector3 hinge =
        vadd(head, vadd(vmul(fwd, -kMouthHingeBack * s), Vector3{0.0f, -kMouthHingeDown * s, 0.0f}));
    const Vector3 jawDir = vadd(vmul(fwd, ca), vmul(up, -sa));
    const Vector3 jawEnd = vadd(hinge, vmul(jawDir, kMouthJawLength * s));
    DrawCylinderEx(hinge, jawEnd, 0.17f * s, 0.09f * s, 6, jaw);
    DrawSphereEx(jawEnd, 0.09f * s, 6, 8, jaw);

    // Snout: the upper jaw lifts a little, so the open mouth reads as a yawn.
    const float lift = kMouthSnoutLift * open;
    const Vector3 snoutRoot =
        vadd(head, vadd(vmul(fwd, 0.1f * s), Vector3{0.0f, -0.06f * s, 0.0f}));
    const Vector3 snoutDir = vadd(vmul(fwd, std::cos(lift)), vmul(up, std::sin(lift)));
    const Vector3 snoutEnd = vadd(snoutRoot, vmul(snoutDir, 0.36f * s));
    DrawCylinderEx(snoutRoot, snoutEnd, 0.16f * s, 0.09f * s, 6, render::tint(p.head, 0.92f));

    // Four fangs (two upper, two lower), scaled in as the jaw opens.
    const float tooth = std::min(1.0f, open * 1.6f);
    const Vector3 jawUp = vadd(vmul(up, ca), vmul(fwd, sa));
    for (int side = -1; side <= 1; side += 2) {
        const Vector3 upperBase =
            vadd(vadd(snoutEnd, vmul(right, 0.08f * s * static_cast<float>(side))),
                 vmul(up, -0.02f * s));
        const Vector3 upperTip =
            vadd(upperBase, vadd(vmul(up, -0.15f * s * tooth), vmul(fwd, 0.03f * s)));
        DrawCylinderEx(upperBase, upperTip, 0.042f * s, 0.0f, 5, kMouthTeeth);

        const Vector3 lowerBase =
            vadd(vadd(hinge, vmul(jawDir, 0.4f * s)), vmul(right, 0.09f * s * static_cast<float>(side)));
        const Vector3 lowerTip = vadd(lowerBase, vmul(jawUp, 0.14f * s * tooth));
        DrawCylinderEx(lowerBase, lowerTip, 0.04f * s, 0.0f, 5, kMouthTeeth);
    }

    // A small tongue inside the open mouth (the idle flick is hidden meanwhile).
    if (open > 0.2f) {
        const Vector3 tBase = vadd(inner, vmul(fwd, 0.04f * s));
        const Vector3 tTip =
            vadd(tBase, vadd(vmul(fwd, 0.24f * s * open), vmul(jawDir, 0.08f * s)));
        DrawCylinderEx(tBase, tTip, 0.05f * s, 0.025f * s, 5, Color{214, 62, 84, 255});
    }
}

// The idle forked tongue flick.
void drawHeadTongue(const HeadPose& p) {
    if (!p.animate || p.mouthOpen > 0.2f) {
        return;
    }
    const float cycle = std::fmod(p.time, 2.4f);
    if (cycle >= 0.35f) {
        return;
    }
    const float s = p.scale;
    const Vector3 fwd = p.fwd;
    const Vector3 right{-fwd.z, 0.0f, fwd.x};
    const float ext = (cycle < 0.17f) ? (cycle / 0.17f) : ((0.35f - cycle) / 0.18f);
    const Vector3 base =
        vadd(p.pos, vadd(vmul(fwd, kHeadTongueFwd * s), Vector3{0.0f, -kHeadTongueDrop * s, 0.0f}));
    const Vector3 tip = vadd(base, vmul(fwd, 0.34f * s * ext));
    const Color tongue{230, 60, 85, 255};
    DrawCylinderEx(base, tip, 0.035f * s, 0.028f * s, 5, tongue);
    const Vector3 forkA = vadd(tip, vmul(vadd(fwd, right), 0.1f * s));
    const Vector3 forkB = vadd(tip, vmul(vadd(fwd, vmul(right, -1.0f)), 0.1f * s));
    DrawCylinderEx(tip, forkA, 0.025f * s, 0.018f * s, 4, tongue);
    DrawCylinderEx(tip, forkB, 0.025f * s, 0.018f * s, 4, tongue);
}

// Eyes: white, pupil, glint. Blink squashes them vertically around the eye
// centre with an rlgl matrix; happy eyes become "^ ^" strokes instead.
void drawHeadEyes(const HeadPose& p) {
    const float s = p.scale;
    const Vector3 fwd = p.fwd;
    const Vector3 right{-fwd.z, 0.0f, fwd.x};
    const Vector3 up{0.0f, 1.0f, 0.0f};
    const Color white{242, 246, 252, 255};
    const float blink = std::clamp(p.blink, 0.0f, 1.0f);
    const float squash = 1.0f - 0.9f * blink;
    const float grow = 1.0f + kHeadEyePupilGrow * std::clamp(p.mouthOpen, 0.0f, 1.0f);

    for (int side = -1; side <= 1; side += 2) {
        const Vector3 eye =
            vadd(p.pos, vadd(vmul(fwd, kHeadEyeFwd * s),
                             vadd(vmul(right, kHeadEyeSpread * s * static_cast<float>(side)),
                                  vmul(up, kHeadEyeHeight * s))));
        // The eye centre sits inside the skull: strokes that replace the eyeball
        // (happy carets, closed lash) have to be pushed onto its front surface.
        const Vector3 facePush = vmul(fwd, kHeadEyeWhite * 0.85f * s);

        if (p.happy) {
            const float half = kHeadEyeWhite * s * 0.75f;
            const Vector3 a = vadd(vadd(eye, facePush), vmul(right, -half));
            const Vector3 b = vadd(vadd(eye, facePush), vmul(up, half * 0.9f));
            const Vector3 c = vadd(vadd(eye, facePush), vmul(right, half));
            DrawCylinderEx(a, b, kHeadEyeHappyThick * s, kHeadEyeHappyThick * s, 5, p.eye);
            DrawCylinderEx(b, c, kHeadEyeHappyThick * s, kHeadEyeHappyThick * s, 5, p.eye);
            continue;
        }

        const Vector3 look = vadd(vmul(right, p.gazeRight * kHeadPupilShift * s),
                                  vmul(fwd, p.gazeFwd * kHeadPupilShift * s));
        rlPushMatrix();
        rlTranslatef(eye.x, eye.y, eye.z);
        rlScalef(1.0f, squash, 1.0f);
        DrawSphereEx(Vector3{0.0f, 0.0f, 0.0f}, kHeadEyeWhite * s, 6, 8, white);
        const Vector3 pupil = vadd(vmul(fwd, kHeadEyePupilFwd * s), look);
        DrawSphereEx(pupil, kHeadEyePupil * s * grow, 6, 8, p.eye);
        if (kHeadEyeShine > 0.0f) {
            const Vector3 shine = vadd(pupil, vadd(vmul(right, kHeadEyeShineOffset * s),
                                                   vmul(up, kHeadEyeShineOffset * s)));
            DrawSphereEx(shine, kHeadEyeShine * s * grow, 5, 6, WHITE);
        }
        rlPopMatrix();

        // A closed eye still needs a crisp lash line once it is nearly shut.
        if (blink > 0.7f) {
            const Vector3 a = vadd(vadd(eye, facePush), vmul(right, -kHeadBlinkLine * s));
            const Vector3 b = vadd(vadd(eye, facePush), vmul(right, kHeadBlinkLine * s));
            DrawCylinderEx(a, b, kHeadBlinkThick * s, kHeadBlinkThick * s, 5, p.eye);
        }
    }
}

// Gold-royal crown, sitting on top of the (possibly larger) skull.
void drawHeadCrown(const HeadPose& p, float headRadius) {
    const float s = headRadius / kHeadRefRadius;
    const Vector3 base{p.pos.x, p.pos.y + 0.5f * s, p.pos.z};
    DrawCubeV(base, Vector3{0.36f * s, 0.06f * s, 0.36f * s}, kGold);
    DrawCubeV(Vector3{base.x - 0.12f * s, base.y + 0.1f * s, base.z},
              Vector3{0.07f * s, 0.18f * s, 0.07f * s}, kGold);
    DrawCubeV(Vector3{base.x, base.y + 0.13f * s, base.z},
              Vector3{0.07f * s, 0.24f * s, 0.07f * s}, kGold);
    DrawCubeV(Vector3{base.x + 0.12f * s, base.y + 0.1f * s, base.z},
              Vector3{0.07f * s, 0.18f * s, 0.07f * s}, kGold);
}

// Soft pink cheeks, flattened along the vertical axis.
void drawHeadBlush(const HeadPose& p) {
    const float s = p.scale;
    const Vector3 fwd = p.fwd;
    const Vector3 right{-fwd.z, 0.0f, fwd.x};
    const Vector3 up{0.0f, 1.0f, 0.0f};
    const Color blush = render::withAlpha(
        render::lerpColor(p.head, kHeadBlush, kHeadBlushMix), kHeadBlushAlpha);

    for (int side = -1; side <= 1; side += 2) {
        const Vector3 centre =
            vadd(p.pos, vadd(vmul(fwd, kHeadBlushFwd * s),
                             vadd(vmul(right, kHeadBlushSpread * s * static_cast<float>(side)),
                                  vmul(up, kHeadBlushHeight * s))));
        rlPushMatrix();
        rlTranslatef(centre.x, centre.y, centre.z);
        rlScalef(1.0f, kHeadBlushFlat, 1.0f);
        DrawSphereEx(Vector3{0.0f, 0.0f, 0.0f}, kHeadBlushSize * s, 8, 10, blush);
        rlPopMatrix();
    }
}

// A small arc of dark cylinders lying on the skull, below the eyes. Skipped while
// the jaw or the happy eyes are showing.
void drawHeadSmile(const HeadPose& p) {
    if (p.mouthOpen > 0.1f || p.happy) {
        return;
    }
    const float s = p.scale;
    const Vector3 fwd = p.fwd;
    const Vector3 right{-fwd.z, 0.0f, fwd.x};
    const Vector3 down{0.0f, -1.0f, 0.0f};
    const Vector3 centreDir = vnorm(vadd(vmul(fwd, kHeadSmileFwd), vmul(down, kHeadSmileDown)));

    Vector3 prev{0.0f, 0.0f, 0.0f};
    for (int i = 0; i <= 3; ++i) {
        const float t = static_cast<float>(i) / 1.5f - 1.0f; // -1, -1/3, 1/3, 1
        const float ang = t * kHeadSmileHalfWidth;
        Vector3 d = vadd(vmul(centreDir, std::cos(ang)), vmul(right, std::sin(ang)));
        d = vnorm(vadd(d, vmul(down, kHeadSmileDroop * (1.0f - t * t))));
        const Vector3 pt = vadd(p.pos, vmul(d, kHeadRadius * s * 1.01f));
        if (i > 0) {
            DrawCylinderEx(prev, pt, kHeadSmileThick * s, kHeadSmileThick * s, 5, kHeadSmile);
        }
        prev = pt;
    }
}

void drawSnakeHeadBody(const HeadPose& p) {
    const float radius = kHeadRadius * p.scale;
    // Bite pulse: the skull throbs briefly as the jaw snaps shut.
    float pulse = 1.0f;
    if (p.animate && p.biteT >= 0.0f && p.biteT < kMouthBitePulse) {
        pulse = 1.0f + kHeadBitePulseScale *
                           std::sin(p.biteT / kMouthBitePulse * 3.141593f);
    }
    const float headRadius = radius * pulse;

    DrawSphereEx(p.pos, headRadius, 12, 14, p.head);
    drawMouthGeometry(p, headRadius);
    if (kHeadBlushSize > 0.0f) {
        drawHeadBlush(p);
    }
    drawHeadSmile(p);
    drawHeadEyes(p);
    drawHeadTongue(p);
    if (p.crown) {
        drawHeadCrown(p, headRadius);
    }
}

// Food that the logic tick already ate: it shrinks and slides into the mouth
// until the drawn head arrives (progress reaches 1 on the next tick).
void drawPendingBites(const Game& g) {
    for (const PendingBite& bite : g.pendingBites) {
        const float t = std::clamp(bite.progress, 0.0f, 1.0f);
        const float pull = t * t * (3.0f - 2.0f * t);
        const Vector3 head = g.visualHeadWorld(0.55f);
        const Vector3 mouth = vadd(head, vmul(dirToWorld(g.dir), 0.3f));
        const Vector3 pos{bite.pos.x + (mouth.x - bite.pos.x) * pull * 0.85f,
                          bite.pos.y + (mouth.y - bite.pos.y) * pull * 0.85f,
                          bite.pos.z + (mouth.z - bite.pos.z) * pull * 0.85f};
        const float scale = std::max(0.15f, 1.0f - 0.75f * pull);
        const float radius = (bite.special ? 0.34f : 0.4f) * scale;

        DrawSphereEx(pos, radius, 10, 12, bite.color);
        if (!bite.special) {
            DrawCylinder(Vector3{pos.x, pos.y + radius * 0.85f, pos.z}, 0.03f, 0.045f,
                         0.18f * scale, 6, kFoodStem);
        } else {
            DrawSphereEx(pos, radius * 1.35f, 6, 8,
                         render::withAlpha(kSpecialHalo, 45));
        }
    }
}

void drawSnakeBody(const Game& g) {
    const size_t n = g.snake.size();
    if (n == 0) {
        return;
    }
    const bool anim = g.animations;
    const float baseY = 0.45f;
    const SkinDef& sk = g.skin();
    const bool glowSkin = (sk.trail == TrailKind::Glow);
    const Vector3 headFwd = dirToWorld(g.dir);
    const Vector3 side{-headFwd.z, 0.0f, headFwd.x};

    // Swallow ripple: a bump that travels head -> tail after eating.
    const auto swallowBump = [&](size_t i) {
        if (!anim || g.swallowClock < 0.0f || n < 2) {
            return 1.0f;
        }
        const float t = std::clamp(g.swallowClock / 0.7f, 0.0f, 1.0f);
        const float pos = t * static_cast<float>(n - 1);
        const float d = std::fabs(static_cast<float>(i) - pos);
        return (d < 1.6f) ? 1.0f + 0.35f * (1.0f - d / 1.6f) : 1.0f;
    };

    // Chibi taper: fat behind the head, slim at the tail, tiny tip.
    const auto radiusAt = [&](size_t i) {
        if (n > 2 && i + 1 == n) {
            return kHeadTailRadius;
        }
        if (n <= 2) {
            return kHeadBodyNear;
        }
        const float t = static_cast<float>(i) / static_cast<float>(n - 2);
        return kHeadBodyNear + (kHeadBodyFar - kHeadBodyNear) * std::clamp(t, 0.0f, 1.0f);
    };

    // Interpolated segment position plus body wave and the tail wag.
    const auto bodyPoint = [&](size_t i) {
        const float wave =
            anim ? kHeadWaveAmp * std::sin(g.animTime * 5.0f - static_cast<float>(i) * 0.55f) : 0.0f;
        Vector3 p = segmentWorld(g, i, baseY + wave);
        if (anim && n > 3 && i + 3 >= n) {
            // The last three segments wag, growing toward the tip.
            const float w = static_cast<float>(i - (n - 3) + 1) / 3.0f;
            const float sway =
                kHeadTailWag * w * std::sin(g.animTime * 7.0f + static_cast<float>(i) * 0.5f);
            p.x += side.x * sway;
            p.z += side.z * sway;
        }
        return p;
    };

    // Round connectors first so the segment caps always sit on top of them.
    for (size_t i = 1; i < n; ++i) {
        const Vector3 a = bodyPoint(i - 1);
        const Vector3 b = bodyPoint(i);
        const float dx = std::fabs(a.x - b.x);
        const float dz = std::fabs(a.z - b.z);
        if (dx > 1.05f || dz > 1.05f) {
            continue; // wrapped across the arena
        }
        const float linkRadius = std::min(radiusAt(i - 1), radiusAt(i)) * 0.92f;
        Color link = g.snakeColorAt(i, n);
        if (sk.shape == SegmentShape::Cube && (i % 2) == 0) {
            link = render::tint(link, kHeadCubeTint);
        }
        DrawCylinderEx(a, b, linkRadius, linkRadius, 8, link);
    }

    for (size_t i = n; i-- > 0;) {
        const float bob = (anim && i == 0) ? 0.03f * std::sin(g.animTime * 6.0f) : 0.0f;
        const float bump = swallowBump(i);
        const Vector3 p = bodyPoint(i);
        const Color col = g.snakeColorAt(i, n);
        const float r = radiusAt(i);

        if (glowSkin) {
            DrawSphereEx(p, r * 1.45f * bump, 6, 8, render::withAlpha(sk.accent, 36));
        }

        if (i == 0) {
            // Head bobs and leans a little as it moves.
            const float sway = anim ? 0.05f * std::sin(g.animTime * 2.2f) : 0.0f;
            HeadPose pose;
            pose.pos = Vector3{p.x + sway, p.y + 0.1f + bob, p.z};
            pose.fwd = headFwd;
            pose.scale = bump;
            pose.time = g.animTime;
            pose.mouthOpen = g.mouthOpen;
            pose.biteT = g.biteT;
            pose.animate = anim;
            pose.head = g.headColor();
            pose.eye = g.eyeColor();
            pose.crown = sk.crown;
            const Vector2 gaze = g.headGaze();
            pose.gazeRight = gaze.x;
            pose.gazeFwd = gaze.y;
            pose.blink = g.blinkProgress();
            pose.happy = g.eyesHappy();
            drawSnakeHeadBody(pose);
        } else {
            // Every skin is round now; Cube skins keep their identity with an
            // alternating band instead of a hard-edged box.
            Color seg = col;
            if (sk.shape == SegmentShape::Cube && (i % 2) == 1) {
                seg = render::tint(col, kHeadCubeTint);
            }
            DrawSphereEx(p, r * bump, 8, 10, seg);
        }
    }
}

void drawGhosts(const Game& g) {
    for (const GhostHead& ghost : g.ghosts) {
        g.drawGhostHead(ghost);
    }
}

void drawParticles3D(const Game& g) {
    for (const WorldParticle& p : g.particles) {
        const float ratio = (p.maxLife > 0.0f) ? std::clamp(p.life / p.maxLife, 0.0f, 1.0f) : 0.0f;
        const Color col = render::withAlpha(p.color, static_cast<unsigned char>(
                                                   static_cast<float>(p.color.a) * ratio));
        const float s = p.size * (0.4f + 0.6f * ratio);
        DrawCubeV(p.pos, Vector3{s, s, s}, col);
    }
}

Vector3 aiSegmentWorld(const AiSnake& ai, size_t i, float y, float alpha) {
    if (i >= ai.body.size()) {
        return Vector3{0.0f, y, 0.0f};
    }
    Vector3 cur = cellToWorld(ai.body[i], y);
    if (ai.prevBody.size() == ai.body.size() && i < ai.prevBody.size() && alpha > 0.0f) {
        const Vector3 prev = cellToWorld(ai.prevBody[i], y);
        if (std::fabs(cur.x - prev.x) <= 1.01f && std::fabs(cur.z - prev.z) <= 1.01f) {
            cur.x = prev.x + (cur.x - prev.x) * alpha;
            cur.z = prev.z + (cur.z - prev.z) * alpha;
        }
    }
    return cur;
}

void drawAiWarnings(const Game& g) {
    for (const AiSpawnWarning& w : g.aiWarnings) {
        const Vector3 base = cellToWorld(w.cell, 0.0f);
        const float blink = 0.5f + 0.5f * std::sin(g.animTime * 11.0f);
        DrawPlane(Vector3{base.x, 0.02f, base.z}, Vector2{0.9f, 0.9f},
                  Color{150, 95, 225, static_cast<unsigned char>(45.0f + 110.0f * blink)});
        DrawCubeWiresV(Vector3{base.x, 0.35f, base.z}, Vector3{0.7f, 0.7f, 0.7f},
                       Color{190, 140, 255, static_cast<unsigned char>(150.0f + 100.0f * blink)});
    }
}

// Rivals are violet; a green wireframe means the player can eat them, red means
// touching them is fatal. The hint is recomputed every frame from both lengths.
void drawAiSnakes(const Game& g) {
    const Color rivalBody{150, 95, 225, 255};
    const Color rivalTail{70, 40, 120, 255};

    for (const AiSnake& ai : g.ai) {
        if (ai.body.empty()) {
            continue;
        }
        const float appear = g.animations ? std::clamp(ai.spawnT, 0.0f, 1.0f) : 1.0f;
        const float fade = ai.dying ? std::clamp(ai.deathT / 0.35f, 0.0f, 1.0f) : 1.0f;
        const float scale = std::max(0.05f, appear * fade);
        const bool edible = ai.body.size() < g.snake.size();
        const Color hint = edible ? Color{110, 240, 140, 225} : Color{245, 90, 90, 225};
        const size_t n = ai.body.size();

        for (size_t i = n; i-- > 0;) {
            const float wave =
                g.animations ? 0.04f * std::sin(g.animTime * 5.0f - static_cast<float>(i) * 0.5f)
                             : 0.0f;
            const Vector3 p = aiSegmentWorld(ai, i, 0.42f + wave, g.tickAlpha);
            const float t = (n > 1) ? static_cast<float>(i) / static_cast<float>(n - 1) : 0.0f;
            const Color col = render::lerpColor(rivalBody, rivalTail, t);

            if (i == 0) {
                const Vector3 headPos{p.x, p.y + 0.05f, p.z};
                DrawSphereEx(headPos, 0.44f * scale, 8, 10, rivalBody);
                DrawSphereWires(headPos, 0.47f * scale, 6, 8, hint);

                // Same eyes as the player, scaled down and blinking out of phase.
                HeadPose face;
                face.pos = headPos;
                face.fwd = dirToWorld(ai.dir);
                face.scale = 0.8f * scale;
                face.head = rivalBody;
                face.eye = Color{34, 22, 52, 255};
                face.animate = g.animations;
                face.blink =
                    g.animations ? blinkAt(g.animTime + ai.blinkPhase, kAiBlinkPeriod) : 0.0f;
                drawHeadEyes(face);
            } else {
                DrawCubeV(p, Vector3{0.74f * scale, 0.6f * scale, 0.74f * scale}, col);
                DrawCubeWiresV(p, Vector3{0.78f * scale, 0.64f * scale, 0.78f * scale}, hint);
            }
        }
    }
}

} // namespace

// Public entry point: the gameplay snake and the skins preview both call this.
void drawSnakeHead(const HeadPose& pose) { drawSnakeHeadBody(pose); }

void Game::drawGhostHead(const GhostHead& g) const {
    const float fade = (g.maxLife > 0.0f) ? (g.life / g.maxLife) : 0.0f;
    const unsigned char alpha = static_cast<unsigned char>(165.0f * fade);
    const Vector3 head = cellToWorld(g.cell, 0.6f);
    const Color body = render::withAlpha(headColor(), alpha);
    DrawSphereEx(head, 0.46f, 8, 10, body);

    const Vector3 fwd = dirToWorld(g.dir);
    const Vector3 right{-fwd.z, 0.0f, fwd.x};
    for (int side = -1; side <= 1; side += 2) {
        const Vector3 eye = vadd(vadd(head, vmul(fwd, 0.28f)),
                                 vadd(vmul(right, 0.18f * static_cast<float>(side)),
                                      Vector3{0.0f, 0.14f, 0.0f}));
        DrawSphereEx(eye, 0.09f, 6, 8, Color{235, 242, 250, alpha});
        DrawSphereEx(vadd(eye, vmul(fwd, 0.06f)), 0.045f, 5, 6, Color{20, 24, 30, alpha});
    }
}

void Game::draw() const {
    // Clear color AND depth. Drawing a full-screen background does not touch the
    // depth buffer, and the canvas render target's depth is undefined, so without
    // this every 3D fragment fails the depth test and the whole arena/snake
    // silently disappears (2D HUD still draws, which hides the cause).
    ClearBackground(kBg);

    // Backdrop for the whole virtual canvas.
    DrawRectangleGradientV(0, 0, kWinW, kWinH, Color{13, 15, 22, 255}, Color{8, 9, 13, 255});
    for (const Particle& p : ambientParticles) {
        DrawCircleV(p.pos, p.size, p.color);
    }

    if (mode == Mode::Settings) {
        drawSettings();
        if (transitionT > 0.0f) {
            DrawRectangle(0, 0, kWinW, kWinH,
                          Color{8, 9, 14, static_cast<unsigned char>(255.0f * transitionT)});
        }
        return;
    }

    if (mode == Mode::Skins) {
        drawSkins();
        if (transitionT > 0.0f) {
            DrawRectangle(0, 0, kWinW, kWinH,
                          Color{8, 9, 14, static_cast<unsigned char>(255.0f * transitionT)});
        }
        return;
    }

    if (mode == Mode::Menu) {
        drawMenu();
        if (transitionT > 0.0f) {
            DrawRectangle(0, 0, kWinW, kWinH,
                          Color{8, 9, 14, static_cast<unsigned char>(255.0f * transitionT)});
        }
        return;
    }

    // Screen shake (2D chrome) and a small camera nudge (3D scene).
    float shakeX = 0.0f;
    float shakeY = 0.0f;
    if (shakeTimer > 0.0f) {
        const float factor = shakeTimer / 0.35f;
        shakeX = std::sin(animTime * 65.0f) * shakeIntensity * factor;
        shakeY = std::cos(animTime * 55.0f) * shakeIntensity * factor;
    }

    // Shake is a pure translation: offset the target with the position so the
    // view never swings its aim around.
    Camera3D cam = camera;
    const float shakeOffX = shakeX * 0.02f;
    const float shakeOffY = shakeY * 0.02f;
    cam.position.x += shakeOffX;
    cam.position.y += shakeOffY;
    cam.target.x += shakeOffX;
    cam.target.y += shakeOffY;

    BeginMode3D(cam);
    drawArenaFloor();
    drawWarnings(*this);
    drawAiWarnings(*this);
    drawObstacles(*this);
    drawGhosts(*this);
    drawAiSnakes(*this);
    drawApple(*this);
    if (hasSpecial) {
        drawSpecialFood(*this);
    }
    drawPendingBites(*this);
    // With animation on, the body is gone on game over: it burst into shards.
    if (!(over && animations)) {
        drawSnakeBody(*this);
    }
    drawParticles3D(*this);
    EndMode3D();

    // Floating score popups projected from world space.
    for (const FloatingText& ft : floatingTexts) {
        const float lifeRatio = (ft.maxLife > 0.0f) ? (ft.life / ft.maxLife) : 0.0f;
        Color col = ft.color;
        col.a = static_cast<unsigned char>(255.0f * std::min(1.0f, lifeRatio * 1.5f));
        const Vector2 screen = GetWorldToScreen(ft.pos, cam);
        constexpr int fSize = 16;
        const int tw = MeasureText(ft.text.c_str(), fSize);
        DrawText(ft.text.c_str(), static_cast<int>(screen.x - tw * 0.5f) + 1,
                 static_cast<int>(screen.y) + 1, fSize, Color{0, 0, 0, col.a});
        DrawText(ft.text.c_str(), static_cast<int>(screen.x - tw * 0.5f),
                 static_cast<int>(screen.y), fSize, col);
    }

    // In-arena special-effect banner.
    if (fxTextTimer > 0.0f) {
        const char* alert = TextFormat("SPECIAL: %s", fxText.c_str());
        constexpr int fontSize = 24;
        const int textWidth = MeasureText(alert, fontSize);
        const Rectangle banner{static_cast<float>(kArenaX + (kArenaW - textWidth - 44) / 2) + shakeX,
                               static_cast<float>(kArenaY + (kArenaH - 58) / 2) + shakeY,
                               static_cast<float>(textWidth + 44), 58.0f};
        DrawRectangleRounded(banner, 0.25f, 8, Color{14, 16, 24, 235});
        DrawRectangleRoundedLinesEx(banner, 0.25f, 8, 2.0f, fxColor);
        DrawText(alert, static_cast<int>(banner.x + (banner.width - textWidth) * 0.5f),
                 static_cast<int>(banner.y + (banner.height - fontSize) * 0.5f), fontSize, fxColor);
    }

    // Arena frame drawn in 2D over the 3D viewport.
    DrawRectangleLinesEx(Rectangle{static_cast<float>(kArenaX) + shakeX,
                                   static_cast<float>(kArenaY) + shakeY,
                                   static_cast<float>(kArenaW), static_cast<float>(kArenaH)},
                         2.0f, Color{52, 60, 80, 255});
    {
        constexpr float cLen = 16.0f;
        constexpr float cThick = 3.0f;
        const Color cornerCol{95, 175, 250, 225};
        const float ax = static_cast<float>(kArenaX) + shakeX;
        const float ay = static_cast<float>(kArenaY) + shakeY;
        const float aw = static_cast<float>(kArenaW);
        const float ah = static_cast<float>(kArenaH);
        DrawLineEx(Vector2{ax - 1.0f, ay}, Vector2{ax + cLen, ay}, cThick, cornerCol);
        DrawLineEx(Vector2{ax, ay - 1.0f}, Vector2{ax, ay + cLen}, cThick, cornerCol);
        DrawLineEx(Vector2{ax + aw - cLen, ay}, Vector2{ax + aw + 1.0f, ay}, cThick, cornerCol);
        DrawLineEx(Vector2{ax + aw, ay - 1.0f}, Vector2{ax + aw, ay + cLen}, cThick, cornerCol);
        DrawLineEx(Vector2{ax - 1.0f, ay + ah}, Vector2{ax + cLen, ay + ah}, cThick, cornerCol);
        DrawLineEx(Vector2{ax, ay + ah - cLen}, Vector2{ax, ay + ah + 1.0f}, cThick, cornerCol);
        DrawLineEx(Vector2{ax + aw - cLen, ay + ah}, Vector2{ax + aw + 1.0f, ay + ah}, cThick,
                   cornerCol);
        DrawLineEx(Vector2{ax + aw, ay + ah - cLen}, Vector2{ax + aw, ay + ah + 1.0f}, cThick,
                   cornerCol);
    }

    // Camera hint inside the arena (keeps the key discoverable).
    DrawText(TextFormat("CAM: %s  [C]", cameraLabel(cameraMode)), kArenaX + 10,
             kArenaY + kArenaH - 16, 11, Color{150, 165, 195, 210});

    // Pause overlay.
    if (paused && !over) {
        DrawRectangle(kArenaX, kArenaY, kArenaW, kArenaH, Color{10, 12, 18, 175});
        const Rectangle pCard = ui::pauseCardRect();
        DrawRectangleRounded(
            Rectangle{pCard.x - 3.0f, pCard.y - 3.0f, pCard.width + 6.0f, pCard.height + 6.0f},
            0.16f, 8, Color{0, 0, 0, 95});
        DrawRectangleRounded(pCard, 0.16f, 8, Color{22, 26, 38, 248});
        DrawRectangleRoundedLinesEx(pCard, 0.16f, 8, 2.0f, kCyan);

        const float iconX = pCard.x + pCard.width * 0.5f;
        const float iconY = pCard.y + 36.0f;
        DrawRectangleRounded(Rectangle{iconX - 11.0f, iconY - 14.0f, 7.0f, 22.0f}, 0.3f, 4, kCyan);
        DrawRectangleRounded(Rectangle{iconX + 4.0f, iconY - 14.0f, 7.0f, 22.0f}, 0.3f, 4, kCyan);

        const char* pTitle = "GAME PAUSED";
        constexpr int ptSize = 24;
        const int ptw = MeasureText(pTitle, ptSize);
        DrawText(pTitle, static_cast<int>(pCard.x + (pCard.width - ptw) * 0.5f),
                 static_cast<int>(pCard.y + 60), ptSize, kText);

        const char* prompt1 = "Press P or click RESUME to continue";
        constexpr int pr1Size = 13;
        const int pr1w = MeasureText(prompt1, pr1Size);
        DrawText(prompt1, static_cast<int>(pCard.x + (pCard.width - pr1w) * 0.5f),
                 static_cast<int>(pCard.y + 100), pr1Size, Color{170, 180, 205, 255});

        const Rectangle sBtn = ui::pauseSettingsRect();
        drawButton(sBtn, "SETTINGS", true, false, CheckCollisionPointRec(uiMouse, sBtn));

        const char* prompt2 = TextFormat("ESC: Menu   |   Camera: %s", cameraLabel(cameraMode));
        constexpr int pr2Size = 12;
        const int pr2w = MeasureText(prompt2, pr2Size);
        DrawText(prompt2, static_cast<int>(pCard.x + (pCard.width - pr2w) * 0.5f),
                 static_cast<int>(pCard.y + 166), pr2Size, Color{125, 135, 155, 255});
    }

    // Top HUD bar (full width so the 3D scene sits behind it cleanly).
    DrawRectangle(0, 0, kWinW, kArenaY, Color{20, 23, 33, 238});
    DrawLine(0, kArenaY, kWinW, kArenaY, Color{52, 60, 82, 255});

    DrawCircle(kPanelX + 20, kPanelY + 16, 4.0f, kGood);
    DrawText("SCORE", kPanelX + 30, kPanelY + 10, 10, Color{140, 148, 168, 255});
    DrawText(TextFormat("%d", score), kPanelX + 30, kPanelY + 22, 19, kText);

    DrawCircle(kPanelX + 130, kPanelY + 16, 4.0f, kGold);
    DrawText("HIGH SCORE", kPanelX + 140, kPanelY + 10, 10, Color{180, 170, 120, 255});
    DrawText(TextFormat("%d", highScore), kPanelX + 140, kPanelY + 22, 19, kGold);

    const int length = static_cast<int>(snake.size());
    DrawCircle(kPanelX + 250, kPanelY + 16, 4.0f, kCyan);
    DrawText("LENGTH", kPanelX + 260, kPanelY + 10, 10, Color{120, 165, 185, 255});
    DrawText(TextFormat("%d", length), kPanelX + 260, kPanelY + 22, 19, kCyan);

    {
        const Rectangle bBadge{static_cast<float>(kPanelX + 355), static_cast<float>(kPanelY + 10),
                               95.0f, 28.0f};
        if (paused) {
            DrawRectangleRounded(bBadge, 0.35f, 6, Color{36, 44, 60, 255});
            DrawRectangleRoundedLinesEx(bBadge, 0.35f, 6, 1.2f, kCyan);
            DrawText("PAUSED", static_cast<int>(bBadge.x + 22), static_cast<int>(bBadge.y + 7), 12,
                     kCyan);
        } else if (boostActive()) {
            DrawRectangleRounded(bBadge, 0.35f, 6, Color{35, 95, 62, 255});
            DrawRectangleRoundedLinesEx(bBadge, 0.35f, 6, 1.5f, kBoostOn);
            DrawText("BOOSTING", static_cast<int>(bBadge.x + 14), static_cast<int>(bBadge.y + 7), 12,
                     WHITE);
        } else if (length <= kMinLength) {
            DrawRectangleRounded(bBadge, 0.35f, 6, Color{28, 30, 40, 255});
            DrawRectangleRoundedLinesEx(bBadge, 0.35f, 6, 1.0f, Color{55, 60, 75, 255});
            DrawText("MIN LEN", static_cast<int>(bBadge.x + 22), static_cast<int>(bBadge.y + 7), 12,
                     GRAY);
        } else {
            DrawRectangleRounded(bBadge, 0.35f, 6, Color{30, 36, 48, 255});
            DrawRectangleRoundedLinesEx(bBadge, 0.35f, 6, 1.0f, Color{60, 72, 95, 255});
            DrawText("BOOST: OK", static_cast<int>(bBadge.x + 12), static_cast<int>(bBadge.y + 7),
                     12, Color{130, 190, 160, 255});
        }
    }

    {
        const Rectangle fxBar{static_cast<float>(kPanelX + 465), static_cast<float>(kPanelY + 10),
                              162.0f, 28.0f};
        DrawRectangleRounded(fxBar, 0.3f, 6, Color{26, 30, 42, 255});
        DrawRectangleRoundedLinesEx(fxBar, 0.3f, 6, 1.0f, Color{50, 58, 80, 255});

        if (effectTimer > 0.0f) {
            const float progress = std::clamp(effectTimer / kEffectDuration, 0.0f, 1.0f);
            const Rectangle fillBar{fxBar.x + 2.0f, fxBar.y + 2.0f, (fxBar.width - 4.0f) * progress,
                                   fxBar.height - 4.0f};
            DrawRectangleRounded(fillBar, 0.25f, 4, Color{fxColor.r, fxColor.g, fxColor.b, 90});
            DrawRectangleRoundedLinesEx(fillBar, 0.25f, 4, 1.0f, fxColor);
            const char* fxStr =
                TextFormat("%s %.1fs", effectName(activeEffect), static_cast<double>(effectTimer));
            const int tw = MeasureText(fxStr, 12);
            DrawText(fxStr, static_cast<int>(fxBar.x + (fxBar.width - tw) * 0.5f),
                     static_cast<int>(fxBar.y + 7), 12, fxColor);
        } else if (fxTextTimer > 0.0f) {
            const char* fxStr = TextFormat("FX: %s", fxText.c_str());
            const int tw = MeasureText(fxStr, 12);
            DrawText(fxStr, static_cast<int>(fxBar.x + (fxBar.width - tw) * 0.5f),
                     static_cast<int>(fxBar.y + 7), 12, fxColor);
        } else {
            const char* fxStr = "FX: NONE";
            const int tw = MeasureText(fxStr, 12);
            DrawText(fxStr, static_cast<int>(fxBar.x + (fxBar.width - tw) * 0.5f),
                     static_cast<int>(fxBar.y + 7), 12, Color{110, 116, 134, 255});
        }
    }

    // Bottom control bar.
    DrawRectangle(0, kBarY, kWinW, kWinH - kBarY, Color{20, 23, 33, 238});
    DrawLine(0, kBarY, kWinW, kBarY, Color{52, 60, 82, 255});

    const bool canPlay = !over;
    drawButton(ui::moveLeftRect(), "<", canPlay && !paused, false,
               CheckCollisionPointRec(uiMouse, ui::moveLeftRect()));
    drawButton(ui::moveUpRect(), "^", canPlay && !paused, false,
               CheckCollisionPointRec(uiMouse, ui::moveUpRect()));
    drawButton(ui::moveDownRect(), "v", canPlay && !paused, false,
               CheckCollisionPointRec(uiMouse, ui::moveDownRect()));
    drawButton(ui::moveRightRect(), ">", canPlay && !paused, false,
               CheckCollisionPointRec(uiMouse, ui::moveRightRect()));
    drawButton(ui::boostRect(), "BOOST", canPlay && !paused, boostActive(),
               CheckCollisionPointRec(uiMouse, ui::boostRect()));
    drawButton(ui::pauseRect(), paused ? "RESUME" : "PAUSE", canPlay, paused,
               CheckCollisionPointRec(uiMouse, ui::pauseRect()));
    drawButton(ui::restartRect(), "RESTART", over, false,
               CheckCollisionPointRec(uiMouse, ui::restartRect()));
    drawButton(ui::quitRect(), "QUIT", true, false,
               CheckCollisionPointRec(uiMouse, ui::quitRect()));

    {
        const float ky = static_cast<float>(kBarY + 56);
        drawKeycap(kPanelX + 14, ky, "WASD", "Turn");
        drawKeycap(kPanelX + 116, ky, "SPACE", "Boost");
        drawKeycap(kPanelX + 230, ky, "P", "Pause");
        drawKeycap(kPanelX + 318, ky, "R", "Restart");
        drawKeycap(kPanelX + 414, ky, "ESC", "Menu");
        drawKeycap(kPanelX + 508, ky, "F11", "Fullscreen");
    }

    // Game over modal.
    if (over) {
        const unsigned char fadeA = static_cast<unsigned char>(205.0f * overFade);
        DrawRectangle(0, 0, kWinW, kWinH, Color{8, 10, 15, fadeA});
        const float slide = (1.0f - overFade) * 14.0f;
        const Rectangle modal{static_cast<float>(kWinW - 420) * 0.5f,
                              static_cast<float>(kWinH - 320) * 0.5f + slide, 420.0f, 320.0f};
        DrawRectangleRounded(Rectangle{modal.x - 4, modal.y - 4, modal.width + 8, modal.height + 8},
                             0.12f, 10, Color{0, 0, 0, 110});
        DrawRectangleRounded(modal, 0.12f, 10,
                             Color{22, 25, 36, static_cast<unsigned char>(248.0f * overFade)});
        DrawRectangleRoundedLinesEx(modal, 0.12f, 10, 2.0f, Color{235, 75, 75, 220});

        const char* title = "GAME OVER";
        constexpr int tSize = 38;
        const int tw = MeasureText(title, tSize);
        DrawText(title, static_cast<int>(modal.x + (modal.width - tw) * 0.5f),
                 static_cast<int>(modal.y + 24), tSize, Color{242, 85, 85, 255});

        if (isNewHighRecord) {
            const char* recordText = "★ NEW BEST RECORD! ★";
            const int rtw = MeasureText(recordText, 14);
            const Rectangle rBadge{modal.x + (modal.width - rtw - 28) * 0.5f, modal.y + 72,
                                   static_cast<float>(rtw + 28), 24.0f};
            DrawRectangleRounded(rBadge, 0.5f, 6, Color{80, 65, 20, 230});
            DrawRectangleRoundedLinesEx(rBadge, 0.5f, 6, 1.5f, kGold);
            DrawText(recordText, static_cast<int>(rBadge.x + 14), static_cast<int>(rBadge.y + 5),
                     14, kGold);
        }

        const float cardY = isNewHighRecord ? (modal.y + 110) : (modal.y + 88);
        constexpr float cardW = 175.0f;
        constexpr float cardH = 76.0f;

        const Rectangle p1{modal.x + 24, cardY, cardW, cardH};
        DrawRectangleRounded(p1, 0.2f, 6, Color{30, 34, 48, 255});
        DrawRectangleRoundedLinesEx(p1, 0.2f, 6, 1.0f, Color{55, 64, 88, 255});
        DrawText("FINAL SCORE", static_cast<int>(p1.x + 14), static_cast<int>(p1.y + 12), 11,
                 Color{150, 160, 180, 255});
        const std::string sVal = std::to_string(score);
        DrawText(sVal.c_str(), static_cast<int>(p1.x + 14), static_cast<int>(p1.y + 32), 28, kGood);

        const Rectangle p2{modal.x + modal.width - cardW - 24, cardY, cardW, cardH};
        DrawRectangleRounded(p2, 0.2f, 6, Color{30, 34, 48, 255});
        DrawRectangleRoundedLinesEx(p2, 0.2f, 6, 1.0f, Color{55, 64, 88, 255});
        DrawText("SESSION BEST", static_cast<int>(p2.x + 14), static_cast<int>(p2.y + 12), 11,
                 Color{180, 170, 130, 255});
        const std::string hVal = std::to_string(highScore);
        DrawText(hVal.c_str(), static_cast<int>(p2.x + 14), static_cast<int>(p2.y + 32), 28, kGold);

        const std::string lenText = "Final Snake Length: " + std::to_string(length);
        const int lenW = MeasureText(lenText.c_str(), 14);
        DrawText(lenText.c_str(), static_cast<int>(modal.x + (modal.width - lenW) * 0.5f),
                 static_cast<int>(cardY + cardH + 18), 14, Color{170, 180, 200, 255});

        const float actionY = modal.y + modal.height - 48.0f;
        drawKeycap(modal.x + 45.0f, actionY, "R", "Play Again");
        drawKeycap(modal.x + 225.0f, actionY, "ESC", "Main Menu");
    }

    if (transitionT > 0.0f) {
        DrawRectangle(0, 0, kWinW, kWinH,
                      Color{8, 9, 14, static_cast<unsigned char>(255.0f * transitionT)});
    }
}

} // namespace snake
