// File: src/utils/MathUtils.cpp
// Mathematical Computations Implementation

#include "MathUtils.hpp"
#include <iostream>
#include <iomanip>

namespace CppVerseHub::Utils::Math {

// ===== FAST MATH IMPLEMENTATIONS =====

float fastInverseSqrt(float x) {
    // Famous Quake III inverse square root
    float x2 = x * 0.5f;
    std::int32_t i = *reinterpret_cast<std::int32_t*>(&x);
    i = 0x5f3759df - (i >> 1);
    x = *reinterpret_cast<float*>(&i);
    x = x * (1.5f - (x2 * x * x));  // Newton-Raphson iteration
    return x;
}

double accurateInverseSqrt(double x) {
    if (x <= 0.0) return 0.0;
    
    double guess = 1.0 / std::sqrt(x);  // Initial guess
    
    // Newton-Raphson iterations for higher precision
    for (int i = 0; i < 3; ++i) {
        guess = 0.5 * guess * (3.0 - x * guess * guess);
    }
    
    return guess;
}

// ===== TRANSFORMATION MATRIX IMPLEMENTATIONS =====

namespace Transform {
    
    Mat3f translation2D(float tx, float ty) {
        Mat3f result = Mat3f::identity();
        result[0][2] = tx;
        result[1][2] = ty;
        return result;
    }
    
    Mat3f rotation2D(float angle) {
        Mat3f result = Mat3f::identity();
        float cos_a = std::cos(angle);
        float sin_a = std::sin(angle);
        
        result[0][0] = cos_a;  result[0][1] = -sin_a;
        result[1][0] = sin_a;  result[1][1] = cos_a;
        
        return result;
    }
    
    Mat3f scale2D(float sx, float sy) {
        Mat3f result = Mat3f::identity();
        result[0][0] = sx;
        result[1][1] = sy;
        return result;
    }
    
    Mat4f translation3D(float tx, float ty, float tz) {
        Mat4f result = Mat4f::identity();
        result[0][3] = tx;
        result[1][3] = ty;
        result[2][3] = tz;
        return result;
    }
    
    Mat4f rotationX3D(float angle) {
        Mat4f result = Mat4f::identity();
        float cos_a = std::cos(angle);
        float sin_a = std::sin(angle);
        
        result[1][1] = cos_a;  result[1][2] = -sin_a;
        result[2][1] = sin_a;  result[2][2] = cos_a;
        
        return result;
    }
    
    Mat4f rotationY3D(float angle) {
        Mat4f result = Mat4f::identity();
        float cos_a = std::cos(angle);
        float sin_a = std::sin(angle);
        
        result[0][0] = cos_a;   result[0][2] = sin_a;
        result[2][0] = -sin_a;  result[2][2] = cos_a;
        
        return result;
    }
    
    Mat4f rotationZ3D(float angle) {
        Mat4f result = Mat4f::identity();
        float cos_a = std::cos(angle);
        float sin_a = std::sin(angle);
        
        result[0][0] = cos_a;  result[0][1] = -sin_a;
        result[1][0] = sin_a;  result[1][1] = cos_a;
        
        return result;
    }
    
    Mat4f rotation3D(float angle, const Vec3f& axis) {
        Vec3f normalized_axis = axis.normalized();
        float cos_a = std::cos(angle);
        float sin_a = std::sin(angle);
        float one_minus_cos = 1.0f - cos_a;
        
        float x = normalized_axis.x();
        float y = normalized_axis.y();
        float z = normalized_axis.z();
        
        Mat4f result;
        
        // Rodrigues' rotation formula
        result[0][0] = cos_a + x * x * one_minus_cos;
        result[0][1] = x * y * one_minus_cos - z * sin_a;
        result[0][2] = x * z * one_minus_cos + y * sin_a;
        result[0][3] = 0.0f;
        
        result[1][0] = y * x * one_minus_cos + z * sin_a;
        result[1][1] = cos_a + y * y * one_minus_cos;
        result[1][2] = y * z * one_minus_cos - x * sin_a;
        result[1][3] = 0.0f;
        
        result[2][0] = z * x * one_minus_cos - y * sin_a;
        result[2][1] = z * y * one_minus_cos + x * sin_a;
        result[2][2] = cos_a + z * z * one_minus_cos;
        result[2][3] = 0.0f;
        
        result[3][0] = result[3][1] = result[3][2] = 0.0f;
        result[3][3] = 1.0f;
        
        return result;
    }
    
    Mat4f scale3D(float sx, float sy, float sz) {
        Mat4f result = Mat4f::identity();
        result[0][0] = sx;
        result[1][1] = sy;
        result[2][2] = sz;
        return result;
    }
    
