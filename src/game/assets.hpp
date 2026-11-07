#pragma once

#include "raylib.h"

#include <string>

namespace snake {

// Assets are looked up relative to the working directory first, then next to the
// executable, so the game runs from any cwd. Empty string == not found.
// (One shared lookup for audio, the FXAA shader and the UI font.)
inline std::string resolveAssetPath(const char* relative) {
    if (FileExists(relative)) {
        return relative;
    }
    const char* appDir = GetApplicationDirectory();
    if (appDir != nullptr) {
        const std::string candidate = std::string(appDir) + relative;
        if (FileExists(candidate.c_str())) {
            return candidate;
        }
    }
    return {};
}

} // namespace snake
