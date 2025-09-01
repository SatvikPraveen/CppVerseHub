/**
 * @file STLUtilities.hpp
 * @brief STL Utilities Demonstrations (pair, tuple, optional, variant, any)
 * @details File location: src/stl_showcase/STLUtilities.hpp
 */

#pragma once

#include <utility>
#include <tuple>
#include <optional>
#include <variant>
#include <any>
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <type_traits>
#include <functional>
#include <algorithm>
#include <numeric>
#include <cmath>

namespace CppVerseHub::STL {

/**
 * @brief Navigation coordinate structure for utility demonstrations
 */
struct NavigationCoordinate {
    double x, y, z;
    std::string reference_frame;
    
    NavigationCoordinate(double x = 0.0, double y = 0.0, double z = 0.0, 
                        const std::string& frame = "Galactic")
        : x(x), y(y), z(z), reference_frame(frame) {}
    
    friend std::ostream& operator<<(std::ostream& os, const NavigationCoordinate& coord) {
        os << "(" << coord.x << ", " << coord.y << ", " << coord.z 
           << ") [" << coord.reference_frame << "]";
        return os;
    }
};

/**
 * @brief Vessel status for variant demonstrations
 */
enum class VesselStatus {
    DOCKED,
    IN_TRANSIT,
    EXPLORING,
    COMBAT,
    MAINTENANCE,
    EMERGENCY
};

/**
 * @brief std::pair Demonstrations
 * 
 * Shows usage of std::pair for returning multiple values,
 * creating key-value associations, and coordinate pairs.
 */
class PairDemonstrations {
public:
    /**
     * @brief Demonstrate basic pair usage
     */
    static void demonstrateBasicPair() {
        std::cout << "\n=== std::pair Basic Demonstrations ===\n";
        
        // Creating pairs
        std::pair<std::string, int> ship_crew{"Enterprise", 430};
        auto fuel_status = std::make_pair("Deuterium", 85.5); // auto deduction
        
        std::cout << "Ship and crew: " << ship_crew.first << " has " << ship_crew.second << " crew members\n";
        std::cout << "Fuel status: " << fuel_status.first << " at " << fuel_status.second << "%\n";
        
        // Pair operations
        std::cout << "\nPair operations:\n";
        auto coordinates = std::make_pair(125.7, 89.3);
        std::cout << "Original coordinates: (" << coordinates.first << ", " << coordinates.second << ")\n";
        
        // Swapping elements within pair
        if (coordinates.first < coordinates.second) {
            std::swap(coordinates.first, coordinates.second);
        }
        std::cout << "After conditional swap: (" << coordinates.first << ", " << coordinates.second << ")\n";
        
        // Pair comparison
        std::pair<int, int> priority1{5, 100};
        std::pair<int, int> priority2{5, 200};
        std::pair<int, int> priority3{6, 50};
        
        std::cout << "\nPair comparisons (lexicographic order):\n";
        std::cout << "priority1 (5,100) < priority2 (5,200): " << (priority1 < priority2) << "\n";
        std::cout << "priority2 (5,200) < priority3 (6,50): " << (priority2 < priority3) << "\n";
        
        // Using pairs with algorithms
        std::vector<std::pair<std::string, double>> distance_readings{
            {"Alpha Centauri", 4.37},
            {"Proxima Centauri", 4.24},
            {"Barnard's Star", 5.96},
            {"Wolf 359", 7.86}
        };
        
        // Sort by distance
        std::sort(distance_readings.begin(), distance_readings.end(),
                 [](const auto& a, const auto& b) { return a.second < b.second; });
        
        std::cout << "\nStar systems by distance:\n";
        for (const auto& [system, distance] : distance_readings) {
            std::cout << "- " << system << ": " << distance << " ly\n";
        }
    }
    
    /**
     * @brief Demonstrate pair with containers
     */
    static void demonstratePairWithContainers() {
        std::cout << "\n=== std::pair with Containers ===\n";
        
        // Map naturally uses pairs
        std::map<std::string, std::vector<std::string>> fleet_assignments{
            {"Exploration", {"Voyager", "Discovery", "Enterprise"}},
            {"Defense", {"Defiant", "Prometheus", "Sovereign"}},
            {"Diplomacy", {"Ambassador", "Galaxy", "Nebula"}}
        };
        
        std::cout << "Fleet assignments:\n";
        for (const auto& [mission_type, ships] : fleet_assignments) {
            std::cout << mission_type << " missions: ";
            for (const auto& ship : ships) std::cout << ship << " ";
            std::cout << "\n";
        }
        
        // Using pair as container element
        std::vector<std::pair<NavigationCoordinate, std::string>> waypoints{
            {{100.0, 200.0, 50.0, "Sol System"}, "Earth Orbit"},
            {{-50.0, 300.0, 125.0, "Sol System"}, "Mars Station"},
            {{0.0, 500.0, 200.0, "Sol System"}, "Jupiter Colony"}
        };
        
        std::cout << "\nNavigation waypoints:\n";
        for (size_t i = 0; i < waypoints.size(); ++i) {
            const auto& [coord, description] = waypoints[i];
            std::cout << i + 1 << ". " << description << " at " << coord << "\n";
        }
        
        // Find closest waypoint to origin
        auto closest = std::min_element(waypoints.begin(), waypoints.end(),
            [](const auto& a, const auto& b) {
                auto dist_a = std::sqrt(a.first.x*a.first.x + a.first.y*a.first.y + a.first.z*a.first.z);
                auto dist_b = std::sqrt(b.first.x*b.first.x + b.first.y*b.first.y + b.first.z*b.first.z);
                return dist_a < dist_b;
            });
        
        if (closest != waypoints.end()) {
            std::cout << "Closest waypoint to origin: " << closest->second << "\n";
        }
    }
    
