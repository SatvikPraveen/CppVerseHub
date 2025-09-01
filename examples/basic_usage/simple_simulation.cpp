// File: examples/basic_usage/simple_simulation.cpp
// CppVerseHub Simple Simulation Example
// Demonstrates a complete simulation loop with entities, missions, and resource management

#include <iostream>
#include <vector>
#include <memory>
#include <chrono>
#include <thread>
#include <random>
#include <iomanip>

// Core system includes
#include "GameEngine.hpp"
#include "Entity.hpp"
#include "Planet.hpp"
#include "Fleet.hpp"
#include "Mission.hpp"
#include "ResourceManager.hpp"
#include "Logger.hpp"

using namespace std;
using namespace chrono;

/**
 * @brief Simple space exploration simulation
 * 
 * This simulation demonstrates:
 * - Creating a solar system with planets and fleets
 * - Running exploration missions
 * - Resource generation and management
 * - Time-based simulation updates
 * - Basic game loop mechanics
 */
class SimpleSpaceSimulation {
private:
    unique_ptr<GameEngine> gameEngine;
    vector<unique_ptr<Planet>> planets;
    vector<unique_ptr<Fleet>> fleets;
    vector<unique_ptr<Mission>> activeMissions;
    
    // Simulation parameters
    int simulationTicks = 0;
    int maxTicks = 100;
    chrono::milliseconds tickDuration{100};  // 100ms per tick
    
    // Random number generation
    mutable mt19937 rng{random_device{}()};
    
public:
    SimpleSpaceSimulation() {
        gameEngine = make_unique<GameEngine>();
        LOG_INFO("Simple simulation initialized");
    }
    
    /**
     * @brief Initialize the simulation world
     */
    void initializeWorld() {
        cout << "\n=== Initializing Simulation World ===" << endl;
        
        // Create home planet (Earth analog)
        auto homeWorld = make_unique<Planet>(
            "Terra", 
            Vector3D{0, 0, 0}, 
            PlanetType::TERRESTRIAL
        );
        homeWorld->addResources("minerals", 5000);
        homeWorld->addResources("energy", 3000);
        homeWorld->addResources("food", 2000);
        homeWorld->setPopulation(1000000);
        homeWorld->setHabitability(0.95);
        
        cout << "Created home world: " << homeWorld->toString() << endl;
        
        // Create distant planets to explore
        auto create_planet = [this](const string& name, Vector3D pos, PlanetType type) {
            auto planet = make_unique<Planet>(name, pos, type);
            
            // Add random resources
            uniform_int_distribution<int> resourceDist(100, 1000);
            planet->addResources("minerals", resourceDist(rng));
            planet->addResources("energy", resourceDist(rng));
            
            if (type == PlanetType::TERRESTRIAL) {
                planet->addResources("food", resourceDist(rng) / 2);
                planet->setHabitability(uniform_real_distribution<double>{0.3, 0.9}(rng));
            } else {
                planet->setHabitability(uniform_real_distribution<double>{0.1, 0.4}(rng));
            }
            
            cout << "Created planet: " << planet->toString() << endl;
            return planet;
        };
        
        planets.push_back(move(homeWorld));
        planets.push_back(create_planet("Alpha Centauri B", Vector3D{100, 50, 25}, PlanetType::TERRESTRIAL));
        planets.push_back(create_planet("Kepler-442b", Vector3D{150, -75, 100}, PlanetType::OCEANIC));
        planets.push_back(create_planet("HD 40307g", Vector3D{-80, 120, 60}, PlanetType::TERRESTRIAL));
        planets.push_back(create_planet("Titan", Vector3D{200, 0, -50}, PlanetType::FROZEN));
        planets.push_back(create_planet("Jupiter-II", Vector3D{-150, -100, 75}, PlanetType::GAS_GIANT));
        
        // Create exploration fleets
        auto createFleet = [](const string& name, Vector3D pos, const vector<Ship>& ships) {
            return make_unique<Fleet>(name, pos, ships);
        };
        
        // Scientific exploration fleet
        fleets.push_back(createFleet(
            "Magellan Explorer",
            Vector3D{0, 0, 0},  // Start at home world
            {
                {"Explorer Alpha", ShipType::SCOUT, 200},
                {"Science Lab", ShipType::SCIENCE, 300},
                {"Supply Vessel", ShipType::TRANSPORT, 400}
            }
        ));
        
        // Long-range colonization fleet
        fleets.push_back(createFleet(
            "Pioneer Convoy",
            Vector3D{0, 0, 0},
            {
                {"Colony Ship", ShipType::TRANSPORT, 1000},
                {"Construction Ship", ShipType::TRANSPORT, 800},
                {"Defense Escort", ShipType::COMBAT, 500},
                {"Scout Runner", ShipType::SCOUT, 150}
            }
        ));
        
        cout << "Created " << planets.size() << " planets and " << fleets.size() << " fleets" << endl;
        
        // Register all entities with game engine
        for (auto& planet : planets) {
            gameEngine->addEntity(planet.get());
        }
        
        for (auto& fleet : fleets) {
            gameEngine->addEntity(fleet.get());
        }
        
        LOG_INFO("World initialization complete");
    }
    
