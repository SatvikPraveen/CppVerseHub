// Tests for stl_showcase/Functors.hpp
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <functional>
#include <iterator>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

#include "stl_showcase/Functors.hpp"

using namespace CppVerseHub::STL;
using Catch::Approx;

TEST_CASE("Starship combat effectiveness", "[functors][data]") {
    // A std::string member cannot live in a constexpr variable, but a temporary can be used
    // inside a constant expression.
    STATIC_REQUIRE(Starship{"T", "X", 50.0, 10.0, 1, 100.0}.combatEffectiveness() == 100.0);
    REQUIRE(sampleStarships().size() == 5);
}

TEST_CASE("IsCombatReady is a configurable constexpr predicate", "[functors][predicate]") {
    constexpr IsCombatReady ready;
    STATIC_REQUIRE(ready.minShields() == 50.0);
    STATIC_REQUIRE(ready(Starship{"a", "b", 60.0, 6.0, 1, 1.0}));
    STATIC_REQUIRE_FALSE(ready(Starship{"a", "b", 40.0, 9.0, 1, 1.0}));
    const auto ships = sampleStarships();
    REQUIRE(std::ranges::count_if(ships, ready) == 3);
    REQUIRE(std::ranges::count_if(ships, IsCombatReady{90.0, 9.0}) == 1);
    REQUIRE(std::ranges::count_if(ships, IsCombatReady{0.0, 0.0}) == 5);
}

TEST_CASE("ByCombatEffectivenessDesc orders ships", "[functors][comparator]") {
    auto ships = sampleStarships();
    std::ranges::sort(ships, ByCombatEffectivenessDesc{});
    REQUIRE(ships.front().name == "Defiant");
    REQUIRE(std::ranges::is_sorted(ships, std::greater<>{}, &Starship::combatEffectiveness));
    const Starship a{"A", "", 50.0, 5.0, 1, 10.0};
    const Starship b{"B", "", 50.0, 5.0, 1, 10.0};
    REQUIRE(ByCombatEffectivenessDesc{}(a, b));  // tie broken by name
    REQUIRE_FALSE(ByCombatEffectivenessDesc{}(b, a));
    REQUIRE_FALSE(ByCombatEffectivenessDesc{}(a, a));  // irreflexive
}

TEST_CASE("FleetStatsAccumulator folds with std::accumulate", "[functors][accumulator]") {
    const auto ships = sampleStarships();
    const auto stats = std::accumulate(ships.begin(), ships.end(), FleetStats{}, FleetStatsAccumulator{});
    REQUIRE(stats.ship_count == 5);
    REQUIRE(stats.total_crew == 934);
    REQUIRE(stats.total_firepower == Approx(3850.0));
    REQUIRE(stats.max_warp == Approx(9.9));
    REQUIRE(stats.averageCrew() == Approx(186.8));
    REQUIRE(FleetStats{}.averageCrew() == 0.0);
}

TEST_CASE("StarshipNameGenerator state survives only through std::ref", "[functors][generator]") {
    StarshipNameGenerator gen("NX", 7);
    std::vector<std::string> by_ref;
    std::generate_n(std::back_inserter(by_ref), 3, std::ref(gen));
    REQUIRE(by_ref == std::vector<std::string>{"NX-007", "NX-008", "NX-009"});
    REQUIRE(gen.generated() == 3);

    std::vector<std::string> by_value;
    std::generate_n(std::back_inserter(by_value), 2, gen);  // a copy advances, not gen
    REQUIRE(by_value == std::vector<std::string>{"NX-010", "NX-011"});
    REQUIRE(gen.generated() == 3);
    REQUIRE(gen() == "NX-010");

    StarshipNameGenerator wide("Z", 1234);
    REQUIRE(wide() == "Z-1234");
}

TEST_CASE("SharedCallCounter shares its count across copies", "[functors][state]") {
    const SharedCallCounter counter([](int x) { return x > 2; });
    const std::vector<int> v{1, 2, 3, 4, 5};
    REQUIRE(std::ranges::count_if(v, counter) == 3);
    REQUIRE(counter.calls() == 5);
    auto copy = counter;
    REQUIRE(copy(10));
    REQUIRE(counter.calls() == 6);
}

TEST_CASE("Closure factories: makeMultiplier and makeCounter", "[functors][lambdas]") {
    constexpr auto times7 = makeMultiplier(7);
    STATIC_REQUIRE(times7(6) == 42);
    auto counter = makeCounter(5);
    REQUIRE(counter() == 5);
    REQUIRE(counter() == 6);
    auto snapshot = counter;  // copying a mutable lambda copies its state
    REQUIRE(snapshot() == 7);
    REQUIRE(counter() == 7);
    constexpr int third = [] {
        auto c = makeCounter();
        (void)c();
        (void)c();
        return c();
    }();
    STATIC_REQUIRE(third == 2);
}

TEST_CASE("compose applies right to left", "[functors][lambdas]") {
    const auto inc = [](int x) { return x + 1; };
    const auto dbl = [](int x) { return x * 2; };
    REQUIRE(compose(inc, dbl)(5) == 11);
    REQUIRE(compose(dbl, inc)(5) == 12);
    REQUIRE(compose(inc)(1) == 2);
    const auto to_string = [](int x) { return std::to_string(x); };
    REQUIRE(compose(to_string, dbl, inc)(4) == "10");
    constexpr auto sq = compose([](int x) { return x * x; }, [](int x) { return x + 1; });
    STATIC_REQUIRE(sq(2) == 9);
}

