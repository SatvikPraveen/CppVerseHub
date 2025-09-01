// File: src/modern/RangesDemo.hpp
// C++20 Ranges Pipelines and Algorithms Demonstration

#pragma once

#include <ranges>
#include <algorithm>
#include <vector>
#include <string>
#include <iostream>
#include <functional>
#include <numeric>
#include <random>
#include <map>
#include <set>

namespace CppVerseHub::Modern::Ranges {

namespace rng = std::ranges;
namespace views = std::views;

// ===== SPACE GAME DATA STRUCTURES =====

struct Planet {
    int id;
    std::string name;
    double distance_from_sun;
    int population;
    bool habitable;
    std::vector<std::string> resources;
    
    Planet(int i, std::string n, double d, int p, bool h, std::vector<std::string> r = {})
        : id(i), name(std::move(n)), distance_from_sun(d), population(p), habitable(h), resources(std::move(r)) {}
        
    bool operator<(const Planet& other) const {
        return distance_from_sun < other.distance_from_sun;
    }
    
    friend std::ostream& operator<<(std::ostream& os, const Planet& p) {
        return os << "Planet{id=" << p.id << ", name=" << p.name 
                  << ", distance=" << p.distance_from_sun << ", pop=" << p.population << "}";
    }
};

struct Fleet {
    int fleet_id;
    std::string commander;
    int ship_count;
    double fuel_level;
    std::string mission_type;
    bool is_active;
    
    Fleet(int id, std::string cmd, int ships, double fuel, std::string mission, bool active = true)
        : fleet_id(id), commander(std::move(cmd)), ship_count(ships), 
          fuel_level(fuel), mission_type(std::move(mission)), is_active(active) {}
          
    friend std::ostream& operator<<(std::ostream& os, const Fleet& f) {
        return os << "Fleet{id=" << f.fleet_id << ", commander=" << f.commander 
                  << ", ships=" << f.ship_count << ", fuel=" << f.fuel_level << "}";
    }
};

struct Mission {
    int mission_id;
    std::string type;
    int priority;
    double completion_percentage;
    std::vector<int> assigned_fleets;
    
    Mission(int id, std::string t, int p, double comp = 0.0)
        : mission_id(id), type(std::move(t)), priority(p), completion_percentage(comp) {}
        
