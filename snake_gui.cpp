// snake_gui.cpp - raylib Snake game (C++17, raylib + standard library only).
//
// Build (Linux/macOS):
//   g++ -std=c++17 -Wall -Wextra snake_gui.cpp -o snake_gui $(pkg-config --cflags --libs raylib)
// Build (Windows/MinGW):
//   g++ -std=c++17 -Wall -Wextra snake_gui.cpp -o snake_gui.exe -lraylib -lopengl32 -lgdi32 -lwinmm
//
// 30x20 grid, 24 px cells, 60 px HUD above the 720x480 arena, plus an 80 px
// GUI control bar. F11 toggles fullscreen; the 720x620 canvas is letterboxed.
// Controls: WASD/arrows turn, hold SPACE for boost, R restarts after game over,
// ESC quits. The bottom bar also exposes clickable buttons for every action.

#include "raylib.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <deque>
#include <random>
#include <string>
#include <vector>

namespace {

constexpr int kCols = 30;
constexpr int kRows = 20;
constexpr int kCell = 24;
constexpr int kHud = 60;
constexpr int kArenaW = kCols * kCell;  // 720
constexpr int kArenaH = kRows * kCell;  // 480
constexpr int kWinW = kArenaW;          // 720
constexpr int kBarH = 80;               // GUI control bar under the arena
constexpr int kBarY = kHud + kArenaH;   // 540
constexpr int kWinH = kBarY + kBarH;    // 620

constexpr float kNormalTick = 0.16f;  // slightly slower than the original 0.12 s
constexpr float kBoostTick = 0.08f;   // boost still halves the interval
constexpr float kObstaclePeriod = 8.0f;
constexpr float kWarningTime = 1.0f;
constexpr float kGhostLife = 0.32f;  // seconds a wrap ghost stays visible
constexpr int kMaxObstacles = 15;
constexpr int kMinLength = 3;

// Special-food effects (beneficial or harmful, picked at random)
constexpr float kEffectDuration = 5.0f;  // timed slow/haste length
constexpr float kFxMessageTime = 1.6f;   // HUD feedback duration
constexpr float kSlowFactor = 1.45f;     // >1 => slower ticks
constexpr float kHasteFactor = 0.72f;    // <1 => faster ticks
constexpr int kSpecialChancePct = 30;    // chance per normal food spawn
constexpr int kBonusScore = 50;
constexpr int kGrowSegments = 3;
constexpr int kTrimSegments = 2;
constexpr int kCurseObstacles = 3;

// Palette
const Color kBg{14, 14, 20, 255};
const Color kHudBg{26, 28, 38, 255};
const Color kArenaBg{20, 22, 30, 255};
const Color kGrid{34, 36, 48, 255};
const Color kBorder{88, 94, 116, 255};
const Color kSnakeHead{130, 225, 140, 255};
const Color kSnakeTail{36, 120, 96, 255};
const Color kFood{232, 64, 64, 255};
const Color kObstacle{112, 116, 128, 255};
const Color kWarning{235, 170, 60, 255};
const Color kEye{18, 26, 22, 255};
const Color kText{235, 238, 245, 255};
const Color kBoostOn{120, 230, 140, 255};
const Color kSpecial{190, 120, 255, 255};
const Color kSpecialHalo{255, 214, 90, 255};
const Color kGood{120, 230, 140, 255};
const Color kBad{240, 110, 90, 255};

struct Cell {
    int x = 0;
    int y = 0;

