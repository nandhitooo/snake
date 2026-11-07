#pragma once

// Sharp UI text (Phase F3): one TTF font (Poppins SemiBold, SIL OFL 1.1) loaded once
// and drawn with DrawTextEx. Falls back to raylib's default font when the file is
// missing, so the game still runs without assets/fonts/.

#include "raylib.h"

namespace snake {

void uiTextShutdown();

void uiText(const char* text, float x, float y, float size, Color color);
float uiMeasure(const char* text, float size);
// Largest size <= `size` at which `text` fits in `maxWidth` pixels.
float uiTextFit(const char* text, float maxWidth, float size);
// The font has no "★": draw a small five-point star instead.
void drawStar(float cx, float cy, float r, Color color);

} // namespace snake
