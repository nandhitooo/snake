#include "game/game.hpp"
#include "render/color.hpp"
#include "ui/layout.hpp"

namespace snake {

void Game::drawButton(const Rectangle& r, const char* label, bool enabled, bool active,
                      bool hovered) const {
    Color fill{40, 44, 56, 255};
    if (active) {
        fill = Color{54, 96, 72, 255};
    }
    if (hovered && enabled) {
        fill = Color{58, 64, 82, 255};
    }
    if (!enabled) {
        fill = Color{28, 30, 38, 255};
    }
    DrawRectangleRounded(r, 0.28f, 8, fill);
    DrawRectangleLinesEx(r, 2.0f, (hovered && enabled) ? kBorder : Color{58, 62, 76, 255});

    const int fontSize = 20;
    const int textW = MeasureText(label, fontSize);
    const Color textColor = enabled ? kText : Color{110, 114, 126, 255};
    DrawText(label, static_cast<int>(r.x + (r.width - static_cast<float>(textW)) / 2.0f),
             static_cast<int>(r.y + (r.height - static_cast<float>(fontSize)) / 2.0f), fontSize,
             textColor);
}

void Game::drawMenu() const {
    DrawRectangleGradientV(0, 0, kWinW, kWinH, Color{17, 21, 26, 255}, Color{9, 10, 14, 255});
    for (int x = kCell; x < kWinW; x += kCell) {
        DrawLine(x, 0, x, kWinH, Color{27, 31, 39, 255});
    }
    for (int y = kCell; y < kWinH; y += kCell) {
        DrawLine(0, y, kWinW, y, Color{27, 31, 39, 255});
    }

    const char* title = "SNAKE";
    const int titleSize = 88;
    const int titleW = MeasureText(title, titleSize);
    const int titleX = (kWinW - titleW) / 2;
    DrawText(title, titleX + 4, 124, titleSize, Color{28, 76, 58, 255});
    DrawText(title, titleX, 120, titleSize, kSnakeHead);

    const char* subtitle = "raylib edition";
    DrawText(subtitle, (kWinW - MeasureText(subtitle, 20)) / 2, 216, 20, GRAY);

    // Small snake-and-food motif under the title.
    for (int i = 0; i < 7; ++i) {
        const float t = static_cast<float>(i) / 6.0f;
        DrawRectangleRounded(Rectangle{248.0f + static_cast<float>(i * 30), 262.0f, 22.0f, 22.0f},
                             0.35f, 8, render::lerpColor(kSnakeTail, kSnakeHead, t));
    }
    const float headX = 248.0f + 6.0f * 30.0f;
    DrawCircle(static_cast<int>(headX) + 15, 268, 2.2f, kEye);
    DrawCircle(static_cast<int>(headX) + 15, 278, 2.2f, kEye);
    DrawCircle(248 + 7 * 30 + 12, 273, 7.0f, kFood);

    const bool fs = IsWindowFullscreen();
    drawButton(ui::menuPlayRect(), "PLAY", true, false,
               CheckCollisionPointRec(uiMouse, ui::menuPlayRect()));
    drawButton(ui::menuFullscreenRect(), fs ? "FULLSCREEN: ON" : "FULLSCREEN: OFF", true, fs,
               CheckCollisionPointRec(uiMouse, ui::menuFullscreenRect()));
    drawButton(ui::menuQuitRect(), "QUIT", true, false,
               CheckCollisionPointRec(uiMouse, ui::menuQuitRect()));

    if (highScore > 0) {
        const std::string best = "Best this session: " + std::to_string(highScore);
        DrawText(best.c_str(), (kWinW - MeasureText(best.c_str(), 16)) / 2, 548, 16, GRAY);
    }
    const char* hint = "ENTER / PLAY to start  |  F11 fullscreen  |  ESC quit";
    DrawText(hint, (kWinW - MeasureText(hint, 14)) / 2, 586, 14, Color{120, 124, 136, 255});
}

} // namespace snake