    bool operator==(const Cell& other) const { return x == other.x && y == other.y; }
};

enum class Dir { Up, Down, Left, Right };

bool isOpposite(Dir a, Dir b) {
    return (a == Dir::Up && b == Dir::Down) || (a == Dir::Down && b == Dir::Up) ||
           (a == Dir::Left && b == Dir::Right) || (a == Dir::Right && b == Dir::Left);
}

enum class Effect { Bonus, Slow, Haste, Grow, Trim, Cleanse, Curse };

const char* effectName(Effect e) {
    switch (e) {
        case Effect::Bonus: return "BONUS +50";
        case Effect::Slow: return "SLOW";
        case Effect::Haste: return "HASTE";
        case Effect::Grow: return "GROW +3";
        case Effect::Trim: return "TRIM -2";
        case Effect::Cleanse: return "CLEANSE";
        case Effect::Curse: return "CURSE +3";
    }
    return "?";
}

struct Warning {
    Cell cell;
    float timer = kWarningTime;
};

// Fading echo of the head at both ends of a wrap teleport.
struct GhostHead {
    Cell cell;
    Dir dir = Dir::Right;
    float life = 0.0f;
    float maxLife = 0.0f;
};

Color lerpColor(Color a, Color b, float t) {
    const float k = (t < 0.0f) ? 0.0f : (t > 1.0f ? 1.0f : t);
    const auto mix = [k](unsigned char x, unsigned char y) {
        return static_cast<unsigned char>(static_cast<float>(x) +
                                         (static_cast<float>(y) - static_cast<float>(x)) * k);
    };
    return Color{mix(a.r, b.r), mix(a.g, b.g), mix(a.b, b.b), 255};
}

Rectangle cellRect(const Cell& c, float inset) {
    return Rectangle{static_cast<float>(c.x * kCell) + inset,
                     static_cast<float>(kHud + c.y * kCell) + inset,
                     static_cast<float>(kCell) - inset * 2.0f,
                     static_cast<float>(kCell) - inset * 2.0f};
}

// Control-bar buttons in virtual (canvas) coordinates.
Rectangle moveLeftRect() { return Rectangle{16.0f, static_cast<float>(kBarY + 16), 52.0f, 48.0f}; }
Rectangle moveUpRect() { return Rectangle{76.0f, static_cast<float>(kBarY + 16), 52.0f, 48.0f}; }
Rectangle moveDownRect() { return Rectangle{136.0f, static_cast<float>(kBarY + 16), 52.0f, 48.0f}; }
Rectangle moveRightRect() { return Rectangle{196.0f, static_cast<float>(kBarY + 16), 52.0f, 48.0f}; }
Rectangle boostRect() { return Rectangle{268.0f, static_cast<float>(kBarY + 16), 120.0f, 48.0f}; }
Rectangle restartRect() { return Rectangle{404.0f, static_cast<float>(kBarY + 16), 120.0f, 48.0f}; }
Rectangle quitRect() { return Rectangle{540.0f, static_cast<float>(kBarY + 16), 100.0f, 48.0f}; }

struct Game {
    std::deque<Cell> snake;      // front == head
    std::deque<Cell> obstacles;  // solid obstacles, front == oldest
    std::vector<Warning> warnings;
    std::vector<GhostHead> ghosts;
    bool hasSpecial = false;
    Cell specialFood{0, 0};
    Effect specialEffect = Effect::Bonus;
    float speedFactor = 1.0f;
    float effectTimer = 0.0f;
    Effect activeEffect = Effect::Slow;  // meaningful while effectTimer > 0
    std::string fxText = "none";
    float fxTextTimer = 0.0f;
    Color fxColor = kText;
    Vector2 uiMouse{-1.0f, -1.0f};
    bool uiBoostHeld = false;
    Cell food{0, 0};
    Dir dir = Dir::Right;
    Dir buffered = Dir::Right;
    bool hasBuffered = false;  // at most one turn queued per tick
    bool boostHeld = false;
    bool over = false;
    bool quit = false;
    int score = 0;
    int highScore = 0;  // session only
    float survival = 0.0f;
    int scoredSeconds = 0;
    float obstacleTimer = 0.0f;
    float accumulator = 0.0f;
    int boostTicks = 0;
    std::mt19937 rng{std::random_device{}()};

    Game() { reset(); }

