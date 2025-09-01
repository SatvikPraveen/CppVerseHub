// File: tests/unit_tests/core_tests/EntityTests.cpp
// Entity hierarchy tests for CppVerseHub core functionality

#include <catch2/catch.hpp>
#include <memory>
#include <vector>
#include <string>
#include <typeinfo>
#include <stdexcept>

// Include the core headers
#include "Entity.hpp"
#include "Planet.hpp"
#include "Fleet.hpp"
#include "Exceptions.hpp"

using namespace CppVerseHub::Core;

/**
 * @brief Test fixture for Entity hierarchy tests
 */
class EntityTestFixture {
public:
    EntityTestFixture() {
        // Set up common test data
        setupTestEntities();
    }
    
    ~EntityTestFixture() {
        // Clean up resources
        entities.clear();
    }
    
protected:
    void setupTestEntities() {
        // Create test entities for various scenarios
        try {
            auto planet = std::make_unique<Planet>("TestPlanet", Vector3D{100.0, 200.0, 300.0});
            planet->setResourceAmount(ResourceType::MINERALS, 1000);
            planet->setResourceAmount(ResourceType::ENERGY, 500);
            entities.push_back(std::move(planet));
            
            auto fleet = std::make_unique<Fleet>("TestFleet", Vector3D{50.0, 75.0, 125.0});
            fleet->setShipCount(10);
            entities.push_back(std::move(fleet));
            
        } catch (const std::exception& e) {
            INFO("Failed to setup test entities: " << e.what());
        }
    }
    
    std::vector<std::unique_ptr<Entity>> entities;
};

TEST_CASE_METHOD(EntityTestFixture, "Entity Base Class Functionality", "[entity][core]") {
    
    SECTION("Entity Creation and Basic Properties") {
        SECTION("Valid entity creation") {
            auto planet = std::make_unique<Planet>("Mars", Vector3D{1000.0, 2000.0, 3000.0});
            
            REQUIRE(planet != nullptr);
            REQUIRE(planet->getName() == "Mars");
            REQUIRE(planet->getPosition().x == 1000.0);
            REQUIRE(planet->getPosition().y == 2000.0);
            REQUIRE(planet->getPosition().z == 3000.0);
            REQUIRE(planet->getId() > 0);
        }
        
        SECTION("Entity with empty name") {
            REQUIRE_THROWS_AS(
                std::make_unique<Planet>("", Vector3D{0, 0, 0}),
                InvalidArgumentException
            );
        }
        
        SECTION("Entity ID uniqueness") {
            auto entity1 = std::make_unique<Planet>("Planet1", Vector3D{0, 0, 0});
            auto entity2 = std::make_unique<Planet>("Planet2", Vector3D{0, 0, 0});
            
            REQUIRE(entity1->getId() != entity2->getId());
        }
    }
    
    SECTION("Entity Position Management") {
        auto entity = std::make_unique<Fleet>("TestFleet", Vector3D{100, 200, 300});
        
        SECTION("Initial position") {
            Vector3D pos = entity->getPosition();
            REQUIRE(pos.x == 100.0);
            REQUIRE(pos.y == 200.0);
            REQUIRE(pos.z == 300.0);
        }
        
        SECTION("Position updates") {
            Vector3D newPos{500.0, 600.0, 700.0};
            entity->setPosition(newPos);
            
            Vector3D updatedPos = entity->getPosition();
            REQUIRE(updatedPos.x == 500.0);
            REQUIRE(updatedPos.y == 600.0);
            REQUIRE(updatedPos.z == 700.0);
        }
        
        SECTION("Distance calculations") {
            auto entity2 = std::make_unique<Planet>("TestPlanet", Vector3D{400, 600, 800});
            
            double distance = entity->distanceTo(*entity2);
            double expected = std::sqrt(
                (400-100)*(400-100) + 
                (600-200)*(600-200) + 
                (800-300)*(800-300)
            );
            
            REQUIRE(distance == Approx(expected).epsilon(0.001));
        }
    }
    
    SECTION("Entity Status and Health") {
        auto entity = std::make_unique<Fleet>("TestFleet", Vector3D{0, 0, 0});
        
        SECTION("Initial status") {
            REQUIRE(entity->getStatus() == EntityStatus::ACTIVE);
            REQUIRE(entity->getHealth() == Approx(100.0));
        }
        
        SECTION("Status changes") {
            entity->setStatus(EntityStatus::INACTIVE);
            REQUIRE(entity->getStatus() == EntityStatus::INACTIVE);
            
            entity->setStatus(EntityStatus::DESTROYED);
            REQUIRE(entity->getStatus() == EntityStatus::DESTROYED);
        }
        
        SECTION("Health modifications") {
            entity->takeDamage(25.0);
            REQUIRE(entity->getHealth() == Approx(75.0));
            
            entity->heal(15.0);
            REQUIRE(entity->getHealth() == Approx(90.0));
            
            // Test over-healing
            entity->heal(50.0);
            REQUIRE(entity->getHealth() == Approx(100.0)); // Should not exceed max
        }
        
        SECTION("Entity destruction on zero health") {
            entity->takeDamage(100.0);
            REQUIRE(entity->getHealth() == Approx(0.0));
            REQUIRE(entity->getStatus() == EntityStatus::DESTROYED);
        }
        
        SECTION("Invalid damage values") {
            REQUIRE_THROWS_AS(entity->takeDamage(-10.0), InvalidArgumentException);
        }
    }
}

