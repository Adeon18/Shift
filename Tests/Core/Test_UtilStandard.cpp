#include <doctest/doctest.h>

#include <cstdint>
#include <string_view>
#include <vector>

#include "Utility/UtilStandard.hpp"

using namespace Shift;

TEST_SUITE("UtilStandard") {

TEST_CASE("StrSplitView splits and preserves empty tokens") {
    std::vector<std::string_view> tokens;

    SUBCASE("plain split") {
        Util::StrSplitView("1.2.3", '.', &tokens);
        REQUIRE(tokens.size() == 3);
        CHECK(tokens[0] == "1");
        CHECK(tokens[1] == "2");
        CHECK(tokens[2] == "3");
    }
    SUBCASE("no delimiter yields the whole string") {
        Util::StrSplitView("abc", '.', &tokens);
        REQUIRE(tokens.size() == 1);
        CHECK(tokens[0] == "abc");
    }
    SUBCASE("adjacent delimiters yield empty tokens") {
        Util::StrSplitView("a..c", '.', &tokens);
        REQUIRE(tokens.size() == 3);
        CHECK(tokens[0] == "a");
        CHECK(tokens[1].empty());
        CHECK(tokens[2] == "c");
    }
    SUBCASE("empty input yields one empty token") {
        Util::StrSplitView("", '.', &tokens);
        REQUIRE(tokens.size() == 1);
        CHECK(tokens[0].empty());
    }
}

TEST_CASE("NormalizePath flips backslashes to forward slashes") {
    CHECK(Util::NormalizePath("A\\B\\c.txt") == "A/B/c.txt");
    CHECK(Util::NormalizePath("already/fine.txt") == "already/fine.txt");
}

TEST_CASE("StrToLower lowercases without touching non-letters") {
    CHECK(Util::StrToLower("AbC-12_x") == "abc-12_x");
}

TEST_CASE("StrToUint32Fast parses valid input and rejects garbage") {
    uint32_t value = 0;

    CHECK(Util::StrToUint32Fast("42", value));
    CHECK(value == 42);

    CHECK(Util::StrToUint32Fast("4294967295", value));
    CHECK(value == 4294967295u);

    CHECK_FALSE(Util::StrToUint32Fast("4294967296", value)); // one past uint32 max
    CHECK_FALSE(Util::StrToUint32Fast("abc", value));
    CHECK_FALSE(Util::StrToUint32Fast("", value));
    CHECK_FALSE(Util::StrToUint32Fast("-1", value));
}

TEST_CASE("ParseVersionTriple") {
    SUBCASE("strict happy path") {
        const auto version = Util::ParseVersionTriple("2.10.345");
        CHECK(version.uMajor == 2);
        CHECK(version.uMinor == 10);
        CHECK(version.uPatch == 345);
    }
    SUBCASE("wrong component count falls back to 1.0.0") {
        const auto version = Util::ParseVersionTriple("1.2");
        CHECK(version.uMajor == 1);
        CHECK(version.uMinor == 0);
        CHECK(version.uPatch == 0);
    }
    SUBCASE("non-numeric component falls back to 1.0.0") {
        const auto version = Util::ParseVersionTriple("a.b.c");
        CHECK(version.uMajor == 1);
        CHECK(version.uMinor == 0);
        CHECK(version.uPatch == 0);
    }
    SUBCASE("partially-numeric component is rejected, not truncated") {
        //! "9x" must NOT parse as 9 — full-token consumption is required
        const auto version = Util::ParseVersionTriple("9x.5.5");
        CHECK(version.uMajor == 1);
        CHECK(version.uMinor == 0);
        CHECK(version.uPatch == 0);
    }
    SUBCASE("empty component falls back to 1.0.0") {
        const auto version = Util::ParseVersionTriple("2..0");
        CHECK(version.uMajor == 1);
        CHECK(version.uMinor == 0);
        CHECK(version.uPatch == 0);
    }
}

TEST_CASE("GetDirectoryFromPath returns the parent with a trailing slash") {
    CHECK(Util::GetDirectoryFromPath("A/B/c.txt") == "A/B/");
}

}