    /**
     * @brief Demonstrate pair return values
     */
    static void demonstratePairReturnValues() {
        std::cout << "\n=== std::pair Return Values ===\n";
        
        // Function returning pair
        auto analyze_fleet_status = [](const std::vector<std::string>& fleet) -> std::pair<int, double> {
            int operational_ships = 0;
            double total_efficiency = 0.0;
            
            for (const auto& ship : fleet) {
                operational_ships++;
                // Simulate efficiency calculation
                total_efficiency += 0.75 + (ship.length() % 10) * 0.02; // Mock calculation
            }
            
            double avg_efficiency = operational_ships > 0 ? total_efficiency / operational_ships : 0.0;
            return {operational_ships, avg_efficiency};
        };
        
        std::vector<std::string> fleet{"Enterprise", "Voyager", "Defiant", "Discovery"};
        
        auto [ship_count, efficiency] = analyze_fleet_status(fleet); // Structured binding (C++17)
        std::cout << "Fleet analysis:\n";
        std::cout << "Operational ships: " << ship_count << "\n";
        std::cout << "Average efficiency: " << efficiency << "\n";
        
        // Using pair for min/max results
        auto find_min_max_crew = [](const std::vector<std::pair<std::string, int>>& ship_crews) {
            if (ship_crews.empty()) return std::make_pair(std::string(""), std::string(""));
            
            auto min_it = std::min_element(ship_crews.begin(), ship_crews.end(),
                [](const auto& a, const auto& b) { return a.second < b.second; });
            
            auto max_it = std::max_element(ship_crews.begin(), ship_crews.end(),
                [](const auto& a, const auto& b) { return a.second < b.second; });
            
            return std::make_pair(min_it->first, max_it->first);
        };
        
        std::vector<std::pair<std::string, int>> ship_crews{
            {"Enterprise", 430}, {"Voyager", 150}, {"Defiant", 50}, {"Discovery", 136}
        };
        
        auto [min_crew_ship, max_crew_ship] = find_min_max_crew(ship_crews);
        std::cout << "\nCrew analysis:\n";
        std::cout << "Smallest crew: " << min_crew_ship << "\n";
        std::cout << "Largest crew: " << max_crew_ship << "\n";
    }
};

/**
 * @brief std::tuple Demonstrations
 * 
 * Shows usage of std::tuple for multiple return values,
 * structured data, and tuple manipulation.
 */
class TupleDemonstrations {
public:
    /**
     * @brief Demonstrate basic tuple usage
     */
    static void demonstrateBasicTuple() {
        std::cout << "\n=== std::tuple Basic Demonstrations ===\n";
        
        // Creating tuples
        std::tuple<std::string, int, double, bool> ship_stats{"Enterprise", 430, 9.0, true};
        auto mission_data = std::make_tuple("Exploration", 72.5, 150, "Alpha Quadrant");
        
        // Accessing tuple elements
        std::cout << "Ship statistics:\n";
        std::cout << "Name: " << std::get<0>(ship_stats) << "\n";
        std::cout << "Crew: " << std::get<1>(ship_stats) << "\n";
        std::cout << "Warp Speed: " << std::get<2>(ship_stats) << "\n";
        std::cout << "Operational: " << (std::get<3>(ship_stats) ? "Yes" : "No") << "\n";
        
        // Using structured bindings (C++17)
        auto [mission_type, completion, crew_count, location] = mission_data;
        std::cout << "\nMission data:\n";
        std::cout << "Type: " << mission_type << "\n";
        std::cout << "Completion: " << completion << "%\n";
        std::cout << "Crew involved: " << crew_count << "\n";
        std::cout << "Location: " << location << "\n";
        
        // Tuple size and type information
        std::cout << "\nTuple metadata:\n";
        std::cout << "Ship stats tuple size: " << std::tuple_size_v<decltype(ship_stats)> << "\n";
        std::cout << "Mission data tuple size: " << std::tuple_size_v<decltype(mission_data)> << "\n";
        
        // Type of specific elements
        using second_element_type = std::tuple_element_t<1, decltype(ship_stats)>;
        std::cout << "Second element is int: " << std::is_same_v<second_element_type, int> << "\n";
    }
    
    /**
     * @brief Demonstrate tuple operations
     */
    static void demonstrateTupleOperations() {
        std::cout << "\n=== std::tuple Operations ===\n";
        
        // Tuple comparison
        std::tuple<int, std::string> priority1{5, "High"};
        std::tuple<int, std::string> priority2{5, "Medium"};
        std::tuple<int, std::string> priority3{6, "Low"};
        
        std::cout << "Tuple comparisons (lexicographic):\n";
        std::cout << "(5, High) < (5, Medium): " << (priority1 < priority2) << "\n";
        std::cout << "(5, Medium) < (6, Low): " << (priority2 < priority3) << "\n";
        
        // Tuple concatenation
        auto basic_info = std::make_tuple("Voyager", 150);
        auto extended_info = std::make_tuple(9.975, true, "Delta Quadrant");
        
        auto complete_info = std::tuple_cat(basic_info, extended_info);
        
        auto [name, crew, warp, operational, quadrant] = complete_info;
        std::cout << "\nConcatenated tuple info:\n";
        std::cout << name << " (Crew: " << crew << ", Warp: " << warp 
                 << ", Operational: " << operational << ", Location: " << quadrant << ")\n";
        
        // Tuple swapping
        std::tuple<std::string, double> coord1{"Alpha", 125.5};
        std::tuple<std::string, double> coord2{"Beta", 89.3};
        
        std::cout << "\nBefore swap: " << std::get<0>(coord1) << "=" << std::get<1>(coord1)
                 << ", " << std::get<0>(coord2) << "=" << std::get<1>(coord2) << "\n";
        
        std::swap(coord1, coord2);
        
        std::cout << "After swap: " << std::get<0>(coord1) << "=" << std::get<1>(coord1)
                 << ", " << std::get<0>(coord2) << "=" << std::get<1>(coord2) << "\n";
        
        // Using tie for multiple assignment
        std::string station_name;
        int station_crew;
        double station_power;
        
        std::tie(station_name, station_crew, station_power) = 
            std::make_tuple("Deep Space 9", 300, 150.5);
        
        std::cout << "\nUsing std::tie assignment:\n";
        std::cout << "Station: " << station_name << ", Crew: " << station_crew 
                 << ", Power: " << station_power << "\n";
    }
    