    Mat4f lookAt(const Vec3f& eye, const Vec3f& target, const Vec3f& up) {
        Vec3f forward = (target - eye).normalized();
        Vec3f right = forward.cross(up.normalized()).normalized();
        Vec3f new_up = right.cross(forward);
        
        Mat4f result;
        
        result[0][0] = right.x();    result[0][1] = right.y();    result[0][2] = right.z();    result[0][3] = -right.dot(eye);
        result[1][0] = new_up.x();   result[1][1] = new_up.y();   result[1][2] = new_up.z();   result[1][3] = -new_up.dot(eye);
        result[2][0] = -forward.x(); result[2][1] = -forward.y(); result[2][2] = -forward.z(); result[2][3] = forward.dot(eye);
        result[3][0] = 0.0f;         result[3][1] = 0.0f;         result[3][2] = 0.0f;         result[3][3] = 1.0f;
        
        return result;
    }
    
    Mat4f perspective(float fov, float aspect, float near, float far) {
        Mat4f result;
        
        float tan_half_fov = std::tan(fov * 0.5f);
        
        result[0][0] = 1.0f / (aspect * tan_half_fov);
        result[1][1] = 1.0f / tan_half_fov;
        result[2][2] = -(far + near) / (far - near);
        result[2][3] = -(2.0f * far * near) / (far - near);
        result[3][2] = -1.0f;
        result[3][3] = 0.0f;
        
        return result;
    }
    
    Mat4f orthographic(float left, float right, float bottom, float top, float near, float far) {
        Mat4f result;
        
        result[0][0] = 2.0f / (right - left);
        result[1][1] = 2.0f / (top - bottom);
        result[2][2] = -2.0f / (far - near);
        result[0][3] = -(right + left) / (right - left);
        result[1][3] = -(top + bottom) / (top - bottom);
        result[2][3] = -(far + near) / (far - near);
        result[3][3] = 1.0f;
        
        return result;
    }
    
} // namespace Transform

// ===== NOISE GENERATION IMPLEMENTATIONS =====

namespace Noise {
    
    PerlinNoise::PerlinNoise(unsigned int seed) {
        // Initialize permutation table
        std::mt19937 generator(seed);
        std::iota(permutation_.begin(), permutation_.begin() + 256, 0);
        std::shuffle(permutation_.begin(), permutation_.begin() + 256, generator);
        
        // Duplicate the permutation table
        std::copy(permutation_.begin(), permutation_.begin() + 256, 
                  permutation_.begin() + 256);
    }
    
    double PerlinNoise::grad(int hash, double x, double y, double z) const {
        int h = hash & 15;
        double u = h < 8 ? x : y;
        double v = h < 4 ? y : h == 12 || h == 14 ? x : z;
        return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
    }
    
    double PerlinNoise::noise(double x, double y, double z) const {
        int X = static_cast<int>(std::floor(x)) & 255;
        int Y = static_cast<int>(std::floor(y)) & 255;
        int Z = static_cast<int>(std::floor(z)) & 255;
        
        x -= std::floor(x);
        y -= std::floor(y);
        z -= std::floor(z);
        
        double u = fade(x);
        double v = fade(y);
        double w = fade(z);
        
        int A = permutation_[X] + Y;
        int AA = permutation_[A] + Z;
        int AB = permutation_[A + 1] + Z;
        int B = permutation_[X + 1] + Y;
        int BA = permutation_[B] + Z;
        int BB = permutation_[B + 1] + Z;
        
        return lerp(w, lerp(v, lerp(u, grad(permutation_[AA], x, y, z),
                                       grad(permutation_[BA], x - 1, y, z)),
                               lerp(u, grad(permutation_[AB], x, y - 1, z),
                                       grad(permutation_[BB], x - 1, y - 1, z))),
                       lerp(v, lerp(u, grad(permutation_[AA + 1], x, y, z - 1),
                                       grad(permutation_[BA + 1], x - 1, y, z - 1)),
                               lerp(u, grad(permutation_[AB + 1], x, y - 1, z - 1),
                                       grad(permutation_[BB + 1], x - 1, y - 1, z - 1))));
    }
    
    double PerlinNoise::octaveNoise(double x, double y, double z, int octaves, double persistence) const {
        double total = 0.0;
        double frequency = 1.0;
        double amplitude = 1.0;
        double max_value = 0.0;
        
        for (int i = 0; i < octaves; ++i) {
            total += noise(x * frequency, y * frequency, z * frequency) * amplitude;
            
            max_value += amplitude;
            amplitude *= persistence;
            frequency *= 2.0;
        }
        
        return total / max_value;
    }
    
    double PerlinNoise::turbulence(double x, double y, double z, int octaves) const {
        double value = 0.0;
        double amplitude = 1.0;
        
        for (int i = 0; i < octaves; ++i) {
            value += std::abs(noise(x, y, z)) * amplitude;
            x *= 2.0;
            y *= 2.0;
            z *= 2.0;
            amplitude *= 0.5;
        }
        
        return value;
    }
    
    SimplexNoise::SimplexNoise(unsigned int seed) {
        std::mt19937 generator(seed);
        std::iota(perm_.begin(), perm_.begin() + 256, 0);
        std::shuffle(perm_.begin(), perm_.begin() + 256, generator);
        
        // Duplicate permutation
        std::copy(perm_.begin(), perm_.begin() + 256, perm_.begin() + 256);
    }
    
