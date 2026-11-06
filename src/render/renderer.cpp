#include "game/game.hpp"
#include "render/color.hpp"
#include "ui/layout.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace snake {
namespace {

Rectangle cellRect(const Cell& c, float inset) {
    return Rectangle{static_cast<float>(kArenaX + c.x * kCell) + inset,
                     static_cast<float>(kArenaY + c.y * kCell) + inset,
                     static_cast<float>(kCell) - inset * 2.0f,
                     static_cast<float>(kCell) - inset * 2.0f};
}

void drawKeycap(float x, float y, const char* key, const char* action) {
    const int keyW = MeasureText(key, 11) + 10;
    const Rectangle kr{x, y, static_cast<float>(keyW), 16.0f};
    DrawRectangleRounded(kr, 0.35f, 4, Color{36, 42, 56, 255});
    DrawRectangleRoundedLinesEx(kr, 0.35f, 4, 1.0f, Color{75, 85, 112, 255});
    DrawText(key, static_cast<int>(x + 5), static_cast<int>(y + 3), 11, Color{235, 240, 250, 255});
    DrawText(action, static_cast<int>(x + keyW + 5), static_cast<int>(y + 3), 11,
             Color{150, 156, 172, 255});
}

} // namespace

void Game::drawEyes() const {
    if (snake.empty()) {
        return;
    }
    const Cell& h = snake.front();
    const float cx = static_cast<float>(kArenaX + h.x * kCell + kCell / 2);
    const float cy = static_cast<float>(kArenaY + h.y * kCell + kCell / 2);
    float dx = 0.0f;
    float dy = 0.0f;
    switch (dir) {
    case Dir::Up:
        dy = -1.0f;
        break;
    case Dir::Down:
        dy = 1.0f;
        break;
    case Dir::Left:
        dx = -1.0f;
        break;
    case Dir::Right:
        dx = 1.0f;
        break;
    }
    const float px = -dy;
    const float py = dx;

    const Vector2 eye1{cx + dx * 4.5f + px * 4.5f, cy + dy * 4.5f + py * 4.5f};
    const Vector2 eye2{cx + dx * 4.5f - px * 4.5f, cy + dy * 4.5f - py * 4.5f};

    if (over) {
        // Cute dead 'X' eyes on game over
        const auto drawX = [](Vector2 p) {
            DrawLineEx(Vector2{p.x - 2.8f, p.y - 2.8f}, Vector2{p.x + 2.8f, p.y + 2.8f}, 1.5f,
                       Color{30, 35, 45, 255});
            DrawLineEx(Vector2{p.x - 2.8f, p.y + 2.8f}, Vector2{p.x + 2.8f, p.y - 2.8f}, 1.5f,
                       Color{30, 35, 45, 255});
        };
        drawX(eye1);
        drawX(eye2);
        return;
    }

    // Eye whites
    DrawCircleV(eye1, 3.6f, Color{242, 246, 252, 255});
    DrawCircleV(eye2, 3.6f, Color{242, 246, 252, 255});

    // Dark pupils looking forward in movement direction
    const Vector2 pupilOffset{dx * 0.9f, dy * 0.9f};
    DrawCircleV(Vector2{eye1.x + pupilOffset.x, eye1.y + pupilOffset.y}, 2.0f,
                Color{20, 24, 30, 255});
    DrawCircleV(Vector2{eye2.x + pupilOffset.x, eye2.y + pupilOffset.y}, 2.0f,
                Color{20, 24, 30, 255});

    // Specular glint
    DrawCircleV(Vector2{eye1.x - 0.8f, eye1.y - 0.8f}, 0.8f, WHITE);
    DrawCircleV(Vector2{eye2.x - 0.8f, eye2.y - 0.8f}, 0.8f, WHITE);

    // Tongue flick animation
    const float tongueCycle = std::fmod(animTime, 2.4f);
    if (tongueCycle < 0.35f) {
        const float ext = (tongueCycle < 0.17f) ? (tongueCycle / 0.17f)
                                                : ((0.35f - tongueCycle) / 0.18f);
        const float tongueLen = 7.0f * ext;
        const Vector2 tBase{cx + dx * 9.5f, cy + dy * 9.5f};
        const Vector2 tTip{tBase.x + dx * tongueLen, tBase.y + dy * tongueLen};
        const Vector2 fork1{tTip.x + (dx + px) * 2.4f, tTip.y + (dy + py) * 2.4f};
        const Vector2 fork2{tTip.x + (dx - px) * 2.4f, tTip.y + (dy - py) * 2.4f};

        const Color tColor{230, 50, 75, 255};
        DrawLineEx(tBase, tTip, 1.8f, tColor);
        DrawLineEx(tTip, fork1, 1.3f, tColor);
        DrawLineEx(tTip, fork2, 1.3f, tColor);
    }
}

