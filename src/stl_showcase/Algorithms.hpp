// File: src/stl_showcase/Algorithms.hpp
// CppVerseHub - STL Algorithms Demonstrations (sort, transform, accumulate, etc.)

#pragma once

#include <algorithm>
#include <numeric>
#include <functional>
#include <vector>
#include <string>
#include <iostream>
#include <random>
#include <chrono>
#include <execution>
#include <map>
#include <set>
#include <iterator>

namespace CppVerseHub::STL {

/**
 * @brief Mission data structure for algorithm demonstrations
 */
struct Mission {
    std::string id;
    std::string type;
    int priority;
    double duration_hours;
    double success_probability;
    std::string assigned_fleet;
    
    Mission(const std::string& mission_id = "", const std::string& mission_type = "",
           int prio = 0, double duration = 0.0, double success = 1.0, 
           const std::string& fleet = "")
        : id(mission_id), type(mission_type), priority(prio), 
          duration_hours(duration), success_probability(success), assigned_fleet(fleet) {}
    
    friend std::ostream& operator<<(std::ostream& os, const Mission& m) {
        os << m.id << " (" << m.type << ") - Priority: " << m.priority 
           << ", Duration: " << m.duration_hours << "h, Success: " 
           << (m.success_probability * 100) << "%";
        return os;
    }
};

/**
 * @brief Resource data structure
 */
struct Resource {
    std::string name;
    int quantity;
    double unit_value;
    std::string location;
    
    Resource(const std::string& n = "", int q = 0, double v = 0.0, const std::string& loc = "")
        : name(n), quantity(q), unit_value(v), location(loc) {}
    
    double getTotalValue() const {
        return quantity * unit_value;
    }
    
    friend std::ostream& operator<<(std::ostream& os, const Resource& r) {
        os << r.name << ": " << r.quantity << " units @ " << r.unit_value 
           << " credits each (Total: " << r.getTotalValue() << " credits)";
        return os;
    }
};

/**
 * @brief Non-modifying Algorithm Demonstrations
 * 
 * Demonstrates algorithms that don't modify the sequence:
 * find, count, search, equal, mismatch, etc.
 */
class NonModifyingAlgorithms {
public:
    /**
     * @brief Demonstrate finding algorithms
     */
    static void demonstrateFindingAlgorithms() {
        std::cout << "\n=== Finding Algorithms ===\n";
        
        std::vector<Mission> missions{
            {"M001", "Exploration", 3, 24.0, 0.8, "Alpha Squadron"},
            {"M002", "Combat", 8, 6.0, 0.6, "Beta Fleet"},
            {"M003", "Transport", 2, 12.0, 0.95, "Cargo Wing"},
            {"M004", "Combat", 9, 8.0, 0.5, "Strike Force"},
            {"M005", "Diplomatic", 5, 48.0, 0.9, "Diplomatic Corps"},
            {"M006", "Rescue", 10, 4.0, 0.7, "Emergency Response"}
        };
        
        // std::find - find by value
        auto mission_id_to_find = "M003";
        auto it = std::find_if(missions.begin(), missions.end(),
            [&mission_id_to_find](const Mission& m) { return m.id == mission_id_to_find; });
        
        if (it != missions.end()) {
            std::cout << "Found mission: " << *it << "\n";
        }
        
        // std::find_if - find by predicate
        auto high_priority_it = std::find_if(missions.begin(), missions.end(),
            [](const Mission& m) { return m.priority >= 8; });
        
        if (high_priority_it != missions.end()) {
            std::cout << "First high priority mission: " << *high_priority_it << "\n";
        }
        
        // std::find_if_not - find first element NOT matching predicate
        auto non_combat_it = std::find_if_not(missions.begin(), missions.end(),
            [](const Mission& m) { return m.type == "Combat"; });
        
        if (non_combat_it != missions.end()) {
            std::cout << "First non-combat mission: " << *non_combat_it << "\n";
        }
        
        // std::count - count elements equal to value
        long combat_count = std::count_if(missions.begin(), missions.end(),
            [](const Mission& m) { return m.type == "Combat"; });
        
        std::cout << "Combat missions count: " << combat_count << "\n";
        
        // std::count_if - count elements matching predicate
        long urgent_count = std::count_if(missions.begin(), missions.end(),
            [](const Mission& m) { return m.priority >= 7; });
        
        std::cout << "Urgent missions (priority >= 7): " << urgent_count << "\n";
        
        // std::all_of, std::any_of, std::none_of
        bool all_assigned = std::all_of(missions.begin(), missions.end(),
            [](const Mission& m) { return !m.assigned_fleet.empty(); });
        std::cout << "All missions assigned: " << (all_assigned ? "Yes" : "No") << "\n";
        
        bool any_high_risk = std::any_of(missions.begin(), missions.end(),
            [](const Mission& m) { return m.success_probability < 0.6; });
        std::cout << "Any high risk missions: " << (any_high_risk ? "Yes" : "No") << "\n";
        
        bool none_zero_duration = std::none_of(missions.begin(), missions.end(),
            [](const Mission& m) { return m.duration_hours == 0.0; });
        std::cout << "No zero duration missions: " << (none_zero_duration ? "Yes" : "No") << "\n";
    }
    
