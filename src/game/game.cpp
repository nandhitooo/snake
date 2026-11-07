#include "game/game.hpp"
#include "game/ai_snake.hpp"
#include "game/audio.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace snake {
namespace {

// Advance one cell in `d` without wrapping (the caller decides where the arena ends).
Cell stepRaw(const Cell& c, Dir d) {
    Cell n = c;
    switch (d) {
    case Dir::Up:
        --n.y;
        break;
    case Dir::Down:
        ++n.y;
        break;
    case Dir::Left:
        --n.x;
        break;
    case Dir::Right:
        ++n.x;
        break;
    }
    return n;
}

// Arena edges wrap instead of killing. One place for the rule, shared by the
// logic tick and the mouth look-ahead scan.
Cell wrapCell(Cell c) {
    c.x = (c.x + kCols) % kCols;
    c.y = (c.y + kRows) % kRows;
    return c;
}

Cell nextCellWrapped(const Cell& c, Dir d) { return wrapCell(stepRaw(c, d)); }

// Normalise an angle to (-pi, pi] so smoothing always takes the short way round.
float wrapAngle(float a) {
    constexpr float kPi = 3.1415927f;
    constexpr float kTau = 6.2831855f;
    a = std::fmod(a + kPi, kTau);
    if (a < 0.0f) {
        a += kTau;
    }
    return a - kPi;
}

Vector3 lerp3(Vector3 a, Vector3 b, float t) {
    return Vector3{a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t};
}

} // namespace

bool isOpposite(Dir a, Dir b) {
    return (a == Dir::Up && b == Dir::Down) || (a == Dir::Down && b == Dir::Up) ||
           (a == Dir::Left && b == Dir::Right) || (a == Dir::Right && b == Dir::Left);
}

const char* effectName(Effect e) {
    switch (e) {
    case Effect::Bonus:
        return "BONUS +50";
    case Effect::Slow:
        return "SLOW";
    case Effect::Haste:
        return "HASTE";
    case Effect::Grow:
        return "GROW +3";
    case Effect::Trim:
        return "TRIM -2";
    case Effect::Cleanse:
        return "CLEANSE";
    case Effect::Curse:
        return "CURSE +3";
    }
    return "?";
}

Game::Game() {
    settings = loadSettings("settings.cfg");
    initAmbientParticles();
    reset();
    applySettings();
    camera.up = Vector3{0.0f, 1.0f, 0.0f};
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    camera.position = Vector3{0.0f, 22.0f, 17.0f};
    camera.target = Vector3{0.0f, 0.0f, 0.0f};
    camTarget = Vector3{0.0f, 0.0f, 0.0f};
}

void Game::reset() {
    snake.clear();
    const int cx = kCols / 2;
    const int cy = kRows / 2;
    snake.push_back(Cell{cx, cy}); // head, moving right
    snake.push_back(Cell{cx - 1, cy});
    snake.push_back(Cell{cx - 2, cy});
    prevSnake = snake;
    tickAlpha = 0.0f;
    dir = Dir::Right;
    buffered = Dir::Right;
    hasBuffered = false;
    boostHeld = false;
    uiBoostHeld = false;
    boostRequiresRelease = true; // a held key from the menu must not boost the new round
    paused = false;
    boostTicks = 0;
    obstacles.clear();
    warnings.clear();
    ghosts.clear();
    particles.clear();
    floatingTexts.clear();
    resetAi(*this);
    hoverId = -1;
    wasBoosting = false;
    shakeTimer = 0.0f;
    shakeIntensity = 0.0f;
    isNewHighRecord = false;
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
    lastTickInterval = 0.0f;
    swallowClock = -1.0f;
    foodAge = 0.0f;
    specialAge = 0.0f;
    overFade = 0.0f;
    pendingBites.clear();
    mouthOpen = 0.0f;
    mouthTarget = 0.0f;
    biteT = kMouthBiteIdle;
    mouthLock = 0.0f;
    camYaw = 0.0f;
    camBlend = 0.0f;
    camAppliedMode = cameraMode;
    spawnFood();
}

void Game::startGame() {
    reset();
    cameraMode = static_cast<CameraMode>(std::clamp(settings.camera, 0, 2));
    mode = Mode::Playing;
    transitionT = animations ? 1.0f : 0.0f;
}

bool Game::takeFullscreenRequest() {
    const bool request = fsRequest;
    fsRequest = false;
    return request;
}

bool Game::boostActive() const {
    return !over && !paused && boostHeld && snake.size() > static_cast<size_t>(kMinLength);
}

float Game::tickInterval() const {
    // normalTick is runtime so the Speed setting can change it without a restart.
    const float base = boostActive() ? normalTick * 0.5f : normalTick;
    return base * speedFactor;
}

bool Game::isSnake(const Cell& c) const {
    for (const Cell& s : snake) {
        if (s == c) {
            return true;
        }
    }
    return false;
}

