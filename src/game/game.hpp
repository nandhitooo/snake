#pragma once

#include "game/config.hpp"
#include "game/settings.hpp"
#include "game/skins.hpp"

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

enum class Mode { Menu, Playing, Settings, Skins };

// Camera rigs available while playing (C cycles through them).
enum class CameraMode { Iso, Top, Third };

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

// Sparks/bursts live in arena world space (1 unit == 1 cell).
struct WorldParticle {
    Vector3 pos{0.0f, 0.0f, 0.0f};
    Vector3 vel{0.0f, 0.0f, 0.0f};
    Color color{255, 255, 255, 255};
    float size = 0.12f;
    float life = 0.0f;
    float maxLife = 1.0f;
    bool gravity = false;
};

// Screen-space ambient motes used by the menu and the outer chrome.
struct Particle {
    Vector2 pos{0.0f, 0.0f};
    Vector2 vel{0.0f, 0.0f};
    Color color{255, 255, 255, 255};
    float size = 3.0f;
    float life = 0.0f;
    float maxLife = 1.0f;
};

struct FloatingText {
    std::string text;
    Vector3 pos{0.0f, 0.0f, 0.0f};
    Color color{255, 255, 255, 255};
    float life = 0.0f;
    float maxLife = 1.0f;
};

// Cell (column, row) -> arena world space. The arena is centred on the origin so
// eyes/lights can stay at a fixed place while the camera follows the snake.
inline Vector3 cellToWorld(const Cell& c, float y = 0.0f) {
    return Vector3{(static_cast<float>(c.x) + 0.5f) - static_cast<float>(kCols) * 0.5f, y,
                   (static_cast<float>(c.y) + 0.5f) - static_cast<float>(kRows) * 0.5f};
}

inline Vector3 dirToWorld(Dir d) {
    switch (d) {
    case Dir::Up:
        return Vector3{0.0f, 0.0f, -1.0f};
    case Dir::Down:
        return Vector3{0.0f, 0.0f, 1.0f};
    case Dir::Left:
        return Vector3{-1.0f, 0.0f, 0.0f};
    case Dir::Right:
    default:
        return Vector3{1.0f, 0.0f, 0.0f};
    }
}

bool isOpposite(Dir a, Dir b);
const char* effectName(Effect e);

struct Game {

    std::deque<Cell> snake;         // front == head
    std::deque<Cell> prevSnake;     // positions before the last logic tick (render interpolation)
    std::deque<Obstacle> obstacles; // solid obstacles, front == oldest
    std::vector<Warning> warnings;
    std::vector<GhostHead> ghosts;
    std::vector<WorldParticle> particles;
    std::vector<Particle> ambientParticles;
    std::vector<FloatingText> floatingTexts;
    float shakeTimer = 0.0f;
    float shakeIntensity = 0.0f;
    float animTime = 0.0f;
    float tickAlpha = 0.0f; // 0..1 progress inside the current logic tick
    bool isNewHighRecord = false;
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
    bool boostRequiresRelease = false; // SPACE/BOOST must be released after a round (re)starts
    bool paused = false;
    bool over = false;
    bool animations = true; // decorative animation toggle (Phase 4: Settings)
    float swallowClock = -1.0f; // seconds since the head swallowed food, <0 == idle
    float foodAge = 0.0f;       // seconds since the current food spawned
    float specialAge = 0.0f;    // seconds since the current special food spawned
    float overFade = 0.0f;      // 0..1 fade-in of the game-over overlay
    float transitionT = 0.0f;   // 0..1 full-screen fade after a mode change
    float trailTimer = 0.0f;    // throttle for skin trail particles
    bool quit = false;
    Mode mode = Mode::Menu;
    bool fsRequest = false;

    // Persisted options + settings-screen state
    Settings settings;
    int settingsIndex = 0;
    Mode settingsReturn = Mode::Menu;
    bool windowSettingsDirty = false;
    float normalTick = kNormalTick;
    bool shakeEnabled = true;
    int score = 0;
    int highScore = 0; // session only
    float survival = 0.0f;
    int scoredSeconds = 0;
    float obstacleTimer = 0.0f;
    float accumulator = 0.0f;
    int boostTicks = 0;
    std::mt19937 rng{std::random_device{}()};

    // 3D presentation state
    Camera3D camera{};
    CameraMode cameraMode = CameraMode::Iso;
    Vector3 camTarget{0.0f, 0.0f, 0.0f};

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
    void togglePause();
    void spawnParticles(Vector3 pos, Color color, int count, float speed = 4.0f,
                        float size = 0.14f, float life = 0.5f, bool gravity = true);
    void addFloatingText(const std::string& text, Vector3 pos, Color color, float duration = 1.0f);
    void triggerShake(float duration, float intensity);
    void initAmbientParticles();
    void updateVfx(float dt);
    void updateCamera(float dt);
    void applySettings();
    void applyWindowSettings();
    void persistSettings();

    // Skin palette (selected from the Skins screen, persisted in settings.cfg).
    const SkinDef& skin() const;
    Color headColor() const;
    Color snakeColorAt(size_t index, size_t count) const;
    Color eyeColor() const;

    void drawEyes() const;
    void drawGhostHead(const GhostHead& g) const;
    void drawButton(const Rectangle& r, const char* label, bool enabled, bool active,
                    bool hovered) const;
    void drawMenu() const;
    void drawSettings() const;
    void drawSkins() const;
    void draw() const;
};

} // namespace snake
