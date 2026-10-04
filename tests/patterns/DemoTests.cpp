/**
 * @file DemoTests.cpp
 * @brief Smoke test of the module demo and the individual showcases.
 */

#include "patterns/Adapter.hpp"
#include "patterns/Builder.hpp"
#include "patterns/Command.hpp"
#include "patterns/Decorator.hpp"
#include "patterns/Demo.hpp"
#include "patterns/Observer.hpp"
#include "patterns/Singleton.hpp"
#include "patterns/State.hpp"
#include "patterns/Strategy.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <sstream>

using namespace CppVerseHub::Patterns;
using Catch::Matchers::ContainsSubstring;

TEST_CASE("runDemo runs every showcase without throwing", "[patterns][demo]") {
    std::ostringstream oss;
    REQUIRE_NOTHROW(runDemo(oss));
    const std::string text = oss.str();
    CHECK_FALSE(text.empty());
    for (const char* header :
         {"Adapter", "Builder", "Command", "Decorator", "Observer", "Singleton", "State", "Strategy"}) {
        CHECK_THAT(text, ContainsSubstring(std::string("=== ") + header + " pattern ==="));
    }
    CHECK_THAT(text, !ContainsSubstring("showcase failed"));
}

TEST_CASE("Showcases report the expected outcomes", "[patterns][demo]") {
    std::ostringstream oss;
    demonstrateAdapter(oss);
    CHECK_THAT(oss.str(), ContainsSubstring("round-trip intact: true"));
    oss.str({});
    demonstrateCommand(oss);
    CHECK_THAT(oss.str(), ContainsSubstring("macro rejected"));
    oss.str({});
    demonstrateObserver(oss);
    CHECK_THAT(oss.str(), ContainsSubstring("live classic observers after logger expired: 2"));
    CHECK_THAT(oss.str(), ContainsSubstring("signal events seen while connected: 2"));
    oss.str({});
    demonstrateState(oss);
    CHECK_THAT(oss.str(), ContainsSubstring("final: Completed"));
    CHECK_THAT(oss.str(), ContainsSubstring("jumps: 1"));
    oss.str({});
    demonstrateDecorator(oss);
    CHECK_THAT(oss.str(), ContainsSubstring("in 21 calls"));
}
