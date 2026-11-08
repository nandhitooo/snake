#pragma once

// Persistent high-score table (Phase G).
//
// Deliberately raylib-free and config-free: the board only knows about text, so the
// same code can be unit-tested with a plain assert-based binary (tests/highscore_test.cpp).

#include <string>
#include <vector>

namespace snake {

// One finished round. `date` is "YYYY-MM-DD" in local time.
struct ScoreEntry {
    std::string name;
    int score = 0;
    int length = 0;
    int seconds = 0;
    std::string date;
};

inline constexpr int kMaxHighScores = 10;
inline constexpr int kMaxScoreValue = 9999999;
inline constexpr int kMaxScoreLength = 9999;
inline constexpr int kMaxScoreSeconds = 999999;
inline constexpr int kMaxPlayerName = 8;

// Uppercases A-Z/0-9/_/- only, drops everything else (including '|'), truncates to
// kMaxPlayerName and falls back to "PLAYER" when nothing survives.
std::string sanitizePlayerName(const std::string& raw);

// "YYYY-MM-DD" from the local clock.
std::string todayDate();

// mm:ss, clamped at 0.
std::string formatScoreTime(int seconds);

class HighScoreBoard {
public:
    // A score only qualifies when it beats the current lowest entry (strictly).
    bool qualifies(int score) const;

    // Inserts a copy, sanitized and clamped. Returns the 1-based rank, or 0 when
    // the score did not make the table. Ties keep the older entry above the new one.
    int insert(ScoreEntry entry);

    // Where `score` would land (1-based), or 0 when it would not qualify.
    int rankFor(int score) const;

    int best() const; // 0 when the table is empty
    void clear();

    int size() const { return static_cast<int>(entries_.size()); }
    const ScoreEntry& at(int index) const { return entries_[static_cast<size_t>(index)]; }
    const std::vector<ScoreEntry>& entries() const { return entries_; }
    bool empty() const { return entries_.empty(); }

    // Tolerant text form: "# snake high scores" / "version=1" / "entry=NAME|score|length|seconds|date".
    void parse(const std::string& text);
    std::string serialize() const;

    // Both silently keep the in-memory state usable when the file is broken or read-only.
    bool load(const std::string& path);
    bool save(const std::string& path) const;

private:
    std::vector<ScoreEntry> entries_; // sorted by score, descending
};

} // namespace snake
