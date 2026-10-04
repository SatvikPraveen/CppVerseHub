/**
 * @file StrategyTests.cpp
 * @brief Tests for routing strategies (runtime, compile-time) and target selectors.
 */

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <random>

#include "patterns/Strategy.hpp"

using namespace CppVerseHub::Patterns;
using Catch::Approx;

namespace {
NavigationContext hazardOnAxis() {
    NavigationContext ctx;
    ctx.hazards.push_back({{50.0, 0.0, 0.0}, 10.0, 5.0});
    return ctx;
}
constexpr Coordinate3D kFrom{0.0, 0.0, 0.0};
constexpr Coordinate3D kTo{100.0, 0.0, 0.0};
}  // namespace

TEST_CASE("Coordinate3D vector maths", "[strategy][geometry]") {
    constexpr Coordinate3D a{1.0, 2.0, 3.0};
    constexpr Coordinate3D b{4.0, 6.0, 3.0};
    STATIC_REQUIRE((a + b) == Coordinate3D{5.0, 8.0, 6.0});
    STATIC_REQUIRE(a.dot(b) == 4.0 + 12.0 + 9.0);
    STATIC_REQUIRE(Coordinate3D{1, 0, 0}.cross({0, 1, 0}) == Coordinate3D{0, 0, 1});
    CHECK(a.distanceTo(b) == Approx(5.0));
    CHECK((b - a).length() == Approx(5.0));
}

TEST_CASE("segmentIntersects detects crossings", "[strategy][geometry]") {
    const Hazard h{{50.0, 0.0, 0.0}, 10.0, 1.0};
    CHECK(segmentIntersects(kFrom, kTo, h));
    CHECK_FALSE(segmentIntersects({0, 20, 0}, {100, 20, 0}, h));
    CHECK_FALSE(segmentIntersects({0, 0, 0}, {30, 0, 0}, h));  // stops short
    CHECK(segmentIntersects({50, 0, 0}, {50, 0, 0}, h));        // degenerate segment inside
}

TEST_CASE("DirectLineStrategy produces the straight full-throttle route", "[strategy]") {
    NavigationContext ctx = hazardOnAxis();
    ctx.cruiseSpeed = 2.0;
    const Route r = DirectLineStrategy{}.plan(kFrom, kTo, ctx);
    CHECK(r.waypoints.size() == 2);
    CHECK(r.distance == Approx(100.0));
    CHECK(r.time == Approx(50.0));
    CHECK(r.fuel == Approx(100.0));
    CHECK(r.risk == Approx(5.0));
    CHECK(r.strategy == "Direct");
}

TEST_CASE("FuelOptimizedStrategy trades time for fuel quadratically", "[strategy]") {
    const NavigationContext ctx;
    const double throttle = GENERATE(0.25, 0.5, 0.8);
    const FuelOptimizedStrategy s(throttle);
    const Route r = s.plan(kFrom, kTo, ctx);
    const Route direct = DirectLineStrategy{}.plan(kFrom, kTo, ctx);
    CHECK(r.fuel == Approx(direct.fuel * throttle * throttle));
    CHECK(r.time == Approx(direct.time / throttle));
    CHECK(s.throttle() == throttle);
    CHECK(FuelOptimizedStrategy(5.0).throttle() == 1.0);  // clamped
}

TEST_CASE("SafeRouteStrategy detours around hazards", "[strategy][safe]") {
    const NavigationContext ctx = hazardOnAxis();
    const Route safe = SafeRouteStrategy{}.plan(kFrom, kTo, ctx);
    CHECK(safe.risk == 0.0);
    CHECK(safe.waypoints.size() > 2);
    CHECK(safe.waypoints.front() == kFrom);
    CHECK(safe.waypoints.back() == kTo);
    CHECK(safe.distance > 100.0);
    for (std::size_t i = 1; i < safe.waypoints.size(); ++i) {
        CHECK_FALSE(segmentIntersects(safe.waypoints[i - 1], safe.waypoints[i], ctx.hazards[0]));
    }
}

TEST_CASE("SafeRouteStrategy avoids several random hazards (seeded)", "[strategy][safe]") {
    std::mt19937 rng(12345);
    std::uniform_real_distribution<double> pos(20.0, 80.0);
    std::uniform_real_distribution<double> off(-15.0, 15.0);
    NavigationContext ctx;
    for (int i = 0; i < 3; ++i) {
        ctx.hazards.push_back({{pos(rng) + i * 5.0, off(rng), off(rng)}, 4.0, 1.0});
    }
    const Route safe = SafeRouteStrategy{}.plan(kFrom, kTo, ctx);
    const Route direct = DirectLineStrategy{}.plan(kFrom, kTo, ctx);
    CHECK(safe.risk <= direct.risk);
    CHECK(safe.risk == 0.0);
}

TEST_CASE("SafeRouteStrategy leaves clear routes straight and ignores unavoidable hazards", "[strategy][safe]") {
    NavigationContext ctx;
    CHECK(SafeRouteStrategy{}.plan(kFrom, kTo, ctx).waypoints.size() == 2);
    ctx.hazards.push_back({kTo, 5.0, 3.0});  // destination inside a hazard
    const Route r = SafeRouteStrategy{}.plan(kFrom, kTo, ctx);
    CHECK(r.waypoints.size() == 2);
    CHECK(r.risk == 3.0);
}