    double SimplexNoise::noise(double x, double y) const {
        // Simplex noise implementation (2D)
        const double F2 = 0.5 * (std::sqrt(3.0) - 1.0);
        const double G2 = (3.0 - std::sqrt(3.0)) / 6.0;
        
        double n0, n1, n2;
        
        double s = (x + y) * F2;
        int i = static_cast<int>(std::floor(x + s));
        int j = static_cast<int>(std::floor(y + s));
        
        double t = (i + j) * G2;
        double X0 = i - t;
        double Y0 = j - t;
        double x0 = x - X0;
        double y0 = y - Y0;
        
        int i1, j1;
        if (x0 > y0) {
            i1 = 1; j1 = 0;
        } else {
            i1 = 0; j1 = 1;
        }
        
        double x1 = x0 - i1 + G2;
        double y1 = y0 - j1 + G2;
        double x2 = x0 - 1.0 + 2.0 * G2;
        double y2 = y0 - 1.0 + 2.0 * G2;
        
        int ii = i & 255;
        int jj = j & 255;
        int gi0 = perm_[ii + perm_[jj]] % 12;
        int gi1 = perm_[ii + i1 + perm_[jj + j1]] % 12;
        int gi2 = perm_[ii + 1 + perm_[jj + 1]] % 12;
        
        // Calculate noise contributions
        double t0 = 0.5 - x0 * x0 - y0 * y0;
        if (t0 < 0) {
            n0 = 0.0;
        } else {
            t0 *= t0;
            n0 = t0 * t0 * (x0 * (gi0 & 1 ? 1 : -1) + y0 * (gi0 & 2 ? 1 : -1));
        }
        
        double t1 = 0.5 - x1 * x1 - y1 * y1;
        if (t1 < 0) {
            n1 = 0.0;
        } else {
            t1 *= t1;
            n1 = t1 * t1 * (x1 * (gi1 & 1 ? 1 : -1) + y1 * (gi1 & 2 ? 1 : -1));
        }
        
        double t2 = 0.5 - x2 * x2 - y2 * y2;
        if (t2 < 0) {
            n2 = 0.0;
        } else {
            t2 *= t2;
            n2 = t2 * t2 * (x2 * (gi2 & 1 ? 1 : -1) + y2 * (gi2 & 2 ? 1 : -1));
        }
        
        return 70.0 * (n0 + n1 + n2);
    }
    
    double SimplexNoise::noise(double x, double y, double z) const {
        // Simplified 3D simplex noise
        return 0.5 * (noise(x, y) + noise(y + 0.1, z + 0.1));
    }
    
} // namespace Noise

// ===== SPACE CALCULATIONS IMPLEMENTATIONS =====

namespace Space {
    
    CartesianCoordinates orbitalToCartesian(const OrbitalElements& elements, double gravitational_parameter) {
        double a = elements.semi_major_axis;
        double e = elements.eccentricity;
        double E = elements.mean_anomaly;  // Simplified: assume E ≈ M
        
        // Calculate position in orbital plane
        double x_orbital = a * (std::cos(E) - e);
        double y_orbital = a * std::sqrt(1.0 - e * e) * std::sin(E);
        
        // Calculate velocity in orbital plane
        double n = std::sqrt(gravitational_parameter / (a * a * a));
        double vx_orbital = -n * a * std::sin(E) / (1.0 - e * std::cos(E));
        double vy_orbital = n * a * std::sqrt(1.0 - e * e) * std::cos(E) / (1.0 - e * std::cos(E));
        
        // Rotation matrices for orbital orientation
        double cos_omega = std::cos(elements.argument_periapsis);
        double sin_omega = std::sin(elements.argument_periapsis);
        double cos_Omega = std::cos(elements.longitude_ascending_node);
        double sin_Omega = std::sin(elements.longitude_ascending_node);
        double cos_i = std::cos(elements.inclination);
        double sin_i = std::sin(elements.inclination);
        
        // Transform to 3D space
        CartesianCoordinates result;
        
        result.position.x() = (cos_Omega * cos_omega - sin_Omega * sin_omega * cos_i) * x_orbital +
                             (-cos_Omega * sin_omega - sin_Omega * cos_omega * cos_i) * y_orbital;
        result.position.y() = (sin_Omega * cos_omega + cos_Omega * sin_omega * cos_i) * x_orbital +
                             (-sin_Omega * sin_omega + cos_Omega * cos_omega * cos_i) * y_orbital;
        result.position.z() = sin_omega * sin_i * x_orbital + cos_omega * sin_i * y_orbital;
        
        result.velocity.x() = (cos_Omega * cos_omega - sin_Omega * sin_omega * cos_i) * vx_orbital +
                             (-cos_Omega * sin_omega - sin_Omega * cos_omega * cos_i) * vy_orbital;
        result.velocity.y() = (sin_Omega * cos_omega + cos_Omega * sin_omega * cos_i) * vx_orbital +
                             (-sin_Omega * sin_omega + cos_Omega * cos_omega * cos_i) * vy_orbital;
        result.velocity.z() = sin_omega * sin_i * vx_orbital + cos_omega * sin_i * vy_orbital;
        
        return result;
    }
    
