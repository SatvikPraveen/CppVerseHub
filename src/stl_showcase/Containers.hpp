// File: src/stl_showcase/Containers.hpp
// CppVerseHub - STL Container Demonstrations (vector, map, set, deque, etc.)

#pragma once

#include <vector>
#include <deque>
#include <list>
#include <forward_list>
#include <array>
#include <map>
#include <unordered_map>
#include <set>
#include <unordered_set>
#include <stack>
#include <queue>
#include <priority_queue>
#include <string>
#include <memory>
#include <iostream>
#include <algorithm>
#include <numeric>
#include <chrono>
#include <random>
#include <functional>

namespace CppVerseHub::STL {

/**
 * @brief Spacecraft data structure for container demonstrations
 */
struct Spacecraft {
    std::string name;
    std::string class_type;
    double mass;
    int crew_size;
    double max_speed;
    double firepower;
    
    Spacecraft(const std::string& n = "", const std::string& ct = "", 
              double m = 0.0, int cs = 0, double ms = 0.0, double fp = 0.0)
        : name(n), class_type(ct), mass(m), crew_size(cs), max_speed(ms), firepower(fp) {}
    
    bool operator<(const Spacecraft& other) const {
        return name < other.name;
    }
    
    bool operator==(const Spacecraft& other) const {
        return name == other.name;
    }
    
    double getCombatRating() const {
        return firepower * (1.0 + crew_size * 0.1) * (max_speed / 100.0);
    }
    
    friend std::ostream& operator<<(std::ostream& os, const Spacecraft& s) {
        os << s.name << " (" << s.class_type << ") - Mass: " << s.mass 
           << ", Crew: " << s.crew_size << ", Speed: " << s.max_speed
           << ", Firepower: " << s.firepower;
        return os;
    }
};

/**
 * @brief Hash function for Spacecraft (required for unordered containers)
 */
struct SpacecraftHash {
    std::size_t operator()(const Spacecraft& s) const {
        return std::hash<std::string>{}(s.name);
    }
};

/**
 * @brief Planet data structure
 */
struct Planet {
    std::string name;
    std::string system;
    double population;
    std::vector<std::string> resources;
    int defense_level;
    
    Planet(const std::string& n = "", const std::string& sys = "",
           double pop = 0.0, int def = 0)
        : name(n), system(sys), population(pop), defense_level(def) {}
    
    void addResource(const std::string& resource) {
        resources.push_back(resource);
    }
    
    friend std::ostream& operator<<(std::ostream& os, const Planet& p) {
        os << p.name << " (" << p.system << ") - Pop: " << p.population
           << ", Defense: " << p.defense_level << ", Resources: " << p.resources.size();
        return os;
    }
};

/**
 * @brief Sequential Container Demonstrations
 * 
 * Demonstrates vector, deque, list, forward_list, and array usage
 * in the context of fleet management and space exploration.
 */
class SequentialContainers {
public:
    /**
     * @brief Demonstrate std::vector usage
     */
    static void demonstrateVector() {
        std::cout << "\n=== Vector Demonstration ===\n";
        
        // Fleet management using vector
        std::vector<Spacecraft> fleet;
        fleet.reserve(10); // Optimize for known size
        
        // Adding spacecraft to the fleet
        fleet.emplace_back("Enterprise", "Heavy Cruiser", 5000.0, 400, 650.0, 1200.0);
        fleet.emplace_back("Millennium Falcon", "Light Freighter", 1050.0, 6, 1200.0, 400.0);
        fleet.emplace_back("Serenity", "Transport", 900.0, 9, 800.0, 200.0);
        fleet.push_back({"Normandy", "Frigate", 2500.0, 150, 900.0, 800.0});
        fleet.push_back({"Galactica", "Battlestar", 15000.0, 5000, 300.0, 2500.0});
        
        std::cout << "Fleet size: " << fleet.size() << "\n";
        std::cout << "Fleet capacity: " << fleet.capacity() << "\n\n";
        
        // Accessing elements
        std::cout << "Fleet roster:\n";
        for (size_t i = 0; i < fleet.size(); ++i) {
            std::cout << i + 1 << ". " << fleet[i] << "\n";
        }
        
        // Range-based for loop
        std::cout << "\nRange-based iteration:\n";
        for (const auto& ship : fleet) {
            std::cout << "- " << ship.name << " (Combat Rating: " 
                     << ship.getCombatRating() << ")\n";
        }
        
        // Vector algorithms
        std::cout << "\nSorting fleet by name:\n";
        std::sort(fleet.begin(), fleet.end());
        for (const auto& ship : fleet) {
            std::cout << "- " << ship.name << "\n";
        }
        
        // Finding elements
        auto it = std::find_if(fleet.begin(), fleet.end(),
            [](const Spacecraft& s) { return s.firepower > 1000.0; });
        
        if (it != fleet.end()) {
            std::cout << "\nFirst heavy combat vessel: " << it->name << "\n";
        }
        
        // Vector-specific operations
        fleet.insert(fleet.begin() + 2, {"Rocinante", "Gunship", 450.0, 4, 1100.0, 350.0});
        std::cout << "After insertion, fleet size: " << fleet.size() << "\n";
        
        // Removing elements
        fleet.erase(std::remove_if(fleet.begin(), fleet.end(),
            [](const Spacecraft& s) { return s.mass < 1000.0; }), fleet.end());
        
        std::cout << "After removing light vessels, fleet size: " << fleet.size() << "\n";
    }
    
