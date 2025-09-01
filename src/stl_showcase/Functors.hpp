// File: src/stl_showcase/Functors.hpp
// CppVerseHub - STL Functors Demonstrations (Function objects and lambdas)

#pragma once

#include <functional>
#include <algorithm>
#include <vector>
#include <map>
#include <set>
#include <string>
#include <iostream>
#include <numeric>
#include <random>
#include <chrono>

namespace CppVerseHub::STL {

/**
 * @brief Starship data structure for functor demonstrations
 */
struct Starship {
    std::string name;
    std::string class_type;
    double mass_tons;
    int crew_capacity;
    double max_warp_speed;
    double shield_strength;
    
    Starship(const std::string& n = "", const std::string& ct = "",
             double mass = 0.0, int crew = 0, double warp = 0.0, double shields = 0.0)
        : name(n), class_type(ct), mass_tons(mass), crew_capacity(crew), 
          max_warp_speed(warp), shield_strength(shields) {}
    
    double getCombatEffectiveness() const {
        return shield_strength * max_warp_speed * (crew_capacity / 100.0);
    }
    
    friend std::ostream& operator<<(std::ostream& os, const Starship& ship) {
        os << ship.name << " (" << ship.class_type << ") - " 
           << ship.mass_tons << " tons, Crew: " << ship.crew_capacity 
           << ", Warp: " << ship.max_warp_speed << ", Shields: " << ship.shield_strength;
        return os;
    }
};

/**
 * @brief Function Object (Functor) Classes
 * 
 * Custom function objects that can be used with STL algorithms.
 * They maintain state and can be more efficient than function pointers.
 */

/**
 * @brief Predicate functor to check if a starship is combat-ready
 */
class IsCombatReady {
private:
    double min_shield_strength_;
    double min_warp_speed_;
    
public:
    IsCombatReady(double min_shields = 50.0, double min_warp = 5.0)
        : min_shield_strength_(min_shields), min_warp_speed_(min_warp) {}
    
    bool operator()(const Starship& ship) const {
        return ship.shield_strength >= min_shield_strength_ && 
               ship.max_warp_speed >= min_warp_speed_;
    }
    
    // Allow adjustment of criteria
    void setMinShields(double min_shields) { min_shield_strength_ = min_shields; }
    void setMinWarpSpeed(double min_warp) { min_warp_speed_ = min_warp; }
};

/**
 * @brief Comparison functor for sorting starships by combat effectiveness
 */
class CombatEffectivenessComparator {
private:
    bool ascending_;
    
public:
    explicit CombatEffectivenessComparator(bool ascending = true) : ascending_(ascending) {}
    
    bool operator()(const Starship& a, const Starship& b) const {
        double effectiveness_a = a.getCombatEffectiveness();
        double effectiveness_b = b.getCombatEffectiveness();
        
        return ascending_ ? effectiveness_a < effectiveness_b : effectiveness_a > effectiveness_b;
    }
};

/**
 * @brief Accumulator functor for calculating total fleet statistics
 */
class FleetStatsAccumulator {
public:
    struct FleetStats {
        double total_mass = 0.0;
        int total_crew = 0;
        double max_warp_achieved = 0.0;
        double total_shield_strength = 0.0;
        int ship_count = 0;
        
        double getAverageMass() const { return ship_count > 0 ? total_mass / ship_count : 0.0; }
        double getAverageCrew() const { return ship_count > 0 ? static_cast<double>(total_crew) / ship_count : 0.0; }
        double getAverageShields() const { return ship_count > 0 ? total_shield_strength / ship_count : 0.0; }
    };
    
    FleetStats operator()(const FleetStats& stats, const Starship& ship) const {
        FleetStats new_stats = stats;
        new_stats.total_mass += ship.mass_tons;
        new_stats.total_crew += ship.crew_capacity;
        new_stats.max_warp_achieved = std::max(new_stats.max_warp_achieved, ship.max_warp_speed);
        new_stats.total_shield_strength += ship.shield_strength;
        new_stats.ship_count++;
        return new_stats;
    }
};

/**
 * @brief Generator functor for creating procedural starship names
 */
class StarshipNameGenerator {
private:
    std::vector<std::string> prefixes_{"USS", "ISV", "UES", "NCC"};
    std::vector<std::string> names_{"Enterprise", "Voyager", "Discovery", "Prometheus", "Excelsior", 
                                   "Constitution", "Defiant", "Intrepid", "Galaxy", "Sovereign"};
    mutable std::random_device rd_;
    mutable std::mt19937 gen_{rd_()};
    mutable int counter_ = 1000;
    
public:
    std::string operator()() const {
        std::uniform_int_distribution<> prefix_dist(0, prefixes_.size() - 1);
        std::uniform_int_distribution<> name_dist(0, names_.size() - 1);
        
        return prefixes_[prefix_dist(gen_)] + " " + names_[name_dist(gen_)] + "-" + std::to_string(counter_++);
    }
};

/**
 * @brief Function Object Demonstrations
 * 
 * Shows how to create and use custom function objects with STL algorithms.
 */
class FunctionObjectDemonstrations {
public:
    /**
     * @brief Demonstrate predicate functors
     */
    static void demonstratePredicateFunctors() {
        std::cout << "\n=== Predicate Functor Demonstration ===\n";
        
        std::vector<Starship> fleet{
            {"Enterprise", "Heavy Cruiser", 4500.0, 400, 9.0, 85.0},
            {"Defiant", "Escort", 350.0, 50, 9.5, 45.0},
            {"Voyager", "Explorer", 3200.0, 150, 9.975, 70.0},
            {"Constitution", "Cruiser", 2800.0, 200, 8.0, 60.0},
            {"Miranda", "Light Cruiser", 1200.0, 100, 6.0, 35.0},
            {"Excelsior", "Heavy Cruiser", 4800.0, 450, 8.5, 80.0}
        };
        
        std::cout << "Full Fleet Roster:\n";
        for (const auto& ship : fleet) {
            std::cout << "- " << ship << "\n";
        }
        
        // Using custom predicate functor
        IsCombatReady combat_ready_check(50.0, 7.0); // Min 50 shields, min warp 7
        
        std::cout << "\nCombat-Ready Ships (Shields >= 50, Warp >= 7):\n";
        for (const auto& ship : fleet) {
            if (combat_ready_check(ship)) {
                std::cout << "- " << ship.name << " (Combat Rating: " 
                         << ship.getCombatEffectiveness() << ")\n";
            }
        }
        
        // Count combat-ready ships
        auto ready_count = std::count_if(fleet.begin(), fleet.end(), combat_ready_check);
        std::cout << "Total combat-ready ships: " << ready_count << "/" << fleet.size() << "\n";
        
        // Adjust criteria and recheck
        combat_ready_check.setMinShields(40.0);
        combat_ready_check.setMinWarpSpeed(6.0);
        
        auto relaxed_ready_count = std::count_if(fleet.begin(), fleet.end(), combat_ready_check);
        std::cout << "With relaxed criteria (Shields >= 40, Warp >= 6): " 
                 << relaxed_ready_count << "/" << fleet.size() << "\n";
        
        // Filter fleet using predicate
        std::vector<Starship> combat_fleet;
        std::copy_if(fleet.begin(), fleet.end(), std::back_inserter(combat_fleet), 
                    IsCombatReady(60.0, 8.0)); // Strict combat requirements
        
        std::cout << "\nElite Combat Fleet (Shields >= 60, Warp >= 8):\n";
        for (const auto& ship : combat_fleet) {
            std::cout << "- " << ship.name << "\n";
        }
    }
    