    /**
     * @brief Demonstrate search algorithms
     */
    static void demonstrateSearchAlgorithms() {
        std::cout << "\n=== Search Algorithms ===\n";
        
        std::vector<int> fleet_ids{101, 102, 103, 104, 105, 106, 107, 108, 109, 110};
        std::vector<int> patrol_pattern{104, 105, 106};
        
        // std::search - find subsequence
        auto patrol_it = std::search(fleet_ids.begin(), fleet_ids.end(),
                                   patrol_pattern.begin(), patrol_pattern.end());
        
        if (patrol_it != fleet_ids.end()) {
            std::cout << "Patrol pattern found starting at fleet ID: " << *patrol_it << "\n";
        }
        
        // std::search_n - find n consecutive elements equal to value
        std::vector<int> sensor_readings{1, 1, 1, 2, 3, 3, 3, 3, 4, 5};
        auto consecutive_it = std::search_n(sensor_readings.begin(), sensor_readings.end(), 
                                          3, 3); // Find 3 consecutive 3's
        
        if (consecutive_it != sensor_readings.end()) {
            std::cout << "Found 3 consecutive readings of value 3 starting at position: "
                     << std::distance(sensor_readings.begin(), consecutive_it) << "\n";
        }
        
        // std::equal - check if two ranges are equal
        std::vector<int> backup_fleet_ids{101, 102, 103, 104, 105, 106, 107, 108, 109, 110};
        bool fleets_match = std::equal(fleet_ids.begin(), fleet_ids.end(), 
                                     backup_fleet_ids.begin());
        std::cout << "Fleet configurations match: " << (fleets_match ? "Yes" : "No") << "\n";
        
        // std::mismatch - find first position where ranges differ
        std::vector<int> modified_fleet{101, 102, 999, 104, 105, 106, 107, 108, 109, 110};
        auto mismatch_pair = std::mismatch(fleet_ids.begin(), fleet_ids.end(),
                                         modified_fleet.begin());
        
        if (mismatch_pair.first != fleet_ids.end()) {
            std::cout << "First difference: " << *mismatch_pair.first 
                     << " vs " << *mismatch_pair.second << "\n";
        }
    }
    
    /**
     * @brief Demonstrate min/max algorithms
     */
    static void demonstrateMinMaxAlgorithms() {
        std::cout << "\n=== Min/Max Algorithms ===\n";
        
        std::vector<Resource> resources{
            {"Deuterium", 500, 10.5, "Mining Station Alpha"},
            {"Tritium", 200, 25.0, "Gas Giant Harvester"},
            {"Dilithium", 50, 100.0, "Crystal Mines Beta"},
            {"Iron Ore", 2000, 2.0, "Asteroid Belt"},
            {"Platinum", 100, 50.0, "Deep Space Refinery"}
        };
        
        // std::min_element / std::max_element
        auto min_quantity_it = std::min_element(resources.begin(), resources.end(),
            [](const Resource& a, const Resource& b) { return a.quantity < b.quantity; });
        
        auto max_value_it = std::max_element(resources.begin(), resources.end(),
            [](const Resource& a, const Resource& b) { return a.unit_value < b.unit_value; });
        
        std::cout << "Resource with minimum quantity: " << *min_quantity_it << "\n";
        std::cout << "Resource with maximum unit value: " << *max_value_it << "\n";
        
        // std::minmax_element - get both min and max in one pass
        auto minmax_total_value = std::minmax_element(resources.begin(), resources.end(),
            [](const Resource& a, const Resource& b) { 
                return a.getTotalValue() < b.getTotalValue(); 
            });
        
        std::cout << "Minimum total value: " << *minmax_total_value.first << "\n";
        std::cout << "Maximum total value: " << *minmax_total_value.second << "\n";
        
        // std::clamp (C++17)
        double sensor_reading = 150.0;
        double min_valid = 0.0;
        double max_valid = 100.0;
        double clamped_reading = std::clamp(sensor_reading, min_valid, max_valid);
        
        std::cout << "Sensor reading " << sensor_reading << " clamped to [" 
                 << min_valid << ", " << max_valid << "] = " << clamped_reading << "\n";
    }
};

/**
 * @brief Modifying Algorithm Demonstrations
 * 
 * Demonstrates algorithms that modify sequences:
 * copy, move, transform, fill, generate, etc.
 */
class ModifyingAlgorithms {
public:
    /**
     * @brief Demonstrate copying algorithms
     */
    static void demonstrateCopyingAlgorithms() {
        std::cout << "\n=== Copying Algorithms ===\n";
        
        std::vector<std::string> fleet_names{
            "Enterprise", "Voyager", "Defiant", "Discovery", "Constitution"
        };
        
        // std::copy
        std::vector<std::string> backup_fleet;
        backup_fleet.resize(fleet_names.size());
        std::copy(fleet_names.begin(), fleet_names.end(), backup_fleet.begin());
        
        std::cout << "Original fleet: ";
        for (const auto& name : fleet_names) std::cout << name << " ";
        std::cout << "\nBackup fleet: ";
        for (const auto& name : backup_fleet) std::cout << name << " ";
        std::cout << "\n";
        
        // std::copy_if - conditional copying
        std::vector<std::string> short_names;
        std::copy_if(fleet_names.begin(), fleet_names.end(), 
                    std::back_inserter(short_names),
                    [](const std::string& name) { return name.length() <= 7; });
        
        std::cout << "Short names (<=7 chars): ";
        for (const auto& name : short_names) std::cout << name << " ";
        std::cout << "\n";
        
        // std::copy_n - copy n elements
        std::vector<std::string> first_three;
        first_three.resize(3);
        std::copy_n(fleet_names.begin(), 3, first_three.begin());
        
        std::cout << "First three ships: ";
        for (const auto& name : first_three) std::cout << name << " ";
        std::cout << "\n";
        
        // std::copy_backward - copy in reverse order
        std::vector<std::string> reverse_copy;
        reverse_copy.resize(fleet_names.size());
        std::copy_backward(fleet_names.begin(), fleet_names.end(), reverse_copy.end());
        
        std::cout << "Reverse copy: ";
        for (const auto& name : reverse_copy) std::cout << name << " ";
        std::cout << "\n";
        
        // std::move - transfer ownership (for moveable types)
        std::vector<std::unique_ptr<std::string>> original_ptrs;
        original_ptrs.push_back(std::make_unique<std::string>("Alpha"));
        original_ptrs.push_back(std::make_unique<std::string>("Beta"));
        original_ptrs.push_back(std::make_unique<std::string>("Gamma"));
        
        std::vector<std::unique_ptr<std::string>> moved_ptrs;
        moved_ptrs.resize(original_ptrs.size());
        
        std::move(original_ptrs.begin(), original_ptrs.end(), moved_ptrs.begin());
        
        std::cout << "After move, moved_ptrs contains: ";
        for (const auto& ptr : moved_ptrs) {
            if (ptr) std::cout << *ptr << " ";
        }
        std::cout << "\n";
    }
    