    /**
     * @brief Create exploration missions
     */
    void createMissions() {
        cout << "\n=== Creating Exploration Missions ===" << endl;
        
        if (fleets.empty() || planets.size() <= 1) {
            cout << "Not enough entities to create missions" << endl;
            return;
        }
        
        // Create mission to Alpha Centauri
        if (planets.size() > 1 && !fleets.empty()) {
            auto mission1 = make_unique<ExplorationMission>(
                fleets[0].get(),
                planets[1].get(),  // Alpha Centauri B
                "Survey Alpha Centauri system for habitable conditions"
            );
            
            cout << "Created mission: " << mission1->getDescription() << endl;
            cout << "  Fleet: " << mission1->getAssignedFleet()->getName() << endl;
            cout << "  Target: " << mission1->getTarget()->getName() << endl;
            
            activeMissions.push_back(move(mission1));
        }
        
        // Create colonization mission to Kepler-442b
        if (planets.size() > 2 && fleets.size() > 1) {
            auto mission2 = make_unique<ColonizationMission>(
                fleets[1].get(),
                planets[2].get(),  // Kepler-442b
                "Establish first colony on Kepler-442b",
                1000  // Colony population
            );
            
            cout << "Created mission: " << mission2->getDescription() << endl;
            cout << "  Fleet: " << mission2->getAssignedFleet()->getName() << endl;
            cout << "  Target: " << mission2->getTarget()->getName() << endl;
            
            activeMissions.push_back(move(mission2));
        }
        
        LOG_INFO("Created {} missions", activeMissions.size());
    }
    
    /**
     * @brief Update simulation state for one tick
     */
    void updateSimulation() {
        simulationTicks++;
        
        // Update resource generation on planets
        for (auto& planet : planets) {
            planet->generateResources();
        }
        
        // Process active missions
        auto it = activeMissions.begin();
        while (it != activeMissions.end()) {
            auto& mission = *it;
            
            if (mission->isComplete()) {
                cout << "\nMission completed: " << mission->getDescription() << endl;
                auto result = mission->getResult();
                cout << "Result: " << (result.success ? "SUCCESS" : "FAILURE") << endl;
                cout << "Report: " << result.report << endl;
                
                if (!result.discoveredResources.empty()) {
                    cout << "Discovered resources:" << endl;
                    for (const auto& [resource, amount] : result.discoveredResources) {
                        cout << "  " << resource << ": " << amount << endl;
                    }
                }
                
                it = activeMissions.erase(it);
            } else {
                // Update mission progress
                mission->update();
                ++it;
            }
        }
        
        // Every 10 ticks, show simulation status
        if (simulationTicks % 10 == 0) {
            showSimulationStatus();
        }
        
        // Create new missions occasionally
        if (simulationTicks % 30 == 0 && activeMissions.empty() && simulationTicks < maxTicks - 20) {
            createRandomMission();
        }
    }
    
    /**
     * @brief Create a random exploration mission
     */
    void createRandomMission() {
        if (fleets.empty() || planets.size() <= 1) return;
        
        // Pick random fleet and planet
        uniform_int_distribution<size_t> fleetDist(0, fleets.size() - 1);
        uniform_int_distribution<size_t> planetDist(1, planets.size() - 1);  // Skip home world
        
        auto selectedFleet = fleets[fleetDist(rng)].get();
        auto selectedPlanet = planets[planetDist(rng)].get();
        
        auto mission = make_unique<ExplorationMission>(
            selectedFleet,
            selectedPlanet,
            "Automated survey mission to " + selectedPlanet->getName()
        );
        
        cout << "\nCreated random mission: " << mission->getDescription() << endl;
        activeMissions.push_back(move(mission));
    }
    
    /**
     * @brief Display current simulation status
     */
    void showSimulationStatus() {
        cout << "\n--- Simulation Status (Tick " << simulationTicks << ") ---" << endl;
        
        // Show resource summary
        auto& resourceManager = ResourceManager::getInstance();
        auto totalResources = resourceManager.getTotalResources();
        
        cout << "Global Resources:" << endl;
        for (const auto& [resource, amount] : totalResources) {
            cout << "  " << resource << ": " << amount << " units" << endl;
        }
        
        // Show active missions
        cout << "Active Missions: " << activeMissions.size() << endl;
        for (const auto& mission : activeMissions) {
            cout << "  - " << mission->getDescription() 
                 << " (Progress: " << fixed << setprecision(1) 
                 << mission->getProgress() * 100 << "%)" << endl;
        }
        
        // Show fleet positions
        cout << "Fleet Positions:" << endl;
        for (const auto& fleet : fleets) {
            cout << "  " << fleet->getName() 
                 << " at " << fleet->getPosition().toString() << endl;
        }
    }
    