    /**
     * @brief Demonstrate comparison functors
     */
    static void demonstrateComparisonFunctors() {
        std::cout << "\n=== Comparison Functor Demonstration ===\n";
        
        std::vector<Starship> fleet{
            {"Enterprise", "Heavy Cruiser", 4500.0, 400, 9.0, 85.0},
            {"Defiant", "Escort", 350.0, 50, 9.5, 45.0},
            {"Voyager", "Explorer", 3200.0, 150, 9.975, 70.0},
            {"Constitution", "Cruiser", 2800.0, 200, 8.0, 60.0}
        };
        
        std::cout << "Original Fleet Order:\n";
        for (size_t i = 0; i < fleet.size(); ++i) {
            std::cout << i + 1 << ". " << fleet[i].name 
                     << " (Effectiveness: " << fleet[i].getCombatEffectiveness() << ")\n";
        }
        
        // Sort by combat effectiveness (ascending)
        CombatEffectivenessComparator ascending_comp(true);
        std::sort(fleet.begin(), fleet.end(), ascending_comp);
        
        std::cout << "\nSorted by Combat Effectiveness (Ascending):\n";
        for (size_t i = 0; i < fleet.size(); ++i) {
            std::cout << i + 1 << ". " << fleet[i].name 
                     << " (Effectiveness: " << fleet[i].getCombatEffectiveness() << ")\n";
        }
        
        // Sort by combat effectiveness (descending)
        CombatEffectivenessComparator descending_comp(false);
        std::sort(fleet.begin(), fleet.end(), descending_comp);
        
        std::cout << "\nSorted by Combat Effectiveness (Descending):\n";
        for (size_t i = 0; i < fleet.size(); ++i) {
            std::cout << i + 1 << ". " << fleet[i].name 
                     << " (Effectiveness: " << fleet[i].getCombatEffectiveness() << ")\n";
        }
        
        // Use with priority queue for deployment order
        std::priority_queue<Starship, std::vector<Starship>, CombatEffectivenessComparator> 
            deployment_queue(CombatEffectivenessComparator(false)); // Most effective first
        
        for (const auto& ship : fleet) {
            deployment_queue.push(ship);
        }
        
        std::cout << "\nDeployment Order (Most Effective First):\n";
        int deployment_order = 1;
        while (!deployment_queue.empty()) {
            auto ship = deployment_queue.top();
            deployment_queue.pop();
            std::cout << deployment_order++ << ". Deploy " << ship.name 
                     << " (Effectiveness: " << ship.getCombatEffectiveness() << ")\n";
        }
    }
    
    /**
     * @brief Demonstrate accumulator functors
     */
    static void demonstrateAccumulatorFunctors() {
        std::cout << "\n=== Accumulator Functor Demonstration ===\n";
        
        std::vector<Starship> fleet{
            {"Enterprise", "Heavy Cruiser", 4500.0, 400, 9.0, 85.0},
            {"Defiant", "Escort", 350.0, 50, 9.5, 45.0},
            {"Voyager", "Explorer", 3200.0, 150, 9.975, 70.0},
            {"Constitution", "Cruiser", 2800.0, 200, 8.0, 60.0},
            {"Miranda", "Light Cruiser", 1200.0, 100, 6.0, 35.0}
        };
        
        std::cout << "Analyzing Fleet of " << fleet.size() << " ships:\n";
        
        // Calculate comprehensive fleet statistics
        FleetStatsAccumulator stats_accumulator;
        FleetStatsAccumulator::FleetStats initial_stats;
        
        auto final_stats = std::accumulate(fleet.begin(), fleet.end(), initial_stats, stats_accumulator);
        
        std::cout << "\nFleet Statistics:\n";
        std::cout << "Total Mass: " << final_stats.total_mass << " tons\n";
        std::cout << "Total Crew: " << final_stats.total_crew << " personnel\n";
        std::cout << "Maximum Warp Speed: " << final_stats.max_warp_achieved << "\n";
        std::cout << "Total Shield Strength: " << final_stats.total_shield_strength << "\n";
        std::cout << "Ship Count: " << final_stats.ship_count << "\n";
        
        std::cout << "\nAverage Statistics:\n";
        std::cout << "Average Mass: " << final_stats.getAverageMass() << " tons\n";
        std::cout << "Average Crew: " << final_stats.getAverageCrew() << " personnel\n";
        std::cout << "Average Shields: " << final_stats.getAverageShields() << "\n";
        
        // Custom accumulation for specific metrics
        auto total_combat_rating = std::accumulate(fleet.begin(), fleet.end(), 0.0,
            [](double sum, const Starship& ship) {
                return sum + ship.getCombatEffectiveness();
            });
        
        std::cout << "\nTotal Fleet Combat Rating: " << total_combat_rating << "\n";
        std::cout << "Average Combat Rating: " << total_combat_rating / fleet.size() << "\n";
        
        // Find fleet composition by class
        std::map<std::string, int> class_distribution;
        std::for_each(fleet.begin(), fleet.end(),
            [&class_distribution](const Starship& ship) {
                class_distribution[ship.class_type]++;
            });
        
        std::cout << "\nFleet Composition by Class:\n";
        for (const auto& [ship_class, count] : class_distribution) {
            std::cout << "- " << ship_class << ": " << count << " ship(s)\n";
        }
    }
    