    /**
     * @brief Demonstrate tuple with algorithms
     */
    static void demonstrateTupleWithAlgorithms() {
        std::cout << "\n=== std::tuple with Algorithms ===\n";
        
        // Vector of tuples for complex data
        std::vector<std::tuple<std::string, NavigationCoordinate, VesselStatus, int>> fleet_data{
            {"Enterprise", {0.0, 0.0, 0.0, "Sol"}, VesselStatus::DOCKED, 430},
            {"Voyager", {70000.0, 0.0, 1000.0, "Delta"}, VesselStatus::EXPLORING, 150},
            {"Defiant", {-10.0, 5.0, 2.0, "Sol"}, VesselStatus::COMBAT, 50},
            {"Discovery", {25.0, -15.0, 8.0, "Sol"}, VesselStatus::IN_TRANSIT, 136}
        };
        
        std::cout << "Fleet status report:\n";
        for (const auto& [name, position, status, crew] : fleet_data) {
            std::cout << name << " at " << position << " - Crew: " << crew;
            
            switch (status) {
                case VesselStatus::DOCKED: std::cout << " (Docked)"; break;
                case VesselStatus::IN_TRANSIT: std::cout << " (In Transit)"; break;
                case VesselStatus::EXPLORING: std::cout << " (Exploring)"; break;
                case VesselStatus::COMBAT: std::cout << " (Combat Ready)"; break;
                case VesselStatus::MAINTENANCE: std::cout << " (Maintenance)"; break;
                case VesselStatus::EMERGENCY: std::cout << " (Emergency)"; break;
            }
            std::cout << "\n";
        }
        
        // Sort by crew size
        std::sort(fleet_data.begin(), fleet_data.end(),
                 [](const auto& a, const auto& b) {
                     return std::get<3>(a) > std::get<3>(b); // Sort by crew (descending)
                 });
        
        std::cout << "\nFleet sorted by crew size (largest first):\n";
        for (const auto& [name, position, status, crew] : fleet_data) {
            std::cout << name << ": " << crew << " crew\n";
        }
        
        // Find ships in Sol system
        auto sol_ships_count = std::count_if(fleet_data.begin(), fleet_data.end(),
            [](const auto& ship_data) {
                const auto& [name, position, status, crew] = ship_data;
                return position.reference_frame == "Sol";
            });
        
        std::cout << "\nShips in Sol system: " << sol_ships_count << "/" << fleet_data.size() << "\n";
        
        // Calculate total crew
        auto total_crew = std::accumulate(fleet_data.begin(), fleet_data.end(), 0,
            [](int sum, const auto& ship_data) {
                return sum + std::get<3>(ship_data);
            });
        
        std::cout << "Total fleet crew: " << total_crew << " personnel\n";
    }
    
    /**
     * @brief Demonstrate tuple return values from functions
     */
    static void demonstrateTupleReturnValues() {
        std::cout << "\n=== std::tuple Return Values ===\n";
        
        // Function returning complex analysis
        auto analyze_sector = [](const std::vector<NavigationCoordinate>& coordinates) {
            if (coordinates.empty()) {
                return std::make_tuple(0.0, 0.0, 0.0, NavigationCoordinate{}, std::string("Empty"));
            }
            
            double sum_x = 0, sum_y = 0, sum_z = 0;
            double max_distance = 0.0;
            NavigationCoordinate center_point;
            
            for (const auto& coord : coordinates) {
                sum_x += coord.x;
                sum_y += coord.y;
                sum_z += coord.z;
                
                double distance = std::sqrt(coord.x*coord.x + coord.y*coord.y + coord.z*coord.z);
                if (distance > max_distance) {
                    max_distance = distance;
                }
            }
            
            center_point.x = sum_x / coordinates.size();
            center_point.y = sum_y / coordinates.size();
            center_point.z = sum_z / coordinates.size();
            center_point.reference_frame = coordinates[0].reference_frame;
            
            std::string classification = max_distance > 100.0 ? "Extended" : 
                                       max_distance > 50.0 ? "Standard" : "Compact";
            
            return std::make_tuple(max_distance, 
                                 static_cast<double>(coordinates.size()),
                                 max_distance / coordinates.size(), // Spread factor
                                 center_point,
                                 classification);
        };
        
        std::vector<NavigationCoordinate> sector_coords{
            {10.0, 20.0, 5.0, "Alpha Sector"},
            {-5.0, 15.0, 8.0, "Alpha Sector"},
            {25.0, -10.0, 12.0, "Alpha Sector"},
            {0.0, 30.0, -3.0, "Alpha Sector"}
        };
        
        auto [max_dist, point_count, spread_factor, center, classification] = analyze_sector(sector_coords);
        
        std::cout << "Sector analysis results:\n";
        std::cout << "Maximum distance from origin: " << max_dist << "\n";
        std::cout << "Number of points: " << static_cast<int>(point_count) << "\n";
        std::cout << "Spread factor: " << spread_factor << "\n";
        std::cout << "Center point: " << center << "\n";
        std::cout << "Classification: " << classification << "\n";
        
        // Multiple function calls with tuple unpacking
        auto get_ship_specs = [](const std::string& ship_class) {
            if (ship_class == "Cruiser") {
                return std::make_tuple(4500.0, 400, 9.0, 85.0);
            } else if (ship_class == "Escort") {
                return std::make_tuple(350.0, 50, 9.5, 45.0);
            } else {
                return std::make_tuple(2000.0, 200, 8.0, 60.0);
            }
        };
        
        std::vector<std::string> ship_classes{"Cruiser", "Escort", "Explorer"};
        
        std::cout << "\nShip class specifications:\n";
        for (const auto& ship_class : ship_classes) {
            auto [mass, crew, warp, shields] = get_ship_specs(ship_class);
            std::cout << ship_class << ": Mass=" << mass << ", Crew=" << crew 
                     << ", Warp=" << warp << ", Shields=" << shields << "\n";
        }
    }
};

/**
 * @brief std::optional Demonstrations
 * 
 * Shows usage of std::optional for handling nullable values,
 * safe operations, and avoiding null pointer issues.
 */
class OptionalDemonstrations {
public:
    /**
     * @brief Demonstrate basic optional usage
     */
    static void demonstrateBasicOptional() {
        std::cout << "\n=== std::optional Basic Demonstrations ===\n";
        
        // Creating optionals
        std::optional<std::string> ship_name = "Enterprise";
        std::optional<int> crew_count = 430;
        std::optional<double> warp_speed; // Empty optional
        
        // Checking if optional has value
        std::cout << "Ship name has value: " << ship_name.has_value() << "\n";
        std::cout << "Warp speed has value: " << warp_speed.has_value() << "\n";
        
        // Accessing values safely
        if (ship_name) { // Implicit bool conversion
            std::cout << "Ship name: " << *ship_name << "\n";
        }
        
        if (crew_count.has_value()) {
            std::cout << "Crew count: " << crew_count.value() << "\n";
        }
        
        // Using value_or for default values
        std::cout << "Warp speed: " << warp_speed.value_or(0.0) << " (default if empty)\n";
        
        // Assigning values to optional
        warp_speed = 9.0;
        std::cout << "After assignment, warp speed: " << warp_speed.value() << "\n";
        
        // Resetting optional
        ship_name.reset();
        std::cout << "After reset, ship name has value: " << ship_name.has_value() << "\n";
        
        // Using nullopt
        std::optional<std::string> mission_status = std::nullopt;
        std::cout << "Mission status: " << mission_status.value_or("Unknown") << "\n";
    }
    
