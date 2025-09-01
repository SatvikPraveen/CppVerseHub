// File: src/modern/LambdaExpressions.hpp
// Advanced Lambda Expressions Demonstration

#pragma once

#include <functional>
#include <algorithm>
#include <vector>
#include <string>
#include <memory>
#include <iostream>
#include <map>
#include <numeric>
#include <future>
#include <type_traits>

namespace CppVerseHub::Modern::LambdaExpressions {

// ===== SPACE GAME DATA STRUCTURES =====

struct SpaceShip {
    int id;
    std::string name;
    std::string class_type;
    double fuel_level;
    int crew_size;
    bool is_active;
    std::vector<std::string> equipment;
    
    SpaceShip(int ship_id, std::string ship_name, std::string ship_class, 
              double fuel, int crew, bool active = true)
        : id(ship_id), name(std::move(ship_name)), class_type(std::move(ship_class)),
          fuel_level(fuel), crew_size(crew), is_active(active) {}
          
    friend std::ostream& operator<<(std::ostream& os, const SpaceShip& ship) {
        return os << "Ship{id=" << ship.id << ", name=" << ship.name 
                  << ", fuel=" << ship.fuel_level << "%, crew=" << ship.crew_size << "}";
    }
};

struct Planet {
    int id;
    std::string name;
    double distance_from_star;
    long long population;
    bool habitable;
    std::vector<std::string> resources;
    
    Planet(int planet_id, std::string planet_name, double distance, 
           long long pop = 0, bool hab = false)
        : id(planet_id), name(std::move(planet_name)), distance_from_star(distance),
          population(pop), habitable(hab) {}
          
    friend std::ostream& operator<<(std::ostream& os, const Planet& planet) {
        return os << "Planet{id=" << planet.id << ", name=" << planet.name 
                  << ", distance=" << planet.distance_from_star << "AU, pop=" << planet.population << "}";
    }
};

struct Mission {
    int id;
    std::string type;
    std::string target;
    double progress;
    int priority;
    bool completed;
    
    Mission(int mission_id, std::string mission_type, std::string mission_target, 
            int mission_priority = 1)
        : id(mission_id), type(std::move(mission_type)), target(std::move(mission_target)),
          progress(0.0), priority(mission_priority), completed(false) {}
          
