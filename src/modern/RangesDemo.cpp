// File: src/modern/RangesDemo.cpp
// C++20 Ranges Pipelines and Algorithms Implementation

#include "RangesDemo.hpp"
#include <format>
#include <chrono>

namespace CppVerseHub::Modern::Ranges {

// ===== SPACE GAME SPECIFIC RANGE UTILITIES =====

class SpaceGameRangeUtilities {
public:
    // Find optimal planets for colonization
    static auto find_colonization_targets(const std::vector<Planet>& planets) {
        return planets
            | views::filter([](const Planet& p) { 
                return p.habitable && p.distance_from_sun < 100.0 && p.population < 1000000; 
            })
            | views::transform([](const Planet& p) { 
                return std::make_tuple(p.name, p.distance_from_sun, p.resources.size()); 
            });
    }
    
    // Calculate fleet efficiency scores
    static auto calculate_fleet_efficiency(const std::vector<Fleet>& fleets) {
        return fleets
            | views::filter([](const Fleet& f) { return f.is_active; })
            | views::transform([](const Fleet& f) {
                double efficiency = (f.fuel_level / 100.0) * (f.ship_count / 10.0);
                return std::make_pair(f.fleet_id, efficiency);
            });
    }
    
    // Group missions by priority levels
    static auto group_missions_by_priority(const std::vector<Mission>& missions) {
        std::map<int, std::vector<Mission>> grouped;
        
        for (const auto& mission : missions) {
            grouped[mission.priority].push_back(mission);
        }
        
        return grouped;
    }
    
    // Find resource-rich planets
    static auto find_resource_rich_planets(const std::vector<Planet>& planets, int min_resources = 2) {
        return planets
            | views::filter([min_resources](const Planet& p) { 
                return p.resources.size() >= static_cast<size_t>(min_resources); 
            })
            | views::transform([](const Planet& p) {
                std::string resource_list;
                for (size_t i = 0; i < p.resources.size(); ++i) {
                    if (i > 0) resource_list += ", ";
                    resource_list += p.resources[i];
                }
                return std::make_tuple(p.name, p.resources.size(), resource_list);
            });
    }
};

// ===== ADVANCED FILTERING AND SEARCHING =====

void demonstrate_advanced_filtering() {
    std::cout << "\n=== Advanced Filtering and Searching ===" << std::endl;
    
    auto planets = generate_planets();
    auto fleets = generate_fleets();
    auto missions = generate_missions();
    
    // Find colonization targets
    std::cout << "Optimal colonization targets:" << std::endl;
    auto colonization_targets = SpaceGameRangeUtilities::find_colonization_targets(planets);
    
    for (const auto& [name, distance, resource_count] : colonization_targets) {
        std::cout << std::format("  - {}: {:.1f} AU, {} resources", name, distance, resource_count) << std::endl;
    }
    
    // Calculate fleet efficiencies
    std::cout << "\nFleet efficiency scores:" << std::endl;
    auto fleet_efficiencies = SpaceGameRangeUtilities::calculate_fleet_efficiency(fleets);
    
    std::vector<std::pair<int, double>> efficiency_vec(fleet_efficiencies.begin(), fleet_efficiencies.end());
    rng::sort(efficiency_vec, [](const auto& a, const auto& b) { return a.second > b.second; });
    
    for (const auto& [fleet_id, efficiency] : efficiency_vec | views::take(5)) {
        std::cout << std::format("  - Fleet {}: {:.2f} efficiency", fleet_id, efficiency) << std::endl;
    }
    
    // Find resource-rich planets
    std::cout << "\nResource-rich planets (3+ resources):" << std::endl;
    auto resource_rich = SpaceGameRangeUtilities::find_resource_rich_planets(planets, 3);
    
    for (const auto& [name, count, resources] : resource_rich) {
        std::cout << std::format("  - {}: {} resources ({})", name, count, resources) << std::endl;
    }
}

// ===== PERFORMANCE BENCHMARKING =====

void benchmark_ranges_vs_traditional() {
    std::cout << "\n=== Performance Benchmark: Ranges vs Traditional ===" << std::endl;
    
    // Generate large dataset
    const size_t data_size = 100000;
    std::vector<int> large_data;
    large_data.reserve(data_size);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 10000);
    
