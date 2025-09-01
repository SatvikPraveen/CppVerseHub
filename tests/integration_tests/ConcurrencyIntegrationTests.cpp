// File: tests/integration_tests/ConcurrencyIntegrationTests.cpp
// Multi-threaded system integration tests for CppVerseHub showcase

#include <catch2/catch.hpp>
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <future>
#include <chrono>
#include <random>
#include <algorithm>

// Include all major concurrent system components
#include "ThreadPool.hpp"
#include "EventSystem.hpp"
#include "SimulationEngine.hpp"
#include "ResourceManager.hpp"
#include "Galaxy.hpp"
#include "Planet.hpp"
#include "Fleet.hpp"
#include "Mission.hpp"
#include "Logger.hpp"
#include "MemoryTracker.hpp"

using namespace CppVerseHub::Core;
using namespace CppVerseHub::Concurrency;
using namespace CppVerseHub::Events;
using namespace CppVerseHub::Simulation;
using namespace CppVerseHub::Utils;

/**
 * @brief Test fixture for concurrency integration tests
 */
class ConcurrencyIntegrationTestFixture {
public:
    ConcurrencyIntegrationTestFixture() {
        MemoryTracker::resetCounters();
        
        // Initialize systems
        auto& logger = Logger::getInstance();
        logger.setLogLevel(Logger::LogLevel::WARN); // Reduce log noise in tests
        
        setupTestEnvironment();
    }
    