    double orbitalPeriod(double semi_major_axis, double gravitational_parameter) {
        return 2.0 * Constants::PI * std::sqrt((semi_major_axis * semi_major_axis * semi_major_axis) / 
                                              gravitational_parameter);
    }
    
    double escapeVelocity(double mass, double radius) {
        return std::sqrt(2.0 * Constants::GRAVITATIONAL_CONSTANT * mass / radius);
    }
    
    Vec3d gravitationalForce(double mass1, double mass2, const Vec3d& position1, const Vec3d& position2) {
        Vec3d direction = position2 - position1;
        double distance = direction.length();
        
        if (distance < std::numeric_limits<double>::epsilon()) {
            return Vec3d(0, 0, 0);
        }
        
        double force_magnitude = Constants::GRAVITATIONAL_CONSTANT * mass1 * mass2 / (distance * distance);
        return direction.normalized() * force_magnitude;
    }
    
    HohmannTransfer calculateHohmannTransfer(double r1, double r2, double gravitational_parameter) {
        HohmannTransfer transfer;
        
        transfer.transfer_semi_major_axis = (r1 + r2) / 2.0;
        
        double v1 = std::sqrt(gravitational_parameter / r1);
        double v2 = std::sqrt(gravitational_parameter / r2);
        double v_transfer_1 = std::sqrt(gravitational_parameter * (2.0 / r1 - 2.0 / (r1 + r2)));
        double v_transfer_2 = std::sqrt(gravitational_parameter * (2.0 / r2 - 2.0 / (r1 + r2)));
        
        transfer.delta_v1 = std::abs(v_transfer_1 - v1);
        transfer.delta_v2 = std::abs(v2 - v_transfer_2);
        transfer.transfer_time = Constants::PI * std::sqrt((r1 + r2) * (r1 + r2) * (r1 + r2) / 
                                                          (8.0 * gravitational_parameter));
        
        return transfer;
    }
    
    void NBodySimulator::addBody(double mass, const Vec3d& position, const Vec3d& velocity) {
        bodies_.push_back({mass, position, velocity, Vec3d(0, 0, 0)});
    }
    
    void NBodySimulator::step() {
        // Calculate accelerations
        for (size_t i = 0; i < bodies_.size(); ++i) {
            bodies_[i].acceleration = Vec3d(0, 0, 0);
            
            for (size_t j = 0; j < bodies_.size(); ++j) {
                if (i == j) continue;
                
                Vec3d force = gravitationalForce(bodies_[i].mass, bodies_[j].mass,
                                               bodies_[i].position, bodies_[j].position);
                bodies_[i].acceleration += force / bodies_[i].mass;
            }
        }
        
        // Update positions and velocities (Verlet integration)
        for (auto& body : bodies_) {
            body.position += body.velocity * time_step_ + body.acceleration * (0.5 * time_step_ * time_step_);
            body.velocity += body.acceleration * time_step_;
        }
    }
    
    void NBodySimulator::simulate(double duration) {
        double elapsed_time = 0.0;
        while (elapsed_time < duration) {
            step();
            elapsed_time += time_step_;
        }
    }
    
} // namespace Space

// ===== RANDOM GENERATOR IMPLEMENTATION =====

namespace Random {
    
    RandomGenerator& getGlobalGenerator() {
        static RandomGenerator generator;
        return generator;
    }
    
} // namespace Random

// ===== DEMONSTRATION FUNCTIONS =====

void demonstrateMathUtils() {
    std::cout << "\n=== Mathematical Utilities Demonstration ===" << std::endl;
    
    // Vector operations
    std::cout << "\n--- Vector Operations ---" << std::endl;
    Vec3f vec1(1.0f, 2.0f, 3.0f);
    Vec3f vec2(4.0f, 5.0f, 6.0f);
    
    Vec3f sum = vec1 + vec2;
    float dot_product = vec1.dot(vec2);
    Vec3f cross_product = vec1.cross(vec2);
    
    std::cout << "vec1 + vec2 = (" << sum.x() << ", " << sum.y() << ", " << sum.z() << ")" << std::endl;
    std::cout << "vec1 · vec2 = " << dot_product << std::endl;
    std::cout << "vec1 × vec2 = (" << cross_product.x() << ", " << cross_product.y() << ", " << cross_product.z() << ")" << std::endl;
    std::cout << "Length of vec1: " << vec1.length() << std::endl;
    std::cout << "Angle between vectors: " << (vec1.angleTo(vec2) * Constants::RAD_TO_DEG) << " degrees" << std::endl;
    
    // Matrix operations
    std::cout << "\n--- Matrix Operations ---" << std::endl;
    Mat3f mat1 = Mat3f::identity();
    mat1[0][1] = 2.0f;
    mat1[1][0] = 3.0f;
    
    Mat3f mat2 = Mat3f::identity();
    mat2[0][0] = 2.0f;
    mat2[1][1] = 2.0f;
    mat2[2][2] = 2.0f;
    
    Mat3f product = mat1 * mat2;
    std::cout << "Matrix multiplication result[0][0]: " << product[0][0] << std::endl;
    std::cout << "Matrix determinant: " << mat1.determinant() << std::endl;
    
    // Transformation matrices
    std::cout << "\n--- Transformations ---" << std::endl;
    Mat3f rotation = Transform::rotation2D(Constants::PI / 4.0f); // 45 degrees
    Mat3f translation = Transform::translation2D(5.0f, 3.0f);
    Mat3f combined = translation * rotation;
    
    Vec3f point(1.0f, 0.0f, 1.0f); // Homogeneous coordinates
    Vec3f transformed = combined * point;
    std::cout << "Transformed point: (" << transformed.x() << ", " << transformed.y() << ")" << std::endl;
    
    // Noise generation
    std::cout << "\n--- Noise Generation ---" << std::endl;
    Noise::PerlinNoise perlin(42);
    
    std::cout << "Perlin noise samples:" << std::endl;
    for (int i = 0; i < 5; ++i) {
        double noise_val = perlin.noise(i * 0.1, 0.0, 0.0);
        std::cout << "  noise(" << (i * 0.1) << ") = " << std::fixed << std::setprecision(4) << noise_val << std::endl;
    }
    
    double octave_noise = perlin.octaveNoise(1.0, 1.0, 0.0, 4, 0.5);
    std::cout << "Octave noise (4 octaves): " << octave_noise << std::endl;
    
    // Statistics
    std::cout << "\n--- Statistics ---" << std::endl;
    std::vector<double> data = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0};
    