TEST_CASE("Planet Entity Specialization", "[planet][entity][core]") {
    
    SECTION("Planet Creation and Resources") {
        SECTION("Basic planet creation") {
            Planet planet("Earth", Vector3D{0, 0, 0});
            
            REQUIRE(planet.getName() == "Earth");
            REQUIRE(planet.getType() == EntityType::PLANET);
            REQUIRE(planet.getPopulation() == 0);
        }
        
        SECTION("Resource management") {
            Planet planet("Mars", Vector3D{1000, 0, 0});
            
            // Test initial resources
            REQUIRE(planet.getResourceAmount(ResourceType::MINERALS) == 0);
            REQUIRE(planet.getResourceAmount(ResourceType::ENERGY) == 0);
            REQUIRE(planet.getResourceAmount(ResourceType::FOOD) == 0);
            
            // Test resource setting
            planet.setResourceAmount(ResourceType::MINERALS, 1500);
            planet.setResourceAmount(ResourceType::ENERGY, 800);
            planet.setResourceAmount(ResourceType::FOOD, 300);
            
            REQUIRE(planet.getResourceAmount(ResourceType::MINERALS) == 1500);
            REQUIRE(planet.getResourceAmount(ResourceType::ENERGY) == 800);
            REQUIRE(planet.getResourceAmount(ResourceType::FOOD) == 300);
        }
        
        SECTION("Resource production") {
            Planet planet("Venus", Vector3D{500, 500, 0});
            
            planet.setResourceProduction(ResourceType::MINERALS, 10.0);
            planet.setResourceProduction(ResourceType::ENERGY, 5.0);
            
            REQUIRE(planet.getResourceProduction(ResourceType::MINERALS) == Approx(10.0));
            REQUIRE(planet.getResourceProduction(ResourceType::ENERGY) == Approx(5.0));
            
            // Simulate resource production
            planet.produceResources(1.0); // 1 time unit
            REQUIRE(planet.getResourceAmount(ResourceType::MINERALS) == Approx(10.0));
            REQUIRE(planet.getResourceAmount(ResourceType::ENERGY) == Approx(5.0));
        }
    }
    
    SECTION("Planet Population Management") {
        Planet planet("Colony", Vector3D{2000, 1000, 500});
        
        SECTION("Population growth") {
            planet.setPopulation(1000);
            planet.setPopulationGrowthRate(0.05); // 5% growth rate
            
            planet.updatePopulation(1.0);
            REQUIRE(planet.getPopulation() == Approx(1050.0));
        }
        
        SECTION("Population limits") {
            planet.setPopulation(10000);
            planet.setMaxPopulation(5000);
            
            // Population should be capped
            REQUIRE(planet.getPopulation() == 5000);
        }
        
        SECTION("Invalid population values") {
            REQUIRE_THROWS_AS(planet.setPopulation(-100), InvalidArgumentException);
            REQUIRE_THROWS_AS(planet.setMaxPopulation(-1), InvalidArgumentException);
        }
    }
    
    SECTION("Planet Environmental Factors") {
        Planet planet("Hostile", Vector3D{0, 0, 0});
        
        SECTION("Habitability rating") {
            planet.setHabitabilityRating(0.75);
            REQUIRE(planet.getHabitabilityRating() == Approx(0.75));
            
            // Test invalid habitability values
            REQUIRE_THROWS_AS(planet.setHabitabilityRating(-0.1), InvalidArgumentException);
            REQUIRE_THROWS_AS(planet.setHabitabilityRating(1.1), InvalidArgumentException);
        }
        
        SECTION("Atmospheric conditions") {
            planet.setAtmosphereType(AtmosphereType::TOXIC);
            REQUIRE(planet.getAtmosphereType() == AtmosphereType::TOXIC);
            
            planet.setTemperature(250.0); // Kelvin
            REQUIRE(planet.getTemperature() == Approx(250.0));
        }
    }
}

