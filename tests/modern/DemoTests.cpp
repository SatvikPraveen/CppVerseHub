#include <catch2/catch_test_macros.hpp>

#include <sstream>
#include <string>

#include "modern/Demo.hpp"

TEST_CASE("runDemo runs every showcase without throwing", "[modern][demo]") {
    std::ostringstream os;
    REQUIRE_NOTHROW(CppVerseHub::Modern::runDemo(os));
    const auto text = os.str();
    CHECK_FALSE(text.empty());
    for (const char* section : {"=== C++20 Concepts ===", "=== Compile-time Programming", "=== Lambda Expressions ===",
                                "=== Move Semantics", "=== C++20 Ranges ===", "=== Structured Bindings ===",
                                "=== C++20 Modules (emulated", "showcase complete"}) {
        CHECK(text.find(section) != std::string::npos);
    }
    CHECK(text.find("aborted") == std::string::npos);
}

TEST_CASE("runDemo is deterministic apart from its constinit counter", "[modern][demo]") {
    std::ostringstream a;
    std::ostringstream b;
    CppVerseHub::Modern::runDemo(a);
    CppVerseHub::Modern::runDemo(b);
    auto strip = [](std::string s) {
        const auto pos = s.find("invocations (constinit counter)");
        return pos == std::string::npos ? s : s.erase(pos, s.find('\n', pos) - pos);
    };
    CHECK(strip(a.str()) == strip(b.str()));
}