    friend std::ostream& operator<<(std::ostream& os, const Mission& m) {
        return os << "Mission{id=" << m.mission_id << ", type=" << m.type 
                  << ", priority=" << m.priority << ", completion=" << m.completion_percentage << "%}";
    }
};

// ===== SAMPLE DATA GENERATORS =====

std::vector<Planet> generate_planets() {
    return {
        {1, "Earth", 1.0, 8000000000, true, {"Water", "Oxygen", "Iron"}},
        {2, "Mars", 1.5, 0, false, {"Iron", "Silicon", "Ice"}},
        {3, "Venus", 0.7, 0, false, {"Carbon", "Sulfur"}},
        {4, "Jupiter", 5.2, 0, false, {"Hydrogen", "Helium"}},
        {5, "Kepler-442b", 1200.0, 50000000, true, {"Water", "Rare_Metals"}},
        {6, "Proxima-Centauri-b", 4.24, 0, true, {"Unknown"}},
        {7, "Titan", 9.5, 0, false, {"Methane", "Nitrogen", "Water_Ice"}},
        {8, "Europa", 5.2, 0, false, {"Water_Ice", "Oxygen"}},
        {9, "Gliese-667Cc", 22.0, 1000000, true, {"Water", "Minerals"}},
        {10, "TRAPPIST-1e", 40.0, 200000, true, {"Water", "Atmosphere"}}
    };
}

std::vector<Fleet> generate_fleets() {
    return {
        {101, "Admiral Zhang", 25, 85.5, "Exploration"},
        {102, "Commander Rodriguez", 12, 92.0, "Combat"},
        {103, "Captain Singh", 8, 45.2, "Colonization"},
        {104, "Admiral Thompson", 30, 76.8, "Trade"},
        {105, "Commander Chen", 15, 20.1, "Rescue"},
        {106, "Captain Johnson", 18, 88.9, "Exploration"},
        {107, "Admiral Kim", 22, 95.5, "Combat"},
        {108, "Commander Wilson", 6, 35.7, "Research"},
        {109, "Captain Davis", 14, 67.3, "Patrol"},
        {110, "Admiral Brown", 35, 55.4, "Colonization"}
    };
}

std::vector<Mission> generate_missions() {
    return {
        {201, "Exploration", 1, 75.5},
        {202, "Combat", 5, 100.0},
        {203, "Colonization", 2, 45.0},
        {204, "Trade", 3, 90.0},
        {205, "Rescue", 5, 10.0},
        {206, "Research", 1, 85.0},
        {207, "Patrol", 4, 60.0},
        {208, "Diplomacy", 2, 25.0},
        {209, "Mining", 3, 95.0},
        {210, "Defense", 5, 40.0}
    };
}

// ===== BASIC RANGES OPERATIONS =====

void demonstrate_basic_ranges() {
    std::cout << "\n=== Basic Ranges Operations ===" << std::endl;
    
    std::vector<int> numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    
    // Simple filter and transform pipeline
    auto result = numbers 
        | views::filter([](int n) { return n % 2 == 0; })
        | views::transform([](int n) { return n * n; })
        | views::take(3);
    
    std::cout << "Even numbers squared (first 3): ";
    for (auto n : result) {
        std::cout << n << " ";
    }
    std::cout << std::endl;
    
    // Reverse and drop
    auto reversed_dropped = numbers 
        | views::reverse 
        | views::drop(3) 
        | views::take(4);
    
    std::cout << "Reversed, drop 3, take 4: ";
    rng::copy(reversed_dropped, std::ostream_iterator<int>(std::cout, " "));
    std::cout << std::endl;
}

// ===== PLANET ANALYSIS WITH RANGES =====

void demonstrate_planet_analysis() {
    std::cout << "\n=== Planet Analysis with Ranges ===" << std::endl;
    
    auto planets = generate_planets();
    
    // Find habitable planets within reasonable distance
    std::cout << "Habitable planets within 50 light-years:" << std::endl;
    auto close_habitable = planets 
        | views::filter([](const Planet& p) { return p.habitable && p.distance_from_sun < 50.0; })
        | views::transform([](const Planet& p) { return p.name; });
    
    for (const auto& name : close_habitable) {
        std::cout << "  - " << name << std::endl;
    }
    
    // Group planets by population ranges
    std::cout << "\nPlanets with populations over 1 million:" << std::endl;
    auto populated_planets = planets 
        | views::filter([](const Planet& p) { return p.population > 1000000; })
        | views::transform([](const Planet& p) { 
            return std::make_pair(p.name, p.population); 
        });
    
    for (const auto& [name, population] : populated_planets) {
        std::cout << "  - " << name << ": " << population << " inhabitants" << std::endl;
    }
    
    // Calculate average distance of habitable planets
    auto habitable_distances = planets 
        | views::filter([](const Planet& p) { return p.habitable; })
        | views::transform([](const Planet& p) { return p.distance_from_sun; });
    
    auto avg_distance = std::accumulate(habitable_distances.begin(), habitable_distances.end(), 0.0) 
                       / std::distance(habitable_distances.begin(), habitable_distances.end());
    
    std::cout << "\nAverage distance of habitable planets: " << avg_distance << " AU" << std::endl;
}

// ===== FLEET MANAGEMENT WITH RANGES =====

void demonstrate_fleet_management() {
    std::cout << "\n=== Fleet Management with Ranges ===" << std::endl;
    
    auto fleets = generate_fleets();
    
    // Find fleets needing refueling (fuel < 50%)
    std::cout << "Fleets needing refueling (fuel < 50%):" << std::endl;
    auto low_fuel_fleets = fleets 
        | views::filter([](const Fleet& f) { return f.fuel_level < 50.0; })
        | views::transform([](const Fleet& f) { 
            return std::format("Fleet {} ({}): {:.1f}%", f.fleet_id, f.commander, f.fuel_level); 
        });
    
    for (const auto& info : low_fuel_fleets) {
        std::cout << "  - " << info << std::endl;
    }
    
    // Group fleets by mission type and count
    std::map<std::string, int> mission_counts;
    auto mission_types = fleets | views::transform([](const Fleet& f) { return f.mission_type; });
    
    for (const auto& mission_type : mission_types) {
        mission_counts[mission_type]++;
    }
    
    std::cout << "\nFleets by mission type:" << std::endl;
    for (const auto& [mission, count] : mission_counts) {
        std::cout << "  - " << mission << ": " << count << " fleets" << std::endl;
    }
    
    // Find the most powerful fleets (ship count > 20)
    std::cout << "\nMost powerful fleets (20+ ships):" << std::endl;
    auto powerful_fleets = fleets 
        | views::filter([](const Fleet& f) { return f.ship_count >= 20; })
        | views::transform([](const Fleet& f) { 
            return std::make_tuple(f.commander, f.ship_count, f.mission_type); 
        });
    
    for (const auto& [commander, ships, mission] : powerful_fleets) {
        std::cout << "  - " << commander << ": " << ships << " ships on " << mission << " mission" << std::endl;
    }
}

// ===== MISSION ANALYSIS WITH RANGES =====

void demonstrate_mission_analysis() {
    std::cout << "\n=== Mission Analysis with Ranges ===" << std::endl;
    
    auto missions = generate_missions();
    
    // Sort missions by priority and completion
    rng::sort(missions, [](const Mission& a, const Mission& b) {
        if (a.priority != b.priority) return a.priority > b.priority;  // Higher priority first
        return a.completion_percentage < b.completion_percentage;      // Less complete first
    });
    
    std::cout << "Missions sorted by priority and completion:" << std::endl;
    for (const auto& mission : missions | views::take(5)) {
        std::cout << "  - " << mission << std::endl;
    }
    
    // Find urgent incomplete missions
    std::cout << "\nUrgent incomplete missions (priority >= 4, completion < 50%):" << std::endl;
    auto urgent_missions = missions 
        | views::filter([](const Mission& m) { 
            return m.priority >= 4 && m.completion_percentage < 50.0; 
        });
    
    for (const auto& mission : urgent_missions) {
        std::cout << "  - " << mission << std::endl;
    }
    
    // Calculate completion statistics
    auto completion_stats = missions 
        | views::transform([](const Mission& m) { return m.completion_percentage; });
    
    auto total_completion = std::accumulate(completion_stats.begin(), completion_stats.end(), 0.0);
    auto avg_completion = total_completion / std::distance(completion_stats.begin(), completion_stats.end());
    
    std::cout << "\nAverage mission completion: " << avg_completion << "%" << std::endl;
}

// ===== ADVANCED RANGES PATTERNS =====

void demonstrate_advanced_patterns() {
    std::cout << "\n=== Advanced Ranges Patterns ===" << std::endl;
    
    // Generate numbers and create complex pipeline
    auto numbers = views::iota(1, 100);  // 1 to 99
    
    // Complex mathematical operations
    auto complex_pipeline = numbers
        | views::filter([](int n) { return n % 3 == 0 || n % 5 == 0; })  // Multiples of 3 or 5
        | views::transform([](int n) { return n * n; })                   // Square them
        | views::filter([](int n) { return n < 1000; })                   // Keep under 1000
        | views::reverse                                                   // Reverse order
        | views::take(10);                                                // Take first 10
    
    std::cout << "Complex pipeline result: ";
    for (auto n : complex_pipeline) {
        std::cout << n << " ";
    }
    std::cout << std::endl;
    
    // String processing pipeline
    std::vector<std::string> words = {
        "space", "exploration", "mission", "fleet", "planet", 
        "galaxy", "universe", "star", "nebula", "asteroid"
    };
    
    auto string_pipeline = words
        | views::filter([](const std::string& s) { return s.length() > 5; })
        | views::transform([](const std::string& s) { 
            std::string upper = s;
            rng::transform(upper, upper.begin(), ::toupper);
            return upper;
        })
        | views::take(5);
    
    std::cout << "Processed strings (length > 5, uppercase): ";
    for (const auto& s : string_pipeline) {
        std::cout << s << " ";
    }
    std::cout << std::endl;
}

// ===== CUSTOM RANGE ADAPTERS =====

template<typename Range, typename Predicate>
class every_nth_view : public std::ranges::view_interface<every_nth_view<Range, Predicate>> {
private:
    Range range_;
    std::size_t n_;
    
public:
    every_nth_view(Range range, std::size_t n) : range_(std::move(range)), n_(n) {}
    
