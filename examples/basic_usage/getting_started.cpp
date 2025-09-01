// File: examples/basic_usage/getting_started.cpp
// CppVerseHub Getting Started Example
// Basic demonstration of core library functionality

#include <iostream>
#include <vector>
#include <memory>
#include <string>
#include <chrono>
#include <thread>

// Core CppVerseHub includes
#include "Entity.hpp"
#include "Planet.hpp"
#include "Fleet.hpp"
#include "Mission.hpp"
#include "ResourceManager.hpp"
#include "Logger.hpp"

using namespace std;

/**
 * @brief Demonstrates basic CppVerseHub usage patterns
 * 
 * This example shows:
 * - Creating entities (planets and fleets)
 * - Basic resource management
 * - Simple mission execution
 * - Logging system usage
 */
void demonstrateBasicUsage() {
    cout << "=== CppVerseHub Getting Started Example ===" << endl;
    
    // Initialize logging system
    auto& logger = Logger::getInstance();
    logger.setLevel(Logger::Level::INFO);
    logger.enableConsoleLogging(true);
    
    LOG_INFO("Starting CppVerseHub demonstration");
    
    try {
        // 1. Create some planets
        cout << "\n1. Creating Planets..." << endl;
        
        auto earth = make_unique<Planet>("Earth", Vector3D{0, 0, 0}, PlanetType::TERRESTRIAL);
        earth->addResources("minerals", 1000);
        earth->addResources("energy", 500);
        earth->setPopulation(1000000);
        
        auto mars = make_unique<Planet>("Mars", Vector3D{100, 50, 25}, PlanetType::TERRESTRIAL);
        mars->addResources("minerals", 750);
        mars->addResources("energy", 200);
        
        cout << "Created planets: " << earth->getName() << " and " << mars->getName() << endl;
        cout << "Distance between planets: " << earth->distanceTo(*mars) << " units" << endl;
        
        // 2. Create a fleet
        cout << "\n2. Creating Fleet..." << endl;
        
        vector<Ship> ships = {
            {"Explorer-1", ShipType::SCOUT, 100},
            {"Explorer-2", ShipType::SCOUT, 100},
            {"Carrier-1", ShipType::TRANSPORT, 500}
        };
        
        auto explorationFleet = make_unique<Fleet>("Alpha Squadron", 
                                                   earth->getPosition(), 
                                                   ships);
        
        cout << "Created fleet: " << explorationFleet->getName() << endl;
        cout << "Fleet strength: " << explorationFleet->getFleetStrength() << endl;
        cout << "Fleet capacity: " << explorationFleet->getTotalCapacity() << endl;
        
        // 3. Load cargo onto fleet
        cout << "\n3. Loading Fleet Cargo..." << endl;
        
        map<string, int> cargo = {
            {"supplies", 100},
            {"equipment", 50}
        };
        
        if (explorationFleet->loadCargo(cargo)) {
            cout << "Successfully loaded cargo onto fleet" << endl;
        } else {
            cout << "Failed to load cargo - insufficient capacity" << endl;
        }
        
        // 4. Create and execute a mission
        cout << "\n4. Creating Exploration Mission..." << endl;
        
        auto mission = make_unique<ExplorationMission>(
            explorationFleet.get(),
            mars.get(),
            "Survey Martian resources"
        );
        
        cout << "Mission created: " << mission->getDescription() << endl;
        cout << "Mission target: " << mission->getTarget()->getName() << endl;
        
        // Execute mission (this will be asynchronous in real usage)
        cout << "Executing mission..." << endl;
        auto result = mission->execute();
        
        if (result.success) {
            cout << "Mission completed successfully!" << endl;
            cout << "Mission report: " << result.report << endl;
            
            // Display discovered resources
            if (!result.discoveredResources.empty()) {
                cout << "Discovered resources:" << endl;
                for (const auto& [resource, amount] : result.discoveredResources) {
                    cout << "  - " << resource << ": " << amount << " units" << endl;
                }
            }
        } else {
            cout << "Mission failed: " << result.report << endl;
        }
        
        // 5. Resource management demonstration
        cout << "\n5. Resource Management..." << endl;
        
        auto& resourceManager = ResourceManager::getInstance();
        
        // Register planets with resource manager
        resourceManager.registerEntity(earth.get());
        resourceManager.registerEntity(mars.get());
        
        // Display global resource summary
        cout << "Global Resource Summary:" << endl;
        auto globalResources = resourceManager.getTotalResources();
        for (const auto& [resource, amount] : globalResources) {
            cout << "  " << resource << ": " << amount << " units" << endl;
        }
        
        // 6. Demonstrate distance calculations
        cout << "\n6. Spatial Calculations..." << endl;
        
        // Move fleet towards Mars
        Vector3D midpoint{50, 25, 12.5};  // Halfway to Mars
        explorationFleet->setPosition(midpoint);
        
        cout << "Fleet moved to position: " << explorationFleet->getPosition().toString() << endl;
        cout << "Distance to Earth: " << explorationFleet->distanceTo(*earth) << " units" << endl;
        cout << "Distance to Mars: " << explorationFleet->distanceTo(*mars) << " units" << endl;
        
        // 7. Demonstrate entity polymorphism
        cout << "\n7. Polymorphic Entity Handling..." << endl;
        
        vector<unique_ptr<Entity>> entities;
        entities.push_back(make_unique<Planet>(*earth));  // Copy constructor
        entities.push_back(make_unique<Fleet>(*explorationFleet));
        
        cout << "Entity inventory:" << endl;
        for (const auto& entity : entities) {
            cout << "  Entity ID: " << entity->getId() 
                 << ", Position: " << entity->getPosition().toString() 
                 << ", Type: " << entity->toString() << endl;
        }
        
        LOG_INFO("Basic demonstration completed successfully");
        
    } catch (const exception& e) {
        LOG_ERROR("Exception during demonstration: {}", e.what());
        cerr << "Error: " << e.what() << endl;
        return;
    }
    
    cout << "\n=== Demonstration Complete ===" << endl;
    cout << "This example showed basic CppVerseHub functionality." << endl;
    cout << "Check out advanced_usage/ for more complex examples." << endl;
}