    /**
     * @brief Demonstrate optional with functions
     */
    static void demonstrateOptionalWithFunctions() {
        std::cout << "\n=== std::optional with Functions ===\n";
        
        // Function that may not return a value
        auto find_ship_by_registry = [](const std::vector<std::pair<std::string, std::string>>& fleet,
                                       const std::string& registry) -> std::optional<std::string> {
            auto it = std::find_if(fleet.begin(), fleet.end(),
                [&registry](const auto& ship) { return ship.first == registry; });
            
            return (it != fleet.end()) ? std::make_optional(it->second) : std::nullopt;
        };
        
        std::vector<std::pair<std::string, std::string>> fleet{
            {"NCC-1701", "Enterprise"},
            {"NCC-74656", "Voyager"},
            {"NX-74205", "Defiant"}
        };
        
        // Successful lookup
        auto enterprise = find_ship_by_registry(fleet, "NCC-1701");
        if (enterprise) {
            std::cout << "Found ship: " << *enterprise << "\n";
        }
        
        // Failed lookup
        auto unknown_ship = find_ship_by_registry(fleet, "NCC-9999");
        std::cout << "Unknown registry result: " << unknown_ship.value_or("Not found") << "\n";
        
        // Function with optional parameters
        auto calculate_eta = [](double distance, std::optional<double> warp_factor = std::nullopt) -> double {
            double speed = warp_factor.value_or(1.0); // Default to warp 1
            double warp_multiplier = std::pow(speed, 3.33); // Warp physics
            return distance / warp_multiplier;
        };
        
        double distance = 50.0;
        std::cout << "\nETA calculations for " << distance << " light years:\n";
        std::cout << "Default warp: " << calculate_eta(distance) << " time units\n";
        std::cout << "Warp 5: " << calculate_eta(distance, 5.0) << " time units\n";
        std::cout << "Warp 9: " << calculate_eta(distance, 9.0) << " time units\n";
        
        // Chain of optional operations
        auto get_ship_warp_rating = [](const std::string& ship_name) -> std::optional<double> {
            if (ship_name == "Enterprise") return 9.0;
            if (ship_name == "Voyager") return 9.975;
            if (ship_name == "Defiant") return 9.5;
            return std::nullopt;
        };
        
        auto calculate_max_range = [](double warp_rating) -> double {
            return warp_rating * 1000.0; // Simplified calculation
        };
        
        std::vector<std::string> ships{"Enterprise", "Voyager", "Unknown Ship"};
        
        std::cout << "\nShip maximum ranges:\n";
        for (const auto& ship : ships) {
            auto warp_rating = get_ship_warp_rating(ship);
            if (warp_rating) {
                double max_range = calculate_max_range(*warp_rating);
                std::cout << ship << ": " << max_range << " light years\n";
            } else {
                std::cout << ship << ": Range unknown (no warp rating)\n";
            }
        }
    }
    
