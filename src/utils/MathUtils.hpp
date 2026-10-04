/**
 * @file MathUtils.hpp
 * @brief Numerical toolkit: constexpr vectors, dense matrices, statistics, noise, geometry and orbits.
 *
 * Demonstrates generic programming with C++20 concepts (`Vector<T, N>` constrained to
 * floating-point `T`, `cross()` only for N == 3 via a `requires` clause), heavy use of `constexpr`
 * (vector algebra, number theory and interpolation are usable at compile time), `std::span` views for
 * algorithms over contiguous data, `std::numbers` constants, numerically careful algorithms
 * (partial-pivot Gaussian elimination, Welford-free two-pass variance, Simpson integration) and a
 * velocity-Verlet N-body integrator that conserves energy well.
 */
#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <numbers>
#include <optional>
#include <ostream>
#include <random>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace CppVerseHub::Utils::Math {

// ===================================================================================================
// Constants
// ===================================================================================================

namespace Constants {
inline constexpr double kPi = std::numbers::pi;                ///< pi.
inline constexpr double kTau = 2.0 * std::numbers::pi;         ///< 2 pi.
inline constexpr double kE = std::numbers::e;                  ///< Euler's number.
inline constexpr double kDegToRad = std::numbers::pi / 180.0;  ///< Degrees -> radians factor.
inline constexpr double kRadToDeg = 180.0 / std::numbers::pi;  ///< Radians -> degrees factor.
inline constexpr double kGravitationalConstant = 6.67430e-11;  ///< G in m^3 kg^-1 s^-2.
inline constexpr double kSpeedOfLight = 299'792'458.0;         ///< c in m/s.
inline constexpr double kEarthMu = 3.986004418e14;             ///< Earth GM in m^3/s^2.
inline constexpr double kSunMu = 1.32712440018e20;             ///< Sun GM in m^3/s^2.
inline constexpr double kEarthRadius = 6.371e6;                ///< Mean Earth radius in m.
inline constexpr double kAstronomicalUnit = 149'597'870'700.0; ///< AU in m.
} // namespace Constants

/**
 * @brief Relative/absolute tolerance comparison for floating-point values.
 * @param a First value.
 * @param b Second value.
 * @param epsilon Tolerance, applied relative to the larger magnitude (and absolutely near zero).
 * @return True if approximately equal.
 */
[[nodiscard]] constexpr bool approxEqual(double a, double b, double epsilon = 1e-9) noexcept {
    const double diff = a > b ? a - b : b - a;
    const double absA = a < 0 ? -a : a;
    const double absB = b < 0 ? -b : b;
    const double scale = absA > absB ? absA : absB;
    return diff <= epsilon * (scale > 1.0 ? scale : 1.0);
}

/**
 * @brief Converts degrees to radians.
 * @param degrees Angle in degrees.
 * @return Angle in radians.
 */
[[nodiscard]] constexpr double toRadians(double degrees) noexcept {
    return degrees * Constants::kDegToRad;
}

/**
 * @brief Converts radians to degrees.
 * @param radians Angle in radians.
 * @return Angle in degrees.
 */
[[nodiscard]] constexpr double toDegrees(double radians) noexcept {
    return radians * Constants::kRadToDeg;
}

// ===================================================================================================
// Vector<T, N>
// ===================================================================================================

/**
 * @brief Fixed-size mathematical vector; an aggregate-like value type usable in constant expressions.
 * @tparam T Floating-point element type.
 * @tparam N Dimension (>= 1).
 */
template <std::floating_point T, std::size_t N>
    requires(N >= 1)
class Vector {
public:
    /// @brief Zero vector.
    constexpr Vector() noexcept = default;

    /**
     * @brief Element-wise construction: `Vector<double, 3>{1.0, 2.0, 3.0}`.
     * @param values Exactly N values convertible to T.
     */
    template <typename... Args>
        requires(sizeof...(Args) == N) && (std::convertible_to<Args, T> && ...)
    // NOLINTNEXTLINE(google-explicit-constructor): explicit exactly when it is a one-argument conversion
    constexpr explicit(N == 1) Vector(Args... values) noexcept : data_{static_cast<T>(values)...} {}

