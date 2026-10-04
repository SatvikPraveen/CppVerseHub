// Tests for utils/MathUtils.hpp.
#include "utils/MathUtils.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <cmath>
#include <sstream>
#include <stdexcept>
#include <vector>

using namespace CppVerseHub::Utils::Math;
using Catch::Approx;

TEST_CASE("approxEqual and angle conversions", "[utils][math]") {
    static_assert(approxEqual(0.1 + 0.2, 0.3));
    static_assert(!approxEqual(1.0, 1.001));
    CHECK(approxEqual(1e12, 1e12 + 1.0));
    CHECK(toDegrees(Constants::kPi) == Approx(180.0));
    CHECK(toRadians(90.0) == Approx(Constants::kPi / 2.0));
}

TEST_CASE("Vector arithmetic is constexpr", "[utils][math][vector]") {
    constexpr Vec3 a{1.0, 2.0, 3.0};
    constexpr Vec3 b{4.0, 5.0, 6.0};
    static_assert(a + b == Vec3{5.0, 7.0, 9.0});
    static_assert(b - a == Vec3{3.0, 3.0, 3.0});
    static_assert(a * 2.0 == Vec3{2.0, 4.0, 6.0});
    static_assert(2.0 * a == a * 2.0);
    static_assert(-a == Vec3{-1.0, -2.0, -3.0});
    static_assert(a.dot(b) == 32.0);
    static_assert(Vec3{1.0, 0.0, 0.0}.cross(Vec3{0.0, 1.0, 0.0}) == Vec3{0.0, 0.0, 1.0});
    static_assert(Vec2{1.0, 0.0}.cross(Vec2{0.0, 1.0}) == 1.0);
    static_assert(Vec3::size() == 3);
    CHECK((b / 2.0) == Vec3{2.0, 2.5, 3.0});
}

TEST_CASE("Vector length, normalisation and distance", "[utils][math][vector]") {
    const Vec3 v{3.0, 4.0, 12.0};
    CHECK(v.length() == Approx(13.0));
    CHECK(v.normalized().length() == Approx(1.0));
    CHECK(Vec3{}.normalized() == Vec3{});
    CHECK(Vec2{0.0, 0.0}.distanceTo(Vec2{3.0, 4.0}) == Approx(5.0));
    const auto mid = lerp(Vec2{0.0, 0.0}, Vec2{10.0, 20.0}, 0.25);
    CHECK(mid == Vec2{2.5, 5.0});
    double sum = 0.0;
    for (const double c : v) {
        sum += c;
    }
    CHECK(sum == 19.0);
    std::ostringstream oss;
    oss << Vec2{1.5, -2.0};
    CHECK(oss.str() == "(1.5, -2)");
}

TEST_CASE("Rotations preserve length and match known results", "[utils][math][vector]") {
    const Vec3 x{1.0, 0.0, 0.0};
    const Vec3 r = rotateZ(x, Constants::kPi / 2.0);
    CHECK(r.x() == Approx(0.0).margin(1e-12));
    CHECK(r.y() == Approx(1.0));
    const Vec3 v{1.0, 2.0, 3.0};
    const Vec3 axisRot = rotateAroundAxis(v, Vec3{0.0, 0.0, 5.0}, 0.7);
    const Vec3 zRot = rotateZ(v, 0.7);
    CHECK(axisRot.x() == Approx(zRot.x()));
    CHECK(axisRot.y() == Approx(zRot.y()));
    CHECK(axisRot.z() == Approx(zRot.z()));
    CHECK(rotateAroundAxis(v, Vec3{1.0, 1.0, 1.0}, 1.234).length() == Approx(v.length()));
    CHECK(rotateAroundAxis(v, Vec3{}, 1.0) == v);
}

TEST_CASE("Matrix construction, access and transpose", "[utils][math][matrix]") {
    const Matrix m{{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}};
    CHECK(m.rows() == 2);
    CHECK(m.cols() == 3);
    CHECK_FALSE(m.isSquare());
    CHECK(m(1, 2) == 6.0);
    CHECK(m.at(0, 1) == 2.0);
    CHECK_THROWS_AS(m.at(2, 0), std::out_of_range);
    const Matrix t = m.transposed();
    CHECK(t.rows() == 3);
    CHECK(t(2, 1) == 6.0);
    CHECK(t.transposed() == m);
    CHECK_THROWS_AS((Matrix{{1.0, 2.0}, {3.0}}), std::invalid_argument);
    CHECK(Matrix::identity(3).trace() == 3.0);
}