    std::generate_n(std::back_inserter(large_data), data_size, [&] { return dis(gen); });
    
    // Traditional approach
    auto start_traditional = std::chrono::high_resolution_clock::now();
    
    std::vector<int> filtered_traditional;
    for (int n : large_data) {
        if (n > 5000 && n % 2 == 0) {
            filtered_traditional.push_back(n * 2);
        }
    }
    
    auto end_traditional = std::chrono::high_resolution_clock::now();
    auto duration_traditional = std::chrono::duration_cast<std::chrono::microseconds>(end_traditional - start_traditional);
    
    // Ranges approach
    auto start_ranges = std::chrono::high_resolution_clock::now();
    
    auto ranges_result = large_data
        | views::filter([](int n) { return n > 5000 && n % 2 == 0; })
        | views::transform([](int n) { return n * 2; });
    
    std::vector<int> filtered_ranges(ranges_result.begin(), ranges_result.end());
    
    auto end_ranges = std::chrono::high_resolution_clock::now();
    auto duration_ranges = std::chrono::duration_cast<std::chrono::microseconds>(end_ranges - start_ranges);
    
    std::cout << std::format("Dataset size: {}", data_size) << std::endl;
    std::cout << std::format("Traditional approach: {} μs", duration_traditional.count()) << std::endl;
    std::cout << std::format("Ranges approach: {} μs", duration_ranges.count()) << std::endl;
    std::cout << std::format("Results match: {}", filtered_traditional.size() == filtered_ranges.size()) << std::endl;
    
    double speedup = static_cast<double>(duration_traditional.count()) / duration_ranges.count();
    std::cout << std::format("Speedup: {:.2f}x", speedup) << std::endl;
}

// ===== COMPLEX DATA TRANSFORMATIONS =====

void demonstrate_complex_transformations() {
    std::cout << "\n=== Complex Data Transformations ===" << std::endl;
    
    auto planets = generate_planets();
    auto fleets = generate_fleets();
    
    // Create a comprehensive space empire report
    struct EmpireStats {
        int total_planets;
        int habitable_planets;
        long long total_population;
        int active_fleets;
        double average_fuel_level;
        std::vector<std::string> all_resources;
    };
    
    // Calculate empire statistics using ranges
    auto habitable_count = rng::count_if(planets, [](const Planet& p) { return p.habitable; });
    
    auto total_pop = planets
        | views::transform([](const Planet& p) { return static_cast<long long>(p.population); })
        | views::common;
    long long population_sum = std::accumulate(total_pop.begin(), total_pop.end(), 0LL);
    
    auto active_fleet_count = rng::count_if(fleets, [](const Fleet& f) { return f.is_active; });
    
    auto fuel_levels = fleets
        | views::filter([](const Fleet& f) { return f.is_active; })
        | views::transform([](const Fleet& f) { return f.fuel_level; })
        | views::common;
    double avg_fuel = std::accumulate(fuel_levels.begin(), fuel_levels.end(), 0.0) / std::distance(fuel_levels.begin(), fuel_levels.end());
    
    // Collect all unique resources
    std::set<std::string> unique_resources;
    for (const auto& planet : planets) {
        for (const auto& resource : planet.resources) {
            unique_resources.insert(resource);
        }
    }
    
    EmpireStats stats{
        static_cast<int>(planets.size()),
        static_cast<int>(habitable_count),
        population_sum,
        static_cast<int>(active_fleet_count),
        avg_fuel,
        std::vector<std::string>(unique_resources.begin(), unique_resources.end())
    };
    
    // Display comprehensive report
    std::cout << "\n🌌 SPACE EMPIRE STATUS REPORT 🌌" << std::endl;
    std::cout << "=================================" << std::endl;
    std::cout << std::format("Total Planets: {}", stats.total_planets) << std::endl;
    std::cout << std::format("Habitable Planets: {}", stats.habitable_planets) << std::endl;
    std::cout << std::format("Total Population: {:L}", stats.total_population) << std::endl;
    std::cout << std::format("Active Fleets: {}", stats.active_fleets) << std::endl;
    std::cout << std::format("Average Fleet Fuel Level: {:.1f}%", stats.average_fuel_level) << std::endl;
    
    std::cout << "Available Resources: ";
    for (size_t i = 0; i < stats.all_resources.size(); ++i) {
        if (i > 0) std::cout << ", ";
        std::cout << stats.all_resources[i];
    }
    std::cout << std::endl;
}

