// File: src/utils/MathUtils.hpp
// Mathematical Computations and Utilities for Space Game

#pragma once

#include <cmath>
#include <vector>
#include <array>
#include <algorithm>
#include <numeric>
#include <random>
#include <complex>
#include <functional>
#include <limits>
#include <type_traits>

namespace CppVerseHub::Utils::Math {

// ===== MATHEMATICAL CONSTANTS =====

namespace Constants {
    constexpr double PI = 3.14159265358979323846;
    constexpr double E = 2.71828182845904523536;
    constexpr double GOLDEN_RATIO = 1.61803398874989484820;
    constexpr double SQRT2 = 1.41421356237309504880;
    constexpr double SQRT3 = 1.73205080756887729352;
    constexpr double DEG_TO_RAD = PI / 180.0;
    constexpr double RAD_TO_DEG = 180.0 / PI;
    
    // Physics constants for space game
    constexpr double LIGHT_SPEED = 299792458.0; // m/s
    constexpr double GRAVITATIONAL_CONSTANT = 6.67430e-11; // m³/kg⋅s²
    constexpr double PLANCK_CONSTANT = 6.62607015e-34; // J⋅Hz⁻¹
    constexpr double BOLTZMANN_CONSTANT = 1.380649e-23; // J/K
    constexpr double AVOGADRO_NUMBER = 6.02214076e23; // mol⁻¹
    
    // Astronomical constants
    constexpr double ASTRONOMICAL_UNIT = 149597870700.0; // meters
    constexpr double LIGHT_YEAR = 9.4607304725808e15; // meters
    constexpr double PARSEC = 3.0856775814913673e16; // meters
    constexpr double SOLAR_MASS = 1.98847e30; // kg
    constexpr double EARTH_MASS = 5.9722e24; // kg
    constexpr double EARTH_RADIUS = 6.371e6; // meters
}

// ===== BASIC MATHEMATICAL FUNCTIONS =====

template<typename T>
constexpr T clamp(T value, T min_val, T max_val) {
    return std::max(min_val, std::min(value, max_val));
}

template<typename T>
constexpr T lerp(T a, T b, T t) {
    return a + t * (b - a);
}

template<typename T>
constexpr T smoothstep(T edge0, T edge1, T x) {
    T t = clamp((x - edge0) / (edge1 - edge0), T(0), T(1));
    return t * t * (T(3) - T(2) * t);
}

template<typename T>
constexpr T sign(T value) {
    return (T(0) < value) - (value < T(0));
}

template<typename T>
constexpr T square(T value) {
    return value * value;
}

template<typename T>
constexpr T cube(T value) {
    return value * value * value;
}

// Safe division with default value
template<typename T>
T safeDivide(T numerator, T denominator, T default_value = T(0)) {
    return (std::abs(denominator) > std::numeric_limits<T>::epsilon()) ? 
           (numerator / denominator) : default_value;
}

// Fast inverse square root (approximation)
float fastInverseSqrt(float x);

// More accurate version using Newton-Raphson
double accurateInverseSqrt(double x);

// ===== VECTOR MATHEMATICS =====

template<typename T, size_t N>
class Vector {
private:
    std::array<T, N> components_;
    
public:
    Vector() { components_.fill(T(0)); }
    
    template<typename... Args>
    Vector(Args... args) : components_{static_cast<T>(args)...} {
        static_assert(sizeof...(args) == N, "Number of arguments must match vector dimension");
    }
    
    explicit Vector(const std::array<T, N>& arr) : components_(arr) {}
    
    // Element access
    T& operator[](size_t index) { return components_[index]; }
    const T& operator[](size_t index) const { return components_[index]; }
    
    T& at(size_t index) { return components_.at(index); }
    const T& at(size_t index) const { return components_.at(index); }
    
    // Common accessors for 2D, 3D vectors
    T& x() { static_assert(N >= 1); return components_[0]; }
    const T& x() const { static_assert(N >= 1); return components_[0]; }
    
    T& y() { static_assert(N >= 2); return components_[1]; }
    const T& y() const { static_assert(N >= 2); return components_[1]; }
    
    T& z() { static_assert(N >= 3); return components_[2]; }
    const T& z() const { static_assert(N >= 3); return components_[2]; }
    
