#include "game/audio.hpp"
#include "game/game.hpp"
#include "ui/layout.hpp"

#include <algorithm>

namespace snake {
namespace {

// Stable ids for the hovered control: the id only changes when the pointer enters
// a different control, which is what makes the hover blip fire once instead of
// every frame.
int hoverTarget(const Game& g, const Vector2& m) {
    if (m.x < 0.0f || m.y < 0.0f || m.x > static_cast<float>(kWinW) ||
        m.y > static_cast<float>(kWinH)) {
        return -1; // outside the letterboxed canvas
    }
    switch (g.mode) {
    case Mode::Menu:
        if (CheckCollisionPointRec(m, ui::menuPlayRect())) return 1001;
        if (CheckCollisionPointRec(m, ui::menuSkinsRect())) return 1002;
        if (CheckCollisionPointRec(m, ui::menuHighScoresRect())) return 1003;
        if (CheckCollisionPointRec(m, ui::menuSettingsRect())) return 1004;
        if (CheckCollisionPointRec(m, ui::menuFullscreenRect())) return 1005;
        if (CheckCollisionPointRec(m, ui::menuQuitRect())) return 1006;
        return -1;
    case Mode::HighScores:
        for (int i = 0; i < kMaxHighScores; ++i) {
            if (CheckCollisionPointRec(m, ui::highScoreRowRect(i))) return 3200 + i;
        }
        if (CheckCollisionPointRec(m, ui::highScoresBackRect())) return 3300;
        if (CheckCollisionPointRec(m, ui::highScoresClearRect())) return 3301;
        return -1;
    case Mode::Settings:
        for (int t = 0; t < kSettingTabCount; ++t) {
            if (CheckCollisionPointRec(m, ui::settingsTabRect(t))) return 2200 + t;
        }
        for (int i = 0; i < settingsRowCount(g.settingsTab); ++i) {
            if (CheckCollisionPointRec(m, ui::settingsRowRect(i))) return 2000 + i;
        }
        if (CheckCollisionPointRec(m, ui::settingsBackRect())) return 2100;
        return -1;
    case Mode::Skins:
        for (int i = 0; i < kSkinCount; ++i) {
            if (CheckCollisionPointRec(m, ui::skinCardRect(i))) return 3000 + i;
        }
        if (CheckCollisionPointRec(m, ui::skinsBackRect())) return 3100;
        return -1;
    case Mode::Playing:
        if (g.over) {
            return CheckCollisionPointRec(m, ui::restartRect()) ? 4001 : -1;
        }
        if (CheckCollisionPointRec(m, ui::pauseRect())) return 4002;
        if (g.paused) {
            return CheckCollisionPointRec(m, ui::pauseSettingsRect()) ? 4003 : -1;
        }
        if (CheckCollisionPointRec(m, ui::moveLeftRect()) ||
            CheckCollisionPointRec(m, ui::moveUpRect()) ||
            CheckCollisionPointRec(m, ui::moveDownRect()) ||
            CheckCollisionPointRec(m, ui::moveRightRect())) {
            return 4004;
        }
        if (CheckCollisionPointRec(m, ui::boostRect())) return 4005;
        if (CheckCollisionPointRec(m, ui::quitRect())) return 4006;
        return -1;
    }
    return -1;
}

} // namespace

void Game::handleInput() {
    // Name entry comes first and swallows every key: typing "M", "R" or "P" must
    // never mute, restart or pause while the player types a name.
    if (enteringName) {
        handleNameEntry();
        return;
    }
    if (IsKeyPressed(KEY_M)) {
        settings.mute = (settings.mute != 0) ? 0 : 1;
        applySettings();
        persistSettings();
    }
    if (IsKeyPressed(KEY_ESCAPE)) {
        audio.play(SfxId::MenuBack);
        if (mode == Mode::Settings || mode == Mode::Skins || mode == Mode::HighScores) {
            mode = (mode == Mode::Settings) ? settingsReturn : Mode::Menu;
            transitionT = animations ? 1.0f : 0.0f;
            return;
        }
        if (mode == Mode::Menu) {
            quit = true;
        } else {
            // Leaving a live round through ESC still records a qualifying score:
            // silently, with the last name, and never with a second entry.
            if (!over && !scoreSaved && board.qualifies(score)) {
                submitRoundScore(settings.playerName);
            }
            mode = Mode::Menu;
            paused = false;
            boostHeld = false;
            uiBoostHeld = false;
            transitionT = animations ? 1.0f : 0.0f;
        }
        return;
    }
    if (mode == Mode::HighScores) {
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
            if (clearArmed) {
                board.clear();
                board.save("highscores.cfg");
                highScore = board.best();
                highlightRank = -1;
                clearArmed = false;
                clearArmTimer = 0.0f;
                audio.play(SfxId::MenuBack);
            } else {
                audio.play(SfxId::MenuClick);
                mode = Mode::Menu;
                transitionT = animations ? 1.0f : 0.0f;
            }
        }
        return;
    }
    if (mode == Mode::Settings) {
        // Q / E / TAB switch tabs; each tab keeps its own row list.
        if (IsKeyPressed(KEY_TAB) || IsKeyPressed(KEY_Q) || IsKeyPressed(KEY_E)) {
            const int dir = IsKeyPressed(KEY_Q) ? -1 : 1;
            settingsTab = (settingsTab + dir + kSettingTabCount) % kSettingTabCount;
            settingsIndex = 0;
            audio.play(SfxId::MenuClick);
            return;
        }
        const int count = settingsRowCount(settingsTab);
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
            audio.play(SfxId::MenuClick);
            const int row = settingsRowAt(settingsTab, settingsIndex);
            if (row == RowReset) {
                // Reset every tab, but never the remembered player name.
                const std::string last = settings.playerName;
                settings = Settings{};
                settings.playerName = last;
            } else {
                adjustSetting(settings, row, delta);
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
            audio.play(SfxId::MenuClick);
            settings.skin = std::clamp(settings.skin + delta, 0, kSkinCount - 1);
            persistSettings();
        }
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_SPACE)) {
            audio.play(SfxId::MenuClick);
            mode = Mode::Menu;
            transitionT = animations ? 1.0f : 0.0f;
        }
        return;
    }
    if (mode == Mode::Menu) {
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_SPACE)) {
            audio.play(SfxId::MenuClick);
            startGame();
        }
        return;
    }
    if (over) {
        boostHeld = false;
        paused = false;
        if (IsKeyPressed(KEY_R)) {
            audio.play(SfxId::MenuClick);
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

    // Every key pressed this frame is offered to the two-slot queue (queueTurn does
    // the validation), so two quick presses in one frame are both kept.
    if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP)) {
        queueTurn(Dir::Up);
    }
    if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN)) {
        queueTurn(Dir::Down);
    }
    if (IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT)) {
        queueTurn(Dir::Left);
    }
    if (IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT)) {
        queueTurn(Dir::Right);
    }
}