    void reset() {
        snake.clear();
        const int cx = kCols / 2;
        const int cy = kRows / 2;
        snake.push_back(Cell{cx, cy});  // head, moving right
        snake.push_back(Cell{cx - 1, cy});
        snake.push_back(Cell{cx - 2, cy});
        dir = Dir::Right;
        buffered = Dir::Right;
        hasBuffered = false;
        boostHeld = false;
        boostTicks = 0;
        obstacles.clear();
        warnings.clear();
        ghosts.clear();
        hasSpecial = false;
        speedFactor = 1.0f;
        effectTimer = 0.0f;
        fxText = "none";
        fxTextTimer = 0.0f;
        fxColor = kText;
        over = false;
        score = 0;
        survival = 0.0f;
        scoredSeconds = 0;
        obstacleTimer = 0.0f;
        accumulator = 0.0f;
        spawnFood();
    }

    bool boostActive() const {
        return !over && boostHeld && snake.size() > static_cast<size_t>(kMinLength);
    }

    float tickInterval() const {
        const float base = boostActive() ? kBoostTick : kNormalTick;
        return base * speedFactor;
    }

    bool isSnake(const Cell& c) const {
        for (const Cell& s : snake) {
            if (s == c) {
                return true;
            }
        }
        return false;
    }

    bool isSolid(const Cell& c) const {
        for (const Cell& o : obstacles) {
            if (o == c) {
                return true;
            }
        }
        return false;
    }

    bool isWarning(const Cell& c) const {
        for (const Warning& w : warnings) {
            if (w.cell == c) {
                return true;
            }
        }
        return false;
    }

    // Empty means: no snake, food, special food, solid obstacle, or warning.
    bool isEmpty(const Cell& c) const {
        if (hasSpecial && c == specialFood) {
            return false;
        }
        return !(c == food) && !isSnake(c) && !isSolid(c) && !isWarning(c);
    }

    void spawnFood() {
        std::vector<Cell> empty;
        empty.reserve(static_cast<size_t>(kCols * kRows));
        for (int y = 0; y < kRows; ++y) {
            for (int x = 0; x < kCols; ++x) {
                const Cell c{x, y};
                if (isEmpty(c)) {
                    empty.push_back(c);
                }
            }
        }
        if (empty.empty()) {
            return;
        }
        std::uniform_int_distribution<size_t> pick(0, empty.size() - 1);
        food = empty[pick(rng)];
        maybeSpawnSpecial();
    }

    // Chance to place one special food with a random beneficial/harmful effect.
    void maybeSpawnSpecial() {
        if (hasSpecial || snake.empty()) {
            return;
        }
        std::uniform_int_distribution<int> roll(1, 100);
        if (roll(rng) > kSpecialChancePct) {
            return;
        }
        std::vector<Cell> empty;
        for (int y = 0; y < kRows; ++y) {
            for (int x = 0; x < kCols; ++x) {
                const Cell c{x, y};
                if (isEmpty(c)) {
                    empty.push_back(c);
                }
            }
        }
        if (empty.empty()) {
            return;
        }
        std::uniform_int_distribution<size_t> pick(0, empty.size() - 1);
        specialFood = empty[pick(rng)];
        std::uniform_int_distribution<int> effectPick(0, 6);
        specialEffect = static_cast<Effect>(effectPick(rng));
        hasSpecial = true;
    }

    // Place one solid obstacle in a random empty cell away from the head.
    void addSolidObstacle() {
        if (snake.empty()) {
            return;
        }
        const Cell head = snake.front();
        std::vector<Cell> candidates;
        for (int y = 0; y < kRows; ++y) {
            for (int x = 0; x < kCols; ++x) {
                const Cell c{x, y};
                if (!isEmpty(c)) {
                    continue;
                }
                if (std::abs(c.x - head.x) <= 4 && std::abs(c.y - head.y) <= 4) {
                    continue;
                }
                candidates.push_back(c);
            }
        }
        if (candidates.empty()) {
            return;
        }
        std::uniform_int_distribution<size_t> pick(0, candidates.size() - 1);
        if (static_cast<int>(obstacles.size()) >= kMaxObstacles) {
            obstacles.pop_front();
        }
        obstacles.push_back(candidates[pick(rng)]);
    }

