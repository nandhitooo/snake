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

#include "game/assets.hpp"
#include "game/audio.hpp"
#include "game/game.hpp"
#include "ui/text.hpp"

#include "rlgl.h"

#include <algorithm>
#include <cmath>
#include <string>

using namespace snake;

namespace {

bool shaderUsable(const Shader& shader) {
#if RAYLIB_VERSION_MAJOR > 5 || (RAYLIB_VERSION_MAJOR == 5 && RAYLIB_VERSION_MINOR >= 5)
    return IsShaderValid(shader);
#else
    return IsShaderReady(shader);
#endif
}

// Phase F1: S = fitScale x quality, snapped to kRenderScaleStep and clamped.
float computeRenderScale(int quality) {
    const float fit = std::min(static_cast<float>(GetRenderWidth()) / static_cast<float>(kWinW),
                               static_cast<float>(GetRenderHeight()) / static_cast<float>(kWinH));
    float s = fit * renderQualityScale(quality);
    s = std::round(s / kRenderScaleStep) * kRenderScaleStep;
    s = std::clamp(s, kRenderScaleMin, kRenderScaleMax);
    const float maxByTexture =
        static_cast<float>(kRenderMaxTexSide) / static_cast<float>(std::max(kWinW, kWinH));
    s = std::min(s, std::floor(maxByTexture / kRenderScaleStep) * kRenderScaleStep);
    return std::max(s, kRenderScaleStep);
}

} // namespace