bool Game::isSolid(const Cell& c) const {
    for (const Obstacle& o : obstacles) {
        if (o.cell == c) {
            return true;
        }
    }
    return false;
}

bool Game::isWarning(const Cell& c) const {
    for (const Warning& w : warnings) {
        if (w.cell == c) {
            return true;
        }
    }
    return false;
}

// Empty means: no snake, food, special food, solid obstacle, or warning.
bool Game::isEmpty(const Cell& c) const {
    if (hasSpecial && c == specialFood) {
        return false;
    }
    if (!(c == food) && !isSnake(c) && !isSolid(c) && !isWarning(c)) {
        for (const AiSnake& rival : ai) {
            if (rival.dying) {
                continue;
            }
            for (const Cell& segment : rival.body) {
                if (segment == c) {
                    return false;
                }
            }
        }
        return true;
    }
    return false;
}

void Game::spawnFood() {
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
    foodAge = 0.0f;
    maybeSpawnSpecial();
}

// Chance to place one special food with a random beneficial/harmful effect.
void Game::maybeSpawnSpecial() {
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
    specialAge = 0.0f;
}

// Place one solid obstacle in a random empty cell away from the head.
void Game::addSolidObstacle() {
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
        obstacles.pop_front(); // drop the oldest at the cap
    }
    obstacles.push_back(Obstacle{candidates[pick(rng)], kObstacleLife});
}

