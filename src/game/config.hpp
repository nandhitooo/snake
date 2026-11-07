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

// Camera rig tuning (Phase A). All rates are per second.
inline constexpr float kCamMaxDt = 0.05f;        // clamp dt so a stall cannot teleport the rig
inline constexpr float kCamFollowSmoothing = 7.0f; // exp smoothing of orbit camera positions
inline constexpr float kCamFocusShare = 0.45f;   // iso/top keep only part of the head offset
inline constexpr float kCamFocusSmoothing = 7.0f;
inline constexpr float kCamMaxSpeed = 16.0f;     // units/s ceiling for the focus (wrap glide);
                                                // above the boosted head speed (~12.5 u/s)
inline constexpr float kCamLookAhead = 1.2f;     // cells of forward bias (iso/top)
inline constexpr float kCamYawSmoothing = 6.0f;  // heading smoothing for the third-person orbit
inline constexpr float kCamYawMaxSpeed = 7.0f;   // rad/s angular ceiling
inline constexpr float kCamThirdRadius = 7.5f;
inline constexpr float kCamThirdHeight = 5.2f;
inline constexpr float kCamThirdMinHeight = 1.5f;
inline constexpr float kCamThirdLookAhead = 2.5f;
inline constexpr float kCamThirdLookY = 0.7f;
inline constexpr float kCamIsoHeight = 22.0f;
inline constexpr float kCamIsoBack = 17.0f;
inline constexpr float kCamTopHeight = 25.0f;
inline constexpr float kCamFovIso = 45.0f;
inline constexpr float kCamFovThird = 55.0f;
inline constexpr float kCamFovSmoothing = 9.0f;
inline constexpr float kCamModeBlend = 0.5f;     // cross-fade time when the mode changes

// Eating / mouth animation (Phase B)
inline constexpr int kMouthScanCells = 3;        // how far ahead the snake looks for food
inline constexpr float kMouthOpenDistance = 0.8f; // mouth is fully open this many cells away
inline constexpr float kMouthCloseDistance = 3.0f; // mouth starts opening at this distance
inline constexpr float kMouthOpenSpeed = 14.0f;  // opening rate (0..1 per second)
inline constexpr float kMouthCloseSpeed = 30.0f; // closing rate (0..1 per second)
inline constexpr float kMouthCloseBiteTime = 0.12f; // fast snap shut after a bite
inline constexpr float kMouthLockTime = 0.2f;    // pause before the mouth may open again
inline constexpr float kMouthBiteIdle = -1.0f;
inline constexpr float kMouthMaxAngle = 0.8726646f; // 50 degrees for the lower jaw
inline constexpr float kMouthSnoutLift = 0.1396263f; // 8 degrees for the snout
inline constexpr float kMouthJawLength = 0.55f;
inline constexpr float kMouthHingeBack = 0.05f;
inline constexpr float kMouthHingeDown = 0.22f;
inline constexpr float kMouthLunge = 0.1f;       // head thrust on the bite
inline constexpr float kMouthBitePulse = 0.18f;  // seconds of the bite head pulse
inline constexpr Color kMouthInner{110, 32, 38, 255};
inline constexpr Color kMouthTeeth{246, 248, 252, 255};

// Snake head design (Phase C). Mouth/jaw offsets are authored at kHeadRefRadius
// and scaled from there, so changing the skull size keeps the face attached.
inline constexpr float kHeadRadius = 0.62f;
inline constexpr float kHeadRefRadius = 0.5f;
inline constexpr float kHeadEyeFwd = 0.4f;      // eye centre, forward offset
inline constexpr float kHeadEyeSpread = 0.26f;  // eye centre, lateral offset
inline constexpr float kHeadEyeHeight = 0.21f;  // eye centre, height offset
inline constexpr float kHeadEyeWhite = 0.17f;
inline constexpr float kHeadEyePupil = 0.088f;
inline constexpr float kHeadEyePupilFwd = 0.078f;
inline constexpr float kHeadEyeShine = 0.032f;  // highlight radius (0 disables)
inline constexpr float kHeadEyeShineOffset = 0.045f;
inline constexpr float kHeadEyePupilGrow = 0.25f; // pupil growth per unit of mouthOpen
inline constexpr float kHeadPupilShift = 0.04f;   // max gaze offset
inline constexpr float kHeadBlinkLine = 0.11f;    // half length of the closed-eye line
inline constexpr float kHeadBlinkThick = 0.018f;
inline constexpr float kHeadEyeHappyThick = 0.02f;
inline constexpr float kHeadBitePulseScale = 0.08f;

// Body: chibi proportions (round segments, tapering toward the tail).
inline constexpr float kHeadBodyNear = 0.42f;  // first body segment
inline constexpr float kHeadBodyFar = 0.28f;   // last regular segment
inline constexpr float kHeadTailRadius = 0.18f;
inline constexpr float kHeadWaveAmp = 0.07f;   // body wave height
inline constexpr float kHeadTailWag = 0.08f;   // lateral tip sway
inline constexpr float kHeadCubeTint = 0.84f;  // alternating band for Cube skins

// Blinking / expressions (Phase C3).
inline constexpr float kBlinkDuration = 0.14f;   // whole blink, seconds
inline constexpr float kBlinkCloseTime = 0.05f;  // closing is faster than opening
inline constexpr float kBlinkMinGap = 2.0f;      // shortest idle gap between blinks
inline constexpr float kBlinkMaxGap = 5.0f;      // longest idle gap between blinks
inline constexpr float kBlinkDoubleChance = 0.2f;
inline constexpr float kBlinkDoubleGap = 0.08f;  // pause inside a double blink
inline constexpr float kHappyTime = 0.5f;        // "^ ^" eyes after a bite
inline constexpr float kAiBlinkPeriod = 4.0f;    // rival blink period (phase-shifted)

// Face details.
inline constexpr float kHeadBlushFwd = 0.36f;
inline constexpr float kHeadBlushSpread = 0.4f;
inline constexpr float kHeadBlushHeight = -0.17f;
inline constexpr float kHeadBlushSize = 0.105f;
inline constexpr float kHeadBlushFlat = 0.55f;    // vertical squash
inline constexpr float kHeadBlushMix = 0.72f;     // blend toward the pink
inline constexpr unsigned char kHeadBlushAlpha = 170;
inline constexpr Color kHeadBlush{255, 140, 165, 255};
inline constexpr float kHeadSmileFwd = 0.9f;
inline constexpr float kHeadSmileDown = 0.55f;
inline constexpr float kHeadSmileHalfWidth = 0.5f; // radians
inline constexpr float kHeadSmileDroop = 0.28f;
inline constexpr float kHeadSmileThick = 0.016f;
inline constexpr Color kHeadSmile{42, 32, 38, 255};
inline constexpr float kHeadTongueFwd = 0.55f;
inline constexpr float kHeadTongueDrop = 0.14f;

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