    void applyEffect(Effect e) {
        fxTextTimer = kFxMessageTime;
        switch (e) {
            case Effect::Bonus:
                score += kBonusScore;
                fxText = "BONUS +50";
                fxColor = kGood;
                break;
            case Effect::Slow:
                speedFactor = kSlowFactor;
                effectTimer = kEffectDuration;
                activeEffect = e;
                fxText = effectName(e);
                fxColor = kGood;
                break;
            case Effect::Haste:
                speedFactor = kHasteFactor;
                effectTimer = kEffectDuration;
                activeEffect = e;
                fxText = effectName(e);
                fxColor = kBad;
                break;
            case Effect::Grow: {
                const Cell tail = snake.back();
                for (int i = 0; i < kGrowSegments; ++i) {
                    snake.push_back(tail);
                }
                fxText = "GROW +3";
                fxColor = kBad;
                break;
            }
            case Effect::Trim:
                for (int i = 0;
                     i < kTrimSegments && snake.size() > static_cast<size_t>(kMinLength); ++i) {
                    snake.pop_back();
                }
                fxText = "TRIM -2";
                fxColor = kGood;
                break;
            case Effect::Cleanse:
                obstacles.clear();
                warnings.clear();
                fxText = "CLEANSE";
                fxColor = kGood;
                break;
            case Effect::Curse:
                for (int i = 0; i < kCurseObstacles; ++i) {
                    addSolidObstacle();
                }
                fxText = "CURSE +3";
                fxColor = kBad;
                break;
        }
    }

    // Every 8 s: mark 1-3 random empty cells that are not within 4 cells of the
    // head. They flash for 1 s before becoming solid (see updateObstacles).
    void spawnWave() {
        if (snake.empty()) {
            return;
        }
        const Cell head = snake.front();
        std::vector<Cell> candidates;
        for (int y = 0; y < kRows; ++y) {
            for (int x = 0; x < kCols; ++x) {
                const Cell c{x, y};
                if (!isEmpty(c)) {
                    continue;
                }
                if (std::abs(c.x - head.x) <= 4 && std::abs(c.y - head.y) <= 4) {
                    continue;
                }
                candidates.push_back(c);
            }
        }
        std::uniform_int_distribution<int> countDist(1, 3);
        const int count = countDist(rng);
        for (int i = 0; i < count && !candidates.empty(); ++i) {
            std::uniform_int_distribution<size_t> pick(0, candidates.size() - 1);
            const size_t index = pick(rng);
            warnings.push_back(Warning{candidates[index], kWarningTime});
            candidates[index] = candidates.back();
            candidates.pop_back();
        }
    }

    // Real-time obstacle bookkeeping: wave timer plus warning countdowns.
    void updateObstacles(float dt) {
        obstacleTimer += dt;
        if (obstacleTimer >= kObstaclePeriod) {
            obstacleTimer -= kObstaclePeriod;
            spawnWave();
        }
        for (size_t i = 0; i < warnings.size();) {
            warnings[i].timer -= dt;
            if (warnings[i].timer <= 0.0f) {
                const Cell c = warnings[i].cell;
                warnings.erase(warnings.begin() + static_cast<std::ptrdiff_t>(i));
                if (static_cast<int>(obstacles.size()) >= kMaxObstacles) {
                    obstacles.pop_front();  // drop the oldest solid obstacle
                }
                obstacles.push_back(c);
            } else {
                ++i;
            }
        }
    }

    // Solid obstacle or own body. Arena edges wrap, so walls are not deadly.
    // Warning cells are not solid yet.
    bool checkCollision(const Cell& c, bool ignoreTail) const {
        if (isSolid(c)) {
            return true;
        }
        const size_t n = snake.size();
        for (size_t i = 0; i < n; ++i) {
            if (ignoreTail && n > 1 && i + 1 == n) {
                continue;  // tail cell vacates this tick
            }
            if (snake[i] == c) {
                return true;
            }
        }
        return false;
    }

