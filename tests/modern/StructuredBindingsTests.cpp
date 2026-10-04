#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

#include "modern/StructuredBindings.hpp"

using namespace CppVerseHub::Modern::StructuredBindings;
using Catch::Approx;

TEST_CASE("orbitParameters decomposes into three values", "[modern][bindings]") {
    const auto [v, period, escape] = orbitParameters(400.0, 4.0);
    CHECK(v == Approx(10.0));
    CHECK(period == Approx(2.0 * 3.14159265358979323846 * 4.0 / 10.0));
    CHECK(escape == Approx(10.0 * std::sqrt(2.0)));
}

TEST_CASE("jumpDistance classifies jumps", "[modern][bindings]") {
    auto [d1, c1] = jumpDistance({0, 0, 0}, {3, 4, 0});
    CHECK(d1 == Approx(5.0));
    CHECK(c1 == "short");
    auto [d2, c2] = jumpDistance({0, 0, 0}, {0, 60, 80});
    CHECK(d2 == Approx(100.0));
    CHECK(c2 == "long");
    CHECK(jumpDistance({1, 1, 1}, {1, 1, 51}).second == "medium");
}

TEST_CASE("findBestFleet returns the highest score or nullopt", "[modern][bindings]") {
    CHECK_FALSE(findBestFleet({}).has_value());
    const auto best = findBestFleet(sampleFleets());
    REQUIRE(best.has_value());
    const auto& [commander, score] = *best;
    CHECK(commander == "Zhang");  // 12*0.85 = 10.2 beats 20*0.45 = 9.0
    CHECK(score == Approx(10.2));
}

TEST_CASE("missionStats counts completions and averages", "[modern][bindings]") {
    const auto [done, mean] = missionStats({{1, "a", 100.0, 1}, {2, "b", 20.0, 1}});
    CHECK(done == 1);
    CHECK(mean == Approx(60.0));
    const auto [none, zero] = missionStats({});
    CHECK(none == 0);
    CHECK(zero == 0.0);
}

TEST_CASE("centerOfMass weights positions by mass", "[modern][bindings]") {
    const auto [x, y, z] = centerOfMass({{"A", 1.0, {0, 0, 0}, false}, {"B", 3.0, {4, 8, -4}, true}});
    CHECK(x == Approx(3.0));
    CHECK(y == Approx(6.0));
    CHECK(z == Approx(-3.0));
    const auto [ex, ey, ez] = centerOfMass({});
    CHECK((ex == 0.0 && ey == 0.0 && ez == 0.0));
}

TEST_CASE("fuelRange returns a named aggregate", "[modern][bindings]") {
    const auto [lo, hi] = fuelRange(sampleFleets());
    CHECK(lo == Approx(30.0));
    CHECK(hi == Approx(92.5));
    const auto empty = fuelRange({});
    CHECK(empty.minimum == 0.0);
    CHECK(empty.maximum == 0.0);
}

TEST_CASE("refuelBelow mutates through reference bindings", "[modern][bindings]") {
    auto fleets = sampleFleets();
    CHECK(refuelBelow(fleets, 50.0) == 2);
    CHECK(fleets[1].fuelPercentage == 100.0);
    CHECK(fleets[3].fuelPercentage == 100.0);
    CHECK(fleets[0].fuelPercentage == 85.0);
    CHECK(refuelBelow(fleets, 50.0) == 0);
}

TEST_CASE("shipsByMission and busiestMission decompose map entries", "[modern][bindings]") {
    const auto totals = shipsByMission(sampleFleets());
    CHECK(totals.size() == 3);
    CHECK(totals.at("Exploration") == 20);
    CHECK(totals.at("Combat") == 20);
    CHECK(totals.at("Trade") == 15);
    const auto busiest = busiestMission(totals);
    REQUIRE(busiest);
    CHECK(busiest->first == "Combat");  // tie broken by key order
    CHECK_FALSE(busiestMission({}).has_value());
}

TEST_CASE("ShipRecord supports the tuple-like protocol", "[modern][bindings]") {
    STATIC_CHECK(std::tuple_size_v<ShipRecord> == 3);
    STATIC_CHECK(std::is_same_v<std::tuple_element_t<1, ShipRecord>, std::string>);
    ShipRecord r(7, "Nova", 40);
    auto& [id, name, crew] = r;
    CHECK(id == 7);
    CHECK(name == "Nova");
    crew = 55;
    CHECK(r.crew() == 55);
    auto [cid, cname, ccrew] = r;  // by-value binding copies the record
    ccrew = 1;
    CHECK(r.crew() == 55);
    CHECK(cname == "Nova");
    CHECK(totalCrew({{1, "a", 10}, {2, "b", 32}}) == 42);
}

TEST_CASE("ShipRecord rvalue get moves out", "[modern][bindings]") {
    ShipRecord r(1, "Moveable", 5);
    std::string taken = std::move(r).get<1>();
    CHECK(taken == "Moveable");
}

TEST_CASE("lambdas can capture structured bindings (C++20)", "[modern][bindings]") {
    const auto f = makeScaledOffset({1.0, -2.0, 0.5}, 3.0);
    const auto [x, y, z] = f(1.0);
    CHECK(x == Approx(4.0));
    CHECK(y == Approx(-5.0));
    CHECK(z == Approx(2.5));
}

TEST_CASE("structured-bindings showcase writes to the stream", "[modern][bindings]") {
    std::ostringstream os;
    demonstrateStructuredBindings(os);
    const auto text = os.str();
    CHECK(text.find("distance=50 (medium jump)") != std::string::npos);
    CHECK(text.find("refuelled 2 fleets") != std::string::npos);
    CHECK(text.find("Explorer (#1) crew 160") != std::string::npos);
}