    /**
     * @brief Element access.
     * @param i Index (< N).
     * @return Reference to the element.
     */
    [[nodiscard]] constexpr T& operator[](std::size_t i) noexcept { return data_[i]; }
    /// @copydoc operator[]
    [[nodiscard]] constexpr const T& operator[](std::size_t i) const noexcept { return data_[i]; }

    /// @brief First component. @return x.
    [[nodiscard]] constexpr T x() const noexcept { return data_[0]; }
    /// @brief Second component. @return y.
    [[nodiscard]] constexpr T y() const noexcept
        requires(N >= 2)
    {
        return data_[1];
    }
    /// @brief Third component. @return z.
    [[nodiscard]] constexpr T z() const noexcept
        requires(N >= 3)
    {
        return data_[2];
    }

    /// @brief Dimension. @return N.
    [[nodiscard]] static constexpr std::size_t size() noexcept { return N; }

    /// @brief Begin iterator. @return Pointer to the first element.
    [[nodiscard]] constexpr auto begin() noexcept { return data_.begin(); }
    /// @brief End iterator. @return One past the last element.
    [[nodiscard]] constexpr auto end() noexcept { return data_.end(); }
    /// @brief Begin iterator. @return Pointer to the first element.
    [[nodiscard]] constexpr auto begin() const noexcept { return data_.begin(); }
    /// @brief End iterator. @return One past the last element.
    [[nodiscard]] constexpr auto end() const noexcept { return data_.end(); }

    /// @brief Component-wise addition. @param o Other. @return `*this`.
    constexpr Vector& operator+=(const Vector& o) noexcept {
        for (std::size_t i = 0; i < N; ++i) {
            data_[i] += o.data_[i];
        }
        return *this;
    }
    /// @brief Component-wise subtraction. @param o Other. @return `*this`.
    constexpr Vector& operator-=(const Vector& o) noexcept {
        for (std::size_t i = 0; i < N; ++i) {
            data_[i] -= o.data_[i];
        }
        return *this;
    }
    /// @brief Scalar multiplication. @param s Scalar. @return `*this`.
    constexpr Vector& operator*=(T s) noexcept {
        for (auto& v : data_) {
            v *= s;
        }
        return *this;
    }
    /// @brief Scalar division. @param s Scalar (non-zero). @return `*this`.
    constexpr Vector& operator/=(T s) noexcept {
        for (auto& v : data_) {
            v /= s;
        }
        return *this;
    }

    /// @brief Sum. @param a Lhs. @param b Rhs. @return a + b.
    [[nodiscard]] friend constexpr Vector operator+(Vector a, const Vector& b) noexcept { return a += b; }
    /// @brief Difference. @param a Lhs. @param b Rhs. @return a - b.
    [[nodiscard]] friend constexpr Vector operator-(Vector a, const Vector& b) noexcept { return a -= b; }
    /// @brief Negation. @param a Operand. @return -a.
    [[nodiscard]] friend constexpr Vector operator-(Vector a) noexcept { return a *= T{-1}; }
    /// @brief Scaling. @param a Vector. @param s Scalar. @return a * s.
    [[nodiscard]] friend constexpr Vector operator*(Vector a, T s) noexcept { return a *= s; }
    /// @brief Scaling. @param s Scalar. @param a Vector. @return s * a.
    [[nodiscard]] friend constexpr Vector operator*(T s, Vector a) noexcept { return a *= s; }
    /// @brief Division. @param a Vector. @param s Scalar. @return a / s.
    [[nodiscard]] friend constexpr Vector operator/(Vector a, T s) noexcept { return a /= s; }
    /// @brief Exact component-wise equality. @param a Lhs. @param b Rhs. @return True if equal.
    [[nodiscard]] friend constexpr bool operator==(const Vector& a, const Vector& b) noexcept = default;

    /// @brief Dot product. @param o Other. @return Sum of component products.
    [[nodiscard]] constexpr T dot(const Vector& o) const noexcept {
        T sum{};
        for (std::size_t i = 0; i < N; ++i) {
            sum += data_[i] * o.data_[i];
        }
        return sum;
    }

    /// @brief Squared length. @return |v|^2.
    [[nodiscard]] constexpr T lengthSquared() const noexcept { return dot(*this); }