    T& w() { static_assert(N >= 4); return components_[3]; }
    const T& w() const { static_assert(N >= 4); return components_[3]; }
    
    // Vector operations
    Vector operator+(const Vector& other) const {
        Vector result;
        for (size_t i = 0; i < N; ++i) {
            result[i] = components_[i] + other[i];
        }
        return result;
    }
    
    Vector operator-(const Vector& other) const {
        Vector result;
        for (size_t i = 0; i < N; ++i) {
            result[i] = components_[i] - other[i];
        }
        return result;
    }
    
    Vector operator*(T scalar) const {
        Vector result;
        for (size_t i = 0; i < N; ++i) {
            result[i] = components_[i] * scalar;
        }
        return result;
    }
    
    Vector operator/(T scalar) const {
        Vector result;
        for (size_t i = 0; i < N; ++i) {
            result[i] = components_[i] / scalar;
        }
        return result;
    }
    
    Vector& operator+=(const Vector& other) {
        for (size_t i = 0; i < N; ++i) {
            components_[i] += other[i];
        }
        return *this;
    }
    
    Vector& operator-=(const Vector& other) {
        for (size_t i = 0; i < N; ++i) {
            components_[i] -= other[i];
        }
        return *this;
    }
    
    Vector& operator*=(T scalar) {
        for (size_t i = 0; i < N; ++i) {
            components_[i] *= scalar;
        }
        return *this;
    }
    
    Vector& operator/=(T scalar) {
        for (size_t i = 0; i < N; ++i) {
            components_[i] /= scalar;
        }
        return *this;
    }
    
    // Vector properties
    T dot(const Vector& other) const {
        T result = T(0);
        for (size_t i = 0; i < N; ++i) {
            result += components_[i] * other[i];
        }
        return result;
    }
    
    T lengthSquared() const {
        return dot(*this);
    }
    
    T length() const {
        return std::sqrt(lengthSquared());
    }
    
    Vector normalized() const {
        T len = length();
        if (len > std::numeric_limits<T>::epsilon()) {
            return *this / len;
        }
        return Vector{};
    }
    
    void normalize() {
        *this = normalized();
    }
    
    T distanceTo(const Vector& other) const {
        return (*this - other).length();
    }
    
    T distanceSquaredTo(const Vector& other) const {
        return (*this - other).lengthSquared();
    }
    
    // Cross product (3D only)
    Vector cross(const Vector& other) const {
        static_assert(N == 3, "Cross product only defined for 3D vectors");
        return Vector(
            components_[1] * other[2] - components_[2] * other[1],
            components_[2] * other[0] - components_[0] * other[2],
            components_[0] * other[1] - components_[1] * other[0]
        );
    }
    
    // Reflection
    Vector reflect(const Vector& normal) const {
        return *this - normal * (T(2) * dot(normal));
    }
    
    // Projection
    Vector project(const Vector& onto) const {
        return onto * (dot(onto) / onto.lengthSquared());
    }
    
    // Angle between vectors
    T angleTo(const Vector& other) const {
        T cos_angle = dot(other) / (length() * other.length());
        return std::acos(clamp(cos_angle, T(-1), T(1)));
    }
    
    // Component-wise operations
    Vector abs() const {
        Vector result;
        for (size_t i = 0; i < N; ++i) {
            result[i] = std::abs(components_[i]);
        }
        return result;
    }
    
    Vector min(const Vector& other) const {
        Vector result;
        for (size_t i = 0; i < N; ++i) {
            result[i] = std::min(components_[i], other[i]);
        }
        return result;
    }
    
    Vector max(const Vector& other) const {
        Vector result;
        for (size_t i = 0; i < N; ++i) {
            result[i] = std::max(components_[i], other[i]);
        }
        return result;
    }
    
    // Utility
    constexpr size_t size() const { return N; }
    const std::array<T, N>& data() const { return components_; }
    std::array<T, N>& data() { return components_; }
    