void Game::applyEffect(Effect e) {
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
        for (int i = 0; i < kTrimSegments && snake.size() > static_cast<size_t>(kMinLength); ++i) {
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

    // Spec grouping: slow/trim/curse are the bad outcomes, the rest are good.
    const bool good = (e == Effect::Bonus || e == Effect::Haste || e == Effect::Grow ||
                       e == Effect::Cleanse);
    audio.play(good ? SfxId::SpecialGood : SfxId::SpecialBad);
}

// Every 8 s: mark 1-3 random empty cells that are not within 4 cells of the
// head. They flash for 1 s before becoming solid (see updateObstacles).
void Game::spawnWave() {
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
    const size_t before = warnings.size();
    for (int i = 0; i < count && !candidates.empty(); ++i) {
        std::uniform_int_distribution<size_t> pick(0, candidates.size() - 1);
        const size_t index = pick(rng);
        warnings.push_back(Warning{candidates[index], kWarningTime});
        candidates[index] = candidates.back();
        candidates.pop_back();
    }
    if (warnings.size() > before) {
        audio.play(SfxId::ObstacleWarning);
    }
}

// Real-time obstacle bookkeeping: wave timer plus warning countdowns.
void Game::updateObstacles(float dt) {
    obstacleTimer += dt;
    if (obstacleTimer >= kObstaclePeriod) {
        obstacleTimer -= kObstaclePeriod;
        spawnWave();
    }
    // Existing solids age out; the last moments fade instead of popping.
    for (size_t i = 0; i < obstacles.size();) {
        obstacles[i].life -= dt;
        if (obstacles[i].life <= 0.0f) {
            obstacles.erase(obstacles.begin() + static_cast<std::ptrdiff_t>(i));
        } else {
            ++i;
        }
    }

    for (size_t i = 0; i < warnings.size();) {
        warnings[i].timer -= dt;
        if (warnings[i].timer <= 0.0f) {
            const Cell c = warnings[i].cell;
            warnings.erase(warnings.begin() + static_cast<std::ptrdiff_t>(i));
            // The snake may have moved onto the warned cell while it blinked: never
            // materialise a solid on the snake, the food or the special food.
            if (isSnake(c) || c == food || (hasSpecial && c == specialFood)) {
                continue;
            }
            if (static_cast<int>(obstacles.size()) >= kMaxObstacles) {
                obstacles.pop_front(); // drop the oldest solid obstacle
            }
            obstacles.push_back(Obstacle{c, kObstacleLife});
            audio.play(SfxId::ObstacleSpawn);
        } else {
            ++i;
        }
    }
}

// Solid obstacle or own body. Arena edges wrap, so walls are not deadly.
// Warning cells are not solid yet.
bool Game::checkCollision(const Cell& c, bool ignoreTail) const {
    if (isSolid(c)) {
        return true;
    }
    const size_t n = snake.size();
    for (size_t i = 0; i < n; ++i) {
        if (ignoreTail && n > 1 && i + 1 == n) {
            continue; // tail cell vacates this tick
        }
        if (snake[i] == c) {
            return true;
        }
    }
    return false;
}

void Game::queueTurn(Dir wanted) {
    if (over || hasBuffered) {
        return;
    }
    if (isOpposite(wanted, dir)) {
        return;
    }
    buffered = wanted;
    hasBuffered = true;
}

// One fixed logic tick.
void Game::update() {
    if (over) {
        return;
    }

    // The previous tick's interval just finished: the visual head has now reached
    // whatever it ate, so the deferred eating effects (and the bite) fire here.
    resolvePendingBites();

    // Snapshot for the render interpolation between this tick and the next one.
    prevSnake = snake;

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
    const Cell exitCell = head;
    const Cell rawHead = stepRaw(head, dir);

    // Wrap around the arena edges instead of hitting a wall.
    const bool wrapped = (rawHead.x < 0 || rawHead.x >= kCols || rawHead.y < 0 || rawHead.y >= kRows);
    head = wrapCell(rawHead);
    if (wrapped) {
        ghosts.push_back(GhostHead{exitCell, dir, kGhostLife, kGhostLife});
        ghosts.push_back(GhostHead{head, dir, kGhostLife, kGhostLife});
    }

    // Length decides who wins: a shorter AI is eaten, an equal/longer one kills.
    if (resolvePlayerAiCollision(*this, head)) {
        return;
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
        if (highScore > 0 && score > highScore) {
            isNewHighRecord = true;
        }
        // Gameplay is instant, visuals are not: the apple keeps shrinking toward
        // the mouth until the drawn head actually arrives (resolvePendingBites).
        PendingBite bite;
        bite.pos = cellToWorld(food, 0.8f);
        bite.color = kFood;
        bite.special = false;
        bite.label = "+10";
        bite.labelColor = kGood;
        pendingBites.push_back(bite);
        spawnFood();
    } else if (eatsSpecial) {
        score += 10;
        if (highScore > 0 && score > highScore) {
            isNewHighRecord = true;
        }
        const Vector3 sfPos = cellToWorld(specialFood, 0.9f);
        hasSpecial = false;
        applyEffect(specialEffect);
        PendingBite bite;
        bite.pos = sfPos;
        bite.color = kSpecial;
        bite.special = true;
        bite.label = fxText;
        bite.labelColor = fxColor;
        pendingBites.push_back(bite);
    } else {
        snake.pop_back();
    }

    updateAiTick(*this);
}

void Game::initAmbientParticles() {
    ambientParticles.clear();
    std::uniform_real_distribution<float> distX(0.0f, static_cast<float>(kWinW));
    std::uniform_real_distribution<float> distY(0.0f, static_cast<float>(kWinH));
    std::uniform_real_distribution<float> distVy(-16.0f, -6.0f);
    std::uniform_real_distribution<float> distVx(-6.0f, 6.0f);
    std::uniform_real_distribution<float> distSz(1.5f, 3.2f);
    std::uniform_int_distribution<int> distA(25, 65);

    for (int i = 0; i < 35; ++i) {
        Particle p;
        p.pos = Vector2{distX(rng), distY(rng)};
        p.vel = Vector2{distVx(rng), distVy(rng)};
        const unsigned char a = static_cast<unsigned char>(distA(rng));
        p.color = Color{90, 140, 200, a};
        p.size = distSz(rng);
        p.life = 100.0f;
        p.maxLife = 100.0f;
        ambientParticles.push_back(p);
    }
}

void Game::spawnParticles(Vector3 pos, Color color, int count, float speed, float size, float life,
                          bool gravity) {
    std::uniform_real_distribution<float> distAngle(0.0f, 6.2831853f);
    std::uniform_real_distribution<float> distSpd(speed * 0.35f, speed);
    std::uniform_real_distribution<float> distSz(size * 0.6f, size * 1.3f);
    std::uniform_real_distribution<float> distLf(life * 0.6f, life * 1.2f);
    std::uniform_real_distribution<float> distUp(0.15f, 1.0f);

    const float scale = particleScale(settings.particles);
    const int scaled = std::max(0, static_cast<int>(std::lround(static_cast<float>(count) * scale)));
    for (int i = 0; i < scaled; ++i) {
        const float ang = distAngle(rng);
        const float spd = distSpd(rng);
        WorldParticle p;
        p.pos = pos;
        p.vel = Vector3{std::cos(ang) * spd, distUp(rng) * spd, std::sin(ang) * spd};
        p.color = color;
        p.size = distSz(rng);
        p.life = distLf(rng);
        p.maxLife = p.life;
        p.gravity = gravity;
        particles.push_back(p);
    }
}

void Game::addFloatingText(const std::string& text, Vector3 pos, Color color, float duration) {
    FloatingText ft;
    ft.text = text;
    ft.pos = pos;
    ft.color = color;
    ft.life = duration;
    ft.maxLife = duration;
    floatingTexts.push_back(ft);
}

void Game::triggerShake(float duration, float intensity) {
    if (!shakeEnabled) {
        return;
    }
    shakeTimer = duration;
    shakeIntensity = intensity;
}

void Game::togglePause() {
    if (mode != Mode::Playing || over) {
        return;
    }
    paused = !paused;
    lastTickInterval = tickInterval(); // resume without an alpha jump
    audio.play(paused ? SfxId::Pause : SfxId::Resume);
    if (paused) {
        boostHeld = false;
        uiBoostHeld = false;
        hasBuffered = false; // a turn queued before the pause must not fire on resume
        buffered = dir;
        boostRequiresRelease = true;
    }
}

void Game::updateVfx(float dt) {
    animTime += dt;

    for (Particle& p : ambientParticles) {
        p.pos.x += p.vel.x * dt;
        p.pos.y += p.vel.y * dt;
        if (p.pos.y < -10.0f) {
            p.pos.y = static_cast<float>(kWinH) + 10.0f;
            std::uniform_real_distribution<float> distX(0.0f, static_cast<float>(kWinW));
            p.pos.x = distX(rng);
        }
        if (p.pos.x < -10.0f) {
            p.pos.x = static_cast<float>(kWinW) + 10.0f;
        } else if (p.pos.x > static_cast<float>(kWinW) + 10.0f) {
            p.pos.x = -10.0f;
        }
    }

    if (paused) {
        return; // freeze in-game particles and floating texts during pause
    }

    if (shakeTimer > 0.0f) {
        shakeTimer -= dt;
        if (shakeTimer <= 0.0f) {
            shakeTimer = 0.0f;
        }
    }

    for (size_t i = 0; i < particles.size();) {
        particles[i].pos.x += particles[i].vel.x * dt;
        particles[i].pos.y += particles[i].vel.y * dt;
        particles[i].pos.z += particles[i].vel.z * dt;
        if (particles[i].gravity) {
            particles[i].vel.y -= 9.0f * dt;
        }
        particles[i].vel.x *= 0.94f;
        particles[i].vel.y *= 0.97f;
        particles[i].vel.z *= 0.94f;
        if (particles[i].pos.y < 0.06f) {
            particles[i].pos.y = 0.06f;
            particles[i].vel.y = std::fabs(particles[i].vel.y) * 0.3f;
        }
        particles[i].life -= dt;
        if (particles[i].life <= 0.0f) {
            particles.erase(particles.begin() + static_cast<std::ptrdiff_t>(i));
        } else {
            ++i;
        }
    }

    for (size_t i = 0; i < floatingTexts.size();) {
        floatingTexts[i].pos.y += 0.9f * dt;
        floatingTexts[i].life -= dt;
        if (floatingTexts[i].life <= 0.0f) {
            floatingTexts.erase(floatingTexts.begin() + static_cast<std::ptrdiff_t>(i));
        } else {
            ++i;
        }
    }
}

// Frame driver: survival score, obstacle timers, and tick accumulation.
void Game::stepFrame(float dt) {
    updateVfx(dt);
    updateCamera(dt);

    // Boost start/end are edges, not per-frame events. Pausing keeps the state so
    // resuming a held boost does not replay the start sound.
    const bool boosting = boostActive();
    if (!paused && !over && boosting != wasBoosting) {
        wasBoosting = boosting;
        audio.play(boosting ? SfxId::BoostStart : SfxId::BoostEnd);
    }

    foodAge += dt;
    specialAge += dt;
    if (swallowClock >= 0.0f) {
        swallowClock += dt;
        if (swallowClock > 0.9f) {
            swallowClock = -1.0f;
        }
    }
    if (over && overFade < 1.0f) {
        overFade = std::min(1.0f, overFade + dt * 3.5f);
    }
    if (transitionT > 0.0f) {
        transitionT = std::max(0.0f, transitionT - dt * 2.6f);
    }

    // Optional per-skin trail (embers / sparkle / neon glow) from the tail.
    trailTimer += dt;
    const SkinDef& trailSkin = skin();
    if (mode == Mode::Playing && !paused && !over && !snake.empty() &&
        trailSkin.trail != TrailKind::None && trailTimer > 0.075f) {
        trailTimer = 0.0f;
        const Vector3 tailPos = cellToWorld(snake.back(), 0.3f);
        switch (trailSkin.trail) {
        case TrailKind::Embers:
            spawnParticles(tailPos, Color{255, 140, 50, 210}, 1, 1.2f, 0.1f, 0.55f);
            break;
        case TrailKind::Sparkle:
            spawnParticles(tailPos, Color{200, 235, 255, 200}, 1, 0.9f, 0.08f, 0.45f, false);
            break;
        case TrailKind::Glow:
            spawnParticles(tailPos,
                           Color{trailSkin.accent.r, trailSkin.accent.g, trailSkin.accent.b, 150},
                           1, 0.6f, 0.09f, 0.35f, false);
            break;
        case TrailKind::None:
            break;
        }
    }

    if (mode == Mode::Menu || paused) {
        return;
    }
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

    if (boostActive() && !snake.empty()) {
        const Cell& tail = snake.back();
        const Vector3 tPos = cellToWorld(tail, 0.35f);
        spawnParticles(tPos, Color{100, 240, 200, 180}, 1, 1.6f, 0.1f, 0.25f, false);
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
    updateAiSpawner(*this, dt);

    // Boost start/stop and the Slow/Haste effects change the tick interval in the
    // middle of a tick. Rescaling the accumulator keeps tickAlpha (and therefore
    // the drawn head) continuous instead of jumping back or forward.
    const float intervalNow = tickInterval();
    if (lastTickInterval > 0.0f && intervalNow > 0.0f &&
        std::fabs(intervalNow - lastTickInterval) > 1e-6f) {
        accumulator *= intervalNow / lastTickInterval;
    }
    lastTickInterval = intervalNow;

    accumulator += dt;
    int guard = 0;
    while (!over && accumulator >= tickInterval() && guard++ < 8) {
        accumulator -= tickInterval();
        update();
    }
    if (accumulator > 1.0f) {
        accumulator = 0.0f; // recover from long stalls without bursting
    }
    const float interval = tickInterval();
    lastTickInterval = interval;
    tickAlpha = (interval > 0.0f) ? std::clamp(accumulator / interval, 0.0f, 1.0f) : 0.0f;

    updateMouth(dt);
}

// Interpolated world position of one segment. The renderer and the camera both
// go through here so the rig always follows the head that is actually drawn.
Vector3 Game::visualSegmentWorld(size_t index, float y) const {
    if (index >= snake.size()) {
        return Vector3{0.0f, y, 0.0f};
    }
    Vector3 cur = cellToWorld(snake[index], y);
    if (prevSnake.size() == snake.size() && index < prevSnake.size() && tickAlpha > 0.0f) {
        const Vector3 prev = cellToWorld(prevSnake[index], y);
        // A wrapped segment jumps across the arena: never lerp through the floor.
        if (std::fabs(cur.x - prev.x) <= 1.01f && std::fabs(cur.z - prev.z) <= 1.01f) {
            cur.x = prev.x + (cur.x - prev.x) * tickAlpha;
            cur.z = prev.z + (cur.z - prev.z) * tickAlpha;
        }
    }
    return cur;
}

// Smooth camera rig. The focus tracks the interpolated head (never the logical
// cell), its speed is capped so a wrap glides instead of snapping, the
// third-person rig orbits around a smoothed yaw, and mode changes cross-fade.
void Game::updateCamera(float dt) {
    dt = std::clamp(dt, 0.0f, kCamMaxDt);

    // --- Heading used by the third-person orbit and by the iso/top look-ahead.
    Vector3 moveDir = dirToWorld(dir);
    if (!snake.empty() && prevSnake.size() == snake.size()) {
        int dx = snake.front().x - prevSnake.front().x;
        int dy = snake.front().y - prevSnake.front().y;
        if (dx > 1) {
            dx -= kCols;
        } else if (dx < -1) {
            dx += kCols;
        }
        if (dy > 1) {
            dy -= kRows;
        } else if (dy < -1) {
            dy += kRows;
        }
        if (std::abs(dx) + std::abs(dy) == 1) {
            moveDir = Vector3{static_cast<float>(dx), 0.0f, static_cast<float>(dy)};
        }
    }
    const float yawTarget = std::atan2(moveDir.z, moveDir.x);
    float yawDelta = wrapAngle(yawTarget - camYaw);
    if (dt > 0.0f) {
        const float maxStep = kCamYawMaxSpeed * dt;
        float step = yawDelta * (1.0f - std::exp(-kCamYawSmoothing * dt));
        if (step > maxStep) {
            step = maxStep;
        } else if (step < -maxStep) {
            step = -maxStep;
        }
        camYaw = wrapAngle(camYaw + step);
    }
    if (!std::isfinite(camYaw)) {
        camYaw = yawTarget;
    }
    const Vector3 heading{std::cos(camYaw), 0.0f, std::sin(camYaw)};

    // --- Focus: interpolated head, partly damped in the overhead rigs.
    Vector3 focus{0.0f, 0.0f, 0.0f};
    if (mode == Mode::Playing && !snake.empty()) {
        const Vector3 head = visualHeadWorld(0.0f);
        const float share = (cameraMode == CameraMode::Third) ? 1.0f : kCamFocusShare;
        focus.x = head.x * share;
        focus.z = head.z * share;
        if (cameraMode != CameraMode::Third) {
            focus.x += heading.x * kCamLookAhead;
            focus.z += heading.z * kCamLookAhead;
        }
    }
    if (dt <= 0.0f) {
        camTarget = focus;
    } else {
        const float k = 1.0f - std::exp(-kCamFocusSmoothing * dt);
        Vector3 delta{(focus.x - camTarget.x) * k, 0.0f, (focus.z - camTarget.z) * k};
        const float len = std::sqrt(delta.x * delta.x + delta.z * delta.z);
        const float maxMove = kCamMaxSpeed * dt;
        if (len > maxMove && len > 1e-6f) {
            delta.x *= maxMove / len;
            delta.z *= maxMove / len;
        }
        camTarget.x += delta.x;
        camTarget.z += delta.z;
        camTarget.y = 0.0f;
    }

    // --- Pose for the current mode.
    Vector3 desired{0.0f, 0.0f, 0.0f};
    Vector3 look = camTarget;
    float fovTarget = kCamFovIso;
    switch (cameraMode) {
    case CameraMode::Top:
        desired = Vector3{camTarget.x, kCamTopHeight, camTarget.z + 0.01f};
        look = Vector3{camTarget.x, 0.0f, camTarget.z};
        break;
    case CameraMode::Third: {
        const float height = std::max(kCamThirdMinHeight, kCamThirdHeight);
        desired = Vector3{camTarget.x - heading.x * kCamThirdRadius, height,
                          camTarget.z - heading.z * kCamThirdRadius};
        look = Vector3{camTarget.x + heading.x * kCamThirdLookAhead, kCamThirdLookY,
                       camTarget.z + heading.z * kCamThirdLookAhead};
        fovTarget = kCamFovThird;
        break;
    }
    case CameraMode::Iso:
    default:
        desired = Vector3{camTarget.x, kCamIsoHeight, camTarget.z + kCamIsoBack};
        look = camTarget;
        break;
    }

    // A mode switch cross-fades from the pose captured at the moment of the switch.
    if (cameraMode != camAppliedMode) {
        camFrom = camera;
        camBlend = kCamModeBlend;
        camAppliedMode = cameraMode;
    }

    Vector3 newPos = desired;
    Vector3 newLook = look;
    if (cameraMode != CameraMode::Third && dt > 0.0f) {
        // Overhead rigs can lerp their position: no body sits in front of the lens.
        const float k = 1.0f - std::exp(-kCamFollowSmoothing * dt);
        newPos = lerp3(camera.position, desired, k);
        newLook = lerp3(camera.target, look, k);
    }
    // The third-person rig orbits (its position is derived from the smoothed yaw
    // and focus), so it is set directly rather than lerped through the snake.

    if (camBlend > 0.0f && dt > 0.0f) {
        camBlend = std::max(0.0f, camBlend - dt);
        const float t = 1.0f - camBlend / kCamModeBlend;
        const float s = t * t * (3.0f - 2.0f * t);
        newPos = lerp3(camFrom.position, newPos, s);
        newLook = lerp3(camFrom.target, newLook, s);
    }

    camera.position = newPos;
    camera.target = newLook;
    if (dt > 0.0f) {
        const float fk = 1.0f - std::exp(-kCamFovSmoothing * dt);
        camera.fovy += (fovTarget - camera.fovy) * fk;
    } else {
        camera.fovy = fovTarget;
    }
    if (!std::isfinite(camera.position.x) || !std::isfinite(camera.position.y) ||
        !std::isfinite(camera.position.z) || !std::isfinite(camera.target.x) ||
        !std::isfinite(camera.target.z) || !std::isfinite(camera.fovy)) {
        camera.position = desired;
        camera.target = look;
        camera.fovy = fovTarget;
    }
    if (camera.position.y < 0.5f) {
        camera.position.y = 0.5f;
    }
    camera.up = Vector3{0.0f, 1.0f, 0.0f};
    camera.projection = CAMERA_PERSPECTIVE;
}

// Fires the actual eating feedback the moment the drawn head reaches the cell.
void Game::resolvePendingBites() {
    if (pendingBites.empty()) {
        return;
    }
    for (const PendingBite& bite : pendingBites) {
        if (bite.special) {
            spawnParticles(bite.pos, kSpecial, 22, 5.5f, 0.18f, 0.6f);
            spawnParticles(bite.pos, kSpecialHalo, 12, 4.0f, 0.13f, 0.5f);
            triggerShake(0.18f, 2.8f);
        } else {
            spawnParticles(bite.pos, kFood, 14, 4.5f, 0.16f, 0.45f);
            spawnParticles(bite.pos, Color{255, 230, 160, 255}, 6, 3.0f, 0.11f, 0.35f);
            audio.play(SfxId::EatFood);
        }
        addFloatingText(bite.label,
                        Vector3{bite.pos.x, bite.pos.y + (bite.special ? 0.6f : 0.5f), bite.pos.z},
                        bite.labelColor, bite.special ? 1.3f : 0.9f);
    }
    pendingBites.clear();
    swallowClock = 0.0f;
    triggerBite();
}

void Game::triggerBite() {
    if (!animations) {
        biteT = kMouthBiteIdle;
        mouthLock = 0.0f;
        mouthOpen = 0.0f;
        return;
    }
    biteT = 0.0f;
    mouthLock = kMouthLockTime;
}

// How many cells ahead (0 == right here) an edible target sits, or -1 if the
// head must keep its mouth shut. Blocks on obstacles and on its own body.
float Game::mouthTargetDistance() const {
    if (snake.empty()) {
        return -1.0f;
    }
    // A bite already registered is the nearest target: the apple is moving in.
    float best = -1.0f;
    for (const PendingBite& bite : pendingBites) {
        const float d = 1.0f - std::clamp(bite.progress, 0.0f, 1.0f);
        if (best < 0.0f || d < best) {
            best = d;
        }
    }
    if (best >= 0.0f) {
        return best;
    }

    int cells = 0;
    Cell target{0, 0};
    if (!findEdibleAhead(cells, target)) {
        return -1.0f;
    }
    return static_cast<float>(cells) + (1.0f - std::clamp(tickAlpha, 0.0f, 1.0f));
}

bool Game::findEdibleAhead(int& cellsOut, Cell& cellOut) const {
    if (snake.empty()) {
        return false;
    }
    Cell c = snake.front();
    for (int cells = 1; cells <= kMouthScanCells; ++cells) {
        c = nextCellWrapped(c, dir);
        if (isSolid(c)) {
            return false; // an obstacle blocks the line
        }
        bool ownBody = false;
        for (size_t i = 1; i < snake.size(); ++i) {
            if (snake[i] == c) {
                ownBody = true;
                break;
            }
        }
        if (ownBody) {
            return false;
        }
        bool edible = (c == food) || (hasSpecial && c == specialFood);
        bool rivalHere = false;
        if (!edible) {
            for (const AiSnake& rival : ai) {
                if (rival.dying) {
                    continue;
                }
                for (const Cell& segment : rival.body) {
                    if (segment == c) {
                        rivalHere = true;
                        edible = rival.body.size() < snake.size();
                        break;
                    }
                }
                if (rivalHere) {
                    break;
                }
            }
        }
        if (rivalHere && !edible) {
            return false; // a rival that would kill us is never a target
        }
        if (edible) {
            cellsOut = cells;
            cellOut = c;
            return true;
        }
    }
    return false;
}

// Pupils follow the nearest edible target, or the turn that is queued up.
Vector2 Game::headGaze() const {
    if (!animations || snake.empty()) {
        return Vector2{0.0f, 0.0f};
    }
    int cells = 0;
    Cell target{0, 0};
    if (findEdibleAhead(cells, target)) {
        int dx = target.x - snake.front().x;
        int dy = target.y - snake.front().y;
        if (dx > kCols / 2) {
            dx -= kCols;
        } else if (dx < -(kCols / 2)) {
            dx += kCols;
        }
        if (dy > kRows / 2) {
            dy -= kRows;
        } else if (dy < -(kRows / 2)) {
            dy += kRows;
        }
        const Vector3 fwd = dirToWorld(dir);
        const Vector3 right{-fwd.z, 0.0f, fwd.x};
        const float wx = static_cast<float>(dx);
        const float wz = static_cast<float>(dy);
        Vector2 gaze{right.x * wx + right.z * wz, fwd.x * wx + fwd.z * wz};
        const float len = std::sqrt(gaze.x * gaze.x + gaze.y * gaze.y);
        if (len > 1e-4f) {
            gaze.x /= len;
            gaze.y /= len;
            return gaze;
        }
        return Vector2{0.0f, 0.0f};
    }
    if (hasBuffered && !isOpposite(buffered, dir) && buffered != dir) {
        const Vector3 fwd = dirToWorld(dir);
        const Vector3 right{-fwd.z, 0.0f, fwd.x};
        const Vector3 want = dirToWorld(buffered);
        return Vector2{right.x * want.x + right.z * want.z, fwd.x * want.x + fwd.z * want.z};
    }
    return Vector2{0.0f, 0.0f};
}

// Per-frame jaw animation: open when food is close ahead, snap shut on the bite.
void Game::updateMouth(float dt) {
    dt = std::clamp(dt, 0.0f, kCamMaxDt);

    for (PendingBite& bite : pendingBites) {
        bite.progress = std::clamp(tickAlpha, 0.0f, 1.0f);
    }

    if (biteT >= 0.0f) {
        biteT += dt;
        if (biteT > 0.6f) {
            biteT = kMouthBiteIdle;
        }
    }
    if (mouthLock > 0.0f) {
        mouthLock = std::max(0.0f, mouthLock - dt);
    }

    if (!animations) {
        mouthTarget = 0.0f;
        mouthOpen = 0.0f;
        return;
    }

    const bool canOpen = mode == Mode::Playing && !paused && !over && mouthLock <= 0.0f;
    float target = 0.0f;
    if (canOpen) {
        const float d = mouthTargetDistance();
        if (d >= 0.0f && d <= kMouthCloseDistance) {
            const float span = kMouthCloseDistance - kMouthOpenDistance;
            const float t = (span > 1e-6f) ? std::clamp((kMouthCloseDistance - d) / span, 0.0f, 1.0f)
                                           : 1.0f;
            target = t * t * (3.0f - 2.0f * t);
        }
    }
    mouthTarget = target;

    // The bite closes fast (kMouthCloseBiteTime) no matter how fast it opened.
    if (biteT >= 0.0f && biteT < kMouthCloseBiteTime && dt > 0.0f) {
        mouthOpen = std::max(0.0f, mouthOpen - dt / kMouthCloseBiteTime);
    }
    const float speed = (mouthTarget > mouthOpen) ? kMouthOpenSpeed : kMouthCloseSpeed;
    const float step = speed * dt;
    if (mouthOpen < mouthTarget) {
        mouthOpen = std::min(mouthTarget, mouthOpen + step);
    } else {
        mouthOpen = std::max(mouthTarget, mouthOpen - step);
    }
    mouthOpen = std::clamp(mouthOpen, 0.0f, 1.0f);
}

const SkinDef& Game::skin() const {
    const int index = std::clamp(settings.skin, 0, kSkinCount - 1);
    return kSkins[index];
}

Color Game::headColor() const {
    const SkinDef& s = skin();
    return s.rainbow ? rainbowColor(animTime * 1.6f) : s.head;
}

Color Game::eyeColor() const { return skin().eye; }

Color Game::snakeColorAt(size_t index, size_t count) const {
    const SkinDef& s = skin();
    if (s.rainbow) {
        return rainbowColor(animTime * 1.6f + static_cast<float>(index) * 0.42f);
    }
    const float t = (count > 1) ? static_cast<float>(index) / static_cast<float>(count - 1) : 0.0f;
    const auto mix = [](unsigned char a, unsigned char b, float k) {
        return static_cast<unsigned char>(static_cast<float>(a) +
                                          (static_cast<float>(b) - static_cast<float>(a)) * k);
    };
    return Color{mix(s.head.r, s.tail.r, t), mix(s.head.g, s.tail.g, t),
                 mix(s.head.b, s.tail.b, t), 255};
}

void Game::applySettings() {
    normalTick = kNormalTick * speedTickScale(settings.speed);
    audio.applyVolumes(settings.masterVol, settings.musicVol, settings.sfxVol, settings.mute != 0);
    animations = settings.animations != 0;
    shakeEnabled = settings.shake != 0;
    cameraMode = static_cast<CameraMode>(std::clamp(settings.camera, 0, 2));
    windowSettingsDirty = true;
}

void Game::persistSettings() { saveSettings(settings, "settings.cfg"); }

// Window-level options cannot go through the canvas: apply them to the real window.
void Game::applyWindowSettings() {
    const bool wantFullscreen = settings.fullscreen != 0;
    if (IsWindowFullscreen() != wantFullscreen) {
        ToggleFullscreen();
    }
    if (!wantFullscreen) {
        const int index = std::clamp(settings.resolution, 0, kResolutionCount - 1);
        SetWindowSize(kResolutions[index].w, kResolutions[index].h);
    }
    if (settings.vsync != 0) {
        SetWindowState(FLAG_VSYNC_HINT);
    } else {
        ClearWindowState(FLAG_VSYNC_HINT);
    }
    SetTargetFPS(fpsTarget(settings.fpsLimit));
}

void Game::gameOver() {
    over = true;
    // A pending bite never lands if the round ends first: drop it silently.
    pendingBites.clear();
    mouthTarget = 0.0f;
    mouthOpen = 0.0f;
    biteT = kMouthBiteIdle;
    mouthLock = 0.0f;
    audio.play(SfxId::GameOver);
    boostHeld = false;
    paused = false;
    boostTicks = 0;
    if (score > highScore) {
        if (highScore > 0) {
            isNewHighRecord = true;
        }
        highScore = score;
    }
    triggerShake(0.35f, 4.5f);
    overFade = animations ? 0.0f : 1.0f;
    if (!snake.empty()) {
        const Cell& h = snake.front();
        const Vector3 hPos = cellToWorld(h, 0.7f);
        spawnParticles(hPos, Color{255, 75, 75, 255}, 28, 6.0f, 0.2f, 0.7f);
        spawnParticles(hPos, Color{255, 200, 60, 255}, 16, 4.5f, 0.15f, 0.5f);

        // The body shatters too: one shard burst every other segment.
        size_t i = 0;
        for (const Cell& c : snake) {
            if (i++ % 2 == 0) {
                spawnParticles(cellToWorld(c, 0.5f), headColor(), 3, 3.2f, 0.15f, 0.6f);
            }
        }
    }
}

} // namespace snake
