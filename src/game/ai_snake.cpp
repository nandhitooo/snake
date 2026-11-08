#include "game/ai_snake.hpp"

#include "game/audio.hpp"
#include "game/game.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <deque>
#include <vector>

namespace snake {
namespace {

Cell stepCell(const Cell& c, Dir d) {
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
    // Same wrap rule as the player: arena edges teleport instead of killing.
    n.x = (n.x + kCols) % kCols;
    n.y = (n.y + kRows) % kRows;
    return n;
}

Dir opposite(Dir d) {
    switch (d) {
    case Dir::Up:
        return Dir::Down;
    case Dir::Down:
        return Dir::Up;
    case Dir::Left:
        return Dir::Right;
    case Dir::Right:
    default:
        return Dir::Left;
    }
}

bool bodyHas(const std::deque<Cell>& body, const Cell& c) {
    for (const Cell& segment : body) {
        if (segment == c) {
            return true;
        }
    }
    return false;
}

bool reservedForPlayer(const Game& g, const Cell& c) {
    // AI snakes never eat: food cells stay reserved for the player.
    return c == g.food || (g.hasSpecial && c == g.specialFood);
}

int manhattan(const Cell& a, const Cell& b) {
    return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}

float nextDelay(Game& g) {
    std::uniform_real_distribution<float> dist(10.0f, 20.0f);
    return dist(g.rng);
}

void killAi(AiSnake& ai) {
    if (!ai.dying) {
        ai.dying = true;
        ai.deathT = 0.35f;
    }
}

bool cellBlocked(const Game& g, const Cell& c, size_t selfIndex) {
    if (g.isSolid(c) || g.isSnake(c) || reservedForPlayer(g, c)) {
        return true;
    }
    for (size_t i = 0; i < g.ai.size(); ++i) {
        if (i == selfIndex || g.ai[i].dying) {
            continue;
        }
        if (bodyHas(g.ai[i].body, c)) {
            return true;
        }
    }
    return false;
}

void trySpawnAi(Game& g, const Cell& at, size_t length) {
    if (g.ai.size() >= 2 || length < 3 || !g.isEmpty(at)) {
        return;
    }
    std::uniform_int_distribution<int> dirPick(0, 3);
    for (int attempt = 0; attempt < 10; ++attempt) {
        const Dir d = static_cast<Dir>(dirPick(g.rng));
        std::deque<Cell> body;
        Cell c = at;
        bool ok = true;
        for (size_t i = 0; i < length; ++i) {
            if (!g.isEmpty(c) || reservedForPlayer(g, c)) {
                ok = false;
                break;
            }
            body.push_back(c);
            c = stepCell(c, opposite(d));
        }
        if (!ok) {
            continue;
        }
        AiSnake ai;
        ai.body = body;
        ai.prevBody = body;
        ai.dir = d;
        ai.spawnT = g.animations ? 0.0f : 1.0f;
        std::uniform_real_distribution<float> phaseDist(0.0f, kAiBlinkPeriod);
        ai.blinkPhase = phaseDist(g.rng); // rivals blink out of sync
        g.ai.push_back(ai);
        audio.play(SfxId::AiSpawn);
        return;
    }
}

void scheduleAiSpawn(Game& g) {
    if (g.ai.size() >= 2 || g.snake.empty()) {
        return;
    }
    const int maxLen = static_cast<int>(g.snake.size()) + 2;
    std::uniform_int_distribution<int> lenDist(3, std::max(3, maxLen));
    const size_t length = static_cast<size_t>(lenDist(g.rng));

    const Cell head = g.snake.front();
    std::vector<Cell> candidates;
    for (int y = 0; y < kRows; ++y) {
        for (int x = 0; x < kCols; ++x) {
            const Cell c{x, y};
            if (!g.isEmpty(c) || reservedForPlayer(g, c)) {
                continue;
            }
            if (std::max(std::abs(c.x - head.x), std::abs(c.y - head.y)) < 6) {
                continue;
            }
            bool alreadyWarned = false;
            for (const AiSpawnWarning& w : g.aiWarnings) {
                if (w.cell == c) {
                    alreadyWarned = true;
                    break;
                }
            }
            if (!alreadyWarned) {
                candidates.push_back(c);
            }
        }
    }
    if (candidates.empty()) {
        return;
    }

    std::uniform_int_distribution<size_t> pick(0, candidates.size() - 1);
    AiSpawnWarning warning;
    warning.cell = candidates[pick(g.rng)];
    warning.timer = 1.2f;
    warning.length = length;
    g.aiWarnings.push_back(warning);
}

void growPlayer(Game& g, size_t segments) {
    if (g.snake.empty()) {
        return;
    }
    const Cell tail = g.snake.back();
    for (size_t i = 0; i < segments; ++i) {
        g.snake.push_back(tail);
    }
}

void eatAiSnake(Game& g, size_t index) {
    AiSnake& ai = g.ai[index];
    const size_t length = ai.body.size();
    const Cell head = ai.body.empty() ? g.snake.front() : ai.body.front();
    const int points = static_cast<int>(length) * 10;

    g.score += points;
    // +1 compensates the tail segment the player drops on the same tick, so the
    // net growth is at least one segment.
    growPlayer(g, std::max<size_t>(1, length / 2) + 1);
    g.addFloatingText("+" + std::to_string(points), cellToWorld(head, 1.5f), kGood, 1.1f);
    g.spawnParticles(cellToWorld(head, 0.6f), Color{120, 255, 160, 220}, 18, 5.0f, 0.18f, 0.6f);
    g.flashPulse = std::max(g.flashPulse, 0.35f); // Phase I4g: small screen flash
    audio.play(SfxId::EatAiSnake);
    // AI are eaten immediately (no deferred bite), so the jaw snaps right away.
    g.triggerBite();
    killAi(ai);
}

} // namespace

void resetAi(Game& g) {
    g.ai.clear();
    g.aiWarnings.clear();
    g.aiSpawnTimer = 10.0f + static_cast<float>(std::abs(static_cast<int>(g.rng() % 1000u))) / 100.0f;
}

void updateAiSpawner(Game& g, float dt) {
    for (size_t i = 0; i < g.aiWarnings.size();) {
        g.aiWarnings[i].timer -= dt;
        if (g.aiWarnings[i].timer <= 0.0f) {
            const Cell at = g.aiWarnings[i].cell;
            const size_t length = g.aiWarnings[i].length;
            g.aiWarnings.erase(g.aiWarnings.begin() + static_cast<std::ptrdiff_t>(i));
            trySpawnAi(g, at, length);
        } else {
            ++i;
        }
    }

    if (g.ai.size() < 2 && g.aiWarnings.empty()) {
        g.aiSpawnTimer -= dt;
        if (g.aiSpawnTimer <= 0.0f) {
            scheduleAiSpawn(g);
            g.aiSpawnTimer = nextDelay(g);
        }
    }

    for (size_t i = 0; i < g.ai.size();) {
        AiSnake& ai = g.ai[i];
        ai.spawnT = g.animations ? std::min(1.0f, ai.spawnT + dt * 4.0f) : 1.0f;
        if (ai.dying) {
            ai.deathT -= dt;
            if (ai.deathT <= 0.0f) {
                g.ai.erase(g.ai.begin() + static_cast<std::ptrdiff_t>(i));
                continue;
            }
        }
        ++i;
    }
}

void updateAiTick(Game& g) {
    std::uniform_real_distribution<float> chance(0.0f, 1.0f);

    for (size_t index = 0; index < g.ai.size(); ++index) {
        AiSnake& ai = g.ai[index];
        if (ai.dying || ai.body.empty()) {
            continue;
        }
        ai.prevBody = ai.body;
        const Cell head = ai.body.front();

        Dir chosen = ai.dir;
        bool moved = false;
        const bool reckless = chance(g.rng) < 0.22f;

        if (reckless) {
            // Blunder: pick any non-reversing direction. It may run into hazards,
            // which is what makes the AI-vs-AI and AI-vs-player rules reachable.
            std::vector<Dir> options;
            for (int d = 0; d < 4; ++d) {
                const Dir dir = static_cast<Dir>(d);
                if (isOpposite(dir, ai.dir)) {
                    continue;
                }
                const Cell next = stepCell(head, dir);
                if (reservedForPlayer(g, next)) {
                    continue;
                }
                options.push_back(dir);
            }
            if (!options.empty()) {
                std::uniform_int_distribution<size_t> pick(0, options.size() - 1);
                chosen = options[pick(g.rng)];
                moved = true;
            }
        } else {
            bool found = false;
            float bestScore = -1.0e9f;
            for (int d = 0; d < 4; ++d) {
                const Dir dir = static_cast<Dir>(d);
                if (isOpposite(dir, ai.dir)) {
                    continue;
                }
                const Cell next = stepCell(head, dir);
                if (cellBlocked(g, next, index) || bodyHas(ai.body, next)) {
                    continue;
                }
                // Prefer food, with a little noise so paths are not identical.
                const float score = -static_cast<float>(manhattan(next, g.food)) + chance(g.rng) * 0.8f;
                if (score > bestScore) {
                    bestScore = score;
                    chosen = dir;
                    found = true;
                }
            }
            moved = found;
        }

        if (!moved) {
            audio.play(SfxId::AiDie);
            killAi(ai); // no legal move: die rather than freeze
            continue;
        }

        const Cell next = stepCell(head, chosen);
        ai.dir = chosen;

        bool died = false;
        if (g.isSolid(next) || g.isSnake(next) || bodyHas(ai.body, next)) {
            died = true;
        }

        for (size_t other = 0; other < g.ai.size() && !died; ++other) {
            if (other == index || g.ai[other].dying) {
                continue;
            }
            if (!bodyHas(g.ai[other].body, next)) {
                continue;
            }
            // Two AI collide: the shorter one dies, equal lengths kill both.
            const size_t mine = ai.body.size();
            const size_t theirs = g.ai[other].body.size();
            if (mine <= theirs) {
                died = true;
            }
            if (theirs <= mine) {
                audio.play(SfxId::AiDie);
                killAi(g.ai[other]);
            }
        }

        if (died) {
            audio.play(SfxId::AiDie);
            killAi(ai);
            continue;
        }

        ai.body.push_front(next);
        ai.body.pop_back();
        ai.spawnT = std::min(1.0f, ai.spawnT + 0.25f);
    }
}

bool resolvePlayerAiCollision(Game& g, const Cell& head) {
    for (size_t i = 0; i < g.ai.size();) {
        if (g.ai[i].dying || !bodyHas(g.ai[i].body, head)) {
            ++i;
            continue;
        }
        if (g.ai[i].body.size() < g.snake.size()) {
            eatAiSnake(g, i); // shorter AI: the player eats it
            ++i;
        } else {
            g.gameOver(); // same size or bigger: the player dies
            return true;
        }
    }
    return false;
}

} // namespace snake
