// Tests for templates/Demo.hpp
#include "templates/Demo.hpp"

#include <catch2/catch_test_macros.hpp>

#include <sstream>
#include <string>

TEST_CASE("runDemo runs every showcase without throwing", "[templates][demo]") {
    std::ostringstream out;
    REQUIRE_NOTHROW(CppVerseHub::Templates::runDemo(out));
    const std::string text = out.str();
    CHECK_FALSE(text.empty());
    CHECK(text.find("--- Concepts ---") != std::string::npos);
    CHECK(text.find("--- Generic containers ---") != std::string::npos);
    CHECK(text.find("--- Metaprogramming ---") != std::string::npos);
    CHECK(text.find("--- SFINAE ---") != std::string::npos);
    CHECK(text.find("--- Template specialization ---") != std::string::npos);
    CHECK(text.find("--- Variadic templates ---") != std::string::npos);
    CHECK(text.find("Templates demo complete") != std::string::npos);
    CHECK(text.find("failed") == std::string::npos);
}

TEST_CASE("runDemo output is deterministic", "[templates][demo]") {
    std::ostringstream first;
    std::ostringstream second;
    CppVerseHub::Templates::runDemo(first);
    CppVerseHub::Templates::runDemo(second);
    CHECK(first.str() == second.str());
}
