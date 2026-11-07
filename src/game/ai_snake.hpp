#pragma once

#include <cstddef>

namespace snake {

struct Game;
struct Cell;

// AI snakes live inside Game (see AiSnake in game.hpp); these helpers own their
// behaviour so game.cpp only needs the hook points.
void resetAi(Game& g);
void updateAiSpawner(Game& g, float dt); // per frame: warnings, spawn timer, pop/fade
void updateAiTick(Game& g);              // per logic tick: movement + AI/AI collisions

// Called with the player's next head cell. Returns true if the player died.
bool resolvePlayerAiCollision(Game& g, const Cell& head);

} // namespace snake