    // Iterator support
    auto begin() { return components_.begin(); }
    auto end() { return components_.end(); }
    auto begin() const { return components_.begin(); }
    auto end() const { return components_.end(); }
};

// Common vector types
using Vec2f = Vector<float, 2>;
using Vec3f = Vector<float, 3>;
using Vec4f = Vector<float, 4>;
using Vec2d = Vector<double, 2>;
using Vec3d = Vector<double, 3>;
using Vec4d = Vector<double, 4>;
using Vec2i = Vector<int, 2>;
using Vec3i = Vector<int, 3>;
using Vec4i = Vector<int, 4>;

// Scalar multiplication (scalar * vector)
template<typename T, size_t N>
Vector<T, N> operator*(T scalar, const Vector<T, N>& vec) {
    return vec * scalar;
}

// ===== MATRIX MATHEMATICS =====

template<typename T, size_t Rows, size_t Cols>
class Matrix {
private:
    std::array<std::array<T, Cols>, Rows> data_;
    
public:
    Matrix() {
        for (auto& row : data_) {
            row.fill(T(0));
        }
    }
    
    // Identity matrix (square matrices only)
    static Matrix identity() {
        static_assert(Rows == Cols, "Identity matrix requires square matrix");
        Matrix result;
        for (size_t i = 0; i < Rows; ++i) {
            result[i][i] = T(1);
        }
        return result;
    }
    
    // Element access
    std::array<T, Cols>& operator[](size_t row) { return data_[row]; }
    const std::array<T, Cols>& operator[](size_t row) const { return data_[row]; }
    
    T& at(size_t row, size_t col) { return data_.at(row).at(col); }
    const T& at(size_t row, size_t col) const { return data_.at(row).at(col); }
    
    // Matrix operations
    Matrix operator+(const Matrix& other) const {
        Matrix result;
        for (size_t i = 0; i < Rows; ++i) {
            for (size_t j = 0; j < Cols; ++j) {
                result[i][j] = data_[i][j] + other[i][j];
            }
        }
        return result;
    }
    
    Matrix operator-(const Matrix& other) const {
        Matrix result;
        for (size_t i = 0; i < Rows; ++i) {
            for (size_t j = 0; j < Cols; ++j) {
                result[i][j] = data_[i][j] - other[i][j];
            }
        }
        return result;
    }
    
    Matrix operator*(T scalar) const {
        Matrix result;
        for (size_t i = 0; i < Rows; ++i) {
            for (size_t j = 0; j < Cols; ++j) {
                result[i][j] = data_[i][j] * scalar;
            }
        }
        return result;
    }
    
    // Matrix multiplication
    template<size_t OtherCols>
    Matrix<T, Rows, OtherCols> operator*(const Matrix<T, Cols, OtherCols>& other) const {
        Matrix<T, Rows, OtherCols> result;
        for (size_t i = 0; i < Rows; ++i) {
            for (size_t j = 0; j < OtherCols; ++j) {
                for (size_t k = 0; k < Cols; ++k) {
                    result[i][j] += data_[i][k] * other[k][j];
                }
            }
        }
        return result;
    }
    
    // Matrix-vector multiplication
    Vector<T, Rows> operator*(const Vector<T, Cols>& vec) const {
        Vector<T, Rows> result;
        for (size_t i = 0; i < Rows; ++i) {
            for (size_t j = 0; j < Cols; ++j) {
                result[i] += data_[i][j] * vec[j];
            }
        }
        return result;
    }
    
    // Transpose
    Matrix<T, Cols, Rows> transpose() const {
        Matrix<T, Cols, Rows> result;
        for (size_t i = 0; i < Rows; ++i) {
            for (size_t j = 0; j < Cols; ++j) {
                result[j][i] = data_[i][j];
            }
        }
        return result;
    }
    
    // Determinant (2x2 and 3x3)
    T determinant() const {
        static_assert(Rows == Cols, "Determinant requires square matrix");
        
        if constexpr (Rows == 2) {
            return data_[0][0] * data_[1][1] - data_[0][1] * data_[1][0];
        } else if constexpr (Rows == 3) {
            return data_[0][0] * (data_[1][1] * data_[2][2] - data_[1][2] * data_[2][1]) -
                   data_[0][1] * (data_[1][0] * data_[2][2] - data_[1][2] * data_[2][0]) +
                   data_[0][2] * (data_[1][0] * data_[2][1] - data_[1][1] * data_[2][0]);
        } else {
            // For larger matrices, use recursive expansion (expensive)
            T det = T(0);
            for (size_t j = 0; j < Cols; ++j) {
                det += data_[0][j] * cofactor(0, j);
            }
            return det;
        }
    }
    