TEST_CASE("countWhere accepts generic lambdas and member pointers", "[functors][lambdas]") {
    const auto ships = sampleStarships();
    REQUIRE(countWhere(ships, [](const auto& s) { return s.crew > 100; }) == 3);
    const std::vector<bool> flags{true, false, true};
    REQUIRE(countWhere(flags, [](bool b) { return b; }) == 2);
    struct Flagged {
        bool active;
    };
    const std::vector<Flagged> items{{true}, {false}, {true}, {true}};
    REQUIRE(countWhere(items, &Flagged::active) == 3);  // data-member pointer via std::invoke
}

TEST_CASE("Memoized caches results", "[functors][memo]") {
    int calls = 0;
    Memoized<int, int> square([&calls](const int& x) {
        ++calls;
        return x * x;
    });
    REQUIRE(square(4) == 16);
    REQUIRE(square(4) == 16);
    REQUIRE(square(5) == 25);
    REQUIRE(calls == 2);
    REQUIRE(square.hits() == 1);
    REQUIRE(square.misses() == 2);
    square.clear();
    REQUIRE(square(4) == 16);
    REQUIRE(calls == 3);
}

TEST_CASE("memoizedFibonacci", "[functors][memo]") {
    REQUIRE(memoizedFibonacci(0) == 0);
    REQUIRE(memoizedFibonacci(1) == 1);
    REQUIRE(memoizedFibonacci(10) == 55);
    REQUIRE(memoizedFibonacci(93) == 12200160415121876738ULL);
    REQUIRE_THROWS_AS(memoizedFibonacci(94), std::out_of_range);
}

TEST_CASE("Distance, warp time and std::bind", "[functors][bind]") {
    REQUIRE(calculateDistance(0, 0, 3, 4) == Approx(5.0));
    const Starship slow{"s", "", 0, 2.0, 0, 0};
    REQUIRE(calculateWarpTime(slow, 80.0) == Approx(10.0));
    REQUIRE(std::isinf(calculateWarpTime(Starship{}, 1.0)));
    const std::vector<Starship> ships{slow, Starship{"f", "", 0, 4.0, 0, 0}};
    const auto times = warpTimesTo(ships, 640.0);
    REQUIRE(times.size() == 2);
    REQUIRE(times[0] == Approx(80.0));
    REQUIRE(times[1] == Approx(10.0));

    using std::placeholders::_1;
    const auto from_origin_x = std::bind(calculateDistance, 0.0, 0.0, _1, 0.0);
    REQUIRE(from_origin_x(-7.0) == Approx(7.0));
    const auto bound = std::bind_front(calculateDistance, 1.0, 1.0);
    REQUIRE(bound(4.0, 5.0) == Approx(5.0));
}

TEST_CASE("namesNotReady uses std::not_fn", "[functors][functional]") {
    const auto ships = sampleStarships();
    REQUIRE(namesNotReady(ships, IsCombatReady{}) == std::vector<std::string>{"Reliant", "Rocinante"});
    REQUIRE(namesNotReady(ships, IsCombatReady{0.0, 0.0}).empty());
}

TEST_CASE("Standard function objects and std::invoke", "[functors][functional]") {
    std::vector<int> v{3, 1, 2};
    std::ranges::sort(v, std::greater<>{});
    REQUIRE(v == std::vector<int>{3, 2, 1});
    REQUIRE(std::accumulate(v.begin(), v.end(), 1, std::multiplies<>{}) == 6);
    REQUIRE(std::invoke(std::minus<>{}, 10, 4) == 6);
    const Starship ship{"Inv", "", 50.0, 10.0, 12, 100.0};
    REQUIRE(std::invoke(&Starship::crew, ship) == 12);
    REQUIRE(std::invoke(&Starship::combatEffectiveness, ship) == Approx(100.0));
    REQUIRE(std::mem_fn(&Starship::name)(ship) == "Inv");
}

TEST_CASE("EventDispatcher subscribe, dispatch and unsubscribe", "[functors][function]") {
    EventDispatcher dispatcher;
    std::vector<std::string> log;
    const auto t1 = dispatcher.subscribe("dock", [&log](const Starship& s) { log.push_back("1:" + s.name); });
    const auto t2 = dispatcher.subscribe("dock", [&log](const Starship& s) { log.push_back("2:" + s.name); });
    dispatcher.subscribe("launch", [&log](const Starship&) { log.emplace_back("launch"); });
    REQUIRE(t1 != t2);
    REQUIRE(dispatcher.handlerCount("dock") == 2);

    const Starship ship{"Ent", "", 0, 0, 0, 0};
    REQUIRE(dispatcher.dispatch("dock", ship) == 2);
    REQUIRE(log == std::vector<std::string>{"1:Ent", "2:Ent"});
    REQUIRE(dispatcher.dispatch("unknown", ship) == 0);

    REQUIRE(dispatcher.unsubscribe(t1));
    REQUIRE_FALSE(dispatcher.unsubscribe(t1));
    REQUIRE(dispatcher.handlerCount("dock") == 1);
    REQUIRE(dispatcher.unsubscribe(t2));
    REQUIRE(dispatcher.handlerCount("dock") == 0);
    REQUIRE(dispatcher.handlerCount("launch") == 1);
    REQUIRE_THROWS_AS(dispatcher.subscribe("x", EventDispatcher::Handler{}), std::invalid_argument);
}