    /**
     * @brief Demonstrate transform algorithms
     */
    static void demonstrateTransformAlgorithms() {
        std::cout << "\n=== Transform Algorithms ===\n";
        
        std::vector<Mission> missions{
            {"M001", "Exploration", 3, 24.0, 0.8, "Alpha"},
            {"M002", "Combat", 8, 6.0, 0.6, "Beta"},
            {"M003", "Transport", 2, 12.0, 0.95, "Gamma"}
        };
        
        // std::transform - unary transformation
        std::vector<std::string> mission_summaries;
        std::transform(missions.begin(), missions.end(), 
                      std::back_inserter(mission_summaries),
                      [](const Mission& m) {
                          return m.id + " (" + m.type + ")";
                      });
        
        std::cout << "Mission summaries:\n";
        for (const auto& summary : mission_summaries) {
            std::cout << "- " << summary << "\n";
        }
        
        // Transform to calculate expected completion times
        std::vector<double> completion_times;
        std::transform(missions.begin(), missions.end(),
                      std::back_inserter(completion_times),
                      [](const Mission& m) {
                          return m.duration_hours / m.success_probability; // Adjusted for risk
                      });
        
        std::cout << "\nAdjusted completion times:\n";
        for (size_t i = 0; i < missions.size(); ++i) {
            std::cout << missions[i].id << ": " << completion_times[i] << " hours\n";
        }
        
        // std::transform - binary transformation
        std::vector<double> base_values{100.0, 200.0, 150.0};
        std::vector<double> multipliers{1.5, 2.0, 0.8};
        std::vector<double> adjusted_values;
        
        std::transform(base_values.begin(), base_values.end(),
                      multipliers.begin(),
                      std::back_inserter(adjusted_values),
                      [](double base, double mult) { return base * mult; });
        
        std::cout << "\nValue adjustments:\n";
        for (size_t i = 0; i < base_values.size(); ++i) {
            std::cout << base_values[i] << " * " << multipliers[i] 
                     << " = " << adjusted_values[i] << "\n";
        }
        
        // In-place transformation
        std::vector<int> coordinates{10, 20, 30, 40, 50};
        std::cout << "\nOriginal coordinates: ";
        for (int coord : coordinates) std::cout << coord << " ";
        
        std::transform(coordinates.begin(), coordinates.end(), coordinates.begin(),
                      [](int coord) { return coord * 2; }); // Scale by 2
        
        std::cout << "\nScaled coordinates: ";
        for (int coord : coordinates) std::cout << coord << " ";
        std::cout << "\n";
    }
    
    /**
     * @brief Demonstrate fill and generate algorithms
     */
    static void demonstrateFillGenerateAlgorithms() {
        std::cout << "\n=== Fill and Generate Algorithms ===\n";
        
        // std::fill - fill with constant value
        std::vector<double> fuel_levels(10);
        std::fill(fuel_levels.begin(), fuel_levels.end(), 100.0);
        
        std::cout << "Initial fuel levels (all full): ";
        for (double level : fuel_levels) std::cout << level << "% ";
        std::cout << "\n";
        
        // std::fill_n - fill n elements
        std::fill_n(fuel_levels.begin(), 3, 50.0); // First 3 ships partially fueled
        
        std::cout << "After partial refuel: ";
        for (double level : fuel_levels) std::cout << level << "% ";
        std::cout << "\n";
        
        // std::generate - fill with generated values
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(0.0, 100.0);
        
        std::vector<double> sensor_readings(8);
        std::generate(sensor_readings.begin(), sensor_readings.end(),
                     [&dis, &gen]() { return dis(gen); });
        
        std::cout << "Random sensor readings: ";
        for (double reading : sensor_readings) {
            std::cout << std::fixed << std::setprecision(1) << reading << "% ";
        }
        std::cout << "\n";
        
        // std::generate_n - generate n elements
        std::vector<int> mission_ids;
        int id_counter = 1000;
        std::generate_n(std::back_inserter(mission_ids), 5,
                       [&id_counter]() { return ++id_counter; });
        
        std::cout << "Generated mission IDs: ";
        for (int id : mission_ids) std::cout << id << " ";
        std::cout << "\n";
        
        // std::iota - fill with sequential values
        std::vector<int> fleet_numbers(10);
        std::iota(fleet_numbers.begin(), fleet_numbers.end(), 100);
        
        std::cout << "Fleet numbers (sequential): ";
        for (int num : fleet_numbers) std::cout << num << " ";
        std::cout << "\n";
    }
    
