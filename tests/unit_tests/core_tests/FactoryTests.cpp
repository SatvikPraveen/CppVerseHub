// File: tests/unit_tests/core_tests/FactoryTests.cpp
// Factory pattern tests for CppVerseHub core functionality

#include <catch2/catch.hpp>
#include <memory>
#include <vector>
#include <string>
#include <map>
#include <typeinfo>
#include <algorithm>

// Include the factory system headers
#include "Factory.hpp"
#include "Entity.hpp"
#include "Planet.hpp"
#include "Fleet.hpp"
#include "Mission.hpp"
#include "ExplorationMission.hpp"
#include "ColonizationMission.hpp"
#include "CombatMission.hpp"
#include "Exceptions.hpp"

using namespace CppVerseHub::Core;

/**
 * @brief Test fixture for Factory pattern tests
 */
class FactoryTestFixture {
public:
    FactoryTestFixture() {
        setupTestParameters();
    }
    
    ~FactoryTestFixture() {
        createdEntities.clear();
        createdMissions.clear();
    }
    
protected:
    void setupTestParameters() {
        // Setup entity creation parameters
        planetParams = {
            {"name", std::string("TestPlanet")},
            {"position", Vector3D{100.0, 200.0, 300.0}},
            {"habitability", 0.8},
            {"minerals", 1000},
            {"energy", 500},
            {"food", 300}
        };
        
        fleetParams = {
            {"name", std::string("TestFleet")},
            {"position", Vector3D{150.0, 250.0, 350.0}},
            {"fighters", 10},
            {"cruisers", 5},
            {"battleships", 2}
        };
        
        explorationMissionParams = {
            {"fleet_id", 1},
            {"target_id", 2},
            {"duration", 30.0},
            {"survey_enabled", true}
        };
        
        colonizationMissionParams = {
            {"fleet_id", 3},
            {"target_id", 4},
            {"duration", 60.0},
            {"colonists", 1000},
            {"initial_infrastructure", 50}
        };
        
        combatMissionParams = {
            {"fleet_id", 5},
            {"target_id", 6},
            {"duration", 25.0},
            {"strategy", std::string("aggressive")},
            {"objective", std::string("orbital_bombardment")}
        };
    }
    
    std::map<std::string, std::any> planetParams;
    std::map<std::string, std::any> fleetParams;
    std::map<std::string, std::any> explorationMissionParams;
    std::map<std::string, std::any> colonizationMissionParams;
    std::map<std::string, std::any> combatMissionParams;
    
    std::vector<std::unique_ptr<Entity>> createdEntities;
    std::vector<std::unique_ptr<Mission>> createdMissions;
};