    double mean_val = Statistics::mean(data);
    double variance_val = Statistics::variance(data);
    double stddev_val = Statistics::standardDeviation(data);
    double median_val = Statistics::median(data);
    
    std::cout << "Dataset: ";
    for (const auto& val : data) std::cout << val << " ";
    std::cout << std::endl;
    std::cout << "Mean: " << mean_val << std::endl;
    std::cout << "Variance: " << variance_val << std::endl;
    std::cout << "Standard Deviation: " << stddev_val << std::endl;
    std::cout << "Median: " << median_val << std::endl;
    
    // Random number generation
    std::cout << "\n--- Random Numbers ---" << std::endl;
    auto& rng = Random::getGlobalGenerator();
    
    std::cout << "Uniform random numbers (0-1): ";
    for (int i = 0; i < 5; ++i) {
        std::cout << std::fixed << std::setprecision(3) << rng.uniform() << " ";
    }
    std::cout << std::endl;
    
    std::cout << "Normal distribution (μ=0, σ=1): ";
    for (int i = 0; i < 5; ++i) {
        std::cout << std::fixed << std::setprecision(3) << rng.normal() << " ";
    }
    std::cout << std::endl;
    
    Vec3d random_sphere_point = rng.randomUnitSphere();
    std::cout << "Random point on unit sphere: (" 
              << random_sphere_point.x() << ", " 
              << random_sphere_point.y() << ", " 
              << random_sphere_point.z() << ")" << std::endl;
    
    // Space calculations
    std::cout << "\n--- Space Calculations ---" << std::endl;
    
    // Earth orbital mechanics
    double earth_orbital_period = Space::orbitalPeriod(Constants::ASTRONOMICAL_UNIT, 
                                                      Constants::GRAVITATIONAL_CONSTANT * Constants::SOLAR_MASS);
    std::cout << "Earth orbital period: " << (earth_orbital_period / (24 * 3600)) << " days" << std::endl;
    
    double earth_escape_velocity = Space::escapeVelocity(Constants::EARTH_MASS, Constants::EARTH_RADIUS);
    std::cout << "Earth escape velocity: " << (earth_escape_velocity / 1000.0) << " km/s" << std::endl;
    
    // Hohmann transfer (Earth to Mars)
    Space::HohmannTransfer transfer = Space::calculateHohmannTransfer(
        Constants::ASTRONOMICAL_UNIT, 1.52 * Constants::ASTRONOMICAL_UNIT,
        Constants::GRAVITATIONAL_CONSTANT * Constants::SOLAR_MASS);
    
    std::cout << "Earth to Mars Hohmann transfer:" << std::endl;
    std::cout << "  Delta-v 1: " << (transfer.delta_v1 / 1000.0) << " km/s" << std::endl;
    std::cout << "  Delta-v 2: " << (transfer.delta_v2 / 1000.0) << " km/s" << std::endl;
    std::cout << "  Transfer time: " << (transfer.transfer_time / (24 * 3600)) << " days" << std::endl;
    
    // Interpolation
    std::cout << "\n--- Interpolation ---" << std::endl;
    double a = 10.0, b = 20.0, t = 0.3;
    
    std::cout << "Linear interpolation: " << Interpolation::linear(a, b, t) << std::endl;
    std::cout << "Cosine interpolation: " << Interpolation::cosine(a, b, t) << std::endl;
    std::cout << "Cubic Bezier: " << Interpolation::cubicBezier(a, 15.0, 18.0, b, t) << std::endl;
    
