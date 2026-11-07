#include "game/game.hpp"
#include "ui/layout.hpp"

namespace snake {

void Game::handleInput() {
    if (IsKeyPressed(KEY_ESCAPE)) {
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
    if (mode == Mode::Menu) {
        if (pressed) {
            if (CheckCollisionPointRec(mouse, ui::menuPlayRect())) {
                startGame();
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
