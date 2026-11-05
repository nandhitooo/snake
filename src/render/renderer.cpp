#include "game/game.hpp"
#include "render/color.hpp"
#include "ui/layout.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace snake {
namespace {

Rectangle cellRect(const Cell& c, float inset) {
    return Rectangle{
        static_cast<float>(c.x * kCell) + inset, static_cast<float>(kHud + c.y * kCell) + inset,
        static_cast<float>(kCell) - inset * 2.0f, static_cast<float>(kCell) - inset * 2.0f};
}

} // namespace

void Game::drawEyes() const {
    if (snake.empty()) {
        return;
    }
    const Cell& h = snake.front();
    const float cx = static_cast<float>(h.x * kCell + kCell / 2);
    const float cy = static_cast<float>(kHud + h.y * kCell + kCell / 2);
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
    DrawCircleV(Vector2{cx + dx * 5.0f + px * 5.0f, cy + dy * 5.0f + py * 5.0f}, 2.6f, kEye);
    DrawCircleV(Vector2{cx + dx * 5.0f - px * 5.0f, cy + dy * 5.0f - py * 5.0f}, 2.6f, kEye);
}

void Game::drawGhostHead(const GhostHead& g) const {
    const float fade = (g.maxLife > 0.0f) ? (g.life / g.maxLife) : 0.0f;
    const unsigned char alpha = static_cast<unsigned char>(160.0f * fade);

    Color body = kSnakeHead;
    body.a = alpha;
    DrawRectangleRounded(cellRect(g.cell, 1.5f), 0.35f, 8, body);

    const float cx = static_cast<float>(g.cell.x * kCell + kCell / 2);
    const float cy = static_cast<float>(kHud + g.cell.y * kCell + kCell / 2);
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
    Color eye = kEye;
    eye.a = alpha;
    DrawCircleV(Vector2{cx + dx * 5.0f + px * 5.0f, cy + dy * 5.0f + py * 5.0f}, 2.6f, eye);
    DrawCircleV(Vector2{cx + dx * 5.0f - px * 5.0f, cy + dy * 5.0f - py * 5.0f}, 2.6f, eye);
}

void Game::draw() const {
    ClearBackground(kBg);
    if (mode == Mode::Menu) {
        drawMenu();
        return;
    }

    // Arena background and subtle grid.
    DrawRectangle(0, kHud, kArenaW, kArenaH, kArenaBg);
    for (int x = 1; x < kCols; ++x) {
        DrawLine(x * kCell, kHud, x * kCell, kHud + kArenaH, kGrid);
    }
    for (int y = 1; y < kRows; ++y) {
        DrawLine(0, kHud + y * kCell, kArenaW, kHud + y * kCell, kGrid);
    }

    DrawCircle(food.x * kCell + kCell / 2, kHud + food.y * kCell + kCell / 2, 8.0f, kFood);

    if (hasSpecial) {
        const float pulse = 0.5f + 0.5f * std::sin(GetTime() * 6.0);
        const int sx = specialFood.x * kCell + kCell / 2;
        const int sy = kHud + specialFood.y * kCell + kCell / 2;
        DrawCircle(sx, sy, 10.5f + 2.0f * pulse,
                   Color{kSpecialHalo.r, kSpecialHalo.g, kSpecialHalo.b, 70});
        DrawCircle(sx, sy, 8.0f, kSpecial);
        DrawCircleLines(sx, sy, 9.5f + 1.5f * pulse, kSpecialHalo);
    }

    for (const Obstacle& o : obstacles) {
        Color color = kObstacle;
        if (o.life < kObstacleFade) {
            const float fade = std::max(0.0f, o.life / kObstacleFade);
            color.a = static_cast<unsigned char>(255.0f * fade);
        }
        DrawRectangleRounded(cellRect(o.cell, 2.0f), 0.18f, 6, color);
    }

    // Warning phase: flashing, not yet deadly.
    const float blink = 0.5f + 0.5f * std::sin(GetTime() * 18.0);
    const unsigned char alpha = static_cast<unsigned char>(90.0f + 130.0f * blink);
    for (const Warning& w : warnings) {
        const Rectangle r = cellRect(w.cell, 2.0f);
        DrawRectangleRounded(r, 0.18f, 6, Color{kWarning.r, kWarning.g, kWarning.b, alpha});
        DrawRectangleLinesEx(r, 2.0f, Color{kWarning.r, kWarning.g, kWarning.b, 220});
    }

    // Snake body with head-to-tail gradient; head drawn last with eyes.
    const size_t n = snake.size();
    for (size_t i = n; i-- > 0;) {
        const float t = (n > 1) ? static_cast<float>(i) / static_cast<float>(n - 1) : 0.0f;
        DrawRectangleRounded(cellRect(snake[i], 1.5f), 0.35f, 8,
                             render::lerpColor(kSnakeHead, kSnakeTail, t));
    }
    drawEyes();

    // Fading ghost heads marking both sides of a wrap.
    for (const GhostHead& g : ghosts) {
        drawGhostHead(g);
    }

    // Visible arena frame.
    DrawRectangleLinesEx(Rectangle{0.0f, static_cast<float>(kHud), static_cast<float>(kArenaW),
                                   static_cast<float>(kArenaH)},
                         2.0f, kBorder);

    // HUD bar.
    DrawRectangle(0, 0, kArenaW, kHud, kHudBg);
    DrawLine(0, kHud, kArenaW, kHud, kBorder);
    DrawText(TextFormat("Score: %d", score), 16, 12, 20, kText);
    DrawText(TextFormat("High Score: %d", highScore), 176, 12, 20, kText);
    DrawText(TextFormat("Length: %d", static_cast<int>(n)), 376, 12, 20, kText);
    DrawText(TextFormat("Boost: %s", boostActive() ? "ON" : "OFF"), 516, 12, 20,
             boostActive() ? kBoostOn : GRAY);

    if (effectTimer > 0.0f) {
        DrawText(
            TextFormat("FX: %s %.1fs", effectName(activeEffect), static_cast<double>(effectTimer)),
            16, 38, 14, fxColor);
    } else if (fxTextTimer > 0.0f) {
        DrawText(TextFormat("FX: %s", fxText.c_str()), 16, 38, 14, fxColor);
    } else {
        DrawText("FX: none", 16, 38, 14, GRAY);
    }

    // GUI control bar: clickable buttons; keyboard still works.
    DrawRectangle(0, kBarY, kArenaW, kBarH, kHudBg);
    DrawLine(0, kBarY, kArenaW, kBarY, kBorder);

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

    DrawText("F11 fullscreen  |  WASD / arrows turn  |  SPACE boost  |  R "
             "restart  |  ESC menu",
             16, kBarY + 66, 12, GRAY);

    if (over) {
        DrawRectangle(0, kHud, kArenaW, kArenaH, Color{0, 0, 0, 180});

        const char* title = "GAME OVER";
        DrawText(title, (kArenaW - MeasureText(title, 44)) / 2, kHud + 150, 44, kText);

        const std::string finalText = "Final score: " + std::to_string(score);
        DrawText(finalText.c_str(), (kArenaW - MeasureText(finalText.c_str(), 24)) / 2, kHud + 220,
                 24, kText);

        const std::string highText = "High score: " + std::to_string(highScore);
        DrawText(highText.c_str(), (kArenaW - MeasureText(highText.c_str(), 24)) / 2, kHud + 256,
                 24, kText);

        const char* prompt = "Press R to restart, ESC for menu";
        DrawText(prompt, (kArenaW - MeasureText(prompt, 20)) / 2, kHud + 320, 20, LIGHTGRAY);
    }
}

} // namespace snake
