#version 330

// Phase H3: one post-process pass over the virtual 960x540 canvas.
// FXAA (same maths as fxaa.fs), single-pass bloom, vignette, a light chromatic
// aberration driven by the boost amount, and a thin flash tint for big events.
// Every stage can be switched off through its uniform.

in vec2 fragTexCoord;
in vec4 fragColor;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

uniform vec2 resolution;
uniform float fxaaOn;
uniform float bloomStrength;   // 0 = off, ~0.9 low, ~1.8 high
uniform float vignetteStrength; // 0 = off
uniform float boost;           // 0..1 smoothed boost amount
uniform float flash;           // 0..1 decaying event flash
uniform float time;            // seconds (reserved for future animated effects)

float luma(vec3 c) { return dot(c, vec3(0.299, 0.587, 0.114)); }

vec3 fxaaSample(vec2 uv, vec2 px) {
    vec3 rgbM  = texture(texture0, uv).rgb;
    vec3 rgbNW = texture(texture0, uv + vec2(-1.0, -1.0) * px).rgb;
    vec3 rgbNE = texture(texture0, uv + vec2( 1.0, -1.0) * px).rgb;
    vec3 rgbSW = texture(texture0, uv + vec2(-1.0,  1.0) * px).rgb;
    vec3 rgbSE = texture(texture0, uv + vec2( 1.0,  1.0) * px).rgb;
    float lM = luma(rgbM), lNW = luma(rgbNW), lNE = luma(rgbNE);
    float lSW = luma(rgbSW), lSE = luma(rgbSE);
    float lMin = min(lM, min(min(lNW, lNE), min(lSW, lSE)));
    float lMax = max(lM, max(max(lNW, lNE), max(lSW, lSE)));
    vec2 dir;
    dir.x = -((lNW + lNE) - (lSW + lSE));
    dir.y =  ((lNW + lSW) - (lNE + lSE));
    float dirReduce = max((lNW + lNE + lSW + lSE) * 0.25 * (1.0 / 8.0), 1.0 / 128.0);
    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
    dir = clamp(dir * rcpDirMin, vec2(-8.0), vec2(8.0)) * px;
    vec3 rgbA = 0.5 * (texture(texture0, uv + dir * (1.0 / 3.0 - 0.5)).rgb +
                       texture(texture0, uv + dir * (2.0 / 3.0 - 0.5)).rgb);
    vec3 rgbB = rgbA * 0.5 + 0.25 * (texture(texture0, uv + dir * -0.5).rgb +
                                     texture(texture0, uv + dir * 0.5).rgb);
    float lB = luma(rgbB);
    return (lB < lMin || lB > lMax) ? rgbA : rgbB;
}

// 12 taps on a circle of about 3 pixels, only the bright part of the image.
vec3 bloomSample(vec2 uv, vec2 px) {
    vec3 sum = vec3(0.0);
    const int TAPS = 12;
    for (int i = 0; i < TAPS; ++i) {
        float a = (float(i) / float(TAPS)) * 6.2831853;
        vec2 off = vec2(cos(a), sin(a)) * px * 3.0;
        vec3 c = texture(texture0, uv + off).rgb;
        sum += max(c - vec3(0.65), vec3(0.0));
    }
    return sum / float(TAPS);
}

void main() {
    vec2 px = 1.0 / resolution;
    vec3 col = (fxaaOn > 0.5) ? fxaaSample(fragTexCoord, px)
                              : texture(texture0, fragTexCoord).rgb;

    if (bloomStrength > 0.0) {
        col += bloomSample(fragTexCoord, px) * bloomStrength;
    }

    // Chromatic aberration along the radial direction, only while boosting.
    if (boost > 0.001) {
        vec2 dir = fragTexCoord - vec2(0.5);
        float len = length(dir);
        vec2 radial = (len > 0.0001) ? dir / len : vec2(0.0);
        vec2 shift = radial * px * (boost * 2.0);
        float r = texture(texture0, fragTexCoord + shift).r;
        float b = texture(texture0, fragTexCoord - shift).b;
        col.r = mix(col.r, r, clamp(boost, 0.0, 1.0));
        col.b = mix(col.b, b, clamp(boost, 0.0, 1.0));
    }

    float d = length((fragTexCoord - vec2(0.5)) * vec2(resolution.x / resolution.y, 1.0));
    col *= 1.0 - vignetteStrength * smoothstep(0.45, 1.0, d);

    col = mix(col, vec3(1.0, 0.88, 0.88), clamp(flash * 0.25, 0.0, 1.0));

    finalColor = vec4(col, 1.0) * colDiffuse * fragColor;
}