    friend std::ostream& operator<<(std::ostream& os, const Mission& mission) {
        return os << "Mission{id=" << mission.id << ", type=" << mission.type 
                  << ", progress=" << mission.progress << "%, priority=" << mission.priority << "}";
    }
};

// ===== BASIC LAMBDA DEMONSTRATIONS =====

void demonstrate_basic_lambdas() {
    std::cout << "\n=== Basic Lambda Expressions ===" << std::endl;
    
    // Simple lambda with no captures
    auto greet = []() {
        return "Welcome to CppVerseHub Space Game!";
    };
    std::cout << greet() << std::endl;
    
    // Lambda with parameters
    auto calculate_distance = [](double x1, double y1, double x2, double y2) {
        return std::sqrt((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1));
    };
    
    double distance = calculate_distance(0.0, 0.0, 3.0, 4.0);
    std::cout << "Distance between points: " << distance << " units" << std::endl;
    
    // Lambda with capture by value
    int base_damage = 100;
    auto calculate_damage = [base_damage](double multiplier) {
        return base_damage * multiplier;
    };
    
    std::cout << "Weapon damage with 1.5x multiplier: " << calculate_damage(1.5) << std::endl;
    
    // Lambda with capture by reference
    int total_score = 0;
    auto add_score = [&total_score](int points) {
        total_score += points;
        std::cout << "Added " << points << " points. Total: " << total_score << std::endl;
    };
    
    add_score(150);
    add_score(200);
    add_score(75);
}

void demonstrate_capture_modes() {
    std::cout << "\n=== Lambda Capture Modes ===" << std::endl;
    
    int fleet_count = 5;
    double fuel_reserve = 1000.0;
    std::string commander_name = "Admiral Zhang";
    
    // Capture by value [=]
    auto fleet_status_value = [=]() {
        std::cout << "Fleet Status (by value): " << fleet_count << " fleets, " 
                  << fuel_reserve << " fuel units, Commander: " << commander_name << std::endl;
    };
    
    // Capture by reference [&]
    auto update_fleet_ref = [&](int new_count, double fuel_consumed) {
        fleet_count = new_count;
        fuel_reserve -= fuel_consumed;
        std::cout << "Fleet updated (by reference): " << fleet_count << " fleets, " 
                  << fuel_reserve << " fuel remaining" << std::endl;
    };
    
    // Mixed capture [=, &fuel_reserve]
    auto mixed_capture = [=, &fuel_reserve](double fuel_cost) {
        fuel_reserve -= fuel_cost;
        std::cout << "Mission cost: " << fuel_cost << " fuel. Commander " << commander_name 
                  << " has " << fuel_reserve << " fuel left for " << fleet_count << " fleets" << std::endl;
    };
    
    fleet_status_value();
    update_fleet_ref(7, 150.0);
    mixed_capture(200.0);
}

// ===== LAMBDA WITH STL ALGORITHMS =====

void demonstrate_stl_lambdas() {
    std::cout << "\n=== Lambdas with STL Algorithms ===" << std::endl;
    
    std::vector<SpaceShip> fleet = {
        {1, "USS Explorer", "Science", 85.5, 150, true},
        {2, "USS Guardian", "Battleship", 92.0, 300, true},
        {3, "USS Voyager", "Scout", 23.1, 50, false},
        {4, "USS Defender", "Destroyer", 67.8, 200, true},
        {5, "USS Discovery", "Research", 91.2, 180, true}
    };
    
    // Filter active ships with enough fuel
    std::vector<SpaceShip> operational_ships;
    std::copy_if(fleet.begin(), fleet.end(), std::back_inserter(operational_ships),
        [](const SpaceShip& ship) {
            return ship.is_active && ship.fuel_level > 50.0;
        });
    
    std::cout << "Operational ships with >50% fuel:" << std::endl;
    for (const auto& ship : operational_ships) {
        std::cout << "  " << ship << std::endl;
    }
    
    // Sort by fuel level (descending)
    std::sort(fleet.begin(), fleet.end(),
        [](const SpaceShip& a, const SpaceShip& b) {
            return a.fuel_level > b.fuel_level;
        });
    
    std::cout << "\nFleet sorted by fuel level (highest first):" << std::endl;
    for (const auto& ship : fleet) {
        std::cout << "  " << ship.name << ": " << ship.fuel_level << "%" << std::endl;
    }
    
    // Calculate total crew size
    int total_crew = std::accumulate(fleet.begin(), fleet.end(), 0,
        [](int sum, const SpaceShip& ship) {
            return ship.is_active ? sum + ship.crew_size : sum;
        });
    
    std::cout << "Total active crew members: " << total_crew << std::endl;
    
    // Transform ship names to uppercase
    std::vector<std::string> ship_names;
    std::transform(fleet.begin(), fleet.end(), std::back_inserter(ship_names),
        [](const SpaceShip& ship) {
            std::string name = ship.name;
            std::transform(name.begin(), name.end(), name.begin(), ::toupper);
            return name;
        });
    
    std::cout << "Ship names (uppercase): ";
    for (const auto& name : ship_names) {
        std::cout << name << " ";
    }
    std::cout << std::endl;
}

// ===== GENERIC LAMBDAS =====

void demonstrate_generic_lambdas() {
    std::cout << "\n=== Generic Lambdas ===" << std::endl;
    
    // Generic lambda for comparison
    auto compare_greater = [](const auto& a, const auto& b) {
        return a > b;
    };
    
    std::cout << "5 > 3: " << compare_greater(5, 3) << std::endl;
    std::cout << "3.14 > 2.71: " << compare_greater(3.14, 2.71) << std::endl;
    std::cout << "\"zebra\" > \"apple\": " << compare_greater(std::string("zebra"), std::string("apple")) << std::endl;
    
    // Generic lambda for container processing
    auto process_container = [](const auto& container, auto processor) {
        std::cout << "Processing container: ";
        for (const auto& item : container) {
            std::cout << processor(item) << " ";
        }
        std::cout << std::endl;
    };
    
    std::vector<int> numbers = {1, 2, 3, 4, 5};
    std::vector<std::string> words = {"space", "game", "lambda", "modern"};
    
    // Process numbers (square them)
    process_container(numbers, [](const auto& n) { return n * n; });
    
    // Process strings (get length)
    process_container(words, [](const auto& word) { return word.length(); });
    
    // Generic lambda with constexpr (C++17)
    auto generic_math = [](const auto& a, const auto& b) constexpr {
        return a * a + b * b;
    };
    
    constexpr auto result = generic_math(3, 4);
    std::cout << "Constexpr generic lambda result: " << result << std::endl;
}

// ===== LAMBDA AS FUNCTION ARGUMENTS =====

template<typename Predicate>
std::vector<Planet> filter_planets(const std::vector<Planet>& planets, Predicate pred) {
    std::vector<Planet> filtered;
    std::copy_if(planets.begin(), planets.end(), std::back_inserter(filtered), pred);
    return filtered;
}

template<typename Transform>
auto transform_planets(const std::vector<Planet>& planets, Transform transform) {
    std::vector<decltype(transform(planets[0]))> results;
    std::transform(planets.begin(), planets.end(), std::back_inserter(results), transform);
    return results;
}

void demonstrate_lambda_as_parameters() {
    std::cout << "\n=== Lambdas as Function Parameters ===" << std::endl;
    
    std::vector<Planet> solar_system = {
        {1, "Mercury", 0.39, 0, false},
        {2, "Venus", 0.72, 0, false},
        {3, "Earth", 1.0, 8000000000LL, true},
        {4, "Mars", 1.52, 0, false},
        {5, "Jupiter", 5.20, 0, false},
        {6, "Kepler-442b", 112.0, 50000000LL, true},
        {7, "Proxima-Centauri-b", 42400.0, 0, true}
    };
    
    // Filter habitable planets
    auto habitable_planets = filter_planets(solar_system, 
        [](const Planet& p) { return p.habitable; });
    
    std::cout << "Habitable planets:" << std::endl;
    for (const auto& planet : habitable_planets) {
        std::cout << "  " << planet << std::endl;
    }
    
    // Filter planets within 10 AU
    auto close_planets = filter_planets(solar_system,
        [](const Planet& p) { return p.distance_from_star <= 10.0; });
    
    std::cout << "\nPlanets within 10 AU:" << std::endl;
    for (const auto& planet : close_planets) {
        std::cout << "  " << planet.name << " at " << planet.distance_from_star << " AU" << std::endl;
    }
    
    // Transform planets to their names
    auto planet_names = transform_planets(solar_system,
        [](const Planet& p) { return p.name; });
    
    std::cout << "\nPlanet names: ";
    for (const auto& name : planet_names) {
        std::cout << name << " ";
    }
    std::cout << std::endl;
    
    // Transform to population density categories
    auto population_categories = transform_planets(solar_system,
        [](const Planet& p) -> std::string {
            if (p.population == 0) return "Uninhabited";
            if (p.population < 1000000) return "Low Population";
            if (p.population < 100000000) return "Medium Population";
            return "High Population";
        });
    
    std::cout << "\nPopulation categories:" << std::endl;
    for (size_t i = 0; i < solar_system.size(); ++i) {
        std::cout << "  " << solar_system[i].name << ": " << population_categories[i] << std::endl;
    }
}

// ===== STATEFUL LAMBDAS =====

void demonstrate_stateful_lambdas() {
    std::cout << "\n=== Stateful Lambdas ===" << std::endl;
    
    // Lambda with mutable capture
    int mission_counter = 0;
    auto mission_generator = [mission_counter](const std::string& type) mutable {
        return "Mission-" + std::to_string(++mission_counter) + "-" + type;
    };
    
    std::cout << "Generated missions:" << std::endl;
    std::cout << "  " << mission_generator("Exploration") << std::endl;
    std::cout << "  " << mission_generator("Combat") << std::endl;
    std::cout << "  " << mission_generator("Colonization") << std::endl;
    std::cout << "  " << mission_generator("Trade") << std::endl;
    
    // Lambda for accumulating statistics
    auto stats_accumulator = [sum = 0.0, count = 0](double value) mutable -> double {
        sum += value;
        ++count;
        return sum / count; // Return running average
    };
    
    std::vector<double> fuel_readings = {85.5, 92.0, 23.1, 67.8, 91.2, 44.7, 76.3};
    
    std::cout << "\nRunning average of fuel readings:" << std::endl;
    for (const auto& reading : fuel_readings) {
        double avg = stats_accumulator(reading);
        std::cout << "  Reading: " << reading << "%, Running avg: " << avg << "%" << std::endl;
    }
    
    // Lambda factory pattern
    auto create_validator = [](double min_val, double max_val) {
        return [min_val, max_val](double value) {
            return value >= min_val && value <= max_val;
        };
    };
    
    auto fuel_validator = create_validator(20.0, 100.0);
    auto crew_validator = create_validator(10, 500);
    
    std::cout << "\nValidation results:" << std::endl;
    std::cout << "  Fuel 75.5%: " << (fuel_validator(75.5) ? "Valid" : "Invalid") << std::endl;
    std::cout << "  Fuel 15.2%: " << (fuel_validator(15.2) ? "Valid" : "Invalid") << std::endl;
    std::cout << "  Crew 150: " << (crew_validator(150) ? "Valid" : "Invalid") << std::endl;
    std::cout << "  Crew 600: " << (crew_validator(600) ? "Valid" : "Invalid") << std::endl;
}

// ===== LAMBDA WITH PERFECT FORWARDING =====

template<typename Func, typename... Args>
auto invoke_with_timing(Func&& func, Args&&... args) {
    auto start = std::chrono::high_resolution_clock::now();
    
    if constexpr (std::is_void_v<std::invoke_result_t<Func, Args...>>) {
        std::invoke(std::forward<Func>(func), std::forward<Args>(args)...);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        std::cout << "Function executed in " << duration.count() << " microseconds" << std::endl;
    } else {
        auto result = std::invoke(std::forward<Func>(func), std::forward<Args>(args)...);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        std::cout << "Function executed in " << duration.count() << " microseconds" << std::endl;
        return result;
    }
}

void demonstrate_advanced_lambda_techniques() {
    std::cout << "\n=== Advanced Lambda Techniques ===" << std::endl;
    
    // Lambda with perfect forwarding
    auto expensive_calculation = [](const std::vector<int>& data) {
        return std::accumulate(data.begin(), data.end(), 0LL,
            [](long long sum, int val) {
                // Simulate expensive operation
                for (int i = 0; i < 1000; ++i) {
                    sum += val * i;
                }
                return sum;
            });
    };
    
    std::vector<int> large_dataset(1000, 42);
    auto result = invoke_with_timing(expensive_calculation, large_dataset);
    std::cout << "Calculation result: " << result << std::endl;
    
    // Recursive lambda (C++14 style)
    std::function<int(int)> factorial = [&factorial](int n) -> int {
        return (n <= 1) ? 1 : n * factorial(n - 1);
    };
    
    std::cout << "Factorial calculations:" << std::endl;
    for (int i = 1; i <= 8; ++i) {
        std::cout << "  " << i << "! = " << factorial(i) << std::endl;
    }
    
    // Lambda with std::function for polymorphism
    std::vector<std::function<double(double)>> operations = {
        [](double x) { return x * x; },         // Square
        [](double x) { return std::sqrt(x); },  // Square root
        [](double x) { return x * 2; },         // Double
        [](double x) { return x / 2; }          // Half
    };
    
    double input = 16.0;
    std::cout << "\nOperations on " << input << ":" << std::endl;
    std::vector<std::string> op_names = {"Square", "Sqrt", "Double", "Half"};
    for (size_t i = 0; i < operations.size(); ++i) {
        std::cout << "  " << op_names[i] << ": " << operations[i](input) << std::endl;
    }
}

// ===== LAMBDA WITH ASYNC PROGRAMMING =====

void demonstrate_async_lambdas() {
    std::cout << "\n=== Lambdas with Async Programming ===" << std::endl;
    
    // Lambda for async mission execution
    auto execute_mission_async = [](const std::string& mission_name, int duration_ms) {
        return std::async(std::launch::async, [mission_name, duration_ms]() {
            std::cout << "Starting mission: " << mission_name << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(duration_ms));
            std::cout << "Completed mission: " << mission_name << std::endl;
            return "Mission " + mission_name + " successful!";
        });
    };
    