TEST_CASE_METHOD(FactoryTestFixture, "Entity Factory Basic Functionality", "[factory][entity][core]") {
    
    SECTION("Entity Factory Registration") {
        SECTION("Factory instance access") {
            auto& factory = EntityFactory::getInstance();
            REQUIRE(&factory == &EntityFactory::getInstance()); // Singleton check
        }
        
        SECTION("Built-in entity types registration") {
            auto& factory = EntityFactory::getInstance();
            
            // Check if standard entity types are registered
            REQUIRE(factory.isTypeRegistered("Planet"));
            REQUIRE(factory.isTypeRegistered("Fleet"));
            
            // Get registered types
            auto registeredTypes = factory.getRegisteredTypes();
            REQUIRE(std::find(registeredTypes.begin(), registeredTypes.end(), "Planet") != registeredTypes.end());
            REQUIRE(std::find(registeredTypes.begin(), registeredTypes.end(), "Fleet") != registeredTypes.end());
        }
        
        SECTION("Custom entity type registration") {
            auto& factory = EntityFactory::getInstance();
            
            // Register a custom creator function
            factory.registerType("CustomPlanet", [](const std::map<std::string, std::any>& params) {
                return std::make_unique<Planet>(
                    std::any_cast<std::string>(params.at("name")),
                    std::any_cast<Vector3D>(params.at("position"))
                );
            });
            
            REQUIRE(factory.isTypeRegistered("CustomPlanet"));
            
            auto types = factory.getRegisteredTypes();
            REQUIRE(std::find(types.begin(), types.end(), "CustomPlanet") != types.end());
        }
    }
    
    SECTION("Entity Creation") {
        auto& factory = EntityFactory::getInstance();
        
        SECTION("Planet creation") {
            auto planet = factory.createEntity("Planet", planetParams);
            
            REQUIRE(planet != nullptr);
            REQUIRE(planet->getType() == EntityType::PLANET);
            REQUIRE(planet->getName() == "TestPlanet");
            REQUIRE(planet->getPosition().x == Approx(100.0));
            REQUIRE(planet->getPosition().y == Approx(200.0));
            REQUIRE(planet->getPosition().z == Approx(300.0));
            
            // Test planet-specific properties
            Planet* planetPtr = dynamic_cast<Planet*>(planet.get());
            REQUIRE(planetPtr != nullptr);
            REQUIRE(planetPtr->getHabitabilityRating() == Approx(0.8));
            REQUIRE(planetPtr->getResourceAmount(ResourceType::MINERALS) == 1000);
            REQUIRE(planetPtr->getResourceAmount(ResourceType::ENERGY) == 500);
            REQUIRE(planetPtr->getResourceAmount(ResourceType::FOOD) == 300);
        }
        
        SECTION("Fleet creation") {
            auto fleet = factory.createEntity("Fleet", fleetParams);
            
            REQUIRE(fleet != nullptr);
            REQUIRE(fleet->getType() == EntityType::FLEET);
            REQUIRE(fleet->getName() == "TestFleet");
            REQUIRE(fleet->getPosition().x == Approx(150.0));
            
            // Test fleet-specific properties
            Fleet* fleetPtr = dynamic_cast<Fleet*>(fleet.get());
            REQUIRE(fleetPtr != nullptr);
            REQUIRE(fleetPtr->getShipCount(ShipType::FIGHTER) == 10);
            REQUIRE(fleetPtr->getShipCount(ShipType::CRUISER) == 5);
            REQUIRE(fleetPtr->getShipCount(ShipType::BATTLESHIP) == 2);
            REQUIRE(fleetPtr->getShipCount() == 17);
        }
        
        SECTION("Invalid entity type") {
            REQUIRE_THROWS_AS(
                factory.createEntity("NonExistentType", planetParams),
                UnknownEntityTypeException
            );
        }
        
        SECTION("Invalid parameters") {
            std::map<std::string, std::any> invalidParams = {
                {"wrong_param", std::string("value")}
            };
            
            REQUIRE_THROWS_AS(
                factory.createEntity("Planet", invalidParams),
                InvalidParameterException
            );
        }
    }
    
    SECTION("Batch Entity Creation") {
        auto& factory = EntityFactory::getInstance();
        
        SECTION("Multiple entities of same type") {
            std::vector<std::map<std::string, std::any>> batchParams;
            
            for (int i = 0; i < 5; ++i) {
                std::map<std::string, std::any> params = {
                    {"name", std::string("Planet_") + std::to_string(i)},
                    {"position", Vector3D{i * 100.0, i * 100.0, i * 100.0}},
                    {"habitability", 0.5 + (i * 0.1)},
                    {"minerals", 1000 + (i * 200)},
                    {"energy", 500 + (i * 100)},
                    {"food", 300 + (i * 50)}
                };
                batchParams.push_back(params);
            }
            
            auto entities = factory.createBatch("Planet", batchParams);
            
            REQUIRE(entities.size() == 5);
            
            for (size_t i = 0; i < entities.size(); ++i) {
                REQUIRE(entities[i] != nullptr);
                REQUIRE(entities[i]->getName() == "Planet_" + std::to_string(i));
                REQUIRE(entities[i]->getType() == EntityType::PLANET);
                
                Planet* planet = dynamic_cast<Planet*>(entities[i].get());
                REQUIRE(planet != nullptr);
                REQUIRE(planet->getResourceAmount(ResourceType::MINERALS) == 1000 + (i * 200));
            }
        }
        
        SECTION("Mixed entity types batch creation") {
            std::vector<std::pair<std::string, std::map<std::string, std::any>>> mixedBatch;
            
            // Add planets
            for (int i = 0; i < 3; ++i) {
                std::map<std::string, std::any> planetParams = {
                    {"name", std::string("BatchPlanet_") + std::to_string(i)},
                    {"position", Vector3D{i * 50.0, i * 50.0, 0.0}},
                    {"habitability", 0.7},
                    {"minerals", 800},
                    {"energy", 400},
                    {"food", 200}
                };
                mixedBatch.emplace_back("Planet", planetParams);
            }
            
            // Add fleets
            for (int i = 0; i < 2; ++i) {
                std::map<std::string, std::any> fleetParams = {
                    {"name", std::string("BatchFleet_") + std::to_string(i)},
                    {"position", Vector3D{i * 75.0, i * 75.0, 100.0}},
                    {"fighters", 8 + i * 2},
                    {"cruisers", 3 + i},
                    {"battleships", 1}
                };
                mixedBatch.emplace_back("Fleet", fleetParams);
            }
            
            auto mixedEntities = factory.createMixedBatch(mixedBatch);
            
            REQUIRE(mixedEntities.size() == 5);
            
            int planetCount = 0, fleetCount = 0;
            for (const auto& entity : mixedEntities) {
                if (entity->getType() == EntityType::PLANET) planetCount++;
                else if (entity->getType() == EntityType::FLEET) fleetCount++;
            }
            
            REQUIRE(planetCount == 3);
            REQUIRE(fleetCount == 2);
        }
    }
}

