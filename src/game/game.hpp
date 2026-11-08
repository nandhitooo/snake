#pragma once

#include "game/config.hpp"
#include "game/highscore.hpp"
#include "game/settings.hpp"
#include "game/skins.hpp"

#include <cstddef>
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

enum class Mode { Menu, Playing, Settings, Skins, HighScores };

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

// Flat ring that expands and fades on the floor after a bite (Phase I4a).
struct Shockwave {
    Vector3 pos{0.0f, 0.0f, 0.0f};
    Color color{255, 255, 255, 255};
    float life = 0.0f;
    float maxLife = kShockwaveLife;
    float radius = kShockwaveRadius;
};

// Fading echo of the head at both ends of a wrap teleport.
struct GhostHead {
    Cell cell;
    Dir dir = Dir::Right;
    float life = 0.0f;
    float maxLife = 0.0f;
};

// A rival snake controlled by the AI module (src/game/ai_snake.cpp).
struct AiSnake {
    std::deque<Cell> body;     // front == head
    std::deque<Cell> prevBody; // for render interpolation
    Dir dir = Dir::Left;
    float spawnT = 0.0f;     // 0..1 pop-in
    float deathT = 0.0f;     // 0..1 fade-out while dying
    float blinkPhase = 0.0f; // random offset so rivals do not blink in sync
    bool dying = false;
};

// Telegraphs an AI spawn so the player gets a warning marker first.
struct AiSpawnWarning {
    Cell cell;
    float timer = 1.2f;
    size_t length = 3;
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

// Food that has been eaten by the logic tick but not yet reached by the visual
// head. It keeps shrinking toward the mouth until the bite lands (progress 1).
struct PendingBite {
    Vector3 pos{0.0f, 0.0f, 0.0f};
    Color color{255, 255, 255, 255};
    bool special = false;
    bool good = true;      // special food only: effectIsGood() of the effect it carried
    float progress = 0.0f; // 0..1, driven by tickAlpha
    std::string label;     // floating text fired when the bite lands
    Color labelColor{235, 238, 245, 255};
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

// True for real fullscreen and for borderless-windowed fullscreen (raylib >= 5).
inline bool isFullscreenNow() { return IsWindowFullscreen() || IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE); }

// Phase F1: all drawing code uses the virtual 960x540 space; the canvas texture is S
// times larger and this puts the S scale back on the modelview matrix. Call it right
// after BeginTextureMode and again after EVERY EndMode3D (which resets modelview).
void applyUiScale(float scale);

bool isOpposite(Dir a, Dir b);
const char* effectName(Effect e);
// Single source of truth for the special-food grouping (text colour, SFX, particles).
// Good: Bonus, Slow, Trim, Cleanse. Bad: Haste, Grow, Curse.
bool effectIsGood(Effect e);

struct Game {

    std::deque<Cell> snake;         // front == head
    std::deque<Cell> prevSnake;     // positions before the last logic tick (render interpolation)
    std::deque<Obstacle> obstacles; // solid obstacles, front == oldest
    std::vector<Warning> warnings;
    std::vector<GhostHead> ghosts;
    std::vector<WorldParticle> particles;
    std::vector<Particle> ambientParticles;
    std::vector<Shockwave> shockwaves; // Phase I4a: bite rings on the floor
    std::vector<Vector3> dust;         // Phase I4e: ambient motes inside the arena
    std::vector<AiSnake> ai;
    std::vector<AiSpawnWarning> aiWarnings;
    float aiSpawnTimer = 12.0f;
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
    Dir turnQueue[kTurnQueueSlots]{}; // turns waiting for upcoming ticks (oldest first)
    int turnQueueCount = 0;
    bool boostHeld = false;
    bool boostRequiresRelease = false; // SPACE/BOOST must be released after a round (re)starts
    bool paused = false;
    bool over = false;
    bool won = false; // the round ended because the board is full
    bool animations = true; // decorative animation toggle (Phase 4: Settings)
    int hoverId = -1;        // audio: last hovered UI control, so hover SFX fires once
    bool wasBoosting = false; // audio: boost start/end edge detection
    float swallowClock = -1.0f; // seconds since the head swallowed food, <0 == idle
    float foodAge = 0.0f;       // seconds since the current food spawned
    float specialAge = 0.0f;    // seconds since the current special food spawned
    float overFade = 0.0f;      // 0..1 fade-in of the game-over overlay
    float transitionT = 0.0f;   // 0..1 full-screen fade after a mode change
    float trailTimer = 0.0f;    // throttle for skin trail particles
    float boostVisual = 0.0f;   // 0..1 smoothed boost (FOV kick, post shader, speed lines)
    float flashPulse = 0.0f;    // 0..1 decaying screen flash (special bite, game over)
    bool quit = false;
    Mode mode = Mode::Menu;
    bool fsRequest = false;
    float uiScale = 1.0f;             // canvas supersampling S, set by main each frame
    bool pendingWindowResize = false; // apply the Resolution setting on the next frame
    int appliedResolution = -1;       // last Resolution index pushed to the window