    /**
     * @brief Demonstrate std::deque usage
     */
    static void demonstrateDeque() {
        std::cout << "\n=== Deque Demonstration ===\n";
        
        // Mission queue using deque (efficient front and back operations)
        std::deque<std::string> mission_queue;
        
        // Adding missions
        mission_queue.push_back("Explore Alpha Centauri");
        mission_queue.push_back("Defend Earth Station");
        mission_queue.push_back("Transport Colonists");
        mission_queue.push_front("URGENT: Rescue Mission"); // High priority
        
        std::cout << "Mission queue size: " << mission_queue.size() << "\n";
        std::cout << "Missions in order:\n";
        
        // Display queue
        for (size_t i = 0; i < mission_queue.size(); ++i) {
            std::cout << i + 1 << ". " << mission_queue[i] << "\n";
        }
        
        // Process missions from front
        std::cout << "\nProcessing missions:\n";
        while (!mission_queue.empty()) {
            std::cout << "Executing: " << mission_queue.front() << "\n";
            mission_queue.pop_front();
            
            // Simulate new urgent mission
            if (mission_queue.size() == 2) {
                mission_queue.push_front("EMERGENCY: Asteroid Threat");
                std::cout << "Added emergency mission to front of queue\n";
            }
        }
        
        // Demonstrate deque's random access
        std::deque<int> coordinates;
        for (int i = 0; i < 10; ++i) {
            coordinates.push_back(i * 10);
        }
        
        std::cout << "\nCoordinate access:\n";
        std::cout << "Middle coordinate: " << coordinates[coordinates.size()/2] << "\n";
        std::cout << "Last coordinate: " << coordinates.back() << "\n";
    }
    
    /**
     * @brief Demonstrate std::list usage
     */
    static void demonstrateList() {
        std::cout << "\n=== List Demonstration ===\n";
        
        // Patrol route using list (efficient insertion/deletion anywhere)
        std::list<std::string> patrol_route;
        
        patrol_route.push_back("Earth Orbit");
        patrol_route.push_back("Mars Station");
        patrol_route.push_back("Jupiter Mining Colony");
        patrol_route.push_back("Saturn Rings");
        
        std::cout << "Initial patrol route:\n";
        int waypoint = 1;
        for (const auto& location : patrol_route) {
            std::cout << waypoint++ << ". " << location << "\n";
        }
        
        // Insert new waypoints efficiently
        auto it = patrol_route.begin();
        std::advance(it, 2); // Move to third position
        patrol_route.insert(it, "Asteroid Belt Checkpoint");
        
        // Add emergency waypoint at beginning
        patrol_route.push_front("Command Briefing");
        
        std::cout << "\nUpdated patrol route:\n";
        waypoint = 1;
        for (const auto& location : patrol_route) {
            std::cout << waypoint++ << ". " << location << "\n";
        }
        
        // List-specific operations
        std::list<int> numbers = {3, 1, 4, 1, 5, 9, 2, 6, 5};
        std::cout << "\nOriginal numbers: ";
        for (int n : numbers) std::cout << n << " ";
        
        numbers.sort();
        std::cout << "\nSorted: ";
        for (int n : numbers) std::cout << n << " ";
        
        numbers.unique();
        std::cout << "\nUnique: ";
        for (int n : numbers) std::cout << n << " ";
        std::cout << "\n";
        
        // Merge two sorted lists
        std::list<int> more_numbers = {1, 3, 7, 8};
        numbers.merge(more_numbers);
        std::cout << "After merge: ";
        for (int n : numbers) std::cout << n << " ";
        std::cout << "\n";
    }
    
