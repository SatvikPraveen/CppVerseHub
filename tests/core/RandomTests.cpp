#include "core/Random.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <array>
#include <random>

using namespace CppVerseHub::Core;

TEST_CASE("DeterministicRng reproduces the standard mt19937_64 sequence", "[core][random]") {
    DeterministicRng rng(5489U);
    std::mt19937_64 reference(5489U);
    for (int i = 0; i < 100; ++i) {
        REQUIRE(rng.nextU64() == reference());
    }
    // The 10000th output of a default-seeded mt19937_64 is fixed by the C++ standard.
    DeterministicRng standard(5489U);
    std::uint64_t value = 0;
    for (int i = 0; i < 10000; ++i) {
        value = standard.nextU64();
    }
    REQUIRE(value == 9981545732273789042ULL);
}

TEST_CASE("DeterministicRng derived distributions stay in range", "[core][random]") {
    DeterministicRng rng(GENERATE(1U, 2U, 99U));
    for (int i = 0; i < 2000; ++i) {
        const double u = rng.uniform01();
        REQUIRE(u >= 0.0);
        REQUIRE(u < 1.0);
        const double v = rng.uniform(-3.0, 7.0);
        REQUIRE(v >= -3.0);
        REQUIRE(v < 7.0);
        const auto k = rng.uniformInt(-5, 5);
        REQUIRE(k >= -5);
        REQUIRE(k <= 5);
        REQUIRE(rng.below(7) < 7U);
    }
    REQUIRE_FALSE(rng.chance(0.0));
    REQUIRE(rng.chance(1.0));
}

TEST_CASE("DeterministicRng uniformInt covers every value roughly uniformly", "[core][random]") {
    DeterministicRng rng(1234);
    std::array<int, 6> counts{};
    constexpr int kDraws = 60000;
    for (int i = 0; i < kDraws; ++i) {
        ++counts[static_cast<std::size_t>(rng.uniformInt(0, 5))];
    }
    for (int c : counts) {
        REQUIRE(c > 9000);
        REQUIRE(c < 11000);
    }
}

TEST_CASE("DeterministicRng state save/load resumes the exact stream", "[core][random]") {
    DeterministicRng a(77);
    for (int i = 0; i < 37; ++i) {
        static_cast<void>(a.nextU64());
    }
    DeterministicRng b(1);
    b.loadState(a.saveState(), a.seed());
    REQUIRE(a == b);
    REQUIRE(b.seed() == 77U);
    for (int i = 0; i < 50; ++i) {
        REQUIRE(a.nextU64() == b.nextU64());
    }
    REQUIRE_THROWS_AS(b.loadState("not a state", 1), SerializationException);
}

TEST_CASE("DeterministicRng rejects invalid ranges", "[core][random]") {
    DeterministicRng rng;
    REQUIRE_THROWS_AS(rng.below(0), InvalidArgumentException);
    REQUIRE_THROWS_AS(rng.uniformInt(3, 2), InvalidArgumentException);
    REQUIRE_THROWS_AS(rng.uniform(1.0, 0.0), InvalidArgumentException);
    rng.reseed(9);
    DeterministicRng fresh(9);
    REQUIRE(rng == fresh);
}
