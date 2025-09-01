// File: examples/basic_usage/entity_creation.cpp
// CppVerseHub Entity Creation Patterns Example
// Comprehensive demonstration of entity creation, configuration, and management

#include <iostream>
#include <memory>
#include <vector>
#include <random>
#include <algorithm>
#include <iomanip>

// Core includes
#include "Entity.hpp"
#include "Planet.hpp"
#include "Fleet.hpp"
#include "Factory.hpp"
#include "Builder.hpp"
#include "Logger.hpp"

using namespace std;

/**
 * @brief Demonstrates basic entity creation patterns
 */
class EntityCreationDemo {
private:
    static mt19937 rng;
    static uniform_real_distribution<double> positionDist;
    static uniform_int_distribution<int> resourceDist;
    
public:
    /**
     * @brief Create planets using direct construction
     */
    static vector<unique_ptr<Planet>> createPlanetsDirectly() {
        cout << "\n=== Direct Planet Creation ===" << endl;
        
        vector<unique_ptr<Planet>> planets;
        
        // Create Earth-like planet
        auto earth = make_unique<Planet>(
            "Terra Prime", 
            Vector3D{0.0, 0.0, 0.0}, 
            PlanetType::TERRESTRIAL
        );
        
        // Configure Earth
        earth->addResources("minerals", 2000);
        earth->addResources("energy", 1500);
        earth->addResources("food", 1000);
        earth->setPopulation(2000000);
        earth->setHabitability(0.95);
        
        cout << "Created: " << earth->toString() << endl;
        planets.push_back(move(earth));
        
        // Create Gas Giant
        auto gasGiant = make_unique<Planet>(
            "Jovian Alpha",
            Vector3D{150.0, 75.0, 25.0},
            PlanetType::GAS_GIANT
        );
        
        gasGiant->addResources("energy", 5000);  // Rich in energy
        gasGiant->addResources("rare_elements", 100);
        gasGiant->setHabitability(0.1);  // Not very habitable
        
        cout << "Created: " << gasGiant->toString() << endl;
        planets.push_back(move(gasGiant));
        
        // Create Desert Planet
        auto desert = make_unique<Planet>(
            "Aridus",
            Vector3D{75.0, -50.0, 100.0},
            PlanetType::DESERT
        );
        
        desert->addResources("minerals", 3000);  // Rich in minerals
        desert->addResources("energy", 200);     // Low energy
        desert->setHabitability(0.3);
        
        cout << "Created: " << desert->toString() << endl;
        planets.push_back(move(desert));
        
        return planets;
    }
    
    /**
     * @brief Create planets using factory pattern
     */
    static vector<unique_ptr<Planet>> createPlanetsWithFactory() {
        cout << "\n=== Factory-Based Planet Creation ===" << endl;
        
        vector<unique_ptr<Planet>> planets;
        auto& factory = PlanetFactory::getInstance();
        
        // Create planets with predefined templates
        auto oceanic = factory.createPlanet("oceanic", "Aquaterra", Vector3D{200, 100, 50});
        if (oceanic) {
            cout << "Factory created: " << oceanic->toString() << endl;
            planets.push_back(move(oceanic));
        }
        
        auto volcanic = factory.createPlanet("volcanic", "Infernus", Vector3D{-100, 200, -75});
        if (volcanic) {
            cout << "Factory created: " << volcanic->toString() << endl;
            planets.push_back(move(volcanic));
        }
        
        auto frozen = factory.createPlanet("frozen", "Glacialis", Vector3D{300, -150, 200});
        if (frozen) {
            cout << "Factory created: " << frozen->toString() << endl;
            planets.push_back(move(frozen));
        }
        
        return planets;
    }
    