TEST_CASE_METHOD(FactoryTestFixture, "Mission Factory Functionality", "[factory][mission][core]") {
    
    SECTION("Mission Factory Registration") {
        SECTION("Mission factory instance") {
            auto& factory = MissionFactory::getInstance();
            REQUIRE(&factory == &MissionFactory::getInstance()); // Singleton check
        }
        
        SECTION("Built-in mission types") {
            auto& factory = MissionFactory::getInstance();
            
            REQUIRE(factory.isTypeRegistered("ExplorationMission"));
            REQUIRE(factory.isTypeRegistered("ColonizationMission"));
            REQUIRE(factory.isTypeRegistered("CombatMission"));
            
            auto registeredTypes = factory.getRegisteredTypes();
            REQUIRE(registeredTypes.size() >= 3);
        }
        
        SECTION("Custom mission type registration") {
            auto& factory = MissionFactory::getInstance();
            
            factory.registerType("CustomMission", [](const std::map<std::string, std::any>& params) {
                // Create a custom exploration mission as example
                auto fleetId = std::any_cast<int>(params.at("fleet_id"));
                auto targetId = std::any_cast<int>(params.at("target_id"));
                auto duration = std::any_cast<double>(params.at("duration"));
                
                // This would need actual fleet and planet pointers in real implementation
                // For test purposes, we'll create dummy objects
                return std::unique_ptr<Mission>(nullptr); // Placeholder
            });
            
            REQUIRE(factory.isTypeRegistered("CustomMission"));
        }
    }
    
    SECTION("Mission Creation") {
        auto& missionFactory = MissionFactory::getInstance();
        auto& entityFactory = EntityFactory::getInstance();
        
        // Create entities needed for missions
        auto fleet = entityFactory.createEntity("Fleet", fleetParams);
        auto planet = entityFactory.createEntity("Planet", planetParams);
        
        SECTION("Exploration mission creation") {
            std::map<std::string, std::any> params = {
                {"fleet", fleet.get()},
                {"target", planet.get()},
                {"duration", 30.0},
                {"survey_enabled", true}
            };
            
            auto mission = missionFactory.createMission("ExplorationMission", params);
            
            REQUIRE(mission != nullptr);
            REQUIRE(mission->getType() == MissionType::EXPLORATION);
            REQUIRE(mission->getDuration() == Approx(30.0));
            REQUIRE(mission->getStatus() == MissionStatus::PENDING);
            
            ExplorationMission* expMission = dynamic_cast<ExplorationMission*>(mission.get());
            REQUIRE(expMission != nullptr);
            REQUIRE(expMission->isResourceSurveyEnabled());
        }
        
        SECTION("Colonization mission creation") {
            std::map<std::string, std::any> params = {
                {"fleet", fleet.get()},
                {"target", planet.get()},
                {"duration", 60.0},
                {"colonists", 1000},
                {"initial_infrastructure", 50}
            };
            
            auto mission = missionFactory.createMission("ColonizationMission", params);
            
            REQUIRE(mission != nullptr);
            REQUIRE(mission->getType() == MissionType::COLONIZATION);
            REQUIRE(mission->getDuration() == Approx(60.0));
            
            ColonizationMission* colMission = dynamic_cast<ColonizationMission*>(mission.get());
            REQUIRE(colMission != nullptr);
            REQUIRE(colMission->getColonistCount() == 1000);
        }
        
        SECTION("Combat mission creation") {
            std::map<std::string, std::any> params = {
                {"fleet", fleet.get()},
                {"target", planet.get()},
                {"duration", 25.0},
                {"strategy", std::string("aggressive")},
                {"objective", std::string("orbital_bombardment")}
            };
            
            auto mission = missionFactory.createMission("CombatMission", params);
            
            REQUIRE(mission != nullptr);
            REQUIRE(mission->getType() == MissionType::COMBAT);
            REQUIRE(mission->getDuration() == Approx(25.0));
            
            CombatMission* combatMission = dynamic_cast<CombatMission*>(mission.get());
            REQUIRE(combatMission != nullptr);
            REQUIRE(combatMission->getTacticalStrategy() == CombatStrategy::AGGRESSIVE);
            REQUIRE(combatMission->getPrimaryObjective() == CombatObjective::ORBITAL_BOMBARDMENT);
        }
    }
    
    SECTION("Mission Template System") {
        auto& factory = MissionFactory::getInstance();
        
        SECTION("Mission template registration") {
            MissionTemplate explorationTemplate;
            explorationTemplate.name = "StandardExploration";
            explorationTemplate.type = "ExplorationMission";
            explorationTemplate.defaultDuration = 25.0;
            explorationTemplate.defaultParameters = {
                {"survey_enabled", true},
                {"detailed_scan", false},
                {"risk_level", std::string("medium")}
            };
            explorationTemplate.requiredParameters = {"fleet", "target"};
            
            factory.registerTemplate(explorationTemplate);
            
            REQUIRE(factory.hasTemplate("StandardExploration"));
            
            auto retrievedTemplate = factory.getTemplate("StandardExploration");
            REQUIRE(retrievedTemplate.name == "StandardExploration");
            REQUIRE(retrievedTemplate.defaultDuration == Approx(25.0));
        }
        
        SECTION("Mission creation from template") {
            auto& entityFactory = EntityFactory::getInstance();
            auto fleet = entityFactory.createEntity("Fleet", fleetParams);
            auto planet = entityFactory.createEntity("Planet", planetParams);
            
            std::map<std::string, std::any> templateParams = {
                {"fleet", fleet.get()},
                {"target", planet.get()}
            };
            
            auto mission = factory.createFromTemplate("StandardExploration", templateParams);
            
            REQUIRE(mission != nullptr);
            REQUIRE(mission->getType() == MissionType::EXPLORATION);
            REQUIRE(mission->getDuration() == Approx(25.0)); // From template
            
            ExplorationMission* expMission = dynamic_cast<ExplorationMission*>(mission.get());
            REQUIRE(expMission != nullptr);
            REQUIRE(expMission->isResourceSurveyEnabled()); // From template defaults
        }
    }
}