    /**
     * @brief Demonstrate std::forward_list usage
     */
    static void demonstrateForwardList() {
        std::cout << "\n=== Forward List Demonstration ===\n";
        
        // Communication chain using forward_list (memory efficient singly-linked list)
        std::forward_list<std::string> comm_chain;
        
        // Note: forward_list doesn't have push_back, only push_front
        comm_chain.push_front("Deep Space Relay");
        comm_chain.push_front("Outer Rim Station");
        comm_chain.push_front("Mars Communication Hub");
        comm_chain.push_front("Earth Command Center");
        
        std::cout << "Communication chain (front to back):\n";
        int hop = 1;
        for (const auto& station : comm_chain) {
            std::cout << "Hop " << hop++ << ": " << station << "\n";
        }
        
        // Insert after specific position (forward_list specialty)
        auto it = comm_chain.begin();
        std::advance(it, 1); // Move to second position
        comm_chain.insert_after(it, "Lunar Relay Station");
        
        std::cout << "\nAfter adding Lunar Relay:\n";
        hop = 1;
        for (const auto& station : comm_chain) {
            std::cout << "Hop " << hop++ << ": " << station << "\n";
        }
        
        // Remove specific elements efficiently
        comm_chain.remove_if([](const std::string& station) {
            return station.find("Outer") != std::string::npos;
        });
        
        std::cout << "\nAfter removing Outer Rim Station:\n";
        hop = 1;
        for (const auto& station : comm_chain) {
            std::cout << "Hop " << hop++ << ": " << station << "\n";
        }
        
        // Demonstrate forward_list's memory efficiency
        std::cout << "\nForward list is memory efficient for large datasets\n";
        std::cout << "with frequent front insertions and sequential access.\n";
    }
    