    // Geometry
    std::cout << "\n--- Geometry ---" << std::endl;
    std::vector<Vec2f> triangle = {
        Vec2f(0.0f, 0.0f),
        Vec2f(3.0f, 0.0f),
        Vec2f(1.5f, 2.0f)
    };
    
    float triangle_area = Geometry::triangleArea(triangle[0], triangle[1], triangle[2]);
    std::cout << "Triangle area: " << triangle_area << std::endl;
    
    Vec2f test_point(1.0f, 1.0f);
    bool inside = Geometry::pointInPolygon(test_point, triangle);
    std::cout << "Point (1, 1) inside triangle: " << (inside ? "Yes" : "No") << std::endl;
    
    // Distance calculations
    Vec2f line_start(0.0f, 0.0f);
    Vec2f line_end(5.0f, 0.0f);
    Vec2f point(2.5f, 3.0f);
    float point_line_distance = Geometry::pointToLineDistance(point, line_start, line_end);
    std::cout << "Point to line distance: " << point_line_distance << std::endl;
    
    // Ray-circle intersection
    Vec2f ray_origin(0.0f, 0.0f);
    Vec2f ray_direction(1.0f, 1.0f);
    Vec2f circle_center(3.0f, 3.0f);
    float circle_radius = 1.5f;
    float t1, t2;
    
    bool intersects = Geometry::rayCircleIntersection(ray_origin, ray_direction.normalized(), 
                                                      circle_center, circle_radius, t1, t2);
    if (intersects) {
        std::cout << "Ray intersects circle at t = " << t1 << " and t = " << t2 << std::endl;
    } else {
        std::cout << "Ray does not intersect circle" << std::endl;
    }
    
    std::cout << "\n✨ Mathematical utilities demonstration complete! ✨" << std::endl;
}