    /**
     * @brief Run the complete simulation
     */
    void runSimulation() {
        cout << "\n=== Starting Simple Space Simulation ===" << endl;
        cout << "Simulation will run for " << maxTicks << " ticks" << endl;
        cout << "Each tick = " << tickDuration.count() << "ms" << endl;
        
        auto startTime = steady_clock::now();
        
        while (simulationTicks < maxTicks) {
            updateSimulation();
            
            // Sleep for realistic timing
            this_thread::sleep_for(tickDuration);
            
            // Check for early termination conditions
            if (activeMissions.empty() && simulationTicks > maxTicks / 2) {
                cout << "\nNo more active missions - ending simulation early" << endl;
                break;
            }
        }
        
        auto endTime = steady_clock::now();
        auto totalTime = duration_cast<milliseconds>(endTime - startTime);
        
        cout << "\n=== Simulation Complete ===" << endl;
        cout << "Total ticks: " << simulationTicks << endl;
        cout << "Real time elapsed: " << totalTime.count() << "ms" << endl;
        cout << "Simulation time: " << simulationTicks * tickDuration.count() << "ms" << endl;
        
        // Final status report
        showFinalReport();
        
        LOG_INFO("Simple simulation completed successfully");
    }
    
    /**
     * @brief Generate final simulation report
     */
    void showFinalReport() {
        cout << "\n=== Final Simulation Report ===" << endl;
        
        // Resource analysis
        auto& resourceManager = ResourceManager::getInstance();
        auto finalResources = resourceManager.getTotalResources();
        
        cout << "Final Resource Totals:" << endl;
        for (const auto& [resource, amount] : finalResources) {
            cout << "  " << resource << ": " << amount << " units" << endl;
        }
        
        // Planet analysis
        cout << "\nPlanet Status:" << endl;
        for (const auto& planet : planets) {
            cout << "  " << planet->getName() 
                 << " - Pop: " << planet->getPopulation()
                 << ", Habitability: " << fixed << setprecision(2) 
                 << planet->getHabitability() << endl;
        }
        
        // Fleet analysis
        cout << "\nFleet Status:" << endl;
        for (const auto& fleet : fleets) {
            cout << "  " << fleet->getName()
                 << " - Ships: " << fleet->getShipCount()
                 << ", Strength: " << fleet->getFleetStrength()
                 << ", Position: " << fleet->getPosition().toString() << endl;
        }
        
        // Performance metrics
        double avgTickTime = tickDuration.count();
        double ticksPerSecond = 1000.0 / avgTickTime;
        
        cout << "\nPerformance Metrics:" << endl;
        cout << "  Ticks per second: " << fixed << setprecision(1) << ticksPerSecond << endl;
        cout << "  Entities managed: " << (planets.size() + fleets.size()) << endl;
        cout << "  Peak active missions: " << activeMissions.size() << endl;  // This shows current, not peak
        
        cout << "\nSimulation completed successfully!" << endl;
    }
};

/**
 * @brief Main function - runs the complete simulation
 */
int main() {
    cout << "CppVerseHub - Simple Space Simulation" << endl;
    cout << "====================================" << endl;
    
    // Initialize logging system
    auto& logger = Logger::getInstance();
    logger.setLevel(Logger::Level::INFO);
    logger.enableConsoleLogging(true);
    
    try {
        // Create and run simulation
        SimpleSpaceSimulation simulation;
        
        // Initialize the simulation world
        simulation.initializeWorld();
        
        // Create initial missions
        simulation.createMissions();
        
        cout << "\nPress Enter to start simulation...";
        cin.get();
        
        // Run the main simulation loop
        simulation.runSimulation();
        
        cout << "\nSimulation complete! Check the logs for detailed information." << endl;
        cout << "\nNext steps:" << endl;
        cout << "  - Try examples/advanced_usage/ for more complex simulations" << endl;
        cout << "  - Explore examples/tutorials/ for learning-focused examples" << endl;
        
        return 0;
        
    } catch (const exception& e) {
        LOG_ERROR("Error in simple simulation: {}", e.what());
        cerr << "Simulation error: " << e.what() << endl;
        return 1;
    } catch (...) {
        LOG_ERROR("Unknown error in simple simulation");
        cerr << "Unknown simulation error occurred" << endl;
        return 2;
    }
}