    /// @brief Euclidean length. @return |v|.
    [[nodiscard]] T length() const noexcept { return std::sqrt(lengthSquared()); }

    /// @brief Unit vector in the same direction (zero vector stays zero). @return v / |v|.
    [[nodiscard]] Vector normalized() const noexcept {
        const T len = length();
        return len > T{0} ? *this / len : *this;
    }

    /// @brief Distance to another point. @param o Other. @return |this - o|.
    [[nodiscard]] T distanceTo(const Vector& o) const noexcept { return (*this - o).length(); }

    /// @brief Cross product (3-D only). @param o Other. @return this x o.
    [[nodiscard]] constexpr Vector cross(const Vector& o) const noexcept
        requires(N == 3)
    {
        return Vector{data_[1] * o.data_[2] - data_[2] * o.data_[1],
                      data_[2] * o.data_[0] - data_[0] * o.data_[2],
                      data_[0] * o.data_[1] - data_[1] * o.data_[0]};
    }

    /// @brief 2-D scalar cross product (z of the 3-D cross). @param o Other. @return x*o.y - y*o.x.
    [[nodiscard]] constexpr T cross(const Vector& o) const noexcept
        requires(N == 2)
    {
        return data_[0] * o.data_[1] - data_[1] * o.data_[0];
    }

    /// @brief Streams "(x, y, ...)". @param os Stream. @param v Vector. @return os.
    friend std::ostream& operator<<(std::ostream& os, const Vector& v) {
        os << '(';
        for (std::size_t i = 0; i < N; ++i) {
            os << (i ? ", " : "") << v.data_[i];
        }
        return os << ')';
    }

private:
    std::array<T, N> data_{};
};

using Vec2 = Vector<double, 2>; ///< 2-D double vector.
using Vec3 = Vector<double, 3>; ///< 3-D double vector.

/**
 * @brief Linear interpolation between vectors.
 * @param a Start.
 * @param b End.
 * @param t Parameter (0 -> a, 1 -> b).
 * @return Interpolated vector.
 */
template <std::floating_point T, std::size_t N>
[[nodiscard]] constexpr Vector<T, N> lerp(const Vector<T, N>& a, const Vector<T, N>& b, T t) noexcept {
    return a + (b - a) * t;
}

// ===================================================================================================
// Matrix
// ===================================================================================================

/**
 * @brief Dense row-major matrix of doubles with value semantics (rule of zero).
 */
class Matrix {
public:
    /// @brief Empty 0x0 matrix.
    Matrix() = default;

    /**
     * @brief Zero-filled matrix.
     * @param rows Row count.
     * @param cols Column count.
     */
    Matrix(std::size_t rows, std::size_t cols);

    /**
     * @brief Construction from nested initializer lists, e.g. `{{1, 2}, {3, 4}}`.
     * @param rows Row values (all rows must have equal length).
     * @throws std::invalid_argument for ragged input.
     */
    Matrix(std::initializer_list<std::initializer_list<double>> rows);

    /**
     * @brief Identity matrix.
     * @param n Size.
     * @return n x n identity.
     */
    [[nodiscard]] static Matrix identity(std::size_t n);

    /// @brief Row count. @return Rows.
    [[nodiscard]] std::size_t rows() const noexcept { return rows_; }
    /// @brief Column count. @return Columns.
    [[nodiscard]] std::size_t cols() const noexcept { return cols_; }
    /// @brief Whether rows == cols. @return True if square.
    [[nodiscard]] bool isSquare() const noexcept { return rows_ == cols_; }

    /**
     * @brief Unchecked element access.
     * @param r Row.
     * @param c Column.
     * @return Element reference.
     */
    [[nodiscard]] double& operator()(std::size_t r, std::size_t c) noexcept { return data_[r * cols_ + c]; }
    /// @copydoc operator()
    [[nodiscard]] double operator()(std::size_t r, std::size_t c) const noexcept {
        return data_[r * cols_ + c];
    }

    /**
     * @brief Bounds-checked element access.
     * @param r Row.
     * @param c Column.
     * @return Element.
     * @throws std::out_of_range when out of bounds.
     */
    [[nodiscard]] double at(std::size_t r, std::size_t c) const;