    /**
     * @brief Demonstrate std::array usage
     */
    static void demonstrateArray() {
        std::cout << "\n=== Array Demonstration ===\n";
        
        // Fixed-size sensor array
        std::array<double, 8> sensor_readings{};
        
        // Fill with random sensor data
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(0.0, 100.0);
        
        std::cout << "Sensor readings:\n";
        for (size_t i = 0; i < sensor_readings.size(); ++i) {
            sensor_readings[i] = dis(gen);
            std::cout << "Sensor " << i + 1 << ": " << std::fixed 
                     << std::setprecision(2) << sensor_readings[i] << "%\n";
        }
        
        // Array-specific operations
        std::cout << "\nArray statistics:\n";
        std::cout << "Size: " << sensor_readings.size() << "\n";
        std::cout << "Max size: " << sensor_readings.max_size() << "\n";
        std::cout << "Front sensor: " << sensor_readings.front() << "\n";
        std::cout << "Back sensor: " << sensor_readings.back() << "\n";
        
        // Algorithm usage with array
        auto minmax = std::minmax_element(sensor_readings.begin(), sensor_readings.end());
        std::cout << "Min reading: " << *minmax.first << "\n";
        std::cout << "Max reading: " << *minmax.second << "\n";
        
        double average = std::accumulate(sensor_readings.begin(), sensor_readings.end(), 0.0) 
                        / sensor_readings.size();
        std::cout << "Average reading: " << average << "\n";
        
        // Array can be used with C-style functions
        std::sort(sensor_readings.begin(), sensor_readings.end());
        std::cout << "\nSorted readings:\n";
        for (size_t i = 0; i < sensor_readings.size(); ++i) {
            std::cout << sensor_readings[i] << " ";
        }
        std::cout << "\n";
    }
};

/**
 * @brief Associative Container Demonstrations
 * 
 * Demonstrates map, multimap, set, multiset, unordered_map, 
 * unordered_multimap, unordered_set, unordered_multiset.
 */
class AssociativeContainers {
public:
    /**
     * @brief Demonstrate std::map usage
     */
    static void demonstrateMap() {
        std::cout << "\n=== Map Demonstration ===\n";
        
        // Fleet registry using map (sorted by key)
        std::map<std::string, Spacecraft> fleet_registry;
        
        // Insert spacecraft
        fleet_registry["ENT-001"] = {"Enterprise", "Heavy Cruiser", 5000.0, 400, 650.0, 1200.0};
        fleet_registry["MF-042"] = {"Millennium Falcon", "Light Freighter", 1050.0, 6, 1200.0, 400.0};
        fleet_registry["NOR-SR2"] = {"Normandy", "Frigate", 2500.0, 150, 900.0, 800.0};
        
        // Using emplace for efficiency
        fleet_registry.emplace("SER-001", Spacecraft{"Serenity", "Transport", 900.0, 9, 800.0, 200.0});
        
        std::cout << "Fleet Registry (sorted by call sign):\n";
        for (const auto& [call_sign, ship] : fleet_registry) {
            std::cout << call_sign << ": " << ship.name << " (" << ship.class_type << ")\n";
        }
        
        // Lookup operations
        std::cout << "\nLookup operations:\n";
        auto it = fleet_registry.find("MF-042");
        if (it != fleet_registry.end()) {
            std::cout << "Found: " << it->second.name << "\n";
        }
        
        // Count and contains (C++20)
        std::cout << "Registry contains ENT-001: " 
                 << (fleet_registry.count("ENT-001") > 0 ? "Yes" : "No") << "\n";
        
        // Range-based operations
        auto lower_bound = fleet_registry.lower_bound("M");
        auto upper_bound = fleet_registry.upper_bound("N");
        
        std::cout << "Ships with call signs M-N:\n";
        for (auto iter = lower_bound; iter != upper_bound; ++iter) {
            std::cout << "- " << iter->first << ": " << iter->second.name << "\n";
        }
        
        // Modify existing entry
        fleet_registry["ENT-001"].firepower += 200.0; // Upgrade
        std::cout << "Enterprise upgraded firepower: " << fleet_registry["ENT-001"].firepower << "\n";
    }
    
    /**
     * @brief Demonstrate std::multimap usage
     */
    static void demonstrateMultimap() {
        std::cout << "\n=== Multimap Demonstration ===\n";
        
        // Ships by class (multiple ships can have same class)
        std::multimap<std::string, std::string> ships_by_class;
        
        ships_by_class.emplace("Cruiser", "Enterprise");
        ships_by_class.emplace("Cruiser", "Excelsior");
        ships_by_class.emplace("Cruiser", "Constitution");
        ships_by_class.emplace("Fighter", "X-Wing");
        ships_by_class.emplace("Fighter", "TIE Fighter");
        ships_by_class.emplace("Fighter", "Viper");
        ships_by_class.emplace("Transport", "Serenity");
        ships_by_class.emplace("Transport", "Cargo Hauler");
        
        std::cout << "Ships by class:\n";
        for (const auto& [ship_class, ship_name] : ships_by_class) {
            std::cout << ship_class << ": " << ship_name << "\n";
        }
        
        // Find all ships of a specific class
        std::cout << "\nAll Cruisers:\n";
        auto range = ships_by_class.equal_range("Cruiser");
        for (auto iter = range.first; iter != range.second; ++iter) {
            std::cout << "- " << iter->second << "\n";
        }
        
        // Count ships by class
        std::cout << "\nClass statistics:\n";
        std::cout << "Cruisers: " << ships_by_class.count("Cruiser") << "\n";
        std::cout << "Fighters: " << ships_by_class.count("Fighter") << "\n";
        std::cout << "Transports: " << ships_by_class.count("Transport") << "\n";
    }
    
