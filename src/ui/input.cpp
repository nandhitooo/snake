#include "game/game.hpp"
#include "ui/layout.hpp"

#include <algorithm>

namespace snake {

void Game::handleInput() {
    if (IsKeyPressed(KEY_ESCAPE)) {
        if (mode == Mode::Settings || mode == Mode::Skins) {
            mode = (mode == Mode::Settings) ? settingsReturn : Mode::Menu;
            transitionT = animations ? 1.0f : 0.0f;
            return;
        }
        if (mode == Mode::Menu) {
            quit = true;
        } else {
            if (score > highScore) {
                highScore = score;
            }
            mode = Mode::Menu;
            paused = false;
            boostHeld = false;
            uiBoostHeld = false;
            transitionT = animations ? 1.0f : 0.0f;
        }
        return;
    }
    if (mode == Mode::Settings) {
        const int count = kSettingRowCount;
        if (IsKeyPressed(KEY_UP)) {
            settingsIndex = (settingsIndex + count - 1) % count;
        }
        if (IsKeyPressed(KEY_DOWN)) {
            settingsIndex = (settingsIndex + 1) % count;
        }
        int delta = 0;
        if (IsKeyPressed(KEY_LEFT)) {
            delta = -1;
        }
        if (IsKeyPressed(KEY_RIGHT)) {
            delta = 1;
        }
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_SPACE)) {
            delta = 1;
        }
        if (delta != 0) {
            if (settingsIndex == RowReset) {
                settings = Settings{};
            } else {
                adjustSetting(settings, settingsIndex, delta);
            }
            applySettings();
            persistSettings();
        }
        return;
    }
    if (mode == Mode::Skins) {
        int delta = 0;
        if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_UP)) {
            delta = -1;
        }
        if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_DOWN)) {
            delta = 1;
        }
        if (delta != 0) {
            settings.skin = std::clamp(settings.skin + delta, 0, kSkinCount - 1);
            persistSettings();
        }
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_SPACE)) {
            mode = Mode::Menu;
            transitionT = animations ? 1.0f : 0.0f;
        }
        return;
    }
    if (mode == Mode::Menu) {
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_SPACE)) {
            startGame();
        }
        return;
    }
    if (over) {
        boostHeld = false;
        paused = false;
        if (IsKeyPressed(KEY_R)) {
            reset();
        }
        return;
    }

    if (IsKeyPressed(KEY_P)) {
        togglePause();
        return;
    }

    if (IsKeyPressed(KEY_C)) {
        cameraMode = (cameraMode == CameraMode::Iso)   ? CameraMode::Top
                     : (cameraMode == CameraMode::Top) ? CameraMode::Third
                                                       : CameraMode::Iso;
    }

    if (paused) {
        return;
    }

    if (boostRequiresRelease) {
        // Ignore a key still held from the menu / previous round until it is released.
        if (!IsKeyDown(KEY_SPACE) && !uiBoostHeld) {
            boostRequiresRelease = false;
        }
        boostHeld = false;
    } else {
        boostHeld = IsKeyDown(KEY_SPACE) || uiBoostHeld;
    }

    if (hasBuffered) {
        return; // one buffered turn per tick at most
    }
    Dir wanted = dir;
    bool pressed = false;
    if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP)) {
        wanted = Dir::Up;
        pressed = true;
    } else if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN)) {
        wanted = Dir::Down;
        pressed = true;
    } else if (IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT)) {
        wanted = Dir::Left;
        pressed = true;
    } else if (IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT)) {
        wanted = Dir::Right;
        pressed = true;
    }
    if (pressed) {
        queueTurn(wanted);
    }
}

// On-screen buttons: arrows, hold-to-boost, restart and quit.
void Game::handleUi(const Vector2& mouse, bool down, bool pressed) {
    uiMouse = mouse;
    uiBoostHeld = false;

    if (mode == Mode::Settings) {
        if (pressed) {
            if (CheckCollisionPointRec(mouse, ui::settingsBackRect())) {
                mode = settingsReturn;
                transitionT = animations ? 1.0f : 0.0f;
                return;
            }
            for (int i = 0; i < kSettingRowCount; ++i) {
                const Rectangle row = ui::settingsRowRect(i);
                if (!CheckCollisionPointRec(mouse, row)) {
                    continue;
                }
                settingsIndex = i;
                const int delta = (mouse.x < row.x + row.width * 0.5f) ? -1 : 1;
                if (i == RowReset) {
                    settings = Settings{};
                } else {
                    adjustSetting(settings, i, delta);
                }
                applySettings();
                persistSettings();
                return;
            }
        }
        return;
    }
    if (mode == Mode::Skins) {
        if (pressed) {
            if (CheckCollisionPointRec(mouse, ui::skinsBackRect())) {
                mode = Mode::Menu;
                transitionT = animations ? 1.0f : 0.0f;
                return;
            }
            for (int i = 0; i < kSkinCount; ++i) {
                if (CheckCollisionPointRec(mouse, ui::skinCardRect(i))) {
                    settings.skin = i;
                    persistSettings();
                    return;
                }
            }
        }
        return;
    }

    if (mode == Mode::Menu) {
        if (pressed) {
            if (CheckCollisionPointRec(mouse, ui::menuPlayRect())) {
                startGame();
            } else if (CheckCollisionPointRec(mouse, ui::menuSkinsRect())) {
                settingsReturn = Mode::Menu;
                mode = Mode::Skins;
                transitionT = animations ? 1.0f : 0.0f;
            } else if (CheckCollisionPointRec(mouse, ui::menuSettingsRect())) {
                settingsReturn = Mode::Menu;
                settingsIndex = 0;
                mode = Mode::Settings;
                transitionT = animations ? 1.0f : 0.0f;
            } else if (CheckCollisionPointRec(mouse, ui::menuFullscreenRect())) {
                fsRequest = true;
            } else if (CheckCollisionPointRec(mouse, ui::menuQuitRect())) {
                quit = true;
            }
        }
        return;
    }
    if (over) {
        // The game-over modal covers the whole window: the control bar underneath
        // must not receive clicks through it.
        if (pressed && CheckCollisionPointRec(mouse, ui::restartRect())) {
            reset();
        }
        return;
    }
    {
        if (pressed && CheckCollisionPointRec(mouse, ui::pauseRect())) {
            togglePause();
        }
        if (paused && pressed && CheckCollisionPointRec(mouse, ui::pauseSettingsRect())) {
            settingsReturn = Mode::Playing;
            settingsIndex = 0;
            mode = Mode::Settings;
            transitionT = animations ? 1.0f : 0.0f;
            return;
        }
        if (!paused) {
            uiBoostHeld = down && CheckCollisionPointRec(mouse, ui::boostRect());
            if (pressed) {
                if (CheckCollisionPointRec(mouse, ui::moveLeftRect())) {
                    queueTurn(Dir::Left);
                } else if (CheckCollisionPointRec(mouse, ui::moveUpRect())) {
                    queueTurn(Dir::Up);
                } else if (CheckCollisionPointRec(mouse, ui::moveDownRect())) {
                    queueTurn(Dir::Down);
                } else if (CheckCollisionPointRec(mouse, ui::moveRightRect())) {
                    queueTurn(Dir::Right);
                }
            }
        }
    }
    if (mode == Mode::Playing && !over && pressed && CheckCollisionPointRec(mouse, ui::quitRect())) {
        quit = true;
    }
}

} // namespace snake