int main() {
    // VSync is more reliable as a creation-time flag than as a state change afterwards.
    // FLAG_WINDOW_HIGHDPI is deliberately NOT set: it would break the mouse mapping.
    const Settings boot = loadSettings("settings.cfg");
    unsigned int flags = FLAG_WINDOW_RESIZABLE;
    if (boot.vsync != 0) {
        flags |= FLAG_VSYNC_HINT;
    }
    SetConfigFlags(flags);
    InitWindow(kWinW, kWinH, "Snake");
    SetExitKey(KEY_NULL); // ESC is handled by the game (menu/quit), not the window
    // Once, for every BeginMode3D (game, skin preview, settings): see kClipNear/kClipFar.
    rlSetClipPlanes(kClipNear, kClipFar);
    SetTargetFPS(boot.vsync != 0 ? 0 : fpsTarget(boot.fpsLimit));
    audio.init(); // no device / no assets = silent, the game still runs

    // Virtual 960x540 canvas, rendered at S x that size and letterboxed to the window.
    // The texture is recreated only when its target size changes (never per frame).
    RenderTexture2D canvas{};
    bool canvasValid = false;
    int canvasPxW = 0;
    int canvasPxH = 0;

    // Post-process (Phase H3): one pass doing FXAA + bloom + vignette + chromatic
    // aberration. A failed load falls back to the plain FXAA shader, then to no
    // shader at all: any failure just degrades the look, never stops the game.
    Shader post{};
    bool postReady = false;
    bool postFull = false;
    int postResLoc = -1;
    int postFxaaLoc = -1;
    int postBloomLoc = -1;
    int postVignetteLoc = -1;
    int postBoostLoc = -1;
    int postFlashLoc = -1;
    int postTimeLoc = -1;
    {
        const auto loadFrom = [](const char* relative, Shader& out) {
            const std::string path = resolveAssetPath(relative);
            if (path.empty()) {
                return false;
            }
            SetTraceLogLevel(LOG_NONE);
            out = LoadShader(nullptr, path.c_str());
            SetTraceLogLevel(LOG_INFO);
            return shaderUsable(out);
        };

        Shader candidate{};
        if (loadFrom("assets/shaders/post.fs", candidate)) {
            post = candidate;
            postFull = true;
            postResLoc = GetShaderLocation(post, "resolution");
            postReady = postResLoc >= 0;
            postFxaaLoc = GetShaderLocation(post, "fxaaOn");
            postBloomLoc = GetShaderLocation(post, "bloomStrength");
            postVignetteLoc = GetShaderLocation(post, "vignetteStrength");
            postBoostLoc = GetShaderLocation(post, "boost");
            postFlashLoc = GetShaderLocation(post, "flash");
            postTimeLoc = GetShaderLocation(post, "time");
        } else if (loadFrom("assets/shaders/fxaa.fs", candidate)) {
            post = candidate;
            postResLoc = GetShaderLocation(post, "resolution");
            postReady = postResLoc >= 0;
        }
    }

    Game game;
    while (!game.quit && !WindowShouldClose()) {
        const float dt = GetFrameTime();

        game.applyPendingWindowResize(); // requested by last frame's fullscreen switch
        if (IsKeyPressed(KEY_F11) || game.takeFullscreenRequest()) {
            game.toggleFullscreen();
        }
        if (game.windowSettingsDirty) {
            game.applyWindowSettings();
            game.windowSettingsDirty = false;
        }

        // Minimized (zero-sized) window: nothing to render, just keep polling events.
        if (GetScreenWidth() <= 0 || GetScreenHeight() <= 0 || GetRenderWidth() <= 0 ||
            GetRenderHeight() <= 0) {
            PollInputEvents();
            WaitTime(0.05);
            continue;
        }

        const float renderS = computeRenderScale(game.settings.renderQuality);
        const int texW = static_cast<int>(std::lround(static_cast<float>(kWinW) * renderS));
        const int texH = static_cast<int>(std::lround(static_cast<float>(kWinH) * renderS));
        if (!canvasValid || texW != canvasPxW || texH != canvasPxH) {
            if (canvasValid) {
                UnloadRenderTexture(canvas);
            }
            canvas = LoadRenderTexture(texW, texH);
            SetTextureFilter(canvas.texture, TEXTURE_FILTER_BILINEAR);
            canvasValid = canvas.id != 0;
            canvasPxW = texW;
            canvasPxH = texH;
            if (postReady && postResLoc >= 0) {
                const float res[2] = {static_cast<float>(texW), static_cast<float>(texH)};
                SetShaderValue(post, postResLoc, res, SHADER_UNIFORM_VEC2);
            }
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
        game.uiScale = renderS;
        applyUiScale(renderS);     // virtual 960x540 coordinates -> S x texture pixels
        rlSetLineWidth(renderS);   // 1-px GL lines would otherwise get thinner with S

        game.draw();
        if (game.settings.showFps != 0) {
            const char* fps = TextFormat("FPS: %d", GetFPS());
            constexpr float fpsSize = 10.0f;
            const float fpsW = uiMeasure(fps, fpsSize);
            uiText(fps, static_cast<float>(kWinW) - fpsW - 8.0f, 6.0f, fpsSize,
                   Color{150, 205, 160, 220});
        }
        EndTextureMode();
        rlSetLineWidth(1.0f);

        // Per-frame post uniforms. A uniform the driver optimised away reports -1
        // and is skipped, so the shader can evolve without breaking the game.
        if (postReady && postFull) {
            const auto setFloat = [&post](int loc, float value) {
                if (loc >= 0) {
                    SetShaderValue(post, loc, &value, SHADER_UNIFORM_FLOAT);
                }
            };
            setFloat(postFxaaLoc, game.settings.fxaa != 0 ? 1.0f : 0.0f);
            const float bloom = (game.settings.bloom == 2)   ? 1.8f
                                : (game.settings.bloom == 1) ? 0.9f
                                                             : 0.0f;
            setFloat(postBloomLoc, bloom);
            setFloat(postVignetteLoc, game.settings.vignette != 0 ? 0.55f : 0.0f);
            setFloat(postBoostLoc, game.boostVisual);
            setFloat(postFlashLoc, game.flashPulse);
            setFloat(postTimeLoc, static_cast<float>(GetTime()));
        }

        // With only fxaa.fs available the FXAA row still decides whether it runs.
        const bool usePost = postReady && (postFull || game.settings.fxaa != 0);
        BeginDrawing();
        ClearBackground(BLACK);
        if (usePost) {
            BeginShaderMode(post);
        }
        DrawTexturePro(canvas.texture,
                       Rectangle{0.0f, 0.0f, static_cast<float>(canvasPxW),
                                 -static_cast<float>(canvasPxH)},
                       Rectangle{offset.x, offset.y, static_cast<float>(kWinW) * scale,
                                 static_cast<float>(kWinH) * scale},
                       Vector2{0.0f, 0.0f}, 0.0f, WHITE);
        if (usePost) {
            EndShaderMode();
        }
        EndDrawing();
    }

    audio.shutdown();
    uiTextShutdown();
    if (postReady) {
        UnloadShader(post);
    }
    if (canvasValid) {
        UnloadRenderTexture(canvas);
    }
    CloseWindow();
    return 0;
}
