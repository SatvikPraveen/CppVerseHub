// Tests for CppVerseHub::Utils::runDemo.
#include "utils/Demo.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <sstream>
#include <string>

TEST_CASE("utils runDemo produces output from every showcase without throwing", "[utils][demo]") {
    std::ostringstream oss;
    REQUIRE_NOTHROW(CppVerseHub::Utils::runDemo(oss));
    const std::string output = oss.str();
    CHECK_FALSE(output.empty());
    for (const char* header :
         {"=== Logging ===", "=== Configuration ===", "=== Parsers ===", "=== Math utilities ===",
          "=== String utilities ===", "=== Time utilities ==="}) {
        CHECK(output.find(header) != std::string::npos);
    }
    CHECK(output.find("[utils demo '") == std::string::npos); // no showcase reported a failure
}

TEST_CASE("utils runDemo is deterministic apart from timing-dependent lines", "[utils][demo]") {
    std::ostringstream a;
    std::ostringstream b;
    CppVerseHub::Utils::runDemo(a);
    CppVerseHub::Utils::runDemo(b);
    // Same number of lines on every run.
    const auto lines = [](const std::string& s) {
        return static_cast<std::size_t>(std::count(s.begin(), s.end(), '\n'));
    };
    CHECK(lines(a.str()) == lines(b.str()));
}