    /**
     * @brief Demonstrate replacement algorithms
     */
    static void demonstrateReplacementAlgorithms() {
        std::cout << "\n=== Replacement Algorithms ===\n";
        
        std::vector<std::string> ship_status{
            "Active", "Maintenance", "Active", "Disabled", "Active", "Maintenance", "Active"
        };
        
        std::cout << "Original status: ";
        for (const auto& status : ship_status) std::cout << status << " ";
        std::cout << "\n";
        
        // std::replace - replace all occurrences
        std::replace(ship_status.begin(), ship_status.end(), 
                    std::string("Maintenance"), std::string("Servicing"));
        
        std::cout << "After replacing 'Maintenance' with 'Servicing': ";
        for (const auto& status : ship_status) std::cout << status << " ";
        std::cout << "\n";
        
        // std::replace_if - conditional replacement
        std::replace_if(ship_status.begin(), ship_status.end(),
                       [](const std::string& status) { return status == "Disabled"; },
                       std::string("Repair"));
        
        std::cout << "After replacing 'Disabled' with 'Repair': ";
        for (const auto& status : ship_status) std::cout << status << " ";
        std::cout << "\n";
        
        // std::replace_copy - replace and copy to new container
        std::vector<std::string> updated_status;
        std::replace_copy(ship_status.begin(), ship_status.end(),
                         std::back_inserter(updated_status),
                         std::string("Active"), std::string("Operational"));
        
        std::cout << "Copy with 'Active' -> 'Operational': ";
        for (const auto& status : updated_status) std::cout << status << " ";
        std::cout << "\n";
        
        // std::replace_copy_if - conditional replace and copy
        std::vector<std::string> filtered_status;
        std::replace_copy_if(ship_status.begin(), ship_status.end(),
                            std::back_inserter(filtered_status),
                            [](const std::string& status) { return status.length() > 6; },
                            std::string("Long"));
        
        std::cout << "Copy with long names -> 'Long': ";
        for (const auto& status : filtered_status) std::cout << status << " ";
        std::cout << "\n";
    }
};

/**
 * @brief Sorting Algorithm Demonstrations
 * 
 * Demonstrates sorting and related algorithms:
 * sort, partial_sort, nth_element, stable_sort, etc.
 */
class SortingAlgorithms {
public:
    /**
     * @brief Demonstrate basic sorting algorithms
     */
    static void demonstrateBasicSorting() {
        std::cout << "\n=== Basic Sorting Algorithms ===\n";
        
        std::vector<Mission> missions{
            {"M005", "Diplomatic", 5, 48.0, 0.9, "Corps"},
            {"M001", "Exploration", 3, 24.0, 0.8, "Alpha"},
            {"M003", "Transport", 2, 12.0, 0.95, "Gamma"},
            {"M004", "Combat", 9, 8.0, 0.5, "Strike"},
            {"M002", "Combat", 8, 6.0, 0.6, "Beta"},
            {"M006", "Rescue", 10, 4.0, 0.7, "Emergency"}
        };
        
        // std::sort - general purpose sort
        std::cout << "Original mission order:\n";
        for (const auto& mission : missions) {
            std::cout << "- " << mission.id << " (Priority: " << mission.priority << ")\n";
        }
        
        // Sort by priority (descending)
        std::sort(missions.begin(), missions.end(),
                 [](const Mission& a, const Mission& b) { return a.priority > b.priority; });
        
        std::cout << "\nSorted by priority (highest first):\n";
        for (const auto& mission : missions) {
            std::cout << "- " << mission.id << " (Priority: " << mission.priority << ")\n";
        }
        
        // std::stable_sort - maintains relative order of equal elements
        std::vector<Mission> missions_copy = missions;
        std::stable_sort(missions_copy.begin(), missions_copy.end(),
                        [](const Mission& a, const Mission& b) { return a.type < b.type; });
        
        std::cout << "\nStable sort by type (maintains priority order within types):\n";
        for (const auto& mission : missions_copy) {
            std::cout << "- " << mission.id << " (" << mission.type 
                     << ", Priority: " << mission.priority << ")\n";
        }
        
        // Sort with multiple criteria
        std::sort(missions.begin(), missions.end(),
                 [](const Mission& a, const Mission& b) {
                     if (a.type != b.type) return a.type < b.type;
                     return a.priority > b.priority; // Higher priority first within same type
                 });
        
        std::cout << "\nSorted by type, then priority:\n";
        for (const auto& mission : missions) {
            std::cout << "- " << mission.id << " (" << mission.type 
                     << ", Priority: " << mission.priority << ")\n";
        }
    }
    
