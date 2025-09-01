// File: src/modern/StructuredBindings.hpp
// C++17 Structured Bindings (Decomposition Declarations) Demonstration

#pragma once

#include <tuple>
#include <string>
#include <vector>
#include <map>
#include <array>
#include <utility>
#include <iostream>
#include <algorithm>
#include <numeric>

namespace CppVerseHub::Modern::StructuredBindings {

// ===== SPACE GAME DATA STRUCTURES =====

struct SpaceCoordinate {
    double x, y, z;
    
    SpaceCoordinate(double x_val = 0.0, double y_val = 0.0, double z_val = 0.0)
        : x(x_val), y(y_val), z(z_val) {}
};

struct PlanetInfo {
    int id;
    std::string name;
    double mass;
    bool habitable;
    std::vector<std::string> resources;
    
    PlanetInfo(int planet_id, std::string planet_name, double planet_mass, 
               bool is_habitable, std::vector<std::string> planet_resources = {})
        : id(planet_id), name(std::move(planet_name)), mass(planet_mass), 
          habitable(is_habitable), resources(std::move(planet_resources)) {}
};

struct FleetStats {
    std::string commander;
    int ship_count;
    double fuel_percentage;
    std::string mission_type;
    
    FleetStats(std::string cmd, int ships, double fuel, std::string mission)
        : commander(std::move(cmd)), ship_count(ships), 
          fuel_percentage(fuel), mission_type(std::move(mission)) {}
};

struct MissionReport {
    int mission_id;
    std::string type;
    double completion;
    int priority;
    std::vector<int> assigned_fleets;
    