    /// @brief Transpose. @return Transposed copy.
    [[nodiscard]] Matrix transposed() const;

    /**
     * @brief Determinant via LU decomposition with partial pivoting.
     * @return det(A).
     * @throws std::logic_error if not square.
     */
    [[nodiscard]] double determinant() const;

    /**
     * @brief Inverse via Gauss-Jordan elimination with partial pivoting.
     * @return The inverse, or `std::nullopt` if singular (pivot below 1e-12).
     * @throws std::logic_error if not square.
     */
    [[nodiscard]] std::optional<Matrix> inverse() const;

    /**
     * @brief Solves A x = b.
     * @param b Right-hand side (size == rows).
     * @return x, or `std::nullopt` if A is singular.
     * @throws std::invalid_argument on dimension mismatch.
     */
    [[nodiscard]] std::optional<std::vector<double>> solve(std::span<const double> b) const;

    /// @brief Trace (sum of diagonal). @return Trace. @throws std::logic_error if not square.
    [[nodiscard]] double trace() const;

    /**
     * @brief Matrix product.
     * @param a Lhs (n x k).
     * @param b Rhs (k x m).
     * @return n x m product.
     * @throws std::invalid_argument on dimension mismatch.
     */
    friend Matrix operator*(const Matrix& a, const Matrix& b);

    /**
     * @brief Matrix-vector product.
     * @param a Matrix (n x k).
     * @param v Vector (size k).
     * @return Result of size n.
     * @throws std::invalid_argument on dimension mismatch.
     */
    friend std::vector<double> operator*(const Matrix& a, std::span<const double> v);

    /// @brief Sum. @param a Lhs. @param b Rhs. @return a + b. @throws std::invalid_argument on mismatch.
    friend Matrix operator+(const Matrix& a, const Matrix& b);
    /// @brief Difference. @param a Lhs. @param b Rhs. @return a - b. @throws std::invalid_argument on
    /// mismatch.
    friend Matrix operator-(const Matrix& a, const Matrix& b);
    /// @brief Scaling. @param a Matrix. @param s Scalar. @return a * s.
    friend Matrix operator*(const Matrix& a, double s);
    /// @brief Exact equality. @param a Lhs. @param b Rhs. @return True if identical.
    [[nodiscard]] friend bool operator==(const Matrix& a, const Matrix& b) = default;

    /**
     * @brief Element-wise approximate comparison.
     * @param other Other matrix.
     * @param epsilon Tolerance.
     * @return True if same shape and all elements approximately equal.
     */
    [[nodiscard]] bool approxEquals(const Matrix& other, double epsilon = 1e-9) const noexcept;

    /// @brief Streams rows on separate lines. @param os Stream. @param m Matrix. @return os.
    friend std::ostream& operator<<(std::ostream& os, const Matrix& m);

private:
    std::size_t rows_ = 0;
    std::size_t cols_ = 0;
    std::vector<double> data_;
};

/**
 * @brief Rotation of a 3-D vector about the Z axis.
 * @param v Vector.
 * @param radians Angle (counter-clockwise).
 * @return Rotated vector.
 */
[[nodiscard]] Vec3 rotateZ(const Vec3& v, double radians) noexcept;

/**
 * @brief Rotation of a vector about an arbitrary axis (Rodrigues' formula).
 * @param v Vector.
 * @param axis Rotation axis (need not be normalised; zero axis returns v).
 * @param radians Angle.
 * @return Rotated vector.
 */
[[nodiscard]] Vec3 rotateAroundAxis(const Vec3& v, const Vec3& axis, double radians) noexcept;

// ===================================================================================================
// Interpolation
// ===================================================================================================