TEST_CASE_METHOD(FactoryTestFixture, "Abstract Factory Pattern", "[factory][abstract][core]") {
    
    SECTION("Factory Family Registration") {
        SECTION("Entity factory family") {
            auto& abstractFactory = AbstractEntityFactory::getInstance();
            
            // Register different factory families
            abstractFactory.registerFactory("Standard", std::make_unique<StandardEntityFactory>());
            abstractFactory.registerFactory("Advanced", std::make_unique<AdvancedEntityFactory>());
            abstractFactory.registerFactory("Experimental", std::make_unique<ExperimentalEntityFactory>());
            
            REQUIRE(abstractFactory.hasFactory("Standard"));
            REQUIRE(abstractFactory.hasFactory("Advanced"));
            REQUIRE(abstractFactory.hasFactory("Experimental"));
            
            auto factoryNames = abstractFactory.getRegisteredFactories();
            REQUIRE(factoryNames.size() >= 3);
        }
        
        SECTION("Factory switching") {
            auto& abstractFactory = AbstractEntityFactory::getInstance();
            
            abstractFactory.setActiveFactory("Standard");
            REQUIRE(abstractFactory.getActiveFactoryName() == "Standard");
            
            auto planet1 = abstractFactory.createEntity("Planet", planetParams);
            REQUIRE(planet1 != nullptr);
            
            // Switch to advanced factory
            abstractFactory.setActiveFactory("Advanced");
            REQUIRE(abstractFactory.getActiveFactoryName() == "Advanced");
            
            auto planet2 = abstractFactory.createEntity("Planet", planetParams);
            REQUIRE(planet2 != nullptr);
            
            // Different factories might create entities with different capabilities
            // This depends on the specific implementation
        }
    }
    
    SECTION("Factory Configuration") {
        auto& abstractFactory = AbstractEntityFactory::getInstance();
        
        SECTION("Configuration-based factory selection") {
            FactoryConfiguration config;
            config.factoryType = "Advanced";
            config.enableOptimizations = true;
            config.useCustomAllocators = false;
            config.debugMode = true;
            
            abstractFactory.configure(config);
            
            REQUIRE(abstractFactory.getActiveFactoryName() == "Advanced");
            REQUIRE(abstractFactory.getConfiguration().enableOptimizations == true);
            REQUIRE(abstractFactory.getConfiguration().debugMode == true);
        }
        
        SECTION("Factory capabilities") {
            abstractFactory.setActiveFactory("Standard");
            auto standardCaps = abstractFactory.getFactoryCapabilities();
            
            abstractFactory.setActiveFactory("Advanced");
            auto advancedCaps = abstractFactory.getFactoryCapabilities();
            
            // Advanced factory should have more capabilities
            REQUIRE(advancedCaps.supportedTypes.size() >= standardCaps.supportedTypes.size());
            REQUIRE(advancedCaps.maxBatchSize >= standardCaps.maxBatchSize);
        }
    }
}