    /**
     * @brief Demonstrate generator functors
     */
    static void demonstrateGeneratorFunctors() {
        std::cout << "\n=== Generator Functor Demonstration ===\n";
        
        StarshipNameGenerator name_generator;
        
        std::cout << "Generating procedural starship names:\n";
        
        // Generate names using the functor
        std::vector<std::string> generated_names;
        std::generate_n(std::back_inserter(generated_names), 8, name_generator);
        
        for (size_t i = 0; i < generated_names.size(); ++i) {
            std::cout << i + 1 << ". " << generated_names[i] << "\n";
        }
        
        // Use generator with algorithm to create a fleet
        std::vector<Starship> procedural_fleet;
        
        // Random number generators for ship attributes
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> mass_dist(500.0, 5000.0);
        std::uniform_int_distribution<> crew_dist(50, 500);
        std::uniform_real_distribution<> warp_dist(6.0, 9.9);
        std::uniform_real_distribution<> shield_dist(30.0, 90.0);
        
        std::vector<std::string> ship_classes{"Cruiser", "Destroyer", "Explorer", "Escort", "Battleship"};
        std::uniform_int_distribution<> class_dist(0, ship_classes.size() - 1);
        
        // Generate complete fleet
        for (int i = 0; i < 5; ++i) {
            std::string ship_name = name_generator();
            std::string ship_class = ship_classes[class_dist(gen)];
            double mass = mass_dist(gen);
            int crew = crew_dist(gen);
            double warp = warp_dist(gen);
            double shields = shield_dist(gen);
            
            procedural_fleet.emplace_back(ship_name, ship_class, mass, crew, warp, shields);
        }
        
        std::cout << "\nProcedurally Generated Fleet:\n";
        for (const auto& ship : procedural_fleet) {
            std::cout << "- " << ship << "\n";
        }
        
        // Demonstrate stateful generator
        std::cout << "\nDemonstrating stateful generation (same generator instance):\n";
        for (int i = 0; i < 3; ++i) {
            std::cout << "Call " << i + 1 << ": " << name_generator() << "\n";
        }
    }
};

/**
 * @brief Lambda Expression Demonstrations
 * 
 * Shows various lambda expression patterns and their usage with STL algorithms.
 */
class LambdaExpressionDemonstrations {
public:
    /**
     * @brief Demonstrate basic lambda expressions
     */
    static void demonstrateBasicLambdas() {
        std::cout << "\n=== Basic Lambda Expression Demonstration ===\n";
        
        std::vector<int> sensor_readings{42, 37, 89, 23, 56, 91, 12, 78, 45, 67};
        
        std::cout << "Sensor readings: ";
        for (int reading : sensor_readings) std::cout << reading << " ";
        std::cout << "\n";
        
        // Simple lambda with no captures
        auto is_high_reading = [](int reading) { return reading > 50; };
        
        auto high_reading_count = std::count_if(sensor_readings.begin(), sensor_readings.end(), is_high_reading);
        std::cout << "High readings (> 50): " << high_reading_count << "/" << sensor_readings.size() << "\n";
        
        // Lambda with capture by value
        int threshold = 60;
        auto above_threshold = [threshold](int reading) { return reading > threshold; };
        
        auto critical_count = std::count_if(sensor_readings.begin(), sensor_readings.end(), above_threshold);
        std::cout << "Critical readings (> " << threshold << "): " << critical_count << "/" << sensor_readings.size() << "\n";
        
        // Lambda with capture by reference
        int adjustment = 5;
        auto adjust_reading = [&adjustment](int& reading) { reading += adjustment; };
        
        std::cout << "Adjusting all readings by +" << adjustment << "\n";
        std::for_each(sensor_readings.begin(), sensor_readings.end(), adjust_reading);
        
        std::cout << "Adjusted readings: ";
        for (int reading : sensor_readings) std::cout << reading << " ";
        std::cout << "\n";
        
        // Lambda with mixed captures
        double multiplier = 1.5;
        int offset = 10;
        auto calibrate = [multiplier, &offset](int reading) -> double {
            return reading * multiplier + offset;
        };
        
        std::cout << "Calibrated readings (x" << multiplier << " +" << offset << "):\n";
        std::transform(sensor_readings.begin(), sensor_readings.end(),
                      std::ostream_iterator<double>(std::cout, " "), calibrate);
        std::cout << "\n";
    }
    