    MissionReport(int id, std::string mission_type, double comp, int prio, 
                  std::vector<int> fleets = {})
        : mission_id(id), type(std::move(mission_type)), completion(comp), 
          priority(prio), assigned_fleets(std::move(fleets)) {}
};

// ===== TUPLE BASED FUNCTIONS =====

std::tuple<double, double, double> calculateOrbitParameters(double mass, double distance) {
    const double G = 6.67430e-11; // Gravitational constant (simplified)
    double velocity = std::sqrt(G * mass / distance);
    double period = 2 * 3.14159 * distance / velocity;
    double energy = -G * mass / (2 * distance);
    
    return {velocity, period, energy};
}

std::tuple<int, std::string, double> findBestFleet(const std::vector<FleetStats>& fleets) {
    if (fleets.empty()) {
        return {-1, "None", 0.0};
    }
    
    auto best_fleet = std::max_element(fleets.begin(), fleets.end(),
        [](const FleetStats& a, const FleetStats& b) {
            return (a.ship_count * a.fuel_percentage) < (b.ship_count * b.fuel_percentage);
        });
    
    int index = static_cast<int>(std::distance(fleets.begin(), best_fleet));
    return {index, best_fleet->commander, best_fleet->fuel_percentage};
}

std::tuple<std::string, int, std::vector<std::string>> 
analyzePlanet(const PlanetInfo& planet) {
    std::string status = planet.habitable ? "Habitable" : "Uninhabitable";
    int resource_count = static_cast<int>(planet.resources.size());
    
    std::vector<std::string> rare_resources;
    for (const auto& resource : planet.resources) {
        if (resource.find("Rare") != std::string::npos || 
            resource.find("Exotic") != std::string::npos) {
            rare_resources.push_back(resource);
        }
    }
    
    return {status, resource_count, rare_resources};
}

// ===== PAIR BASED FUNCTIONS =====

std::pair<double, std::string> calculateJumpDistance(const SpaceCoordinate& from, 
                                                     const SpaceCoordinate& to) {
    double dx = to.x - from.x;
    double dy = to.y - from.y;
    double dz = to.z - from.z;
    double distance = std::sqrt(dx*dx + dy*dy + dz*dz);
    
    std::string classification;
    if (distance < 1.0) classification = "Local";
    else if (distance < 10.0) classification = "System";
    else if (distance < 100.0) classification = "Sector";
    else classification = "Long Range";
    
    return {distance, classification};
}

std::pair<int, double> getMissionStats(const std::vector<MissionReport>& missions) {
    if (missions.empty()) {
        return {0, 0.0};
    }
    
    int total_missions = static_cast<int>(missions.size());
    double avg_completion = std::accumulate(missions.begin(), missions.end(), 0.0,
        [](double sum, const MissionReport& mission) {
            return sum + mission.completion;
        }) / total_missions;
    
    return {total_missions, avg_completion};
}

// ===== ARRAY BASED FUNCTIONS =====

std::array<double, 3> calculateCenterOfMass(const std::vector<PlanetInfo>& planets) {
    if (planets.empty()) {
        return {0.0, 0.0, 0.0};
    }
    
    double total_mass = 0.0;
    std::array<double, 3> center = {0.0, 0.0, 0.0};
    
    // For simplicity, assume planets are at their ID positions
    for (const auto& planet : planets) {
        total_mass += planet.mass;
        center[0] += planet.mass * planet.id;  // x-coordinate
        center[1] += planet.mass * (planet.id * 0.5);  // y-coordinate
        center[2] += planet.mass * (planet.id * 0.25); // z-coordinate
    }
    
    if (total_mass > 0) {
        center[0] /= total_mass;
        center[1] /= total_mass;
        center[2] /= total_mass;
    }
    
    return center;
}

// ===== STRUCTURED BINDINGS DEMONSTRATIONS =====

void demonstrate_tuple_bindings() {
    std::cout << "\n=== Tuple Structured Bindings ===" << std::endl;
    
    // Basic tuple decomposition
    auto planet_orbit = calculateOrbitParameters(1.989e30, 1.496e11); // Sun-Earth system
    auto [orbital_velocity, orbital_period, binding_energy] = planet_orbit;
    
    std::cout << "Orbital Parameters:" << std::endl;
    std::cout << "  Velocity: " << orbital_velocity << " m/s" << std::endl;
    std::cout << "  Period: " << orbital_period << " seconds" << std::endl;
    std::cout << "  Binding Energy: " << binding_energy << " J" << std::endl;
    
    // Fleet analysis with tuple binding
    std::vector<FleetStats> fleets = {
        {"Admiral Zhang", 25, 85.5, "Exploration"},
        {"Commander Rodriguez", 12, 92.0, "Combat"},
        {"Captain Singh", 8, 45.2, "Colonization"},
        {"Admiral Thompson", 30, 76.8, "Trade"}
    };
    
    auto [best_index, best_commander, best_fuel] = findBestFleet(fleets);
    std::cout << "\nBest Fleet Analysis:" << std::endl;
    std::cout << "  Index: " << best_index << std::endl;
    std::cout << "  Commander: " << best_commander << std::endl;
    std::cout << "  Fuel Level: " << best_fuel << "%" << std::endl;
    
    // Planet analysis
    PlanetInfo kepler442b(5, "Kepler-442b", 4.34e24, true, 
                         {"Water", "Oxygen", "Rare_Metals", "Exotic_Matter"});
    
    auto [habitability_status, resource_count, rare_resources] = analyzePlanet(kepler442b);
    std::cout << "\nPlanet Analysis for " << kepler442b.name << ":" << std::endl;
    std::cout << "  Status: " << habitability_status << std::endl;
    std::cout << "  Resources: " << resource_count << " types" << std::endl;
    std::cout << "  Rare Resources: ";
    for (const auto& resource : rare_resources) {
        std::cout << resource << " ";
    }
    std::cout << std::endl;
}

void demonstrate_pair_bindings() {
    std::cout << "\n=== Pair Structured Bindings ===" << std::endl;
    
    // Distance calculations
    SpaceCoordinate earth(0.0, 0.0, 0.0);
    SpaceCoordinate mars(5.2, 2.8, 1.1);
    SpaceCoordinate proxima_centauri(42000.0, 15000.0, 8500.0);
    
    auto [distance_to_mars, mars_classification] = calculateJumpDistance(earth, mars);
    std::cout << "Jump to Mars:" << std::endl;
    std::cout << "  Distance: " << distance_to_mars << " AU" << std::endl;
    std::cout << "  Classification: " << mars_classification << std::endl;
    
    auto [distance_to_proxima, proxima_classification] = calculateJumpDistance(earth, proxima_centauri);
    std::cout << "Jump to Proxima Centauri:" << std::endl;
    std::cout << "  Distance: " << distance_to_proxima << " AU" << std::endl;
    std::cout << "  Classification: " << proxima_classification << std::endl;
    
    // Mission statistics
    std::vector<MissionReport> missions = {
        {101, "Exploration", 75.5, 2},
        {102, "Combat", 100.0, 5},
        {103, "Colonization", 45.0, 1},
        {104, "Trade", 90.0, 3},
        {105, "Rescue", 10.0, 5}
    };
    
    auto [total_missions, avg_completion] = getMissionStats(missions);
    std::cout << "\nMission Statistics:" << std::endl;
    std::cout << "  Total Missions: " << total_missions << std::endl;
    std::cout << "  Average Completion: " << avg_completion << "%" << std::endl;
}

void demonstrate_array_bindings() {
    std::cout << "\n=== Array Structured Bindings ===" << std::endl;
    
    // Coordinate array binding
    std::array<double, 3> ship_position = {12.5, 8.3, -4.7};
    auto [ship_x, ship_y, ship_z] = ship_position;
    
    std::cout << "Ship Position:" << std::endl;
    std::cout << "  X: " << ship_x << " AU" << std::endl;
    std::cout << "  Y: " << ship_y << " AU" << std::endl;
    std::cout << "  Z: " << ship_z << " AU" << std::endl;
    
    // Center of mass calculation
    std::vector<PlanetInfo> solar_system = {
        {1, "Mercury", 3.301e23, false, {"Iron", "Silicon"}},
        {2, "Venus", 4.867e24, false, {"Carbon", "Sulfur"}},
        {3, "Earth", 5.972e24, true, {"Water", "Oxygen", "Iron"}},
        {4, "Mars", 6.417e23, false, {"Iron", "Silicon", "Ice"}}
    };
    
    auto [center_x, center_y, center_z] = calculateCenterOfMass(solar_system);
    std::cout << "\nCenter of Mass (Solar System):" << std::endl;
    std::cout << "  X: " << center_x << std::endl;
    std::cout << "  Y: " << center_y << std::endl;
    std::cout << "  Z: " << center_z << std::endl;
    
    // RGB color array (space theme)
    std::array<int, 3> nebula_color = {138, 43, 226}; // Blue-violet
    auto [red, green, blue] = nebula_color;
    
    std::cout << "\nNebula Color (RGB):" << std::endl;
    std::cout << "  Red: " << red << std::endl;
    std::cout << "  Green: " << green << std::endl;
    std::cout << "  Blue: " << blue << std::endl;
}

void demonstrate_map_bindings() {
    std::cout << "\n=== Map/Container Structured Bindings ===" << std::endl;
    
    // Resource inventory
    std::map<std::string, int> resource_inventory = {
        {"Water", 1500},
        {"Oxygen", 800},
        {"Iron", 2200},
        {"Rare_Metals", 45},
        {"Exotic_Matter", 3}
    };
    
    std::cout << "Resource Inventory:" << std::endl;
    for (const auto& [resource_name, quantity] : resource_inventory) {
        std::cout << "  " << resource_name << ": " << quantity << " units" << std::endl;
    }
    
    // Find most abundant resource
    auto most_abundant = std::max_element(resource_inventory.begin(), resource_inventory.end(),
        [](const auto& a, const auto& b) {
            return a.second < b.second;
        });
    
    auto [abundant_resource, abundant_quantity] = *most_abundant;
    std::cout << "\nMost Abundant Resource: " << abundant_resource 
              << " (" << abundant_quantity << " units)" << std::endl;
    
    // Fleet commanders and their ratings
    std::map<std::string, double> commander_ratings = {
        {"Admiral Zhang", 9.2},
        {"Commander Rodriguez", 8.7},
        {"Captain Singh", 7.8},
        {"Admiral Thompson", 9.5},
        {"Commander Chen", 8.1}
    };
    
    std::cout << "\nCommander Ratings:" << std::endl;
    for (const auto& [commander, rating] : commander_ratings) {
        std::cout << "  " << commander << ": " << rating << "/10" << std::endl;
    }
}

void demonstrate_struct_bindings() {
    std::cout << "\n=== Struct Structured Bindings ===" << std::endl;
    
    // Note: Structured bindings work with structs/classes with public members
    struct SimpleFleet {
        std::string name;
        int ships;
        double fuel;
    };
    
    SimpleFleet alpha_fleet{"Alpha Squadron", 12, 87.5};
    auto [fleet_name, ship_count, fuel_level] = alpha_fleet;
    
    std::cout << "Fleet Information:" << std::endl;
    std::cout << "  Name: " << fleet_name << std::endl;
    std::cout << "  Ships: " << ship_count << std::endl;
    std::cout << "  Fuel: " << fuel_level << "%" << std::endl;
    
    // Multiple struct decomposition
    struct PlanetarySystem {
        std::string star_name;
        int planet_count;
        bool has_habitable_zone;
        double distance_from_earth;
    };
    
    std::vector<PlanetarySystem> star_systems = {
        {"Alpha Centauri", 3, true, 4.37},
        {"Wolf 359", 2, false, 7.86},
        {"Barnard's Star", 1, false, 5.96},
        {"TRAPPIST-1", 7, true, 40.7}
    };
    
    std::cout << "\nPlanetary Systems:" << std::endl;
    for (const auto& [star, planets, habitable, distance] : star_systems) {
        std::cout << "  " << star << ": " << planets << " planets, " 
                  << distance << " ly away";
        if (habitable) {
            std::cout << " (has habitable zone)";
        }
        std::cout << std::endl;
    }
}

void demonstrate_function_return_bindings() {
    std::cout << "\n=== Function Return Structured Bindings ===" << std::endl;
    
    // Lambda returning multiple values
    auto analyze_fleet_composition = [](const std::vector<FleetStats>& fleets) {
        int exploration_count = 0;
        int combat_count = 0;
        int other_count = 0;
        double total_fuel = 0.0;
        
        for (const auto& fleet : fleets) {
            total_fuel += fleet.fuel_percentage;
            
            if (fleet.mission_type == "Exploration") {
                exploration_count++;
            } else if (fleet.mission_type == "Combat") {
                combat_count++;
            } else {
                other_count++;
            }
        }
        
        double avg_fuel = fleets.empty() ? 0.0 : total_fuel / fleets.size();
        
        return std::make_tuple(exploration_count, combat_count, other_count, avg_fuel);
    };
    
    std::vector<FleetStats> empire_fleets = {
        {"Admiral Zhang", 25, 85.5, "Exploration"},
        {"Commander Rodriguez", 12, 92.0, "Combat"},
        {"Captain Singh", 8, 45.2, "Colonization"},
        {"Admiral Thompson", 30, 76.8, "Trade"},
        {"Commander Chen", 15, 20.1, "Exploration"},
        {"Captain Johnson", 18, 88.9, "Combat"}
    };
    
    auto [exploration_fleets, combat_fleets, other_fleets, average_fuel] = 
        analyze_fleet_composition(empire_fleets);
    
    std::cout << "Fleet Composition Analysis:" << std::endl;
    std::cout << "  Exploration Fleets: " << exploration_fleets << std::endl;
    std::cout << "  Combat Fleets: " << combat_fleets << std::endl;
    std::cout << "  Other Mission Fleets: " << other_fleets << std::endl;
    std::cout << "  Average Fuel Level: " << average_fuel << "%" << std::endl;
}

void demonstrate_nested_bindings() {
    std::cout << "\n=== Nested Structured Bindings ===" << std::endl;
    
    // Map of planets with their coordinate tuples
    std::map<std::string, std::tuple<double, double, double, bool>> planetary_data = {
        {"Earth", {0.0, 0.0, 0.0, true}},
        {"Mars", {1.52, 0.0, 0.0, false}},
        {"Jupiter", {5.20, 0.0, 0.0, false}},
        {"Kepler-452b", {1400.0, 500.0, 200.0, true}}
    };
    
    std::cout << "Planetary Data Analysis:" << std::endl;
    for (const auto& [planet_name, data] : planetary_data) {
        auto [x, y, z, habitable] = data;
        
        double distance_from_origin = std::sqrt(x*x + y*y + z*z);
        
        std::cout << "  " << planet_name << ":" << std::endl;
        std::cout << "    Position: (" << x << ", " << y << ", " << z << ")" << std::endl;
        std::cout << "    Distance from origin: " << distance_from_origin << " AU" << std::endl;
        std::cout << "    Habitable: " << (habitable ? "Yes" : "No") << std::endl;
    }
}

// ===== MAIN DEMONSTRATION FUNCTION =====

void demonstrate_all_structured_bindings() {
    std::cout << "\n🔗 C++17 Structured Bindings Demonstration 🔗" << std::endl;
    std::cout << "=============================================" << std::endl;
    
    demonstrate_tuple_bindings();
    demonstrate_pair_bindings();
    demonstrate_array_bindings();
    demonstrate_map_bindings();
    demonstrate_struct_bindings();
    demonstrate_function_return_bindings();
    demonstrate_nested_bindings();
    
    std::cout << "\n✨ Structured bindings demonstration complete! ✨" << std::endl;
    std::cout << "\nKey Benefits:" << std::endl;
    std::cout << "• Cleaner, more readable code" << std::endl;
    std::cout << "• Automatic type deduction" << std::endl;
    std::cout << "• Works with tuples, pairs, arrays, and structs" << std::endl;
    std::cout << "• Eliminates need for std::tie or std::get" << std::endl;
    std::cout << "• Improves maintainability" << std::endl;
}

} // namespace CppVerseHub::Modern::StructuredBindings