TEST_CASE_METHOD(FactoryTestFixture, "Factory Performance and Optimization", "[factory][performance][core]") {
    
    SECTION("Creation Performance") {
        auto& factory = EntityFactory::getInstance();
        
        SECTION("Single entity creation performance") {
            const int entityCount = 1000;
            std::vector<std::unique_ptr<Entity>> entities;
            entities.reserve(entityCount);
            
            auto duration = measurePerformance([&]() {
                for (int i = 0; i < entityCount; ++i) {
                    std::map<std::string, std::any> params = {
                        {"name", std::string("PerfPlanet_") + std::to_string(i)},
                        {"position", Vector3D{i, i, i}},
                        {"habitability", 0.5},
                        {"minerals", 1000},
                        {"energy", 500},
                        {"food", 300}
                    };
                    
                    entities.push_back(factory.createEntity("Planet", params));
                }
            }, "Creating " + std::to_string(entityCount) + " planets");
            
            REQUIRE(duration.count() < 500000); // 500ms threshold
            REQUIRE(entities.size() == entityCount);
            
            // Verify all entities were created properly
            for (const auto& entity : entities) {
                REQUIRE(entity != nullptr);
                REQUIRE(entity->getType() == EntityType::PLANET);
            }
        }
        
        SECTION("Batch creation performance") {
            const int batchSize = 2000;
            std::vector<std::map<std::string, std::any>> batchParams;
            batchParams.reserve(batchSize);
            
            for (int i = 0; i < batchSize; ++i) {
                batchParams.push_back({
                    {"name", std::string("BatchPlanet_") + std::to_string(i)},
                    {"position", Vector3D{i, i, i}},
                    {"habitability", 0.6},
                    {"minerals", 800},
                    {"energy", 400},
                    {"food", 200}
                });
            }
            
            std::vector<std::unique_ptr<Entity>> batchEntities;
            
            auto duration = measurePerformance([&]() {
                batchEntities = factory.createBatch("Planet", batchParams);
            }, "Batch creating " + std::to_string(batchSize) + " planets");
            
            REQUIRE(duration.count() < 1000000); // 1s threshold
            REQUIRE(batchEntities.size() == batchSize);
        }
    }
    
    SECTION("Memory Management") {
        auto& factory = EntityFactory::getInstance();
        
        SECTION("Memory pool utilization") {
            MemoryTracker::printMemoryStats("Before factory operations");
            
            {
                std::vector<std::unique_ptr<Entity>> entities;
                for (int i = 0; i < 5000; ++i) {
                    entities.push_back(factory.createEntity("Planet", planetParams));
                }
                
                MemoryTracker::printMemoryStats("After creating 5000 entities");
                
                // Test entity destruction and memory cleanup
            } // Entities destroyed here
            
            MemoryTracker::printMemoryStats("After entity cleanup");
        }
        
        SECTION("Factory caching efficiency") {
            // Enable factory caching if available
            factory.enableCaching(true);
            
            auto duration1 = measurePerformance([&]() {
                for (int i = 0; i < 100; ++i) {
                    factory.createEntity("Planet", planetParams);
                }
            }, "First batch with caching enabled");
            
            auto duration2 = measurePerformance([&]() {
                for (int i = 0; i < 100; ++i) {
                    factory.createEntity("Planet", planetParams);
                }
            }, "Second batch with caching");
            
            // Second batch should be faster due to caching
            // (This depends on the specific caching implementation)
            INFO("First duration: " << duration1.count() << "μs");
            INFO("Second duration: " << duration2.count() << "μs");
        }
    }
}

