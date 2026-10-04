#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <sstream>
#include <type_traits>
#include <unordered_set>

#include "core/Identifiers.hpp"
#include "core/Vector3D.hpp"

using namespace CppVerseHub::Core;
using Catch::Approx;

TEST_CASE("Vector3D arithmetic is usable in constant expressions", "[core][vector]") {
    constexpr Vector3D a{1.0, 2.0, 3.0};
    constexpr Vector3D b{-4.0, 0.5, 2.0};
    static_assert(a + b == Vector3D{-3.0, 2.5, 5.0});
    static_assert(a - b == Vector3D{5.0, 1.5, 1.0});
    static_assert(a * 2.0 == Vector3D{2.0, 4.0, 6.0});
    static_assert(2.0 * a == a * 2.0);
    static_assert(a / 2.0 == Vector3D{0.5, 1.0, 1.5});
    static_assert(-a == Vector3D{-1.0, -2.0, -3.0});
    static_assert(+a == a);
    static_assert(a.dot(b) == -4.0 + 1.0 + 6.0);
    static_assert(Vector3D::unitX().cross(Vector3D::unitY()) == Vector3D::unitZ());
    static_assert(a.lengthSquared() == 14.0);
    static_assert(Vector3D::lerp(a, b, 0.0) == a && Vector3D::lerp(a, b, 1.0) == b);
    static_assert(std::is_trivially_copyable_v<Vector3D>);
    SUCCEED();
}

TEST_CASE("Vector3D compound assignment mutates in place", "[core][vector]") {
    Vector3D v{1.0, 1.0, 1.0};
    v += {1.0, 2.0, 3.0};
    REQUIRE(v == Vector3D{2.0, 3.0, 4.0});
    v -= {2.0, 2.0, 2.0};
    REQUIRE(v == Vector3D{0.0, 1.0, 2.0});
    v *= 3.0;
    REQUIRE(v == Vector3D{0.0, 3.0, 6.0});
    v /= 3.0;
    REQUIRE(v == Vector3D{0.0, 1.0, 2.0});
}

TEST_CASE("Vector3D length, distance and normalisation", "[core][vector]") {
    const Vector3D v{3.0, 4.0, 12.0};
    REQUIRE(v.length() == 13.0);
    REQUIRE(v.distanceTo(Vector3D::zero()) == 13.0);
    REQUIRE(Vector3D{1, 1, 1}.distanceSquaredTo({2, 3, 4}) == 14.0);
    REQUIRE(v.normalized().length() == Approx(1.0));
    REQUIRE(v.normalized().approxEquals(v / 13.0));
    REQUIRE(Vector3D::zero().normalized() == Vector3D::zero());
}

TEST_CASE("Vector3D cross product is orthogonal and anti-commutative", "[core][vector]") {
    const auto x = GENERATE(-2.0, 0.5, 3.0);
    const Vector3D a{x, 2.0, -1.0};
    const Vector3D b{1.0, -x, 4.0};
    const Vector3D c = a.cross(b);
    REQUIRE(c.dot(a) == Approx(0.0).margin(1e-12));
    REQUIRE(c.dot(b) == Approx(0.0).margin(1e-12));
    REQUIRE(b.cross(a) == -c);
}

TEST_CASE("Vector3D approxEquals and streaming", "[core][vector]") {
    REQUIRE(Vector3D{1.0, 2.0, 3.0}.approxEquals({1.0 + 1e-12, 2.0, 3.0 - 1e-12}));
    REQUIRE_FALSE(Vector3D{1.0, 2.0, 3.0}.approxEquals({1.1, 2.0, 3.0}, 0.05));
    std::ostringstream os;
    os << Vector3D{1.5, -2.0, 0.0};
    REQUIRE(os.str() == "(1.5, -2, 0)");
}

TEST_CASE("StrongId is a distinct, ordered, hashable type", "[core][ids]") {
    static_assert(!std::is_convertible_v<EntityId, MissionId>);
    static_assert(!std::is_convertible_v<std::uint64_t, EntityId>);
    constexpr EntityId a{1};
    constexpr EntityId b{2};
    static_assert(a < b && a != b && a == EntityId{1});
    static_assert(!EntityId{}.isValid() && a.isValid());
    std::unordered_set<EntityId> set{a, b, EntityId{1}};
    REQUIRE(set.size() == 2);
    std::ostringstream os;
    os << MissionId{42};
    REQUIRE(os.str() == "#42");
}
