// File: tests/unit_tests/core_tests/ResourceManagerTests.cpp
// Singleton pattern tests for CppVerseHub ResourceManager functionality

#include <catch2/catch.hpp>
#include <memory>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <future>
#include <map>

// Include the resource management headers
#include "ResourceManager.hpp"
#include "Planet.hpp"
#include "Fleet.hpp"
#include "Exceptions.hpp"

using namespace CppVerseHub::Core;

/**
 * @brief Test fixture for ResourceManager tests
 */
class ResourceManagerTestFixture {
public:
    ResourceManagerTestFixture() {
        // Clean slate for each test
        resetResourceManager();
        setupTestEntities();
    }
    
    ~ResourceManagerTestFixture() {
        // Clean up test entities
        testPlanets.clear();
        testFleets.clear();
    }
    
protected:
    void resetResourceManager() {
        // Reset the singleton state if possible
        // This is implementation-dependent
        auto& manager = ResourceManager::getInstance();
        manager.reset(); // Assuming reset method exists
    }
    
    void setupTestEntities() {
        // Create test planets with resources
        auto planet1 = std::make_unique<Planet>("ResourcePlanet1", Vector3D{100, 100, 100});
        planet1->setResourceAmount(ResourceType::MINERALS, 1000);
        planet1->setResourceAmount(ResourceType::ENERGY, 500);
        planet1->setResourceAmount(ResourceType::FOOD, 300);
        planet1->setResourceProduction(ResourceType::MINERALS, 10.0);
        planet1->setResourceProduction(ResourceType::ENERGY, 5.0);
        testPlanets.push_back(std::move(planet1));
        
        auto planet2 = std::make_unique<Planet>("ResourcePlanet2", Vector3D{200, 200, 200});
        planet2->setResourceAmount(ResourceType::MINERALS, 800);
        planet2->setResourceAmount(ResourceType::ENERGY, 1200);
        planet2->setResourceAmount(ResourceType::FOOD, 600);
        planet2->setResourceProduction(ResourceType::MINERALS, 8.0);
        planet2->setResourceProduction(ResourceType::ENERGY, 12.0);
        testPlanets.push_back(std::move(planet2));
        
        // Create test fleets that consume resources
        auto fleet1 = std::make_unique<Fleet>("ConsumerFleet1", Vector3D{150, 150, 150});
        fleet1->addShips(ShipType::FIGHTER, 10);
        fleet1->addShips(ShipType::CRUISER, 5);
        testFleets.push_back(std::move(fleet1));
        
        auto fleet2 = std::make_unique<Fleet>("ConsumerFleet2", Vector3D{250, 250, 250});
        fleet2->addShips(ShipType::BATTLESHIP, 3);
        fleet2->addShips(ShipType::TRANSPORT, 7);
        testFleets.push_back(std::move(fleet2));
    }
    
    std::vector<std::unique_ptr<Planet>> testPlanets;
    std::vector<std::unique_ptr<Fleet>> testFleets;
};

TEST_CASE_METHOD(ResourceManagerTestFixture, "ResourceManager Singleton Pattern", "[resource][singleton][core]") {
    
    SECTION("Singleton Instance Creation") {
        SECTION("Single instance guarantee") {
            auto& manager1 = ResourceManager::getInstance();
            auto& manager2 = ResourceManager::getInstance();
            
            // Both references should point to the same instance
            REQUIRE(&manager1 == &manager2);
        }
        
        SECTION("Thread-safe singleton creation") {
            std::atomic<ResourceManager*> instances[10];
            std::vector<std::thread> threads;
            
            // Create multiple threads trying to get instance simultaneously
            for (int i = 0; i < 10; ++i) {
                threads.emplace_back([&instances, i]() {
                    instances[i] = &ResourceManager::getInstance();
                });
            }
            
            // Wait for all threads to complete
            for (auto& thread : threads) {
                thread.join();
            }
            
            // All instances should be the same
            ResourceManager* firstInstance = instances[0];
            for (int i = 1; i < 10; ++i) {
                REQUIRE(instances[i] == firstInstance);
            }
        }
        
        SECTION("Initialization state") {
            auto& manager = ResourceManager::getInstance();
            
            // Manager should be properly initialized
            REQUIRE(manager.isInitialized());
            REQUIRE(manager.getTotalResourceTypes() > 0);
        }
    }
    
    SECTION("Singleton Lifecycle Management") {
        SECTION("Persistent state") {
            auto& manager = ResourceManager::getInstance();
            
            // Add a resource producer
            manager.registerResourceProducer(testPlanets[0].get());
            int producerCount = manager.getProducerCount();
            
            // Get instance again - should maintain state
            auto& manager2 = ResourceManager::getInstance();
            REQUIRE(manager2.getProducerCount() == producerCount);
        }
        
        SECTION("Reset functionality") {
            auto& manager = ResourceManager::getInstance();
            
            manager.registerResourceProducer(testPlanets[0].get());
            manager.registerResourceConsumer(testFleets[0].get());
            
            REQUIRE(manager.getProducerCount() > 0);
            REQUIRE(manager.getConsumerCount() > 0);
            
            // Reset should clear all registered entities
            manager.reset();
            
            REQUIRE(manager.getProducerCount() == 0);
            REQUIRE(manager.getConsumerCount() == 0);
        }
    }
}