    /**
     * @brief Demonstrate lambda captures
     */
    static void demonstrateLambdaCaptures() {
        std::cout << "\n=== Lambda Capture Demonstration ===\n";
        
        std::vector<Starship> fleet{
            {"Enterprise", "Heavy Cruiser", 4500.0, 400, 9.0, 85.0},
            {"Defiant", "Escort", 350.0, 50, 9.5, 45.0},
            {"Voyager", "Explorer", 3200.0, 150, 9.975, 70.0}
        };
        
        // Capture by value [=]
        double min_effectiveness = 5000.0;
        auto find_elite_ships = [=](const Starship& ship) {
            return ship.getCombatEffectiveness() > min_effectiveness;
        };
        
        std::cout << "Ships with effectiveness > " << min_effectiveness << ":\n";
        for (const auto& ship : fleet) {
            if (find_elite_ships(ship)) {
                std::cout << "- " << ship.name << " (Effectiveness: " 
                         << ship.getCombatEffectiveness() << ")\n";
            }
        }
        
        // Capture by reference [&]
        std::map<std::string, int> class_count;
        auto count_by_class = [&class_count](const Starship& ship) {
            class_count[ship.class_type]++;
        };
        
        std::for_each(fleet.begin(), fleet.end(), count_by_class);
        
        std::cout << "\nFleet composition:\n";
        for (const auto& [ship_class, count] : class_count) {
            std::cout << "- " << ship_class << ": " << count << "\n";
        }
        
        // Mixed captures
        std::string target_class = "Cruiser";
        int min_crew = 100;
        auto find_specific_ships = [target_class, &min_crew](const Starship& ship) {
            return ship.class_type.find(target_class) != std::string::npos && 
                   ship.crew_capacity >= min_crew;
        };
        
        std::cout << "\nShips matching '" << target_class << "' with crew >= " << min_crew << ":\n";
        for (const auto& ship : fleet) {
            if (find_specific_ships(ship)) {
                std::cout << "- " << ship.name << " (" << ship.class_type 
                         << ", Crew: " << ship.crew_capacity << ")\n";
            }
        }
        
        // Capture by move (C++14)
        auto expensive_data = std::make_unique<std::vector<int>>(1000, 42);
        auto process_with_data = [data = std::move(expensive_data)](int value) {
            return value + data->size();
        };
        
        std::cout << "\nUsing moved data in lambda: " << process_with_data(10) << "\n";
        
        // Init capture (C++14)
        auto accumulator = [sum = 0](int value) mutable -> int {
            sum += value;
            return sum;
        };
        
        std::cout << "Running accumulator: ";
        for (int value : {10, 20, 30, 40}) {
            std::cout << accumulator(value) << " ";
        }
        std::cout << "\n";
    }
    
    /**
     * @brief Demonstrate generic lambdas (C++14)
     */
    static void demonstrateGenericLambdas() {
        std::cout << "\n=== Generic Lambda Demonstration (C++14) ===\n";
        
        // Generic lambda with auto parameters
        auto print_container = [](const auto& container) {
            std::cout << "Container contents: ";
            for (const auto& element : container) {
                std::cout << element << " ";
            }
            std::cout << "\n";
        };
        
        std::vector<int> numbers{1, 2, 3, 4, 5};
        std::vector<std::string> names{"Alpha", "Beta", "Gamma"};
        
        print_container(numbers);
        print_container(names);
        
        // Generic lambda for comparison
        auto max_element = [](const auto& a, const auto& b) {
            return (a > b) ? a : b;
        };
        
        std::cout << "Max of 10 and 20: " << max_element(10, 20) << "\n";
        std::cout << "Max of 'apple' and 'banana': " << max_element(std::string("apple"), std::string("banana")) << "\n";
        
        // Generic lambda with templates (C++20 style, but shown as C++14 equivalent)
        auto transform_and_print = [](const auto& input_container, auto transformer) {
            std::cout << "Transformed: ";
            for (const auto& element : input_container) {
                std::cout << transformer(element) << " ";
            }
            std::cout << "\n";
        };
        
        std::vector<double> values{1.5, 2.7, 3.1, 4.9};
        transform_and_print(values, [](double x) { return static_cast<int>(x); });
        transform_and_print(names, [](const std::string& s) { return s.length(); });
        
        // Generic lambda for finding elements
        auto find_matching = [](const auto& container, auto predicate) {
            return std::find_if(container.begin(), container.end(), predicate);
        };
        
        auto found_number = find_matching(numbers, [](int n) { return n > 3; });
        if (found_number != numbers.end()) {
            std::cout << "Found number > 3: " << *found_number << "\n";
        }
        
        auto found_name = find_matching(names, [](const std::string& s) { return s.length() > 4; });
        if (found_name != names.end()) {
            std::cout << "Found name with length > 4: " << *found_name << "\n";
        }
    }
    