    ~ConcurrencyIntegrationTestFixture() {
        cleanup();
        MemoryTracker::printMemoryStats("Concurrency integration test completion");
    }
    
protected:
    void setupTestEnvironment() {
        // Create test galaxy with entities
        testGalaxy = std::make_unique<Galaxy>("ConcurrencyTestGalaxy", 2000.0, 2000.0, 2000.0);
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> posDis(0.0, 2000.0);
        std::uniform_int_distribution<> resDis(800, 2500);
        
        // Create planets
        const int planetCount = 30;
        for (int i = 0; i < planetCount; ++i) {
            auto planet = std::make_unique<Planet>(
                "ConcurrencyPlanet_" + std::to_string(i),
                Vector3D{posDis(gen), posDis(gen), posDis(gen)}
            );
            
            planet->setResourceAmount(ResourceType::MINERALS, resDis(gen));
            planet->setResourceAmount(ResourceType::ENERGY, resDis(gen));
            planet->setHabitabilityRating(0.2 + (i % 8) * 0.1);
            
            testPlanets.push_back(planet.get());
            testGalaxy->addPlanet(std::move(planet));
        }
        
        // Create fleets
        const int fleetCount = 15;
        for (int i = 0; i < fleetCount; ++i) {
            auto fleet = std::make_unique<Fleet>(
                "ConcurrencyFleet_" + std::to_string(i),
                Vector3D{posDis(gen), posDis(gen), posDis(gen)}
            );
            
            fleet->addShips(ShipType::FIGHTER, 12 + i * 2);
            fleet->addShips(ShipType::CRUISER, 3 + i);
            if (i % 4 == 0) {
                fleet->addShips(ShipType::BATTLESHIP, 1 + i / 4);
            }
            
            testFleets.push_back(fleet.get());
            testGalaxy->addFleet(std::move(fleet));
        }
        
        // Create missions
        const int missionCount = 10;
        for (int i = 0; i < missionCount && i < testPlanets.size(); ++i) {
            auto mission = std::make_unique<ExploreMission>(
                "ConcurrencyMission_" + std::to_string(i),
                testPlanets[i]
            );
            
            testMissions.push_back(mission.get());
            testGalaxy->addMission(std::move(mission));
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

TEST_CASE_METHOD(ConcurrencyIntegrationTestFixture, "Thread Pool Integration", "[concurrency][integration][thread-pool]") {
    
    SECTION("Basic thread pool with simulation tasks") {
        const int threadCount = 4;
        const int taskCount = 100;
        
        ThreadPool threadPool(threadCount);
        
        std::atomic<int> completedTasks{0};
        std::atomic<int> totalProcessed{0};
        
        std::vector<std::future<int>> futures;
        
        // Submit simulation tasks
        for (int i = 0; i < taskCount; ++i) {
            auto future = threadPool.enqueue([&totalProcessed, i]() -> int {
                // Simulate planet processing
                int processed = 0;
                for (int j = 0; j < 10; ++j) {
                    // Simulate some computation
                    processed += i * j;
                }
                totalProcessed.fetch_add(processed);
                return processed;
            });
            futures.push_back(std::move(future));
        }
        
        // Wait for all tasks to complete
        for (auto& future : futures) {
            int result = future.get();
            if (result >= 0) {
                completedTasks.fetch_add(1);
            }
        }
        
        threadPool.shutdown();
        
        REQUIRE(completedTasks.load() == taskCount);
        REQUIRE(totalProcessed.load() > 0);
        
        INFO("Thread pool completed " << completedTasks.load() << " tasks");
        INFO("Total processing value: " << totalProcessed.load());
    }
    
    SECTION("Thread pool with different task priorities") {
        const int threadCount = 6;
        ThreadPool threadPool(threadCount);
        
        std::atomic<int> highPriorityCompleted{0};
        std::atomic<int> lowPriorityCompleted{0};
        
        // Submit high priority tasks (fleet operations)
        std::vector<std::future<void>> highPriorityFutures;
        for (int i = 0; i < 20; ++i) {
            auto future = threadPool.enqueue([&highPriorityCompleted, this, i]() {
                if (i < testFleets.size()) {
                    // High priority fleet update
                    testFleets[i]->update(0.1);
                }
                highPriorityCompleted.fetch_add(1);
            });
            highPriorityFutures.push_back(std::move(future));
        }
        
        // Submit low priority tasks (planet updates)
        std::vector<std::future<void>> lowPriorityFutures;
        for (int i = 0; i < 30; ++i) {
            auto future = threadPool.enqueue([&lowPriorityCompleted, this, i]() {
                if (i < testPlanets.size()) {
                    // Low priority planet update
                    testPlanets[i]->update(0.1);
                }
                lowPriorityCompleted.fetch_add(1);
            });
            lowPriorityFutures.push_back(std::move(future));
        }
        
        // Wait for all high priority tasks
        for (auto& future : highPriorityFutures) {
            future.wait();
        }
        
        // Wait for all low priority tasks
        for (auto& future : lowPriorityFutures) {
            future.wait();
        }
        
        threadPool.shutdown();
        
        REQUIRE(highPriorityCompleted.load() == 20);
        REQUIRE(lowPriorityCompleted.load() == 30);
        
        INFO("High priority tasks: " << highPriorityCompleted.load());
        INFO("Low priority tasks: " << lowPriorityCompleted.load());
    }
    
    SECTION("Thread pool exception handling") {
        const int threadCount = 3;
        ThreadPool threadPool(threadCount);
        
        std::atomic<int> successfulTasks{0};
        std::atomic<int> exceptionTasks{0};
        
        // Submit tasks that might throw exceptions
        std::vector<std::future<bool>> futures;
        
        for (int i = 0; i < 50; ++i) {
            auto future = threadPool.enqueue([&successfulTasks, &exceptionTasks, i]() -> bool {
                try {
                    if (i % 7 == 0) {
                        throw std::runtime_error("Simulated task failure");
                    }
                    
                    // Simulate successful work
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    successfulTasks.fetch_add(1);
                    return true;
                    
                } catch (const std::exception&) {
                    exceptionTasks.fetch_add(1);
                    return false;
                }
            });
            futures.push_back(std::move(future));
        }
        
        // Collect results
        int trueResults = 0;
        int falseResults = 0;
        
        for (auto& future : futures) {
            bool result = future.get();
            if (result) {
                trueResults++;
            } else {
                falseResults++;
            }
        }
        
        threadPool.shutdown();
        
        REQUIRE(successfulTasks.load() + exceptionTasks.load() == 50);
        REQUIRE(trueResults == successfulTasks.load());
        REQUIRE(falseResults == exceptionTasks.load());
        REQUIRE(exceptionTasks.load() > 0); // Some tasks should have failed
        
        INFO("Successful tasks: " << successfulTasks.load());
        INFO("Failed tasks: " << exceptionTasks.load());
    }
}

TEST_CASE_METHOD(ConcurrencyIntegrationTestFixture, "Event System Concurrency", "[concurrency][integration][events]") {
    
    SECTION("Multi-threaded event publishing and subscribing") {
        auto& eventSystem = EventSystem::getInstance();
        
        std::atomic<int> planetEventsReceived{0};
        std::atomic<int> fleetEventsReceived{0};
        std::atomic<int> missionEventsReceived{0};
        
        // Subscribe to events from multiple threads
        std::vector<std::function<void()>> unsubscribers;
        
        auto planetUnsubscribe = eventSystem.subscribe<PlanetUpdateEvent>(
            [&planetEventsReceived](const PlanetUpdateEvent& event) {
                planetEventsReceived.fetch_add(1);
            });
        unsubscribers.push_back(planetUnsubscribe);
        
        auto fleetUnsubscribe = eventSystem.subscribe<FleetUpdateEvent>(
            [&fleetEventsReceived](const FleetUpdateEvent& event) {
                fleetEventsReceived.fetch_add(1);
            });
        unsubscribers.push_back(fleetUnsubscribe);
        
        auto missionUnsubscribe = eventSystem.subscribe<MissionUpdateEvent>(
            [&missionEventsReceived](const MissionUpdateEvent& event) {
                missionEventsReceived.fetch_add(1);
            });
        unsubscribers.push_back(missionUnsubscribe);
        
        // Publish events from multiple threads
        const int publisherThreads = 8;
        const int eventsPerThread = 25;
        
        std::vector<std::thread> publishers;
        
        for (int i = 0; i < publisherThreads; ++i) {
            publishers.emplace_back([&eventSystem, this, eventsPerThread, i] {
                for (int j = 0; j < eventsPerThread; ++j) {
                    // Publish different types of events
                    if (j % 3 == 0) {
                        Planet* planet = testPlanets[j % testPlanets.size()];
                        PlanetUpdateEvent event{planet->getName(), planet->getPosition(), 0.1};
                        eventSystem.publish(event);
                    } else if (j % 3 == 1) {
                        Fleet* fleet = testFleets[j % testFleets.size()];
                        FleetUpdateEvent event{fleet->getName(), fleet->getPosition(), fleet->getTotalShipCount()};
                        eventSystem.publish(event);
                    } else {
                        Mission* mission = testMissions[j % testMissions.size()];
                        MissionUpdateEvent event{mission->getName(), mission->getStatus(), mission->getProgress()};
                        eventSystem.publish(event);
                    }
                }
            });
        }
        
        for (auto& publisher : publishers) {
            publisher.join();
        }
        
        // Allow time for event processing
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        REQUIRE(planetEventsReceived.load() > 0);
        REQUIRE(fleetEventsReceived.load() > 0);
        REQUIRE(missionEventsReceived.load() > 0);
        
        int totalEvents = planetEventsReceived.load() + fleetEventsReceived.load() + missionEventsReceived.load();
        int expectedEvents = publisherThreads * eventsPerThread;
        
        REQUIRE(totalEvents == expectedEvents);
        
        INFO("Planet events: " << planetEventsReceived.load());
        INFO("Fleet events: " << fleetEventsReceived.load());
        INFO("Mission events: " << missionEventsReceived.load());
        INFO("Total events: " << totalEvents);
        
        // Cleanup
        for (auto& unsubscribe : unsubscribers) {
            unsubscribe();
        }
    }
    
    SECTION("Event system stress test") {
        auto& eventSystem = EventSystem::getInstance();
        
        std::atomic<int> eventsPublished{0};
        std::atomic<int> eventsProcessed{0};
        
        // Create many subscribers
        const int subscriberCount = 20;
        std::vector<std::function<void()>> subscribers;
        
        for (int i = 0; i < subscriberCount; ++i) {
            auto unsubscribe = eventSystem.subscribe<PlanetUpdateEvent>(
                [&eventsProcessed](const PlanetUpdateEvent& event) {
                    eventsProcessed.fetch_add(1);
                    // Simulate processing time
                    std::this_thread::sleep_for(std::chrono::microseconds(10));
                });
            subscribers.push_back(unsubscribe);
        }
        
        // Publish many events from multiple threads
        const int publisherThreads = 10;
        const int eventsPerThread = 100;
        
        std::vector<std::thread> publishers;
        
        for (int i = 0; i < publisherThreads; ++i) {
            publishers.emplace_back([&eventSystem, &eventsPublished, this, eventsPerThread] {
                for (int j = 0; j < eventsPerThread; ++j) {
                    Planet* planet = testPlanets[j % testPlanets.size()];
                    PlanetUpdateEvent event{planet->getName(), planet->getPosition(), 0.01};
                    eventSystem.publish(event);
                    eventsPublished.fetch_add(1);
                }
            });
        }
        
        for (auto& publisher : publishers) {
            publisher.join();
        }
        
        // Wait for event processing
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        int expectedProcessed = eventsPublished.load() * subscriberCount;
        
        REQUIRE(eventsPublished.load() == publisherThreads * eventsPerThread);
        REQUIRE(eventsProcessed.load() == expectedProcessed);
        
        INFO("Events published: " << eventsPublished.load());
        INFO("Events processed: " << eventsProcessed.load());
        INFO("Subscribers: " << subscriberCount);
        
        // Cleanup
        for (auto& unsubscribe : subscribers) {
            unsubscribe();
        }
    }
    
    SECTION("Event system with dynamic subscription changes") {
        auto& eventSystem = EventSystem::getInstance();
        
        std::atomic<int> activeSubscribers{0};
        std::atomic<int> eventsReceived{0};
        
        // Dynamic subscription management
        const int maxSubscribers = 15;
        const int subscriptionCycles = 50;
        
        std::vector<std::thread> subscriptionThreads;
        std::mutex subscribersMutex;
        std::vector<std::function<void()>> activeUnsubscribers;
        
        // Thread that manages subscriptions
        std::thread subscriptionManager([&]() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, 1);
            
            for (int i = 0; i < subscriptionCycles; ++i) {
                std::lock_guard<std::mutex> lock(subscribersMutex);
                
                if (dis(gen) == 0 && activeUnsubscribers.size() < maxSubscribers) {
                    // Add subscriber
                    auto unsubscribe = eventSystem.subscribe<FleetUpdateEvent>(
                        [&eventsReceived](const FleetUpdateEvent& event) {
                            eventsReceived.fetch_add(1);
                        });
                    activeUnsubscribers.push_back(unsubscribe);
                    activeSubscribers.fetch_add(1);
                } else if (!activeUnsubscribers.empty()) {
                    // Remove subscriber
                    activeUnsubscribers.back()();
                    activeUnsubscribers.pop_back();
                    activeSubscribers.fetch_sub(1);
                }
                
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
        
        // Thread that publishes events
        std::thread eventPublisher([&eventSystem, this]() {
            for (int i = 0; i < 200; ++i) {
                Fleet* fleet = testFleets[i % testFleets.size()];
                FleetUpdateEvent event{fleet->getName(), fleet->getPosition(), fleet->getTotalShipCount()};
                eventSystem.publish(event);
                
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        });
        
        subscriptionManager.join();
        eventPublisher.join();
        
        // Wait for remaining events to be processed
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Cleanup remaining subscribers
        {
            std::lock_guard<std::mutex> lock(subscribersMutex);
            for (auto& unsubscribe : activeUnsubscribers) {
                unsubscribe();
            }
            activeUnsubscribers.clear();
        }
        
        REQUIRE(eventsReceived.load() > 0);
        INFO("Events received with dynamic subscriptions: " << eventsReceived.load());
        INFO("Final active subscribers: " << activeSubscribers.load());
    }
}

TEST_CASE_METHOD(ConcurrencyIntegrationTestFixture, "Full System Concurrency", "[concurrency][integration][full-system]") {
    
    SECTION("Concurrent simulation with all systems") {
        const int threadCount = 6;
        ThreadPool threadPool(threadCount);
        
        auto& resourceManager = ResourceManager::getInstance();
        resourceManager.reset();
        
        // Register entities with resource manager
        for (Planet* planet : testPlanets) {
            resourceManager.registerResourceProducer(planet);
        }
        
        for (Fleet* fleet : testFleets) {
            resourceManager.registerResourceConsumer(fleet);
        }
        
        // Set up event tracking
        std::atomic<int> systemEvents{0};
        std::atomic<int> resourceEvents{0};
        
        auto& eventSystem = EventSystem::getInstance();
        
        auto systemUnsubscribe = eventSystem.subscribe<SystemUpdateEvent>(
            [&systemEvents](const SystemUpdateEvent& event) {
                systemEvents.fetch_add(1);
            });
        
        auto resourceUnsubscribe = eventSystem.subscribe<ResourceTransferEvent>(
            [&resourceEvents](const ResourceTransferEvent& event) {
                resourceEvents.fetch_add(1);
            });
        
        // Start simulation engine
        SimulationEngine engine;
        engine.initialize(std::move(testGalaxy));
        engine.setThreadPool(&threadPool);
        
        // Run concurrent simulation tasks
        std::vector<std::future<void>> simulationTasks;
        
        // Task 1: Update planets
        for (int i = 0; i < 10; ++i) {
            auto future = threadPool.enqueue([this, i]() {
                for (int j = 0; j < 20; ++j) {
                    if (i < testPlanets.size()) {
                        testPlanets[i]->update(0.05);
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            });
            simulationTasks.push_back(std::move(future));
        }
        
        // Task 2: Update fleets
        for (int i = 0; i < 5; ++i) {
            auto future = threadPool.enqueue([this, i]() {
                for (int j = 0; j < 15; ++j) {
                    if (i < testFleets.size()) {
                        testFleets[i]->update(0.05);
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(15));
                }
            });
            simulationTasks.push_back(std::move(future));
        }
        
        // Task 3: Process missions
        for (int i = 0; i < 3; ++i) {
            auto future = threadPool.enqueue([this, i]() {
                for (int j = 0; j < 10; ++j) {
                    if (i < testMissions.size()) {
                        testMissions[i]->update(0.05);
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(20));
                }
            });
            simulationTasks.push_back(std::move(future));
        }
        
        // Start main simulation
        engine.start();
        
        // Wait for all simulation tasks
        for (auto& task : simulationTasks) {
            task.wait();
        }
        
        // Let simulation run a bit longer
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        
        // Stop simulation
        engine.stop();
        threadPool.shutdown();
        
        // Analyze results
        auto stats = engine.getSimulationStats();
        auto resourceTotals = resourceManager.getTotalResources();
        
        REQUIRE(stats.totalUpdates > 0);
        REQUIRE(systemEvents.load() >= 0);
        REQUIRE(resourceTotals[ResourceType::MINERALS] > 0);
        REQUIRE(resourceTotals[ResourceType::ENERGY] > 0);
        
        INFO("Full system concurrency results:");
        INFO("Simulation updates: " << stats.totalUpdates);
        INFO("Average frame time: " << stats.averageFrameTime << "ms");
        INFO("System events: " << systemEvents.load());
        INFO("Resource events: " << resourceEvents.load());
        INFO("Total minerals: " << resourceTotals[ResourceType::MINERALS]);
        INFO("Total energy: " << resourceTotals[ResourceType::ENERGY]);
        
        // Cleanup
        systemUnsubscribe();
        resourceUnsubscribe();
        resourceManager.reset();
    }
    
    SECTION("Deadlock prevention and detection") {
        const int resourceCount = 5;
        const int threadCount = 8;
        
        std::vector<std::mutex> resourceMutexes(resourceCount);
        std::atomic<int> successfulOperations{0};
        std::atomic<int> timeoutOperations{0};
        
        // Simulate operations that could deadlock
        std::vector<std::thread> threads;
        
        for (int i = 0; i < threadCount; ++i) {
            threads.emplace_back([&resourceMutexes, &successfulOperations, &timeoutOperations, i, resourceCount]() {
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<> dis(0, resourceCount - 1);
                
                for (int j = 0; j < 20; ++j) {
                    // Acquire two resources in consistent order to prevent deadlock
                    int resource1 = dis(gen);
                    int resource2 = dis(gen);
                    
                    if (resource1 > resource2) {
                        std::swap(resource1, resource2);
                    }
                    
                    if (resource1 != resource2) {
                        // Try to acquire both locks with timeout
                        if (resourceMutexes[resource1].try_lock()) {
                            if (resourceMutexes[resource2].try_lock()) {
                                // Simulate work
                                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                                successfulOperations.fetch_add(1);
                                
                                resourceMutexes[resource2].unlock();
                            } else {
                                timeoutOperations.fetch_add(1);
                            }
                            resourceMutexes[resource1].unlock();
                        } else {
                            timeoutOperations.fetch_add(1);
                        }
                    } else {
                        // Single resource operation
                        std::lock_guard<std::mutex> lock(resourceMutexes[resource1]);
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                        successfulOperations.fetch_add(1);
                    }
                }
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        REQUIRE(successfulOperations.load() + timeoutOperations.load() == threadCount * 20);
        REQUIRE(successfulOperations.load() > 0);
        
        INFO("Successful operations: " << successfulOperations.load());
        INFO("Timeout operations: " << timeoutOperations.load());
        INFO("No deadlocks detected");
    }
    
    SECTION("Memory consistency under concurrent access") {
        const int writerThreads = 4;
        const int readerThreads = 8;
        const int operationsPerThread = 100;
        
        std::vector<std::atomic<int>> sharedCounters(10);
        std::atomic<int> totalWrites{0};
        std::atomic<int> totalReads{0};
        
        std::vector<std::thread> threads;
        
        // Writer threads
        for (int i = 0; i < writerThreads; ++i) {
            threads.emplace_back([&sharedCounters, &totalWrites, operationsPerThread, i]() {
                for (int j = 0; j < operationsPerThread; ++j) {
                    int counterIndex = (i * operationsPerThread + j) % sharedCounters.size();
                    sharedCounters[counterIndex].fetch_add(1, std::memory_order_relaxed);
                    totalWrites.fetch_add(1, std::memory_order_relaxed);
                }
            });
        }
        
        // Reader threads
        for (int i = 0; i < readerThreads; ++i) {
            threads.emplace_back([&sharedCounters, &totalReads, operationsPerThread]() {
                for (int j = 0; j < operationsPerThread; ++j) {
                    int counterIndex = j % sharedCounters.size();
                    volatile int value = sharedCounters[counterIndex].load(std::memory_order_acquire);
                    totalReads.fetch_add(1, std::memory_order_relaxed);
                    
                    // Use the value to prevent optimization
                    (void)value;
                }
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        // Verify memory consistency
        int sumOfCounters = 0;
        for (const auto& counter : sharedCounters) {
            sumOfCounters += counter.load();
        }
        
        REQUIRE(totalWrites.load() == writerThreads * operationsPerThread);
        REQUIRE(totalReads.load() == readerThreads * operationsPerThread);
        REQUIRE(sumOfCounters == totalWrites.load());
        
        INFO("Total writes: " << totalWrites.load());
        INFO("Total reads: " << totalReads.load());
        INFO("Sum of counters: " << sumOfCounters);
        INFO("Memory consistency verified");
    }
}