#include "game/game.hpp"
#include "render/color.hpp"
#include "render/snake_head.hpp"
#include "ui/layout.hpp"

#include <cmath>
#include <cstring>
#include <string>

namespace snake {
namespace {

const char* settingName(int row) {
    switch (row) {
    case RowResolution:
        return "RESOLUTION";
    case RowFullscreen:
        return "FULLSCREEN";
    case RowVSync:
        return "VSYNC";
    case RowFps:
        return "FPS LIMIT";
    case RowSpeed:
        return "GAME SPEED";
    case RowCamera:
        return "CAMERA";
    case RowAnimations:
        return "ANIMATIONS";
    case RowParticles:
        return "PARTICLES";
    case RowShake:
        return "SCREEN SHAKE";
    case RowMasterVol:
        return "MASTER VOLUME";
    case RowMusicVol:
        return "MUSIC VOLUME";
    case RowSfxVol:
        return "SFX VOLUME";
    case RowMute:
        return "MUTE";
    case RowReset:
        return "RESET TO DEFAULT";
    default:
        return "";
    }
}

} // namespace

void Game::drawButton(const Rectangle& r, const char* label, bool enabled, bool active,
                      bool hovered) const {
    Color fill{34, 38, 52, 255};
    Color border{58, 66, 88, 255};
    Color textColor = kText;
    Color iconColor = kText;

    if (!enabled) {
        fill = Color{24, 27, 36, 255};
        border = Color{42, 46, 60, 255};
        textColor = Color{100, 106, 122, 255};
        iconColor = Color{100, 106, 122, 255};
    } else if (active) {
        fill = Color{36, 88, 62, 255};
        border = kBoostOn;
        textColor = WHITE;
        iconColor = kBoostOn;
    } else if (hovered) {
        fill = Color{46, 54, 74, 255};
        border = Color{95, 175, 250, 235};
        textColor = WHITE;
        iconColor = Color{120, 195, 255, 255};
    }

    // Hover treatment: a small breathing scale plus a soft glow behind the card.
    Rectangle base = r;
    if (hovered && enabled && animations) {
        const float grow = 1.0f + 0.015f * (0.5f + 0.5f * std::sin(animTime * 7.0f));
        const float w = r.width * grow;
        const float h = r.height * grow;
        base = Rectangle{r.x - (w - r.width) * 0.5f, r.y - (h - r.height) * 0.5f, w, h};
        DrawRectangleRounded(Rectangle{base.x - 3.0f, base.y - 3.0f, base.width + 6.0f,
                                       base.height + 6.0f},
                             0.3f, 8, Color{95, 175, 250, 26});
    }
    const Rectangle r2 = base;

    // Drop shadow
    DrawRectangleRounded(Rectangle{r2.x + 1.0f, r2.y + 2.0f, r2.width, r2.height}, 0.28f, 8,
                         Color{0, 0, 0, 70});

    // Base button
    DrawRectangleRounded(r2, 0.28f, 8, fill);

    // Top subtle bevel reflection line
    if (enabled) {
        DrawLineEx(Vector2{r2.x + 8.0f, r2.y + 2.0f}, Vector2{r2.x + r2.width - 8.0f, r2.y + 2.0f},
                   1.2f,
                   Color{255, 255, 255, static_cast<unsigned char>(hovered ? 45 : 22)});
    }

    // Border line
    DrawRectangleRoundedLinesEx(r2, 0.28f, 8, 1.8f, border);

    const float cx = r2.x + r2.width * 0.5f;
    const float cy = r2.y + r2.height * 0.5f;

    // Custom vector icons for directional buttons and action buttons
    if (std::strcmp(label, "<") == 0) {
        DrawTriangle(Vector2{cx - 6.5f, cy}, Vector2{cx + 4.5f, cy + 7.5f},
                     Vector2{cx + 4.5f, cy - 7.5f}, iconColor);
        return;
    }
    if (std::strcmp(label, ">") == 0) {
        DrawTriangle(Vector2{cx + 6.5f, cy}, Vector2{cx - 4.5f, cy - 7.5f},
                     Vector2{cx - 4.5f, cy + 7.5f}, iconColor);
        return;
    }
    if (std::strcmp(label, "^") == 0) {
        DrawTriangle(Vector2{cx, cy - 6.5f}, Vector2{cx - 7.5f, cy + 4.5f},
                     Vector2{cx + 7.5f, cy + 4.5f}, iconColor);
        return;
    }
    if (std::strcmp(label, "v") == 0) {
        DrawTriangle(Vector2{cx, cy + 6.5f}, Vector2{cx + 7.5f, cy - 4.5f},
                     Vector2{cx - 7.5f, cy - 4.5f}, iconColor);
        return;
    }

    if (std::strcmp(label, "BOOST") == 0) {
        // Lightning bolt icon
        const float ix = r.x + 15.0f;
        const float iy = cy;
        DrawTriangle(Vector2{ix + 2.0f, iy - 8.0f}, Vector2{ix - 4.0f, iy + 0.5f},
                     Vector2{ix + 1.5f, iy + 0.5f}, iconColor);
        DrawTriangle(Vector2{ix - 1.5f, iy - 0.5f}, Vector2{ix + 4.0f, iy - 0.5f},
                     Vector2{ix - 2.0f, iy + 8.0f}, iconColor);

        constexpr int fSize = 15;
        DrawText(label, static_cast<int>(r.x + 28.0f), static_cast<int>(cy - fSize * 0.5f), fSize,
                 textColor);
        return;
    }

    if (std::strcmp(label, "PAUSE") == 0) {
        // Pause icon (two vertical bars)
        const float ix = r.x + 16.0f;
        const float iy = cy;
        DrawRectangleRounded(Rectangle{ix - 4.5f, iy - 6.5f, 3.2f, 13.0f}, 0.25f, 2, iconColor);
        DrawRectangleRounded(Rectangle{ix + 1.5f, iy - 6.5f, 3.2f, 13.0f}, 0.25f, 2, iconColor);

        constexpr int fSize = 14;
        DrawText(label, static_cast<int>(r.x + 27.0f), static_cast<int>(cy - fSize * 0.5f), fSize,
                 textColor);
        return;
    }

    if (std::strcmp(label, "RESUME") == 0) {
        // Resume play icon
        const float ix = r.x + 14.0f;
        DrawTriangle(Vector2{ix + 8.0f, cy}, Vector2{ix, cy - 6.0f}, Vector2{ix, cy + 6.0f},
                     iconColor);

        constexpr int fSize = 13;
        DrawText(label, static_cast<int>(r.x + 25.0f), static_cast<int>(cy - fSize * 0.5f), fSize,
                 textColor);
        return;
    }

    if (std::strcmp(label, "RESTART") == 0) {
        // Circular reload arrow
        const float ix = r.x + 16.0f;
        const float iy = cy;
        DrawRing(Vector2{ix, iy}, 5.0f, 7.2f, 40.0f, 320.0f, 20, iconColor);
        DrawTriangle(Vector2{ix + 4.0f, iy - 7.5f}, Vector2{ix + 7.5f, iy - 3.0f},
                     Vector2{ix + 1.0f, iy - 3.8f}, iconColor);

        constexpr int fSize = 15;
        DrawText(label, static_cast<int>(r.x + 30.0f), static_cast<int>(cy - fSize * 0.5f), fSize,
                 textColor);
        return;
    }

    if (std::strcmp(label, "QUIT") == 0 && r.height < 50.0f) {
        // Mini power / exit icon for in-game bar
        const float ix = r.x + 22.0f;
        const float iy = cy;
        DrawRing(Vector2{ix, iy + 1.0f}, 4.5f, 6.5f, 45.0f, 315.0f, 18, iconColor);
        DrawLineEx(Vector2{ix, iy - 6.5f}, Vector2{ix, iy}, 2.0f, iconColor);

        constexpr int fSize = 15;
        DrawText(label, static_cast<int>(r.x + 38.0f), static_cast<int>(cy - fSize * 0.5f), fSize,
                 textColor);
        return;
    }

    if (std::strcmp(label, "PLAY") == 0) {
        // Large Play button in menu with Play triangle
        constexpr int fSize = 22;
        const int tw = MeasureText(label, fSize);
        const float startX = r.x + (r.width - tw - 24.0f) * 0.5f;

        // Play triangle icon
        DrawTriangle(Vector2{startX + 14.0f, cy}, Vector2{startX, cy - 8.0f},
                     Vector2{startX, cy + 8.0f}, iconColor);
        DrawText(label, static_cast<int>(startX + 24.0f), static_cast<int>(cy - fSize * 0.5f),
                 fSize, textColor);
        return;
    }

    if (std::strncmp(label, "FULLSCREEN", 10) == 0) {
        // Screen icon
        constexpr int fSize = 18;
        const int tw = MeasureText(label, fSize);
        const float startX = r.x + (r.width - tw - 26.0f) * 0.5f;
        const float ix = startX + 8.0f;
        const Rectangle sRect{ix - 8.0f, cy - 6.0f, 16.0f, 12.0f};
        DrawRectangleLinesEx(sRect, 1.8f, iconColor);
        DrawLineEx(Vector2{ix - 3.5f, cy + 8.0f}, Vector2{ix + 3.5f, cy + 8.0f}, 1.8f, iconColor);

        DrawText(label, static_cast<int>(startX + 26.0f), static_cast<int>(cy - fSize * 0.5f),
                 fSize, textColor);
        return;
    }

    // Default text rendering
    constexpr int fontSize = 20;
    const int textW = MeasureText(label, fontSize);
    DrawText(label, static_cast<int>(r.x + (r.width - static_cast<float>(textW)) * 0.5f),
             static_cast<int>(r.y + (r.height - static_cast<float>(fontSize)) * 0.5f), fontSize,
             textColor);
}

void Game::drawMenu() const {
    // Futuristic gradient background
    DrawRectangleGradientV(0, 0, kWinW, kWinH, Color{13, 16, 25, 255}, Color{8, 9, 14, 255});

    // Ambient floating particles
    for (const Particle& p : ambientParticles) {
        DrawCircleV(p.pos, p.size, p.color);
    }

    // Subtle background grid lines & intersection dots
    for (int x = kCell; x < kWinW; x += kCell * 2) {
        DrawLine(x, 0, x, kWinH, Color{22, 26, 36, 180});
    }
    for (int y = kCell; y < kWinH; y += kCell * 2) {
        DrawLine(0, y, kWinW, y, Color{22, 26, 36, 180});
    }
    for (int y = kCell; y < kWinH; y += kCell * 2) {
        for (int x = kCell; x < kWinW; x += kCell * 2) {
            DrawCircle(x, y, 1.2f, Color{45, 52, 72, 200});
        }
    }

    // Title "SNAKE" with multi-layered glow
    const char* title = "SNAKE";
    constexpr int titleSize = 88;
    const int titleW = MeasureText(title, titleSize);
    const int titleX = (kWinW - titleW) / 2;

    // Outer glow / shadow layers
    DrawText(title, titleX + 4, 58, titleSize, Color{18, 55, 42, 255});
    DrawText(title, titleX + 2, 56, titleSize, Color{35, 120, 80, 200});
    // Crisp foreground text
    DrawText(title, titleX, 54, titleSize, kSnakeHead);

    // Modern pill badge for edition
    {
        const char* subtitle = "★ RAYLIB EDITION ★";
        constexpr int subSize = 13;
        const int subW = MeasureText(subtitle, subSize);
        const Rectangle subPill{static_cast<float>(kWinW - subW - 30) * 0.5f, 150.0f,
                                static_cast<float>(subW + 30), 22.0f};
        DrawRectangleRounded(subPill, 0.5f, 6, Color{22, 28, 42, 240});
        DrawRectangleRoundedLinesEx(subPill, 0.5f, 6, 1.2f, Color{70, 130, 200, 220});
        DrawText(subtitle, static_cast<int>(subPill.x + 15), static_cast<int>(subPill.y + 5),
                 subSize, Color{140, 210, 255, 255});
    }

    // ==========================================
    // ANIMATED SLITHERING SNAKE & APPLE MOTIF
    // ==========================================
    {
        constexpr float motifBaseX = (static_cast<float>(kWinW) - 260.0f) * 0.5f;
        constexpr float motifBaseY = 186.0f;
        constexpr int motifCount = 8;
        constexpr float bodyThick = 18.0f;

        // Bridge connectors between adjacent animated segments
        for (int i = 1; i < motifCount; ++i) {
            const float x1 = motifBaseX + static_cast<float>((i - 1) * 26);
            const float y1 =
                motifBaseY +
                std::sin(animTime * 3.5f + static_cast<float>(i - 1) * 0.5f) * 6.5f;
            const float x2 = motifBaseX + static_cast<float>(i * 26);
            const float y2 =
                motifBaseY + std::sin(animTime * 3.5f + static_cast<float>(i) * 0.5f) * 6.5f;

            const float t = static_cast<float>(i) / static_cast<float>(motifCount);
            const Color bCol = render::lerpColor(kSnakeTail, kSnakeHead, t);
            DrawLineEx(Vector2{x1, y1}, Vector2{x2, y2}, bodyThick, bCol);
        }

        // Draw segments
        for (int i = 0; i < motifCount; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(motifCount - 1);
            const Color segCol = render::lerpColor(kSnakeTail, kSnakeHead, t);
            const float sx = motifBaseX + static_cast<float>(i * 26);
            const float sy =
                motifBaseY + std::sin(animTime * 3.5f + static_cast<float>(i) * 0.5f) * 6.5f;

            if (i == 0) {
                // Tapered rounded tail
                DrawCircle(static_cast<int>(sx), static_cast<int>(sy), 7.0f, segCol);
            } else if (i == motifCount - 1) {
                // Head segment with expressive eyes
                DrawCircle(static_cast<int>(sx), static_cast<int>(sy), 11.0f, segCol);

                // Cute eyes looking right towards the food
                DrawCircle(static_cast<int>(sx + 3.0f), static_cast<int>(sy - 4.5f), 3.2f,
                           Color{242, 246, 252, 255});
                DrawCircle(static_cast<int>(sx + 3.0f), static_cast<int>(sy + 4.5f), 3.2f,
                           Color{242, 246, 252, 255});
                DrawCircle(static_cast<int>(sx + 4.0f), static_cast<int>(sy - 4.5f), 1.8f,
                           Color{20, 24, 30, 255});
                DrawCircle(static_cast<int>(sx + 4.0f), static_cast<int>(sy + 4.5f), 1.8f,
                           Color{20, 24, 30, 255});
                DrawCircle(static_cast<int>(sx + 4.6f), static_cast<int>(sy - 5.2f), 0.8f, WHITE);
                DrawCircle(static_cast<int>(sx + 4.6f), static_cast<int>(sy + 3.8f), 0.8f, WHITE);

                // Animated flicking red tongue
                const float tCycle = std::fmod(animTime, 2.2f);
                if (tCycle < 0.4f) {
                    const float ext =
                        (tCycle < 0.2f) ? (tCycle / 0.2f) : ((0.4f - tCycle) / 0.2f);
                    const float tLen = 9.0f * ext;
                    const Vector2 tb{sx + 10.5f, sy};
                    const Vector2 tt{tb.x + tLen, sy};
                    DrawLineEx(tb, tt, 1.8f, Color{235, 50, 75, 255});
                    DrawLineEx(tt, Vector2{tt.x + 2.5f, sy - 2.5f}, 1.3f, Color{235, 50, 75, 255});
                    DrawLineEx(tt, Vector2{tt.x + 2.5f, sy + 2.5f}, 1.3f, Color{235, 50, 75, 255});
                }
            } else {
                // Body segments with spine shine
                DrawCircle(static_cast<int>(sx), static_cast<int>(sy), 9.0f, segCol);
                DrawCircle(static_cast<int>(sx), static_cast<int>(sy), 2.2f,
                           Color{255, 255, 255, 40});
            }
        }

        // Juicy bobbing apple in front of the snake
        {
            const float foodX = motifBaseX + static_cast<float>(motifCount * 26) + 14.0f;
            const float bob = std::sin(animTime * 4.0f) * 2.0f;
            const float foodY = motifBaseY + bob;

            DrawCircle(static_cast<int>(foodX), static_cast<int>(foodY), 8.0f, kFood);
            DrawCircle(static_cast<int>(foodX - 2.5f), static_cast<int>(foodY - 2.0f), 2.5f,
                       Color{255, 150, 150, 200});
            DrawCircle(static_cast<int>(foodX - 2.0f), static_cast<int>(foodY - 2.0f), 1.2f,
                       WHITE);
            DrawLineEx(Vector2{foodX, foodY - 7.0f}, Vector2{foodX + 2.0f, foodY - 11.0f}, 1.8f,
                       kFoodStem);
            DrawTriangle(Vector2{foodX + 1.0f, foodY - 9.0f},
                         Vector2{foodX + 6.0f, foodY - 11.5f},
                         Vector2{foodX + 4.5f, foodY - 7.5f}, kFoodLeaf);
        }
    }

    // Menu Buttons
    const bool fs = IsWindowFullscreen();
    drawButton(ui::menuPlayRect(), "PLAY", true, false,
               CheckCollisionPointRec(uiMouse, ui::menuPlayRect()));
    drawButton(ui::menuSkinsRect(), "SKINS", true, false,
               CheckCollisionPointRec(uiMouse, ui::menuSkinsRect()));
    drawButton(ui::menuSettingsRect(), "SETTINGS", true, false,
               CheckCollisionPointRec(uiMouse, ui::menuSettingsRect()));
    drawButton(ui::menuFullscreenRect(), fs ? "FULLSCREEN: ON" : "FULLSCREEN: OFF", true, fs,
               CheckCollisionPointRec(uiMouse, ui::menuFullscreenRect()));
    drawButton(ui::menuQuitRect(), "QUIT", true, false,
               CheckCollisionPointRec(uiMouse, ui::menuQuitRect()));

    // Session High Score Card (top-right so it never collides with the buttons)
    if (highScore > 0) {
        const std::string best = "★ SESSION BEST: " + std::to_string(highScore) + " ★";
        constexpr int bestSize = 13;
        const int bestW = MeasureText(best.c_str(), bestSize);
        const Rectangle bCard{static_cast<float>(kWinW - bestW - 46), 16.0f,
                              static_cast<float>(bestW + 30), 26.0f};
        DrawRectangleRounded(bCard, 0.4f, 6, Color{32, 28, 16, 230});
        DrawRectangleRoundedLinesEx(bCard, 0.4f, 6, 1.2f, kGold);
        DrawText(best.c_str(), static_cast<int>(bCard.x + 15), static_cast<int>(bCard.y + 6),
                 bestSize, kGold);
    }

    // Keyboard navigation hints
    const char* hint = "ENTER / PLAY to start  |  F11 fullscreen  |  ESC quit";
    constexpr int hintSize = 13;
    const int hintW = MeasureText(hint, hintSize);
    DrawText(hint, (kWinW - hintW) / 2, 505, hintSize, Color{125, 132, 148, 255});
}

void Game::drawSettings() const {
    DrawRectangleGradientV(0, 0, kWinW, kWinH, Color{13, 16, 25, 255}, Color{8, 9, 14, 255});
    for (const Particle& p : ambientParticles) {
        DrawCircleV(p.pos, p.size, p.color);
    }
    for (int x = kCell; x < kWinW; x += kCell * 2) {
        DrawLine(x, 0, x, kWinH, Color{22, 26, 36, 150});
    }

    const char* title = "SETTINGS";
    constexpr int titleSize = 40;
    const int titleW = MeasureText(title, titleSize);
    DrawText(title, (kWinW - titleW) / 2 + 2, 42, titleSize, Color{28, 62, 48, 255});
    DrawText(title, (kWinW - titleW) / 2, 40, titleSize, kSnakeHead);

    const Rectangle card{160.0f, 94.0f, 640.0f, 344.0f};
    DrawRectangleRounded(card, 0.06f, 10, Color{22, 25, 36, 242});
    DrawRectangleRoundedLinesEx(card, 0.06f, 10, 1.6f, Color{55, 62, 82, 255});

    for (int i = 0; i < kSettingRowCount; ++i) {
        const Rectangle row = ui::settingsRowRect(i);
        const bool selected = (i == settingsIndex);
        const bool hovered = CheckCollisionPointRec(uiMouse, row);
        Color fill{26, 30, 42, 255};
        if (selected) {
            fill = Color{36, 52, 74, 255};
        } else if (hovered) {
            fill = Color{32, 38, 54, 255};
        }
        DrawRectangleRounded(row, 0.35f, 6, fill);
        if (selected) {
            DrawRectangleRoundedLinesEx(row, 0.35f, 6, 1.4f, kCyan);
        }

        DrawText(settingName(i), static_cast<int>(row.x + 14), static_cast<int>(row.y + 4), 12,
                 selected ? kText : Color{180, 190, 210, 255});

        const std::string value = settingLabel(settings, i);
        const float zoneL = row.x + row.width - 200.0f;
        const float zoneR = row.x + row.width - 36.0f;
        const int valueW = MeasureText(value.c_str(), 12);
        if (i != RowReset) {
            DrawText("<", static_cast<int>(zoneL), static_cast<int>(row.y + 3), 13,
                     selected ? kCyan : Color{110, 120, 140, 255});
            DrawText(">", static_cast<int>(zoneR), static_cast<int>(row.y + 3), 13,
                     selected ? kCyan : Color{110, 120, 140, 255});
        }
        DrawText(value.c_str(),
                 static_cast<int>((zoneL + zoneR) * 0.5f - static_cast<float>(valueW) * 0.5f),
                 static_cast<int>(row.y + 4), 12,
                 selected ? kGold : Color{150, 200, 225, 255});
    }

    drawButton(ui::settingsBackRect(), "BACK", true, false,
               CheckCollisionPointRec(uiMouse, ui::settingsBackRect()));

    const char* hint = "UP/DOWN select  |  LEFT/RIGHT change  |  M mute  |  ESC back";
    constexpr int hintSize = 12;
    const int hintW = MeasureText(hint, hintSize);
    DrawText(hint, (kWinW - hintW) / 2, 502, hintSize, Color{130, 140, 160, 255});
}

void Game::drawSkins() const {
    DrawRectangleGradientV(0, 0, kWinW, kWinH, Color{13, 16, 25, 255}, Color{8, 9, 14, 255});
    for (const Particle& p : ambientParticles) {
        DrawCircleV(p.pos, p.size, p.color);
    }

    const SkinDef& sk = skin();

    // 3D turntable: the selected skin slithers in a ring so every angle is visible.
    Camera3D cam{};
    cam.position = Vector3{0.0f, 7.0f, 10.5f};
    cam.target = Vector3{0.0f, 1.2f, 0.0f};
    cam.up = Vector3{0.0f, 1.0f, 0.0f};
    cam.fovy = 34.0f;
    cam.projection = CAMERA_PERSPECTIVE;

    constexpr size_t previewCount = 9;
    BeginMode3D(cam);
    DrawCubeV(Vector3{0.0f, -0.3f, 0.0f}, Vector3{5.6f, 0.6f, 5.6f}, Color{20, 24, 34, 255});
    DrawCubeWiresV(Vector3{0.0f, -0.3f, 0.0f}, Vector3{5.6f, 0.6f, 5.6f}, Color{50, 58, 78, 255});

    for (size_t i = 0; i < previewCount; ++i) {
        const float a = animTime * 0.9f + static_cast<float>(i) * 0.55f;
        const Vector3 p{std::cos(a) * 1.9f,
                        1.2f + 0.18f * std::sin(animTime * 2.2f + static_cast<float>(i) * 0.5f),
                        std::sin(a) * 1.9f};
        const Color col = snakeColorAt(i, previewCount);

        if (sk.trail == TrailKind::Glow) {
            DrawSphereEx(p, 0.58f, 6, 8, render::withAlpha(sk.accent, 34));
        }

        if (i + 1 == previewCount) {
            // Same shared head as gameplay so every skin is previewed honestly.
            HeadPose pose;
            pose.pos = p;
            pose.fwd = Vector3{-std::sin(a), 0.0f, std::cos(a)};
            pose.time = animTime;
            pose.animate = animations;
            pose.head = headColor();
            pose.eye = eyeColor();
            pose.crown = sk.crown;
            drawSnakeHead(pose);
        } else if (sk.shape == SegmentShape::Sphere) {
            DrawSphereEx(p, 0.42f, 8, 10, col);
        } else {
            DrawCubeV(p, Vector3{0.78f, 0.7f, 0.78f}, col);
        }
    }
    EndMode3D();

    const char* title = "SKINS";
    constexpr int titleSize = 36;
    const int titleW = MeasureText(title, titleSize);
    DrawText(title, (kWinW - titleW) / 2 + 2, 26, titleSize, Color{28, 62, 48, 255});
    DrawText(title, (kWinW - titleW) / 2, 24, titleSize, kSnakeHead);

    const int nameW = MeasureText(sk.name, 20);
    DrawText(sk.name, (kWinW - nameW) / 2, 318, 20, kGold);

    for (int i = 0; i < kSkinCount; ++i) {
        const Rectangle skinCard = ui::skinCardRect(i);
        const bool selected = (i == settings.skin);
        const bool hovered = CheckCollisionPointRec(uiMouse, skinCard);
        Color fill{24, 28, 38, 245};
        if (selected) {
            fill = Color{40, 56, 78, 250};
        } else if (hovered) {
            fill = Color{32, 38, 52, 245};
        }
        DrawRectangleRounded(skinCard, 0.16f, 8, fill);
        DrawRectangleRoundedLinesEx(skinCard, 0.16f, 8, selected ? 2.0f : 1.2f,
                                    selected ? kGold : Color{55, 62, 82, 255});

        const int labelW = MeasureText(kSkins[i].name, 11);
        DrawText(kSkins[i].name, static_cast<int>(skinCard.x + (skinCard.width - labelW) * 0.5f),
                 static_cast<int>(skinCard.y + 8), 11,
                 selected ? kText : Color{175, 185, 205, 255});

        const float swatchY = skinCard.y + skinCard.height - 22.0f;
        const float swatchX = skinCard.x + skinCard.width * 0.5f - 26.0f;
        DrawCircleV(Vector2{swatchX, swatchY}, 7.0f, kSkins[i].head);
        DrawCircleV(Vector2{swatchX + 18.0f, swatchY}, 7.0f, kSkins[i].body);
        DrawCircleV(Vector2{swatchX + 36.0f, swatchY}, 7.0f, kSkins[i].tail);
    }

    drawButton(ui::skinsBackRect(), "BACK", true, false,
               CheckCollisionPointRec(uiMouse, ui::skinsBackRect()));

    const char* hint = "LEFT/RIGHT or click a card to equip  |  ENTER/ESC back";
    constexpr int hintSize = 12;
    const int hintW = MeasureText(hint, hintSize);
    DrawText(hint, (kWinW - hintW) / 2, 502, hintSize, Color{130, 140, 160, 255});
}

} // namespace snake