    /**
     * @brief Demonstrate partial sorting algorithms
     */
    static void demonstratePartialSorting() {
        std::cout << "\n=== Partial Sorting Algorithms ===\n";
        
        std::vector<Resource> resources{
            {"Iron", 2000, 2.0, "Asteroid"},
            {"Gold", 100, 50.0, "Mining"},
            {"Platinum", 50, 100.0, "Refinery"},
            {"Silver", 300, 20.0, "Station"},
            {"Copper", 1500, 5.0, "Colony"},
            {"Titanium", 200, 80.0, "Harvester"},
            {"Uranium", 75, 150.0, "Deep Mine"},
            {"Diamonds", 25, 500.0, "Special"}
        };
        
        // std::partial_sort - get top N elements
        std::cout << "All resources:\n";
        for (const auto& r : resources) {
            std::cout << "- " << r.name << ": " << r.getTotalValue() << " total value\n";
        }
        
        // Get top 3 most valuable resources
        auto resources_copy = resources;
        std::partial_sort(resources_copy.begin(), resources_copy.begin() + 3, resources_copy.end(),
                         [](const Resource& a, const Resource& b) {
                             return a.getTotalValue() > b.getTotalValue();
                         });
        
        std::cout << "\nTop 3 most valuable resources:\n";
        for (size_t i = 0; i < 3; ++i) {
            std::cout << i+1 << ". " << resources_copy[i].name 
                     << ": " << resources_copy[i].getTotalValue() << " credits\n";
        }
        
        // std::nth_element - get the nth element as if fully sorted
        resources_copy = resources;
        std::nth_element(resources_copy.begin(), resources_copy.begin() + 3, resources_copy.end(),
                        [](const Resource& a, const Resource& b) {
                            return a.unit_value > b.unit_value;
                        });
        
        std::cout << "\n4th highest unit value resource: " << resources_copy[3].name 
                 << " (" << resources_copy[3].unit_value << " credits/unit)\n";
        
        // Elements before the nth are smaller, after are larger (but not sorted)
        std::cout << "Resources with higher unit value: ";
        for (size_t i = 0; i < 3; ++i) {
            std::cout << resources_copy[i].name << " ";
        }
        std::cout << "\nResources with lower unit value: ";
        for (size_t i = 4; i < resources_copy.size(); ++i) {
            std::cout << resources_copy[i].name << " ";
        }
        std::cout << "\n";
    }
    
    /**
     * @brief Demonstrate heap algorithms
     */
    static void demonstrateHeapAlgorithms() {
        std::cout << "\n=== Heap Algorithms ===\n";
        
        std::vector<int> priorities{3, 1, 4, 1, 5, 9, 2, 6, 5, 3};
        
        std::cout << "Original priorities: ";
        for (int p : priorities) std::cout << p << " ";
        std::cout << "\n";
        
        // std::make_heap - create a heap (max heap by default)
        std::make_heap(priorities.begin(), priorities.end());
        
        std::cout << "After make_heap: ";
        for (int p : priorities) std::cout << p << " ";
        std::cout << "\nMax element (heap property): " << priorities.front() << "\n";
        
        // std::push_heap - add element to heap
        priorities.push_back(10);
        std::push_heap(priorities.begin(), priorities.end());
        
        std::cout << "After adding 10: ";
        for (int p : priorities) std::cout << p << " ";
        std::cout << "\nNew max element: " << priorities.front() << "\n";
        
        // std::pop_heap - remove max element
        std::pop_heap(priorities.begin(), priorities.end());
        int max_element = priorities.back();
        priorities.pop_back();
        
        std::cout << "Removed max element: " << max_element << "\n";
        std::cout << "After pop_heap: ";
        for (int p : priorities) std::cout << p << " ";
        std::cout << "\nNew max element: " << priorities.front() << "\n";
        
        // std::sort_heap - sort the heap (destroys heap property)
        std::sort_heap(priorities.begin(), priorities.end());
        
        std::cout << "After sort_heap (ascending): ";
        for (int p : priorities) std::cout << p << " ";
        std::cout << "\n";
        
        // std::is_heap - check if range is a heap
        bool is_heap_now = std::is_heap(priorities.begin(), priorities.end());
        std::cout << "Is still a heap after sorting: " << (is_heap_now ? "Yes" : "No") << "\n";
    }
};

/**
 * @brief Numeric Algorithm Demonstrations
 * 
 * Demonstrates numeric algorithms from <numeric>:
 * accumulate, inner_product, partial_sum, adjacent_difference, etc.
 */
class NumericAlgorithms {
public:
    /**
     * @brief Demonstrate accumulation algorithms
     */
    static void demonstrateAccumulationAlgorithms() {
        std::cout << "\n=== Accumulation Algorithms ===\n";
        
        std::vector<Resource> inventory{
            {"Fuel", 1000, 5.0, "Station A"},
            {"Food", 500, 10.0, "Station B"}, 
            {"Medicine", 100, 50.0, "Station C"},
            {"Equipment", 200, 25.0, "Station D"}
        };
        
        // std::accumulate - sum with initial value
        double total_value = std::accumulate(inventory.begin(), inventory.end(), 0.0,
            [](double sum, const Resource& r) { return sum + r.getTotalValue(); });
        
        std::cout << "Total inventory value: " << total_value << " credits\n";
        
        // Find total quantity
        int total_quantity = std::accumulate(inventory.begin(), inventory.end(), 0,
            [](int sum, const Resource& r) { return sum + r.quantity; });
        
        std::cout << "Total inventory quantity: " << total_quantity << " units\n";
        
        // Find most expensive resource using accumulate
        auto most_expensive = std::accumulate(inventory.begin() + 1, inventory.end(), 
                                            inventory[0],
            [](const Resource& max_so_far, const Resource& current) {
                return (current.unit_value > max_so_far.unit_value) ? current : max_so_far;
            });
        
        std::cout << "Most expensive resource: " << most_expensive.name 
                 << " (" << most_expensive.unit_value << " credits/unit)\n";
        
        // std::reduce (C++17) - similar to accumulate but can be parallelized
        #ifdef __cpp_lib_parallel_algorithm
        double parallel_total = std::reduce(std::execution::par,
                                          inventory.begin(), inventory.end(), 0.0,
            [](double sum, const Resource& r) { return sum + r.getTotalValue(); });
        std::cout << "Parallel total value: " << parallel_total << " credits\n";
        #endif
    }
    
