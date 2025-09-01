// File: tests/integration_tests/SimulationIntegrationTests.cpp
// Full simulation integration tests for CppVerseHub showcase

#include <catch2/catch.hpp>
#include <memory>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <atomic>
#include <algorithm>
#include <random>

// Include all major system components
#include "SimulationEngine.hpp"
#include "Galaxy.hpp"
#include "Planet.hpp"
#include "Fleet.hpp"
#include "Mission.hpp"
#include "ResourceManager.hpp"
#include "EventSystem.hpp"
#include "ThreadPool.hpp"
#include "ConfigManager.hpp"
#include "Logger.hpp"
#include "MemoryTracker.hpp"

using namespace CppVerseHub::Core;
using namespace CppVerseHub::Simulation;
using namespace CppVerseHub::Events;
using namespace CppVerseHub::Concurrency;
using namespace CppVerseHub::Utils;

/**
 * @brief Test fixture for simulation integration tests
 */
class SimulationIntegrationTestFixture {
public:
    SimulationIntegrationTestFixture() {
        MemoryTracker::resetCounters();
        
        // Initialize configuration
        auto& config = ConfigManager::getInstance();
        config.setProperty("simulation.timestep", "0.1");
        config.setProperty("simulation.max_threads", "4");
        config.setProperty("galaxy.size_x", "1000.0");
        config.setProperty("galaxy.size_y", "1000.0");
        config.setProperty("galaxy.size_z", "1000.0");
        config.setProperty("planet.min_resources", "100");
        config.setProperty("fleet.max_speed", "50.0");
        
        // Initialize logger
        auto& logger = Logger::getInstance();
        logger.setLogLevel(Logger::LogLevel::INFO);
        
        setupTestEnvironment();
    }
    
    ~SimulationIntegrationTestFixture() {
        cleanup();
        MemoryTracker::printMemoryStats("Simulation integration test completion");
    }
    
protected:
    void setupTestEnvironment() {
        // Create test galaxy
        testGalaxy = std::make_unique<Galaxy>("TestGalaxy", 1000.0, 1000.0, 1000.0);
        
        // Create test planets
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> posDis(0.0, 1000.0);
        std::uniform_int_distribution<> resDis(500, 2000);
        
        for (int i = 0; i < 20; ++i) {
            auto planet = std::make_unique<Planet>(
                "IntegrationPlanet_" + std::to_string(i),
                Vector3D{posDis(gen), posDis(gen), posDis(gen)}
            );
            
            planet->setResourceAmount(ResourceType::MINERALS, resDis(gen));
            planet->setResourceAmount(ResourceType::ENERGY, resDis(gen));
            planet->setHabitabilityRating(0.3 + (i % 7) * 0.1);
            
            testPlanets.push_back(planet.get());
            testGalaxy->addPlanet(std::move(planet));
        }
        
        // Create test fleets
        for (int i = 0; i < 8; ++i) {
            auto fleet = std::make_unique<Fleet>(
                "IntegrationFleet_" + std::to_string(i),
                Vector3D{posDis(gen), posDis(gen), posDis(gen)}
            );
            
            fleet->addShips(ShipType::FIGHTER, 10 + i * 3);
            fleet->addShips(ShipType::CRUISER, 2 + i);
            if (i % 3 == 0) {
                fleet->addShips(ShipType::BATTLESHIP, 1 + i / 3);
            }
            
            testFleets.push_back(fleet.get());
            testGalaxy->addFleet(std::move(fleet));
        }
        
        // Create test missions
        for (int i = 0; i < 5; ++i) {
            if (i < testPlanets.size()) {
                auto mission = std::make_unique<ExploreMission>(
                    "IntegrationMission_" + std::to_string(i),
                    testPlanets[i]
                );
                
                testMissions.push_back(mission.get());
                testGalaxy->addMission(std::move(mission));
            }
        }
    }
    