    // Inverse (2x2 and 3x3)
    Matrix inverse() const {
        static_assert(Rows == Cols, "Inverse requires square matrix");
        
        T det = determinant();
        if (std::abs(det) < std::numeric_limits<T>::epsilon()) {
            throw std::runtime_error("Matrix is singular and cannot be inverted");
        }
        
        if constexpr (Rows == 2) {
            Matrix result;
            result[0][0] = data_[1][1] / det;
            result[0][1] = -data_[0][1] / det;
            result[1][0] = -data_[1][0] / det;
            result[1][1] = data_[0][0] / det;
            return result;
        } else if constexpr (Rows == 3) {
            Matrix result;
            result[0][0] = (data_[1][1] * data_[2][2] - data_[1][2] * data_[2][1]) / det;
            result[0][1] = (data_[0][2] * data_[2][1] - data_[0][1] * data_[2][2]) / det;
            result[0][2] = (data_[0][1] * data_[1][2] - data_[0][2] * data_[1][1]) / det;
            result[1][0] = (data_[1][2] * data_[2][0] - data_[1][0] * data_[2][2]) / det;
            result[1][1] = (data_[0][0] * data_[2][2] - data_[0][2] * data_[2][0]) / det;
            result[1][2] = (data_[0][2] * data_[1][0] - data_[0][0] * data_[1][2]) / det;
            result[2][0] = (data_[1][0] * data_[2][1] - data_[1][1] * data_[2][0]) / det;
            result[2][1] = (data_[0][1] * data_[2][0] - data_[0][0] * data_[2][1]) / det;
            result[2][2] = (data_[0][0] * data_[1][1] - data_[0][1] * data_[1][0]) / det;
            return result;
        } else {
            // For larger matrices, use Gauss-Jordan elimination
            return gaussJordanInverse();
        }
    }
    
    // Utility
    constexpr size_t rows() const { return Rows; }
    constexpr size_t cols() const { return Cols; }
    
private:
    T cofactor(size_t row, size_t col) const {
        Matrix<T, Rows-1, Cols-1> minor;
        size_t minor_row = 0;
        
        for (size_t i = 0; i < Rows; ++i) {
            if (i == row) continue;
            size_t minor_col = 0;
            
            for (size_t j = 0; j < Cols; ++j) {
                if (j == col) continue;
                minor[minor_row][minor_col] = data_[i][j];
                ++minor_col;
            }
            ++minor_row;
        }
        
        T sign = ((row + col) % 2 == 0) ? T(1) : T(-1);
        return sign * minor.determinant();
    }
    
    Matrix gaussJordanInverse() const {
        // Implementation for larger matrices using Gauss-Jordan elimination
        // This is a simplified version - full implementation would be more robust
        Matrix<T, Rows, Cols * 2> augmented;
        
        // Create augmented matrix [A | I]
        for (size_t i = 0; i < Rows; ++i) {
            for (size_t j = 0; j < Cols; ++j) {
                augmented[i][j] = data_[i][j];
                augmented[i][j + Cols] = (i == j) ? T(1) : T(0);
            }
        }
        
        // Forward elimination and back substitution would go here
        // For brevity, throwing an exception for larger matrices
        throw std::runtime_error("Inverse not implemented for matrices larger than 3x3");
    }
};

// Common matrix types
using Mat2f = Matrix<float, 2, 2>;
using Mat3f = Matrix<float, 3, 3>;
using Mat4f = Matrix<float, 4, 4>;
using Mat2d = Matrix<double, 2, 2>;
using Mat3d = Matrix<double, 3, 3>;
using Mat4d = Matrix<double, 4, 4>;

// ===== TRANSFORMATION MATRICES =====

namespace Transform {
    
    // 2D transformations
    Mat3f translation2D(float tx, float ty);
    Mat3f rotation2D(float angle);
    Mat3f scale2D(float sx, float sy);
    
