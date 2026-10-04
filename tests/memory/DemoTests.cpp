/**
 * @file DemoTests.cpp
 * @brief Smoke test for the module's runDemo entry point.
 */

#include "memory/Demo.hpp"

#include <catch2/catch_test_macros.hpp>

#include <sstream>
#include <string>

TEST_CASE("Memory runDemo runs every showcase without throwing", "[memory][demo]") {
    std::ostringstream out;
    REQUIRE_NOTHROW(CppVerseHub::Memory::runDemo(out));
    const std::string text = out.str();
    CHECK_FALSE(text.empty());
    CHECK(text.find("=== Custom Allocators ===") != std::string::npos);
    CHECK(text.find("=== Memory Pools ===") != std::string::npos);
    CHECK(text.find("=== RAII ===") != std::string::npos);
    CHECK(text.find("=== Smart Pointers ===") != std::string::npos);
    CHECK(text.find("[memory demo error]") == std::string::npos);
}