    /**
     * @brief Demonstrate lambda expressions with STL algorithms
     */
    static void demonstrateLambdaWithAlgorithms() {
        std::cout << "\n=== Lambda with STL Algorithms Demonstration ===\n";
        
        std::vector<Starship> fleet{
            {"Enterprise", "Heavy Cruiser", 4500.0, 400, 9.0, 85.0},
            {"Defiant", "Escort", 350.0, 50, 9.5, 45.0},
            {"Voyager", "Explorer", 3200.0, 150, 9.975, 70.0},
            {"Constitution", "Cruiser", 2800.0, 200, 8.0, 60.0},
            {"Miranda", "Light Cruiser", 1200.0, 100, 6.0, 35.0}
        };
        
        // std::sort with lambda
        std::cout << "Sorting fleet by warp speed (descending):\n";
        std::sort(fleet.begin(), fleet.end(), 
                 [](const Starship& a, const Starship& b) {
                     return a.max_warp_speed > b.max_warp_speed;
                 });
        
        for (const auto& ship : fleet) {
            std::cout << "- " << ship.name << " (Warp " << ship.max_warp_speed << ")\n";
        }
        
        // std::transform with lambda
        std::vector<double> combat_ratings;
        std::transform(fleet.begin(), fleet.end(), std::back_inserter(combat_ratings),
                      [](const Starship& ship) { return ship.getCombatEffectiveness(); });
        
        std::cout << "\nCombat effectiveness ratings: ";
        for (double rating : combat_ratings) {
            std::cout << std::fixed << std::setprecision(1) << rating << " ";
        }
        std::cout << "\n";
        
        // std::partition with lambda
        auto original_fleet = fleet;
        auto partition_point = std::partition(fleet.begin(), fleet.end(),
                                            [](const Starship& ship) { 
                                                return ship.shield_strength >= 60.0; 
                                            });
        
        std::cout << "\nFleet partitioned by shield strength (>= 60):\n";
        std::cout << "High-shield ships:\n";
        for (auto it = fleet.begin(); it != partition_point; ++it) {
            std::cout << "- " << it->name << " (Shields: " << it->shield_strength << ")\n";
        }
        
        std::cout << "Lower-shield ships:\n";
        for (auto it = partition_point; it != fleet.end(); ++it) {
            std::cout << "- " << it->name << " (Shields: " << it->shield_strength << ")\n";
        }
        
        // Complex lambda with multiple operations
        fleet = original_fleet; // Restore original order
        
        std::cout << "\nComplex fleet analysis:\n";
        auto fleet_analysis = std::accumulate(fleet.begin(), fleet.end(),
            std::make_pair(0.0, 0), // {total_effectiveness, heavy_ship_count}
            [](const auto& acc, const Starship& ship) {
                double effectiveness = ship.getCombatEffectiveness();
                int heavy_ships = ship.mass_tons > 3000.0 ? 1 : 0;
                return std::make_pair(acc.first + effectiveness, acc.second + heavy_ships);
            });
        
        std::cout << "Total fleet effectiveness: " << fleet_analysis.first << "\n";
        std::cout << "Heavy ships (> 3000 tons): " << fleet_analysis.second << "\n";
        std::cout << "Average effectiveness: " << fleet_analysis.first / fleet.size() << "\n";
        
        // Lambda with std::remove_if
        auto weak_ships_removed = std::remove_if(fleet.begin(), fleet.end(),
            [](const Starship& ship) { 
                return ship.getCombatEffectiveness() < 2000.0; 
            });
        
        fleet.erase(weak_ships_removed, fleet.end());
        
        std::cout << "\nAfter removing weak ships (effectiveness < 2000):\n";
        for (const auto& ship : fleet) {
            std::cout << "- " << ship.name << " (Effectiveness: " 
                     << ship.getCombatEffectiveness() << ")\n";
        }
    }
};

/**
 * @brief Standard Library Function Objects
 * 
 * Demonstrates the predefined function objects from <functional>.
 */
class StandardFunctionObjects {
public:
    /**
     * @brief Demonstrate arithmetic function objects
     */
    static void demonstrateArithmeticFunctors() {
        std::cout << "\n=== Standard Arithmetic Functors ===\n";
        
        std::vector<double> power_levels{100.0, 75.0, 125.0, 90.0, 110.0};
        std::vector<double> efficiency_factors{0.95, 0.87, 0.92, 0.89, 0.94};
        
        std::cout << "Power levels: ";
        for (double power : power_levels) std::cout << power << " ";
        std::cout << "\nEfficiency factors: ";
        for (double eff : efficiency_factors) std::cout << eff << " ";
        std::cout << "\n";
        
        // std::multiplies - multiply corresponding elements
        std::vector<double> effective_power;
        std::transform(power_levels.begin(), power_levels.end(),
                      efficiency_factors.begin(), std::back_inserter(effective_power),
                      std::multiplies<double>());
        
        std::cout << "Effective power (power * efficiency): ";
        for (double power : effective_power) std::cout << power << " ";
        std::cout << "\n";
        
        // std::plus with accumulate
        double total_power = std::accumulate(power_levels.begin(), power_levels.end(), 0.0, std::plus<double>());
        std::cout << "Total power: " << total_power << "\n";
        
        // std::minus with adjacent_difference
        std::vector<double> power_changes;
        std::adjacent_difference(power_levels.begin(), power_levels.end(),
                               std::back_inserter(power_changes), std::minus<double>());
        
        std::cout << "Power changes: ";
        for (size_t i = 1; i < power_changes.size(); ++i) { // Skip first element (original value)
            std::cout << power_changes[i] << " ";
        }
        std::cout << "\n";
        
        // std::divides for normalization
        std::vector<double> normalized_power;
        double max_power = *std::max_element(power_levels.begin(), power_levels.end());
        
        std::transform(power_levels.begin(), power_levels.end(),
                      std::back_inserter(normalized_power),
                      [max_power](double power) { return std::divides<double>()(power, max_power); });
        
        std::cout << "Normalized power (divided by max " << max_power << "): ";
        for (double power : normalized_power) std::cout << power << " ";
        std::cout << "\n";
    }
    
    /**
     * @brief Demonstrate comparison function objects
     */
    static void demonstrateComparisonFunctors() {
        std::cout << "\n=== Standard Comparison Functors ===\n";
        
        std::vector<int> mission_priorities{5, 2, 8, 1, 9, 3, 7, 4, 6};
        
        std::cout << "Original priorities: ";
        for (int p : mission_priorities) std::cout << p << " ";
        std::cout << "\n";
        
        // std::greater for descending sort
        std::sort(mission_priorities.begin(), mission_priorities.end(), std::greater<int>());
        
        std::cout << "Sorted (descending): ";
        for (int p : mission_priorities) std::cout << p << " ";
        std::cout << "\n";
        
        // std::less for ascending sort
        std::sort(mission_priorities.begin(), mission_priorities.end(), std::less<int>());
        
        std::cout << "Sorted (ascending): ";
        for (int p : mission_priorities) std::cout << p << " ";
        std::cout << "\n";
        
        // Using with priority_queue
        std::priority_queue<int, std::vector<int>, std::greater<int>> min_heap; // Min heap
        for (int p : mission_priorities) {
            min_heap.push(p);
        }
        
        std::cout << "Min heap processing (smallest first): ";
        while (!min_heap.empty()) {
            std::cout << min_heap.top() << " ";
            min_heap.pop();
        }
        std::cout << "\n";
        
        // std::equal_to with count
        int target_priority = 5;
        auto equal_count = std::count_if(mission_priorities.begin(), mission_priorities.end(),
                                       [target_priority](int p) { 
                                           return std::equal_to<int>()(p, target_priority); 
                                       });
        
        std::cout << "Missions with priority " << target_priority << ": " << equal_count << "\n";
        
        // std::greater_equal for filtering
        int min_priority = 6;
        auto high_priority_count = std::count_if(mission_priorities.begin(), mission_priorities.end(),
                                               [min_priority](int p) { 
                                                   return std::greater_equal<int>()(p, min_priority); 
                                               });
        
        std::cout << "High priority missions (>= " << min_priority << "): " << high_priority_count << "\n";
    }
    