    /**
     * @brief Demonstrate optional transformations
     */
    static void demonstrateOptionalTransformations() {
        std::cout << "\n=== std::optional Transformations ===\n";
        
        // Using transform with optional (C++23 feature, simulated here)
        auto transform_if_present = [](auto opt, auto func) -> std::optional<decltype(func(*opt))> {
            return opt ? std::make_optional(func(*opt)) : std::nullopt;
        };
        
        std::optional<int> crew_size = 150;
        std::optional<int> empty_crew;
        
        // Transform crew size to crew categories
        auto categorize_crew = [](int size) -> std::string {
            if (size < 50) return "Small";
            if (size < 200) return "Medium";
            return "Large";
        };
        
        auto crew_category = transform_if_present(crew_size, categorize_crew);
        auto empty_category = transform_if_present(empty_crew, categorize_crew);
        
        std::cout << "Crew size " << crew_size.value_or(0) << " -> Category: " 
                 << crew_category.value_or("Unknown") << "\n";
        std::cout << "Empty crew -> Category: " << empty_category.value_or("Unknown") << "\n";
        
        // Optional chaining for complex operations
        auto parse_coordinate = [](const std::string& coord_str) -> std::optional<double> {
            try {
                return std::stod(coord_str);
            } catch (...) {
                return std::nullopt;
            }
        };
        
        auto validate_coordinate = [](double coord) -> std::optional<double> {
            return (coord >= -1000.0 && coord <= 1000.0) ? std::make_optional(coord) : std::nullopt;
        };
        
        auto format_coordinate = [](double coord) -> std::string {
            return std::to_string(coord) + " units";
        };
        
        std::vector<std::string> coordinate_inputs{"125.5", "invalid", "2000.0", "-500.2"};
        
        std::cout << "\nCoordinate processing chain:\n";
        for (const auto& input : coordinate_inputs) {
            std::cout << "Input: \"" << input << "\" -> ";
            
            auto parsed = parse_coordinate(input);
            if (parsed) {
                auto validated = validate_coordinate(*parsed);
                if (validated) {
                    std::cout << "Valid: " << format_coordinate(*validated) << "\n";
                } else {
                    std::cout << "Invalid: out of range\n";
                }
            } else {
                std::cout << "Invalid: parse error\n";
            }
        }
        
        // Using optional with containers
        std::vector<std::optional<std::string>> mission_reports{
            "Mission completed successfully",
            std::nullopt,
            "Encountered hostile forces",
            std::nullopt,
            "All objectives achieved"
        };
        
        std::cout << "\nMission reports:\n";
        for (size_t i = 0; i < mission_reports.size(); ++i) {
            std::cout << "Mission " << i + 1 << ": ";
            if (mission_reports[i]) {
                std::cout << *mission_reports[i] << "\n";
            } else {
                std::cout << "[No report available]\n";
            }
        }
        
        // Count valid reports
        auto valid_reports = std::count_if(mission_reports.begin(), mission_reports.end(),
            [](const auto& report) { return report.has_value(); });
        
        std::cout << "Valid reports: " << valid_reports << "/" << mission_reports.size() << "\n";
    }
};

/**
 * @brief std::variant Demonstrations
 * 
 * Shows usage of std::variant for type-safe unions,
 * handling multiple types, and visitor patterns.
 */
class VariantDemonstrations {
public:
    /**
     * @brief Demonstrate basic variant usage
     */
    static void demonstrateBasicVariant() {
        std::cout << "\n=== std::variant Basic Demonstrations ===\n";
        
        // Creating variants that can hold different types
        std::variant<int, double, std::string> sensor_reading;
        
        // Assigning different types
        sensor_reading = 42;
        std::cout << "Integer reading: " << std::get<int>(sensor_reading) << "\n";
        
        sensor_reading = 98.6;
        std::cout << "Double reading: " << std::get<double>(sensor_reading) << "\n";
        
        sensor_reading = "Temperature nominal";
        std::cout << "String reading: " << std::get<std::string>(sensor_reading) << "\n";
        
        // Check which type is currently held
        std::cout << "\nCurrent variant index: " << sensor_reading.index() << "\n";
        std::cout << "Holds int: " << std::holds_alternative<int>(sensor_reading) << "\n";
        std::cout << "Holds double: " << std::holds_alternative<double>(sensor_reading) << "\n";
        std::cout << "Holds string: " << std::holds_alternative<std::string>(sensor_reading) << "\n";
        
        // Safe access with get_if
        if (auto* str_ptr = std::get_if<std::string>(&sensor_reading)) {
            std::cout << "String value via get_if: " << *str_ptr << "\n";
        }
        
        if (auto* int_ptr = std::get_if<int>(&sensor_reading)) {
            std::cout << "This won't print (variant doesn't hold int)\n";
        } else {
            std::cout << "Variant doesn't currently hold an int\n";
        }
    }
    
    /**
     * @brief Demonstrate variant with complex types
     */
    static void demonstrateVariantWithComplexTypes() {
        std::cout << "\n=== std::variant with Complex Types ===\n";
        
        // Variant for different types of space objects
        using SpaceObject = std::variant<NavigationCoordinate, std::string, VesselStatus, std::vector<double>>;
        
        std::vector<SpaceObject> space_data;
        
        // Add different types of objects
        space_data.emplace_back(NavigationCoordinate{100.0, 200.0, 50.0, "Sol System"});
        space_data.emplace_back(std::string("USS Enterprise"));
        space_data.emplace_back(VesselStatus::IN_TRANSIT);
        space_data.emplace_back(std::vector<double>{1.5, 2.3, 4.7, 8.1});
        
        std::cout << "Space object data:\n";
        for (size_t i = 0; i < space_data.size(); ++i) {
            std::cout << "Object " << i + 1 << ": ";
            
            // Using visitor pattern with std::visit
            std::visit([](const auto& obj) {
                using T = std::decay_t<decltype(obj)>;
                
                if constexpr (std::is_same_v<T, NavigationCoordinate>) {
                    std::cout << "Coordinate " << obj;
                }
                else if constexpr (std::is_same_v<T, std::string>) {
                    std::cout << "Ship name: " << obj;
                }
                else if constexpr (std::is_same_v<T, VesselStatus>) {
                    std::cout << "Status: ";
                    switch (obj) {
                        case VesselStatus::DOCKED: std::cout << "Docked"; break;
                        case VesselStatus::IN_TRANSIT: std::cout << "In Transit"; break;
                        case VesselStatus::EXPLORING: std::cout << "Exploring"; break;
                        case VesselStatus::COMBAT: std::cout << "Combat Ready"; break;
                        case VesselStatus::MAINTENANCE: std::cout << "Maintenance"; break;
                        case VesselStatus::EMERGENCY: std::cout << "Emergency"; break;
                    }
                }
                else if constexpr (std::is_same_v<T, std::vector<double>>) {
                    std::cout << "Sensor array: ";
                    for (const auto& value : obj) std::cout << value << " ";
                }
            }, space_data[i]);
            
            std::cout << "\n";
        }
        
        // Count objects by type
        auto coord_count = std::count_if(space_data.begin(), space_data.end(),
            [](const SpaceObject& obj) { return std::holds_alternative<NavigationCoordinate>(obj); });
        
        auto string_count = std::count_if(space_data.begin(), space_data.end(),
            [](const SpaceObject& obj) { return std::holds_alternative<std::string>(obj); });
        
        std::cout << "\nType distribution:\n";
        std::cout << "Coordinates: " << coord_count << "\n";
        std::cout << "Strings: " << string_count << "\n";
    }
    