void performanceBenchmarks() {
    std::cout << "\n=== Mathematical Performance Benchmarks ===" << std::endl;
    
    const int iterations = 1000000;
    
    // Vector operations benchmark
    auto start_time = std::chrono::high_resolution_clock::now();
    
    Vec3f result(0, 0, 0);
    for (int i = 0; i < iterations; ++i) {
        Vec3f v1(static_cast<float>(i), static_cast<float>(i + 1), static_cast<float>(i + 2));
        Vec3f v2(static_cast<float>(i + 3), static_cast<float>(i + 4), static_cast<float>(i + 5));
        result += v1.cross(v2);
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto vector_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "Vector cross products (" << iterations << "): " << vector_duration.count() << " ms" << std::endl;
    std::cout << "Operations per second: " << (iterations * 1000 / vector_duration.count()) << std::endl;
    
    // Matrix multiplication benchmark
    start_time = std::chrono::high_resolution_clock::now();
    
    Mat3f mat_result = Mat3f::identity();
    for (int i = 0; i < iterations / 1000; ++i) {  // Fewer iterations for matrix ops
        Mat3f m1 = Transform::rotation2D(static_cast<float>(i) * 0.001f);
        Mat3f m2 = Transform::scale2D(1.1f, 1.1f);
        mat_result = mat_result * m1 * m2;
    }
    
    end_time = std::chrono::high_resolution_clock::now();
    auto matrix_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "Matrix multiplications (" << (iterations / 1000) << "): " << matrix_duration.count() << " ms" << std::endl;
    
    // Fast vs accurate inverse square root
    start_time = std::chrono::high_resolution_clock::now();
    
    float fast_sum = 0.0f;
    for (int i = 1; i < iterations; ++i) {
        fast_sum += fastInverseSqrt(static_cast<float>(i));
    }
    
    end_time = std::chrono::high_resolution_clock::now();
    auto fast_isqrt_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    start_time = std::chrono::high_resolution_clock::now();
    
    double accurate_sum = 0.0;
    for (int i = 1; i < iterations; ++i) {
        accurate_sum += accurateInverseSqrt(static_cast<double>(i));
    }
    
    end_time = std::chrono::high_resolution_clock::now();
    auto accurate_isqrt_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "Fast inverse sqrt: " << fast_isqrt_duration.count() << " ms" << std::endl;
    std::cout << "Accurate inverse sqrt: " << accurate_isqrt_duration.count() << " ms" << std::endl;
    std::cout << "Speedup: " << (static_cast<double>(accurate_isqrt_duration.count()) / fast_isqrt_duration.count()) << "x" << std::endl;
    
    // Noise generation benchmark
    start_time = std::chrono::high_resolution_clock::now();
    
    Noise::PerlinNoise perlin(42);
    double noise_sum = 0.0;
    for (int i = 0; i < iterations / 100; ++i) {  // Noise is expensive
        noise_sum += perlin.noise(i * 0.01, i * 0.01, 0.0);
    }
    
    end_time = std::chrono::high_resolution_clock::now();
    auto noise_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "Perlin noise generation (" << (iterations / 100) << "): " << noise_duration.count() << " ms" << std::endl;
    
    std::cout << "\nBenchmark results summary:" << std::endl;
    std::cout << "- Vector operations: " << (iterations / vector_duration.count()) << " ops/ms" << std::endl;
    std::cout << "- Matrix operations: " << ((iterations / 1000) / matrix_duration.count()) << " ops/ms" << std::endl;
    std::cout << "- Noise generation: " << ((iterations / 100) / noise_duration.count()) << " ops/ms" << std::endl;
}

void demonstrateSpacePhysics() {
    std::cout << "\n=== Space Physics Simulation ===" << std::endl;
    
    // Create a simple solar system simulation
    Space::NBodySimulator simulator(0.01); // 0.01 day time step
    
    // Add Sun
    simulator.addBody(Constants::SOLAR_MASS, Vec3d(0, 0, 0), Vec3d(0, 0, 0));
    
    // Add Earth
    double earth_orbital_velocity = std::sqrt(Constants::GRAVITATIONAL_CONSTANT * Constants::SOLAR_MASS / 
                                            Constants::ASTRONOMICAL_UNIT);
    simulator.addBody(Constants::EARTH_MASS, 
                     Vec3d(Constants::ASTRONOMICAL_UNIT, 0, 0),
                     Vec3d(0, earth_orbital_velocity, 0));
    
    // Add Mars
    double mars_distance = 1.52 * Constants::ASTRONOMICAL_UNIT;
    double mars_orbital_velocity = std::sqrt(Constants::GRAVITATIONAL_CONSTANT * Constants::SOLAR_MASS / mars_distance);
    simulator.addBody(0.107 * Constants::EARTH_MASS,  // Mars mass
                     Vec3d(mars_distance, 0, 0),
                     Vec3d(0, mars_orbital_velocity, 0));
    
    std::cout << "Initial positions (AU):" << std::endl;
    std::cout << "  Sun: (0.0, 0.0, 0.0)" << std::endl;
    std::cout << "  Earth: (" << (simulator.getBodyPosition(1).x() / Constants::ASTRONOMICAL_UNIT) << ", 0.0, 0.0)" << std::endl;
    std::cout << "  Mars: (" << (simulator.getBodyPosition(2).x() / Constants::ASTRONOMICAL_UNIT) << ", 0.0, 0.0)" << std::endl;
    
    // Simulate for 1 year (365.25 days)
    std::cout << "\nSimulating for 1 year..." << std::endl;
    simulator.simulate(365.25);
    
    std::cout << "Final positions after 1 year (AU):" << std::endl;
    Vec3d earth_final = simulator.getBodyPosition(1);
    Vec3d mars_final = simulator.getBodyPosition(2);
    
    std::cout << "  Earth: (" 
              << std::fixed << std::setprecision(3) << (earth_final.x() / Constants::ASTRONOMICAL_UNIT) << ", "
              << (earth_final.y() / Constants::ASTRONOMICAL_UNIT) << ", "
              << (earth_final.z() / Constants::ASTRONOMICAL_UNIT) << ")" << std::endl;
    std::cout << "  Mars: ("
              << (mars_final.x() / Constants::ASTRONOMICAL_UNIT) << ", "
              << (mars_final.y() / Constants::ASTRONOMICAL_UNIT) << ", "
              << (mars_final.z() / Constants::ASTRONOMICAL_UNIT) << ")" << std::endl;
    
    // Check if Earth completed approximately one orbit
    double earth_distance = earth_final.length() / Constants::ASTRONOMICAL_UNIT;
    std::cout << "Earth's final distance from Sun: " << earth_distance << " AU" << std::endl;
    std::cout << "Expected distance: ~1.0 AU" << std::endl;
    
    if (std::abs(earth_distance - 1.0) < 0.1) {
        std::cout << "✓ Earth orbit simulation successful!" << std::endl;
    } else {
        std::cout << "⚠ Earth orbit simulation may need refinement" << std::endl;
    }
}

void demonstrateAdvancedMath() {
    std::cout << "\n=== Advanced Mathematical Concepts ===" << std::endl;
    
    // Fourier transform approximation using interpolation
    std::cout << "\n--- Signal Processing with Interpolation ---" << std::endl;
    
    // Generate a test signal
    std::vector<double> signal_samples;
    const int num_samples = 64;
    
    for (int i = 0; i < num_samples; ++i) {
        double t = static_cast<double>(i) / num_samples;
        // Composite signal: sine wave + noise
        double signal = std::sin(2.0 * Constants::PI * 3.0 * t) + 
                       0.5 * std::sin(2.0 * Constants::PI * 7.0 * t) +
                       0.1 * (Random::getGlobalGenerator().uniform() - 0.5);
        signal_samples.push_back(signal);
    }
    
    std::cout << "Generated test signal with " << num_samples << " samples" << std::endl;
    std::cout << "Signal statistics:" << std::endl;
    std::cout << "  Mean: " << Statistics::mean(signal_samples) << std::endl;
    std::cout << "  Std Dev: " << Statistics::standardDeviation(signal_samples) << std::endl;
    std::cout << "  Min/Max: " << *std::min_element(signal_samples.begin(), signal_samples.end())
              << " / " << *std::max_element(signal_samples.begin(), signal_samples.end()) << std::endl;
    
    // Demonstrate spline interpolation for upsampling
    std::vector<double> upsampled_signal;
    for (int i = 0; i < num_samples - 1; ++i) {
        upsampled_signal.push_back(signal_samples[i]);
        
        // Interpolate between samples
        for (int j = 1; j < 4; ++j) {
            double t = static_cast<double>(j) / 4.0;
            double interpolated;
            
            if (i == 0) {
                interpolated = Interpolation::linear(signal_samples[i], signal_samples[i + 1], t);
            } else if (i == num_samples - 2) {
                interpolated = Interpolation::linear(signal_samples[i], signal_samples[i + 1], t);
            } else {
                interpolated = Interpolation::catmullRom(
                    signal_samples[i - 1], signal_samples[i], 
                    signal_samples[i + 1], signal_samples[i + 2], t);
            }
            
            upsampled_signal.push_back(interpolated);
        }
    }
    upsampled_signal.push_back(signal_samples.back());
    
    std::cout << "Upsampled signal to " << upsampled_signal.size() << " samples using Catmull-Rom splines" << std::endl;
    
    // Matrix eigenvalue approximation (power iteration)
    std::cout << "\n--- Matrix Eigenvalue Estimation ---" << std::endl;
    
    Mat3f test_matrix;
    test_matrix[0][0] = 4.0f;  test_matrix[0][1] = 1.0f;  test_matrix[0][2] = 0.0f;
    test_matrix[1][0] = 1.0f;  test_matrix[1][1] = 3.0f;  test_matrix[1][2] = 1.0f;
    test_matrix[2][0] = 0.0f;  test_matrix[2][1] = 1.0f;  test_matrix[2][2] = 2.0f;
    
    // Power iteration to find dominant eigenvalue
    Vec3f eigenvector(1.0f, 1.0f, 1.0f);  // Initial guess
    float eigenvalue = 0.0f;
    
    for (int iter = 0; iter < 20; ++iter) {
        Vec3f new_vector = test_matrix * eigenvector;
        eigenvalue = new_vector.length();
        eigenvector = new_vector.normalized();
    }
    
    std::cout << "Dominant eigenvalue (approximation): " << eigenvalue << std::endl;
    std::cout << "Corresponding eigenvector: (" 
              << eigenvector.x() << ", " << eigenvector.y() << ", " << eigenvector.z() << ")" << std::endl;
    
    // Verify: A * v ≈ λ * v
    Vec3f verification = test_matrix * eigenvector;
    Vec3f expected = eigenvector * eigenvalue;
    Vec3f error = verification - expected;
    std::cout << "Verification error magnitude: " << error.length() << std::endl;
    
    // Numerical differentiation
    std::cout << "\n--- Numerical Differentiation ---" << std::endl;
    
    auto test_function = [](double x) { return x * x * x - 2.0 * x * x + x - 1.0; };  // f(x) = x³ - 2x² + x - 1
    auto analytical_derivative = [](double x) { return 3.0 * x * x - 4.0 * x + 1.0; };  // f'(x) = 3x² - 4x + 1
    
    double test_point = 2.0;
    double h = 0.001;
    
    // Forward difference
    double forward_diff = (test_function(test_point + h) - test_function(test_point)) / h;
    
    // Central difference
    double central_diff = (test_function(test_point + h) - test_function(test_point - h)) / (2.0 * h);
    
    // Analytical result
    double analytical = analytical_derivative(test_point);
    
    std::cout << "Numerical differentiation at x = " << test_point << ":" << std::endl;
    std::cout << "  Forward difference: " << forward_diff << std::endl;
    std::cout << "  Central difference: " << central_diff << std::endl;
    std::cout << "  Analytical: " << analytical << std::endl;
    std::cout << "  Forward error: " << std::abs(forward_diff - analytical) << std::endl;
    std::cout << "  Central error: " << std::abs(central_diff - analytical) << std::endl;
    
    // Numerical integration (Simpson's rule)
    std::cout << "\n--- Numerical Integration ---" << std::endl;
    
    auto integrand = [](double x) { return x * x; };  // ∫x²dx = x³/3
    
    double a = 0.0, b = 2.0;
    int n = 1000;  // Number of intervals (must be even for Simpson's rule)
    
    double h_int = (b - a) / n;
    double simpson_sum = integrand(a) + integrand(b);
    
    for (int i = 1; i < n; ++i) {
        double x = a + i * h_int;
        if (i % 2 == 0) {
            simpson_sum += 2.0 * integrand(x);
        } else {
            simpson_sum += 4.0 * integrand(x);
        }
    }
    
    double simpson_result = (h_int / 3.0) * simpson_sum;
    double analytical_integral = (b * b * b - a * a * a) / 3.0;  // x³/3 from 0 to 2
    
    std::cout << "∫₀² x² dx using Simpson's rule: " << simpson_result << std::endl;
    std::cout << "Analytical result: " << analytical_integral << std::endl;
    std::cout << "Integration error: " << std::abs(simpson_result - analytical_integral) << std::endl;
}

} // namespace CppVerseHub::Utils::Math