TEST_CASE_METHOD(FactoryTestFixture, "Factory Error Handling", "[factory][exceptions][core]") {
    
    SECTION("Creation Failures") {
        auto& factory = EntityFactory::getInstance();
        
        SECTION("Unknown entity type") {
            REQUIRE_THROWS_AS(
                factory.createEntity("UnknownType", planetParams),
                UnknownEntityTypeException
            );
        }
        
        SECTION("Missing required parameters") {
            std::map<std::string, std::any> incompleteParams = {
                {"name", std::string("IncompletePlanet")}
                // Missing position and other required params
            };
            
            REQUIRE_THROWS_AS(
                factory.createEntity("Planet", incompleteParams),
                MissingParameterException
            );
        }
        
        SECTION("Invalid parameter types") {
            std::map<std::string, std::any> invalidParams = {
                {"name", 123}, // Should be string
                {"position", std::string("not a vector")}, // Should be Vector3D
                {"habitability", std::string("invalid")}, // Should be double
                {"minerals", -100}, // Should be positive
                {"energy", -50},
                {"food", -25}
            };
            
            REQUIRE_THROWS_AS(
                factory.createEntity("Planet", invalidParams),
                InvalidParameterTypeException
            );
        }
        
        SECTION("Parameter validation failures") {
            std::map<std::string, std::any> invalidRangeParams = {
                {"name", std::string("ValidPlanet")},
                {"position", Vector3D{0, 0, 0}},
                {"habitability", 1.5}, // Out of range [0, 1]
                {"minerals", -500}, // Negative resources
                {"energy", -200},
                {"food", -100}
            };
            
            REQUIRE_THROWS_AS(
                factory.createEntity("Planet", invalidRangeParams),
                ParameterValidationException
            );
        }
    }
    
    SECTION("Factory State Errors") {
        auto& factory = EntityFactory::getInstance();
        
        SECTION("Uninitialized factory") {
            // Test what happens when factory is not properly initialized
            // This depends on implementation - might be automatic
            
            // Reset factory if possible
            if (factory.canReset()) {
                factory.reset();
                
                REQUIRE_THROWS_AS(
                    factory.createEntity("Planet", planetParams),
                    FactoryNotInitializedException
                );
                
                // Reinitialize
                factory.initialize();
            }
        }
        
        SECTION("Factory resource exhaustion") {
            // Test behavior when factory resources are exhausted
            // This might involve memory limits, handle limits, etc.
            
            try {
                std::vector<std::unique_ptr<Entity>> entities;
                
                // Try to create a very large number of entities
                for (int i = 0; i < 100000; ++i) {
                    entities.push_back(factory.createEntity("Planet", planetParams));
                }
                
                // If we get here, the factory handled large-scale creation well
                REQUIRE(entities.size() == 100000);
                
            } catch (const ResourceExhaustedException& e) {
                // This is acceptable - factory properly reported resource exhaustion
                INFO("Factory resource exhaustion handled correctly: " << e.what());
            }
        }
    }
    
    SECTION("Concurrent Access Errors") {
        auto& factory = EntityFactory::getInstance();
        
        SECTION("Thread safety verification") {
            const int threadCount = 10;
            const int entitiesPerThread = 100;
            
            std::vector<std::thread> threads;
            std::atomic<int> successfulCreations{0};
            std::atomic<int> exceptions{0};
            
            for (int t = 0; t < threadCount; ++t) {
                threads.emplace_back([&, t]() {
                    for (int i = 0; i < entitiesPerThread; ++i) {
                        try {
                            std::map<std::string, std::any> params = {
                                {"name", std::string("ThreadPlanet_") + std::to_string(t) + "_" + std::to_string(i)},
                                {"position", Vector3D{t * 100.0 + i, t * 100.0 + i, 0}},
                                {"habitability", 0.5},
                                {"minerals", 1000},
                                {"energy", 500},
                                {"food", 300}
                            };
                            
                            auto entity = factory.createEntity("Planet", params);
                            if (entity) {
                                successfulCreations++;
                            }
                            
                        } catch (const std::exception&) {
                            exceptions++;
                        }
                    }
                });
            }
            
            for (auto& thread : threads) {
                thread.join();
            }
            
            // Most creations should succeed
            REQUIRE(successfulCreations > (threadCount * entitiesPerThread) / 2);
            
            // Exception rate should be reasonable
            REQUIRE(exceptions < (threadCount * entitiesPerThread) / 4);
            
            INFO("Successful creations: " << successfulCreations);
            INFO("Exceptions: " << exceptions);
        }
    }
}