    /**
     * @brief Demonstrate variant visitor patterns
     */
    static void demonstrateVariantVisitors() {
        std::cout << "\n=== std::variant Visitor Patterns ===\n";
        
        // Command variant for different operations
        using Command = std::variant<
            std::tuple<std::string, double, double, double>, // Move command: (ship, x, y, z)
            std::tuple<std::string, std::string>,            // Attack command: (attacker, target)
            std::tuple<std::string, int>,                    // Repair command: (ship, repair_points)
            std::string                                      // Status request: (ship)
        >;
        
        std::vector<Command> command_queue{
            std::make_tuple(std::string("Enterprise"), 100.0, 200.0, 50.0),
            std::make_tuple(std::string("Defiant"), std::string("Enemy Ship")),
            std::make_tuple(std::string("Voyager"), 25),
            std::string("Discovery")
        };
        
        // Generic visitor using overload pattern
        struct CommandVisitor {
            void operator()(const std::tuple<std::string, double, double, double>& move_cmd) const {
                const auto& [ship, x, y, z] = move_cmd;
                std::cout << "MOVE: " << ship << " to (" << x << ", " << y << ", " << z << ")\n";
            }
            
            void operator()(const std::tuple<std::string, std::string>& attack_cmd) const {
                const auto& [attacker, target] = attack_cmd;
                std::cout << "ATTACK: " << attacker << " attacks " << target << "\n";
            }
            
            void operator()(const std::tuple<std::string, int>& repair_cmd) const {
                const auto& [ship, points] = repair_cmd;
                std::cout << "REPAIR: " << ship << " repairs " << points << " hull points\n";
            }
            
            void operator()(const std::string& status_cmd) const {
                std::cout << "STATUS: Request status of " << status_cmd << "\n";
            }
        };
        
        std::cout << "Processing command queue:\n";
        for (const auto& command : command_queue) {
            std::visit(CommandVisitor{}, command);
        }
        
        // Lambda visitor for calculations
        auto calculate_command_priority = [](const Command& cmd) -> int {
            return std::visit([](const auto& specific_cmd) -> int {
                using T = std::decay_t<decltype(specific_cmd)>;
                
                if constexpr (std::is_same_v<T, std::tuple<std::string, double, double, double>>) {
                    return 3; // Move command priority
                }
                else if constexpr (std::is_same_v<T, std::tuple<std::string, std::string>>) {
                    return 9; // Attack command priority (high)
                }
                else if constexpr (std::is_same_v<T, std::tuple<std::string, int>>) {
                    return 5; // Repair command priority
                }
                else if constexpr (std::is_same_v<T, std::string>) {
                    return 1; // Status request priority (low)
                }
                
                return 0;
            }, cmd);
        };
        
        std::cout << "\nCommand priorities:\n";
        for (size_t i = 0; i < command_queue.size(); ++i) {
            int priority = calculate_command_priority(command_queue[i]);
            std::cout << "Command " << i + 1 << ": Priority " << priority << "\n";
        }
        
        // Sort commands by priority
        std::sort(command_queue.begin(), command_queue.end(),
            [&calculate_command_priority](const Command& a, const Command& b) {
                return calculate_command_priority(a) > calculate_command_priority(b);
            });
        
        std::cout << "\nCommands sorted by priority (highest first):\n";
        for (const auto& command : command_queue) {
            std::visit(CommandVisitor{}, command);
        }
    }
    