    auto begin() const {
        auto it = std::ranges::begin(range_);
        return it;
    }
    
    auto end() const {
        return std::ranges::end(range_);
    }
};

void demonstrate_custom_views() {
    std::cout << "\n=== Custom Range Views ===" << std::endl;
    
    std::vector<int> data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    
    // Every 3rd element
    auto every_third = data | views::stride(3);
    
    std::cout << "Every 3rd element: ";
    for (auto n : every_third) {
        std::cout << n << " ";
    }
    std::cout << std::endl;
    
    // Chunk into groups
    auto chunks = data | views::chunk(4);
    
    std::cout << "Chunked into groups of 4:" << std::endl;
    for (auto chunk : chunks) {
        std::cout << "  Chunk: ";
        for (auto n : chunk) {
            std::cout << n << " ";
        }
        std::cout << std::endl;
    }
}

// ===== PERFORMANCE OPTIMIZED RANGES =====

void demonstrate_performance_patterns() {
    std::cout << "\n=== Performance Optimized Ranges ===" << std::endl;
    
    // Generate large dataset
    std::vector<int> large_data;
    large_data.reserve(10000);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 1000);
    
    std::generate_n(std::back_inserter(large_data), 10000, [&] { return dis(gen); });
    
    // Efficient filtering and processing
    auto processed = large_data
        | views::filter([](int n) { return n > 500; })     // Filter once
        | views::transform([](int n) { return n * 2; })    // Transform once
        | views::take(100);                                 // Limit early
    
    std::cout << "Processed " << std::distance(processed.begin(), processed.end()) 
              << " elements from large dataset" << std::endl;
    
    // Demonstrate lazy evaluation
    std::cout << "First 5 processed values: ";
    for (auto it = processed.begin(); it != processed.end() && std::distance(processed.begin(), it) < 5; ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;
}

// ===== MAIN DEMONSTRATION FUNCTION =====

void demonstrate_all_ranges() {
    std::cout << "\n🚀 C++20 Ranges Demonstration for Space Game 🚀" << std::endl;
    std::cout << "=================================================" << std::endl;
    
    demonstrate_basic_ranges();
    demonstrate_planet_analysis();
    demonstrate_fleet_management();
    demonstrate_mission_analysis();
    demonstrate_advanced_patterns();
    demonstrate_custom_views();
    demonstrate_performance_patterns();
    
    std::cout << "\n✨ Ranges demonstration complete! ✨" << std::endl;
}

} // namespace CppVerseHub::Modern::Ranges