TEST_CASE("Fleet Entity Specialization", "[fleet][entity][core]") {
    
    SECTION("Fleet Creation and Basic Properties") {
        Fleet fleet("Armada", Vector3D{1000, 2000, 3000});
        
        REQUIRE(fleet.getName() == "Armada");
        REQUIRE(fleet.getType() == EntityType::FLEET);
        REQUIRE(fleet.getShipCount() == 0);
        REQUIRE(fleet.getMaxSpeed() > 0.0);
    }
    
    SECTION("Fleet Composition") {
        Fleet fleet("TestFleet", Vector3D{0, 0, 0});
        
        SECTION("Ship management") {
            fleet.addShips(ShipType::FIGHTER, 10);
            fleet.addShips(ShipType::CRUISER, 5);
            fleet.addShips(ShipType::BATTLESHIP, 2);
            
            REQUIRE(fleet.getShipCount(ShipType::FIGHTER) == 10);
            REQUIRE(fleet.getShipCount(ShipType::CRUISER) == 5);
            REQUIRE(fleet.getShipCount(ShipType::BATTLESHIP) == 2);
            REQUIRE(fleet.getShipCount() == 17);
            
            // Remove ships
            fleet.removeShips(ShipType::FIGHTER, 3);
            REQUIRE(fleet.getShipCount(ShipType::FIGHTER) == 7);
            REQUIRE(fleet.getShipCount() == 14);
        }
        
        SECTION("Invalid ship operations") {
            REQUIRE_THROWS_AS(fleet.addShips(ShipType::FIGHTER, -5), InvalidArgumentException);
            REQUIRE_THROWS_AS(fleet.removeShips(ShipType::CRUISER, 10), InvalidOperationException);
        }
    }
    
    SECTION("Fleet Movement and Navigation") {
        Fleet fleet("Navigator", Vector3D{0, 0, 0});
        
        SECTION("Movement commands") {
            Vector3D destination{1000, 1000, 1000};
            fleet.setDestination(destination);
            
            Vector3D retrievedDest = fleet.getDestination();
            REQUIRE(retrievedDest.x == 1000.0);
            REQUIRE(retrievedDest.y == 1000.0);
            REQUIRE(retrievedDest.z == 1000.0);
            
            REQUIRE(fleet.isMoving() == true);
        }
        
        SECTION("Speed calculations") {
            fleet.addShips(ShipType::FIGHTER, 5);    // Fast ships
            fleet.addShips(ShipType::BATTLESHIP, 2); // Slow ships
            
            double maxSpeed = fleet.getMaxSpeed();
            double effectiveSpeed = fleet.getEffectiveSpeed();
            
            REQUIRE(effectiveSpeed <= maxSpeed);
            REQUIRE(effectiveSpeed > 0);
        }
        
        SECTION("Movement simulation") {
            Vector3D start{0, 0, 0};
            Vector3D end{100, 0, 0};
            
            fleet.setPosition(start);
            fleet.setDestination(end);
            
            // Simulate movement for 1 time unit
            fleet.updateMovement(1.0);
            
            Vector3D newPos = fleet.getPosition();
            // Should have moved towards destination
            REQUIRE(newPos.x > 0);
            REQUIRE(newPos.x <= 100);
        }
    }
    
    SECTION("Fleet Combat Capabilities") {
        Fleet fleet("Warrior", Vector3D{0, 0, 0});
        
        SECTION("Combat power calculation") {
            fleet.addShips(ShipType::FIGHTER, 10);
            fleet.addShips(ShipType::CRUISER, 5);
            
            double combatPower = fleet.getCombatPower();
            REQUIRE(combatPower > 0);
            
            // Adding more ships should increase combat power
            fleet.addShips(ShipType::BATTLESHIP, 1);
            double newCombatPower = fleet.getCombatPower();
            REQUIRE(newCombatPower > combatPower);
        }
        
        SECTION("Fleet damage and repairs") {
            fleet.addShips(ShipType::FIGHTER, 10);
            
            // Take damage - should reduce ship count
            int initialCount = fleet.getShipCount();
            fleet.takeCombatDamage(30.0);
            
            REQUIRE(fleet.getShipCount() <= initialCount);
            REQUIRE(fleet.getHealth() < 100.0);
        }
    }
}