TEST_CASE_METHOD(ResourceManagerTestFixture, "Resource Registration and Management", "[resource][management][core]") {
    
    SECTION("Resource Producer Registration") {
        auto& manager = ResourceManager::getInstance();
        
        SECTION("Single producer registration") {
            int initialCount = manager.getProducerCount();
            manager.registerResourceProducer(testPlanets[0].get());
            
            REQUIRE(manager.getProducerCount() == initialCount + 1);
            REQUIRE(manager.isRegistered(testPlanets[0].get()));
        }
        
        SECTION("Multiple producer registration") {
            manager.registerResourceProducer(testPlanets[0].get());
            manager.registerResourceProducer(testPlanets[1].get());
            
            REQUIRE(manager.getProducerCount() == 2);
            REQUIRE(manager.isRegistered(testPlanets[0].get()));
            REQUIRE(manager.isRegistered(testPlanets[1].get()));
        }
        
        SECTION("Duplicate registration prevention") {
            manager.registerResourceProducer(testPlanets[0].get());
            int countAfterFirst = manager.getProducerCount();
            
            // Try to register same producer again
            manager.registerResourceProducer(testPlanets[0].get());
            
            // Count should not increase
            REQUIRE(manager.getProducerCount() == countAfterFirst);
        }
        
        SECTION("Invalid producer registration") {
            REQUIRE_THROWS_AS(
                manager.registerResourceProducer(nullptr),
                InvalidArgumentException
            );
        }
    }
    
    SECTION("Resource Consumer Registration") {
        auto& manager = ResourceManager::getInstance();
        
        SECTION("Fleet consumer registration") {
            int initialCount = manager.getConsumerCount();
            manager.registerResourceConsumer(testFleets[0].get());
            
            REQUIRE(manager.getConsumerCount() == initialCount + 1);
            REQUIRE(manager.isRegisteredConsumer(testFleets[0].get()));
        }
        
        SECTION("Consumer resource requirements") {
            manager.registerResourceConsumer(testFleets[0].get());
            
            auto requirements = manager.getResourceRequirements(testFleets[0].get());
            REQUIRE(requirements.find(ResourceType::ENERGY) != requirements.end());
            REQUIRE(requirements[ResourceType::ENERGY] > 0);
        }
        
        SECTION("Dynamic requirement updates") {
            manager.registerResourceConsumer(testFleets[0].get());
            auto initialReq = manager.getResourceRequirements(testFleets[0].get());
            
            // Add more ships to increase requirements
            testFleets[0]->addShips(ShipType::BATTLESHIP, 2);
            manager.updateConsumerRequirements(testFleets[0].get());
            
            auto updatedReq = manager.getResourceRequirements(testFleets[0].get());
            REQUIRE(updatedReq[ResourceType::ENERGY] > initialReq[ResourceType::ENERGY]);
        }
    }
    
    SECTION("Resource Unregistration") {
        auto& manager = ResourceManager::getInstance();
        
        manager.registerResourceProducer(testPlanets[0].get());
        manager.registerResourceConsumer(testFleets[0].get());
        
        SECTION("Producer unregistration") {
            REQUIRE(manager.isRegistered(testPlanets[0].get()));
            
            manager.unregisterResourceProducer(testPlanets[0].get());
            
            REQUIRE(!manager.isRegistered(testPlanets[0].get()));
            REQUIRE(manager.getProducerCount() == 0);
        }
        
        SECTION("Consumer unregistration") {
            REQUIRE(manager.isRegisteredConsumer(testFleets[0].get()));
            
            manager.unregisterResourceConsumer(testFleets[0].get());
            
            REQUIRE(!manager.isRegisteredConsumer(testFleets[0].get()));
            REQUIRE(manager.getConsumerCount() == 0);
        }
        
        SECTION("Unregistering non-existent entity") {
            REQUIRE_NOTHROW(manager.unregisterResourceProducer(testPlanets[1].get()));
            REQUIRE_NOTHROW(manager.unregisterResourceConsumer(testFleets[1].get()));
        }
    }
}

