#version 330
in vec2 fragTexCoord;
in vec4 fragColor;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec2 resolution;
out vec4 finalColor;

float luma(vec3 c) { return dot(c, vec3(0.299, 0.587, 0.114)); }

void main() {
    vec2 px = 1.0 / resolution;
    vec3 rgbM  = texture(texture0, fragTexCoord).rgb;
    vec3 rgbNW = texture(texture0, fragTexCoord + vec2(-1.0, -1.0) * px).rgb;
    vec3 rgbNE = texture(texture0, fragTexCoord + vec2( 1.0, -1.0) * px).rgb;
    vec3 rgbSW = texture(texture0, fragTexCoord + vec2(-1.0,  1.0) * px).rgb;
    vec3 rgbSE = texture(texture0, fragTexCoord + vec2( 1.0,  1.0) * px).rgb;
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
    vec3 rgbA = 0.5 * (texture(texture0, fragTexCoord + dir * (1.0 / 3.0 - 0.5)).rgb +
                       texture(texture0, fragTexCoord + dir * (2.0 / 3.0 - 0.5)).rgb);
    vec3 rgbB = rgbA * 0.5 + 0.25 * (texture(texture0, fragTexCoord + dir * -0.5).rgb +
                                     texture(texture0, fragTexCoord + dir * 0.5).rgb);
    float lB = luma(rgbB);
    vec3 outc = (lB < lMin || lB > lMax) ? rgbA : rgbB;
    finalColor = vec4(outc, 1.0) * colDiffuse * fragColor;
}
