// Standalone tests for the persistent high-score board (Phase G2).
// No raylib, no game code: build and run with
//   g++ -std=c++17 -Isrc tests/highscore_test.cpp src/game/highscore.cpp
//       -o build/highscore_test && ./build/highscore_test
// Never part of the game build.

#ifdef NDEBUG
#undef NDEBUG // assertions must stay live even in a release-style build
#endif
#include <cassert>

#include "game/highscore.hpp"

#include <cstdio>
#include <string>

using namespace snake;

namespace {

ScoreEntry makeEntry(const char* name, int score, int length = 5, int seconds = 30) {
    ScoreEntry e;
    e.name = name;
    e.score = score;
    e.length = length;
    e.seconds = seconds;
    e.date = "2026-11-08";
    return e;
}

void testEmptyBoard() {
    HighScoreBoard b;
    assert(b.size() == 0);
    assert(b.empty());
    assert(b.best() == 0);
    assert(b.qualifies(1));
    assert(b.qualifies(10));
    assert(!b.qualifies(0));
    assert(!b.qualifies(-5));
    assert(b.rankFor(10) == 1);
}

void testOrderAndRanks() {
    HighScoreBoard b;
    assert(b.insert(makeEntry("AAA", 10)) == 1);
    assert(b.insert(makeEntry("BBB", 30)) == 1);
    assert(b.insert(makeEntry("CCC", 20)) == 2);
    assert(b.size() == 3);
    assert(b.best() == 30);
    assert(b.at(0).name == "BBB");
    assert(b.at(1).name == "CCC");
    assert(b.at(2).name == "AAA");
}

void testLimit() {
    HighScoreBoard b;
    for (int i = 1; i <= 15; ++i) {
        b.insert(makeEntry("P", i));
    }
    assert(b.size() == kMaxHighScores);
    assert(b.best() == 15);
    assert(b.at(kMaxHighScores - 1).score == 6);
    assert(!b.qualifies(5));
    assert(!b.qualifies(6)); // must beat the lowest entry, not match it
    assert(b.qualifies(7));
    assert(b.rankFor(5) == 0);
    assert(b.rankFor(7) == kMaxHighScores);
}

void testTieKeepsOlderAbove() {
    HighScoreBoard b;
    assert(b.insert(makeEntry("OLD", 50)) == 1);
    assert(b.insert(makeEntry("NEW", 50)) == 2);
    assert(b.at(0).name == "OLD");
    assert(b.at(1).name == "NEW");
    assert(b.rankFor(50) == 3);
}

void testNameSanitize() {
    assert(sanitizePlayerName("bob") == "BOB");
    assert(sanitizePlayerName("a-b_c9") == "A-B_C9");
    assert(sanitizePlayerName("") == "PLAYER");
    assert(sanitizePlayerName("!!|@#") == "PLAYER");
    assert(sanitizePlayerName("abcdefghijklm") == "ABCDEFGH"); // max 8
    assert(sanitizePlayerName("BO|B") == "BOB");               // '|' cannot survive
    assert(sanitizePlayerName("  z  ") == "Z");
}

void testClamp() {
    HighScoreBoard b;
    const int rank = b.insert(makeEntry("BIG", 100000000, -5, 100000000));
    assert(rank == 1);
    assert(b.at(0).score == kMaxScoreValue);
    assert(b.at(0).length == 0);
    assert(b.at(0).seconds == kMaxScoreSeconds);
}

void testParseTolerant() {
    HighScoreBoard b;
    const std::string text = "# snake high scores\n"
                             "version=1\n"
                             "entry=BOB|120|7|42|2026-11-08\n"
                             "this line is garbage\n"
                             "entry=BAD|notanumber|3|4|2026-11-08\n"
                             "entry=EVIL|HACK|10|1|2|3\n"
                             "entry=bob|50|3|10|2026-11-01\n";
    b.parse(text);
    assert(b.size() == 2);
    assert(b.best() == 120);
    assert(b.at(0).name == "BOB");
    assert(b.at(0).score == 120);
    assert(b.at(0).length == 7);
    assert(b.at(0).seconds == 42);
    assert(b.at(0).date == "2026-11-08");
    assert(b.at(1).name == "BOB"); // lowercase normalised
    assert(b.at(1).score == 50);
}

void testParseTrimsAndReorders() {
    HighScoreBoard b;
    std::string text = "entry=  low  |5|1|1|2026-01-01\n";
    for (int i = 200; i >= 100; i -= 10) {
        text += "entry=P|" + std::to_string(i) + "|1|1|2026-01-01\n";
    }
    b.parse(text);
    assert(b.size() == kMaxHighScores); // 11 lines -> truncated to 10
    assert(b.best() == 200);
    assert(b.at(0).score == 200);
    assert(b.at(9).score == 110);
}

void testRoundTrip() {
    HighScoreBoard a;
    a.insert(makeEntry("AAA", 10, 3, 5));
    a.insert(makeEntry("ZZZ", 900, 12, 61));
    a.insert(makeEntry("MMM", 500, 8, 3600));

    HighScoreBoard b;
    b.parse(a.serialize());
    assert(b.size() == a.size());
    for (int i = 0; i < a.size(); ++i) {
        assert(b.at(i).name == a.at(i).name);
        assert(b.at(i).score == a.at(i).score);
        assert(b.at(i).length == a.at(i).length);
        assert(b.at(i).seconds == a.at(i).seconds);
        assert(b.at(i).date == a.at(i).date);
    }
}

void testFileRoundTrip() {
    const std::string path = "highscore_test_tmp.cfg";
    std::remove(path.c_str());

    HighScoreBoard empty;
    assert(!empty.load(path)); // missing file: empty board, no crash
    assert(empty.size() == 0);

    HighScoreBoard a;
    a.insert(makeEntry("FILE", 77, 9, 12));
    assert(a.save(path));

    HighScoreBoard b;
    assert(b.load(path));
    assert(b.size() == 1);
    assert(b.best() == 77);
    assert(b.at(0).name == "FILE");
    assert(b.at(0).length == 9);

    // A corrupt file must degrade to "whatever still parses", never crash.
    {
        std::FILE* f = std::fopen(path.c_str(), "wb");
        assert(f != nullptr);
        std::fputs("entry=|garbage\nentry=OK|5|1|1|2026-02-02\n", f);
        std::fclose(f);
    }
    HighScoreBoard c;
    assert(c.load(path));
    assert(c.size() == 1);
    assert(c.at(0).name == "OK");

    std::remove(path.c_str());
    std::remove((path + ".tmp").c_str());
}

void testClear() {
    HighScoreBoard b;
    b.insert(makeEntry("A", 10));
    b.clear();
    assert(b.size() == 0);
    assert(b.best() == 0);
}

void testTimeFormat() {
    assert(formatScoreTime(0) == "00:00");
    assert(formatScoreTime(61) == "01:01");
    assert(formatScoreTime(3600) == "60:00");
    assert(formatScoreTime(-4) == "00:00");
}

} // namespace

int main() {
    testEmptyBoard();
    testOrderAndRanks();
    testLimit();
    testTieKeepsOlderAbove();
    testNameSanitize();
    testClamp();
    testParseTolerant();
    testParseTrimsAndReorders();
    testRoundTrip();
    testFileRoundTrip();
    testClear();
    testTimeFormat();
    std::puts("highscore tests passed");
    return 0;
}