    /**
     * @brief Demonstrate variant error handling
     */
    static void demonstrateVariantErrorHandling() {
        std::cout << "\n=== std::variant Error Handling ===\n";
        
        // Result type that can represent success or various error types
        using Result = std::variant<
            std::string,                    // Success with message
            std::tuple<int, std::string>,   // Error with code and message
            std::exception_ptr             // Exception
        >;
        
        // Function that returns different result types
        auto process_navigation_data = [](const std::string& input) -> Result {
            if (input.empty()) {
                return std::make_tuple(400, "Empty input data");
            }
            
            if (input == "invalid") {
                try {
                    throw std::runtime_error("Invalid navigation format");
                } catch (...) {
                    return std::current_exception();
                }
            }
            
            if (input.find("error") != std::string::npos) {
                return std::make_tuple(500, "Processing error detected");
            }
            
            return "Navigation data processed successfully: " + input;
        };
        
        std::vector<std::string> inputs{"valid_coords", "", "invalid", "error_data", "good_data"};
        
        std::cout << "Processing navigation inputs:\n";
        for (const auto& input : inputs) {
            std::cout << "Input: \"" << input << "\" -> ";
            
            Result result = process_navigation_data(input);
            
            std::visit([](const auto& res) {
                using T = std::decay_t<decltype(res)>;
                
                if constexpr (std::is_same_v<T, std::string>) {
                    std::cout << "SUCCESS: " << res;
                }
                else if constexpr (std::is_same_v<T, std::tuple<int, std::string>>) {
                    const auto& [code, message] = res;
                    std::cout << "ERROR " << code << ": " << message;
                }
                else if constexpr (std::is_same_v<T, std::exception_ptr>) {
                    try {
                        std::rethrow_exception(res);
                    } catch (const std::exception& e) {
                        std::cout << "EXCEPTION: " << e.what();
                    }
                }
            }, result);
            
            std::cout << "\n";
        }
        
        // Count successful operations
        auto success_count = std::count_if(inputs.begin(), inputs.end(),
            [&process_navigation_data](const std::string& input) {
                Result result = process_navigation_data(input);
                return std::holds_alternative<std::string>(result);
            });
        
        std::cout << "\nSuccessful operations: " << success_count << "/" << inputs.size() << "\n";
    }
};

/**
 * @brief std::any Demonstrations
 * 
 * Shows usage of std::any for type-erased storage,
 * runtime type handling, and flexible data containers.
 */
class AnyDemonstrations {
public:
    /**
     * @brief Demonstrate basic std::any usage
     */
    static void demonstrateBasicAny() {
        std::cout << "\n=== std::any Basic Demonstrations ===\n";
        
        // Creating std::any objects
        std::any sensor_data;
        
        // Assigning different types
        sensor_data = 42;
        std::cout << "Stored integer: " << std::any_cast<int>(sensor_data) << "\n";
        
        sensor_data = 3.14159;
        std::cout << "Stored double: " << std::any_cast<double>(sensor_data) << "\n";
        
        sensor_data = std::string("Hull integrity nominal");
        std::cout << "Stored string: " << std::any_cast<std::string>(sensor_data) << "\n";
        
        sensor_data = NavigationCoordinate{125.0, 89.0, 45.0, "Local"};
        std::cout << "Stored coordinate: " << std::any_cast<NavigationCoordinate>(sensor_data) << "\n";
        
        // Check if any has value
        std::cout << "\nAny has value: " << sensor_data.has_value() << "\n";
        
        // Get type information
        std::cout << "Type info: " << sensor_data.type().name() << "\n";
        
        // Reset any
        sensor_data.reset();
        std::cout << "After reset, has value: " << sensor_data.has_value() << "\n";
        
        // Using any_cast with pointers for safe casting
        sensor_data = 123;
        
        if (int* int_ptr = std::any_cast<int>(&sensor_data)) {
            std::cout << "Safe cast to int: " << *int_ptr << "\n";
        }
        
        if (double* double_ptr = std::any_cast<double>(&sensor_data)) {
            std::cout << "This won't print (wrong type)\n";
        } else {
            std::cout << "Safe cast to double failed (as expected)\n";
        }
    }
    
    /**
     * @brief Demonstrate std::any with containers
     */
    static void demonstrateAnyWithContainers() {
        std::cout << "\n=== std::any with Containers ===\n";
        
        // Heterogeneous container using std::any
        std::vector<std::any> mixed_data{
            42,
            3.14159,
            std::string("Enterprise"),
            NavigationCoordinate{100.0, 200.0, 50.0},
            VesselStatus::IN_TRANSIT,
            std::vector<int>{1, 2, 3, 4, 5}
        };
        
        std::cout << "Mixed data container contents:\n";
        for (size_t i = 0; i < mixed_data.size(); ++i) {
            std::cout << "Element " << i << ": ";
            
            const auto& item = mixed_data[i];
            
            // Type identification and safe casting
            if (item.type() == typeid(int)) {
                std::cout << "int = " << std::any_cast<int>(item);
            }
            else if (item.type() == typeid(double)) {
                std::cout << "double = " << std::any_cast<double>(item);
            }
            else if (item.type() == typeid(std::string)) {
                std::cout << "string = \"" << std::any_cast<std::string>(item) << "\"";
            }
            else if (item.type() == typeid(NavigationCoordinate)) {
                std::cout << "coordinate = " << std::any_cast<NavigationCoordinate>(item);
            }
            else if (item.type() == typeid(VesselStatus)) {
                auto status = std::any_cast<VesselStatus>(item);
                std::cout << "status = ";
                switch (status) {
                    case VesselStatus::DOCKED: std::cout << "DOCKED"; break;
                    case VesselStatus::IN_TRANSIT: std::cout << "IN_TRANSIT"; break;
                    case VesselStatus::EXPLORING: std::cout << "EXPLORING"; break;
                    case VesselStatus::COMBAT: std::cout << "COMBAT"; break;
                    case VesselStatus::MAINTENANCE: std::cout << "MAINTENANCE"; break;
                    case VesselStatus::EMERGENCY: std::cout << "EMERGENCY"; break;
                }
            }
            else if (item.type() == typeid(std::vector<int>)) {
                auto vec = std::any_cast<std::vector<int>>(item);
                std::cout << "vector<int> = {";
                for (size_t j = 0; j < vec.size(); ++j) {
                    std::cout << vec[j];
                    if (j < vec.size() - 1) std::cout << ", ";
                }
                std::cout << "}";
            }
            else {
                std::cout << "unknown type: " << item.type().name();
            }
            
            std::cout << "\n";
        }
        
        // Count elements by type
        auto count_type = [&mixed_data](const std::type_info& target_type) {
            return std::count_if(mixed_data.begin(), mixed_data.end(),
                [&target_type](const std::any& item) {
                    return item.type() == target_type;
                });
        };
        
        std::cout << "\nType distribution:\n";
        std::cout << "int: " << count_type(typeid(int)) << "\n";
        std::cout << "double: " << count_type(typeid(double)) << "\n";
        std::cout << "string: " << count_type(typeid(std::string)) << "\n";
        std::cout << "NavigationCoordinate: " << count_type(typeid(NavigationCoordinate)) << "\n";
    }
    
