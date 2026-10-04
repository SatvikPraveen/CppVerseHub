#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <list>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

#include "modern/RangesDemo.hpp"

using namespace CppVerseHub::Modern::Ranges;

TEST_CASE("sample data is deterministic", "[modern][ranges]") {
    CHECK(generatePlanets().size() == 10);
    CHECK(generateFleets().size() == 6);
    CHECK(generateMissions().size() == 8);
    CHECK(makeRandomValues(100, 7) == makeRandomValues(100, 7));
    CHECK(makeRandomValues(100, 7) != makeRandomValues(100, 8));
    for (int v : makeRandomValues(500, 1)) {
        CHECK((v >= 0 && v < 1000));
    }
}

TEST_CASE("toVector materialises views", "[modern][ranges]") {
    CHECK(toVector(std::views::iota(1, 5)) == std::vector<int>{1, 2, 3, 4});
    std::list<char> letters{'a', 'b'};
    CHECK(toVector(letters) == std::vector<char>{'a', 'b'});
    CHECK(toVector(std::views::empty<int>).empty());
}

TEST_CASE("habitablePlanetNames filters and projects", "[modern][ranges]") {
    CHECK(habitablePlanetNames(generatePlanets()) ==
          std::vector<std::string>{"Earth", "Mars", "Proxima-b", "Kepler-442b"});
    CHECK(habitablePlanetNames({}).empty());
}

TEST_CASE("topByPopulation sorts by projection", "[modern][ranges]") {
    const auto planets = generatePlanets();
    CHECK(topByPopulation(planets, 3) == std::vector<std::string>{"Earth", "Kepler-442b", "Proxima-b"});
    CHECK(topByPopulation(planets, 0).empty());
    CHECK(topByPopulation(planets, 100).size() == planets.size());
}

TEST_CASE("totalPopulation sums a projected view", "[modern][ranges]") {
    CHECK(totalPopulation(generatePlanets()) == 8'000'000'000LL + 2'000'000LL + 15'000'000LL + 50'000'000LL + 1'000LL);
    CHECK(totalPopulation({}) == 0);
}

TEST_CASE("readyFleetIds applies all criteria", "[modern][ranges]") {
    const auto fleets = generateFleets();
    CHECK(readyFleetIds(fleets, 60.0, 10) == std::vector<int>{1, 6});
    CHECK(readyFleetIds(fleets, 0.0, 0) == std::vector<int>{1, 2, 3, 5, 6});  // fleet 4 inactive
    CHECK(readyFleetIds(fleets, 100.0, 0).empty());
}

TEST_CASE("missionIdsByUrgency orders by priority then progress, stably", "[modern][ranges]") {
    CHECK(missionIdsByUrgency(generateMissions()) == std::vector<int>{2, 8, 4, 7, 5, 6, 1, 3});
}

TEST_CASE("planetsBySystem groups names", "[modern][ranges]") {
    const auto groups = planetsBySystem(generatePlanets());
    REQUIRE(groups.size() == 3);
    CHECK(groups.at("Sol").size() == 5);
    CHECK(groups.at("Alpha Centauri") == std::vector<std::string>{"Proxima-b", "Proxima-d"});
    CHECK(groups.at("Kepler-442").front() == "Kepler-442b");
}

TEST_CASE("splitWords drops empty tokens", "[modern][ranges]") {
    CHECK(splitWords("a,b,,c,", ',') == std::vector<std::string>{"a", "b", "c"});
    CHECK(splitWords("", ',').empty());
    CHECK(splitWords("single", ',') == std::vector<std::string>{"single"});
    CHECK(splitWords("  two  words ", ' ') == std::vector<std::string>{"two", "words"});
}

TEST_CASE("flatten joins nested ranges", "[modern][ranges]") {
    CHECK(flatten({{1}, {}, {2, 3}, {4}}) == std::vector<int>{1, 2, 3, 4});
    CHECK(flatten({}).empty());
}

TEST_CASE("squaresOfOdds takes from an infinite view", "[modern][ranges]") {
    CHECK(squaresOfOdds(4) == std::vector<long long>{1, 9, 25, 49});
    CHECK(squaresOfOdds(0).empty());
}

TEST_CASE("views are lazy", "[modern][ranges]") {
    const std::vector<int> data(1000, 3);
    const std::size_t n = GENERATE(0U, 1U, 5U, 1000U, 2000U);
    CHECK(countEvaluationsForFirst(data, n) == std::min<std::size_t>(n, data.size()));
}

TEST_CASE("take_while / drop_while split at the first failing element", "[modern][ranges]") {
    const auto [prefix, rest] = splitAtFirstNotBelow({1, 2, 9, 3, 4}, 5);
    CHECK(prefix == std::vector<int>{1, 2});
    CHECK(rest == std::vector<int>{9, 3, 4});
    const auto [all, none] = splitAtFirstNotBelow({1, 2}, 5);
    CHECK(all.size() == 2);
    CHECK(none.empty());
}

TEST_CASE("ranges pipeline and raw loop agree", "[modern][ranges]") {
    const auto seed = GENERATE(1U, 2U, 42U);
    const auto data = makeRandomValues(5000, seed);
    CHECK(sumSquaresOfEvensRanges(data) == sumSquaresOfEvensLoop(data));
    CHECK(sumSquaresOfEvensRanges({1, 2, 3, 4}) == 20);
}

TEST_CASE("EveryNthView yields every n-th element", "[modern][ranges]") {
    std::vector<int> v(10);
    std::iota(v.begin(), v.end(), 0);
    CHECK(toVector(v | everyNth(3)) == std::vector<int>{0, 3, 6, 9});
    CHECK(toVector(v | everyNth(1)) == v);
    CHECK(toVector(v | everyNth(10)) == std::vector<int>{0});
    CHECK(toVector(v | everyNth(25)) == std::vector<int>{0});
    CHECK(toVector(v | everyNth(0)) == v);  // invalid strides are clamped to 1
    CHECK(toVector(std::vector<int>{} | everyNth(2)).empty());
}

TEST_CASE("EveryNthView composes with standard adaptors", "[modern][ranges]") {
    std::list<int> l{1, 2, 3, 4, 5, 6, 7};
    auto view = l | everyNth(2) | std::views::transform([](int x) { return x * 10; });
    CHECK(toVector(view) == std::vector<int>{10, 30, 50, 70});
    CHECK(toVector(std::views::iota(0, 20) | everyNth(7)) == std::vector<int>{0, 7, 14});
    CHECK(toVector(std::views::iota(0, 12) | std::views::reverse | everyNth(5)) == std::vector<int>{11, 6, 1});
    STATIC_CHECK(std::ranges::forward_range<decltype(view)>);
}

TEST_CASE("EveryNthView supports multi-pass iteration and mutation", "[modern][ranges]") {
    std::vector<int> v{1, 1, 1, 1, 1};
    auto view = v | everyNth(2);
    for (int& x : view) {
        x = 0;
    }
    CHECK(v == std::vector<int>{0, 1, 0, 1, 0});
    CHECK(std::ranges::distance(view.begin(), view.end()) == 3);
    CHECK(std::ranges::distance(view.begin(), view.end()) == 3);
    CHECK(view.step() == 2);
}

TEST_CASE("ranges showcases write to the stream", "[modern][ranges]") {
    std::ostringstream os;
    demonstrateAllRanges(os);
    const auto text = os.str();
    CHECK(text.find("every 3rd: 1 4 7 10") != std::string::npos);
    CHECK(text.find("evaluated the transform 3 times") != std::string::npos);
    CHECK(text.find("keys: Alpha Beta Gamma") != std::string::npos);
}
