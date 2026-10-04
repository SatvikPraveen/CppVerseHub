#include "modern/ConstexprProgramming.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <cmath>
#include <sstream>

using namespace CppVerseHub::Modern::ConstexprProgramming;
using Catch::Approx;

TEST_CASE("power handles positive, zero and negative exponents", "[modern][constexpr]") {
    STATIC_CHECK(power(3.0, 4) == 81.0);
    CHECK(power(2.0, -3) == Approx(0.125));
    CHECK(power(-2.0, 3) == Approx(-8.0));
    CHECK(power(1.5, 0) == 1.0);
}

TEST_CASE("factorial is exact up to 20 and saturates beyond", "[modern][constexpr]") {
    STATIC_CHECK(factorial(5) == 120);
    CHECK(factorial(20) == 2432902008176640000ULL);
    CHECK(factorial(21) == std::numeric_limits<std::uint64_t>::max());
    STATIC_CHECK(compileTimeFactorial(7) == 5040);
}

TEST_CASE("sqrtNewton matches std::sqrt", "[modern][constexpr]") {
    const double x = GENERATE(0.0, 1e-6, 0.5, 2.0, 10.0, 12345.678, 1e20);
    CHECK(sqrtNewton(x) == Approx(std::sqrt(x)).epsilon(1e-12));
    CHECK(adaptiveSqrt(x) == Approx(std::sqrt(x)));
}

TEST_CASE("sqrtNewton of a negative number is NaN", "[modern][constexpr]") {
    CHECK(std::isnan(sqrtNewton(-4.0)));
}

TEST_CASE("sinTaylor matches std::sin across several periods", "[modern][constexpr]") {
    const double x = GENERATE(-10.0, -3.0, -0.5, 0.0, 0.5, 1.0, 3.0, 7.5, 20.0);
    CHECK(sinTaylor(x) == Approx(std::sin(x)).margin(1e-9));
}

TEST_CASE("gcd, lcm and primality", "[modern][constexpr]") {
    CHECK(gcd(0, 7) == 7);
    CHECK(gcd(270, 192) == 6);
    CHECK(lcm(21, 6) == 42);
    CHECK(lcm(-4, 6) == 12);
    CHECK(isPrime(7919));
    CHECK_FALSE(isPrime(7917));
    CHECK_FALSE(isPrime(0));
}

TEST_CASE("compile-time Fibonacci and prime tables", "[modern][constexpr]") {
    STATIC_CHECK(compileTimeFibonacci(1) == 1);
    for (std::size_t i = 2; i < FIBONACCI_SEQUENCE.size(); ++i) {
        CHECK(FIBONACCI_SEQUENCE[i] == FIBONACCI_SEQUENCE[i - 1] + FIBONACCI_SEQUENCE[i - 2]);
    }
    for (auto p : FIRST_PRIMES) {
        CHECK(isPrime(p));
    }
    constexpr auto primes = generatePrimes<25>();
    CHECK(primes.back() == 97);
}

TEST_CASE("string utilities at compile time", "[modern][constexpr]") {
    STATIC_CHECK(fnv1a("CppVerseHub") == GAME_NAME_HASH);
    CHECK(fnv1a("abc") != fnv1a("acb"));
    CHECK(countChar("banana", 'a') == 3);
    CHECK(toUpper('z') == 'Z');
    CHECK(isAlpha('Q'));
    CHECK_FALSE(isDigit('x'));
}

TEST_CASE("Caesar cipher round-trips for any shift", "[modern][constexpr]") {
    const int shift = GENERATE(-27, -3, 0, 1, 13, 25, 52, 100);
    for (char c : std::string_view("Hello, World xyz ABC!")) {
        CHECK(caesarDecode(caesarEncode(c, shift), shift) == c);
    }
    constexpr auto rot13 = caesarEncodeString("Hello", 13);
    CHECK(std::string_view(rot13.data()) == "Uryyb");
}

TEST_CASE("FixedString and NamedTag work as NTTPs", "[modern][constexpr]") {
    constexpr FixedString fs("probe");
    STATIC_CHECK(fs.size() == 5);
    CHECK(fs.view() == "probe");
    CHECK(NamedTag<"Mission">::name == "Mission");
    CHECK(NamedTag<"Mission">::id == fnv1a("Mission"));
}