// On-screen buttons: arrows, hold-to-boost, restart and quit.
void Game::handleUi(const Vector2& mouse, bool down, bool pressed) {
    uiMouse = mouse;
    uiBoostHeld = false;
    if (enteringName) {
        return; // clicks behind the name prompt must not restart or leave the round
    }

    const int hovered = hoverTarget(*this, mouse);
    if (hovered != hoverId) {
        hoverId = hovered;
        if (hovered >= 0) {
            audio.play(SfxId::MenuHover);
        }
    }

    if (mode == Mode::Settings) {
        if (pressed) {
            if (CheckCollisionPointRec(mouse, ui::settingsBackRect())) {
                audio.play(SfxId::MenuBack);
                mode = settingsReturn;
                transitionT = animations ? 1.0f : 0.0f;
                return;
            }
            for (int t = 0; t < kSettingTabCount; ++t) {
                if (CheckCollisionPointRec(mouse, ui::settingsTabRect(t))) {
                    audio.play(SfxId::MenuClick);
                    settingsTab = t;
                    settingsIndex = 0;
                    return;
                }
            }
            const int rowCount = settingsRowCount(settingsTab);
            for (int i = 0; i < rowCount; ++i) {
                const Rectangle row = ui::settingsRowRect(i);
                if (!CheckCollisionPointRec(mouse, row)) {
                    continue;
                }
                settingsIndex = i;
                audio.play(SfxId::MenuClick);
                const int delta = (mouse.x < row.x + row.width * 0.5f) ? -1 : 1;
                const int rowId = settingsRowAt(settingsTab, i);
                if (rowId == RowReset) {
                    const std::string last = settings.playerName;
                    settings = Settings{};
                    settings.playerName = last;
                } else {
                    adjustSetting(settings, rowId, delta);
                }
                applySettings();
                persistSettings();
                return;
            }
        }
        return;
    }
    if (mode == Mode::HighScores) {
        if (pressed) {
            if (CheckCollisionPointRec(mouse, ui::highScoresBackRect())) {
                audio.play(SfxId::MenuBack);
                clearArmed = false;
                mode = Mode::Menu;
                transitionT = animations ? 1.0f : 0.0f;
                return;
            }
            if (CheckCollisionPointRec(mouse, ui::highScoresClearRect())) {
                if (clearArmed) {
                    board.clear();
                    board.save("highscores.cfg");
                    highScore = board.best();
                    highlightRank = -1;
                    clearArmed = false;
                    clearArmTimer = 0.0f;
                    audio.play(SfxId::MenuBack);
                } else {
                    clearArmed = true;
                    clearArmTimer = 3.0f;
                    audio.play(SfxId::MenuClick);
                }
            }
        }
        return;
    }
    if (mode == Mode::Skins) {
        if (pressed) {
            if (CheckCollisionPointRec(mouse, ui::skinsBackRect())) {
                audio.play(SfxId::MenuBack);
                mode = Mode::Menu;
                transitionT = animations ? 1.0f : 0.0f;
                return;
            }
            for (int i = 0; i < kSkinCount; ++i) {
                if (CheckCollisionPointRec(mouse, ui::skinCardRect(i))) {
                    audio.play(SfxId::MenuClick);
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
                audio.play(SfxId::MenuClick);
                startGame();
            } else if (CheckCollisionPointRec(mouse, ui::menuSkinsRect())) {
                audio.play(SfxId::MenuClick);
                settingsReturn = Mode::Menu;
                mode = Mode::Skins;
                transitionT = animations ? 1.0f : 0.0f;
            } else if (CheckCollisionPointRec(mouse, ui::menuHighScoresRect())) {
                audio.play(SfxId::MenuClick);
                clearArmed = false;
                mode = Mode::HighScores;
                transitionT = animations ? 1.0f : 0.0f;
            } else if (CheckCollisionPointRec(mouse, ui::menuSettingsRect())) {
                audio.play(SfxId::MenuClick);
                settingsReturn = Mode::Menu;
                settingsIndex = 0;
                mode = Mode::Settings;
                transitionT = animations ? 1.0f : 0.0f;
            } else if (CheckCollisionPointRec(mouse, ui::menuFullscreenRect())) {
                audio.play(SfxId::MenuClick);
                fsRequest = true;
            } else if (CheckCollisionPointRec(mouse, ui::menuQuitRect())) {
                audio.play(SfxId::MenuClick);
                quit = true;
            }
        }
        return;
    }
    if (over) {
        // The game-over modal covers the whole window: the control bar underneath
        // must not receive clicks through it.
        if (pressed && CheckCollisionPointRec(mouse, ui::restartRect())) {
            audio.play(SfxId::MenuClick);
            reset();
        }
        return;
    }
    {
        if (pressed && CheckCollisionPointRec(mouse, ui::pauseRect())) {
            togglePause();
        }
        if (paused && pressed && CheckCollisionPointRec(mouse, ui::pauseSettingsRect())) {
            audio.play(SfxId::MenuClick);
            settingsReturn = Mode::Playing;
            settingsIndex = 0;
            mode = Mode::Settings;
            transitionT = animations ? 1.0f : 0.0f;
            return;
        }
        if (!paused) {
            uiBoostHeld = down && CheckCollisionPointRec(mouse, ui::boostRect());
            if (pressed) {
                audio.play(SfxId::MenuClick);
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
        audio.play(SfxId::MenuClick);
        quit = true;
    }
}

} // namespace snake
