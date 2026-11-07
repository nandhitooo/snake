#include "ui/text.hpp"

#include "game/assets.hpp"

#include <cmath>
#include <string>

namespace snake {
namespace {

constexpr int kFontLoadSize = 64;      // rasterised size; scaled down when drawing
constexpr int kFirstCodepoint = 32;    // ASCII 32..126
constexpr int kCodepointCount = 95;
constexpr float kSpacingFactor = 0.02f;
constexpr float kBaselineShift = 0.06f; // lifts Poppins so it sits where the old font did

Font gFont{};
bool gLoaded = false;
bool gCustom = false;

void ensureFont() {
    if (gLoaded) {
        return;
    }
    gLoaded = true;
    const std::string path = resolveAssetPath("assets/fonts/Poppins-SemiBold.ttf");
    if (!path.empty()) {
        int codepoints[kCodepointCount];
        for (int i = 0; i < kCodepointCount; ++i) {
            codepoints[i] = kFirstCodepoint + i;
        }
        Font f = LoadFontEx(path.c_str(), kFontLoadSize, codepoints, kCodepointCount);
        if (f.texture.id != 0 && f.glyphCount > 0) {
            GenTextureMipmaps(&f.texture);
            SetTextureFilter(f.texture, TEXTURE_FILTER_TRILINEAR);
            gFont = f;
            gCustom = true;
            return;
        }
        if (f.texture.id != 0) {
            UnloadFont(f);
        }
    }
    gFont = GetFontDefault();
}

} // namespace

void uiTextShutdown() {
    if (gLoaded && gCustom) {
        UnloadFont(gFont);
    }
    gLoaded = false;
    gCustom = false;
}

void uiText(const char* text, float x, float y, float size, Color color) {
    ensureFont();
    const float shift = gCustom ? -size * kBaselineShift : 0.0f;
    DrawTextEx(gFont, text, Vector2{x, y + shift}, size, size * kSpacingFactor, color);
}

float uiMeasure(const char* text, float size) {
    ensureFont();
    return MeasureTextEx(gFont, text, size, size * kSpacingFactor).x;
}

float uiTextFit(const char* text, float maxWidth, float size) {
    const float w = uiMeasure(text, size);
    if (w <= maxWidth || w <= 0.0f || maxWidth <= 0.0f) {
        return size;
    }
    return size * (maxWidth / w); // width is linear in size
}

void drawStar(float cx, float cy, float r, Color color) {
    constexpr float kPi = 3.14159265f;
    Vector2 outer[5];
    Vector2 inner[5];
    for (int i = 0; i < 5; ++i) {
        const float a = -kPi * 0.5f + static_cast<float>(i) * 2.0f * kPi / 5.0f;
        const float b = a + kPi / 5.0f;
        outer[i] = Vector2{cx + std::cos(a) * r, cy + std::sin(a) * r};
        inner[i] = Vector2{cx + std::cos(b) * r * 0.42f, cy + std::sin(b) * r * 0.42f};
    }
    const Vector2 c{cx, cy};
    // DrawTriangle wants counter-clockwise points; in screen space that is the
    // reverse of the order we generate them in.
    for (int i = 0; i < 5; ++i) {
        const int n = (i + 1) % 5;
        DrawTriangle(c, inner[i], outer[i], color);
        DrawTriangle(c, outer[n], inner[i], color);
    }
}

} // namespace snake