void Game::drawGhostHead(const GhostHead& g) const {
    const float fade = (g.maxLife > 0.0f) ? (g.life / g.maxLife) : 0.0f;
    const unsigned char alpha = static_cast<unsigned char>(170.0f * fade);

    Color body = kSnakeHead;
    body.a = alpha;
    DrawRectangleRounded(cellRect(g.cell, 1.2f), 0.45f, 8, body);

    const float cx = static_cast<float>(kArenaX + g.cell.x * kCell + kCell / 2);
    const float cy = static_cast<float>(kArenaY + g.cell.y * kCell + kCell / 2);
    float dx = 0.0f;
    float dy = 0.0f;
    switch (g.dir) {
    case Dir::Up:
        dy = -1.0f;
        break;
    case Dir::Down:
        dy = 1.0f;
        break;
    case Dir::Left:
        dx = -1.0f;
        break;
    case Dir::Right:
        dx = 1.0f;
        break;
    }
    const float px = -dy;
    const float py = dx;

    Color eye = Color{240, 245, 252, alpha};
    Color pupil = Color{20, 24, 30, alpha};
    const Vector2 eye1{cx + dx * 4.5f + px * 4.5f, cy + dy * 4.5f + py * 4.5f};
    const Vector2 eye2{cx + dx * 4.5f - px * 4.5f, cy + dy * 4.5f - py * 4.5f};

    DrawCircleV(eye1, 3.2f, eye);
    DrawCircleV(eye2, 3.2f, eye);
    DrawCircleV(Vector2{eye1.x + dx * 0.8f, eye1.y + dy * 0.8f}, 1.8f, pupil);
    DrawCircleV(Vector2{eye2.x + dx * 0.8f, eye2.y + dy * 0.8f}, 1.8f, pupil);
}