TEST_CASE("array algorithms", "[modern][constexpr]") {
    constexpr std::array<int, 6> data{5, -1, 9, 3, 3, 0};
    constexpr auto sorted = sortedCopy(data);
    STATIC_CHECK(sorted == std::array<int, 6>{-1, 0, 3, 3, 5, 9});
    CHECK(arraySum(data) == 19);
    CHECK(arrayMax(data) == 9);
    CHECK(arrayMin(data) == -1);
    CHECK(binarySearch(sorted, 9) == 5);
    CHECK(binarySearch(sorted, -1) == 0);
    CHECK(binarySearch(sorted, 4) == sorted.size());
    CHECK(binarySearch(std::array<int, 0>{}, 1) == 0);
}

TEST_CASE("makeTable builds lookup tables", "[modern][constexpr]") {
    constexpr auto cubes = makeTable<5>([](std::size_t i) { return static_cast<long>(i * i * i); });
    STATIC_CHECK(cubes[4] == 64);
    CHECK(SQUARES_TABLE[7] == 49);
    CHECK(SINE_TABLE[0] == Approx(0.0).margin(1e-12));
    CHECK(SINE_TABLE[8] == Approx(0.0).margin(1e-9));
    CHECK(SINE_TABLE[2] == Approx(std::sqrt(0.5)));
}

TEST_CASE("constexpr vector computation agrees with the closed form", "[modern][constexpr]") {
    const int n = GENERATE(0, 1, 5, 100);
    const auto expected = static_cast<std::int64_t>(n) * (n + 1) * (2 * n + 1) / 6;
    CHECK(sumOfSquaresViaVector(n) == expected);
}

TEST_CASE("physics helpers", "[modern][constexpr]") {
    CHECK(EARTH_ESCAPE_VELOCITY == Approx(11186.0).epsilon(1e-3));
    CHECK(escapeVelocity(EARTH_MASS, EARTH_RADIUS) ==
          Approx(std::sqrt(2.0) * orbitalVelocity(EARTH_MASS, EARTH_RADIUS)));
    CHECK(distance3d(1, 2, 3, 4, 6, 3) == Approx(5.0));
    CHECK(sphereVolume(1.0) == Approx(4.0 / 3.0 * PI));
    CHECK(sphereSurfaceArea(2.0) == Approx(16.0 * PI));
}

TEST_CASE("solar-system table and fleet validation", "[modern][constexpr]") {
    CHECK(countHabitable(SOLAR_SYSTEM) == 1);
    CHECK(countOfType(SOLAR_SYSTEM, PlanetType::IceGiant) == 2);
    CHECK(countOfType(SOLAR_SYSTEM, PlanetType::DwarfPlanet) == 0);
    CHECK(SOLAR_SYSTEM[4].surfaceGravity() > SOLAR_SYSTEM[2].surfaceGravity());
    CHECK(totalMass(SOLAR_SYSTEM) == Approx(2.6673e27).epsilon(1e-4));

    constexpr ConstexprFleet low{1, 3, 50.0, MissionType::Trade};
    CHECK(low.isOperational());
    CHECK(low.maxRange() == Approx(75.0));
    CHECK_FALSE(validateFleetConfiguration(std::array{low}));    // too few ships
    CHECK(validateFleetConfiguration(std::array{low}, 3, 50.0)); // custom thresholds
    CHECK_FALSE(validateFleetConfiguration(std::array<ConstexprFleet, 0>{}));
    CHECK_FALSE(ConstexprFleet{2, 0, 90.0, MissionType::Rescue}.isOperational());
}

TEST_CASE("variadic metaprogramming helpers", "[modern][constexpr]") {
    STATIC_CHECK(countTypes<>() == 0);
    STATIC_CHECK(containsType<double, int, double>());
    STATIC_CHECK(indexOfType<int, int, int>() == 0);
    STATIC_CHECK(indexOfType<long, int, short>() == 2);
    STATIC_CHECK(multiplyValues<>() == 1);
    STATIC_CHECK(sumValues<10, -3>() == 7);
}

TEST_CASE("demonstrateConstexpr output", "[modern][constexpr]") {
    std::ostringstream os;
    demonstrateConstexpr(os);
    CHECK(os.str().find("consteval 12! = 479001600") != std::string::npos);
    CHECK(os.str().find("Habitable planets: 1") != std::string::npos);
}