    /**
     * @brief Demonstrate logical function objects
     */
    static void demonstrateLogicalFunctors() {
        std::cout << "\n=== Standard Logical Functors ===\n";
        
        std::vector<bool> system_status{true, false, true, true, false, true, false, true};
        std::vector<bool> backup_status{false, true, true, false, true, true, true, false};
        
        std::cout << "System status:  ";
        for (bool status : system_status) std::cout << (status ? "OK " : "ER ");
        std::cout << "\nBackup status:  ";
        for (bool status : backup_status) std::cout << (status ? "OK " : "ER ");
        std::cout << "\n";
        
        // std::logical_and - both systems must be OK
        std::vector<bool> both_ok;
        std::transform(system_status.begin(), system_status.end(),
                      backup_status.begin(), std::back_inserter(both_ok),
                      std::logical_and<bool>());
        
        std::cout << "Both systems OK: ";
        for (bool status : both_ok) std::cout << (status ? "OK " : "ER ");
        std::cout << "\n";
        
        // std::logical_or - at least one system OK
        std::vector<bool> either_ok;
        std::transform(system_status.begin(), system_status.end(),
                      backup_status.begin(), std::back_inserter(either_ok),
                      std::logical_or<bool>());
        
        std::cout << "Either system OK: ";
        for (bool status : either_ok) std::cout << (status ? "OK " : "ER ");
        std::cout << "\n";
        
        // std::logical_not - invert status
        std::vector<bool> system_errors;
        std::transform(system_status.begin(), system_status.end(),
                      std::back_inserter(system_errors),
                      std::logical_not<bool>());
        
        std::cout << "System errors:   ";
        for (bool error : system_errors) std::cout << (error ? "ER " : "OK ");
        std::cout << "\n";
        
        // Count systems using logical functors
        auto all_systems_ok = std::all_of(both_ok.begin(), both_ok.end(),
                                        [](bool status) { return status; });
        
        auto any_system_ok = std::any_of(either_ok.begin(), either_ok.end(),
                                       [](bool status) { return status; });
        
        auto no_errors = std::none_of(system_errors.begin(), system_errors.end(),
                                    [](bool error) { return error; });
        
        std::cout << "\nAll systems fully operational: " << (all_systems_ok ? "Yes" : "No") << "\n";
        std::cout << "Any system operational: " << (any_system_ok ? "Yes" : "No") << "\n";
        std::cout << "No system errors: " << (no_errors ? "Yes" : "No") << "\n";
    }
};

/**
 * @brief Function Binding and Adaptation
 * 
 * Demonstrates std::bind, std::function, and function adaptation techniques.
 */
class FunctionBinding {
public:
    /**
     * @brief Sample free function for binding demonstrations
     */
    static double calculateDistance(double x1, double y1, double x2, double y2) {
        double dx = x2 - x1;
        double dy = y2 - y1;
        return std::sqrt(dx * dx + dy * dy);
    }
    
    /**
     * @brief Sample member function for binding demonstrations
     */
    static double calculateWarpTime(const Starship& ship, double distance) {
        if (ship.max_warp_speed <= 0) return std::numeric_limits<double>::infinity();
        
        // Simplified warp time calculation
        double warp_factor = std::pow(ship.max_warp_speed, 3.33); // Warp physics approximation
        return distance / warp_factor;
    }
    
    /**
     * @brief Demonstrate std::bind
     */
    static void demonstrateStdBind() {
        std::cout << "\n=== std::bind Demonstration ===\n";
        
        // Bind function arguments
        double earth_x = 0.0, earth_y = 0.0;
        
        // Create a function that calculates distance from Earth
        auto distance_from_earth = std::bind(calculateDistance, earth_x, earth_y, 
                                           std::placeholders::_1, std::placeholders::_2);
        
        std::cout << "Distances from Earth (0,0):\n";
        std::vector<std::pair<double, double>> locations{
            {3.5, 4.2}, {1.2, 8.9}, {7.1, 2.3}, {5.5, 6.7}
        };
        
        for (size_t i = 0; i < locations.size(); ++i) {
            auto [x, y] = locations[i];
            double dist = distance_from_earth(x, y);
            std::cout << "Location " << i + 1 << " (" << x << ", " << y << "): " 
                     << dist << " units\n";
        }
        
        // Bind with reordered arguments
        auto distance_reversed = std::bind(calculateDistance, 
                                         std::placeholders::_2, std::placeholders::_1,
                                         std::placeholders::_4, std::placeholders::_3);
        
        double dist_normal = calculateDistance(1.0, 2.0, 3.0, 4.0);
        double dist_reversed = distance_reversed(1.0, 2.0, 3.0, 4.0); // Args: _2,_1,_4,_3 = 2,1,4,3
        
        std::cout << "\nNormal distance(1,2,3,4): " << dist_normal << "\n";
        std::cout << "Reversed distance(1,2,3,4) -> (2,1,4,3): " << dist_reversed << "\n";
        
        // Bind member functions
        Starship enterprise("Enterprise", "Heavy Cruiser", 4500.0, 400, 9.0, 85.0);
        
        auto enterprise_travel_time = std::bind(calculateWarpTime, enterprise, std::placeholders::_1);
        
        std::cout << "\nTravel times for Enterprise:\n";
        std::vector<double> distances{10.0, 25.5, 50.0, 100.0};
        for (double distance : distances) {
            double time = enterprise_travel_time(distance);
            std::cout << "Distance " << distance << " ly: " << time << " time units\n";
        }
        
        // Bind with member function pointers (alternative syntax)
        std::vector<Starship> fleet{
            {"Enterprise", "Heavy Cruiser", 4500.0, 400, 9.0, 85.0},
            {"Defiant", "Escort", 350.0, 50, 9.5, 45.0},
            {"Voyager", "Explorer", 3200.0, 150, 9.975, 70.0}
        };
        
        // Using std::bind with member function
        auto get_warp_speed = std::bind(&Starship::max_warp_speed, std::placeholders::_1);
        
        std::cout << "\nWarp speeds using std::bind:\n";
        for (const auto& ship : fleet) {
            std::cout << ship.name << ": Warp " << get_warp_speed(ship) << "\n";
        }
    }
    
