#include "core/Demo.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <sstream>

TEST_CASE("Core runDemo runs end-to-end, deterministically, without throwing", "[core][demo]") {
    std::ostringstream first;
    REQUIRE_NOTHROW(CppVerseHub::Core::runDemo(first));
    const std::string text = first.str();
    REQUIRE_FALSE(text.empty());
    REQUIRE_THAT(text, Catch::Matchers::ContainsSubstring("core demo complete"));
    REQUIRE_THAT(text, Catch::Matchers::ContainsSubstring("same seed, same digest: true"));
    REQUIRE_THAT(text, Catch::Matchers::ContainsSubstring("restored run matches original: true"));
    REQUIRE_THAT(text, Catch::Matchers::ContainsSubstring("conserved true"));
    REQUIRE(text.find("aborted") == std::string::npos);
    std::ostringstream second;
    CppVerseHub::Core::runDemo(second);
    REQUIRE(second.str() == text);
}