TEST_CASE_METHOD(ResourceManagerTestFixture, "Resource Production and Distribution", "[resource][production][core]") {
    
    SECTION("Global Resource Tracking") {
        auto& manager = ResourceManager::getInstance();
        
        manager.registerResourceProducer(testPlanets[0].get());
        manager.registerResourceProducer(testPlanets[1].get());
        
        SECTION("Total resource calculation") {
            auto totals = manager.getTotalResources();
            
            int expectedMinerals = testPlanets[0]->getResourceAmount(ResourceType::MINERALS) +
                                  testPlanets[1]->getResourceAmount(ResourceType::MINERALS);
            int expectedEnergy = testPlanets[0]->getResourceAmount(ResourceType::ENERGY) +
                                testPlanets[1]->getResourceAmount(ResourceType::ENERGY);
            
            REQUIRE(totals[ResourceType::MINERALS] == expectedMinerals);
            REQUIRE(totals[ResourceType::ENERGY] == expectedEnergy);
        }
        
        SECTION("Production rate calculation") {
            auto productionRates = manager.getTotalProductionRates();
            
            double expectedMineralRate = testPlanets[0]->getResourceProduction(ResourceType::MINERALS) +
                                        testPlanets[1]->getResourceProduction(ResourceType::MINERALS);
            double expectedEnergyRate = testPlanets[0]->getResourceProduction(ResourceType::ENERGY) +
                                       testPlanets[1]->getResourceProduction(ResourceType::ENERGY);
            
            REQUIRE(productionRates[ResourceType::MINERALS] == Approx(expectedMineralRate));
            REQUIRE(productionRates[ResourceType::ENERGY] == Approx(expectedEnergyRate));
        }
        
        SECTION("Resource availability check") {
            REQUIRE(manager.isResourceAvailable(ResourceType::MINERALS, 500));
            REQUIRE(manager.isResourceAvailable(ResourceType::ENERGY, 1000));
            REQUIRE(!manager.isResourceAvailable(ResourceType::MINERALS, 5000)); // Too much
        }
    }
    
    SECTION("Resource Allocation System") {
        auto& manager = ResourceManager::getInstance();
        
        manager.registerResourceProducer(testPlanets[0].get());
        manager.registerResourceProducer(testPlanets[1].get());
        manager.registerResourceConsumer(testFleets[0].get());
        manager.registerResourceConsumer(testFleets[1].get());
        
        SECTION("Simple resource allocation") {
            ResourceRequest request;
            request.requesterId = testFleets[0]->getId();
            request.resourceType = ResourceType::ENERGY;
            request.amount = 100;
            request.priority = RequestPriority::NORMAL;
            
            bool allocated = manager.allocateResource(request);
            REQUIRE(allocated);
            
            // Global resources should be reduced
            auto totals = manager.getTotalResources();
            REQUIRE(totals[ResourceType::ENERGY] < 
                   testPlanets[0]->getResourceAmount(ResourceType::ENERGY) +
                   testPlanets[1]->getResourceAmount(ResourceType::ENERGY));
        }
        
        SECTION("Priority-based allocation") {
            ResourceRequest highPriorityRequest;
            highPriorityRequest.requesterId = testFleets[0]->getId();
            highPriorityRequest.resourceType = ResourceType::MINERALS;
            highPriorityRequest.amount = 1500; // Most available minerals
            highPriorityRequest.priority = RequestPriority::HIGH;
            
            ResourceRequest lowPriorityRequest;
            lowPriorityRequest.requesterId = testFleets[1]->getId();
            lowPriorityRequest.resourceType = ResourceType::MINERALS;
            lowPriorityRequest.amount = 500;
            lowPriorityRequest.priority = RequestPriority::LOW;
            
            // Submit low priority first
            manager.submitResourceRequest(lowPriorityRequest);
            manager.submitResourceRequest(highPriorityRequest);
            
            // Process requests - high priority should be fulfilled first
            manager.processResourceRequests();
            
            auto allocationResults = manager.getAllocationResults();
            auto highResult = allocationResults.find(highPriorityRequest.requesterId);
            auto lowResult = allocationResults.find(lowPriorityRequest.requesterId);
            
            REQUIRE(highResult != allocationResults.end());
            REQUIRE(highResult->second.fullyAllocated);
            
            if (lowResult != allocationResults.end()) {
                // Low priority might be partially fulfilled or rejected
                REQUIRE(lowResult->second.allocatedAmount <= lowPriorityRequest.amount);
            }
        }
        
        SECTION("Insufficient resources handling") {
            ResourceRequest excessiveRequest;
            excessiveRequest.requesterId = testFleets[0]->getId();
            excessiveRequest.resourceType = ResourceType::FOOD;
            excessiveRequest.amount = 10000; // More than available
            excessiveRequest.priority = RequestPriority::CRITICAL;
            
            bool allocated = manager.allocateResource(excessiveRequest);
            REQUIRE(!allocated);
        }
    }
    
    SECTION("Resource Distribution Strategies") {
        auto& manager = ResourceManager::getInstance();
        
        manager.registerResourceProducer(testPlanets[0].get());
        manager.registerResourceProducer(testPlanets[1].get());
        manager.registerResourceConsumer(testFleets[0].get());
        manager.registerResourceConsumer(testFleets[1].get());
        
        SECTION("Fair distribution strategy") {
            manager.setDistributionStrategy(DistributionStrategy::FAIR);
            
            ResourceRequest request1;
            request1.requesterId = testFleets[0]->getId();
            request1.resourceType = ResourceType::ENERGY;
            request1.amount = 400;
            
            ResourceRequest request2;
            request2.requesterId = testFleets[1]->getId();
            request2.resourceType = ResourceType::ENERGY;
            request2.amount = 400;
            
            manager.submitResourceRequest(request1);
            manager.submitResourceRequest(request2);
            manager.processResourceRequests();
            
            auto results = manager.getAllocationResults();
            
            // Both should get fair share
            REQUIRE(results[request1.requesterId].allocatedAmount > 0);
            REQUIRE(results[request2.requesterId].allocatedAmount > 0);
            
            // Allocations should be similar (fair)
            double allocation1 = results[request1.requesterId].allocatedAmount;
            double allocation2 = results[request2.requesterId].allocatedAmount;
            REQUIRE(std::abs(allocation1 - allocation2) < 100); // Within reasonable range
        }
        
        SECTION("Priority-based distribution strategy") {
            manager.setDistributionStrategy(DistributionStrategy::PRIORITY);
            
            ResourceRequest highPriority;
            highPriority.requesterId = testFleets[0]->getId();
            highPriority.resourceType = ResourceType::ENERGY;
            highPriority.amount = 600;
            highPriority.priority = RequestPriority::HIGH;
            
            ResourceRequest normalPriority;
            normalPriority.requesterId = testFleets[1]->getId();
            normalPriority.resourceType = ResourceType::ENERGY;
            normalPriority.amount = 600;
            normalPriority.priority = RequestPriority::NORMAL;
            
            manager.submitResourceRequest(normalPriority);
            manager.submitResourceRequest(highPriority);
            manager.processResourceRequests();
            
            auto results = manager.getAllocationResults();
            
            // High priority should get better allocation
            double highAllocation = results[highPriority.requesterId].allocatedAmount;
            double normalAllocation = results[normalPriority.requesterId].allocatedAmount;
            
            REQUIRE(highAllocation >= normalAllocation);
        }
    }
}