    // 3D transformations
    Mat4f translation3D(float tx, float ty, float tz);
    Mat4f rotationX3D(float angle);
    Mat4f rotationY3D(float angle);
    Mat4f rotationZ3D(float angle);
    Mat4f rotation3D(float angle, const Vec3f& axis);
    Mat4f scale3D(float sx, float sy, float sz);
    
    // Camera transformations
    Mat4f lookAt(const Vec3f& eye, const Vec3f& target, const Vec3f& up);
    Mat4f perspective(float fov, float aspect, float near, float far);
    Mat4f orthographic(float left, float right, float bottom, float top, float near, float far);
    
} // namespace Transform

// ===== INTERPOLATION FUNCTIONS =====

namespace Interpolation {
    
    // Various interpolation methods
    template<typename T>
    T linear(T a, T b, T t) {
        return lerp(a, b, t);
    }
    
    template<typename T>
    T cosine(T a, T b, T t) {
        T ft = t * Constants::PI;
        T f = (T(1) - std::cos(ft)) * T(0.5);
        return a * (T(1) - f) + b * f;
    }
    
    template<typename T>
    T cubic(T a, T b, T c, T d, T t) {
        T t2 = t * t;
        T a0 = d - c - a + b;
        T a1 = a - b - a0;
        T a2 = c - a;
        T a3 = b;
        return a0 * t * t2 + a1 * t2 + a2 * t + a3;
    }
    
    // Bezier curves
    template<typename T>
    T quadraticBezier(T p0, T p1, T p2, T t) {
        T u = T(1) - t;
        return u * u * p0 + T(2) * u * t * p1 + t * t * p2;
    }
    
    template<typename T>
    T cubicBezier(T p0, T p1, T p2, T p3, T t) {
        T u = T(1) - t;
        T u2 = u * u;
        T t2 = t * t;
        return u2 * u * p0 + T(3) * u2 * t * p1 + T(3) * u * t2 * p2 + t2 * t * p3;
    }
    
    // Spline interpolation
    template<typename T>
    T catmullRom(T p0, T p1, T p2, T p3, T t) {
        T t2 = t * t;
        T t3 = t2 * t;
        
        return T(0.5) * ((T(2) * p1) +
                        (-p0 + p2) * t +
                        (T(2) * p0 - T(5) * p1 + T(4) * p2 - p3) * t2 +
                        (-p0 + T(3) * p1 - T(3) * p2 + p3) * t3);
    }
    
} // namespace Interpolation

// ===== NOISE GENERATION =====

namespace Noise {
    
    class PerlinNoise {
    private:
        std::array<int, 512> permutation_;
        
        double fade(double t) const { return t * t * t * (t * (t * 6 - 15) + 10); }
        double grad(int hash, double x, double y, double z) const;
        
    public:
        PerlinNoise(unsigned int seed = 0);
        
        double noise(double x, double y, double z = 0.0) const;
        double octaveNoise(double x, double y, double z, int octaves, double persistence) const;
        double turbulence(double x, double y, double z, int octaves) const;
    };
    
    class SimplexNoise {
    private:
        std::array<int, 512> perm_;
        
    public:
        SimplexNoise(unsigned int seed = 0);
        
        double noise(double x, double y) const;
        double noise(double x, double y, double z) const;
    };
    
} // namespace Noise

// ===== STATISTICS AND ANALYSIS =====

namespace Statistics {
    
    template<typename Container>
    auto mean(const Container& data) -> decltype(*data.begin()) {
        using ValueType = decltype(*data.begin());
        if (data.empty()) return ValueType{};
        
        auto sum = std::accumulate(data.begin(), data.end(), ValueType{});
        return sum / static_cast<ValueType>(data.size());
    }
    
    template<typename Container>
    auto variance(const Container& data) -> decltype(*data.begin()) {
        using ValueType = decltype(*data.begin());
        if (data.size() < 2) return ValueType{};
        
        ValueType mean_val = mean(data);
        ValueType sum_sq_diff{};
        
        for (const auto& value : data) {
            ValueType diff = value - mean_val;
            sum_sq_diff += diff * diff;
        }
        
        return sum_sq_diff / static_cast<ValueType>(data.size() - 1);
    }
    
    template<typename Container>
    auto standardDeviation(const Container& data) -> decltype(*data.begin()) {
        return std::sqrt(variance(data));
    }
    
