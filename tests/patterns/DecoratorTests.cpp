/**
 * @file DecoratorTests.cpp
 * @brief Tests for mission decorators and function decorators.
 */

#include "patterns/Decorator.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <atomic>
#include <cstdint>
#include <stdexcept>
#include <thread>

using namespace CppVerseHub::Patterns;
using Catch::Approx;

TEST_CASE("BasicMission values depend on kind", "[decorator][base]") {
    const BasicMission e(MissionKind::Exploration, "X");
    const BasicMission c(MissionKind::Combat, "X");
    const BasicMission k(MissionKind::Colonization, "X");
    CHECK(e.description() == "Exploration of X");
    CHECK(c.description() == "Combat operation at X");
    CHECK(k.description() == "Colonization of X");
    CHECK(c.cost() > e.cost());
    CHECK(k.durationHours() > e.durationHours());
    CHECK(e.enhancements().empty());
    CHECK(c.kind() == MissionKind::Combat);
}

TEST_CASE("Single decorators modify exactly their aspect", "[decorator]") {
    const BasicMission base(MissionKind::Exploration, "Y");
    auto make = [] { return std::make_unique<BasicMission>(MissionKind::Exploration, "Y"); };
    SECTION("stealth") {
        const auto m = decorate<StealthEnhancement>(make());
        CHECK(m->cost() == Approx(base.cost() + 2000.0));
        CHECK(m->durationHours() == Approx(base.durationHours()));
        CHECK(m->successProbability() == Approx(0.8 + 0.2 * 0.15));
        CHECK(m->description() == "Exploration of Y + stealth");
    }
    SECTION("speed") {
        const auto m = decorate<SpeedBoost>(make());
        CHECK(m->durationHours() == Approx(base.durationHours() * 0.7));
        CHECK(m->cost() == Approx(base.cost() * 1.25));
        CHECK(m->successProbability() == Approx(base.successProbability()));
    }
    SECTION("medical") {
        const auto m = decorate<MedicalSupport>(make());
        CHECK(m->durationHours() == Approx(base.durationHours() + 1.0));
        CHECK(m->cost() == Approx(base.cost() + 800.0));
    }
}

TEST_CASE("HeavyArmament helps combat missions more", "[decorator]") {
    const auto combat = decorate<HeavyArmament>(std::make_unique<BasicMission>(MissionKind::Combat, "Z"),
                                                true);
    const auto explore =
        decorate<HeavyArmament>(std::make_unique<BasicMission>(MissionKind::Exploration, "Z"), false);
    CHECK(combat->successProbability() == Approx(0.55 + 0.45 * 0.40));
    CHECK(explore->successProbability() == Approx(0.80 + 0.20 * 0.05));
    CHECK(combat->cost() == Approx(30000.0));
}

TEST_CASE("Decorators stack and record enhancement order", "[decorator][stack]") {
    MissionPtr m = std::make_unique<BasicMission>(MissionKind::Combat, "R");
    m = decorate<HeavyArmament>(std::move(m), true);
    m = decorate<StealthEnhancement>(std::move(m));
    m = decorate<SpeedBoost>(std::move(m));
    CHECK(m->enhancements() == std::vector<std::string>{"HeavyArmament", "Stealth", "SpeedBoost"});
    CHECK(m->description() == "Combat operation at R + heavy armament + stealth + speed boost");
    CHECK(m->cost() == Approx((25000.0 + 5000.0 + 2000.0) * 1.25));
}

TEST_CASE("Decorator order matters for non-commutative effects", "[decorator][stack]") {
    auto base = [] { return std::make_unique<BasicMission>(MissionKind::Exploration, "O"); };
    const auto speedThenStealth = decorate<StealthEnhancement>(decorate<SpeedBoost>(base()));
    const auto stealthThenSpeed = decorate<SpeedBoost>(decorate<StealthEnhancement>(base()));
    CHECK(speedThenStealth->cost() == Approx(10000.0 * 1.25 + 2000.0));
    CHECK(stealthThenSpeed->cost() == Approx((10000.0 + 2000.0) * 1.25));
    CHECK(speedThenStealth->successProbability() == Approx(stealthThenSpeed->successProbability()));
}