// ===== RANGE COMPOSITION PATTERNS =====

void demonstrate_range_composition() {
    std::cout << "\n=== Range Composition Patterns ===" << std::endl;
    
    // Create a complex pipeline that combines multiple data sources
    auto planets = generate_planets();
    auto fleets = generate_fleets();
    auto missions = generate_missions();
    
    // Multi-step analysis pipeline
    auto analysis_pipeline = [&]() {
        // Step 1: Find active exploration fleets
        auto exploration_fleets = fleets
            | views::filter([](const Fleet& f) { 
                return f.is_active && f.mission_type == "Exploration" && f.fuel_level > 60.0; 
            });
        
        // Step 2: Find suitable target planets
        auto target_planets = planets
            | views::filter([](const Planet& p) { 
                return p.distance_from_sun < 50.0 && !p.resources.empty(); 
            });
        
        // Step 3: Create fleet-planet pairs for optimal assignments
        std::vector<std::pair<std::string, std::string>> assignments;
        
        auto fleet_names = exploration_fleets 
            | views::transform([](const Fleet& f) { return f.commander; });
        auto planet_names = target_planets 
            | views::transform([](const Planet& p) { return p.name; });
        
        // Simple pairing (in real scenario, would use more complex assignment logic)
        auto fleet_it = fleet_names.begin();
        auto planet_it = planet_names.begin();
        
        while (fleet_it != fleet_names.end() && planet_it != planet_names.end()) {
            assignments.emplace_back(*fleet_it, *planet_it);
            ++fleet_it;
            ++planet_it;
        }
        
        return assignments;
    };
    
    auto assignments = analysis_pipeline();
    
    std::cout << "Optimal Fleet-Planet Assignments:" << std::endl;
    for (const auto& [commander, planet] : assignments) {
        std::cout << std::format("  - {} → {}", commander, planet) << std::endl;
    }
    
    // Demonstrate range adaptors chaining
    std::cout << "\nComplex Range Adaptor Chain:" << std::endl;
    auto complex_chain = views::iota(1, 50)  // Numbers 1-49
        | views::filter([](int n) { return n % 2 == 1; })     // Odd numbers
        | views::transform([](int n) { return n * n; })       // Square them
        | views::filter([](int n) { return n < 500; })        // Keep under 500
        | views::reverse                                       // Reverse order
        | views::chunk(3)                                      // Group in chunks of 3
        | views::take(3);                                      // Take first 3 chunks
    
    for (auto chunk : complex_chain) {
        std::cout << "Chunk: ";
        for (auto n : chunk) {
            std::cout << n << " ";
        }
        std::cout << std::endl;
    }
}

// ===== INTEGRATION WITH STL ALGORITHMS =====

void demonstrate_stl_integration() {
    std::cout << "\n=== STL Algorithms Integration with Ranges ===" << std::endl;
    
    auto planets = generate_planets();
    
    // Use ranges with STL algorithms
    std::vector<double> distances;
    rng::transform(planets, std::back_inserter(distances), 
                   [](const Planet& p) { return p.distance_from_sun; });
    
    // Sort distances
    rng::sort(distances);
    
    std::cout << "Sorted planet distances: ";
    for (size_t i = 0; i < std::min(distances.size(), size_t{5}); ++i) {
        std::cout << distances[i] << " ";
    }
    std::cout << std::endl;
    
    // Find median distance
    auto median_it = distances.begin() + distances.size() / 2;
    std::nth_element(distances.begin(), median_it, distances.end());
    std::cout << std::format("Median distance: {:.1f} AU", *median_it) << std::endl;
    
    // Use binary search on sorted data
    double search_distance = 5.0;
    bool found = std::binary_search(distances.begin(), distances.end(), search_distance);
    std::cout << std::format("Distance {:.1f} AU found: {}", search_distance, found ? "Yes" : "No") << std::endl;
}

} // namespace CppVerseHub::Modern::Ranges