    void handleInput() {
        if (IsKeyPressed(KEY_ESCAPE)) {
            quit = true;
            return;
        }
        if (over) {
            boostHeld = false;
            if (IsKeyPressed(KEY_R)) {
                reset();
            }
            return;
        }

        boostHeld = IsKeyDown(KEY_SPACE) || uiBoostHeld;

        if (hasBuffered) {
            return;  // one buffered turn per tick at most
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

    void queueTurn(Dir wanted) {
        if (over || hasBuffered) {
            return;
        }
        if (isOpposite(wanted, dir)) {
            return;
        }
        buffered = wanted;
        hasBuffered = true;
    }

    // On-screen buttons: arrows, hold-to-boost, restart and quit.
    void handleUi(const Vector2& mouse, bool down, bool pressed) {
        uiMouse = mouse;
        uiBoostHeld = false;
        if (over) {
            if (pressed && CheckCollisionPointRec(mouse, restartRect())) {
                reset();
            }
        } else {
            uiBoostHeld = down && CheckCollisionPointRec(mouse, boostRect());
            if (pressed) {
                if (CheckCollisionPointRec(mouse, moveLeftRect())) {
                    queueTurn(Dir::Left);
                } else if (CheckCollisionPointRec(mouse, moveUpRect())) {
                    queueTurn(Dir::Up);
                } else if (CheckCollisionPointRec(mouse, moveDownRect())) {
                    queueTurn(Dir::Down);
                } else if (CheckCollisionPointRec(mouse, moveRightRect())) {
                    queueTurn(Dir::Right);
                }
            }
        }
        if (pressed && CheckCollisionPointRec(mouse, quitRect())) {
            quit = true;
        }
    }

    // One fixed logic tick.
    void update() {
        if (over) {
            return;
        }

        if (hasBuffered) {
            if (!isOpposite(buffered, dir)) {
                dir = buffered;
            }
            hasBuffered = false;
        }

        // Boost wear: one tail segment every third boosted tick, never below 3.
        if (boostActive()) {
            ++boostTicks;
            if (boostTicks % 3 == 0 && snake.size() > static_cast<size_t>(kMinLength)) {
                snake.pop_back();
            }
        } else {
            boostTicks = 0;
        }

        Cell head = snake.front();
        switch (dir) {
            case Dir::Up: --head.y; break;
            case Dir::Down: ++head.y; break;
            case Dir::Left: --head.x; break;
            case Dir::Right: ++head.x; break;
        }

        // Wrap around the arena edges instead of hitting a wall.
        const bool wrapped =
            (head.x < 0 || head.x >= kCols || head.y < 0 || head.y >= kRows);
        const Cell exitCell = snake.front();
        head.x = (head.x + kCols) % kCols;
        head.y = (head.y + kRows) % kRows;
        if (wrapped) {
            ghosts.push_back(GhostHead{exitCell, dir, kGhostLife, kGhostLife});
            ghosts.push_back(GhostHead{head, dir, kGhostLife, kGhostLife});
        }

        const bool eats = (head == food);
        const bool eatsSpecial = hasSpecial && (head == specialFood);
        if (checkCollision(head, !eats && !eatsSpecial)) {
            gameOver();
            return;
        }

        snake.push_front(head);
        if (eats) {
            score += 10;
            spawnFood();
        } else if (eatsSpecial) {
            score += 10;
            hasSpecial = false;
            applyEffect(specialEffect);
        } else {
            snake.pop_back();
        }
    }

    // Frame driver: survival score, obstacle timers, and tick accumulation.
    void stepFrame(float dt) {
        // Ghost heads keep fading even while the game-over overlay is shown.
        for (size_t i = 0; i < ghosts.size();) {
            ghosts[i].life -= dt;
            if (ghosts[i].life <= 0.0f) {
                ghosts.erase(ghosts.begin() + static_cast<std::ptrdiff_t>(i));
            } else {
                ++i;
            }
        }

        if (over || quit) {
            return;
        }
        survival += dt;
        const int whole = static_cast<int>(survival);
        if (whole > scoredSeconds) {
            score += whole - scoredSeconds;
            scoredSeconds = whole;
        }

        if (effectTimer > 0.0f) {
            effectTimer -= dt;
            if (effectTimer <= 0.0f) {
                effectTimer = 0.0f;
                speedFactor = 1.0f;
            }
        }
        if (fxTextTimer > 0.0f) {
            fxTextTimer -= dt;
        }

        updateObstacles(dt);

        accumulator += dt;
        int guard = 0;
        while (!over && accumulator >= tickInterval() && guard++ < 8) {
            accumulator -= tickInterval();
            update();
        }
        if (accumulator > 1.0f) {
            accumulator = 0.0f;  // recover from long stalls without bursting
        }
    }

    void gameOver() {
        over = true;
        boostHeld = false;
        boostTicks = 0;
        if (score > highScore) {
            highScore = score;
        }
    }

    void drawEyes() const {
        if (snake.empty()) {
            return;
        }
        const Cell& h = snake.front();
        const float cx = static_cast<float>(h.x * kCell + kCell / 2);
        const float cy = static_cast<float>(kHud + h.y * kCell + kCell / 2);
        float dx = 0.0f;
        float dy = 0.0f;
        switch (dir) {
            case Dir::Up: dy = -1.0f; break;
            case Dir::Down: dy = 1.0f; break;
            case Dir::Left: dx = -1.0f; break;
            case Dir::Right: dx = 1.0f; break;
        }
        const float px = -dy;
        const float py = dx;
        DrawCircleV(Vector2{cx + dx * 5.0f + px * 5.0f, cy + dy * 5.0f + py * 5.0f}, 2.6f, kEye);
        DrawCircleV(Vector2{cx + dx * 5.0f - px * 5.0f, cy + dy * 5.0f - py * 5.0f}, 2.6f, kEye);
    }

    void drawGhostHead(const GhostHead& g) const {
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
            case Dir::Up: dy = -1.0f; break;
            case Dir::Down: dy = 1.0f; break;
            case Dir::Left: dx = -1.0f; break;
            case Dir::Right: dx = 1.0f; break;
        }
        const float px = -dy;
        const float py = dx;
        Color eye = kEye;
        eye.a = alpha;
        DrawCircleV(Vector2{cx + dx * 5.0f + px * 5.0f, cy + dy * 5.0f + py * 5.0f}, 2.6f, eye);
        DrawCircleV(Vector2{cx + dx * 5.0f - px * 5.0f, cy + dy * 5.0f - py * 5.0f}, 2.6f, eye);
    }

    void drawButton(const Rectangle& r, const char* label, bool enabled, bool active,
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
        DrawRectangleLinesEx(r, 2.0f,
                             (hovered && enabled) ? kBorder : Color{58, 62, 76, 255});

        const int fontSize = 20;
        const int textW = MeasureText(label, fontSize);
        const Color textColor = enabled ? kText : Color{110, 114, 126, 255};
        DrawText(label,
                 static_cast<int>(r.x + (r.width - static_cast<float>(textW)) / 2.0f),
                 static_cast<int>(r.y + (r.height - static_cast<float>(fontSize)) / 2.0f),
                 fontSize, textColor);
    }

    void draw() const {
        ClearBackground(kBg);

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

        for (const Cell& o : obstacles) {
            DrawRectangleRounded(cellRect(o, 2.0f), 0.18f, 6, kObstacle);
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
            DrawRectangleRounded(cellRect(snake[i], 1.5f), 0.35f, 8, lerpColor(kSnakeHead, kSnakeTail, t));
        }
        drawEyes();

        // Fading ghost heads marking both sides of a wrap.
        for (const GhostHead& g : ghosts) {
            drawGhostHead(g);
        }

        // Visible arena frame.
        DrawRectangleLinesEx(Rectangle{0.0f, static_cast<float>(kHud),
                                       static_cast<float>(kArenaW), static_cast<float>(kArenaH)},
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
            DrawText(TextFormat("FX: %s %.1fs", effectName(activeEffect),
                                static_cast<double>(effectTimer)),
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
        drawButton(moveLeftRect(), "<", canPlay, false,
                   CheckCollisionPointRec(uiMouse, moveLeftRect()));
        drawButton(moveUpRect(), "^", canPlay, false,
                   CheckCollisionPointRec(uiMouse, moveUpRect()));
        drawButton(moveDownRect(), "v", canPlay, false,
                   CheckCollisionPointRec(uiMouse, moveDownRect()));
        drawButton(moveRightRect(), ">", canPlay, false,
                   CheckCollisionPointRec(uiMouse, moveRightRect()));
        drawButton(boostRect(), "BOOST", canPlay, boostActive(),
                   CheckCollisionPointRec(uiMouse, boostRect()));
        drawButton(restartRect(), "RESTART", over, false,
                   CheckCollisionPointRec(uiMouse, restartRect()));
        drawButton(quitRect(), "QUIT", true, false,
                   CheckCollisionPointRec(uiMouse, quitRect()));

        DrawText("F11 fullscreen  |  WASD / arrows turn  |  SPACE boost  |  R restart  |  ESC quit",
                 16, kBarY + 66, 12, GRAY);

        if (over) {
            DrawRectangle(0, kHud, kArenaW, kArenaH, Color{0, 0, 0, 180});

            const char* title = "GAME OVER";
            DrawText(title, (kArenaW - MeasureText(title, 44)) / 2, kHud + 150, 44, kText);

            const std::string finalText = "Final score: " + std::to_string(score);
            DrawText(finalText.c_str(), (kArenaW - MeasureText(finalText.c_str(), 24)) / 2,
                     kHud + 220, 24, kText);

            const std::string highText = "High score: " + std::to_string(highScore);
            DrawText(highText.c_str(), (kArenaW - MeasureText(highText.c_str(), 24)) / 2,
                     kHud + 256, 24, kText);

            const char* prompt = "Press R to restart, ESC to quit";
            DrawText(prompt, (kArenaW - MeasureText(prompt, 20)) / 2, kHud + 320, 20, LIGHTGRAY);
        }
    }
};

}  // namespace

int main() {
    InitWindow(kWinW, kWinH, "Snake");
    SetTargetFPS(60);

    // Fixed-size virtual canvas, letterboxed to the real window (fullscreen too).
    RenderTexture2D canvas = LoadRenderTexture(kWinW, kWinH);
    SetTextureFilter(canvas.texture, TEXTURE_FILTER_BILINEAR);

    Game game;
    while (!game.quit && !WindowShouldClose()) {
        const float dt = GetFrameTime();

        if (IsKeyPressed(KEY_F11)) {
            ToggleFullscreen();
        }

        const float screenW = static_cast<float>(GetScreenWidth());
        const float screenH = static_cast<float>(GetScreenHeight());
        float scale = std::min(screenW / static_cast<float>(kWinW),
                               screenH / static_cast<float>(kWinH));
        if (scale <= 0.0f) {
            scale = 1.0f;
        }
        const Vector2 offset{(screenW - static_cast<float>(kWinW) * scale) * 0.5f,
                             (screenH - static_cast<float>(kWinH) * scale) * 0.5f};
        const Vector2 mouse = GetMousePosition();
        const Vector2 virtualMouse{(mouse.x - offset.x) / scale,
                                   (mouse.y - offset.y) / scale};

        game.handleUi(virtualMouse, IsMouseButtonDown(MOUSE_BUTTON_LEFT),
                      IsMouseButtonPressed(MOUSE_BUTTON_LEFT));
        game.handleInput();
        game.stepFrame(dt);

        BeginTextureMode(canvas);
        game.draw();
        EndTextureMode();

        BeginDrawing();
        ClearBackground(BLACK);
        DrawTexturePro(canvas.texture,
                       Rectangle{0.0f, 0.0f, static_cast<float>(kWinW),
                                 -static_cast<float>(kWinH)},
                       Rectangle{offset.x, offset.y, static_cast<float>(kWinW) * scale,
                                 static_cast<float>(kWinH) * scale},
                       Vector2{0.0f, 0.0f}, 0.0f, WHITE);
        EndDrawing();
    }

    UnloadRenderTexture(canvas);
    CloseWindow();
    return 0;
}