namespace Interpolation {
/// @brief Linear interpolation. @param a Start. @param b End. @param t Parameter. @return a + (b-a) t.
[[nodiscard]] constexpr double lerp(double a, double b, double t) noexcept {
    return a + (b - a) * t;
}

/// @brief Inverse of lerp. @param a Start. @param b End (!= a). @param v Value. @return t such that lerp = v.
[[nodiscard]] constexpr double inverseLerp(double a, double b, double v) noexcept {
    return a == b ? 0.0 : (v - a) / (b - a);
}

/**
 * @brief Maps `v` from [inMin, inMax] to [outMin, outMax].
 * @param v Value. @param inMin Input start. @param inMax Input end. @param outMin Output start.
 * @param outMax Output end.
 * @return Remapped value.
 */
[[nodiscard]] constexpr double remap(double v, double inMin, double inMax, double outMin,
                                     double outMax) noexcept {
    return lerp(outMin, outMax, inverseLerp(inMin, inMax, v));
}

/// @brief Hermite smoothstep. @param e0 Lower edge. @param e1 Upper edge. @param x Input. @return Value in
/// [0,1].
[[nodiscard]] constexpr double smoothstep(double e0, double e1, double x) noexcept {
    const double t = std::clamp(inverseLerp(e0, e1, x), 0.0, 1.0);
    return t * t * (3.0 - 2.0 * t);
}

/**
 * @brief Cubic Bezier curve evaluation (de Casteljau, scalar form).
 * @param p0 Start. @param p1 Control 1. @param p2 Control 2. @param p3 End. @param t Parameter in [0,1].
 * @return Curve value.
 */
[[nodiscard]] constexpr double cubicBezier(double p0, double p1, double p2, double p3, double t) noexcept {
    const double u = 1.0 - t;
    return u * u * u * p0 + 3.0 * u * u * t * p1 + 3.0 * u * t * t * p2 + t * t * t * p3;
}

/**
 * @brief Catmull-Rom spline segment between p1 and p2.
 * @param p0 Previous point. @param p1 Segment start. @param p2 Segment end. @param p3 Next point.
 * @param t Parameter in [0,1].
 * @return Interpolated value (passes through p1 at t=0, p2 at t=1).
 */
[[nodiscard]] constexpr double catmullRom(double p0, double p1, double p2, double p3, double t) noexcept {
    const double t2 = t * t;
    const double t3 = t2 * t;
    return 0.5 * ((2.0 * p1) + (-p0 + p2) * t + (2.0 * p0 - 5.0 * p1 + 4.0 * p2 - p3) * t2 +
                  (-p0 + 3.0 * p1 - 3.0 * p2 + p3) * t3);
}
} // namespace Interpolation

// ===================================================================================================
// Number theory (constexpr)
// ===================================================================================================

namespace NumberTheory {
/// @brief Deterministic trial-division primality test. @param n Number. @return True if prime.
[[nodiscard]] constexpr bool isPrime(std::uint64_t n) noexcept {
    if (n < 2) {
        return false;
    }
    if (n % 2 == 0) {
        return n == 2;
    }
    if (n % 3 == 0) {
        return n == 3;
    }
    for (std::uint64_t i = 5; i <= n / i; i += 6) {
        if (n % i == 0 || n % (i + 2) == 0) {
            return false;
        }
    }
    return true;
}

/**
 * @brief n! with overflow detection.
 * @param n Argument.
 * @return n!, or `std::nullopt` if it does not fit in 64 bits (n > 20).
 */
[[nodiscard]] constexpr std::optional<std::uint64_t> factorial(unsigned n) noexcept {
    if (n > 20) {
        return std::nullopt;
    }
    std::uint64_t r = 1;
    for (unsigned i = 2; i <= n; ++i) {
        r *= i;
    }
    return r;
}

/// @brief Binomial coefficient C(n, k) computed multiplicatively. @param n n. @param k k. @return C(n,k).
[[nodiscard]] constexpr std::uint64_t binomial(unsigned n, unsigned k) noexcept {
    if (k > n) {
        return 0;
    }
    k = k < n - k ? k : n - k;
    std::uint64_t r = 1;
    for (unsigned i = 1; i <= k; ++i) {
        r = r * (n - k + i) / i; // exact at every step
    }
    return r;
}

/// @brief n-th Fibonacci number (F0 = 0) iteratively; valid for n <= 93. @param n Index. @return F(n).
[[nodiscard]] constexpr std::uint64_t fibonacci(unsigned n) noexcept {
    std::uint64_t a = 0;
    std::uint64_t b = 1;
    for (unsigned i = 0; i < n; ++i) {
        const std::uint64_t next = a + b;
        a = b;
        b = next;
    }
    return a;
}

/**
 * @brief Modular exponentiation by squaring.
 * @param base Base. @param exponent Exponent. @param modulus Modulus (> 0, < 2^32 to avoid overflow).
 * @return base^exponent mod modulus.
 */
[[nodiscard]] constexpr std::uint64_t modPow(std::uint64_t base, std::uint64_t exponent,
                                             std::uint64_t modulus) noexcept {
    if (modulus == 1) {
        return 0;
    }
    std::uint64_t result = 1;
    base %= modulus;
    while (exponent > 0) {
        if ((exponent & 1U) != 0U) {
            result = result * base % modulus;
        }
        base = base * base % modulus;
        exponent >>= 1U;
    }
    return result;
}

/**
 * @brief Sieve of Eratosthenes.
 * @param limit Inclusive upper bound.
 * @return All primes <= limit.
 */
[[nodiscard]] std::vector<std::uint64_t> primesUpTo(std::uint64_t limit);
} // namespace NumberTheory