    /**
     * @brief Demonstrate std::function
     */
    static void demonstrateStdFunction() {
        std::cout << "\n=== std::function Demonstration ===\n";
        
        // std::function can hold various callable objects
        std::vector<std::function<bool(const Starship&)>> ship_filters;
        
        // Add lambda
        ship_filters.push_back([](const Starship& ship) { 
            return ship.max_warp_speed > 8.0; 
        });
        
        // Add function object
        ship_filters.push_back(IsCombatReady(60.0, 7.0));
        
        // Add bound function
        auto has_large_crew = std::bind([](int min_crew, const Starship& ship) {
            return ship.crew_capacity >= min_crew;
        }, 200, std::placeholders::_1);
        ship_filters.push_back(has_large_crew);
        
        std::vector<Starship> fleet{
            {"Enterprise", "Heavy Cruiser", 4500.0, 400, 9.0, 85.0},
            {"Defiant", "Escort", 350.0, 50, 9.5, 45.0},
            {"Voyager", "Explorer", 3200.0, 150, 9.975, 70.0},
            {"Miranda", "Light Cruiser", 1200.0, 100, 6.0, 35.0}
        };
        
        std::vector<std::string> filter_names{
            "High Warp Speed (> 8.0)",
            "Combat Ready (Shields >= 60, Warp >= 7)",
            "Large Crew (>= 200)"
        };
        
        std::cout << "Applying multiple filters to fleet:\n";
        
        for (size_t filter_idx = 0; filter_idx < ship_filters.size(); ++filter_idx) {
            std::cout << "\nFilter " << filter_idx + 1 << ": " << filter_names[filter_idx] << "\n";
            
            for (const auto& ship : fleet) {
                if (ship_filters[filter_idx](ship)) {
                    std::cout << "  ✓ " << ship.name << "\n";
                } else {
                    std::cout << "  ✗ " << ship.name << "\n";
                }
            }
        }
        
        // std::function with different signatures
        std::function<double(double, double)> math_operation;
        
        // Assign different operations
        math_operation = [](double a, double b) { return a + b; };
        std::cout << "\nAddition: 5.5 + 3.2 = " << math_operation(5.5, 3.2) << "\n";
        
        math_operation = [](double a, double b) { return a * b; };
        std::cout << "Multiplication: 5.5 * 3.2 = " << math_operation(5.5, 3.2) << "\n";
        
        math_operation = std::bind(calculateDistance, 0, 0, std::placeholders::_1, std::placeholders::_2);
        std::cout << "Distance from origin: (5.5, 3.2) = " << math_operation(5.5, 3.2) << "\n";
        
        // Function composition
        auto power_calculator = [](const Starship& ship) -> double {
            return ship.shield_strength * ship.max_warp_speed;
        };
        
        auto efficiency_calculator = [](double power, double mass) -> double {
            return power / mass;
        };
        
        std::function<double(const Starship&)> efficiency_from_ship = 
            [power_calculator, efficiency_calculator](const Starship& ship) {
                double power = power_calculator(ship);
                return efficiency_calculator(power, ship.mass_tons);
            };
        
        std::cout << "\nShip efficiency calculations:\n";
        for (const auto& ship : fleet) {
            double efficiency = efficiency_from_ship(ship);
            std::cout << ship.name << ": " << efficiency << " efficiency units\n";
        }
    }
    
    /**
     * @brief Demonstrate function adaptation and transformation
     */
    static void demonstrateFunctionAdaptation() {
        std::cout << "\n=== Function Adaptation Demonstration ===\n";
        
        std::vector<Starship> fleet{
            {"Enterprise", "Heavy Cruiser", 4500.0, 400, 9.0, 85.0},
            {"Defiant", "Escort", 350.0, 50, 9.5, 45.0},
            {"Voyager", "Explorer", 3200.0, 150, 9.975, 70.0}
        };
        
        // Create adaptable function objects
        auto extract_mass = [](const Starship& ship) { return ship.mass_tons; };
        auto extract_crew = [](const Starship& ship) { return ship.crew_capacity; };
        auto extract_warp = [](const Starship& ship) { return ship.max_warp_speed; };
        
        // Function composition for complex calculations
        auto calculate_crew_density = [extract_mass, extract_crew](const Starship& ship) {
            return static_cast<double>(extract_crew(ship)) / extract_mass(ship);
        };
        
        auto calculate_warp_mass_ratio = [extract_mass, extract_warp](const Starship& ship) {
            return extract_warp(ship) / (extract_mass(ship) / 1000.0); // Per 1000 tons
        };
        
        std::cout << "Ship analysis using function composition:\n";
        for (const auto& ship : fleet) {
            double crew_density = calculate_crew_density(ship);
            double warp_ratio = calculate_warp_mass_ratio(ship);
            
            std::cout << ship.name << ":\n";
            std::cout << "  Crew density: " << crew_density << " crew/ton\n";
            std::cout << "  Warp/mass ratio: " << warp_ratio << " warp per 1000 tons\n";
        }
        
        // Higher-order functions
        auto create_threshold_filter = [](double threshold) {
            return [threshold](const std::function<double(const Starship&)>& extractor) {
                return [threshold, extractor](const Starship& ship) {
                    return extractor(ship) > threshold;
                };
            };
        };
        
        auto high_threshold_filter = create_threshold_filter(8.0);
        auto warp_filter = high_threshold_filter(extract_warp);
        
        std::cout << "\nShips with warp > 8.0 (using higher-order function):\n";
        for (const auto& ship : fleet) {
            if (warp_filter(ship)) {
                std::cout << "✓ " << ship.name << " (Warp " << ship.max_warp_speed << ")\n";
            }
        }
        
        // Function pipeline
        auto create_pipeline = [](auto... functions) {
            return [functions...](const auto& input) {
                auto result = input;
                ((result = functions(result)), ...); // C++17 fold expression
                return result;
            };
        };
        
        // Pipeline for processing ship data
        auto normalize_mass = [](double mass) { return mass / 1000.0; }; // Convert to thousands
        auto apply_efficiency = [](double value) { return value * 0.85; }; // 85% efficiency
        auto round_value = [](double value) { return std::round(value * 100.0) / 100.0; };
        
        auto mass_pipeline = create_pipeline(normalize_mass, apply_efficiency, round_value);
        
        std::cout << "\nProcessed masses (normalized, 85% efficiency, rounded):\n";
        for (const auto& ship : fleet) {
            double processed_mass = mass_pipeline(ship.mass_tons);
            std::cout << ship.name << ": " << ship.mass_tons << " tons -> " 
                     << processed_mass << " processed units\n";
        }
    }
};

/**
 * @brief Performance Comparison of Function Objects
 * 
 * Compares performance of different callable object types.
 */
class FunctorPerformanceComparison {
public:
    /**
     * @brief Regular function for performance testing
     */
    static bool regular_function_predicate(int value) {
        return value % 2 == 0;
    }
    