TEST_CASE_METHOD(FactoryTestFixture, "Factory Extension and Customization", "[factory][extension][core]") {
    
    SECTION("Custom Entity Factory") {
        SECTION("Factory inheritance") {
            class CustomEntityFactory : public EntityFactory {
            public:
                std::unique_ptr<Entity> createCustomEntity(const std::string& type, 
                                                         const std::map<std::string, std::any>& params) override {
                    if (type == "SuperPlanet") {
                        auto planet = createEntity("Planet", params);
                        Planet* planetPtr = dynamic_cast<Planet*>(planet.get());
                        if (planetPtr) {
                            // Enhance with super planet capabilities
                            planetPtr->setResourceAmount(ResourceType::MINERALS, 
                                                       planetPtr->getResourceAmount(ResourceType::MINERALS) * 2);
                            planetPtr->setResourceAmount(ResourceType::ENERGY, 
                                                       planetPtr->getResourceAmount(ResourceType::ENERGY) * 2);
                            planetPtr->setHabitabilityRating(
                                std::min(1.0, planetPtr->getHabitabilityRating() * 1.5));
                        }
                        return planet;
                    }
                    
                    return EntityFactory::createEntity(type, params);
                }
            };
            
            CustomEntityFactory customFactory;
            auto superPlanet = customFactory.createCustomEntity("SuperPlanet", planetParams);
            
            REQUIRE(superPlanet != nullptr);
            Planet* planetPtr = dynamic_cast<Planet*>(superPlanet.get());
            REQUIRE(planetPtr != nullptr);
            
            // Should have enhanced resources
            REQUIRE(planetPtr->getResourceAmount(ResourceType::MINERALS) == 2000); // 2x original
            REQUIRE(planetPtr->getResourceAmount(ResourceType::ENERGY) == 1000);   // 2x original
            REQUIRE(planetPtr->getHabitabilityRating() > 0.8); // Enhanced habitability
        }
    }
    
    SECTION("Plugin System Integration") {
        auto& factory = EntityFactory::getInstance();
        
        SECTION("Dynamic factory extension") {
            // Register a plugin-provided entity type
            factory.registerPlugin("AlienTechnology", [](const std::map<std::string, std::any>& params) {
                // Create a special fleet with alien technology
                auto fleet = std::make_unique<Fleet>(
                    std::any_cast<std::string>(params.at("name")),
                    std::any_cast<Vector3D>(params.at("position"))
                );
                
                // Add alien ships with special capabilities
                fleet->addShips(ShipType::FIGHTER, 20);
                fleet->addShips(ShipType::CRUISER, 10);
                // fleet->setAlienTechnology(true); // Hypothetical method
                
                return std::unique_ptr<Entity>(fleet.release());
            });
            
            REQUIRE(factory.isPluginRegistered("AlienTechnology"));
            
            std::map<std::string, std::any> alienParams = {
                {"name", std::string("AlienFleet")},
                {"position", Vector3D{1000, 1000, 1000}}
            };
            
            auto alienFleet = factory.createFromPlugin("AlienTechnology", alienParams);
            
            REQUIRE(alienFleet != nullptr);
            REQUIRE(alienFleet->getType() == EntityType::FLEET);
            
            Fleet* fleetPtr = dynamic_cast<Fleet*>(alienFleet.get());
            REQUIRE(fleetPtr != nullptr);
            REQUIRE(fleetPtr->getShipCount() == 30);
        }
        
        SECTION("Factory middleware") {
            // Register middleware to modify all created entities
            factory.registerMiddleware([](std::unique_ptr<Entity>& entity, 
                                        const std::string& type,
                                        const std::map<std::string, std::any>& params) {
                if (entity) {
                    // Add creation timestamp to all entities
                    entity->setProperty("creation_time", std::chrono::system_clock::now());
                    
                    // Add factory version info
                    entity->setProperty("factory_version", std::string("1.0.0"));
                    
                    // Log entity creation
                    entity->setProperty("creation_logged", true);
                }
            });
            
            auto planet = factory.createEntity("Planet", planetParams);
            
            REQUIRE(planet != nullptr);
            REQUIRE(planet->hasProperty("creation_time"));
            REQUIRE(planet->hasProperty("factory_version"));
            REQUIRE(planet->hasProperty("creation_logged"));
            
            auto versionStr = planet->getProperty<std::string>("factory_version");
            REQUIRE(versionStr == "1.0.0");
        }
    }
}

