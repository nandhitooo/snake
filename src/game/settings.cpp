#include "game/settings.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <string>

namespace snake {
namespace {

int clampInt(int v, int lo, int hi) { return std::max(lo, std::min(hi, v)); }

std::string trim(const std::string& in) {
    size_t begin = 0;
    size_t end = in.size();
    while (begin < end && std::isspace(static_cast<unsigned char>(in[begin])) != 0) {
        ++begin;
    }
    while (end > begin && std::isspace(static_cast<unsigned char>(in[end - 1])) != 0) {
        --end;
    }
    return in.substr(begin, end - begin);
}

int readInt(const std::string& value, int fallback) {
    try {
        size_t used = 0;
        const int parsed = std::stoi(value, &used);
        if (used == 0) {
            return fallback;
        }
        return parsed;
    } catch (...) {
        return fallback; // malformed value: keep the default
    }
}

} // namespace

Settings loadSettings(const std::string& path) {
    Settings s; // defaults double as the fallback for missing keys
    std::ifstream in(path);
    if (!in) {
        return s;
    }

    std::string line;
    while (std::getline(in, line)) {
        const std::string text = trim(line);
        if (text.empty() || text[0] == '#') {
            continue;
        }
        const size_t eq = text.find('=');
        if (eq == std::string::npos) {
            continue;
        }
        const std::string key = trim(text.substr(0, eq));
        const std::string raw = trim(text.substr(eq + 1));

        // A malformed number keeps the field's current value (its default), so a
        // corrupt settings.cfg degrades to sane options instead of resetting to 0.
        if (key == "resolution") {
            s.resolution = clampInt(readInt(raw, s.resolution), 0, kResolutionCount - 1);
        } else if (key == "fullscreen") {
            s.fullscreen = clampInt(readInt(raw, s.fullscreen), 0, 1);
        } else if (key == "vsync") {
            s.vsync = clampInt(readInt(raw, s.vsync), 0, 1);
        } else if (key == "fps") {
            s.fpsLimit = clampInt(readInt(raw, s.fpsLimit), 0, 3);
        } else if (key == "speed") {
            s.speed = clampInt(readInt(raw, s.speed), 0, 2);
        } else if (key == "camera") {
            s.camera = clampInt(readInt(raw, s.camera), 0, 2);
        } else if (key == "animations") {
            s.animations = clampInt(readInt(raw, s.animations), 0, 1);
        } else if (key == "particles") {
            s.particles = clampInt(readInt(raw, s.particles), 0, 2);
        } else if (key == "shake") {
            s.shake = clampInt(readInt(raw, s.shake), 0, 1);
        } else if (key == "skin") {
            s.skin = clampInt(readInt(raw, s.skin), 0, 5);
        }
    }
    return s;
}

void saveSettings(const Settings& s, const std::string& path) {
    std::ofstream out(path, std::ios::trunc);
    if (!out) {
        return; // read-only working directory: keep playing with in-memory settings
    }
    out << "# snake settings (key=value)\n";
    out << "resolution=" << s.resolution << '\n';
    out << "fullscreen=" << s.fullscreen << '\n';
    out << "vsync=" << s.vsync << '\n';
    out << "fps=" << s.fpsLimit << '\n';
    out << "speed=" << s.speed << '\n';
    out << "camera=" << s.camera << '\n';
    out << "animations=" << s.animations << '\n';
    out << "particles=" << s.particles << '\n';
    out << "shake=" << s.shake << '\n';
    out << "skin=" << s.skin << '\n';
}

int fpsTarget(int index) {
    switch (clampInt(index, 0, 3)) {
    case 0:
        return 30;
    case 2:
        return 120;
    case 3:
        return 0; // unlimited
    case 1:
    default:
        return 60;
    }
}

float speedTickScale(int speed) {
    switch (clampInt(speed, 0, 2)) {
    case 0:
        return 1.25f; // slow: longer interval
    case 2:
        return 0.8f; // fast: shorter interval
    case 1:
    default:
        return 1.0f;
    }
}

float particleScale(int particles) {
    switch (clampInt(particles, 0, 2)) {
    case 0:
        return 0.45f;
    case 2:
        return 1.7f;
    case 1:
    default:
        return 1.0f;
    }
}

std::string settingLabel(const Settings& s, int row) {
    switch (row) {
    case RowResolution:
        return kResolutions[clampInt(s.resolution, 0, kResolutionCount - 1)].label;
    case RowFullscreen:
        return s.fullscreen != 0 ? "ON" : "OFF";
    case RowVSync:
        return s.vsync != 0 ? "ON" : "OFF";
    case RowFps:
        switch (clampInt(s.fpsLimit, 0, 3)) {
        case 0:
            return "30";
        case 2:
            return "120";
        case 3:
            return "UNLIMITED";
        case 1:
        default:
            return "60";
        }
    case RowSpeed:
        switch (clampInt(s.speed, 0, 2)) {
        case 0:
            return "SLOW";
        case 2:
            return "FAST";
        case 1:
        default:
            return "NORMAL";
        }
    case RowCamera:
        switch (clampInt(s.camera, 0, 2)) {
        case 1:
            return "TOP-DOWN";
        case 2:
            return "THIRD-PERSON";
        case 0:
        default:
            return "ISOMETRIC";
        }
    case RowAnimations:
        return s.animations != 0 ? "ON" : "OFF";
    case RowParticles:
        switch (clampInt(s.particles, 0, 2)) {
        case 0:
            return "LOW";
        case 2:
            return "HIGH";
        case 1:
        default:
            return "NORMAL";
        }
    case RowShake:
        return s.shake != 0 ? "ON" : "OFF";
    case RowReset:
        return "PRESS ENTER";
    default:
        return "";
    }
}

void adjustSetting(Settings& s, int row, int delta) {
    const int step = (delta >= 0) ? 1 : -1;
    switch (row) {
    case RowResolution:
        s.resolution = clampInt(s.resolution + step, 0, kResolutionCount - 1);
        break;
    case RowFullscreen:
        s.fullscreen = (s.fullscreen != 0) ? 0 : 1;
        break;
    case RowVSync:
        s.vsync = (s.vsync != 0) ? 0 : 1;
        break;
    case RowFps:
        s.fpsLimit = clampInt(s.fpsLimit + step, 0, 3);
        break;
    case RowSpeed:
        s.speed = clampInt(s.speed + step, 0, 2);
        break;
    case RowCamera:
        s.camera = clampInt(s.camera + step, 0, 2);
        break;
    case RowAnimations:
        s.animations = (s.animations != 0) ? 0 : 1;
        break;
    case RowParticles:
        s.particles = clampInt(s.particles + step, 0, 2);
        break;
    case RowShake:
        s.shake = (s.shake != 0) ? 0 : 1;
        break;
    case RowReset:
    default:
        break;
    }
}

} // namespace snake