TEST_CASE_METHOD(ResourceManagerTestFixture, "Resource Flow Simulation", "[resource][simulation][core]") {
    
    SECTION("Production Cycle Simulation") {
        auto& manager = ResourceManager::getInstance();
        
        manager.registerResourceProducer(testPlanets[0].get());
        manager.registerResourceProducer(testPlanets[1].get());
        
        SECTION("Single cycle production") {
            auto initialTotals = manager.getTotalResources();
            
            // Simulate one production cycle
            manager.simulateProductionCycle(1.0); // 1 time unit
            
            auto afterTotals = manager.getTotalResources();
            
            // Resources should have increased
            REQUIRE(afterTotals[ResourceType::MINERALS] > initialTotals[ResourceType::MINERALS]);
            REQUIRE(afterTotals[ResourceType::ENERGY] > initialTotals[ResourceType::ENERGY]);
            
            // Increase should match production rates
            double mineralIncrease = afterTotals[ResourceType::MINERALS] - initialTotals[ResourceType::MINERALS];
            double expectedIncrease = testPlanets[0]->getResourceProduction(ResourceType::MINERALS) +
                                     testPlanets[1]->getResourceProduction(ResourceType::MINERALS);
            
            REQUIRE(mineralIncrease == Approx(expectedIncrease));
        }
        
        SECTION("Multiple cycle simulation") {
            auto initialTotals = manager.getTotalResources();
            
            // Simulate 10 production cycles
            for (int i = 0; i < 10; ++i) {
                manager.simulateProductionCycle(1.0);
            }
            
            auto finalTotals = manager.getTotalResources();
            
            // Check cumulative production
            double mineralIncrease = finalTotals[ResourceType::MINERALS] - initialTotals[ResourceType::MINERALS];
            double expectedIncrease = 10 * (testPlanets[0]->getResourceProduction(ResourceType::MINERALS) +
                                           testPlanets[1]->getResourceProduction(ResourceType::MINERALS));
            
            REQUIRE(mineralIncrease == Approx(expectedIncrease));
        }
    }
    
    SECTION("Consumption Simulation") {
        auto& manager = ResourceManager::getInstance();
        
        manager.registerResourceProducer(testPlanets[0].get());
        manager.registerResourceConsumer(testFleets[0].get());
        
        SECTION("Continuous consumption") {
            auto initialTotals = manager.getTotalResources();
            
            // Set up continuous consumption
            manager.setContinuousConsumption(testFleets[0]->getId(), ResourceType::ENERGY, 5.0);
            
            // Simulate with consumption
            manager.simulateResourceFlow(10.0); // 10 time units
            
            auto finalTotals = manager.getTotalResources();
            
            // Net change should be production - consumption
            double netEnergyChange = finalTotals[ResourceType::ENERGY] - initialTotals[ResourceType::ENERGY];
            double expectedChange = 10.0 * (testPlanets[0]->getResourceProduction(ResourceType::ENERGY) - 5.0);
            
            REQUIRE(netEnergyChange == Approx(expectedChange));
        }
        
        SECTION("Supply and demand balance") {
            manager.registerResourceConsumer(testFleets[1].get());
            
            // Set high consumption
            manager.setContinuousConsumption(testFleets[0]->getId(), ResourceType::ENERGY, 8.0);
            manager.setContinuousConsumption(testFleets[1]->getId(), ResourceType::ENERGY, 7.0);
            
            auto productionRates = manager.getTotalProductionRates();
            double totalConsumption = 15.0;
            
            if (productionRates[ResourceType::ENERGY] < totalConsumption) {
                // Consumption exceeds production - should trigger shortage
                manager.simulateResourceFlow(5.0);
                
                REQUIRE(manager.hasResourceShortage(ResourceType::ENERGY));
                
                auto shortageInfo = manager.getShortageInfo(ResourceType::ENERGY);
                REQUIRE(shortageInfo.severity > 0.0);
            }
        }
    }
    
    SECTION("Economic Simulation") {
        auto& manager = ResourceManager::getInstance();
        
        manager.registerResourceProducer(testPlanets[0].get());
        manager.registerResourceProducer(testPlanets[1].get());
        manager.registerResourceConsumer(testFleets[0].get());
        manager.registerResourceConsumer(testFleets[1].get());
        
        SECTION("Resource trading simulation") {
            manager.enableResourceTrading(true);
            
            // Create resource imbalance
            testPlanets[0]->setResourceAmount(ResourceType::MINERALS, 5000);
            testPlanets[0]->setResourceAmount(ResourceType::ENERGY, 100);
            
            testPlanets[1]->setResourceAmount(ResourceType::MINERALS, 100);
            testPlanets[1]->setResourceAmount(ResourceType::ENERGY, 5000);
            
            // Simulate trading
            manager.simulateTradingCycle(1.0);
            
            // Resources should be more balanced after trading
            auto planet1Resources = manager.getEntityResources(testPlanets[0]->getId());
            auto planet2Resources = manager.getEntityResources(testPlanets[1]->getId());
            
            // Some trading should have occurred
            REQUIRE(planet1Resources[ResourceType::ENERGY] > 100);
            REQUIRE(planet2Resources[ResourceType::MINERALS] > 100);
        }
        
        SECTION("Resource price dynamics") {
            manager.enableDynamicPricing(true);
            
            auto initialPrices = manager.getResourcePrices();
            
            // Create artificial scarcity
            manager.consumeResource(ResourceType::ENERGY, 1500);
            
            // Simulate market response
            manager.updateResourcePrices();
            
            auto updatedPrices = manager.getResourcePrices();
            
            // Energy price should have increased due to scarcity
            REQUIRE(updatedPrices[ResourceType::ENERGY] > initialPrices[ResourceType::ENERGY]);
        }
    }
}

