#pragma once

#include "raylib.h"

namespace snake {

inline constexpr int kCols = 32;
inline constexpr int kRows = 18;
inline constexpr int kCell = 20;
inline constexpr int kHud = 48;
inline constexpr int kArenaW = kCols * kCell; // 640
inline constexpr int kArenaH = kRows * kCell; // 360
inline constexpr int kWinW = 960;
inline constexpr int kWinH = 540;
inline constexpr int kBarH = 76;
inline constexpr int kArenaX = (kWinW - kArenaW) / 2;
inline constexpr int kPanelH = kHud + kArenaH + kBarH;
inline constexpr int kPanelX = kArenaX;
inline constexpr int kPanelY = (kWinH - kPanelH) / 2;
inline constexpr int kArenaY = kPanelY + kHud;
inline constexpr int kBarY = kArenaY + kArenaH;

inline constexpr float kNormalTick = 0.16f; // slightly slower than the original 0.12 s
inline constexpr float kBoostTick = 0.08f;  // boost still halves the interval
inline constexpr float kObstaclePeriod = 8.0f;
inline constexpr float kWarningTime = 1.0f;
inline constexpr float kGhostLife = 0.32f; // seconds a wrap ghost stays visible
inline constexpr int kMaxObstacles = 15;
inline constexpr float kObstacleLife = 12.0f; // solid obstacles expire after this long
inline constexpr float kObstacleFade = 1.5f;  // fade-out window before they vanish
inline constexpr int kMinLength = 3;

// Special-food effects (beneficial or harmful, picked at random)
inline constexpr float kEffectDuration = 5.0f; // timed slow/haste length
inline constexpr float kFxMessageTime = 1.6f;  // HUD feedback duration
inline constexpr float kSlowFactor = 1.45f;    // >1 => slower ticks
inline constexpr float kHasteFactor = 0.72f;   // <1 => faster ticks
inline constexpr int kSpecialChancePct = 30;   // chance per normal food spawn
inline constexpr int kBonusScore = 50;
inline constexpr int kGrowSegments = 3;
inline constexpr int kTrimSegments = 2;
inline constexpr int kCurseObstacles = 3;

// Palette
inline const Color kBg{14, 14, 20, 255};
inline const Color kHudBg{26, 28, 38, 255};
inline const Color kArenaBg{20, 22, 30, 255};
inline const Color kGrid{34, 36, 48, 255};
inline const Color kBorder{88, 94, 116, 255};
inline const Color kSnakeHead{130, 225, 140, 255};
inline const Color kSnakeTail{36, 120, 96, 255};
inline const Color kFood{232, 64, 64, 255};
inline const Color kObstacle{112, 116, 128, 255};
inline const Color kWarning{235, 170, 60, 255};
inline const Color kEye{18, 26, 22, 255};
inline const Color kText{235, 238, 245, 255};
inline const Color kBoostOn{120, 230, 140, 255};
inline const Color kSpecial{190, 120, 255, 255};
inline const Color kSpecialHalo{255, 214, 90, 255};
inline const Color kGood{120, 230, 140, 255};
inline const Color kBad{240, 110, 90, 255};
inline const Color kGold{255, 204, 50, 255};
inline const Color kCyan{80, 205, 240, 255};
inline const Color kCardBg{22, 25, 36, 240};
inline const Color kCardBorder{55, 62, 82, 255};
inline const Color kFoodLeaf{80, 215, 95, 255};
inline const Color kFoodStem{120, 75, 45, 255};

} // namespace snake