    /**
     * @brief Function object for performance testing
     */
    struct FunctionObjectPredicate {
        bool operator()(int value) const {
            return value % 2 == 0;
        }
    };
    
    /**
     * @brief Compare performance of different callable types
     */
    static void compareCallablePerformance() {
        std::cout << "\n=== Callable Performance Comparison ===\n";
        
        const size_t test_size = 1000000;
        std::vector<int> test_data;
        test_data.reserve(test_size);
        
        // Generate test data
        for (size_t i = 0; i < test_size; ++i) {
            test_data.push_back(static_cast<int>(i));
        }
        
        // Test function pointer
        auto start = std::chrono::high_resolution_clock::now();
        auto count1 = std::count_if(test_data.begin(), test_data.end(), regular_function_predicate);
        auto end = std::chrono::high_resolution_clock::now();
        auto function_time = std::chrono::duration<double, std::milli>(end - start).count();
        
        // Test function object
        start = std::chrono::high_resolution_clock::now();
        auto count2 = std::count_if(test_data.begin(), test_data.end(), FunctionObjectPredicate{});
        end = std::chrono::high_resolution_clock::now();
        auto functor_time = std::chrono::duration<double, std::milli>(end - start).count();
        
        // Test lambda
        start = std::chrono::high_resolution_clock::now();
        auto count3 = std::count_if(test_data.begin(), test_data.end(), 
                                   [](int value) { return value % 2 == 0; });
        end = std::chrono::high_resolution_clock::now();
        auto lambda_time = std::chrono::duration<double, std::milli>(end - start).count();
        
        // Test std::function with lambda
        std::function<bool(int)> std_function_lambda = [](int value) { return value % 2 == 0; };
        start = std::chrono::high_resolution_clock::now();
        auto count4 = std::count_if(test_data.begin(), test_data.end(), std_function_lambda);
        end = std::chrono::high_resolution_clock::now();
        auto std_function_time = std::chrono::duration<double, std::milli>(end - start).count();
        
        std::cout << "Performance test results (" << test_size << " elements):\n";
        std::cout << "Function pointer: " << function_time << " ms (count: " << count1 << ")\n";
        std::cout << "Function object:  " << functor_time << " ms (count: " << count2 << ")\n";
        std::cout << "Lambda:          " << lambda_time << " ms (count: " << count3 << ")\n";
        std::cout << "std::function:   " << std_function_time << " ms (count: " << count4 << ")\n";
        
        std::cout << "\nRelative performance (lower is better):\n";
        double baseline = std::min({function_time, functor_time, lambda_time, std_function_time});
        std::cout << "Function pointer: " << (function_time / baseline) << "x\n";
        std::cout << "Function object:  " << (functor_time / baseline) << "x\n";
        std::cout << "Lambda:          " << (lambda_time / baseline) << "x\n";
        std::cout << "std::function:   " << (std_function_time / baseline) << "x\n";
        
        std::cout << "\nKey insights:\n";
        std::cout << "- Lambdas and function objects are often fastest (inlined)\n";
        std::cout << "- Function pointers prevent inlining\n";
        std::cout << "- std::function has call overhead but offers flexibility\n";
        std::cout << "- Choose based on performance needs vs. flexibility\n";
    }
    
    /**
     * @brief Demonstrate functor state and its performance implications
     */
    static void demonstrateStatefulFunctorPerformance() {
        std::cout << "\n=== Stateful Functor Performance ===\n";
        
        const size_t test_size = 100000;
        std::vector<int> test_data;
        for (size_t i = 0; i < test_size; ++i) {
            test_data.push_back(static_cast<int>(i % 100));
        }
        
        // Stateful functor that counts calls
        struct CountingPredicate {
            mutable int call_count = 0;
            
            bool operator()(int value) const {
                call_count++;
                return value > 50;
            }
            
            int getCallCount() const { return call_count; }
        };
        
        // Test stateful functor
        CountingPredicate counter;
        auto start = std::chrono::high_resolution_clock::now();
        auto count = std::count_if(test_data.begin(), test_data.end(), std::ref(counter));
        auto end = std::chrono::high_resolution_clock::now();
        auto stateful_time = std::chrono::duration<double, std::milli>(end - start).count();
        
        std::cout << "Stateful functor test:\n";
        std::cout << "Elements > 50: " << count << "\n";
        std::cout << "Function calls: " << counter.getCallCount() << "\n";
        std::cout << "Execution time: " << stateful_time << " ms\n";
        
        // Compare with stateless lambda
        start = std::chrono::high_resolution_clock::now();
        auto lambda_count = std::count_if(test_data.begin(), test_data.end(),
                                        [](int value) { return value > 50; });
        end = std::chrono::high_resolution_clock::now();
        auto stateless_time = std::chrono::duration<double, std::milli>(end - start).count();
        
        std::cout << "\nStateless lambda comparison:\n";
        std::cout << "Elements > 50: " << lambda_count << "\n";
        std::cout << "Execution time: " << stateless_time << " ms\n";
        std::cout << "Performance ratio: " << (stateful_time / stateless_time) << "x\n";
        
        std::cout << "\nStateful functors provide:\n";
        std::cout << "+ Ability to maintain state across calls\n";
        std::cout << "+ More complex logic possibilities\n";
        std::cout << "- Slight performance overhead\n";
        std::cout << "- Potential thread safety concerns\n";
    }
};

} // namespace CppVerseHub::STL