    /**
     * @brief Demonstrate std::set usage
     */
    static void demonstrateSet() {
        std::cout << "\n=== Set Demonstration ===\n";
        
        // Unique visited systems
        std::set<std::string> visited_systems;
        
        // Insert systems (duplicates automatically ignored)
        visited_systems.insert("Sol");
        visited_systems.insert("Alpha Centauri");
        visited_systems.insert("Proxima");
        visited_systems.insert("Vega");
        visited_systems.insert("Sol"); // Duplicate - will be ignored
        visited_systems.insert("Sirius");
        visited_systems.insert("Alpha Centauri"); // Another duplicate
        
        std::cout << "Unique systems visited (" << visited_systems.size() << "):\n";
        for (const auto& system : visited_systems) {
            std::cout << "- " << system << "\n";
        }
        
        // Set operations
        std::set<std::string> planned_systems{"Tau Ceti", "Wolf 359", "Barnard's Star", "Sol"};
        
        std::cout << "\nPlanned systems:\n";
        for (const auto& system : planned_systems) {
            std::cout << "- " << system << "\n";
        }
        
        // Find intersection (systems both visited and planned)
        std::set<std::string> intersection;
        std::set_intersection(
            visited_systems.begin(), visited_systems.end(),
            planned_systems.begin(), planned_systems.end(),
            std::inserter(intersection, intersection.begin())
        );
        
        std::cout << "\nSystems both visited and planned:\n";
        for (const auto& system : intersection) {
            std::cout << "- " << system << "\n";
        }
        
        // Find difference (planned but not visited)
        std::set<std::string> difference;
        std::set_difference(
            planned_systems.begin(), planned_systems.end(),
            visited_systems.begin(), visited_systems.end(),
            std::inserter(difference, difference.begin())
        );
        
        std::cout << "\nSystems planned but not yet visited:\n";
        for (const auto& system : difference) {
            std::cout << "- " << system << "\n";
        }
    }
    
    /**
     * @brief Demonstrate std::unordered_map usage
     */
    static void demonstrateUnorderedMap() {
        std::cout << "\n=== Unordered Map Demonstration ===\n";
        
        // Resource inventory (fast lookup)
        std::unordered_map<std::string, int> resource_inventory;
        
        resource_inventory["Deuterium"] = 1500;
        resource_inventory["Tritium"] = 800;
        resource_inventory["Dilithium"] = 50;
        resource_inventory["Iron Ore"] = 25000;
        resource_inventory["Platinum"] = 200;
        resource_inventory["Rare Earth Elements"] = 75;
        
        std::cout << "Current resource inventory:\n";
        for (const auto& [resource, quantity] : resource_inventory) {
            std::cout << resource << ": " << quantity << " units\n";
        }
        
        // Fast lookups (O(1) average case)
        std::cout << "\nResource lookup:\n";
        std::string resource_query = "Dilithium";
        if (resource_inventory.find(resource_query) != resource_inventory.end()) {
            std::cout << resource_query << " available: " << resource_inventory[resource_query] << " units\n";
        }
        
        // Update inventory
        resource_inventory["Deuterium"] -= 200; // Consumed
        resource_inventory["New Element"] = 10;  // Discovered
        
        // Check load factor and bucket information
        std::cout << "\nHash table statistics:\n";
        std::cout << "Size: " << resource_inventory.size() << "\n";
        std::cout << "Bucket count: " << resource_inventory.bucket_count() << "\n";
        std::cout << "Load factor: " << resource_inventory.load_factor() << "\n";
        std::cout << "Max load factor: " << resource_inventory.max_load_factor() << "\n";
        
        // Custom hash example with Spacecraft
        std::unordered_map<Spacecraft, std::string, SpacecraftHash> ship_status;
        ship_status[{"Enterprise", "Cruiser", 5000.0, 400, 650.0, 1200.0}] = "Active";
        ship_status[{"Voyager", "Explorer", 3500.0, 150, 800.0, 600.0}] = "Deep Space";
        
        std::cout << "\nShip status (using custom hash):\n";
        for (const auto& [ship, status] : ship_status) {
            std::cout << ship.name << ": " << status << "\n";
        }
    }
    