    template<typename Container>
    auto median(Container data) -> decltype(*data.begin()) {
        if (data.empty()) return {};
        
        std::sort(data.begin(), data.end());
        size_t n = data.size();
        
        if (n % 2 == 0) {
            return (data[n/2 - 1] + data[n/2]) / 2;
        } else {
            return data[n/2];
        }
    }
    
    template<typename Container>
    auto mode(const Container& data) -> decltype(*data.begin()) {
        using ValueType = decltype(*data.begin());
        if (data.empty()) return ValueType{};
        
        std::map<ValueType, size_t> frequency;
        for (const auto& value : data) {
            ++frequency[value];
        }
        
        auto max_freq = std::max_element(frequency.begin(), frequency.end(),
            [](const auto& a, const auto& b) { return a.second < b.second; });
        
        return max_freq->first;
    }
    
} // namespace Statistics

// ===== SPACE-SPECIFIC CALCULATIONS =====

namespace Space {
    
    // Orbital mechanics
    struct OrbitalElements {
        double semi_major_axis;    // a (AU)
        double eccentricity;       // e
        double inclination;        // i (radians)
        double longitude_ascending_node; // Ω (radians)
        double argument_periapsis; // ω (radians)
        double mean_anomaly;       // M (radians)
    };
    
    struct CartesianCoordinates {
        Vec3d position;  // (x, y, z) in AU
        Vec3d velocity;  // (vx, vy, vz) in AU/day
    };
    
    // Convert orbital elements to Cartesian coordinates
    CartesianCoordinates orbitalToCartesian(const OrbitalElements& elements, double gravitational_parameter);
    
    // Convert Cartesian coordinates to orbital elements
    OrbitalElements cartesianToOrbital(const CartesianCoordinates& coords, double gravitational_parameter);
    
    // Calculate orbital period
    double orbitalPeriod(double semi_major_axis, double gravitational_parameter);
    
    // Calculate escape velocity
    double escapeVelocity(double mass, double radius);
    
    // Calculate gravitational force between two masses
    Vec3d gravitationalForce(double mass1, double mass2, const Vec3d& position1, const Vec3d& position2);
    
    // Calculate Hohmann transfer orbit
    struct HohmannTransfer {
        double delta_v1;  // First burn delta-v
        double delta_v2;  // Second burn delta-v
        double transfer_time;  // Transfer duration
        double transfer_semi_major_axis;
    };
    
    HohmannTransfer calculateHohmannTransfer(double r1, double r2, double gravitational_parameter);
    
    // N-body gravitational simulation utilities
    class NBodySimulator {
    private:
        struct Body {
            double mass;
            Vec3d position;
            Vec3d velocity;
            Vec3d acceleration;
        };
        
        std::vector<Body> bodies_;
        double time_step_;
        
    public:
        NBodySimulator(double dt = 0.01) : time_step_(dt) {}
        
        void addBody(double mass, const Vec3d& position, const Vec3d& velocity);
        void step();
        void simulate(double duration);
        
        size_t getBodyCount() const { return bodies_.size(); }
        Vec3d getBodyPosition(size_t index) const { return bodies_.at(index).position; }
        Vec3d getBodyVelocity(size_t index) const { return bodies_.at(index).velocity; }
        double getBodyMass(size_t index) const { return bodies_.at(index).mass; }
    };
    
} // namespace Space

// ===== RANDOM NUMBER GENERATION =====

namespace Random {
    
    class RandomGenerator {
    private:
        mutable std::mt19937 generator_;
        
    public:
        explicit RandomGenerator(std::random_device::result_type seed = std::random_device{}()) 
            : generator_(seed) {}
        
        // Integer distributions
        int uniform(int min, int max) const {
            std::uniform_int_distribution<int> dist(min, max);
            return dist(generator_);
        }
        
        // Real distributions
        double uniform(double min = 0.0, double max = 1.0) const {
            std::uniform_real_distribution<double> dist(min, max);
            return dist(generator_);
        }
        
        double normal(double mean = 0.0, double stddev = 1.0) const {
            std::normal_distribution<double> dist(mean, stddev);
            return dist(generator_);
        }
        
        double exponential(double lambda = 1.0) const {
            std::exponential_distribution<double> dist(lambda);
            return dist(generator_);
        }
        
