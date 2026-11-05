#pragma once

#include "game/config.hpp"

#include <deque>
#include <random>
#include <string>
#include <vector>

namespace snake {

struct Cell {
    int x = 0;
    int y = 0;

    bool operator==(const Cell& other) const { return x == other.x && y == other.y; }
};

enum class Dir { Up, Down, Left, Right };

enum class Effect { Bonus, Slow, Haste, Grow, Trim, Cleanse, Curse };

enum class Mode { Menu, Playing };

struct Warning {
    Cell cell;
    float timer = kWarningTime;
};

// Solid obstacle with a finite lifetime so the arena keeps changing.
struct Obstacle {
    Cell cell;
    float life = 0.0f;
};

// Fading echo of the head at both ends of a wrap teleport.
struct GhostHead {
    Cell cell;
    Dir dir = Dir::Right;
    float life = 0.0f;
    float maxLife = 0.0f;
};

bool isOpposite(Dir a, Dir b);
const char* effectName(Effect e);

struct Game {

    std::deque<Cell> snake;         // front == head
    std::deque<Obstacle> obstacles; // solid obstacles, front == oldest
    std::vector<Warning> warnings;
    std::vector<GhostHead> ghosts;
    bool hasSpecial = false;
    Cell specialFood{0, 0};
    Effect specialEffect = Effect::Bonus;
    float speedFactor = 1.0f;
    float effectTimer = 0.0f;
    Effect activeEffect = Effect::Slow; // meaningful while effectTimer > 0
    std::string fxText = "none";
    float fxTextTimer = 0.0f;
    Color fxColor = kText;
    Vector2 uiMouse{-1.0f, -1.0f};
    bool uiBoostHeld = false;
    Cell food{0, 0};
    Dir dir = Dir::Right;
    Dir buffered = Dir::Right;
    bool hasBuffered = false; // at most one turn queued per tick
    bool boostHeld = false;
    bool over = false;
    bool quit = false;
    Mode mode = Mode::Menu;
    bool fsRequest = false;
    int score = 0;
    int highScore = 0; // session only
    float survival = 0.0f;
    int scoredSeconds = 0;
    float obstacleTimer = 0.0f;
    float accumulator = 0.0f;
    int boostTicks = 0;
    std::mt19937 rng{std::random_device{}()};

    Game();
    void reset();
    void startGame();
    bool takeFullscreenRequest();
    bool boostActive() const;
    float tickInterval() const;
    bool isSnake(const Cell& c) const;
    bool isSolid(const Cell& c) const;
    bool isWarning(const Cell& c) const;
    bool isEmpty(const Cell& c) const;
    void spawnFood();
    void maybeSpawnSpecial();
    void addSolidObstacle();
    void applyEffect(Effect e);
    void spawnWave();
    void updateObstacles(float dt);
    bool checkCollision(const Cell& c, bool ignoreTail) const;
    void handleInput();
    void queueTurn(Dir wanted);
    void handleUi(const Vector2& mouse, bool down, bool pressed);
    void update();
    void stepFrame(float dt);
    void gameOver();
    void drawEyes() const;
    void drawGhostHead(const GhostHead& g) const;
    void drawButton(const Rectangle& r, const char* label, bool enabled, bool active,
                    bool hovered) const;
    void drawMenu() const;
    void draw() const;
};

} // namespace snake
