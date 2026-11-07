#pragma once

#include "raylib.h"

namespace snake {

// Everything the shared snake-head renderer needs. Both the in-game snake and the
// skin-preview turntable build one of these, so the head is drawn in exactly one
// place (src/render/renderer.cpp: drawSnakeHead).
struct HeadPose {
    Vector3 pos{0.0f, 0.0f, 0.0f}; // head centre in world space
    Vector3 fwd{1.0f, 0.0f, 0.0f}; // unit forward direction (XZ plane)
    float scale = 1.0f;            // overall head scale (swallow bump)
    float time = 0.0f;             // animation clock (seconds)
    float blink = 0.0f;            // 0 = open, 1 = fully closed
    bool happy = false;            // "^ ^" eyes after a bite
    float mouthOpen = 0.0f;        // 0..1
    float biteT = -1.0f;           // seconds since the bite, <0 == idle
    bool animate = true;           // Settings -> Animations
    float gazeRight = 0.0f;        // -1..1 pupil offset along `right`
    float gazeFwd = 0.0f;          // -1..1 pupil offset along `fwd`
    Color head{130, 225, 140, 255};
    Color eye{18, 26, 22, 255};
    bool crown = false;
};

// Draws the whole head: skull, mouth/jaw, eyes, blush, smile, tongue and crown.
void drawSnakeHead(const HeadPose& pose);

} // namespace snake