    /**
     * @brief Demonstrate std::unordered_set usage  
     */
    static void demonstrateUnorderedSet() {
        std::cout << "\n=== Unordered Set Demonstration ===\n";
        
        // Blacklisted systems (fast membership testing)
        std::unordered_set<std::string> blacklisted_systems{
            "Borg Space",
            "Klingon Neutral Zone", 
            "Romulan Territory",
            "Cardassian Space",
            "Dominion Controlled"
        };
        
        std::cout << "Blacklisted systems:\n";
        for (const auto& system : blacklisted_systems) {
            std::cout << "- " << system << "\n";
        }
        
        // Fast membership testing
        std::vector<std::string> navigation_requests{
            "Federation Space",
            "Vulcan System", 
            "Borg Space",
            "Andorian Territory",
            "Klingon Neutral Zone",
            "Earth System"
        };
        
        std::cout << "\nNavigation request validation:\n";
        for (const auto& system : navigation_requests) {
            bool is_safe = blacklisted_systems.find(system) == blacklisted_systems.end();
            std::cout << system << ": " << (is_safe ? "APPROVED" : "DENIED - RESTRICTED") << "\n";
        }
        
        // Set operations with unordered_set
        blacklisted_systems.insert("Unknown Nebula");
        blacklisted_systems.erase("Cardassian Space"); // Relations improved
        
        std::cout << "\nUpdated blacklist size: " << blacklisted_systems.size() << "\n";
        
        // Performance comparison note
        std::cout << "\nUnordered containers provide O(1) average case lookup\n";
        std::cout << "vs O(log n) for ordered containers, but lose ordering.\n";
    }
};

/**
 * @brief Container Adapter Demonstrations
 * 
 * Demonstrates stack, queue, and priority_queue usage.
 */
class ContainerAdapters {
public:
    /**
     * @brief Demonstrate std::stack usage
     */
    static void demonstrateStack() {
        std::cout << "\n=== Stack Demonstration ===\n";
        
        // Navigation history using stack (LIFO)
        std::stack<std::string> navigation_history;
        
        // Navigate through systems
        std::vector<std::string> journey{
            "Earth Orbit",
            "Mars Station", 
            "Asteroid Belt",
            "Jupiter Colony",
            "Saturn Rings",
            "Titan Base"
        };
        
        std::cout << "Navigation journey:\n";
        for (const auto& location : journey) {
            navigation_history.push(location);
            std::cout << "Arrived at: " << location << "\n";
        }
        
        std::cout << "\nStack size: " << navigation_history.size() << "\n";
        
        // Backtrack through journey (LIFO order)
        std::cout << "\nBacktracking:\n";
        while (!navigation_history.empty()) {
            std::cout << "Leaving: " << navigation_history.top() << "\n";
            navigation_history.pop();
        }
        
        // Function call stack simulation
        std::stack<std::string> call_stack;
        std::cout << "\nSimulating function call stack:\n";
        
        call_stack.push("main()");
        std::cout << "Call: " << call_stack.top() << "\n";
        
        call_stack.push("initializeFleet()");
        std::cout << "Call: " << call_stack.top() << "\n";
        
        call_stack.push("loadSpacecraft()");
        std::cout << "Call: " << call_stack.top() << "\n";
        
        call_stack.push("validateConfiguration()");
        std::cout << "Call: " << call_stack.top() << "\n";
        
        // Return from functions
        std::cout << "\nReturning from functions:\n";
        while (!call_stack.empty()) {
            std::cout << "Return from: " << call_stack.top() << "\n";
            call_stack.pop();
        }
    }
    
