// End-to-end tests for the stl_showcase demonstrations.
#include <catch2/catch_test_macros.hpp>

#include <sstream>
#include <string>

#include "stl_showcase/Algorithms.hpp"
#include "stl_showcase/Containers.hpp"
#include "stl_showcase/Demo.hpp"
#include "stl_showcase/Functors.hpp"
#include "stl_showcase/Iterators.hpp"
#include "stl_showcase/STLUtilities.hpp"

using namespace CppVerseHub::STL;

TEST_CASE("runDemo writes every section to the supplied stream", "[demo]") {
    std::ostringstream oss;
    REQUIRE_NOTHROW(runDemo(oss));
    const std::string text = oss.str();
    REQUIRE_FALSE(text.empty());
    for (const char* heading : {"Sequence Containers", "Container Adapters", "Sorting Algorithms", "Set Algorithms",
                                "Custom Iterators", "Lambda Expressions", "Function Binding", "std::variant",
                                "std::any", "STL Showcase complete"}) {
        CAPTURE(heading);
        REQUIRE(text.find(heading) != std::string::npos);
    }
    REQUIRE(text.find("aborted") == std::string::npos);
}

TEST_CASE("runDemo output is deterministic", "[demo]") {
    std::ostringstream first;
    std::ostringstream second;
    runDemo(first);
    runDemo(second);
    REQUIRE(first.str() == second.str());
}

TEST_CASE("Per-file demo entry points produce output", "[demo]") {
    std::ostringstream oss;
    runContainersDemo(oss);
    REQUIRE(oss.str().find("LRU cache") != std::string::npos);
    oss.str("");
    runAlgorithmsDemo(oss);
    REQUIRE(oss.str().find("next_permutation") != std::string::npos);
    oss.str("");
    runIteratorsDemo(oss);
    REQUIRE(oss.str().find("FilterView") != std::string::npos);
    oss.str("");
    runFunctorsDemo(oss);
    REQUIRE(oss.str().find("std::bind") != std::string::npos);
    oss.str("");
    runSTLUtilitiesDemo(oss);
    REQUIRE(oss.str().find("PropertyBag") != std::string::npos);
}