    /**
     * @brief Demonstrate product and transformation algorithms
     */
    static void demonstrateProductAlgorithms() {
        std::cout << "\n=== Product Algorithms ===\n";
        
        std::vector<double> base_prices{10.0, 15.0, 20.0, 25.0, 30.0};
        std::vector<double> multipliers{1.2, 0.9, 1.5, 0.8, 1.1};
        
        // std::inner_product - dot product of two ranges
        double total_adjusted_value = std::inner_product(
            base_prices.begin(), base_prices.end(),
            multipliers.begin(), 0.0
        );
        
        std::cout << "Base prices: ";
        for (double price : base_prices) std::cout << price << " ";
        std::cout << "\nMultipliers: ";
        for (double mult : multipliers) std::cout << mult << " ";
        std::cout << "\nTotal adjusted value: " << total_adjusted_value << "\n";
        
        // Custom binary operations with inner_product
        double max_adjusted_price = std::inner_product(
            base_prices.begin(), base_prices.end(),
            multipliers.begin(), 0.0,
            [](double a, double b) { return std::max(a, b); }, // Replace addition
            [](double price, double mult) { return price * mult; } // Replace multiplication
        );
        
        std::cout << "Maximum adjusted price: " << max_adjusted_price << "\n";
        
        // Calculate variance using inner_product
        double mean = std::accumulate(base_prices.begin(), base_prices.end(), 0.0) / base_prices.size();
        double variance = std::inner_product(
            base_prices.begin(), base_prices.end(),
            base_prices.begin(), 0.0,
            std::plus<double>(),
            [mean](double a, double b) { 
                double diff = a - mean;
                return diff * diff;
            }
        ) / base_prices.size();
        
        std::cout << "Mean price: " << mean << "\n";
        std::cout << "Price variance: " << variance << "\n";
    }
    
    /**
     * @brief Demonstrate sequence generation algorithms
     */
    static void demonstrateSequenceAlgorithms() {
        std::cout << "\n=== Sequence Generation Algorithms ===\n";
        
        // std::partial_sum - running totals
        std::vector<int> daily_production{100, 150, 120, 180, 90, 200, 160};
        std::vector<int> cumulative_production;
        
        std::partial_sum(daily_production.begin(), daily_production.end(),
                        std::back_inserter(cumulative_production));
        
        std::cout << "Daily production: ";
        for (int prod : daily_production) std::cout << prod << " ";
        std::cout << "\nCumulative production: ";
        for (int cum : cumulative_production) std::cout << cum << " ";
        std::cout << "\n";
        
        // std::adjacent_difference - differences between consecutive elements
        std::vector<int> production_changes;
        std::adjacent_difference(daily_production.begin(), daily_production.end(),
                               std::back_inserter(production_changes));
        
        std::cout << "Production changes: ";
        for (int change : production_changes) std::cout << change << " ";
        std::cout << "\n(Note: first element is the original value)\n";
        
        // Custom operation with partial_sum - running product
        std::vector<double> growth_factors{1.1, 1.05, 1.15, 0.95, 1.2};
        std::vector<double> cumulative_growth;
        
        std::partial_sum(growth_factors.begin(), growth_factors.end(),
                        std::back_inserter(cumulative_growth),
                        std::multiplies<double>());
        
        std::cout << "Growth factors: ";
        for (double factor : growth_factors) std::cout << factor << " ";
        std::cout << "\nCumulative growth: ";
        for (double cum : cumulative_growth) std::cout << cum << " ";
        std::cout << "\n";
        
        // std::exclusive_scan and std::inclusive_scan (C++17)
        std::vector<int> resource_consumption{10, 20, 15, 25, 30};
        std::cout << "\nResource consumption: ";
        for (int cons : resource_consumption) std::cout << cons << " ";
        
        #ifdef __cpp_lib_parallel_algorithm
        std::vector<int> exclusive_totals;
        std::exclusive_scan(resource_consumption.begin(), resource_consumption.end(),
                          std::back_inserter(exclusive_totals), 0);
        
        std::cout << "\nExclusive scan (running total before current): ";
        for (int total : exclusive_totals) std::cout << total << " ";
        std::cout << "\n";
        #endif
    }
};

/**
 * @brief Set Algorithm Demonstrations
 * 
 * Demonstrates set operations on sorted ranges:
 * set_union, set_intersection, set_difference, etc.
 */
class SetAlgorithms {
public:
    /**
     * @brief Demonstrate set operations
     */
    static void demonstrateSetOperations() {
        std::cout << "\n=== Set Operations ===\n";
        
        // Sorted ranges required for set algorithms
        std::vector<std::string> fleet_alpha{"Cruiser", "Destroyer", "Fighter", "Scout", "Transport"};
        std::vector<std::string> fleet_beta{"Battleship", "Cruiser", "Fighter", "Frigate", "Transport"};
        
        std::cout << "Fleet Alpha: ";
        for (const auto& ship : fleet_alpha) std::cout << ship << " ";
        std::cout << "\nFleet Beta: ";
        for (const auto& ship : fleet_beta) std::cout << ship << " ";
        std::cout << "\n";
        
        // std::set_union - combine both sets
        std::vector<std::string> combined_fleet;
        std::set_union(fleet_alpha.begin(), fleet_alpha.end(),
                      fleet_beta.begin(), fleet_beta.end(),
                      std::back_inserter(combined_fleet));
        
        std::cout << "\nUnion (all ship types): ";
        for (const auto& ship : combined_fleet) std::cout << ship << " ";
        std::cout << "\n";
        
        // std::set_intersection - common elements
        std::vector<std::string> common_ships;
        std::set_intersection(fleet_alpha.begin(), fleet_alpha.end(),
                            fleet_beta.begin(), fleet_beta.end(),
                            std::back_inserter(common_ships));
        
        std::cout << "Intersection (common ship types): ";
        for (const auto& ship : common_ships) std::cout << ship << " ";
        std::cout << "\n";
        
        // std::set_difference - in first but not second
        std::vector<std::string> alpha_only;
        std::set_difference(fleet_alpha.begin(), fleet_alpha.end(),
                          fleet_beta.begin(), fleet_beta.end(),
                          std::back_inserter(alpha_only));
        
        std::cout << "Alpha only (in Alpha but not Beta): ";
        for (const auto& ship : alpha_only) std::cout << ship << " ";
        std::cout << "\n";
        
        // std::set_symmetric_difference - in either but not both
        std::vector<std::string> unique_ships;
        std::set_symmetric_difference(fleet_alpha.begin(), fleet_alpha.end(),
                                    fleet_beta.begin(), fleet_beta.end(),
                                    std::back_inserter(unique_ships));
        
        std::cout << "Symmetric difference (unique to each fleet): ";
        for (const auto& ship : unique_ships) std::cout << ship << " ";
        std::cout << "\n";
        
        // Test subset relationships
        std::vector<std::string> small_fleet{"Fighter", "Scout"};
        bool is_subset = std::includes(fleet_alpha.begin(), fleet_alpha.end(),
                                     small_fleet.begin(), small_fleet.end());
        
        std::cout << "Is {Fighter, Scout} a subset of Fleet Alpha: " 
                 << (is_subset ? "Yes" : "No") << "\n";
    }
    