TEST_CASE("BalancedStrategy picks the candidate with the lowest weighted score", "[strategy][balanced]") {
    const NavigationContext ctx = hazardOnAxis();
    SECTION("risk dominates -> safe route") {
        const Route r = BalancedStrategy({0.0, 0.0, 1000.0}).plan(kFrom, kTo, ctx);
        CHECK(r.strategy == "Balanced(SafeRoute)");
        CHECK(r.risk == 0.0);
    }
    SECTION("time dominates -> direct") {
        const Route r = BalancedStrategy({1.0, 0.0, 0.0}).plan(kFrom, kTo, ctx);
        CHECK(r.strategy == "Balanced(Direct)");
    }
    SECTION("fuel dominates -> fuel optimised") {
        const Route r = BalancedStrategy({0.0, 1.0, 0.0}).plan(kFrom, kTo, ctx);
        CHECK(r.strategy == "Balanced(FuelOptimized)");
    }
}

TEST_CASE("score is the weighted sum of metrics", "[strategy]") {
    Route r;
    r.time = 2.0;
    r.fuel = 3.0;
    r.risk = 4.0;
    CHECK(score(r, {1.0, 10.0, 100.0}) == Approx(432.0));
}

TEST_CASE("FleetRouter swaps strategies at run time", "[strategy][router]") {
    FleetRouter router(makeRoutingStrategy(RoutingStrategyType::Direct), hazardOnAxis());
    CHECK(router.strategyName() == "Direct");
    const Route direct = router.plan(kFrom, kTo);
    router.setStrategy(makeRoutingStrategy(RoutingStrategyType::SafeRoute));
    CHECK(router.strategyName() == "SafeRoute");
    const Route safe = router.plan(kFrom, kTo);
    CHECK(safe.risk < direct.risk);
    router.context().hazards.clear();
    CHECK(router.plan(kFrom, kTo).waypoints.size() == 2);
    CHECK_THROWS_AS(router.setStrategy(nullptr), std::invalid_argument);
    CHECK_THROWS_AS(FleetRouter(nullptr), std::invalid_argument);
}

TEST_CASE("makeRoutingStrategy creates every type", "[strategy][factory]") {
    const auto [type, name] = GENERATE(table<RoutingStrategyType, std::string_view>(
        {{RoutingStrategyType::Direct, "Direct"},
         {RoutingStrategyType::FuelOptimized, "FuelOptimized"},
         {RoutingStrategyType::SafeRoute, "SafeRoute"},
         {RoutingStrategyType::Balanced, "Balanced"}}));
    const auto s = makeRoutingStrategy(type);
    REQUIRE(s != nullptr);
    CHECK(s->name() == name);
}

namespace {
struct ManhattanPolicy {
    [[nodiscard]] Route plan(const Coordinate3D& from, const Coordinate3D& to, const NavigationContext& ctx) const {
        const Coordinate3D corner{to.x, from.y, from.z};
        return evaluateRoute({from, corner, to}, 1.0, ctx, "Manhattan");
    }
};
struct NotAPolicy {};
}  // namespace

TEST_CASE("StaticRouter accepts any RoutingPolicy at compile time", "[strategy][policy]") {
    STATIC_REQUIRE(RoutingPolicy<ManhattanPolicy>);
    STATIC_REQUIRE(RoutingPolicy<DirectLineStrategy>);
    STATIC_REQUIRE_FALSE(RoutingPolicy<NotAPolicy>);
    const StaticRouter<ManhattanPolicy> router;
    const Route r = router.plan({0, 0, 0}, {3, 4, 0});
    CHECK(r.distance == Approx(7.0));
    CHECK(r.waypoints.size() == 3);
}

TEST_CASE("Target selectors choose according to their criteria", "[strategy][targets]") {
    const std::vector<PlanetTarget> targets{
        {"Near", {5, 0, 0}, 10.0, 1.0},
        {"Rich", {90, 0, 0}, 100.0, 80.0},
        {"Sweet", {20, 0, 0}, 60.0, 5.0},
    };
    CHECK(nearestTarget(targets, kFrom) == 0u);
    CHECK(highestValueTarget(targets, kFrom) == 1u);
    CHECK(bestValueRatioTarget(targets, kFrom) == 2u);
    const TargetSelector capped = weakerThan(10.0, highestValueTarget);
    CHECK(capped(targets, kFrom) == 2u);  // index refers to the original list
    CHECK_FALSE(weakerThan(0.5, nearestTarget)(targets, kFrom).has_value());
    CHECK_THROWS_AS(weakerThan(1.0, nullptr), std::invalid_argument);
}

TEST_CASE("Target selectors handle empty input and ties deterministically", "[strategy][targets]") {
    const std::vector<PlanetTarget> none;
    CHECK_FALSE(nearestTarget(none, kFrom).has_value());
    CHECK_FALSE(highestValueTarget(none, kFrom).has_value());
    const std::vector<PlanetTarget> tied{{"A", {1, 0, 0}, 5.0, 0.0}, {"B", {-1, 0, 0}, 5.0, 0.0}};
    CHECK(nearestTarget(tied, kFrom) == 0u);
    CHECK(highestValueTarget(tied, kFrom) == 0u);
}