    void cleanup() {
        testPlanets.clear();
        testFleets.clear();
        testMissions.clear();
        testGalaxy.reset();
    }
    
    std::unique_ptr<Galaxy> testGalaxy;
    std::vector<Planet*> testPlanets;
    std::vector<Fleet*> testFleets;
    std::vector<Mission*> testMissions;
};

TEST_CASE_METHOD(SimulationIntegrationTestFixture, "Basic Simulation Engine Integration", "[integration][simulation][basic]") {
    
    SECTION("Engine initialization and startup") {
        SimulationEngine engine;
        
        REQUIRE(engine.getState() == SimulationState::STOPPED);
        
        // Initialize with test galaxy
        engine.initialize(std::move(testGalaxy));
        REQUIRE(engine.getState() == SimulationState::INITIALIZED);
        
        // Start simulation
        engine.start();
        REQUIRE(engine.getState() == SimulationState::RUNNING);
        
        // Run for a short time
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Stop simulation
        engine.stop();
        REQUIRE(engine.getState() == SimulationState::STOPPED);
        
        // Get final stats
        auto stats = engine.getSimulationStats();
        REQUIRE(stats.totalUpdates > 0);
        REQUIRE(stats.averageFrameTime > 0.0);
    }
    
    SECTION("Multi-system simulation") {
        SimulationEngine engine;
        engine.initialize(std::move(testGalaxy));
        
        // Register event handlers
        std::atomic<int> planetUpdates{0};
        std::atomic<int> fleetUpdates{0};
        std::atomic<int> missionUpdates{0};
        
        auto& eventSystem = EventSystem::getInstance();
        
        eventSystem.subscribe<PlanetUpdateEvent>([&planetUpdates](const PlanetUpdateEvent& event) {
            planetUpdates.fetch_add(1);
        });
        
        eventSystem.subscribe<FleetUpdateEvent>([&fleetUpdates](const FleetUpdateEvent& event) {
            fleetUpdates.fetch_add(1);
        });
        
        eventSystem.subscribe<MissionUpdateEvent>([&missionUpdates](const MissionUpdateEvent& event) {
            missionUpdates.fetch_add(1);
        });
        
        engine.start();
        
        // Run simulation for multiple frames
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        engine.stop();
        
        // Verify all systems were updated
        REQUIRE(planetUpdates.load() > 0);
        REQUIRE(fleetUpdates.load() > 0);
        REQUIRE(missionUpdates.load() > 0);
        
        auto stats = engine.getSimulationStats();
        INFO("Simulation ran for " << stats.totalUpdates << " updates");
        INFO("Average frame time: " << stats.averageFrameTime << "ms");
        INFO("Planet updates: " << planetUpdates.load());
        INFO("Fleet updates: " << fleetUpdates.load());
        INFO("Mission updates: " << missionUpdates.load());
    }
    
    SECTION("Resource management integration") {
        auto& resourceManager = ResourceManager::getInstance();
        resourceManager.reset();
        
        // Register all planets as resource producers
        for (Planet* planet : testPlanets) {
            resourceManager.registerResourceProducer(planet);
        }
        
        REQUIRE(resourceManager.getProducerCount() == testPlanets.size());
        
        // Register all fleets as resource consumers
        for (Fleet* fleet : testFleets) {
            resourceManager.registerResourceConsumer(fleet);
        }
        
        REQUIRE(resourceManager.getConsumerCount() == testFleets.size());
        
        // Run resource management cycle
        SimulationEngine engine;
        engine.initialize(std::move(testGalaxy));
        engine.start();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        
        engine.stop();
        
        // Verify resource management occurred
        auto totalResources = resourceManager.getTotalResources();
        REQUIRE(totalResources[ResourceType::MINERALS] > 0);
        REQUIRE(totalResources[ResourceType::ENERGY] > 0);
        
        resourceManager.reset();
    }
}