TEST_CASE("Matrix products and arithmetic", "[utils][math][matrix]") {
    const Matrix a{{1.0, 2.0}, {3.0, 4.0}};
    const Matrix b{{5.0, 6.0}, {7.0, 8.0}};
    CHECK(a * b == Matrix{{19.0, 22.0}, {43.0, 50.0}});
    CHECK(a * Matrix::identity(2) == a);
    CHECK(a + b == Matrix{{6.0, 8.0}, {10.0, 12.0}});
    CHECK(b - a == Matrix{{4.0, 4.0}, {4.0, 4.0}});
    CHECK(a * 2.0 == Matrix{{2.0, 4.0}, {6.0, 8.0}});
    const std::vector<double> v{1.0, 1.0};
    CHECK(a * std::span<const double>{v} == std::vector<double>{3.0, 7.0});
    CHECK_THROWS_AS(a * Matrix(3, 3), std::invalid_argument);
    CHECK_THROWS_AS(a + Matrix(3, 3), std::invalid_argument);
    std::ostringstream oss;
    oss << a;
    CHECK(oss.str() == "[1, 2]\n[3, 4]\n");
}

TEST_CASE("Matrix determinant, inverse and solve", "[utils][math][matrix]") {
    const Matrix m{{2.0, -1.0, 0.0}, {-1.0, 2.0, -1.0}, {0.0, -1.0, 2.0}};
    CHECK(m.determinant() == Approx(4.0));
    const auto inv = m.inverse();
    REQUIRE(inv.has_value());
    CHECK((m * *inv).approxEquals(Matrix::identity(3), 1e-12));
    const std::vector<double> rhs{1.0, 0.0, 1.0};
    const auto x = m.solve(rhs);
    REQUIRE(x.has_value());
    CHECK((*x)[0] == Approx(1.0));
    CHECK((*x)[1] == Approx(1.0));
    CHECK((*x)[2] == Approx(1.0));

    const Matrix singular{{1.0, 2.0}, {2.0, 4.0}};
    CHECK(singular.determinant() == Approx(0.0).margin(1e-12));
    CHECK_FALSE(singular.inverse().has_value());
    CHECK_FALSE(singular.solve(std::vector<double>{1.0, 2.0}).has_value());
    CHECK_THROWS_AS(Matrix(2, 3).determinant(), std::logic_error);
    CHECK_THROWS_AS(m.solve(std::vector<double>{1.0}), std::invalid_argument);

    // A permutation matrix needs pivoting and has determinant -1.
    const Matrix perm{{0.0, 1.0}, {1.0, 0.0}};
    CHECK(perm.determinant() == Approx(-1.0));
}

TEST_CASE("Interpolation helpers", "[utils][math]") {
    using namespace Interpolation;
    static_assert(lerp(0.0, 10.0, 0.5) == 5.0);
    static_assert(inverseLerp(0.0, 10.0, 2.5) == 0.25);
    static_assert(remap(5.0, 0.0, 10.0, 100.0, 200.0) == 150.0);
    static_assert(smoothstep(0.0, 1.0, -1.0) == 0.0 && smoothstep(0.0, 1.0, 2.0) == 1.0);
    CHECK(smoothstep(0.0, 1.0, 0.5) == Approx(0.5));
    CHECK(cubicBezier(0.0, 1.0, 2.0, 3.0, 0.5) == Approx(1.5));
    CHECK(cubicBezier(1.0, 5.0, -3.0, 7.0, 0.0) == 1.0);
    CHECK(cubicBezier(1.0, 5.0, -3.0, 7.0, 1.0) == 7.0);
    CHECK(catmullRom(0.0, 1.0, 2.0, 3.0, 0.0) == Approx(1.0));
    CHECK(catmullRom(0.0, 1.0, 2.0, 3.0, 1.0) == Approx(2.0));
    CHECK(catmullRom(0.0, 1.0, 2.0, 3.0, 0.5) == Approx(1.5));
    CHECK(inverseLerp(3.0, 3.0, 5.0) == 0.0);
}

