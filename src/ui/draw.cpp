#include "game/game.hpp"
#include "render/color.hpp"
#include "ui/layout.hpp"

#include <cmath>
#include <cstring>
#include <string>

namespace snake {

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
        constexpr float motifBaseY = 200.0f;
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
    drawButton(ui::menuFullscreenRect(), fs ? "FULLSCREEN: ON" : "FULLSCREEN: OFF", true, fs,
               CheckCollisionPointRec(uiMouse, ui::menuFullscreenRect()));
    drawButton(ui::menuQuitRect(), "QUIT", true, false,
               CheckCollisionPointRec(uiMouse, ui::menuQuitRect()));

    // Session High Score Card
    if (highScore > 0) {
        const std::string best = "★ SESSION BEST: " + std::to_string(highScore) + " ★";
        constexpr int bestSize = 14;
        const int bestW = MeasureText(best.c_str(), bestSize);
        const Rectangle bCard{static_cast<float>(kWinW - bestW - 32) * 0.5f, 458.0f,
                              static_cast<float>(bestW + 32), 26.0f};
        DrawRectangleRounded(bCard, 0.4f, 6, Color{32, 28, 16, 230});
        DrawRectangleRoundedLinesEx(bCard, 0.4f, 6, 1.2f, kGold);
        DrawText(best.c_str(), static_cast<int>(bCard.x + 16), static_cast<int>(bCard.y + 6),
                 bestSize, kGold);
    }

    // Keyboard navigation hints
    const char* hint = "ENTER / PLAY to start  |  F11 fullscreen  |  ESC quit";
    constexpr int hintSize = 13;
    const int hintW = MeasureText(hint, hintSize);
    DrawText(hint, (kWinW - hintW) / 2, 498, hintSize, Color{125, 132, 148, 255});
}

} // namespace snake