// ===================================================================================================
// Numerical methods
// ===================================================================================================

namespace Numerical {
/**
 * @brief Composite Simpson's rule.
 * @param f Integrand.
 * @param a Lower bound.
 * @param b Upper bound.
 * @param intervals Number of sub-intervals (rounded up to even, at least 2).
 * @return Approximate integral.
 */
[[nodiscard]] double integrateSimpson(const std::function<double(double)>& f, double a, double b,
                                      std::size_t intervals = 1000);

/**
 * @brief Newton-Raphson root finding.
 * @param f Function.
 * @param df Derivative.
 * @param x0 Initial guess.
 * @param tolerance Stop when |step| < tolerance.
 * @param maxIterations Iteration cap.
 * @return Root, or `std::nullopt` if it did not converge or hit a zero derivative.
 */
[[nodiscard]] std::optional<double> newtonRaphson(const std::function<double(double)>& f,
                                                  const std::function<double(double)>& df, double x0,
                                                  double tolerance = 1e-12, int maxIterations = 100);

/**
 * @brief Bisection root finding on a bracketing interval.
 * @param f Continuous function with f(lo) and f(hi) of opposite sign.
 * @param lo Lower bound. @param hi Upper bound. @param tolerance Interval width at which to stop.
 * @return Root, or `std::nullopt` if the interval does not bracket a root.
 */
[[nodiscard]] std::optional<double> bisection(const std::function<double(double)>& f, double lo, double hi,
                                              double tolerance = 1e-12);
} // namespace Numerical

// ===================================================================================================
// Statistics
// ===================================================================================================

namespace Statistics {
/// @brief Result of an ordinary least-squares fit y = slope * x + intercept.
struct LinearFit {
    double slope = 0.0;     ///< Slope.
    double intercept = 0.0; ///< Intercept.
    double rSquared = 0.0;  ///< Coefficient of determination.
};

/// @brief Arithmetic mean. @param data Samples (non-empty). @return Mean. @throws std::invalid_argument if
/// empty.
[[nodiscard]] double mean(std::span<const double> data);
/**
 * @brief Variance (two-pass algorithm for numerical stability).
 * @param data Samples.
 * @param sample If true divide by n-1 (requires n >= 2), else by n.
 * @return Variance.
 * @throws std::invalid_argument for too few samples.
 */
[[nodiscard]] double variance(std::span<const double> data, bool sample = true);
/// @brief Standard deviation. @param data Samples. @param sample Bessel correction. @return sqrt(variance).
[[nodiscard]] double standardDeviation(std::span<const double> data, bool sample = true);
/// @brief Median. @param data Samples (non-empty). @return Median. @throws std::invalid_argument if empty.
[[nodiscard]] double median(std::span<const double> data);
/**
 * @brief Percentile with linear interpolation between closest ranks.
 * @param data Samples (non-empty).
 * @param p Percentile in [0, 100].
 * @return Value at percentile.
 * @throws std::invalid_argument if empty or p out of range.
 */
[[nodiscard]] double percentile(std::span<const double> data, double p);
/// @brief Pearson correlation. @param x Xs. @param y Ys (same size >= 2). @return r in [-1, 1].
[[nodiscard]] double correlation(std::span<const double> x, std::span<const double> y);
/// @brief Least-squares line fit. @param x Xs. @param y Ys (same size >= 2). @return Fit.
[[nodiscard]] LinearFit linearRegression(std::span<const double> x, std::span<const double> y);
} // namespace Statistics