/**
 * @brief Simple performance timing demonstration
 */
void demonstratePerformance() {
    cout << "\n=== Performance Demonstration ===" << endl;
    
    auto start = chrono::high_resolution_clock::now();
    
    // Create many entities to test performance
    vector<unique_ptr<Planet>> planets;
    for (int i = 0; i < 1000; ++i) {
        auto planet = make_unique<Planet>(
            "Planet-" + to_string(i),
            Vector3D{static_cast<double>(i * 10), 0, 0},
            PlanetType::TERRESTRIAL
        );
        planet->addResources("minerals", 100 + i);
        planets.push_back(move(planet));
    }
    
    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::microseconds>(end - start);
    
    cout << "Created 1000 planets in " << duration.count() << " microseconds" << endl;
    cout << "Average: " << duration.count() / 1000.0 << " microseconds per planet" << endl;
    
    // Test distance calculations
    start = chrono::high_resolution_clock::now();
    
    double totalDistance = 0.0;
    for (size_t i = 1; i < planets.size(); ++i) {
        totalDistance += planets[0]->distanceTo(*planets[i]);
    }
    
    end = chrono::high_resolution_clock::now();
    duration = chrono::duration_cast<chrono::microseconds>(end - start);
    
    cout << "Calculated " << planets.size() - 1 << " distances in " 
         << duration.count() << " microseconds" << endl;
    cout << "Average distance calculation: " 
         << duration.count() / (planets.size() - 1) << " microseconds" << endl;
    cout << "Total distance calculated: " << totalDistance << " units" << endl;
}

/**
 * @brief Main function - entry point for getting started example
 */
int main() {
    cout << "CppVerseHub - Getting Started Example" << endl;
    cout << "====================================" << endl;
    
    try {
        // Run basic functionality demonstration
        demonstrateBasicUsage();
        
        // Show some performance characteristics
        demonstratePerformance();
        
        cout << "\nExample completed successfully!" << endl;
        cout << "Next steps:" << endl;
        cout << "  - Try examples/basic_usage/entity_creation.cpp" << endl;
        cout << "  - Try examples/basic_usage/simple_simulation.cpp" << endl;
        cout << "  - Explore examples/advanced_usage/ for complex scenarios" << endl;
        
        return 0;
        
    } catch (const exception& e) {
        cerr << "Fatal error in getting started example: " << e.what() << endl;
        return 1;
    } catch (...) {
        cerr << "Unknown fatal error occurred" << endl;
        return 2;
    }
}