        double gamma(double alpha, double beta) const {
            std::gamma_distribution<double> dist(alpha, beta);
            return dist(generator_);
        }
        
        // Boolean with probability
        bool bernoulli(double probability = 0.5) const {
            std::bernoulli_distribution dist(probability);
            return dist(generator_);
        }
        
        // Random point on unit sphere
        Vec3d randomUnitSphere() const {
            double z = uniform(-1.0, 1.0);
            double phi = uniform(0.0, 2.0 * Constants::PI);
            double r = std::sqrt(1.0 - z * z);
            return Vec3d(r * std::cos(phi), r * std::sin(phi), z);
        }
        
        // Random point in unit circle
        Vec2d randomUnitCircle() const {
            double angle = uniform(0.0, 2.0 * Constants::PI);
            return Vec2d(std::cos(angle), std::sin(angle));
        }
        
        // Seed the generator
        void seed(std::random_device::result_type new_seed) {
            generator_.seed(new_seed);
        }
    };
    
    // Global random generator
    RandomGenerator& getGlobalGenerator();
    
} // namespace Random

// ===== GEOMETRY UTILITIES =====

namespace Geometry {
    
    // Distance calculations
    template<typename T>
    T pointToLineDistance(const Vector<T, 2>& point, const Vector<T, 2>& line_start, const Vector<T, 2>& line_end) {
        Vector<T, 2> line_vec = line_end - line_start;
        Vector<T, 2> point_vec = point - line_start;
        
        T line_len_sq = line_vec.lengthSquared();
        if (line_len_sq == T(0)) {
            return point_vec.length();
        }
        
        T t = std::max(T(0), std::min(T(1), point_vec.dot(line_vec) / line_len_sq));
        Vector<T, 2> projection = line_start + line_vec * t;
        return (point - projection).length();
    }
    
    // Intersection tests
    template<typename T>
    bool rayCircleIntersection(const Vector<T, 2>& ray_origin, const Vector<T, 2>& ray_direction,
                              const Vector<T, 2>& circle_center, T circle_radius, T& t1, T& t2) {
        Vector<T, 2> oc = ray_origin - circle_center;
        T a = ray_direction.dot(ray_direction);
        T b = T(2) * oc.dot(ray_direction);
        T c = oc.dot(oc) - circle_radius * circle_radius;
        
        T discriminant = b * b - T(4) * a * c;
        if (discriminant < T(0)) {
            return false;
        }
        
        T sqrt_discriminant = std::sqrt(discriminant);
        t1 = (-b - sqrt_discriminant) / (T(2) * a);
        t2 = (-b + sqrt_discriminant) / (T(2) * a);
        
        return true;
    }
    
    // Area calculations
    template<typename T>
    T triangleArea(const Vector<T, 2>& a, const Vector<T, 2>& b, const Vector<T, 2>& c) {
        return std::abs((b.x() - a.x()) * (c.y() - a.y()) - (c.x() - a.x()) * (b.y() - a.y())) / T(2);
    }
    
    template<typename T>
    T polygonArea(const std::vector<Vector<T, 2>>& vertices) {
        if (vertices.size() < 3) return T(0);
        
        T area = T(0);
        size_t n = vertices.size();
        
        for (size_t i = 0; i < n; ++i) {
            size_t j = (i + 1) % n;
            area += vertices[i].x() * vertices[j].y();
            area -= vertices[j].x() * vertices[i].y();
        }
        
        return std::abs(area) / T(2);
    }
    
    // Point in polygon test
    template<typename T>
    bool pointInPolygon(const Vector<T, 2>& point, const std::vector<Vector<T, 2>>& vertices) {
        bool inside = false;
        size_t n = vertices.size();
        
        for (size_t i = 0, j = n - 1; i < n; j = i++) {
            if (((vertices[i].y() > point.y()) != (vertices[j].y() > point.y())) &&
                (point.x() < (vertices[j].x() - vertices[i].x()) * (point.y() - vertices[i].y()) / 
                            (vertices[j].y() - vertices[i].y()) + vertices[i].x())) {
                inside = !inside;
            }
        }
        
        return inside;
    }
    
} // namespace Geometry

} // namespace CppVerseHub::Utils::Math