TEST_CASE_METHOD(SimulationIntegrationTestFixture, "Concurrent Simulation Systems", "[integration][simulation][concurrent]") {
    
    SECTION("Multi-threaded simulation execution") {
        ThreadPool threadPool(4);
        SimulationEngine engine;
        
        engine.initialize(std::move(testGalaxy));
        engine.setThreadPool(&threadPool);
        
        std::atomic<int> taskExecutions{0};
        std::atomic<int> systemUpdates{0};
        
        // Submit concurrent simulation tasks
        for (int i = 0; i < 20; ++i) {
            threadPool.enqueue([&taskExecutions, &systemUpdates, i]() {
                taskExecutions.fetch_add(1);
                
                // Simulate some work
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                
                systemUpdates.fetch_add(1);
            });
        }
        
        engine.start();
        
        // Wait for tasks and simulation
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        
        engine.stop();
        threadPool.shutdown();
        
        REQUIRE(taskExecutions.load() == 20);
        REQUIRE(systemUpdates.load() == 20);
        
        auto stats = engine.getSimulationStats();
        REQUIRE(stats.totalUpdates > 0);
        INFO("Concurrent simulation completed " << stats.totalUpdates << " updates");
    }
    
    SECTION("Thread-safe resource access") {
        const int threadCount = 8;
        const int operationsPerThread = 100;
        
        std::atomic<int> totalOperations{0};
        std::atomic<int> resourceModifications{0};
        
        std::vector<std::thread> threads;
        
        for (int i = 0; i < threadCount; ++i) {
            threads.emplace_back([this, &totalOperations, &resourceModifications, operationsPerThread, i] {
                for (int j = 0; j < operationsPerThread; ++j) {
                    // Access different planets safely
                    Planet* planet = testPlanets[j % testPlanets.size()];
                    
                    // Simulate resource operations
                    int currentMinerals = planet->getResourceAmount(ResourceType::MINERALS);
                    planet->setResourceAmount(ResourceType::MINERALS, currentMinerals + 1);
                    
                    resourceModifications.fetch_add(1);
                    totalOperations.fetch_add(1);
                }
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        REQUIRE(totalOperations.load() == threadCount * operationsPerThread);
        REQUIRE(resourceModifications.load() == threadCount * operationsPerThread);
        
        // Verify resource consistency
        int totalMinerals = 0;
        for (Planet* planet : testPlanets) {
            totalMinerals += planet->getResourceAmount(ResourceType::MINERALS);
        }
        
        REQUIRE(totalMinerals > 0);
        INFO("Total minerals after concurrent access: " << totalMinerals);
    }
    
    SECTION("Event system stress test") {
        auto& eventSystem = EventSystem::getInstance();
        
        std::atomic<int> eventsPublished{0};
        std::atomic<int> eventsReceived{0};
        
        // Register multiple subscribers
        const int subscriberCount = 10;
        std::vector<std::function<void()>> unsubscribers;
        
        for (int i = 0; i < subscriberCount; ++i) {
            auto unsubscribe = eventSystem.subscribe<PlanetUpdateEvent>(
                [&eventsReceived](const PlanetUpdateEvent& event) {
                    eventsReceived.fetch_add(1);
                });
            unsubscribers.push_back(unsubscribe);
        }
        
        // Publish events from multiple threads
        const int publisherThreads = 5;
        const int eventsPerThread = 50;
        
        std::vector<std::thread> publishers;
        
        for (int i = 0; i < publisherThreads; ++i) {
            publishers.emplace_back([&eventSystem, &eventsPublished, eventsPerThread, this, i] {
                for (int j = 0; j < eventsPerThread; ++j) {
                    Planet* planet = testPlanets[j % testPlanets.size()];
                    PlanetUpdateEvent event{planet->getName(), planet->getPosition(), 0.1};
                    
                    eventSystem.publish(event);
                    eventsPublished.fetch_add(1);
                }
            });
        }
        
        for (auto& publisher : publishers) {
            publisher.join();
        }
        
        // Allow time for event processing
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        REQUIRE(eventsPublished.load() == publisherThreads * eventsPerThread);
        REQUIRE(eventsReceived.load() == eventsPublished.load() * subscriberCount);
        
        // Cleanup subscribers
        for (auto& unsubscribe : unsubscribers) {
            unsubscribe();
        }
    }
}

TEST_CASE_METHOD(SimulationIntegrationTestFixture, "Complex Simulation Scenarios", "[integration][simulation][complex]") {
    
    SECTION("Fleet mission execution") {
        SimulationEngine engine;
        engine.initialize(std::move(testGalaxy));
        
        // Assign fleets to missions
        for (size_t i = 0; i < std::min(testFleets.size(), testMissions.size()); ++i) {
            testMissions[i]->assignFleet(testFleets[i]);
        }
        
        std::atomic<int> missionsCompleted{0};
        std::atomic<int> fleetMovements{0};
        
        auto& eventSystem = EventSystem::getInstance();
        
        // Track mission completions
        auto missionUnsubscribe = eventSystem.subscribe<MissionCompleteEvent>(
            [&missionsCompleted](const MissionCompleteEvent& event) {
                missionsCompleted.fetch_add(1);
            });
        
        // Track fleet movements
        auto fleetUnsubscribe = eventSystem.subscribe<FleetMoveEvent>(
            [&fleetMovements](const FleetMoveEvent& event) {
                fleetMovements.fetch_add(1);
            });
        
        engine.start();
        
        // Run simulation until missions progress
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        engine.stop();
        
        // Check mission progress
        int inProgressMissions = 0;
        for (Mission* mission : testMissions) {
            if (mission->getStatus() == MissionStatus::IN_PROGRESS) {
                inProgressMissions++;
            }
        }
        
        REQUIRE(inProgressMissions > 0);
        REQUIRE(fleetMovements.load() > 0);
        INFO("Missions in progress: " << inProgressMissions);
        INFO("Fleet movements: " << fleetMovements.load());
        INFO("Missions completed: " << missionsCompleted.load());
        
        missionUnsubscribe();
        fleetUnsubscribe();
    }
    
    SECTION("Resource economy simulation") {
        auto& resourceManager = ResourceManager::getInstance();
        resourceManager.reset();
        
        // Set up economy
        for (Planet* planet : testPlanets) {
            resourceManager.registerResourceProducer(planet);
        }
        
        for (Fleet* fleet : testFleets) {
            resourceManager.registerResourceConsumer(fleet);
        }
        
        // Track resource changes
        std::atomic<int> resourceTransactions{0};
        
        auto& eventSystem = EventSystem::getInstance();
        auto unsubscribe = eventSystem.subscribe<ResourceTransferEvent>(
            [&resourceTransactions](const ResourceTransferEvent& event) {
                resourceTransactions.fetch_add(1);
            });
        
        SimulationEngine engine;
        engine.initialize(std::move(testGalaxy));
        engine.start();
        
        // Let economy run
        std::this_thread::sleep_for(std::chrono::milliseconds(400));
        
        engine.stop();
        
        // Analyze economy
        auto totalResources = resourceManager.getTotalResources();
        auto resourceStats = resourceManager.getResourceStatistics();
        
        REQUIRE(totalResources[ResourceType::MINERALS] > 0);
        REQUIRE(totalResources[ResourceType::ENERGY] > 0);
        REQUIRE(resourceStats.totalProduction > 0);
        REQUIRE(resourceStats.totalConsumption >= 0);
        
        INFO("Total minerals: " << totalResources[ResourceType::MINERALS]);
        INFO("Total energy: " << totalResources[ResourceType::ENERGY]);
        INFO("Resource transactions: " << resourceTransactions.load());
        INFO("Production: " << resourceStats.totalProduction);
        INFO("Consumption: " << resourceStats.totalConsumption);
        
        unsubscribe();
        resourceManager.reset();
    }
    
    SECTION("Full simulation lifecycle") {
        // Create comprehensive simulation
        SimulationEngine engine;
        auto& config = ConfigManager::getInstance();
        auto& logger = Logger::getInstance();
        
        // Configure simulation parameters
        config.setProperty("simulation.timestep", "0.05");
        config.setProperty("simulation.max_updates", "100");
        
        logger.log("Starting full simulation lifecycle test", Logger::LogLevel::INFO);
        
        // Initialize all systems
        engine.initialize(std::move(testGalaxy));
        
        auto& resourceManager = ResourceManager::getInstance();
        resourceManager.reset();
        
        for (Planet* planet : testPlanets) {
            resourceManager.registerResourceProducer(planet);
        }
        
        for (Fleet* fleet : testFleets) {
            resourceManager.registerResourceConsumer(fleet);
        }
        
        // Assign missions
        for (size_t i = 0; i < std::min(testFleets.size(), testMissions.size()); ++i) {
            testMissions[i]->assignFleet(testFleets[i]);
        }
        
        // Track comprehensive metrics
        std::atomic<int> totalEvents{0};
        std::atomic<int> systemUpdates{0};
        
        auto& eventSystem = EventSystem::getInstance();
        
        // Universal event counter
        auto planetSub = eventSystem.subscribe<PlanetUpdateEvent>([&totalEvents](const auto&) { totalEvents.fetch_add(1); });
        auto fleetSub = eventSystem.subscribe<FleetUpdateEvent>([&totalEvents](const auto&) { totalEvents.fetch_add(1); });
        auto missionSub = eventSystem.subscribe<MissionUpdateEvent>([&totalEvents](const auto&) { totalEvents.fetch_add(1); });
        auto resourceSub = eventSystem.subscribe<ResourceTransferEvent>([&totalEvents](const auto&) { totalEvents.fetch_add(1); });
        
        // Start simulation
        auto startTime = std::chrono::high_resolution_clock::now();
        engine.start();
        
        // Run for substantial time
        std::this_thread::sleep_for(std::chrono::milliseconds(800));
        
        // Stop and analyze
        engine.stop();
        auto endTime = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        auto stats = engine.getSimulationStats();
        auto resourceTotals = resourceManager.getTotalResources();
        
        // Verify comprehensive simulation
        REQUIRE(stats.totalUpdates > 0);
        REQUIRE(totalEvents.load() > 0);
        REQUIRE(duration.count() > 0);
        
        // Log final statistics
        logger.log("Simulation completed successfully", Logger::LogLevel::INFO);
        logger.log("Total updates: " + std::to_string(stats.totalUpdates), Logger::LogLevel::INFO);
        logger.log("Total events: " + std::to_string(totalEvents.load()), Logger::LogLevel::INFO);
        logger.log("Duration: " + std::to_string(duration.count()) + "ms", Logger::LogLevel::INFO);
        logger.log("Average frame time: " + std::to_string(stats.averageFrameTime) + "ms", Logger::LogLevel::INFO);
        
        INFO("Comprehensive simulation results:");
        INFO("Total updates: " << stats.totalUpdates);
        INFO("Total events: " << totalEvents.load());
        INFO("Duration: " << duration.count() << "ms");
        INFO("Average frame time: " << stats.averageFrameTime << "ms");
        INFO("Final mineral count: " << resourceTotals[ResourceType::MINERALS]);
        INFO("Final energy count: " << resourceTotals[ResourceType::ENERGY]);
        
        // Cleanup
        planetSub();
        fleetSub();
        missionSub();
        resourceSub();
        resourceManager.reset();
    }
}

TEST_CASE_METHOD(SimulationIntegrationTestFixture, "Simulation Performance Analysis", "[integration][simulation][performance]") {
    
    SECTION("Simulation throughput measurement") {
        SimulationEngine engine;
        engine.initialize(std::move(testGalaxy));
        
        const auto targetDuration = std::chrono::milliseconds(1000);
        
        auto startTime = std::chrono::high_resolution_clock::now();
        engine.start();
        
        std::this_thread::sleep_for(targetDuration);
        
        engine.stop();
        auto endTime = std::chrono::high_resolution_clock::now();
        
        auto actualDuration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        auto stats = engine.getSimulationStats();
        
        double updatesPerSecond = (stats.totalUpdates * 1000.0) / actualDuration.count();
        double averageFrameTimeMs = stats.averageFrameTime;
        
        REQUIRE(stats.totalUpdates > 0);
        REQUIRE(updatesPerSecond > 0);
        REQUIRE(averageFrameTimeMs > 0);
        
        INFO("Performance metrics:");
        INFO("Total updates: " << stats.totalUpdates);
        INFO("Updates per second: " << updatesPerSecond);
        INFO("Average frame time: " << averageFrameTimeMs << "ms");
        INFO("Target duration: " << targetDuration.count() << "ms");
        INFO("Actual duration: " << actualDuration.count() << "ms");
        
        // Performance requirements
        REQUIRE(updatesPerSecond > 10); // At least 10 updates per second
        REQUIRE(averageFrameTimeMs < 100); // Average frame time under 100ms
    }
    
    SECTION("Memory usage during simulation") {
        MemoryTracker::resetCounters();
        
        auto initialStats = MemoryTracker::getCurrentStats();
        
        SimulationEngine engine;
        engine.initialize(std::move(testGalaxy));
        
        auto afterInitStats = MemoryTracker::getCurrentStats();
        
        engine.start();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        engine.stop();
        
        auto finalStats = MemoryTracker::getCurrentStats();
        
        INFO("Memory usage analysis:");
        INFO("Initial allocations: " << initialStats.totalAllocations);
        INFO("After init allocations: " << afterInitStats.totalAllocations);
        INFO("Final allocations: " << finalStats.totalAllocations);
        INFO("Peak memory usage: " << finalStats.peakMemoryUsage << " bytes");
        
        // Memory should be reasonable
        REQUIRE(finalStats.totalAllocations >= initialStats.totalAllocations);
        REQUIRE(finalStats.peakMemoryUsage > 0);
        
        // Memory shouldn't grow excessively during simulation
        size_t memoryGrowth = finalStats.peakMemoryUsage - afterInitStats.currentMemoryUsage;
        REQUIRE(memoryGrowth < 1024 * 1024); // Less than 1MB growth during simulation
    }
    
    SECTION("Scalability with entity count") {
        struct ScalabilityResult {
            int entityCount;
            double averageFrameTime;
            double updatesPerSecond;
        };
        
        std::vector<ScalabilityResult> results;
        std::vector<int> entityCounts = {10, 20, 50, 100};
        
        for (int entityCount : entityCounts) {
            // Create scaled test environment
            auto scaledGalaxy = std::make_unique<Galaxy>("ScaledGalaxy", 1000.0, 1000.0, 1000.0);
            
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<> posDis(0.0, 1000.0);
            std::uniform_int_distribution<> resDis(500, 2000);
            
            // Add planets
            int planetCount = entityCount / 2;
            for (int i = 0; i < planetCount; ++i) {
                auto planet = std::make_unique<Planet>(
                    "ScalePlanet_" + std::to_string(i),
                    Vector3D{posDis(gen), posDis(gen), posDis(gen)}
                );
                planet->setResourceAmount(ResourceType::MINERALS, resDis(gen));
                scaledGalaxy->addPlanet(std::move(planet));
            }
            
            // Add fleets
            int fleetCount = entityCount / 2;
            for (int i = 0; i < fleetCount; ++i) {
                auto fleet = std::make_unique<Fleet>(
                    "ScaleFleet_" + std::to_string(i),
                    Vector3D{posDis(gen), posDis(gen), posDis(gen)}
                );
                fleet->addShips(ShipType::FIGHTER, 5 + i);
                scaledGalaxy->addFleet(std::move(fleet));
            }
            
            // Run simulation
            SimulationEngine engine;
            engine.initialize(std::move(scaledGalaxy));
            
            auto startTime = std::chrono::high_resolution_clock::now();
            engine.start();
            
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            
            engine.stop();
            auto endTime = std::chrono::high_resolution_clock::now();
            
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            auto stats = engine.getSimulationStats();
            
            double updatesPerSecond = (stats.totalUpdates * 1000.0) / duration.count();
            
            results.push_back({
                entityCount,
                stats.averageFrameTime,
                updatesPerSecond
            });
            
            INFO("Entity count " << entityCount << ": " << 
                 stats.averageFrameTime << "ms avg frame time, " <<
                 updatesPerSecond << " updates/sec");
        }
        
        REQUIRE(results.size() == entityCounts.size());
        
        // Verify performance scales reasonably
        for (const auto& result : results) {
            REQUIRE(result.averageFrameTime > 0);
            REQUIRE(result.updatesPerSecond > 0);
            
            // Performance shouldn't degrade too dramatically
            REQUIRE(result.averageFrameTime < 200); // Under 200ms even with many entities
            REQUIRE(result.updatesPerSecond > 5);   // At least 5 updates per second
        }
    }
}

TEST_CASE_METHOD(SimulationIntegrationTestFixture, "Error Handling and Recovery", "[integration][simulation][error-handling]") {
    
    SECTION("Graceful error recovery") {
        SimulationEngine engine;
        
        // Test with invalid configuration
        auto& config = ConfigManager::getInstance();
        config.setProperty("simulation.timestep", "-1.0"); // Invalid timestep
        
        // Should handle gracefully
        engine.initialize(std::move(testGalaxy));
        
        REQUIRE(engine.getState() == SimulationState::INITIALIZED);
        
        engine.start();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        engine.stop();
        
        // Should still complete without crashing
        auto stats = engine.getSimulationStats();
        REQUIRE(stats.totalUpdates >= 0);
    }
    
    SECTION("Exception handling during simulation") {
        class FaultyComponent {
        private:
            int updateCount_;
            
        public:
            FaultyComponent() : updateCount_(0) {}
            
            void update() {
                updateCount_++;
                if (updateCount_ % 10 == 0) {
                    throw std::runtime_error("Simulated component failure");
                }
            }
            
            int getUpdateCount() const { return updateCount_; }
        };
        
        FaultyComponent faultyComponent;
        std::atomic<int> exceptionsHandled{0};
        
        // Simulate component updates with exception handling
        std::vector<std::thread> workers;
        
        for (int i = 0; i < 5; ++i) {
            workers.emplace_back([&faultyComponent, &exceptionsHandled] {
                for (int j = 0; j < 50; ++j) {
                    try {
                        faultyComponent.update();
                    } catch (const std::runtime_error& e) {
                        exceptionsHandled.fetch_add(1);
                        // Continue operation despite exception
                    }
                    
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            });
        }
        
        for (auto& worker : workers) {
            worker.join();
        }
        
        REQUIRE(faultyComponent.getUpdateCount() == 250);
        REQUIRE(exceptionsHandled.load() > 0);
        INFO("Handled " << exceptionsHandled.load() << " exceptions gracefully");
    }
    
    SECTION("Resource cleanup on simulation failure") {
        ResourceCounter::resetCounter();
        
        {
            SimulationEngine engine;
            
            // Add resource counters to track cleanup
            std::vector<ResourceCounter> resources;
            for (int i = 0; i < 10; ++i) {
                resources.emplace_back("SimulationResource_" + std::to_string(i));
            }
            
            REQUIRE(ResourceCounter::getActiveCount() == 10);
            
            engine.initialize(std::move(testGalaxy));
            engine.start();
            
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            
            // Simulate emergency shutdown
            engine.stop();
            
        } // Resources should be cleaned up here
        
        REQUIRE(ResourceCounter::getActiveCount() == 0);
    }
}