TEST_CASE("Success probability stays within [0,1] however deeply stacked", "[decorator][stack]") {
    const int depth = GENERATE(1, 5, 50);
    MissionPtr m = std::make_unique<BasicMission>(MissionKind::Combat, "D");
    for (int i = 0; i < depth; ++i) {
        m = decorate<HeavyArmament>(std::move(m), true);
        m = decorate<MedicalSupport>(std::move(m));
    }
    CHECK(m->successProbability() <= 1.0);
    CHECK(m->successProbability() > 0.55);
    CHECK(m->enhancements().size() == static_cast<std::size_t>(2 * depth));
}

TEST_CASE("Decorating null is rejected", "[decorator]") {
    CHECK_THROWS_AS(decorate<SpeedBoost>(nullptr), std::invalid_argument);
}

TEST_CASE("withRetry retries until success or exhaustion", "[decorator][function]") {
    int failures = 2;
    int calls = 0;
    auto flaky = [&](int x) {
        ++calls;
        if (failures-- > 0) {
            throw std::runtime_error("transient");
        }
        return x + 1;
    };
    auto retried = withRetry(flaky, 3);
    CHECK(retried(41) == 42);
    CHECK(calls == 3);

    int alwaysCalls = 0;
    auto always = withRetry(
        [&] {
            ++alwaysCalls;
            throw std::logic_error("permanent");
        },
        4);
    CHECK_THROWS_AS(always(), std::logic_error);
    CHECK(alwaysCalls == 4);
    CHECK_THROWS_AS(withRetry([] {}, 0), std::invalid_argument);
}

TEST_CASE("withCallCounter counts invocations transparently", "[decorator][function]") {
    auto counter = std::make_shared<std::size_t>(0);
    auto add = withCallCounter([](int a, int b) { return a + b; }, counter);
    CHECK(add(1, 2) == 3);
    CHECK(add(5, 5) == 10);
    CHECK(*counter == 2);
}

TEST_CASE("memoize caches results per argument tuple", "[decorator][function]") {
    int evaluations = 0;
    auto slowSquare = memoize<long, int>([&](int x) -> long {
        ++evaluations;
        return static_cast<long>(x) * x;
    });
    CHECK(slowSquare(12) == 144);
    CHECK(slowSquare(12) == 144);
    CHECK(slowSquare(3) == 9);
    CHECK(evaluations == 2);
    auto copy = slowSquare; // copies share the cache
    CHECK(copy(12) == 144);
    CHECK(evaluations == 2);
}

TEST_CASE("memoize turns exponential recursion linear", "[decorator][function]") {
    auto calls = std::make_shared<std::size_t>(0);
    std::function<std::uint64_t(int)> fib;
    fib = memoize<std::uint64_t, int>(withCallCounter(
        [&fib](int n) -> std::uint64_t {
            return n < 2 ? static_cast<std::uint64_t>(n) : fib(n - 1) + fib(n - 2);
        },
        calls));
    CHECK(fib(60) == 1548008755920ULL);
    CHECK(*calls == 61);
}

TEST_CASE("memoize is safe to call concurrently", "[decorator][function][threads]") {
    auto twice = memoize<int, int>([](int x) { return 2 * x; });
    std::vector<std::thread> threads;
    std::atomic<int> wrong{0};
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([&] {
            for (int i = 0; i < 200; ++i) {
                if (twice(i % 17) != 2 * (i % 17)) {
                    ++wrong;
                }
            }
        });
    }
    for (auto& th : threads) {
        th.join();
    }
    CHECK(wrong.load() == 0);
}