    /**
     * @brief Create fleets using builder pattern
     */
    static vector<unique_ptr<Fleet>> createFleetsWithBuilder() {
        cout << "\n=== Builder-Based Fleet Creation ===" << endl;
        
        vector<unique_ptr<Fleet>> fleets;
        
        // Build exploration fleet
        auto explorationFleet = FleetBuilder()
            .setName("Deep Space Explorers")
            .setPosition(Vector3D{10, 10, 10})
            .addShip("Scout Alpha", ShipType::SCOUT, 150)
            .addShip("Scout Beta", ShipType::SCOUT, 150)
            .addShip("Science Vessel", ShipType::SCIENCE, 300)
            .addShip("Supply Ship", ShipType::TRANSPORT, 500)
            .setHomeBase("Terra Prime")
            .build();
        
        if (explorationFleet) {
            cout << "Built exploration fleet: " << explorationFleet->getName() << endl;
            cout << "  Ships: " << explorationFleet->getShipCount() << endl;
            cout << "  Strength: " << explorationFleet->getFleetStrength() << endl;
            cout << "  Capacity: " << explorationFleet->getTotalCapacity() << endl;
            fleets.push_back(move(explorationFleet));
        }
        
        // Build combat fleet
        auto combatFleet = FleetBuilder()
            .setName("Aegis Defense Force")
            .setPosition(Vector3D{0, 0, 0})  // Near Terra Prime
            .addShip("Destroyer Alpha", ShipType::COMBAT, 800)
            .addShip("Destroyer Beta", ShipType::COMBAT, 800)
            .addShip("Frigate One", ShipType::COMBAT, 400)
            .addShip("Frigate Two", ShipType::COMBAT, 400)
            .addShip("Command Ship", ShipType::COMMAND, 600)
            .setHomeBase("Terra Prime")
            .build();
        
        if (combatFleet) {
            cout << "Built combat fleet: " << combatFleet->getName() << endl;
            cout << "  Ships: " << combatFleet->getShipCount() << endl;
            cout << "  Strength: " << combatFleet->getFleetStrength() << endl;
            fleets.push_back(move(combatFleet));
        }
        
        // Build trade fleet
        auto tradeFleet = FleetBuilder()
            .setName("Merchant Convoy")
            .setPosition(Vector3D{50, 25, 0})
            .addShip("Freighter Alpha", ShipType::TRANSPORT, 1000)
            .addShip("Freighter Beta", ShipType::TRANSPORT, 1000)
            .addShip("Freighter Gamma", ShipType::TRANSPORT, 1200)
            .addShip("Escort Vessel", ShipType::COMBAT, 300)
            .setHomeBase("Terra Prime")
            .build();
        
        if (tradeFleet) {
            cout << "Built trade fleet: " << tradeFleet->getName() << endl;
            cout << "  Cargo capacity: " << tradeFleet->getTotalCapacity() << endl;
            fleets.push_back(move(tradeFleet));
        }
        
        return fleets;
    }
    
    /**
     * @brief Create random entities for testing
     */
    static vector<unique_ptr<Entity>> createRandomEntities(size_t count) {
        cout << "\n=== Random Entity Generation ===" << endl;
        cout << "Generating " << count << " random entities..." << endl;
        
        vector<unique_ptr<Entity>> entities;
        entities.reserve(count);
        
        for (size_t i = 0; i < count; ++i) {
            // Random position
            Vector3D position{
                positionDist(rng),
                positionDist(rng), 
                positionDist(rng)
            };
            
            if (i % 3 == 0) {
                // Create random planet
                string name = "Planet-" + to_string(i);
                auto planetType = static_cast<PlanetType>(i % 4);  // Cycle through types
                
                auto planet = make_unique<Planet>(name, position, planetType);
                planet->addResources("minerals", resourceDist(rng));
                planet->addResources("energy", resourceDist(rng));
                planet->setHabitability(uniform_real_distribution<double>{0.1, 1.0}(rng));
                
                entities.push_back(move(planet));
                
            } else {
                // Create random fleet
                string name = "Fleet-" + to_string(i);
                vector<Ship> ships;
                
                int shipCount = uniform_int_distribution<int>{1, 5}(rng);
                for (int j = 0; j < shipCount; ++j) {
                    ships.emplace_back(
                        "Ship-" + to_string(j),
                        static_cast<ShipType>(j % 4),
                        uniform_int_distribution<int>{100, 800}(rng)
                    );
                }
                
                auto fleet = make_unique<Fleet>(name, position, ships);
                entities.push_back(move(fleet));
            }
        }
        
        cout << "Generated " << entities.size() << " entities" << endl;
        return entities;
    }
    