    /**
     * @brief Demonstrate merge algorithms
     */
    static void demonstrateMergeAlgorithms() {
        std::cout << "\n=== Merge Algorithms ===\n";
        
        // Two sorted sequences to merge
        std::vector<int> priorities_a{1, 3, 5, 7, 9};
        std::vector<int> priorities_b{2, 4, 6, 8, 10};
        
        std::cout << "Priorities A: ";
        for (int p : priorities_a) std::cout << p << " ";
        std::cout << "\nPriorities B: ";
        for (int p : priorities_b) std::cout << p << " ";
        std::cout << "\n";
        
        // std::merge - merge two sorted ranges
        std::vector<int> merged_priorities;
        std::merge(priorities_a.begin(), priorities_a.end(),
                  priorities_b.begin(), priorities_b.end(),
                  std::back_inserter(merged_priorities));
        
        std::cout << "Merged priorities: ";
        for (int p : merged_priorities) std::cout << p << " ";
        std::cout << "\n";
        
        // std::inplace_merge - merge two consecutive sorted ranges in-place
        std::vector<int> combined{1, 5, 9, 2, 6, 10}; // Two sorted subsequences
        auto middle = combined.begin() + 3;
        
        std::cout << "Before inplace_merge: ";
        for (int p : combined) std::cout << p << " ";
        std::cout << "\n";
        
        std::inplace_merge(combined.begin(), middle, combined.end());
        
        std::cout << "After inplace_merge: ";
        for (int p : combined) std::cout << p << " ";
        std::cout << "\n";
    }
};

/**
 * @brief Permutation Algorithm Demonstrations
 * 
 * Demonstrates permutation algorithms:
 * next_permutation, prev_permutation, is_permutation, etc.
 */
class PermutationAlgorithms {
public:
    /**
     * @brief Demonstrate permutation generation
     */
    static void demonstratePermutationGeneration() {
        std::cout << "\n=== Permutation Generation ===\n";
        
        std::vector<std::string> patrol_route{"Alpha", "Beta", "Gamma"};
        
        std::cout << "All possible patrol routes:\n";
        std::sort(patrol_route.begin(), patrol_route.end()); // Start with lexicographically first
        
        int route_count = 1;
        do {
            std::cout << "Route " << route_count++ << ": ";
            for (const auto& station : patrol_route) std::cout << station << " -> ";
            std::cout << "Alpha (return)\n";
        } while (std::next_permutation(patrol_route.begin(), patrol_route.end()));
        
        std::cout << "\nTotal possible routes: " << route_count - 1 << "\n";
        
        // Working backwards with prev_permutation
        std::vector<int> formation{3, 2, 1};
        std::cout << "\nFormation patterns (reverse order):\n";
        
        int pattern_count = 1;
        do {
            std::cout << "Pattern " << pattern_count++ << ": ";
            for (int ship : formation) std::cout << "Ship" << ship << " ";
            std::cout << "\n";
        } while (std::prev_permutation(formation.begin(), formation.end()));
        
        // Check if one sequence is a permutation of another
        std::vector<std::string> original_fleet{"Enterprise", "Voyager", "Defiant"};
        std::vector<std::string> reordered_fleet{"Defiant", "Enterprise", "Voyager"};
        std::vector<std::string> different_fleet{"Enterprise", "Discovery", "Constitution"};
        
        bool is_perm1 = std::is_permutation(original_fleet.begin(), original_fleet.end(),
                                           reordered_fleet.begin());
        bool is_perm2 = std::is_permutation(original_fleet.begin(), original_fleet.end(),
                                           different_fleet.begin());
        
        std::cout << "\nIs reordered_fleet a permutation of original_fleet: " 
                 << (is_perm1 ? "Yes" : "No") << "\n";
        std::cout << "Is different_fleet a permutation of original_fleet: " 
                 << (is_perm2 ? "Yes" : "No") << "\n";
    }
    