void Game::draw() const {
    // Rich outer background gradient
    DrawRectangleGradientV(0, 0, kWinW, kWinH, Color{13, 15, 22, 255}, Color{8, 9, 13, 255});

    // Ambient floating particles
    for (const Particle& p : ambientParticles) {
        DrawCircleV(p.pos, p.size, p.color);
    }

    if (mode == Mode::Menu) {
        drawMenu();
        return;
    }

    // Screen shake calculation
    float shakeX = 0.0f;
    float shakeY = 0.0f;
    if (shakeTimer > 0.0f) {
        const float factor = shakeTimer / 0.35f;
        shakeX = std::sin(animTime * 65.0f) * shakeIntensity * factor;
        shakeY = std::cos(animTime * 55.0f) * shakeIntensity * factor;
    }

    // Arena soft drop-shadow
    DrawRectangleRounded(Rectangle{static_cast<float>(kArenaX) - 6.0f + shakeX,
                                   static_cast<float>(kArenaY) - 6.0f + shakeY,
                                   static_cast<float>(kArenaW) + 12.0f,
                                   static_cast<float>(kArenaH) + 12.0f},
                         0.02f, 4, Color{0, 0, 0, 90});

    // Arena floor
    DrawRectangle(kArenaX + static_cast<int>(shakeX), kArenaY + static_cast<int>(shakeY), kArenaW,
                  kArenaH, kArenaBg);

    // Arena grid: subtle lines + dot-matrix intersections
    for (int x = 1; x < kCols; ++x) {
        const int gx = kArenaX + x * kCell + static_cast<int>(shakeX);
        DrawLine(gx, kArenaY + static_cast<int>(shakeY), gx,
                 kArenaY + kArenaH + static_cast<int>(shakeY), Color{26, 29, 39, 255});
    }
    for (int y = 1; y < kRows; ++y) {
        const int gy = kArenaY + y * kCell + static_cast<int>(shakeY);
        DrawLine(kArenaX + static_cast<int>(shakeX), gy,
                 kArenaX + kArenaW + static_cast<int>(shakeX), gy, Color{26, 29, 39, 255});
    }
    for (int y = 1; y < kRows; ++y) {
        for (int x = 1; x < kCols; ++x) {
            DrawCircle(kArenaX + x * kCell + static_cast<int>(shakeX),
                       kArenaY + y * kCell + static_cast<int>(shakeY), 1.1f, Color{45, 50, 68, 255});
        }
    }

    // Normal Food: Juicy stylized apple with leaf and stem
    {
        const float cx = static_cast<float>(kArenaX + food.x * kCell + kCell / 2) + shakeX;
        const float cy = static_cast<float>(kArenaY + food.y * kCell + kCell / 2) + shakeY;
        const float bob = std::sin(animTime * 4.5f) * 1.5f;
        const float ay = cy + bob;

        // Shadow under floating apple
        DrawEllipse(static_cast<int>(cx), static_cast<int>(cy + 7.5f), 7.0f, 3.0f,
                    Color{8, 10, 16, 120});

        // Apple body
        DrawCircle(static_cast<int>(cx), static_cast<int>(ay + 1.0f), 7.5f, kFood);

        // Apple 3D highlights
        DrawCircle(static_cast<int>(cx - 2.5f), static_cast<int>(ay - 1.5f), 2.5f,
                   Color{255, 150, 150, 200});
        DrawCircle(static_cast<int>(cx - 2.0f), static_cast<int>(ay - 1.5f), 1.2f, WHITE);

        // Curved brown stem
        DrawLineEx(Vector2{cx, ay - 6.5f}, Vector2{cx + 2.0f, ay - 10.5f}, 1.8f, kFoodStem);

        // Crisp green leaf
        DrawTriangle(Vector2{cx + 1.0f, ay - 9.0f}, Vector2{cx + 6.0f, ay - 11.5f},
                     Vector2{cx + 4.5f, ay - 7.2f}, kFoodLeaf);
    }

    // Special Food: Mystical glowing orb with pulsing halo and rotating star rays
    if (hasSpecial) {
        const float sx = static_cast<float>(kArenaX + specialFood.x * kCell + kCell / 2) + shakeX;
        const float sy = static_cast<float>(kArenaY + specialFood.y * kCell + kCell / 2) + shakeY;
        const float pulse = 0.5f + 0.5f * std::sin(animTime * 6.0f);
        const float rot = animTime * 2.0f;

        // Ground shadow
        DrawEllipse(static_cast<int>(sx), static_cast<int>(sy + 8.0f), 8.5f, 3.5f,
                    Color{8, 10, 16, 140});

        // Pulsing outer halo
        DrawCircle(static_cast<int>(sx), static_cast<int>(sy), 13.0f + 3.0f * pulse,
                   Color{kSpecialHalo.r, kSpecialHalo.g, kSpecialHalo.b, 45});

        // Outer rotating diamond rays
        for (int i = 0; i < 4; ++i) {
            const float angle = rot + static_cast<float>(i) * 1.570796f;
            const float rayLen = 11.0f + 2.5f * pulse;
            const Vector2 p1{sx + std::cos(angle) * rayLen, sy + std::sin(angle) * rayLen};
            const Vector2 pSide1{sx + std::cos(angle + 0.35f) * 6.0f,
                                 sy + std::sin(angle + 0.35f) * 6.0f};
            const Vector2 pSide2{sx + std::cos(angle - 0.35f) * 6.0f,
                                 sy + std::sin(angle - 0.35f) * 6.0f};
            DrawTriangle(pSide1, p1, pSide2,
                         Color{kSpecialHalo.r, kSpecialHalo.g, kSpecialHalo.b, 160});
        }

        // Concentric glowing ring
        DrawRing(Vector2{sx, sy}, 9.0f + 1.2f * pulse, 11.0f + 1.2f * pulse, 0.0f, 360.0f, 24,
                 kSpecialHalo);

        // Core purple orb
        DrawCircle(static_cast<int>(sx), static_cast<int>(sy), 7.8f, kSpecial);
        DrawCircle(static_cast<int>(sx - 2.5f), static_cast<int>(sy - 2.5f), 2.5f,
                   Color{240, 200, 255, 210});
        DrawCircle(static_cast<int>(sx - 2.0f), static_cast<int>(sy - 2.0f), 1.2f, WHITE);
    }

    // Solid Obstacles: High-tech beveled barricade blocks
    for (const Obstacle& o : obstacles) {
        Rectangle r = cellRect(o.cell, 1.8f);
        r.x += shakeX;
        r.y += shakeY;

        float fade = 1.0f;
        if (o.life < kObstacleFade) {
            fade = std::max(0.0f, o.life / kObstacleFade);
        }
        const unsigned char a = static_cast<unsigned char>(255.0f * fade);

        // Slate block base
        DrawRectangleRounded(r, 0.22f, 6, Color{36, 40, 52, a});

        // 3D Bevel highlight (top/left) and shadow (bottom/right)
        DrawLineEx(Vector2{r.x + 2.0f, r.y + 1.5f}, Vector2{r.x + r.width - 2.0f, r.y + 1.5f},
                   1.5f, Color{75, 85, 112, static_cast<unsigned char>(a * 0.7f)});
        DrawLineEx(Vector2{r.x + 1.5f, r.y + 2.0f}, Vector2{r.x + 1.5f, r.y + r.height - 2.0f},
                   1.5f, Color{75, 85, 112, static_cast<unsigned char>(a * 0.7f)});

        // Center hazard cross
        const float mx = r.x + r.width * 0.5f;
        const float my = r.y + r.height * 0.5f;
        const Color crossCol = Color{120, 130, 155, static_cast<unsigned char>(180 * fade)};
        DrawLineEx(Vector2{mx - 4.0f, my - 4.0f}, Vector2{mx + 4.0f, my + 4.0f}, 1.8f, crossCol);
        DrawLineEx(Vector2{mx - 4.0f, my + 4.0f}, Vector2{mx + 4.0f, my - 4.0f}, 1.8f, crossCol);

        // Border
        DrawRectangleRoundedLinesEx(r, 0.22f, 6, 1.5f, Color{60, 68, 88, a});
    }

    // Warning Phase: Expanding radar circle and pulsing hazard reticle
    for (const Warning& w : warnings) {
        Rectangle r = cellRect(w.cell, 2.0f);
        r.x += shakeX;
        r.y += shakeY;

        const float urgency = 1.0f - std::max(0.0f, w.timer / kWarningTime);
        const float freq = 12.0f + urgency * 20.0f;
        const float blink = 0.5f + 0.5f * std::sin(animTime * freq);

        const unsigned char fillA = static_cast<unsigned char>(50.0f + 120.0f * blink);
        const unsigned char lineA = static_cast<unsigned char>(180.0f + 75.0f * blink);

        // Danger fill and border
        DrawRectangleRounded(r, 0.2f, 6, Color{kWarning.r, kWarning.g, kWarning.b, fillA});
        DrawRectangleRoundedLinesEx(r, 0.2f, 6, 2.0f,
                                    Color{kWarning.r, kWarning.g, kWarning.b, lineA});

        // Expanding radar ping
        const float cx = r.x + r.width * 0.5f;
        const float cy = r.y + r.height * 0.5f;
        const float radar = std::fmod(animTime * 2.5f, 1.0f);
        DrawCircleLines(static_cast<int>(cx), static_cast<int>(cy), 3.0f + 8.0f * radar,
                        Color{kWarning.r, kWarning.g, kWarning.b,
                              static_cast<unsigned char>(160.0f * (1.0f - radar))});

        // Exclamation mark
        DrawText("!", static_cast<int>(cx - 3.0f), static_cast<int>(cy - 7.0f), 15,
                 Color{255, 245, 220, static_cast<unsigned char>(200.0f + 55.0f * blink)});
    }

    // Snake Body: Connected smooth segments with gradient and 3D spine
    const size_t n = snake.size();
    if (n > 1) {
        // Draw bridge connectors between adjacent segments
        const float bodyThickness = static_cast<float>(kCell) - 2.5f;
        for (size_t i = 1; i < n; ++i) {
            const Cell& curr = snake[i];
            const Cell& prev = snake[i - 1];
            const int dx = prev.x - curr.x;
            const int dy = prev.y - curr.y;

            // Only connect if directly adjacent (not wrapped across the arena)
            if ((std::abs(dx) == 1 && dy == 0) || (std::abs(dy) == 1 && dx == 0)) {
                const float c1x =
                    static_cast<float>(kArenaX + curr.x * kCell + kCell / 2) + shakeX;
                const float c1y =
                    static_cast<float>(kArenaY + curr.y * kCell + kCell / 2) + shakeY;
                const float c2x =
                    static_cast<float>(kArenaX + prev.x * kCell + kCell / 2) + shakeX;
                const float c2y =
                    static_cast<float>(kArenaY + prev.y * kCell + kCell / 2) + shakeY;

                const float t = static_cast<float>(i) / static_cast<float>(n);
                const Color bridgeColor = render::lerpColor(kSnakeHead, kSnakeTail, t);

                if (dx != 0) {
                    const float minX = std::min(c1x, c2x);
                    const float maxX = std::max(c1x, c2x);
                    DrawRectangleRec(
                        Rectangle{minX, c1y - bodyThickness * 0.5f, maxX - minX, bodyThickness},
                        bridgeColor);
                } else {
                    const float minY = std::min(c1y, c2y);
                    const float maxY = std::max(c1y, c2y);
                    DrawRectangleRec(
                        Rectangle{c1x - bodyThickness * 0.5f, minY, bodyThickness, maxY - minY},
                        bridgeColor);
                }
            }
        }
    }

    // Draw segment pills/caps
    for (size_t i = n; i-- > 0;) {
        const float t = (n > 1) ? static_cast<float>(i) / static_cast<float>(n - 1) : 0.0f;
        const Color segColor = render::lerpColor(kSnakeHead, kSnakeTail, t);

        if (i == n - 1 && n > 2) {
            // Tapered rounded tail tip
            Rectangle r = cellRect(snake[i], 3.0f);
            r.x += shakeX;
            r.y += shakeY;
            DrawRectangleRounded(r, 0.75f, 8, segColor);
        } else if (i == 0) {
            // Head segment
            Rectangle r = cellRect(snake[i], 1.2f);
            r.x += shakeX;
            r.y += shakeY;
            DrawRectangleRounded(r, 0.45f, 8, segColor);

            if (boostActive()) {
                DrawRectangleRoundedLinesEx(r, 0.45f, 8, 2.0f, Color{255, 255, 180, 220});
            }
        } else {
            // Body segment with scale highlight
            Rectangle r = cellRect(snake[i], 1.5f);
            r.x += shakeX;
            r.y += shakeY;
            DrawRectangleRounded(r, 0.38f, 8, segColor);

            // Subtle spine highlight
            DrawCircle(static_cast<int>(r.x + r.width * 0.5f),
                       static_cast<int>(r.y + r.height * 0.5f), 2.0f, Color{255, 255, 255, 40});
        }
    }

    drawEyes();

    // Wrap ghosts
    for (const GhostHead& g : ghosts) {
        drawGhostHead(g);
    }

    // Particles (eating bursts, sparks, death shards)
    for (const Particle& p : particles) {
        const float lifeRatio = (p.maxLife > 0.0f) ? (p.life / p.maxLife) : 0.0f;
        Color col = p.color;
        col.a = static_cast<unsigned char>(static_cast<float>(p.color.a) * lifeRatio);
        DrawCircleV(p.pos, p.size * lifeRatio, col);
    }

    // Floating text popups (+10, +50, etc.)
    for (const FloatingText& ft : floatingTexts) {
        const float lifeRatio = (ft.maxLife > 0.0f) ? (ft.life / ft.maxLife) : 0.0f;
        Color col = ft.color;
        col.a = static_cast<unsigned char>(255.0f * std::min(1.0f, lifeRatio * 1.5f));
        constexpr int fSize = 16;
        const int tw = MeasureText(ft.text.c_str(), fSize);
        // Shadow + text
        DrawText(ft.text.c_str(), static_cast<int>(ft.pos.x - tw * 0.5f) + 1,
                 static_cast<int>(ft.pos.y) + 1, fSize, Color{0, 0, 0, col.a});
        DrawText(ft.text.c_str(), static_cast<int>(ft.pos.x - tw * 0.5f),
                 static_cast<int>(ft.pos.y), fSize, col);
    }

    // In-arena Special FX alert banner
    if (fxTextTimer > 0.0f) {
        const char* alert = TextFormat("SPECIAL: %s", fxText.c_str());
        constexpr int fontSize = 24;
        const int textWidth = MeasureText(alert, fontSize);
        const Rectangle banner{static_cast<float>(kArenaX + (kArenaW - textWidth - 44) / 2) +
                                   shakeX,
                               static_cast<float>(kArenaY + (kArenaH - 58) / 2) + shakeY,
                               static_cast<float>(textWidth + 44), 58.0f};
        DrawRectangleRounded(banner, 0.25f, 8, Color{14, 16, 24, 235});
        DrawRectangleRoundedLinesEx(banner, 0.25f, 8, 2.0f, fxColor);
        DrawText(alert, static_cast<int>(banner.x + (banner.width - textWidth) * 0.5f),
                 static_cast<int>(banner.y + (banner.height - fontSize) * 0.5f), fontSize, fxColor);
    }

    // Visible arena frame + Sci-Fi corner accents
    DrawRectangleLinesEx(Rectangle{static_cast<float>(kArenaX) + shakeX,
                                   static_cast<float>(kArenaY) + shakeY,
                                   static_cast<float>(kArenaW), static_cast<float>(kArenaH)},
                         2.0f, Color{52, 60, 80, 255});

    // Neon corner brackets
    {
        constexpr float cLen = 16.0f;
        constexpr float cThick = 3.0f;
        const Color cornerCol{95, 175, 250, 225};

        const float ax = static_cast<float>(kArenaX) + shakeX;
        const float ay = static_cast<float>(kArenaY) + shakeY;
        const float aw = static_cast<float>(kArenaW);
        const float ah = static_cast<float>(kArenaH);

        // Top-left
        DrawLineEx(Vector2{ax - 1.0f, ay}, Vector2{ax + cLen, ay}, cThick, cornerCol);
        DrawLineEx(Vector2{ax, ay - 1.0f}, Vector2{ax, ay + cLen}, cThick, cornerCol);
        // Top-right
        DrawLineEx(Vector2{ax + aw - cLen, ay}, Vector2{ax + aw + 1.0f, ay}, cThick, cornerCol);
        DrawLineEx(Vector2{ax + aw, ay - 1.0f}, Vector2{ax + aw, ay + cLen}, cThick, cornerCol);
        // Bottom-left
        DrawLineEx(Vector2{ax - 1.0f, ay + ah}, Vector2{ax + cLen, ay + ah}, cThick, cornerCol);
        DrawLineEx(Vector2{ax, ay + ah - cLen}, Vector2{ax, ay + ah + 1.0f}, cThick, cornerCol);
        // Bottom-right
        DrawLineEx(Vector2{ax + aw - cLen, ay + ah}, Vector2{ax + aw + 1.0f, ay + ah}, cThick,
                   cornerCol);
        DrawLineEx(Vector2{ax + aw, ay + ah - cLen}, Vector2{ax + aw, ay + ah + 1.0f}, cThick,
                   cornerCol);
    }

    // ==========================================
    // TOP HUD BAR
    // ==========================================
    DrawRectangle(kPanelX, kPanelY, kArenaW, kHud, Color{20, 23, 33, 255});
    DrawLine(kPanelX, kPanelY, kPanelX + kArenaW, kPanelY, Color{45, 52, 72, 255});
    DrawLine(kPanelX, kArenaY, kPanelX + kArenaW, kArenaY, Color{52, 60, 82, 255});

    // 1. Score Badge
    DrawCircle(kPanelX + 20, kPanelY + 16, 4.0f, kGood);
    DrawText("SCORE", kPanelX + 30, kPanelY + 10, 10, Color{140, 148, 168, 255});
    DrawText(TextFormat("%d", score), kPanelX + 30, kPanelY + 22, 19, kText);

    // 2. High Score Badge
    DrawCircle(kPanelX + 130, kPanelY + 16, 4.0f, kGold);
    DrawText("HIGH SCORE", kPanelX + 140, kPanelY + 10, 10, Color{180, 170, 120, 255});
    DrawText(TextFormat("%d", highScore), kPanelX + 140, kPanelY + 22, 19, kGold);

    // 3. Length Badge
    DrawCircle(kPanelX + 250, kPanelY + 16, 4.0f, kCyan);
    DrawText("LENGTH", kPanelX + 260, kPanelY + 10, 10, Color{120, 165, 185, 255});
    DrawText(TextFormat("%d", static_cast<int>(n)), kPanelX + 260, kPanelY + 22, 19, kCyan);

    // 4. Boost Indicator Badge
    {
        const Rectangle bBadge{static_cast<float>(kPanelX + 355), static_cast<float>(kPanelY + 10),
                               95.0f, 28.0f};
        if (boostActive()) {
            DrawRectangleRounded(bBadge, 0.35f, 6, Color{35, 95, 62, 255});
            DrawRectangleRoundedLinesEx(bBadge, 0.35f, 6, 1.5f, kBoostOn);
            DrawText("BOOSTING", static_cast<int>(bBadge.x + 14), static_cast<int>(bBadge.y + 7),
                     12, WHITE);
        } else if (snake.size() <= static_cast<size_t>(kMinLength)) {
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

    // 5. Special Effect Progress Bar (Right side of HUD)
    {
        const Rectangle fxBar{static_cast<float>(kPanelX + 465), static_cast<float>(kPanelY + 10),
                              162.0f, 28.0f};
        DrawRectangleRounded(fxBar, 0.3f, 6, Color{26, 30, 42, 255});
        DrawRectangleRoundedLinesEx(fxBar, 0.3f, 6, 1.0f, Color{50, 58, 80, 255});

        if (effectTimer > 0.0f) {
            const float progress = std::clamp(effectTimer / kEffectDuration, 0.0f, 1.0f);
            const Rectangle fillBar{fxBar.x + 2.0f, fxBar.y + 2.0f, (fxBar.width - 4.0f) * progress,
                                    fxBar.height - 4.0f};
            DrawRectangleRounded(fillBar, 0.25f, 4,
                                 Color{fxColor.r, fxColor.g, fxColor.b, 90});
            DrawRectangleRoundedLinesEx(fillBar, 0.25f, 4, 1.0f, fxColor);

            const char* fxStr = TextFormat("%s %.1fs", effectName(activeEffect),
                                           static_cast<double>(effectTimer));
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

    // ==========================================
    // BOTTOM CONTROL BAR & BUTTONS
    // ==========================================
    DrawRectangle(kPanelX, kBarY, kArenaW, kBarH, Color{20, 23, 33, 255});
    DrawLine(kPanelX, kBarY, kPanelX + kArenaW, kBarY, Color{52, 60, 82, 255});

    const bool canPlay = !over;
    drawButton(ui::moveLeftRect(), "<", canPlay, false,
               CheckCollisionPointRec(uiMouse, ui::moveLeftRect()));
    drawButton(ui::moveUpRect(), "^", canPlay, false,
               CheckCollisionPointRec(uiMouse, ui::moveUpRect()));
    drawButton(ui::moveDownRect(), "v", canPlay, false,
               CheckCollisionPointRec(uiMouse, ui::moveDownRect()));
    drawButton(ui::moveRightRect(), ">", canPlay, false,
               CheckCollisionPointRec(uiMouse, ui::moveRightRect()));
    drawButton(ui::boostRect(), "BOOST", canPlay, boostActive(),
               CheckCollisionPointRec(uiMouse, ui::boostRect()));
    drawButton(ui::restartRect(), "RESTART", over, false,
               CheckCollisionPointRec(uiMouse, ui::restartRect()));
    drawButton(ui::quitRect(), "QUIT", true, false,
               CheckCollisionPointRec(uiMouse, ui::quitRect()));

    // Keycap shortcut legend
    {
        const float ky = static_cast<float>(kBarY + 56);
        drawKeycap(kPanelX + 16, ky, "WASD", "Turn");
        drawKeycap(kPanelX + 130, ky, "SPACE", "Boost");
        drawKeycap(kPanelX + 252, ky, "R", "Restart");
        drawKeycap(kPanelX + 360, ky, "ESC", "Menu");
        drawKeycap(kPanelX + 472, ky, "F11", "Fullscreen");
    }

    // ==========================================
    // GAME OVER MODAL OVERLAY
    // ==========================================
    if (over) {
        // Darkened glass backdrop
        DrawRectangle(0, 0, kWinW, kWinH, Color{8, 10, 15, 205});

        const Rectangle modal{static_cast<float>(kWinW - 420) * 0.5f,
                              static_cast<float>(kWinH - 320) * 0.5f, 420.0f, 320.0f};

        // Outer glow & shadow
        DrawRectangleRounded(Rectangle{modal.x - 4, modal.y - 4, modal.width + 8, modal.height + 8},
                             0.12f, 10, Color{0, 0, 0, 110});
        DrawRectangleRounded(modal, 0.12f, 10, Color{22, 25, 36, 248});
        DrawRectangleRoundedLinesEx(modal, 0.12f, 10, 2.0f, Color{235, 75, 75, 220});

        // Header Title
        const char* title = "GAME OVER";
        constexpr int tSize = 38;
        const int tw = MeasureText(title, tSize);
        DrawText(title, static_cast<int>(modal.x + (modal.width - tw) * 0.5f),
                 static_cast<int>(modal.y + 24), tSize, Color{242, 85, 85, 255});

        // New High Score celebratory banner
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

        // Stat Card Panels (Final Score & High Score)
        const float cardY = isNewHighRecord ? (modal.y + 110) : (modal.y + 88);
        const float cardW = 175.0f;
        const float cardH = 76.0f;

        // Panel 1: Final Score
        const Rectangle p1{modal.x + 24, cardY, cardW, cardH};
        DrawRectangleRounded(p1, 0.2f, 6, Color{30, 34, 48, 255});
        DrawRectangleRoundedLinesEx(p1, 0.2f, 6, 1.0f, Color{55, 64, 88, 255});
        DrawText("FINAL SCORE", static_cast<int>(p1.x + 14), static_cast<int>(p1.y + 12), 11,
                 Color{150, 160, 180, 255});
        const std::string sVal = std::to_string(score);
        DrawText(sVal.c_str(), static_cast<int>(p1.x + 14), static_cast<int>(p1.y + 32), 28,
                 kGood);

        // Panel 2: Best Score
        const Rectangle p2{modal.x + modal.width - cardW - 24, cardY, cardW, cardH};
        DrawRectangleRounded(p2, 0.2f, 6, Color{30, 34, 48, 255});
        DrawRectangleRoundedLinesEx(p2, 0.2f, 6, 1.0f, Color{55, 64, 88, 255});
        DrawText("SESSION BEST", static_cast<int>(p2.x + 14), static_cast<int>(p2.y + 12), 11,
                 Color{180, 170, 130, 255});
        const std::string hVal = std::to_string(highScore);
        DrawText(hVal.c_str(), static_cast<int>(p2.x + 14), static_cast<int>(p2.y + 32), 28,
                 kGold);

        // Length achieved info
        const std::string lenText = "Final Snake Length: " + std::to_string(n);
        const int lenW = MeasureText(lenText.c_str(), 14);
        DrawText(lenText.c_str(), static_cast<int>(modal.x + (modal.width - lenW) * 0.5f),
                 static_cast<int>(cardY + cardH + 18), 14, Color{170, 180, 200, 255});

        // Action Prompts
        const float actionY = modal.y + modal.height - 48.0f;
        drawKeycap(modal.x + 45.0f, actionY, "R", "Play Again");
        drawKeycap(modal.x + 225.0f, actionY, "ESC", "Main Menu");
    }
}

} // namespace snake