    /**
     * @brief Demonstrate std::any for configuration systems
     */
    static void demonstrateAnyConfiguration() {
        std::cout << "\n=== std::any Configuration System ===\n";
        
        // Configuration map using std::any for flexible value types
        std::map<std::string, std::any> ship_config{
            {"name", std::string("USS Enterprise")},
            {"crew_capacity", 430},
            {"max_warp", 9.0},
            {"coordinates", NavigationCoordinate{0.0, 0.0, 0.0, "Earth Orbit"}},
            {"operational", true},
            {"mission_types", std::vector<std::string>{"Exploration", "Diplomacy", "Defense"}}
        };
        
        // Helper function to safely get config values
        auto get_config = [&ship_config](const std::string& key) -> std::any* {
            auto it = ship_config.find(key);
            return (it != ship_config.end()) ? &it->second : nullptr;
        };
        
        // Safely access different types of configuration values
        std::cout << "Ship configuration:\n";
        
        if (auto* name_any = get_config("name")) {
            if (name_any->type() == typeid(std::string)) {
                std::cout << "Name: " << std::any_cast<std::string>(*name_any) << "\n";
            }
        }
        
        if (auto* crew_any = get_config("crew_capacity")) {
            if (crew_any->type() == typeid(int)) {
                std::cout << "Crew capacity: " << std::any_cast<int>(*crew_any) << "\n";
            }
        }
        
        if (auto* warp_any = get_config("max_warp")) {
            if (warp_any->type() == typeid(double)) {
                std::cout << "Max warp: " << std::any_cast<double>(*warp_any) << "\n";
            }
        }
        
        if (auto* coord_any = get_config("coordinates")) {
            if (coord_any->type() == typeid(NavigationCoordinate)) {
                std::cout << "Position: " << std::any_cast<NavigationCoordinate>(*coord_any) << "\n";
            }
        }
        
        if (auto* missions_any = get_config("mission_types")) {
            if (missions_any->type() == typeid(std::vector<std::string>)) {
                auto missions = std::any_cast<std::vector<std::string>>(*missions_any);
                std::cout << "Mission types: ";
                for (size_t i = 0; i < missions.size(); ++i) {
                    std::cout << missions[i];
                    if (i < missions.size() - 1) std::cout << ", ";
                }
                std::cout << "\n";
            }
        }
        
        // Dynamic configuration updates
        std::cout << "\nUpdating configuration:\n";
        ship_config["crew_capacity"] = 450;  // Update crew
        ship_config["status"] = std::string("Active");  // Add new field
        ship_config["max_warp"] = 9.2;  // Update warp capability
        
        std::cout << "Updated crew capacity: " << std::any_cast<int>(ship_config["crew_capacity"]) << "\n";
        std::cout << "New status: " << std::any_cast<std::string>(ship_config["status"]) << "\n";
        std::cout << "Updated max warp: " << std::any_cast<double>(ship_config["max_warp"]) << "\n";
        
        // Configuration validation
        std::cout << "\nConfiguration validation:\n";
        for (const auto& [key, value] : ship_config) {
            std::cout << key << ": ";
            
            try {
                if (value.type() == typeid(int)) {
                    int val = std::any_cast<int>(value);
                    std::cout << "int(" << val << ") - " << (val > 0 ? "valid" : "invalid");
                }
                else if (value.type() == typeid(double)) {
                    double val = std::any_cast<double>(value);
                    std::cout << "double(" << val << ") - " << (val > 0.0 ? "valid" : "invalid");
                }
                else if (value.type() == typeid(std::string)) {
                    std::string val = std::any_cast<std::string>(value);
                    std::cout << "string(\"" << val << "\") - " << (!val.empty() ? "valid" : "invalid");
                }
                else if (value.type() == typeid(bool)) {
                    bool val = std::any_cast<bool>(value);
                    std::cout << "bool(" << (val ? "true" : "false") << ") - valid";
                }
                else {
                    std::cout << "complex type - assumed valid";
                }
            } catch (const std::bad_any_cast& e) {
                std::cout << "cast error: " << e.what();
            }
            
            std::cout << "\n";
        }
    }
};

/**
 * @brief Comprehensive STL Utilities Demonstrations
 * 
 * Orchestrates all utility demonstrations and provides
 * combined usage examples.
 */
class STLUtilitiesDemo {
public:
    /**
     * @brief Run all STL utility demonstrations
     */
    static void runAllDemonstrations() {
        std::cout << "========== STL UTILITIES COMPREHENSIVE DEMONSTRATION ==========\n";
        
        PairDemonstrations::demonstrateBasicPair();
        PairDemonstrations::demonstratePairWithContainers();
        PairDemonstrations::demonstratePairReturnValues();
        
        TupleDemonstrations::demonstrateBasicTuple();
        TupleDemonstrations::demonstrateTupleOperations();
        TupleDemonstrations::demonstrateTupleWithAlgorithms();
        TupleDemonstrations::demonstrateTupleReturnValues();
        
        OptionalDemonstrations::demonstrateBasicOptional();
        OptionalDemonstrations::demonstrateOptionalWithFunctions();
        OptionalDemonstrations::demonstrateOptionalTransformations();
        
        VariantDemonstrations::demonstrateBasicVariant();
        VariantDemonstrations::demonstrateVariantWithComplexTypes();
        VariantDemonstrations::demonstrateVariantVisitors();
        VariantDemonstrations::demonstrateVariantErrorHandling();
        
        AnyDemonstrations::demonstrateBasicAny();
        AnyDemonstrations::demonstrateAnyWithContainers();
        AnyDemonstrations::demonstrateAnyConfiguration();
        
        demonstrateCombinedUsage();
        
        std::cout << "\n========== STL UTILITIES DEMONSTRATION COMPLETE ==========\n";
    }
    
private:
    /**
     * @brief Demonstrate combined usage of multiple utilities
     */
    static void demonstrateCombinedUsage();
};

} // namespace CppVerseHub::STL