    /**
     * @brief Demonstrate lexicographic operations
     */
    static void demonstrateLexicographicOperations() {
        std::cout << "\n=== Lexicographic Operations ===\n";
        
        std::vector<std::string> mission_alpha{"Alpha", "Beta", "Gamma"};
        std::vector<std::string> mission_beta{"Alpha", "Charlie", "Delta"};
        std::vector<std::string> mission_gamma{"Alpha", "Beta"};
        
        // std::lexicographical_compare
        bool alpha_less_beta = std::lexicographical_compare(
            mission_alpha.begin(), mission_alpha.end(),
            mission_beta.begin(), mission_beta.end()
        );
        
        bool alpha_less_gamma = std::lexicographical_compare(
            mission_alpha.begin(), mission_alpha.end(),
            mission_gamma.begin(), mission_gamma.end()
        );
        
        std::cout << "Mission Alpha < Mission Beta: " << (alpha_less_beta ? "Yes" : "No") << "\n";
        std::cout << "Mission Alpha < Mission Gamma: " << (alpha_less_gamma ? "Yes" : "No") << "\n";
        
        // Custom comparison
        auto case_insensitive_compare = [](const std::string& a, const std::string& b) {
            return std::lexicographical_compare(
                a.begin(), a.end(),
                b.begin(), b.end(),
                [](char c1, char c2) { return std::tolower(c1) < std::tolower(c2); }
            );
        };
        
        std::string word1 = "Hello";
        std::string word2 = "WORLD";
        bool case_insensitive_less = case_insensitive_compare(word1, word2);
        
        std::cout << "'" << word1 << "' < '" << word2 << "' (case insensitive): " 
                 << (case_insensitive_less ? "Yes" : "No") << "\n";
    }
};

/**
 * @brief Algorithm Performance Analysis
 * 
 * Analyzes performance characteristics of different algorithms.
 */
class AlgorithmPerformanceAnalysis {
public:
    /**
     * @brief Compare sorting algorithm performance
     */
    static void compareSortingPerformance() {
        std::cout << "\n=== Sorting Algorithm Performance Comparison ===\n";
        
        const int test_size = 100000;
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(1, 1000000);
        
        // Generate test data
        std::vector<int> test_data;
        test_data.reserve(test_size);
        for (int i = 0; i < test_size; ++i) {
            test_data.push_back(dis(gen));
        }
        
        // Test std::sort
        auto data_copy = test_data;
        auto start = std::chrono::high_resolution_clock::now();
        std::sort(data_copy.begin(), data_copy.end());
        auto end = std::chrono::high_resolution_clock::now();
        auto sort_time = std::chrono::duration<double, std::milli>(end - start).count();
        
        // Test std::stable_sort
        data_copy = test_data;
        start = std::chrono::high_resolution_clock::now();
        std::stable_sort(data_copy.begin(), data_copy.end());
        end = std::chrono::high_resolution_clock::now();
        auto stable_sort_time = std::chrono::duration<double, std::milli>(end - start).count();
        
        // Test std::partial_sort (top 1000 elements)
        data_copy = test_data;
        start = std::chrono::high_resolution_clock::now();
        std::partial_sort(data_copy.begin(), data_copy.begin() + 1000, data_copy.end());
        end = std::chrono::high_resolution_clock::now();
        auto partial_sort_time = std::chrono::duration<double, std::milli>(end - start).count();
        
        std::cout << "Sorting " << test_size << " elements:\n";
        std::cout << "std::sort: " << sort_time << " ms\n";
        std::cout << "std::stable_sort: " << stable_sort_time << " ms\n";
        std::cout << "std::partial_sort (top 1000): " << partial_sort_time << " ms\n";
        
        // Test nth_element
        data_copy = test_data;
        start = std::chrono::high_resolution_clock::now();
        std::nth_element(data_copy.begin(), data_copy.begin() + test_size/2, data_copy.end());
        end = std::chrono::high_resolution_clock::now();
        auto nth_element_time = std::chrono::duration<double, std::milli>(end - start).count();
        
        std::cout << "std::nth_element (median): " << nth_element_time << " ms\n";
    }
    
    /**
     * @brief Compare search algorithm performance
     */
    static void compareSearchPerformance() {
        std::cout << "\n=== Search Algorithm Performance Comparison ===\n";
        
        const int test_size = 1000000;
        const int search_count = 1000;
        
        // Create sorted data for binary search
        std::vector<int> sorted_data;
        for (int i = 0; i < test_size; ++i) {
            sorted_data.push_back(i);
        }
        
        // Create search targets
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, test_size - 1);
        
        std::vector<int> search_targets;
        for (int i = 0; i < search_count; ++i) {
            search_targets.push_back(dis(gen));
        }
        
        // Test linear search
        auto start = std::chrono::high_resolution_clock::now();
        int found_count = 0;
        for (int target : search_targets) {
            auto it = std::find(sorted_data.begin(), sorted_data.end(), target);
            if (it != sorted_data.end()) found_count++;
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto linear_time = std::chrono::duration<double, std::milli>(end - start).count();
        
        // Test binary search
        start = std::chrono::high_resolution_clock::now();
        found_count = 0;
        for (int target : search_targets) {
            if (std::binary_search(sorted_data.begin(), sorted_data.end(), target)) {
                found_count++;
            }
        }
        end = std::chrono::high_resolution_clock::now();
        auto binary_time = std::chrono::duration<double, std::milli>(end - start).count();
        
        std::cout << search_count << " searches in " << test_size << " elements:\n";
        std::cout << "Linear search (std::find): " << linear_time << " ms\n";
        std::cout << "Binary search: " << binary_time << " ms\n";
        std::cout << "Binary search speedup: " << (linear_time / binary_time) << "x\n";
    }
};

} // namespace CppVerseHub::STL