// ===================================================================================================
// Random
// ===================================================================================================

/**
 * @brief Seeded wrapper around `std::mt19937_64`.
 *
 * The engine sequence is fully specified by the standard; distribution outputs may differ across
 * standard libraries, so results are reproducible per platform for a given seed.
 */
class RandomGenerator {
public:
    /**
     * @brief Seeds the engine.
     * @param seed Seed value.
     */
    explicit RandomGenerator(std::uint64_t seed = 42) noexcept : engine_(seed) {}

    /// @brief Uniform integer in [lo, hi]. @param lo Lower bound. @param hi Upper bound. @return Value.
    [[nodiscard]] std::int64_t uniformInt(std::int64_t lo, std::int64_t hi) {
        return std::uniform_int_distribution<std::int64_t>{lo, hi}(engine_);
    }
    /// @brief Uniform real in [lo, hi). @param lo Lower bound. @param hi Upper bound. @return Value.
    [[nodiscard]] double uniformReal(double lo = 0.0, double hi = 1.0) {
        return std::uniform_real_distribution<double>{lo, hi}(engine_);
    }
    /// @brief Normal variate. @param mean Mean. @param stddev Standard deviation. @return Value.
    [[nodiscard]] double normal(double mean = 0.0, double stddev = 1.0) {
        return std::normal_distribution<double>{mean, stddev}(engine_);
    }
    /// @brief Bernoulli trial. @param probability Success probability in [0,1]. @return True on success.
    [[nodiscard]] bool chance(double probability) {
        return std::bernoulli_distribution{std::clamp(probability, 0.0, 1.0)}(engine_);
    }
    /// @brief Point uniformly distributed on the unit sphere. @return Unit vector.
    [[nodiscard]] Vec3 unitVector();

    /// @brief Fisher-Yates shuffle. @param values Values to shuffle in place.
    template <typename T>
    void shuffle(std::vector<T>& values) {
        std::shuffle(values.begin(), values.end(), engine_);
    }

    /// @brief Access to the engine for use with other distributions. @return Engine reference.
    [[nodiscard]] std::mt19937_64& engine() noexcept { return engine_; }

private:
    std::mt19937_64 engine_;
};

// ===================================================================================================
// Noise
// ===================================================================================================

/**
 * @brief Improved Perlin noise (Ken Perlin, 2002) with a seeded permutation table.
 */
class PerlinNoise {
public:
    /**
     * @brief Builds the permutation table.
     * @param seed Seed for the permutation shuffle.
     */
    explicit PerlinNoise(std::uint64_t seed = 0);

    /**
     * @brief 3-D gradient noise.
     * @param x X. @param y Y. @param z Z.
     * @return Value in approximately [-1, 1]; exactly 0 at integer lattice points.
     */
    [[nodiscard]] double noise(double x, double y, double z = 0.0) const noexcept;

    /**
     * @brief Fractal Brownian motion: sum of octaves with halving amplitude and doubling frequency.
     * @param x X. @param y Y. @param z Z. @param octaves Octave count (>= 1).
     * @param persistence Amplitude multiplier per octave.
     * @return Normalised value in approximately [-1, 1].
     */
    [[nodiscard]] double fractal(double x, double y, double z, int octaves,
                                 double persistence = 0.5) const noexcept;

private:
    std::array<std::uint8_t, 512> perm_{};
};

// ===================================================================================================
// Geometry
// ===================================================================================================