TEST_CASE("Entity Polymorphism and Virtual Functions", "[entity][polymorphism][core]") {
    
    SECTION("Polymorphic Behavior") {
        std::vector<std::unique_ptr<Entity>> entities;
        
        entities.push_back(std::make_unique<Planet>("PolyPlanet", Vector3D{0, 0, 0}));
        entities.push_back(std::make_unique<Fleet>("PolyFleet", Vector3D{100, 100, 100}));
        
        SECTION("Virtual function calls") {
            for (auto& entity : entities) {
                // Test virtual function dispatch
                REQUIRE(!entity->getName().empty());
                REQUIRE(entity->getId() > 0);
                REQUIRE(entity->getHealth() == Approx(100.0));
                
                // Test polymorphic update
                entity->update(1.0);
                
                // Test type identification
                EntityType type = entity->getType();
                REQUIRE((type == EntityType::PLANET || type == EntityType::FLEET));
            }
        }
        
        SECTION("Runtime type identification") {
            for (auto& entity : entities) {
                if (entity->getType() == EntityType::PLANET) {
                    Planet* planet = dynamic_cast<Planet*>(entity.get());
                    REQUIRE(planet != nullptr);
                    
                    // Test planet-specific functionality
                    planet->setResourceAmount(ResourceType::MINERALS, 100);
                    REQUIRE(planet->getResourceAmount(ResourceType::MINERALS) == 100);
                }
                
                if (entity->getType() == EntityType::FLEET) {
                    Fleet* fleet = dynamic_cast<Fleet*>(entity.get());
                    REQUIRE(fleet != nullptr);
                    
                    // Test fleet-specific functionality
                    fleet->addShips(ShipType::FIGHTER, 5);
                    REQUIRE(fleet->getShipCount() == 5);
                }
            }
        }
    }
    
    SECTION("Abstract Base Class Enforcement") {
        // Entity is abstract - cannot be instantiated directly
        // This test ensures the design is properly abstract
        
        // Test that derived classes implement required virtual functions
        auto planet = std::make_unique<Planet>("TestPlanet", Vector3D{0, 0, 0});
        auto fleet = std::make_unique<Fleet>("TestFleet", Vector3D{0, 0, 0});
        
        // These should not throw - implementations should exist
        REQUIRE_NOTHROW(planet->update(1.0));
        REQUIRE_NOTHROW(planet->render());
        REQUIRE_NOTHROW(fleet->update(1.0));
        REQUIRE_NOTHROW(fleet->render());
    }
}

