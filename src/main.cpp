// Snake game entry point (C++17, raylib).
//
// Build (Linux/macOS):
//   g++ -std=c++17 -Wall -Wextra -Isrc src/main.cpp src/game/game.cpp
//   src/ui/input.cpp src/ui/draw.cpp src/render/renderer.cpp -o build/snake_gui
//   $(pkg-config --cflags --libs raylib)
// Build (Windows/MinGW):
//   g++ -std=c++17 -Isrc src/main.cpp src/game/game.cpp src/ui/input.cpp
//   src/ui/draw.cpp src/render/renderer.cpp -o build/snake_gui.exe $(pkg-config
//   --cflags --libs raylib)
//
// 16:9 canvas (960x540), with a 32x18 grid and centered 640x360 arena.
// F11 or the menu button toggles fullscreen. While playing, ESC returns to the
// menu. Controls: WASD/arrows turn, hold SPACE for boost, R restarts after
// game over. The bottom bar exposes clickable controls.

#include "game/audio.hpp"
#include "game/game.hpp"

#include <algorithm>

using namespace snake;

int main() {
    InitWindow(kWinW, kWinH, "Snake");
    SetExitKey(KEY_NULL); // ESC is handled by the game (menu/quit), not the window
    SetTargetFPS(60);
    audio.init(); // no device / no assets = silent, the game still runs

    // Fixed-size virtual canvas, letterboxed to the real window (fullscreen too).
    RenderTexture2D canvas = LoadRenderTexture(kWinW, kWinH);
    SetTextureFilter(canvas.texture, TEXTURE_FILTER_BILINEAR);

    Game game;
    while (!game.quit && !WindowShouldClose()) {
        const float dt = GetFrameTime();

        if (IsKeyPressed(KEY_F11) || game.takeFullscreenRequest()) {
            ToggleFullscreen();
            // Keep the persisted option in sync with the F11 shortcut.
            game.settings.fullscreen = IsWindowFullscreen() ? 1 : 0;
            game.persistSettings();
        }
        if (game.windowSettingsDirty) {
            game.applyWindowSettings();
            game.windowSettingsDirty = false;
        }

        const float screenW = static_cast<float>(GetScreenWidth());
        const float screenH = static_cast<float>(GetScreenHeight());
        float scale =
            std::min(screenW / static_cast<float>(kWinW), screenH / static_cast<float>(kWinH));
        if (scale <= 0.0f) {
            scale = 1.0f;
        }
        const Vector2 offset{(screenW - static_cast<float>(kWinW) * scale) * 0.5f,
                             (screenH - static_cast<float>(kWinH) * scale) * 0.5f};
        const Vector2 mouse = GetMousePosition();
        // Points outside the letterboxed canvas are not part of the UI: map them off-screen
        // so a click in the black bars can never hit a button.
        const float canvasW = static_cast<float>(kWinW) * scale;
        const float canvasH = static_cast<float>(kWinH) * scale;
        const bool inCanvas = mouse.x >= offset.x && mouse.x <= offset.x + canvasW &&
                              mouse.y >= offset.y && mouse.y <= offset.y + canvasH;
        const Vector2 virtualMouse = inCanvas
                                         ? Vector2{(mouse.x - offset.x) / scale,
                                                   (mouse.y - offset.y) / scale}
                                         : Vector2{-1000.0f, -1000.0f};

        game.handleUi(virtualMouse, IsMouseButtonDown(MOUSE_BUTTON_LEFT),
                      IsMouseButtonPressed(MOUSE_BUTTON_LEFT));
        game.handleInput();
        game.stepFrame(dt);

        // Menu music outside a run, gameplay music inside it, ducked on pause/over.
        const MusicTrack wanted =
            (game.mode == Mode::Playing) ? MusicTrack::Gameplay : MusicTrack::Menu;
        float duck = 1.0f;
        if (game.mode == Mode::Playing && game.paused) {
            duck = 0.3f;
        } else if (game.mode == Mode::Playing && game.over) {
            duck = 0.4f;
        }
        audio.update(wanted, duck, dt);

        BeginTextureMode(canvas);
        game.draw();
        EndTextureMode();

        BeginDrawing();
        ClearBackground(BLACK);
        DrawTexturePro(canvas.texture,
                       Rectangle{0.0f, 0.0f, static_cast<float>(kWinW), -static_cast<float>(kWinH)},
                       Rectangle{offset.x, offset.y, static_cast<float>(kWinW) * scale,
                                 static_cast<float>(kWinH) * scale},
                       Vector2{0.0f, 0.0f}, 0.0f, WHITE);
        EndDrawing();
    }

    audio.shutdown();
    UnloadRenderTexture(canvas);
    CloseWindow();
    return 0;
}
