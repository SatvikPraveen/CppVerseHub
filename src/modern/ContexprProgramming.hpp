// File: src/modern/ConstexprProgramming.hpp
// Compile-time Programming with constexpr Demonstration

#pragma once

#include <array>
#include <string_view>
#include <algorithm>
#include <cmath>
#include <type_traits>
#include <iostream>

namespace CppVerseHub::Modern::ConstexprProgramming {

// ===== COMPILE-TIME CONSTANTS =====

constexpr double PI = 3.14159265358979323846;
constexpr double LIGHT_SPEED = 299792458.0; // m/s
constexpr double EARTH_MASS = 5.972e24; // kg
constexpr double GRAVITATIONAL_CONSTANT = 6.67430e-11; // m³/kg⋅s²

constexpr int MAX_PLANETS = 100;
constexpr int MAX_FLEETS = 50;
constexpr int MAX_MISSIONS = 200;

// ===== COMPILE-TIME MATHEMATICAL FUNCTIONS =====

constexpr double power(double base, int exponent) {
    if (exponent == 0) return 1.0;
    if (exponent == 1) return base;
    if (exponent < 0) return 1.0 / power(base, -exponent);
    
    double result = 1.0;
    for (int i = 0; i < exponent; ++i) {
        result *= base;
    }
    return result;
}

constexpr double factorial(int n) {
    if (n <= 1) return 1.0;
    double result = 1.0;
    for (int i = 2; i <= n; ++i) {
        result *= i;
    }
    return result;
}

constexpr double sqrt_newton(double x, double guess = 1.0, int iterations = 10) {
    if (iterations == 0) return guess;
    return sqrt_newton(x, (guess + x / guess) / 2.0, iterations - 1);
}

constexpr double sqrt_constexpr(double x) {
    return (x >= 0) ? sqrt_newton(x) : 0.0;
}

// ===== COMPILE-TIME SPACE CALCULATIONS =====

constexpr double orbital_velocity(double mass, double radius) {
    return sqrt_constexpr(GRAVITATIONAL_CONSTANT * mass / radius);
}

constexpr double escape_velocity(double mass, double radius) {
    return sqrt_constexpr(2.0 * GRAVITATIONAL_CONSTANT * mass / radius);
}

constexpr double orbital_period(double semi_major_axis, double central_mass) {
    return 2.0 * PI * sqrt_constexpr(power(semi_major_axis, 3) / (GRAVITATIONAL_CONSTANT * central_mass));
}

constexpr double distance_3d(double x1, double y1, double z1, double x2, double y2, double z2) {
    double dx = x2 - x1;
    double dy = y2 - y1;
    double dz = z2 - z1;
    return sqrt_constexpr(dx*dx + dy*dy + dz*dz);
}

constexpr double sphere_volume(double radius) {
    return (4.0 / 3.0) * PI * power(radius, 3);
}

constexpr double sphere_surface_area(double radius) {
    return 4.0 * PI * radius * radius;
}

// ===== COMPILE-TIME STRING PROCESSING =====

constexpr size_t string_length(const char* str) {
    size_t length = 0;
    while (str[length] != '\0') {
        ++length;
    }
    return length;
}

constexpr bool strings_equal(const char* str1, const char* str2) {
    size_t i = 0;
    while (str1[i] != '\0' && str2[i] != '\0') {
        if (str1[i] != str2[i]) {
            return false;
        }
        ++i;
    }
    return str1[i] == str2[i];
}

constexpr char to_upper(char c) {
    return (c >= 'a' && c <= 'z') ? c - ('a' - 'A') : c;
}

constexpr bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

constexpr bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

// ===== COMPILE-TIME ARRAY OPERATIONS =====

template<typename T, size_t N>
constexpr T array_sum(const std::array<T, N>& arr) {
    T sum = T{};
    for (size_t i = 0; i < N; ++i) {
        sum += arr[i];
    }
    return sum;
}

template<typename T, size_t N>
constexpr T array_max(const std::array<T, N>& arr) {
    if constexpr (N == 0) return T{};
    
    T max_val = arr[0];
    for (size_t i = 1; i < N; ++i) {
        if (arr[i] > max_val) {
            max_val = arr[i];
        }
    }
    return max_val;
}

template<typename T, size_t N>
constexpr T array_min(const std::array<T, N>& arr) {
    if constexpr (N == 0) return T{};
    
    T min_val = arr[0];
    for (size_t i = 1; i < N; ++i) {
        if (arr[i] < min_val) {
            min_val = arr[i];
        }
    }
    return min_val;
}

template<size_t N>
constexpr std::array<int, N> generate_fibonacci() {
    std::array<int, N> fib{};
    if constexpr (N >= 1) fib[0] = 1;
    if constexpr (N >= 2) fib[1] = 1;
    
    for (size_t i = 2; i < N; ++i) {
        fib[i] = fib[i-1] + fib[i-2];
    }
    return fib;
}

template<size_t N>
constexpr std::array<int, N> generate_primes() {
    std::array<int, N> primes{};
    if constexpr (N == 0) return primes;
    
    primes[0] = 2;
    size_t count = 1;
    int candidate = 3;
    
    while (count < N) {
        bool is_prime = true;
        for (size_t i = 0; i < count && primes[i] * primes[i] <= candidate; ++i) {
            if (candidate % primes[i] == 0) {
                is_prime = false;
                break;
            }
        }
        
        if (is_prime) {
            primes[count++] = candidate;
        }
        candidate += 2; // Only check odd numbers after 2
    }
    
    return primes;
}

// ===== COMPILE-TIME SPACE GAME ENTITIES =====

enum class PlanetType : int {
    Terrestrial = 1,
    GasGiant = 2,
    IceGiant = 3,
    DwarfPlanet = 4
};

enum class MissionType : int {
    Exploration = 1,
    Combat = 2,
    Colonization = 3,
    Trade = 4,
    Rescue = 5
};

struct ConstexprPlanet {
    int id;
    PlanetType type;
    double mass;
    double radius;
    bool habitable;
    