TEST_CASE_METHOD(ResourceManagerTestFixture, "Thread Safety and Concurrency", "[resource][concurrency][core]") {
    
    SECTION("Concurrent Resource Access") {
        auto& manager = ResourceManager::getInstance();
        
        manager.registerResourceProducer(testPlanets[0].get());
        manager.registerResourceProducer(testPlanets[1].get());
        
        SECTION("Concurrent resource allocation") {
            const int numThreads = 10;
            const int allocationsPerThread = 100;
            
            std::vector<std::future<int>> futures;
            std::atomic<int> successfulAllocations{0};
            
            for (int t = 0; t < numThreads; ++t) {
                futures.push_back(std::async(std::launch::async, [&, t]() {
                    int successes = 0;
                    for (int i = 0; i < allocationsPerThread; ++i) {
                        ResourceRequest request;
                        request.requesterId = t * allocationsPerThread + i;
                        request.resourceType = ResourceType::ENERGY;
                        request.amount = 1; // Small amounts
                        request.priority = RequestPriority::NORMAL;
                        
                        if (manager.allocateResource(request)) {
                            successes++;
                        }
                    }
                    return successes;
                }));
            }
            
            // Wait for all threads and sum results
            int totalSuccesses = 0;
            for (auto& future : futures) {
                totalSuccesses += future.get();
            }
            
            // Some allocations should succeed without corruption
            REQUIRE(totalSuccesses > 0);
            REQUIRE(totalSuccesses <= numThreads * allocationsPerThread);
            
            // Resource state should be consistent
            auto totals = manager.getTotalResources();
            REQUIRE(totals[ResourceType::ENERGY] >= 0);
        }
        
        SECTION("Concurrent producer registration") {
            const int numThreads = 20;
            std::vector<std::thread> threads;
            std::vector<std::unique_ptr<Planet>> concurrentPlanets;
            
            // Create planets for concurrent registration
            for (int i = 0; i < numThreads; ++i) {
                concurrentPlanets.push_back(
                    std::make_unique<Planet>("ConcurrentPlanet" + std::to_string(i),
                                           Vector3D{i * 100, i * 100, i * 100})
                );
            }
            
            // Register producers concurrently
            for (int i = 0; i < numThreads; ++i) {
                threads.emplace_back([&manager, &concurrentPlanets, i]() {
                    manager.registerResourceProducer(concurrentPlanets[i].get());
                });
            }
            
            // Wait for all registrations
            for (auto& thread : threads) {
                thread.join();
            }
            
            // All producers should be registered
            REQUIRE(manager.getProducerCount() >= numThreads);
            
            for (int i = 0; i < numThreads; ++i) {
                REQUIRE(manager.isRegistered(concurrentPlanets[i].get()));
            }
        }
    }
    
    SECTION("Resource Flow Synchronization") {
        auto& manager = ResourceManager::getInstance();
        
        manager.registerResourceProducer(testPlanets[0].get());
        manager.registerResourceConsumer(testFleets[0].get());
        
        SECTION("Concurrent production and consumption") {
            std::atomic<bool> stopSimulation{false};
            std::vector<std::thread> simulationThreads;
            
            // Producer thread
            simulationThreads.emplace_back([&]() {
                while (!stopSimulation.load()) {
                    manager.simulateProductionCycle(0.1);
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            });
            
            // Consumer thread
            simulationThreads.emplace_back([&]() {
                while (!stopSimulation.load()) {
                    ResourceRequest request;
                    request.requesterId = testFleets[0]->getId();
                    request.resourceType = ResourceType::ENERGY;
                    request.amount = 5;
                    request.priority = RequestPriority::NORMAL;
                    
                    manager.allocateResource(request);
                    std::this_thread::sleep_for(std::chrono::milliseconds(15));
                }
            });
            
            // Monitor thread
            std::atomic<int> monitoringCount{0};
            simulationThreads.emplace_back([&]() {
                while (!stopSimulation.load()) {
                    auto totals = manager.getTotalResources();
                    monitoringCount++;
                    std::this_thread::sleep_for(std::chrono::milliseconds(20));
                }
            });
            
            // Run simulation for a short time
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            stopSimulation = true;
            
            // Wait for all threads
            for (auto& thread : simulationThreads) {
                thread.join();
            }
            
            // System should remain stable
            auto finalTotals = manager.getTotalResources();
            REQUIRE(finalTotals[ResourceType::ENERGY] >= 0);
            REQUIRE(monitoringCount > 0);
        }
    }
}

TEST_CASE_METHOD(ResourceManagerTestFixture, "Performance and Optimization", "[resource][performance][core]") {
    
    SECTION("Large Scale Operations") {
        auto& manager = ResourceManager::getInstance();
        
        SECTION("Bulk producer registration") {
            const int producerCount = 1000;
            std::vector<std::unique_ptr<Planet>> bulkPlanets;
            
            // Create bulk planets
            for (int i = 0; i < producerCount; ++i) {
                bulkPlanets.push_back(
                    std::make_unique<Planet>("BulkPlanet" + std::to_string(i),
                                           Vector3D{i, i, i})
                );
                bulkPlanets[i]->setResourceAmount(ResourceType::MINERALS, 100 + i);
            }
            
            auto duration = measurePerformance([&]() {
                for (auto& planet : bulkPlanets) {
                    manager.registerResourceProducer(planet.get());
                }
            }, "Registering " + std::to_string(producerCount) + " producers");
            
            REQUIRE(duration.count() < 500000); // 500ms threshold
            REQUIRE(manager.getProducerCount() >= producerCount);
        }
        
        SECTION("Bulk resource calculations") {
            // Register many producers first
            for (int i = 0; i < 500; ++i) {
                auto planet = std::make_unique<Planet>("CalcPlanet" + std::to_string(i),
                                                     Vector3D{i, i, i});
                planet->setResourceAmount(ResourceType::MINERALS, 100);
                planet->setResourceAmount(ResourceType::ENERGY, 50);
                planet->setResourceProduction(ResourceType::MINERALS, 1.0);
                planet->setResourceProduction(ResourceType::ENERGY, 0.5);
                
                manager.registerResourceProducer(planet.get());
                testPlanets.push_back(std::move(planet));
            }
            
            auto duration = measurePerformance([&]() {
                auto totals = manager.getTotalResources();
                auto rates = manager.getTotalProductionRates();
            }, "Calculating totals for 500 producers");
            
            REQUIRE(duration.count() < 100000); // 100ms threshold
        }
        
        SECTION("High frequency allocation") {
            manager.registerResourceProducer(testPlanets[0].get());
            
            const int requestCount = 10000;
            int successfulAllocations = 0;
            
            auto duration = measurePerformance([&]() {
                for (int i = 0; i < requestCount; ++i) {
                    ResourceRequest request;
                    request.requesterId = i;
                    request.resourceType = ResourceType::ENERGY;
                    request.amount = 1;
                    request.priority = RequestPriority::NORMAL;
                    
                    if (manager.allocateResource(request)) {
                        successfulAllocations++;
                    }
                }
            }, "Processing " + std::to_string(requestCount) + " allocation requests");
            
            REQUIRE(duration.count() < 2000000); // 2s threshold
            REQUIRE(successfulAllocations > 0);
        }
    }
    
    SECTION("Memory Usage Optimization") {
        auto& manager = ResourceManager::getInstance();
        
        MemoryTracker::printMemoryStats("Before bulk resource operations");
        
        {
            // Create large number of entities
            std::vector<std::unique_ptr<Planet>> memoryTestPlanets;
            for (int i = 0; i < 5000; ++i) {
                memoryTestPlanets.push_back(
                    std::make_unique<Planet>("MemPlanet" + std::to_string(i),
                                           Vector3D{i, i, i})
                );
                manager.registerResourceProducer(memoryTestPlanets[i].get());
            }
            
            MemoryTracker::printMemoryStats("After creating 5000 producers");
            
            // Perform operations
            auto totals = manager.getTotalResources();
            manager.simulateProductionCycle(1.0);
            
            MemoryTracker::printMemoryStats("After resource operations");
            
        } // Entities destroyed here
        
        manager.reset();
        MemoryTracker::printMemoryStats("After cleanup");
    }
}

TEST_CASE_METHOD(ResourceManagerTestFixture, "Error Handling and Edge Cases", "[resource][exceptions][core]") {
    
    SECTION("Invalid Operations") {
        auto& manager = ResourceManager::getInstance();
        
        SECTION("Null pointer handling") {
            REQUIRE_THROWS_AS(
                manager.registerResourceProducer(nullptr),
                InvalidArgumentException
            );
            
            REQUIRE_THROWS_AS(
                manager.registerResourceConsumer(nullptr),
                InvalidArgumentException
            );
        }
        
        SECTION("Invalid resource requests") {
            ResourceRequest invalidRequest;
            invalidRequest.requesterId = 0; // Invalid ID
            invalidRequest.resourceType = ResourceType::MINERALS;
            invalidRequest.amount = -100; // Negative amount
            
            REQUIRE_THROWS_AS(
                manager.allocateResource(invalidRequest),
                InvalidArgumentException
            );
        }
        
        SECTION("Operations on unregistered entities") {
            REQUIRE_THROWS_AS(
                manager.getEntityResources(99999), // Non-existent ID
                EntityNotFoundException
            );
            
            REQUIRE_THROWS_AS(
                manager.updateConsumerRequirements(testFleets[0].get()), // Not registered
                InvalidOperationException
            );
        }
    }
    
    SECTION("Resource Overflow and Underflow") {
        auto& manager = ResourceManager::getInstance();
        manager.registerResourceProducer(testPlanets[0].get());
        
        SECTION("Maximum resource limits") {
            // Try to allocate more than maximum
            ResourceRequest excessiveRequest;
            excessiveRequest.requesterId = 1;
            excessiveRequest.resourceType = ResourceType::MINERALS;
            excessiveRequest.amount = std::numeric_limits<int>::max();
            
            REQUIRE_THROWS_AS(
                manager.allocateResource(excessiveRequest),
                ResourceOverflowException
            );
        }
        
        SECTION("Negative resource prevention") {
            // Consume all resources
            auto totals = manager.getTotalResources();
            manager.consumeResource(ResourceType::ENERGY, totals[ResourceType::ENERGY]);
            
            // Try to consume more
            REQUIRE_THROWS_AS(
                manager.consumeResource(ResourceType::ENERGY, 100),
                InsufficientResourceException
            );
        }
    }
    
    SECTION("Concurrent Access Conflicts") {
        auto& manager = ResourceManager::getInstance();
        manager.registerResourceProducer(testPlanets[0].get());
        
        SECTION("Race condition handling") {
            const int threadCount = 5;
            const int operationsPerThread = 200;
            
            std::vector<std::thread> conflictThreads;
            std::atomic<int> exceptions{0};
            
            for (int t = 0; t < threadCount; ++t) {
                conflictThreads.emplace_back([&, t]() {
                    for (int i = 0; i < operationsPerThread; ++i) {
                        try {
                            // Mix different operations to create conflicts
                            if (i % 3 == 0) {
                                auto totals = manager.getTotalResources();
                            } else if (i % 3 == 1) {
                                ResourceRequest request;
                                request.requesterId = t * operationsPerThread + i;
                                request.resourceType = ResourceType::ENERGY;
                                request.amount = 1;
                                manager.allocateResource(request);
                            } else {
                                manager.simulateProductionCycle(0.01);
                            }
                        } catch (const std::exception&) {
                            exceptions++;
                        }
                    }
                });
            }
            
            for (auto& thread : conflictThreads) {
                thread.join();
            }
            
            // System should handle conflicts gracefully
            REQUIRE(exceptions < threadCount * operationsPerThread / 2); // Not too many exceptions
            
            // Final state should be consistent
            auto finalTotals = manager.getTotalResources();
            REQUIRE(finalTotals[ResourceType::ENERGY] >= 0);
        }
    }
}