    /**
     * @brief Demonstrate std::queue usage
     */
    static void demonstrateQueue() {
        std::cout << "\n=== Queue Demonstration ===\n";
        
        // Mission queue using queue (FIFO)
        std::queue<std::string> mission_queue;
        
        // Add missions to queue
        mission_queue.push("Patrol Sector 7");
        mission_queue.push("Escort Convoy");
        mission_queue.push("Reconnaissance Mars");
        mission_queue.push("Resource Extraction");
        mission_queue.push("Station Resupply");
        
        std::cout << "Missions queued: " << mission_queue.size() << "\n";
        
        // Process missions in order (FIFO)
        std::cout << "\nProcessing missions in order:\n";
        int mission_count = 1;
        while (!mission_queue.empty()) {
            std::cout << "Mission " << mission_count++ << ": " 
                     << mission_queue.front() << " - STARTED\n";
            mission_queue.pop();
            
            // Simulate mission completion time
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            std::cout << "Mission " << (mission_count-1) << ": COMPLETED\n";
            
            // Add urgent mission mid-processing
            if (mission_queue.size() == 2) {
                std::cout << "*** Adding urgent mission to queue ***\n";
                mission_queue.push("URGENT: Distress Signal Response");
            }
        }
        
        // Producer-consumer simulation
        std::cout << "\nProducer-Consumer simulation:\n";
        std::queue<int> data_buffer;
        
        // Producer adds data
        std::cout << "Producer adding data: ";
        for (int i = 1; i <= 5; ++i) {
            data_buffer.push(i * 10);
            std::cout << i * 10 << " ";
        }
        std::cout << "\n";
        
        // Consumer processes data  
        std::cout << "Consumer processing data: ";
        while (!data_buffer.empty()) {
            std::cout << data_buffer.front() << " ";
            data_buffer.pop();
        }
        std::cout << "\n";
    }
    