    constexpr ConstexprPlanet(int planet_id, PlanetType planet_type, double planet_mass, 
                              double planet_radius, bool is_habitable)
        : id(planet_id), type(planet_type), mass(planet_mass), 
          radius(planet_radius), habitable(is_habitable) {}
    
    constexpr double surface_gravity() const {
        return GRAVITATIONAL_CONSTANT * mass / (radius * radius);
    }
    
    constexpr double escape_velocity() const {
        return sqrt_constexpr(2.0 * GRAVITATIONAL_CONSTANT * mass / radius);
    }
    
    constexpr double surface_area() const {
        return sphere_surface_area(radius);
    }
    
    constexpr double volume() const {
        return sphere_volume(radius);
    }
    
    constexpr double density() const {
        return mass / volume();
    }
};

struct ConstexprFleet {
    int id;
    int ship_count;
    double fuel_percentage;
    MissionType current_mission;
    
    constexpr ConstexprFleet(int fleet_id, int ships, double fuel, MissionType mission)
        : id(fleet_id), ship_count(ships), fuel_percentage(fuel), current_mission(mission) {}
    
    constexpr double combat_power() const {
        return ship_count * (fuel_percentage / 100.0) * 10.0;
    }
    
    constexpr bool is_operational() const {
        return fuel_percentage > 20.0 && ship_count > 0;
    }
    