namespace Geometry {
/// @brief Signed area of triangle abc (positive if counter-clockwise). @param a A. @param b B. @param c C.
/// @return Signed area.
[[nodiscard]] constexpr double signedTriangleArea(const Vec2& a, const Vec2& b, const Vec2& c) noexcept {
    return 0.5 * (b - a).cross(c - a);
}
/// @brief Signed polygon area by the shoelace formula. @param polygon Vertices in order. @return Signed area.
[[nodiscard]] double polygonArea(std::span<const Vec2> polygon) noexcept;
/// @brief Even-odd point-in-polygon test. @param p Point. @param polygon Vertices. @return True if inside.
[[nodiscard]] bool pointInPolygon(const Vec2& p, std::span<const Vec2> polygon) noexcept;
/// @brief Convex hull (Andrew's monotone chain), counter-clockwise, no collinear points.
/// @param points Input points. @return Hull vertices.
[[nodiscard]] std::vector<Vec2> convexHull(std::vector<Vec2> points);
/// @brief Whether closed segments p1p2 and q1q2 intersect. @param p1 P1. @param p2 P2. @param q1 Q1.
/// @param q2 Q2. @return True if they intersect (including touching).
[[nodiscard]] bool segmentsIntersect(const Vec2& p1, const Vec2& p2, const Vec2& q1, const Vec2& q2) noexcept;
/// @brief Distance from point p to segment ab. @param p Point. @param a A. @param b B. @return Distance.
[[nodiscard]] double pointSegmentDistance(const Vec2& p, const Vec2& a, const Vec2& b) noexcept;
} // namespace Geometry

// ===================================================================================================
// Orbital mechanics
// ===================================================================================================

namespace Space {
/// @brief Result of a two-impulse Hohmann transfer between circular coplanar orbits.
struct HohmannTransfer {
    double deltaV1 = 0.0;      ///< Burn at departure (m/s).
    double deltaV2 = 0.0;      ///< Burn at arrival (m/s).
    double totalDeltaV = 0.0;  ///< |dv1| + |dv2| (m/s).
    double transferTime = 0.0; ///< Half the transfer-orbit period (s).
};

/// @brief Kepler's third law. @param semiMajorAxis a (m). @param mu GM (m^3/s^2). @return Period (s).
[[nodiscard]] double orbitalPeriod(double semiMajorAxis, double mu = Constants::kEarthMu);
/// @brief Circular orbital speed. @param radius r (m). @param mu GM. @return Speed (m/s).
[[nodiscard]] double circularVelocity(double radius, double mu = Constants::kEarthMu);
/// @brief Escape speed. @param radius r (m). @param mu GM. @return Speed (m/s).
[[nodiscard]] double escapeVelocity(double radius, double mu = Constants::kEarthMu);
/// @brief Hohmann transfer. @param r1 Initial radius. @param r2 Final radius. @param mu GM. @return Transfer.
/// @throws std::invalid_argument for non-positive radii.
[[nodiscard]] HohmannTransfer hohmannTransfer(double r1, double r2, double mu = Constants::kEarthMu);

/**
 * @brief Gravitational N-body simulation integrated with velocity Verlet (symplectic, 2nd order).
 */
class NBodySimulator {
public:
    /// @brief A point mass.
    struct Body {
        Vec3 position;   ///< Position (m).
        Vec3 velocity;   ///< Velocity (m/s).
        double mass = 0; ///< Mass (kg).
    };

    /**
     * @brief Creates a simulator.
     * @param gravitationalConstant G (override to use natural units).
     * @param softening Plummer softening length to avoid singularities.
     */
    explicit NBodySimulator(double gravitationalConstant = Constants::kGravitationalConstant,
                            double softening = 0.0) noexcept
        : g_(gravitationalConstant), softening_(softening) {}

    /// @brief Adds a body. @param body Body. @return Its index.
    std::size_t addBody(const Body& body);
    /// @brief Advances by `dt`. @param dt Time step (s).
    void step(double dt);
    /// @brief Total kinetic + potential energy. @return Energy (J).
    [[nodiscard]] double totalEnergy() const;
    /// @brief Total linear momentum. @return Momentum.
    [[nodiscard]] Vec3 totalMomentum() const noexcept;
    /// @brief Bodies. @return Read-only view.
    [[nodiscard]] const std::vector<Body>& bodies() const noexcept { return bodies_; }

private:
    void computeAccelerations();

    double g_;
    double softening_;
    std::vector<Body> bodies_;
    std::vector<Vec3> accelerations_;
    bool accelerationsValid_ = false;
};
} // namespace Space

/**
 * @brief Runs the math showcase, writing only to `out`.
 * @param out Destination stream.
 */
void demonstrateMath(std::ostream& out = std::cout);

} // namespace CppVerseHub::Utils::Math
