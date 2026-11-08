#include "game/highscore.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <sstream>

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

// Strict integer parse: the whole (trimmed) field must be a number.
int readInt(const std::string& value, bool& ok) {
    try {
        size_t used = 0;
        const int parsed = std::stoi(value, &used);
        ok = used > 0 && used == value.size();
        return parsed;
    } catch (...) {
        ok = false;
        return 0;
    }
}

std::vector<std::string> split(const std::string& text, char sep) {
    std::vector<std::string> parts;
    size_t start = 0;
    while (true) {
        const size_t at = text.find(sep, start);
        if (at == std::string::npos) {
            parts.push_back(text.substr(start));
            break;
        }
        parts.push_back(text.substr(start, at - start));
        start = at + 1;
    }
    return parts;
}

} // namespace

std::string sanitizePlayerName(const std::string& raw) {
    std::string out;
    out.reserve(static_cast<size_t>(kMaxPlayerName));
    for (const char ch : raw) {
        if (out.size() >= static_cast<size_t>(kMaxPlayerName)) {
            break;
        }
        const unsigned char c = static_cast<unsigned char>(ch);
        if (c >= 'a' && c <= 'z') {
            out.push_back(static_cast<char>(c - 'a' + 'A'));
        } else if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-') {
            out.push_back(static_cast<char>(c));
        }
        // Everything else (including '|') is dropped, so a name can never forge a field.
    }
    if (out.empty()) {
        out = "PLAYER";
    }
    return out;
}

std::string todayDate() {
    const std::time_t now = std::time(nullptr);
    std::tm local{};
#if defined(_WIN32)
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif
    char buf[16] = {0};
    if (std::strftime(buf, sizeof(buf), "%Y-%m-%d", &local) == 0) {
        return "1970-01-01";
    }
    return std::string(buf);
}

std::string formatScoreTime(int seconds) {
    const int s = std::max(0, seconds);
    char buf[32] = {0};
    std::snprintf(buf, sizeof(buf), "%02d:%02d", s / 60, s % 60);
    return std::string(buf);
}

bool HighScoreBoard::qualifies(int score) const {
    if (score <= 0) {
        return false;
    }
    if (static_cast<int>(entries_.size()) < kMaxHighScores) {
        return true;
    }
    return score > entries_.back().score;
}

int HighScoreBoard::rankFor(int score) const {
    if (!qualifies(score)) {
        return 0;
    }
    // Ties are inserted *after* the existing entries with the same score.
    size_t pos = 0;
    while (pos < entries_.size() && entries_[pos].score >= score) {
        ++pos;
    }
    return static_cast<int>(pos) + 1;
}

int HighScoreBoard::insert(ScoreEntry entry) {
    entry.name = sanitizePlayerName(entry.name);
    entry.score = clampInt(entry.score, 0, kMaxScoreValue);
    entry.length = clampInt(entry.length, 0, kMaxScoreLength);
    entry.seconds = clampInt(entry.seconds, 0, kMaxScoreSeconds);
    if (!qualifies(entry.score)) {
        return 0;
    }
    size_t pos = 0;
    while (pos < entries_.size() && entries_[pos].score >= entry.score) {
        ++pos;
    }
    entries_.insert(entries_.begin() + static_cast<std::ptrdiff_t>(pos), entry);
    if (static_cast<int>(entries_.size()) > kMaxHighScores) {
        entries_.pop_back();
    }
    return (pos < static_cast<size_t>(kMaxHighScores)) ? static_cast<int>(pos) + 1 : 0;
}

int HighScoreBoard::best() const { return entries_.empty() ? 0 : entries_.front().score; }

void HighScoreBoard::clear() { entries_.clear(); }

void HighScoreBoard::parse(const std::string& text) {
    entries_.clear();
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line)) {
        const std::string t = trim(line);
        if (t.empty() || t[0] == '#') {
            continue;
        }
        if (t.rfind("version=", 0) == 0) {
            continue;
        }
        if (t.rfind("entry=", 0) != 0) {
            continue; // unknown line: ignore it, never abort the whole file
        }
        const std::vector<std::string> fields = split(t.substr(6), '|');
        if (fields.size() != 5) {
            continue; // a '|' in the name would inflate the field count: reject the line
        }
        bool scoreOk = false;
        bool lengthOk = false;
        bool secondsOk = false;
        ScoreEntry e;
        e.score = readInt(trim(fields[1]), scoreOk);
        e.length = readInt(trim(fields[2]), lengthOk);
        e.seconds = readInt(trim(fields[3]), secondsOk);
        if (!scoreOk || !lengthOk || !secondsOk) {
            continue;
        }
        e.name = sanitizePlayerName(trim(fields[0]));
        e.score = clampInt(e.score, 0, kMaxScoreValue);
        e.length = clampInt(e.length, 0, kMaxScoreLength);
        e.seconds = clampInt(e.seconds, 0, kMaxScoreSeconds);
        std::string date = trim(fields[4]);
        if (date.size() > 10) {
            date.resize(10);
        }
        e.date = date;
        entries_.push_back(e);
    }
    // stable: entries already in the file keep their relative order on a tie.
    std::stable_sort(entries_.begin(), entries_.end(),
                     [](const ScoreEntry& a, const ScoreEntry& b) { return a.score > b.score; });
    if (static_cast<int>(entries_.size()) > kMaxHighScores) {
        entries_.resize(static_cast<size_t>(kMaxHighScores));
    }
}

std::string HighScoreBoard::serialize() const {
    std::string out = "# snake high scores\nversion=1\n";
    for (const ScoreEntry& e : entries_) {
        out += "entry=" + e.name + "|" + std::to_string(e.score) + "|" + std::to_string(e.length) +
               "|" + std::to_string(e.seconds) + "|" + e.date + "\n";
    }
    return out;
}

bool HighScoreBoard::load(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        entries_.clear();
        return false; // no file yet: an empty table, not an error
    }
    std::stringstream buffer;
    buffer << in.rdbuf();
    parse(buffer.str());
    return true;
}

bool HighScoreBoard::save(const std::string& path) const {
    const std::string temp = path + ".tmp";
    {
        std::ofstream out(temp, std::ios::trunc);
        if (!out) {
            return false; // read-only directory: keep playing with the in-memory board
        }
        out << serialize();
        if (!out) {
            return false;
        }
    }
    // Atomic replace: a crash mid-write can never leave a half-written table.
    return std::rename(temp.c_str(), path.c_str()) == 0;
}

} // namespace snake