    // Persisted options + settings-screen state
    Settings settings;
    int settingsTab = TabGraphics; // Phase H: GRAPHICS | GAMEPLAY | AUDIO
    int settingsIndex = 0;
    int skinPage = 0; // Phase I3: 2 x 4 cards per page, two pages
    Mode settingsReturn = Mode::Menu;
    bool windowSettingsDirty = false;
    float normalTick = kNormalTick;
    bool shakeEnabled = true;
    int score = 0;
    int highScore = 0; // best of the persistent board (Phase G)

    // Phase G: persistent high scores, name prompt and the CLEAR confirmation.
    HighScoreBoard board;
    bool enteringName = false;  // the game-over name prompt is open
    std::string nameBuffer;     // characters typed so far (max kMaxPlayerName)
    int nameRank = 0;           // 1-based rank of this round, 0 = did not qualify
    bool scoreSaved = false;    // one round inserts at most one entry
    int highlightRank = -1;     // row highlighted on the High Scores screen
    bool newBestThisRound = false;
    bool clearArmed = false;    // two-step CLEAR confirmation
    float clearArmTimer = 0.0f; // seconds left to confirm
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
    float camYaw = 0.0f;          // smoothed heading (radians) used by the third-person orbit
    int camDirX = 1;              // last stable movement direction (grid cells)
    int camDirZ = 0;
    float camHeadingStable = 0.0f; // seconds the movement heading has been unchanged
    float camBlend = 0.0f;        // remaining cross-fade time after a camera-mode change
    Camera3D camFrom{};           // pose captured when the mode changed
    CameraMode camAppliedMode = CameraMode::Iso;
    float lastTickInterval = 0.0f; // previous frame's tick interval (alpha continuity)

    // Eating animation state (Phase B)
    std::deque<PendingBite> pendingBites;
    float mouthOpen = 0.0f;   // 0..1 smoothed jaw opening
    float mouthTarget = 0.0f; // 0..1 wanted jaw opening for this frame
    float biteT = -1.0f;      // seconds since the last bite, <0 == idle
    float mouthLock = 0.0f;   // short cooldown after a bite, in seconds
    float blinkT = -1.0f;     // seconds into the current blink, <0 == eyes open
    float nextBlinkIn = 2.5f; // countdown to the next blink
    int blinkExtra = 0;       // extra blinks queued (double blink)

    Game();
    void reset();
    void startGame();
    bool takeFullscreenRequest();
    void toggleFullscreen();          // F11, menu button and Setting all go through here
    void applyPendingWindowResize();  // call once per frame, before toggles are handled
    void clearTurnQueue();
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
    void spawnEatBurst(const Vector3& pos, Color color); // Phase I1: per-skin bite style
    void initDust();
    void addFloatingText(const std::string& text, Vector3 pos, Color color, float duration = 1.0f);
    void triggerShake(float duration, float intensity);
    void initAmbientParticles();
    void updateVfx(float dt);
    void updateCamera(float dt);
    void updateMouth(float dt);
    void updateBlink(float dt);
    float blinkProgress() const; // 0 = open, 1 = fully closed
    bool eyesHappy() const;      // "^ ^" after a bite
    void resolvePendingBites();
    void triggerBite();
    float mouthTargetDistance() const;

    // First edible cell within kMouthScanCells ahead of the head, or false if the
    // line is blocked (obstacle, own body, or a rival that would kill us).
    bool findEdibleAhead(int& cellsOut, Cell& cellOut) const;
    // Pupil look direction: x == right, y == forward. Unit length or zero.
    Vector2 headGaze() const;

    // Interpolated world position of segment `index` (same maths the renderer
    // uses) so the camera follows the drawn head instead of the logical cell.
    Vector3 visualSegmentWorld(size_t index, float y) const;
    Vector3 visualHeadWorld(float y = 0.0f) const { return visualSegmentWorld(0, y); }
    void applySettings();
    void applyWindowSettings();
    void persistSettings();

    // Skin palette (selected from the Skins screen, persisted in settings.cfg).
    const SkinDef& skin() const;
    Color headColor() const;
    Color snakeColorAt(size_t index, size_t count) const;
    Color eyeColor() const;

    void drawGhostHead(const GhostHead& g) const;
    void drawButton(const Rectangle& r, const char* label, bool enabled, bool active,
                    bool hovered) const;
    void drawMenu() const;
    void drawSettings() const;
    void drawSkins() const;
    void drawHighScores() const;
    void draw() const;

    // Phase G: game-over name entry and the single insert per round.
    void handleNameEntry();
    void submitRoundScore(const std::string& name);
};

} // namespace snake