    constexpr double max_range() const {
        return fuel_percentage * ship_count * 0.5;
    }
};

// ===== COMPILE-TIME LOOKUP TABLES =====

template<size_t N>
constexpr std::array<double, N> generate_planet_distances() {
    std::array<double, N> distances{};
    for (size_t i = 0; i < N; ++i) {
        distances[i] = 0.4 + i * 0.7; // Simplified Titius-Bode law
    }
    return distances;
}

template<size_t N>
constexpr std::array<ConstexprPlanet, N> create_solar_system() {
    std::array<ConstexprPlanet, N> planets{};
    
    if constexpr (N >= 1) {
        planets[0] = ConstexprPlanet(1, PlanetType::Terrestrial, 3.301e23, 2.4397e6, false);
    }
    if constexpr (N >= 2) {
        planets[1] = ConstexprPlanet(2, PlanetType::Terrestrial, 4.867e24, 6.0518e6, false);
    }
    if constexpr (N >= 3) {
        planets[2] = ConstexprPlanet(3, PlanetType::Terrestrial, 5.972e24, 6.371e6, true);
    }
    if constexpr (N >= 4) {
        planets[3] = ConstexprPlanet(4, PlanetType::Terrestrial, 6.417e23, 3.3896e6, false);
    }
    if constexpr (N >= 5) {
        planets[4] = ConstexprPlanet(5, PlanetType::GasGiant, 1.898e27, 6.9911e7, false);
    }
    if constexpr (N >= 6) {
        planets[5] = ConstexprPlanet(6, PlanetType::GasGiant, 5.683e26, 5.8232e7, false);
    }
    if constexpr (N >= 7) {
        planets[6] = ConstexprPlanet(7, PlanetType::IceGiant, 8.681e25, 2.5362e7, false);
    }
    if constexpr (N >= 8) {
        planets[7] = ConstexprPlanet(8, PlanetType::IceGiant, 1.024e26, 2.4622e7, false);
    }
    
    return planets;
}

// ===== COMPILE-TIME ALGORITHMS =====

template<typename T, size_t N>
constexpr size_t binary_search(const std::array<T, N>& arr, const T& value) {
    size_t left = 0;
    size_t right = N;
    
    while (left < right) {
        size_t mid = left + (right - left) / 2;
        if (arr[mid] == value) {
            return mid;
        } else if (arr[mid] < value) {
            left = mid + 1;
        } else {
            right = mid;
        }
    }
    return N; // Not found
}

template<typename T, size_t N>
constexpr std::array<T, N> bubble_sort(std::array<T, N> arr) {
    for (size_t i = 0; i < N; ++i) {
        for (size_t j = 0; j < N - 1 - i; ++j) {
            if (arr[j] > arr[j + 1]) {
                T temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
        }
    }
    return arr;
}

constexpr int gcd(int a, int b) {
    while (b != 0) {
        int temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}

constexpr int lcm(int a, int b) {
    return (a * b) / gcd(a, b);
}

// ===== COMPILE-TIME TYPE COMPUTATIONS =====

template<typename T>
constexpr bool is_space_entity() {
    return std::is_same_v<T, ConstexprPlanet> || std::is_same_v<T, ConstexprFleet>;
}

template<int N>
constexpr int compile_time_factorial() {
    if constexpr (N <= 1) {
        return 1;
    } else {
        return N * compile_time_factorial<N - 1>();
    }
}

template<int N>
constexpr int fibonacci() {
    if constexpr (N <= 1) {
        return N;
    } else {
        return fibonacci<N - 1>() + fibonacci<N - 2>();
    }
}

template<int Base, int Exponent>
constexpr int compile_time_power() {
    if constexpr (Exponent == 0) {
        return 1;
    } else if constexpr (Exponent == 1) {
        return Base;
    } else {
        return Base * compile_time_power<Base, Exponent - 1>();
    }
}

// ===== COMPILE-TIME VALIDATION =====

template<size_t N>
constexpr bool validate_fleet_configuration(const std::array<ConstexprFleet, N>& fleets) {
    int total_ships = 0;
    double total_fuel = 0.0;
    
    for (size_t i = 0; i < N; ++i) {
        if (!fleets[i].is_operational()) {
            return false; // All fleets must be operational
        }
        total_ships += fleets[i].ship_count;
        total_fuel += fleets[i].fuel_percentage;
    }
    
    return total_ships >= 10 && (total_fuel / N) >= 50.0; // Minimum requirements
}

template<size_t N>
constexpr int count_habitable_planets(const std::array<ConstexprPlanet, N>& planets) {
    int count = 0;
    for (size_t i = 0; i < N; ++i) {
        if (planets[i].habitable) {
            count++;
        }
    }
    return count;
}

template<size_t N>
constexpr double calculate_total_mass(const std::array<ConstexprPlanet, N>& planets) {
    double total_mass = 0.0;
    for (size_t i = 0; i < N; ++i) {
        total_mass += planets[i].mass;
    }
    return total_mass;
}

// ===== CONSTEXPR METAPROGRAMMING =====

template<typename... Args>
constexpr size_t count_types() {
    return sizeof...(Args);
}

template<typename T, typename... Args>
constexpr bool contains_type() {
    return (std::is_same_v<T, Args> || ...);
}

template<int... Values>
constexpr int sum_values() {
    return (Values + ...);
}

template<int... Values>
constexpr int multiply_values() {
    return (Values * ...);
}

// ===== CONSTEXPR HASH AND ENCRYPTION =====

constexpr uint32_t simple_hash(const char* str) {
    uint32_t hash = 5381;
    while (*str != '\0') {
        hash = ((hash << 5) + hash) + static_cast<uint32_t>(*str);
        ++str;
    }
    return hash;
}

constexpr char caesar_cipher_encode(char c, int shift) {
    if (c >= 'a' && c <= 'z') {
        return 'a' + (c - 'a' + shift) % 26;
    } else if (c >= 'A' && c <= 'Z') {
        return 'A' + (c - 'A' + shift) % 26;
    }
    return c;
}

constexpr char caesar_cipher_decode(char c, int shift) {
    return caesar_cipher_encode(c, 26 - shift);
}

// ===== COMPILE-TIME CONSTANTS AND LOOKUPS =====

constexpr std::array<double, 8> PLANET_DISTANCES = generate_planet_distances<8>();
constexpr std::array<int, 10> FIBONACCI_SEQUENCE = generate_fibonacci<10>();
constexpr std::array<int, 10> FIRST_PRIMES = generate_primes<10>();
constexpr std::array<ConstexprPlanet, 8> SOLAR_SYSTEM = create_solar_system<8>();

// Compile-time calculations
constexpr double EARTH_ESCAPE_VELOCITY = escape_velocity(EARTH_MASS, 6.371e6);
constexpr double EARTH_ORBITAL_VELOCITY = orbital_velocity(1.989e30, 1.496e11);
constexpr int FACTORIAL_10 = compile_time_factorial<10>();
constexpr int FIBONACCI_15 = fibonacci<15>();
constexpr int POWER_2_10 = compile_time_power<2, 10>();

// Hash of game name
constexpr uint32_t GAME_NAME_HASH = simple_hash("CppVerseHub");

// ===== CONSTEXPR DEMONSTRATION FUNCTIONS =====

constexpr double demonstrate_orbital_mechanics() {
    // Calculate orbital parameters for Earth
    constexpr double earth_mass = 5.972e24;
    constexpr double earth_radius = 6.371e6;
    constexpr double orbit_radius = 1.496e11; // Distance to Sun
    
    constexpr double surface_gravity = GRAVITATIONAL_CONSTANT * earth_mass / (earth_radius * earth_radius);
    constexpr double escape_vel = escape_velocity(earth_mass, earth_radius);
    constexpr double orbital_vel = orbital_velocity(1.989e30, orbit_radius);
    
    return surface_gravity + escape_vel + orbital_vel; // Combined result
}

constexpr int demonstrate_compile_time_algorithms() {
    constexpr std::array<int, 5> numbers = {64, 25, 12, 22, 11};
    constexpr auto sorted_numbers = bubble_sort(numbers);
    constexpr int max_value = array_max(sorted_numbers);
    constexpr int sum_value = array_sum(sorted_numbers);
    
    return max_value + sum_value;
}

constexpr bool demonstrate_space_fleet_validation() {
    constexpr std::array<ConstexprFleet, 3> test_fleets = {{
        ConstexprFleet(1, 15, 85.5, MissionType::Exploration),
        ConstexprFleet(2, 12, 92.0, MissionType::Combat),
        ConstexprFleet(3, 8, 75.2, MissionType::Colonization)
    }};
    
    return validate_fleet_configuration(test_fleets);
}

// ===== DEMONSTRATION CONSTANTS =====

constexpr double DEMO_ORBITAL_RESULT = demonstrate_orbital_mechanics();
constexpr int DEMO_ALGORITHM_RESULT = demonstrate_compile_time_algorithms();
constexpr bool DEMO_FLEET_VALID = demonstrate_space_fleet_validation();
constexpr int DEMO_HABITABLE_COUNT = count_habitable_planets(SOLAR_SYSTEM);
constexpr double DEMO_TOTAL_MASS = calculate_total_mass(SOLAR_SYSTEM);

// ===== RUNTIME DEMONSTRATION FUNCTIONS =====

void print_compile_time_results() {
    std::cout << "\n=== Compile-time Calculation Results ===" << std::endl;
    std::cout << "Earth escape velocity: " << EARTH_ESCAPE_VELOCITY << " m/s" << std::endl;
    std::cout << "Earth orbital velocity: " << EARTH_ORBITAL_VELOCITY << " m/s" << std::endl;
    std::cout << "10! = " << FACTORIAL_10 << std::endl;
    std::cout << "15th Fibonacci number: " << FIBONACCI_15 << std::endl;
    std::cout << "2^10 = " << POWER_2_10 << std::endl;
    
    std::cout << "\nSolar System Analysis:" << std::endl;
    std::cout << "Habitable planets: " << DEMO_HABITABLE_COUNT << std::endl;
    std::cout << "Total system mass: " << DEMO_TOTAL_MASS << " kg" << std::endl;
    std::cout << "Fleet configuration valid: " << (DEMO_FLEET_VALID ? "Yes" : "No") << std::endl;
    
    std::cout << "\nCompile-time Arrays:" << std::endl;
    std::cout << "First 10 Fibonacci numbers: ";
    for (size_t i = 0; i < FIBONACCI_SEQUENCE.size(); ++i) {
        std::cout << FIBONACCI_SEQUENCE[i] << " ";
    }
    std::cout << std::endl;
    
    std::cout << "First 10 prime numbers: ";
    for (size_t i = 0; i < FIRST_PRIMES.size(); ++i) {
        std::cout << FIRST_PRIMES[i] << " ";
    }
    std::cout << std::endl;
    
    std::cout << "Game name hash: " << GAME_NAME_HASH << std::endl;
}

void demonstrate_constexpr_containers() {
    std::cout << "\n=== Constexpr Container Operations ===" << std::endl;
    
    constexpr std::array<int, 8> test_array = {42, 17, 89, 3, 56, 23, 91, 12};
    constexpr auto sorted_array = bubble_sort(test_array);
    constexpr int array_maximum = array_max(sorted_array);
    constexpr int array_minimum = array_min(sorted_array);
    constexpr int array_total = array_sum(sorted_array);
    
    std::cout << "Original array: ";
    for (const auto& val : test_array) {
        std::cout << val << " ";
    }
    std::cout << std::endl;
    
    std::cout << "Sorted array: ";
    for (const auto& val : sorted_array) {
        std::cout << val << " ";
    }
    std::cout << std::endl;
    
    std::cout << "Max: " << array_maximum << ", Min: " << array_minimum << ", Sum: " << array_total << std::endl;
}

void demonstrate_constexpr_planets() {
    std::cout << "\n=== Constexpr Planet Analysis ===" << std::endl;
    
    constexpr auto earth = SOLAR_SYSTEM[2]; // Earth
    constexpr auto jupiter = SOLAR_SYSTEM[4]; // Jupiter
    
    std::cout << "Earth Analysis:" << std::endl;
    std::cout << "  Surface gravity: " << earth.surface_gravity() << " m/s²" << std::endl;
    std::cout << "  Escape velocity: " << earth.escape_velocity() << " m/s" << std::endl;
    std::cout << "  Density: " << earth.density() << " kg/m³" << std::endl;
    
    std::cout << "Jupiter Analysis:" << std::endl;
    std::cout << "  Surface gravity: " << jupiter.surface_gravity() << " m/s²" << std::endl;
    std::cout << "  Escape velocity: " << jupiter.escape_velocity() << " m/s" << std::endl;
    std::cout << "  Density: " << jupiter.density() << " kg/m³" << std::endl;
}

void demonstrate_constexpr_string_operations() {
    std::cout << "\n=== Constexpr String Operations ===" << std::endl;
    
    constexpr const char* test_string = "CppVerseHub";
    constexpr size_t str_length = string_length(test_string);
    constexpr uint32_t str_hash = simple_hash(test_string);
    constexpr bool equals_check = strings_equal(test_string, "CppVerseHub");
    
    std::cout << "String: " << test_string << std::endl;
    std::cout << "Length: " << str_length << std::endl;
    std::cout << "Hash: " << str_hash << std::endl;
    std::cout << "Equals check: " << (equals_check ? "true" : "false") << std::endl;
    
    // Demonstrate caesar cipher
    constexpr char original = 'h';
    constexpr char encoded = caesar_cipher_encode(original, 3);
    constexpr char decoded = caesar_cipher_decode(encoded, 3);
    
    std::cout << "Caesar cipher: " << original << " -> " << encoded << " -> " << decoded << std::endl;
}

void demonstrate_constexpr_metaprogramming() {
    std::cout << "\n=== Constexpr Metaprogramming ===" << std::endl;
    
    constexpr size_t type_count = count_types<int, double, float, char>();
    constexpr bool has_int = contains_type<int, double, float, int, char>();
    constexpr bool has_string = contains_type<std::string, double, float, int, char>();
    constexpr int values_sum = sum_values<1, 2, 3, 4, 5>();
    constexpr int values_product = multiply_values<2, 3, 4>();
    
    std::cout << "Type count: " << type_count << std::endl;
    std::cout << "Contains int: " << (has_int ? "true" : "false") << std::endl;
    std::cout << "Contains string: " << (has_string ? "true" : "false") << std::endl;
    std::cout << "Sum of values: " << values_sum << std::endl;
    std::cout << "Product of values: " << values_product << std::endl;
}

// ===== MAIN DEMONSTRATION FUNCTION =====

void demonstrate_all_constexpr() {
    std::cout << "\nCompile-time Programming Demonstration" << std::endl;
    std::cout << "=============================================" << std::endl;
    
    print_compile_time_results();
    demonstrate_constexpr_containers();
    demonstrate_constexpr_planets();
    demonstrate_constexpr_string_operations();
    demonstrate_constexpr_metaprogramming();
    
    std::cout << "\nConstexpr programming demonstration complete!" << std::endl;
    std::cout << "\nKey Benefits:" << std::endl;
    std::cout << "• Calculations performed at compile time" << std::endl;
    std::cout << "• Zero runtime overhead for computations" << std::endl;
    std::cout << "• Compile-time validation and error checking" << std::endl;
    std::cout << "• Improved performance and reduced binary size" << std::endl;
    std::cout << "• Type-safe compile-time programming" << std::endl;
}

} // namespace CppVerseHub::Modern::ConstexprProgramming