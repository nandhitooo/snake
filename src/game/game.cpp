#include "game/game.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace snake {

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
    initAmbientParticles();
    reset();
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
    swallowClock = -1.0f;
    foodAge = 0.0f;
    specialAge = 0.0f;
    overFade = 0.0f;
    spawnFood();
}

void Game::startGame() {
    reset();
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
    const float base = boostActive() ? kBoostTick : kNormalTick;
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
    return !(c == food) && !isSnake(c) && !isSolid(c) && !isWarning(c);
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
    for (int i = 0; i < count && !candidates.empty(); ++i) {
        std::uniform_int_distribution<size_t> pick(0, candidates.size() - 1);
        const size_t index = pick(rng);
        warnings.push_back(Warning{candidates[index], kWarningTime});
        candidates[index] = candidates.back();
        candidates.pop_back();
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
    switch (dir) {
    case Dir::Up:
        --head.y;
        break;
    case Dir::Down:
        ++head.y;
        break;
    case Dir::Left:
        --head.x;
        break;
    case Dir::Right:
        ++head.x;
        break;
    }

    // Wrap around the arena edges instead of hitting a wall.
    const bool wrapped = (head.x < 0 || head.x >= kCols || head.y < 0 || head.y >= kRows);
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
        swallowClock = 0.0f;
        score += 10;
        if (highScore > 0 && score > highScore) {
            isNewHighRecord = true;
        }
        const Vector3 fPos = cellToWorld(food, 0.8f);
        spawnParticles(fPos, kFood, 14, 4.5f, 0.16f, 0.45f);
        spawnParticles(fPos, Color{255, 230, 160, 255}, 6, 3.0f, 0.11f, 0.35f);
        addFloatingText("+10", Vector3{fPos.x, 1.3f, fPos.z}, kGood, 0.9f);
        spawnFood();
    } else if (eatsSpecial) {
        swallowClock = 0.0f;
        score += 10;
        if (highScore > 0 && score > highScore) {
            isNewHighRecord = true;
        }
        const Vector3 sfPos = cellToWorld(specialFood, 0.9f);
        hasSpecial = false;
        applyEffect(specialEffect);
        spawnParticles(sfPos, kSpecial, 22, 5.5f, 0.18f, 0.6f);
        spawnParticles(sfPos, kSpecialHalo, 12, 4.0f, 0.13f, 0.5f);
        triggerShake(0.18f, 2.8f);
        addFloatingText(fxText, Vector3{sfPos.x, 1.5f, sfPos.z}, fxColor, 1.3f);
    } else {
        snake.pop_back();
    }
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

    for (int i = 0; i < count; ++i) {
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
    shakeTimer = duration;
    shakeIntensity = intensity;
}

void Game::togglePause() {
    if (mode != Mode::Playing || over) {
        return;
    }
    paused = !paused;
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
    tickAlpha = (interval > 0.0f) ? std::clamp(accumulator / interval, 0.0f, 1.0f) : 0.0f;
}

// Smooth camera rig: follows the head at 45% so the arena stays readable.
void Game::updateCamera(float dt) {
    const float k = 1.0f - std::exp(-7.0f * dt);
    Vector3 focus{0.0f, 0.0f, 0.0f};
    if (mode == Mode::Playing && !snake.empty()) {
        const Vector3 head = cellToWorld(snake.front(), 0.0f);
        focus.x = head.x * 0.45f;
        focus.z = head.z * 0.45f;
    }
    if (dt <= 0.0f) {
        camTarget = focus;
    } else {
        camTarget.x += (focus.x - camTarget.x) * k;
        camTarget.y = 0.0f;
        camTarget.z += (focus.z - camTarget.z) * k;
    }

    const Vector3 forward = dirToWorld(dir);
    Vector3 desired{0.0f, 0.0f, 0.0f};
    Vector3 look = camTarget;
    float fovy = 45.0f;
    switch (cameraMode) {
    case CameraMode::Top:
        desired = Vector3{camTarget.x, 25.0f, camTarget.z + 0.01f};
        look = Vector3{camTarget.x, 0.0f, camTarget.z};
        break;
    case CameraMode::Third:
        desired = Vector3{camTarget.x - forward.x * 7.5f, 5.2f, camTarget.z - forward.z * 7.5f};
        look = Vector3{camTarget.x + forward.x * 2.5f, 0.7f, camTarget.z + forward.z * 2.5f};
        fovy = 55.0f;
        break;
    case CameraMode::Iso:
    default:
        desired = Vector3{camTarget.x, 22.0f, camTarget.z + 17.0f};
        look = camTarget;
        break;
    }

    if (dt <= 0.0f) {
        camera.position = desired;
        camera.target = look;
    } else {
        camera.position.x += (desired.x - camera.position.x) * k;
        camera.position.y += (desired.y - camera.position.y) * k;
        camera.position.z += (desired.z - camera.position.z) * k;
        camera.target.x += (look.x - camera.target.x) * k;
        camera.target.y += (look.y - camera.target.y) * k;
        camera.target.z += (look.z - camera.target.z) * k;
    }
    camera.up = Vector3{0.0f, 1.0f, 0.0f};
    camera.fovy = fovy;
    camera.projection = CAMERA_PERSPECTIVE;
}

Color Game::headColor() const { return kSnakeHead; }

Color Game::eyeColor() const { return kEye; }

Color Game::snakeColorAt(size_t index, size_t count) const {
    const float t = (count > 1) ? static_cast<float>(index) / static_cast<float>(count - 1) : 0.0f;
    const auto mix = [](unsigned char a, unsigned char b, float k) {
        return static_cast<unsigned char>(static_cast<float>(a) +
                                          (static_cast<float>(b) - static_cast<float>(a)) * k);
    };
    return Color{mix(kSnakeHead.r, kSnakeTail.r, t), mix(kSnakeHead.g, kSnakeTail.g, t),
                 mix(kSnakeHead.b, kSnakeTail.b, t), 255};
}

void Game::gameOver() {
    over = true;
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