    // Launch multiple async missions
    auto mission1 = execute_mission_async("Alpha Exploration", 100);
    auto mission2 = execute_mission_async("Beta Combat", 150);
    auto mission3 = execute_mission_async("Gamma Research", 80);
    
    // Wait for all missions to complete
    std::cout << "Waiting for missions to complete..." << std::endl;
    std::cout << mission1.get() << std::endl;
    std::cout << mission2.get() << std::endl;
    std::cout << mission3.get() << std::endl;
    
    // Parallel processing with lambdas
    std::vector<int> large_numbers(1000);
    std::iota(large_numbers.begin(), large_numbers.end(), 1);
    
    auto parallel_sum = std::async(std::launch::async, [&large_numbers]() {
        return std::accumulate(large_numbers.begin(), large_numbers.end(), 0LL);
    });
    
    auto parallel_product = std::async(std::launch::async, [&large_numbers]() {
        return std::accumulate(large_numbers.begin(), large_numbers.begin() + 10, 1LL,
            [](long long prod, int val) { return prod * val; });
    });
    
    std::cout << "Sum of first 1000 numbers: " << parallel_sum.get() << std::endl;
    std::cout << "Product of first 10 numbers: " << parallel_product.get() << std::endl;
}

// ===== MAIN DEMONSTRATION FUNCTION =====

void demonstrate_all_lambda_expressions() {
    std::cout << "\n🔥 Advanced Lambda Expressions Demonstration 🔥" << std::endl;
    std::cout << "===============================================" << std::endl;
    
    demonstrate_basic_lambdas();
    demonstrate_capture_modes();
    demonstrate_stl_lambdas();
    demonstrate_generic_lambdas();
    demonstrate_lambda_as_parameters();
    demonstrate_stateful_lambdas();
    demonstrate_advanced_lambda_techniques();
    demonstrate_async_lambdas();
    
    std::cout << "\n✨ Lambda expressions demonstration complete! ✨" << std::endl;
    std::cout << "\nKey Lambda Features Demonstrated:" << std::endl;
    std::cout << "• Basic syntax and captures" << std::endl;
    std::cout << "• Generic lambdas (auto parameters)" << std::endl;
    std::cout << "• Mutable lambdas for state" << std::endl;
    std::cout << "• Lambdas with STL algorithms" << std::endl;
    std::cout << "• Perfect forwarding and timing" << std::endl;
    std::cout << "• Async programming with lambdas" << std::endl;
    std::cout << "• Recursive and polymorphic lambdas" << std::endl;
}

} // namespace CppVerseHub::Modern::LambdaExpressions