    /**
     * @brief Demonstrate std::priority_queue usage
     */
    static void demonstratePriorityQueue() {
        std::cout << "\n=== Priority Queue Demonstration ===\n";
        
        // Emergency response system using priority queue
        // Higher numbers = higher priority (max heap by default)
        std::priority_queue<std::pair<int, std::string>> emergency_queue;
        
        // Add emergencies with priority levels
        emergency_queue.push({3, "Ship malfunction in outer sector"});
        emergency_queue.push({8, "Hull breach on space station"});
        emergency_queue.push({5, "Lost communication with colony"});
        emergency_queue.push({9, "Asteroid collision imminent"});
        emergency_queue.push({2, "Routine maintenance request"});
        emergency_queue.push({7, "Life support failure"});
        emergency_queue.push({1, "Supply delivery delayed"});
        
        std::cout << "Processing emergencies by priority (highest first):\n";
        int response_team = 1;
        while (!emergency_queue.empty()) {
            auto [priority, emergency] = emergency_queue.top();
            std::cout << "Team " << response_team++ << " responding to (Priority " 
                     << priority << "): " << emergency << "\n";
            emergency_queue.pop();
        }
        
        // Custom priority queue with spacecraft by combat rating
        auto combat_comparator = [](const Spacecraft& a, const Spacecraft& b) {
            return a.getCombatRating() < b.getCombatRating(); // Min heap for lowest rating first
        };
        
        std::priority_queue<Spacecraft, std::vector<Spacecraft>, decltype(combat_comparator)> 
            combat_queue(combat_comparator);
        
        combat_queue.push({"Enterprise", "Cruiser", 5000.0, 400, 650.0, 1200.0});
        combat_queue.push({"Fighter Alpha", "Fighter", 200.0, 1, 1200.0, 400.0});
        combat_queue.push({"Battleship Zeus", "Battleship", 12000.0, 800, 400.0, 3000.0});
        combat_queue.push({"Scout Beta", "Scout", 150.0, 1, 1500.0, 100.0});
        
        std::cout << "\nDeployment order (weakest first for support):\n";
        while (!combat_queue.empty()) {
            auto ship = combat_queue.top();
            std::cout << ship.name << " (Combat Rating: " 
                     << ship.getCombatRating() << ") deployed\n";
            combat_queue.pop();
        }
        
        // Min heap example using greater comparator
        std::priority_queue<int, std::vector<int>, std::greater<int>> min_heap;
        min_heap.push(30);
        min_heap.push(10);
        min_heap.push(50);
        min_heap.push(20);
        
        std::cout << "\nMin heap processing (smallest first): ";
        while (!min_heap.empty()) {
            std::cout << min_heap.top() << " ";
            min_heap.pop();
        }
        std::cout << "\n";
    }
};

/**
 * @brief Container Performance Analysis
 * 
 * Demonstrates performance characteristics of different containers.
 */
class ContainerPerformanceAnalysis {
public:
    /**
     * @brief Analyze insertion performance
     */
    static void analyzeInsertionPerformance() {
        std::cout << "\n=== Container Insertion Performance Analysis ===\n";
        
        const int test_size = 10000;
        
        // Vector push_back performance
        auto start = std::chrono::high_resolution_clock::now();
        std::vector<int> vec;
        vec.reserve(test_size); // Pre-allocate for fair comparison
        for (int i = 0; i < test_size; ++i) {
            vec.push_back(i);
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto vector_time = std::chrono::duration<double, std::milli>(end - start).count();
        
        // Deque push_back performance
        start = std::chrono::high_resolution_clock::now();
        std::deque<int> deq;
        for (int i = 0; i < test_size; ++i) {
            deq.push_back(i);
        }
        end = std::chrono::high_resolution_clock::now();
        auto deque_time = std::chrono::duration<double, std::milli>(end - start).count();
        
        // List push_back performance
        start = std::chrono::high_resolution_clock::now();
        std::list<int> lst;
        for (int i = 0; i < test_size; ++i) {
            lst.push_back(i);
        }
        end = std::chrono::high_resolution_clock::now();
        auto list_time = std::chrono::duration<double, std::milli>(end - start).count();
        
        std::cout << "Insertion of " << test_size << " elements (back insertion):\n";
        std::cout << "Vector (reserved): " << vector_time << " ms\n";
        std::cout << "Deque: " << deque_time << " ms\n"; 
        std::cout << "List: " << list_time << " ms\n";
    }
    
    /**
     * @brief Analyze lookup performance
     */
    static void analyzeLookupPerformance() {
        std::cout << "\n=== Container Lookup Performance Analysis ===\n";
        
        const int test_size = 100000;
        const int lookup_count = 1000;
        
        // Prepare data
        std::vector<int> search_keys;
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, test_size - 1);
        
        for (int i = 0; i < lookup_count; ++i) {
            search_keys.push_back(dis(gen));
        }
        
        // Vector with binary search (requires sorted data)
        std::vector<int> sorted_vec;
        for (int i = 0; i < test_size; ++i) {
            sorted_vec.push_back(i);
        }
        
        auto start = std::chrono::high_resolution_clock::now();
        int found_count = 0;
        for (int key : search_keys) {
            if (std::binary_search(sorted_vec.begin(), sorted_vec.end(), key)) {
                found_count++;
            }
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto vector_time = std::chrono::duration<double, std::milli>(end - start).count();
        
        // Set lookup
        std::set<int> int_set;
        for (int i = 0; i < test_size; ++i) {
            int_set.insert(i);
        }
        
        start = std::chrono::high_resolution_clock::now();
        found_count = 0;
        for (int key : search_keys) {
            if (int_set.find(key) != int_set.end()) {
                found_count++;
            }
        }
        end = std::chrono::high_resolution_clock::now();
        auto set_time = std::chrono::duration<double, std::milli>(end - start).count();
        
        // Unordered_set lookup
        std::unordered_set<int> int_uset;
        for (int i = 0; i < test_size; ++i) {
            int_uset.insert(i);
        }
        
        start = std::chrono::high_resolution_clock::now();
        found_count = 0;
        for (int key : search_keys) {
            if (int_uset.find(key) != int_uset.end()) {
                found_count++;
            }
        }
        end = std::chrono::high_resolution_clock::now();
        auto uset_time = std::chrono::duration<double, std::milli>(end - start).count();
        
        std::cout << lookup_count << " lookups in containers of size " << test_size << ":\n";
        std::cout << "Vector (binary_search): " << vector_time << " ms\n";
        std::cout << "Set (find): " << set_time << " ms\n";
        std::cout << "Unordered_set (find): " << uset_time << " ms\n";
    }
};

} // namespace CppVerseHub::STL