TEST_CASE("Number theory functions", "[utils][math]") {
    using namespace NumberTheory;
    static_assert(!isPrime(0) && !isPrime(1) && isPrime(2) && isPrime(3) && !isPrime(9) && isPrime(97));
    static_assert(isPrime(2'147'483'647ULL));
    static_assert(factorial(0) == 1ULL && factorial(20) == 2'432'902'008'176'640'000ULL);
    static_assert(!factorial(21).has_value());
    static_assert(binomial(5, 2) == 10 && binomial(52, 5) == 2'598'960 && binomial(3, 5) == 0);
    static_assert(fibonacci(0) == 0 && fibonacci(1) == 1 && fibonacci(10) == 55);
    static_assert(fibonacci(93) == 12'200'160'415'121'876'738ULL);
    static_assert(modPow(2, 10, 1000) == 24 && modPow(5, 0, 7) == 1 && modPow(3, 4, 1) == 0);
    CHECK(primesUpTo(30) == std::vector<std::uint64_t>{2, 3, 5, 7, 11, 13, 17, 19, 23, 29});
    CHECK(primesUpTo(1).empty());
    CHECK(primesUpTo(10'000).size() == 1229);
}

TEST_CASE("Numerical integration and root finding", "[utils][math]") {
    using namespace Numerical;
    CHECK(integrateSimpson([](double x) { return std::sin(x); }, 0.0, Constants::kPi) == Approx(2.0).epsilon(1e-10));
    CHECK(integrateSimpson([](double x) { return x * x * x; }, 0.0, 2.0, 3) == Approx(4.0)); // exact for cubics
    const auto sqrt2 = newtonRaphson([](double x) { return x * x - 2.0; }, [](double x) { return 2.0 * x; }, 1.0);
    REQUIRE(sqrt2.has_value());
    CHECK(*sqrt2 == Approx(std::sqrt(2.0)).epsilon(1e-14));
    CHECK_FALSE(newtonRaphson([](double x) { return x * x + 1.0; }, [](double) { return 0.0; }, 1.0).has_value());
    const auto root = bisection([](double x) { return std::cos(x) - x; }, 0.0, 1.0);
    REQUIRE(root.has_value());
    CHECK(*root == Approx(0.7390851332).epsilon(1e-9));
    CHECK_FALSE(bisection([](double x) { return x * x + 1.0; }, -1.0, 1.0).has_value());
    CHECK(bisection([](double x) { return x; }, 0.0, 1.0) == 0.0);
}

TEST_CASE("Descriptive statistics", "[utils][math][stats]") {
    using namespace Statistics;
    const std::vector<double> data{2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0};
    CHECK(mean(data) == Approx(5.0));
    CHECK(variance(data, false) == Approx(4.0));
    CHECK(standardDeviation(data, false) == Approx(2.0));
    CHECK(variance(data, true) == Approx(32.0 / 7.0));
    CHECK(median(data) == Approx(4.5));
    CHECK(median(std::vector<double>{3.0, 1.0, 2.0}) == 2.0);
    CHECK(percentile(data, 0.0) == 2.0);
    CHECK(percentile(data, 100.0) == 9.0);
    CHECK(percentile(std::vector<double>{1.0, 2.0, 3.0, 4.0}, 25.0) == Approx(1.75));
    CHECK_THROWS_AS(mean(std::vector<double>{}), std::invalid_argument);
    CHECK_THROWS_AS(variance(std::vector<double>{1.0}, true), std::invalid_argument);
    CHECK_THROWS_AS(percentile(data, 101.0), std::invalid_argument);
}

TEST_CASE("Variance is numerically stable for large offsets", "[utils][math][stats]") {
    const std::vector<double> data{1e9 + 4.0, 1e9 + 7.0, 1e9 + 13.0, 1e9 + 16.0};
    CHECK(Statistics::variance(data) == Approx(30.0).epsilon(1e-9));
}

TEST_CASE("Correlation and linear regression", "[utils][math][stats]") {
    using namespace Statistics;
    const std::vector<double> x{1.0, 2.0, 3.0, 4.0, 5.0};
    const std::vector<double> y{3.0, 5.0, 7.0, 9.0, 11.0};
    CHECK(correlation(x, y) == Approx(1.0));
    const std::vector<double> neg{5.0, 4.0, 3.0, 2.0, 1.0};
    CHECK(correlation(x, neg) == Approx(-1.0));
    const auto fit = linearRegression(x, y);
    CHECK(fit.slope == Approx(2.0));
    CHECK(fit.intercept == Approx(1.0));
    CHECK(fit.rSquared == Approx(1.0));
    CHECK(correlation(x, std::vector<double>(5, 2.0)) == 0.0);
    CHECK_THROWS_AS(linearRegression(std::vector<double>(3, 1.0), std::vector<double>{1.0, 2.0, 3.0}),
                    std::invalid_argument);
    CHECK_THROWS_AS(correlation(x, std::vector<double>{1.0}), std::invalid_argument);
}

TEST_CASE("RandomGenerator is reproducible and respects ranges", "[utils][math][random]") {
    RandomGenerator a{123};
    RandomGenerator b{123};
    for (int i = 0; i < 100; ++i) {
        const auto va = a.uniformInt(-5, 5);
        CHECK(va == b.uniformInt(-5, 5));
        CHECK((va >= -5 && va <= 5));
    }
    RandomGenerator rng{7};
    double sum = 0.0;
    for (int i = 0; i < 2000; ++i) {
        const double r = rng.uniformReal(2.0, 3.0);
        CHECK((r >= 2.0 && r < 3.0));
        sum += rng.normal(10.0, 1.0);
    }
    CHECK(sum / 2000.0 == Approx(10.0).margin(0.1));
    CHECK_FALSE(rng.chance(0.0));
    CHECK(rng.chance(1.0));
    CHECK(rng.unitVector().length() == Approx(1.0));
    std::vector<int> values{1, 2, 3, 4, 5, 6};
    rng.shuffle(values);
    std::sort(values.begin(), values.end());
    CHECK(values == std::vector<int>{1, 2, 3, 4, 5, 6});
}

TEST_CASE("PerlinNoise is deterministic, bounded and zero on the lattice", "[utils][math][noise]") {
    const PerlinNoise n1{99};
    const PerlinNoise n2{99};
    const PerlinNoise other{100};
    bool anyDifferent = false;
    for (int i = 0; i < 200; ++i) {
        const double x = i * 0.137;
        const double y = i * 0.071;
        const double v = n1.noise(x, y, 0.5);
        CHECK(v == n2.noise(x, y, 0.5));
        CHECK((v >= -1.0 && v <= 1.0));
        anyDifferent = anyDifferent || v != other.noise(x, y, 0.5);
        const double f = n1.fractal(x, y, 0.0, 5);
        CHECK((f >= -1.0 && f <= 1.0));
    }
    CHECK(anyDifferent);
    CHECK(n1.noise(3.0, 4.0, 5.0) == 0.0);
    CHECK(n1.noise(-2.0, 7.0, 0.0) == 0.0);
}

TEST_CASE("Polygon area and point containment", "[utils][math][geometry]") {
    using namespace Geometry;
    const std::vector<Vec2> square{{0.0, 0.0}, {4.0, 0.0}, {4.0, 4.0}, {0.0, 4.0}};
    CHECK(polygonArea(square) == Approx(16.0));
    const std::vector<Vec2> clockwise(square.rbegin(), square.rend());
    CHECK(polygonArea(clockwise) == Approx(-16.0));
    static_assert(signedTriangleArea(Vec2{0.0, 0.0}, Vec2{2.0, 0.0}, Vec2{0.0, 2.0}) == 2.0);
    CHECK(pointInPolygon(Vec2{2.0, 2.0}, square));
    CHECK_FALSE(pointInPolygon(Vec2{5.0, 2.0}, square));
    const std::vector<Vec2> concave{{0.0, 0.0}, {4.0, 0.0}, {4.0, 4.0}, {2.0, 1.0}, {0.0, 4.0}};
    CHECK_FALSE(pointInPolygon(Vec2{2.0, 3.0}, concave));
    CHECK(pointInPolygon(Vec2{1.0, 1.0}, concave));
    CHECK(polygonArea(std::vector<Vec2>{{0.0, 0.0}, {1.0, 1.0}}) == 0.0);
}

TEST_CASE("Convex hull via monotone chain", "[utils][math][geometry]") {
    std::vector<Vec2> points{{0.0, 0.0}, {2.0, 0.0}, {1.0, 1.0}, {2.0, 2.0}, {0.0, 2.0}, {1.0, 0.0}, {0.0, 0.0}};
    const auto hull = Geometry::convexHull(points);
    CHECK(hull == std::vector<Vec2>{{0.0, 0.0}, {2.0, 0.0}, {2.0, 2.0}, {0.0, 2.0}});
    CHECK(Geometry::polygonArea(hull) == Approx(4.0));
    CHECK(Geometry::convexHull({{1.0, 1.0}, {1.0, 1.0}}).size() == 1);
}

TEST_CASE("Segment intersection and point-segment distance", "[utils][math][geometry]") {
    using namespace Geometry;
    CHECK(segmentsIntersect(Vec2{0.0, 0.0}, Vec2{2.0, 2.0}, Vec2{0.0, 2.0}, Vec2{2.0, 0.0}));
    CHECK_FALSE(segmentsIntersect(Vec2{0.0, 0.0}, Vec2{1.0, 0.0}, Vec2{0.0, 1.0}, Vec2{1.0, 1.0}));
    CHECK(segmentsIntersect(Vec2{0.0, 0.0}, Vec2{2.0, 0.0}, Vec2{1.0, 0.0}, Vec2{3.0, 0.0})); // collinear overlap
    CHECK(segmentsIntersect(Vec2{0.0, 0.0}, Vec2{1.0, 0.0}, Vec2{1.0, 0.0}, Vec2{1.0, 5.0}));  // touching
    CHECK(pointSegmentDistance(Vec2{1.0, 1.0}, Vec2{0.0, 0.0}, Vec2{2.0, 0.0}) == Approx(1.0));
    CHECK(pointSegmentDistance(Vec2{3.0, 4.0}, Vec2{0.0, 0.0}, Vec2{0.0, 0.0}) == Approx(5.0));
    CHECK(pointSegmentDistance(Vec2{-3.0, 4.0}, Vec2{0.0, 0.0}, Vec2{1.0, 0.0}) == Approx(5.0));
}

TEST_CASE("Orbital mechanics formulas", "[utils][math][space]") {
    using namespace Space;
    const double r = Constants::kEarthRadius + 400e3;
    CHECK(circularVelocity(r) == Approx(7672.6).epsilon(1e-3));
    CHECK(escapeVelocity(r) == Approx(std::sqrt(2.0) * circularVelocity(r)));
    CHECK(orbitalPeriod(42'164e3) == Approx(86'164.0).epsilon(1e-3)); // sidereal day
    const auto t = hohmannTransfer(r, 42'164e3);
    CHECK(t.totalDeltaV == Approx(3'854.0).epsilon(5e-3));
    CHECK(t.transferTime / 3600.0 == Approx(5.28).epsilon(5e-3));
    const auto down = hohmannTransfer(42'164e3, r);
    CHECK(down.totalDeltaV == Approx(t.totalDeltaV));
    CHECK_THROWS_AS(hohmannTransfer(-1.0, 2.0), std::invalid_argument);
}

TEST_CASE("NBodySimulator conserves energy and momentum", "[utils][math][space]") {
    Space::NBodySimulator sim{1.0};
    sim.addBody({Vec3{0.0, 0.0, 0.0}, Vec3{0.0, 0.0, 0.0}, 1.0});
    const auto planet = sim.addBody({Vec3{1.0, 0.0, 0.0}, Vec3{0.0, 1.0, 0.0}, 1e-6});
    const double e0 = sim.totalEnergy();
    const Vec3 p0 = sim.totalMomentum();
    const double dt = Constants::kTau / 2000.0;
    for (int i = 0; i < 2000; ++i) {
        sim.step(dt);
    }
    CHECK(std::abs((sim.totalEnergy() - e0) / e0) < 1e-6);
    const Vec3 p1 = sim.totalMomentum();
    CHECK((p1 - p0).length() < 1e-12);
    // After ~one period the planet is back near its starting point.
    CHECK(sim.bodies()[planet].position.distanceTo(Vec3{1.0, 0.0, 0.0}) < 0.01);
    CHECK(sim.bodies().size() == 2);
}