    /**
     * @brief Analyze created entities
     */
    static void analyzeEntities(const vector<unique_ptr<Entity>>& entities) {
        cout << "\n=== Entity Analysis ===" << endl;
        
        size_t planetCount = 0;
        size_t fleetCount = 0;
        double totalDistance = 0.0;
        Vector3D centerOfMass{0, 0, 0};
        
        // Count types and calculate center of mass
        for (const auto& entity : entities) {
            if (dynamic_cast<Planet*>(entity.get())) {
                planetCount++;
            } else if (dynamic_cast<Fleet*>(entity.get())) {
                fleetCount++;
            }
            
            auto pos = entity->getPosition();
            centerOfMass.x += pos.x;
            centerOfMass.y += pos.y;
            centerOfMass.z += pos.z;
        }
        
        // Calculate center of mass
        if (!entities.empty()) {
            centerOfMass.x /= entities.size();
            centerOfMass.y /= entities.size();
            centerOfMass.z /= entities.size();
        }
        
        // Calculate average distance from center
        for (const auto& entity : entities) {
            totalDistance += entity->getPosition().distanceTo(centerOfMass);
        }
        double avgDistance = entities.empty() ? 0.0 : totalDistance / entities.size();
        
        // Find furthest entities
        double maxDistance = 0.0;
        pair<Entity*, Entity*> furthestPair{nullptr, nullptr};
        
        for (size_t i = 0; i < entities.size(); ++i) {
            for (size_t j = i + 1; j < entities.size(); ++j) {
                double distance = entities[i]->distanceTo(*entities[j]);
                if (distance > maxDistance) {
                    maxDistance = distance;
                    furthestPair = {entities[i].get(), entities[j].get()};
                }
            }
        }
        
        // Display analysis results
        cout << fixed << setprecision(2);
        cout << "Total entities: " << entities.size() << endl;
        cout << "Planets: " << planetCount << " (" 
             << (100.0 * planetCount / entities.size()) << "%)" << endl;
        cout << "Fleets: " << fleetCount << " (" 
             << (100.0 * fleetCount / entities.size()) << "%)" << endl;
        cout << "Center of mass: " << centerOfMass.toString() << endl;
        cout << "Average distance from center: " << avgDistance << " units" << endl;
        
        if (furthestPair.first && furthestPair.second) {
            cout << "Furthest apart entities:" << endl;
            cout << "  " << furthestPair.first->toString() << endl;
            cout << "  " << furthestPair.second->toString() << endl;
            cout << "  Distance: " << maxDistance << " units" << endl;
        }
    }
};

// Static member initialization
mt19937 EntityCreationDemo::rng{random_device{}()};
uniform_real_distribution<double> EntityCreationDemo::positionDist{-500.0, 500.0};
uniform_int_distribution<int> EntityCreationDemo::resourceDist{100, 2000};

/**
 * @brief Main function demonstrating various entity creation patterns
 */
int main() {
    cout << "CppVerseHub - Entity Creation Patterns" << endl;
    cout << "=====================================" << endl;
    
    // Initialize logging
    auto& logger = Logger::getInstance();
    logger.setLevel(Logger::Level::INFO);
    logger.enableConsoleLogging(true);
    
    try {
        vector<unique_ptr<Entity>> allEntities;
        
        // 1. Direct creation
        auto directPlanets = EntityCreationDemo::createPlanetsDirectly();
        for (auto& planet : directPlanets) {
            allEntities.push_back(move(planet));
        }
        
        // 2. Factory-based creation
        auto factoryPlanets = EntityCreationDemo::createPlanetsWithFactory();
        for (auto& planet : factoryPlanets) {
            allEntities.push_back(move(planet));
        }
        
        // 3. Builder-based creation
        auto builderFleets = EntityCreationDemo::createFleetsWithBuilder();
        for (auto& fleet : builderFleets) {
            allEntities.push_back(move(fleet));
        }
        
        // 4. Random generation
        auto randomEntities = EntityCreationDemo::createRandomEntities(20);
        for (auto& entity : randomEntities) {
            allEntities.push_back(move(entity));
        }
        
        // 5. Analyze all created entities
        EntityCreationDemo::analyzeEntities(allEntities);
        
        cout << "\n=== Entity Creation Complete ===" << endl;
        cout << "Successfully demonstrated various entity creation patterns:" << endl;
        cout << "  - Direct construction" << endl;
        cout << "  - Factory pattern" << endl;
        cout << "  - Builder pattern" << endl;
        cout << "  - Random generation" << endl;
        cout << "  - Entity analysis" << endl;
        
        LOG_INFO("Entity creation example completed successfully");
        
        return 0;
        
    } catch (const exception& e) {
        LOG_ERROR("Error in entity creation example: {}", e.what());
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
}