TEST_CASE_METHOD(FactoryTestFixture, "Factory Configuration and Serialization", "[factory][config][core]") {
    
    SECTION("Factory Configuration") {
        auto& factory = EntityFactory::getInstance();
        
        SECTION("Configuration from JSON") {
            nlohmann::json factoryConfig = {
                {"default_entity_settings", {
                    {"Planet", {
                        {"default_habitability", 0.5},
                        {"default_minerals", 1000},
                        {"default_energy", 500}
                    }},
                    {"Fleet", {
                        {"default_fighters", 5},
                        {"default_cruisers", 2},
                        {"default_battleships", 1}
                    }}
                }},
                {"factory_settings", {
                    {"enable_caching", true},
                    {"max_cache_size", 1000},
                    {"enable_validation", true},
                    {"debug_mode", false}
                }}
            };
            
            factory.loadConfiguration(factoryConfig);
            
            // Test that defaults are applied
            std::map<std::string, std::any> minimalParams = {
                {"name", std::string("DefaultPlanet")},
                {"position", Vector3D{0, 0, 0}}
            };
            
            auto planet = factory.createEntity("Planet", minimalParams);
            Planet* planetPtr = dynamic_cast<Planet*>(planet.get());
            
            REQUIRE(planetPtr != nullptr);
            REQUIRE(planetPtr->getHabitabilityRating() == Approx(0.5)); // From config defaults
            REQUIRE(planetPtr->getResourceAmount(ResourceType::MINERALS) == 1000);
        }
        
        SECTION("Configuration persistence") {
            auto config = factory.getConfiguration();
            
            // Modify configuration
            config.enableCaching = false;
            config.maxCacheSize = 500;
            factory.updateConfiguration(config);
            
            // Verify changes persisted
            auto updatedConfig = factory.getConfiguration();
            REQUIRE(updatedConfig.enableCaching == false);
            REQUIRE(updatedConfig.maxCacheSize == 500);
        }
    }
    
    SECTION("Entity Template Serialization") {
        auto& factory = EntityFactory::getInstance();
        
        SECTION("Save and load entity templates") {
            EntityTemplate planetTemplate;
            planetTemplate.type = "Planet";
            planetTemplate.name = "TerranWorld";
            planetTemplate.description = "Standard human-habitable planet";
            planetTemplate.parameters = {
                {"habitability", 0.8},
                {"minerals", 1500},
                {"energy", 750},
                {"food", 400},
                {"atmosphere", std::string("breathable")},
                {"gravity", 1.0}
            };
            
            factory.saveTemplate(planetTemplate, "terran_world.json");
            
            auto loadedTemplate = factory.loadTemplate("terran_world.json");
            
            REQUIRE(loadedTemplate.type == "Planet");
            REQUIRE(loadedTemplate.name == "TerranWorld");
            REQUIRE(std::any_cast<double>(loadedTemplate.parameters.at("habitability")) == Approx(0.8));
            REQUIRE(std::any_cast<int>(loadedTemplate.parameters.at("minerals")) == 1500);
        }
        
        SECTION("Template-based entity creation") {
            std::map<std::string, std::any> templateOverrides = {
                {"name", std::string("NewTerranWorld")},
                {"position", Vector3D{500, 500, 500}},
                {"minerals", 2000} // Override template value
            };
            
            auto planet = factory.createFromTemplate("TerranWorld", templateOverrides);
            
            REQUIRE(planet != nullptr);
            REQUIRE(planet->getName() == "NewTerranWorld");
            
            Planet* planetPtr = dynamic_cast<Planet*>(planet.get());
            REQUIRE(planetPtr != nullptr);
            REQUIRE(planetPtr->getHabitabilityRating() == Approx(0.8)); // From template
            REQUIRE(planetPtr->getResourceAmount(ResourceType::MINERALS) == 2000); // Override
            REQUIRE(planetPtr->getResourceAmount(ResourceType::ENERGY) == 750); // From template
        }
    }
}