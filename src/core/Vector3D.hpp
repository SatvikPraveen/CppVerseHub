/**
 * @file Vector3D.hpp
 * @brief A `constexpr` 3D vector value type used for all spatial quantities in the simulation.
 *
 * Demonstrates how a small regular type should look in modern C++: aggregate-like value semantics,
 * a complete and consistent operator set (arithmetic, compound assignment, comparison), `constexpr`
 * evaluation wherever the language allows (everything except functions that need `std::sqrt`, which
 * only becomes `constexpr` in C++26), and `noexcept` on every operation that cannot fail.
 */
#pragma once

#include <cmath>
#include <ostream>

namespace CppVerseHub::Core {

/**
 * @brief Immutable-by-convention 3D vector of doubles with full value semantics.
 *
 * All arithmetic is component-wise IEEE-754 double arithmetic, so results are deterministic for a
 * given compiler and floating-point configuration.
 */
struct Vector3D {
    double x{0.0}; ///< X component.
    double y{0.0}; ///< Y component.
    double z{0.0}; ///< Z component.

    /// @brief Zero vector.
    constexpr Vector3D() noexcept = default;

    /**
     * @brief Component-wise constructor.
     * @param xValue X component.
     * @param yValue Y component.
     * @param zValue Z component.
     */
    constexpr Vector3D(double xValue, double yValue, double zValue) noexcept
        : x(xValue), y(yValue), z(zValue) {}

    /// @brief The zero vector. @return (0,0,0).
    [[nodiscard]] static constexpr Vector3D zero() noexcept { return {}; }
    /// @brief Unit vector along X. @return (1,0,0).
    [[nodiscard]] static constexpr Vector3D unitX() noexcept { return {1.0, 0.0, 0.0}; }
    /// @brief Unit vector along Y. @return (0,1,0).
    [[nodiscard]] static constexpr Vector3D unitY() noexcept { return {0.0, 1.0, 0.0}; }
    /// @brief Unit vector along Z. @return (0,0,1).
    [[nodiscard]] static constexpr Vector3D unitZ() noexcept { return {0.0, 0.0, 1.0}; }

    /// @brief Component-wise addition. @param o Other vector. @return Reference to `*this`.
    constexpr Vector3D& operator+=(const Vector3D& o) noexcept {
        x += o.x;
        y += o.y;
        z += o.z;
        return *this;
    }
    /// @brief Component-wise subtraction. @param o Other vector. @return Reference to `*this`.
    constexpr Vector3D& operator-=(const Vector3D& o) noexcept {
        x -= o.x;
        y -= o.y;
        z -= o.z;
        return *this;
    }
    /// @brief Scalar multiplication. @param s Scale factor. @return Reference to `*this`.
    constexpr Vector3D& operator*=(double s) noexcept {
        x *= s;
        y *= s;
        z *= s;
        return *this;
    }
    /// @brief Scalar division (IEEE semantics for s == 0). @param s Divisor. @return Reference to `*this`.
    constexpr Vector3D& operator/=(double s) noexcept {
        x /= s;
        y /= s;
        z /= s;
        return *this;
    }

    /// @brief Sum. @param a Lhs. @param b Rhs. @return a + b.
    [[nodiscard]] friend constexpr Vector3D operator+(Vector3D a, const Vector3D& b) noexcept { return a += b; }
    /// @brief Difference. @param a Lhs. @param b Rhs. @return a - b.
    [[nodiscard]] friend constexpr Vector3D operator-(Vector3D a, const Vector3D& b) noexcept { return a -= b; }
    /// @brief Scale. @param a Vector. @param s Scalar. @return a * s.
    [[nodiscard]] friend constexpr Vector3D operator*(Vector3D a, double s) noexcept { return a *= s; }
    /// @brief Scale. @param s Scalar. @param a Vector. @return s * a.
    [[nodiscard]] friend constexpr Vector3D operator*(double s, Vector3D a) noexcept { return a *= s; }
    /// @brief Divide. @param a Vector. @param s Scalar. @return a / s.
    [[nodiscard]] friend constexpr Vector3D operator/(Vector3D a, double s) noexcept { return a /= s; }
    /// @brief Negation. @param a Vector. @return -a.
    [[nodiscard]] friend constexpr Vector3D operator-(const Vector3D& a) noexcept { return {-a.x, -a.y, -a.z}; }
    /// @brief Unary plus. @param a Vector. @return a.
    [[nodiscard]] friend constexpr Vector3D operator+(const Vector3D& a) noexcept { return a; }

    /// @brief Exact component-wise equality. @param a Lhs. @param b Rhs. @return true if all equal.
    [[nodiscard]] friend constexpr bool operator==(const Vector3D& a, const Vector3D& b) noexcept = default;

    /// @brief Dot product. @param o Other vector. @return this . o.
    [[nodiscard]] constexpr double dot(const Vector3D& o) const noexcept { return x * o.x + y * o.y + z * o.z; }

    /// @brief Cross product. @param o Other vector. @return this x o.
    [[nodiscard]] constexpr Vector3D cross(const Vector3D& o) const noexcept {
        return {y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x};
    }

    /// @brief Squared Euclidean length (constexpr, no sqrt). @return |v|^2.
    [[nodiscard]] constexpr double lengthSquared() const noexcept { return dot(*this); }

    /// @brief Euclidean length. @return |v|.
    [[nodiscard]] double length() const noexcept { return std::sqrt(lengthSquared()); }

    /// @brief Squared distance to another point. @param o Other point. @return |this - o|^2.
    [[nodiscard]] constexpr double distanceSquaredTo(const Vector3D& o) const noexcept {
        return (*this - o).lengthSquared();
    }

    /// @brief Euclidean distance to another point. @param o Other point. @return |this - o|.
    [[nodiscard]] double distanceTo(const Vector3D& o) const noexcept { return (*this - o).length(); }

    /**
     * @brief Unit vector in the same direction.
     * @return The normalised vector, or the zero vector if this vector has zero length.
     */
    [[nodiscard]] Vector3D normalized() const noexcept {
        const double len = length();
        return len > 0.0 ? *this / len : Vector3D{};
    }

    /**
     * @brief Linear interpolation.
     * @param a Start point (t = 0).
     * @param b End point (t = 1).
     * @param t Interpolation parameter (not clamped).
     * @return a + (b - a) * t.
     */
    [[nodiscard]] static constexpr Vector3D lerp(const Vector3D& a, const Vector3D& b, double t) noexcept {
        return a + (b - a) * t;
    }

    /**
     * @brief Approximate equality using an absolute per-component tolerance.
     * @param o Other vector.
     * @param epsilon Maximum allowed absolute difference per component.
     * @return true if every component differs by at most epsilon.
     */
    [[nodiscard]] constexpr bool approxEquals(const Vector3D& o, double epsilon = 1e-9) const noexcept {
        const auto close = [epsilon](double a, double b) { return (a > b ? a - b : b - a) <= epsilon; };
        return close(x, o.x) && close(y, o.y) && close(z, o.z);
    }

    /**
     * @brief Stream a vector as "(x, y, z)".
     * @param os Output stream.
     * @param v Vector to print.
     * @return The stream.
     */
    friend std::ostream& operator<<(std::ostream& os, const Vector3D& v) {
        return os << '(' << v.x << ", " << v.y << ", " << v.z << ')';
    }
};

} // namespace CppVerseHub::Core