TEST_CASE("Entity Exception Handling", "[entity][exceptions][core]") {
    
    SECTION("Construction Exceptions") {
        SECTION("Invalid name parameter") {
            REQUIRE_THROWS_AS(
                Planet("", Vector3D{0, 0, 0}),
                InvalidArgumentException
            );
        }
        
        SECTION("Null pointer handling") {
            // Test various null pointer scenarios if applicable
            // This depends on the actual implementation
        }
    }
    
    SECTION("Operation Exceptions") {
        Planet planet("TestPlanet", Vector3D{0, 0, 0});
        Fleet fleet("TestFleet", Vector3D{0, 0, 0});
        
        SECTION("Invalid resource operations") {
            REQUIRE_THROWS_AS(
                planet.setResourceAmount(ResourceType::MINERALS, -100),
                InvalidArgumentException
            );
        }
        
        SECTION("Invalid fleet operations") {
            REQUIRE_THROWS_AS(
                fleet.removeShips(ShipType::FIGHTER, 10),
                InvalidOperationException
            );
        }
        
        SECTION("Invalid health operations") {
            REQUIRE_THROWS_AS(
                planet.takeDamage(-50.0),
                InvalidArgumentException
            );
        }
    }
}

TEST_CASE("Entity Performance and Memory", "[entity][performance][core]") {
    
    SECTION("Entity Creation Performance") {
        const int entityCount = 1000;
        std::vector<std::unique_ptr<Entity>> entities;
        entities.reserve(entityCount);
        
        auto duration = measurePerformance([&]() {
            for (int i = 0; i < entityCount; ++i) {
                if (i % 2 == 0) {
                    entities.push_back(
                        std::make_unique<Planet>("Planet" + std::to_string(i), 
                                               Vector3D{i, i, i})
                    );
                } else {
                    entities.push_back(
                        std::make_unique<Fleet>("Fleet" + std::to_string(i), 
                                              Vector3D{i, i, i})
                    );
                }
            }
        }, "Creating " + std::to_string(entityCount) + " entities");
        
        // Performance should be reasonable (adjust threshold as needed)
        REQUIRE(duration.count() < 100000); // 100ms
        REQUIRE(entities.size() == entityCount);
    }
    
    SECTION("Entity Update Performance") {
        std::vector<std::unique_ptr<Entity>> entities;
        for (int i = 0; i < 100; ++i) {
            entities.push_back(
                std::make_unique<Planet>("Planet" + std::to_string(i), 
                                       Vector3D{i, i, i})
            );
        }
        
        auto duration = measurePerformance([&]() {
            for (auto& entity : entities) {
                entity->update(1.0);
            }
        }, "Updating 100 entities");
        
        // Update performance should be fast
        REQUIRE(duration.count() < 10000); // 10ms
    }
    
    SECTION("Memory Usage") {
        MemoryTracker::printMemoryStats("Before entity creation");
        
        {
            std::vector<std::unique_ptr<Entity>> entities;
            for (int i = 0; i < 1000; ++i) {
                entities.push_back(
                    std::make_unique<Planet>("Planet" + std::to_string(i), 
                                           Vector3D{i, i, i})
                );
            }
            MemoryTracker::printMemoryStats("After entity creation");
        } // entities destroyed here
        
        MemoryTracker::printMemoryStats("After entity destruction");
    }
}