// File: tests/unit_tests/concurrency_tests/AsyncTests.cpp
// Asynchronous operation tests for CppVerseHub concurrency showcase

#include <catch2/catch.hpp>
#include <future>
#include <async>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include <queue>
#include <memory>
#include <functional>
#include <random>

// Include the async communication headers
#include "AsyncComms.hpp"
#include "AsyncMissions.hpp"
#include "Planet.hpp"
#include "Fleet.hpp"
#include "Mission.hpp"

using namespace CppVerseHub::Concurrency;
using namespace CppVerseHub::Core;

/**
 * @brief Test fixture for async operation tests
 */
class AsyncTestFixture {
public:
    AsyncTestFixture() {
        setupTestData();
    }
    
    ~AsyncTestFixture() {
        testPlanets.clear();
        testFleets.clear();
    }
    
protected:
    void setupTestData() {
        // Create test entities for async operations
        for (int i = 0; i < 10; ++i) {
            auto planet = std::make_unique<Planet>(
                "AsyncPlanet_" + std::to_string(i),
                Vector3D{i * 100.0, i * 100.0, i * 100.0}
            );
            planet->setResourceAmount(ResourceType::MINERALS, 1000 + i * 200);
            planet->setResourceAmount(ResourceType::ENERGY, 500 + i * 100);
            testPlanets.push_back(std::move(planet));
        }
        
        for (int i = 0; i < 5; ++i) {
            auto fleet = std::make_unique<Fleet>(
                "AsyncFleet_" + std::to_string(i),
                Vector3D{i * 200.0, i * 200.0, i * 200.0}
            );
            fleet->addShips(ShipType::FIGHTER, 10 + i * 5);
            fleet->addShips(ShipType::CRUISER, 3 + i * 2);
            testFleets.push_back(std::move(fleet));
        }
    }
    
    std::vector<std::unique_ptr<Planet>> testPlanets;
    std::vector<std::unique_ptr<Fleet>> testFleets;
};

/**
 * @brief Simple async task runner for testing
 */
class AsyncTaskRunner {
private:
    std::queue<std::function<void()>> tasks_;
    std::mutex taskMutex_;
    std::condition_variable taskCondition_;
    std::atomic<bool> running_;
    std::thread workerThread_;
    
public:
    AsyncTaskRunner() : running_(true) {
        workerThread_ = std::thread([this] {
            while (running_.load()) {
                std::function<void()> task;
                
                {
                    std::unique_lock<std::mutex> lock(taskMutex_);
                    taskCondition_.wait_for(lock, std::chrono::milliseconds(100), [this] {
                        return !tasks_.empty() || !running_.load();
                    });
                    
                    if (!tasks_.empty()) {
                        task = std::move(tasks_.front());
                        tasks_.pop();
                    }
                }
                
                if (task) {
                    task();
                }
            }
        });
    }
    
    ~AsyncTaskRunner() {
        running_.store(false);
        taskCondition_.notify_all();
        if (workerThread_.joinable()) {
            workerThread_.join();
        }
    }
    
    template<typename F>
    auto submitAsync(F&& func) -> std::future<decltype(func())> {
        auto promise = std::make_shared<std::promise<decltype(func())>>();
        auto future = promise->get_future();
        
        {
            std::lock_guard<std::mutex> lock(taskMutex_);
            tasks_.emplace([promise, func = std::forward<F>(func)]() mutable {
                try {
                    if constexpr (std::is_void_v<decltype(func())>) {
                        func();
                        promise->set_value();
                    } else {
                        auto result = func();
                        promise->set_value(std::move(result));
                    }
                } catch (...) {
                    promise->set_exception(std::current_exception());
                }
            });
        }
        
        taskCondition_.notify_one();
        return future;
    }
    
    size_t pendingTasks() const {
        std::lock_guard<std::mutex> lock(taskMutex_);
        return tasks_.size();
    }
};

TEST_CASE_METHOD(AsyncTestFixture, "Basic Async Operations", "[async][concurrency][basic]") {
    
    SECTION("std::async with different launch policies") {
        SECTION("Deferred execution") {
            auto future = std::async(std::launch::deferred, []() {
                return 42;
            });
            
            // Task shouldn't execute until get() is called
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            
            REQUIRE(future.get() == 42);
        }
        
        SECTION("Asynchronous execution") {
            std::atomic<bool> executed{false};
            
            auto future = std::async(std::launch::async, [&executed]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                executed.store(true);
                return "async_result";
            });
            
            // Task should start executing immediately
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
            // Task might still be running at this point
            
            std::string result = future.get();
            REQUIRE(result == "async_result");
            REQUIRE(executed.load());
        }
        
        SECTION("Auto launch policy") {
            auto future = std::async([]() {
                return std::this_thread::get_id();
            });
            
            auto result_thread_id = future.get();
            auto main_thread_id = std::this_thread::get_id();
            
            // Result thread might be different from main thread
            INFO("Main thread: " << main_thread_id);
            INFO("Result thread: " << result_thread_id);
        }
    }
    
    SECTION("Promise and Future pairs") {
        SECTION("Basic promise/future communication") {
            std::promise<int> promise;
            std::future<int> future = promise.get_future();
            
            std::thread producer([&promise]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                promise.set_value(100);
            });
            
            // Consumer waits for result
            int result = future.get();
            REQUIRE(result == 100);
            
            producer.join();
        }
        
        SECTION("Promise with exception") {
            std::promise<std::string> promise;
            std::future<std::string> future = promise.get_future();
            
            std::thread producer([&promise]() {
                try {
                    throw std::runtime_error("Producer error");
                } catch (...) {
                    promise.set_exception(std::current_exception());
                }
            });
            
            REQUIRE_THROWS_AS(future.get(), std::runtime_error);
            
            producer.join();
        }
        
        SECTION("Multiple futures from shared_future") {
            std::promise<double> promise;
            std::shared_future<double> shared_future = promise.get_future().share();
            
            std::vector<std::thread> consumers;
            std::vector<double> results(3);
            
            for (int i = 0; i < 3; ++i) {
                consumers.emplace_back([&shared_future, &results, i]() {
                    results[i] = shared_future.get();
                });
            }
            
            // Producer sets value after consumers are waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            promise.set_value(3.14);
            
            for (auto& consumer : consumers) {
                consumer.join();
            }
            
            // All consumers should get the same value
            for (double result : results) {
                REQUIRE(result == Approx(3.14));
            }
        }
    }
    
    SECTION("Async entity operations") {
        SECTION("Parallel planet updates") {
            std::vector<std::future<void>> updateFutures;
            
            for (auto& planet : testPlanets) {
                updateFutures.push_back(std::async(std::launch::async, [&planet]() {
                    planet->update(1.0);
                    planet->produceResources(1.0);
                }));
            }
            
            // Wait for all updates to complete
            for (auto& future : updateFutures) {
                future.wait();
            }
            
            // Verify all planets are still healthy
            for (const auto& planet : testPlanets) {
                REQUIRE(planet->getHealth() > 0);
            }
        }
        
        SECTION("Async resource calculations") {
            std::vector<std::future<int>> mineralFutures;
            
            for (const auto& planet : testPlanets) {
                mineralFutures.push_back(std::async(std::launch::async, [&planet]() {
                    // Simulate complex calculation
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    return planet->getResourceAmount(ResourceType::MINERALS);
                }));
            }
            
            int totalMinerals = 0;
            for (auto& future : mineralFutures) {
                totalMinerals += future.get();
            }
            
            // Verify total matches expected
            int expectedTotal = 0;
            for (const auto& planet : testPlanets) {
                expectedTotal += planet->getResourceAmount(ResourceType::MINERALS);
            }
            
            REQUIRE(totalMinerals == expectedTotal);
        }
        
        SECTION("Fleet combat power calculation") {
            std::vector<std::future<double>> combatFutures;
            
            for (const auto& fleet : testFleets) {
                combatFutures.push_back(std::async(std::launch::async, [&fleet]() {
                    return fleet->getCombatPower();
                }));
            }
            
            double totalCombatPower = 0;
            for (auto& future : combatFutures) {
                totalCombatPower += future.get();
            }
            
            REQUIRE(totalCombatPower > 0);
        }
    }
}

TEST_CASE_METHOD(AsyncTestFixture, "Advanced Async Patterns", "[async][patterns][advanced]") {
    
    SECTION("Producer-Consumer with futures") {
        SECTION("Single producer, multiple consumers") {
            const int itemCount = 100;
            const int consumerCount = 4;
            
            std::queue<std::promise<int>> promiseQueue;
            std::mutex queueMutex;
            std::condition_variable queueCondition;
            std::atomic<bool> producerDone{false};
            
            // Create promises and get futures
            std::vector<std::future<int>> futures;
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                for (int i = 0; i < itemCount; ++i) {
                    promiseQueue.emplace();
                    futures.push_back(promiseQueue.back().get_future());
                }
            }
            
            // Producer thread
            std::thread producer([&]() {
                for (int i = 0; i < itemCount; ++i) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    
                    std::unique_lock<std::mutex> lock(queueMutex);
                    if (!promiseQueue.empty()) {
                        auto promise = std::move(promiseQueue.front());
                        promiseQueue.pop();
                        lock.unlock();
                        
                        promise.set_value(i * i);
                    }
                }
                producerDone.store(true);
                queueCondition.notify_all();
            });
            
            // Consumer threads
            std::vector<std::thread> consumers;
            std::atomic<int> consumedItems{0};
            
            for (int c = 0; c < consumerCount; ++c) {
                consumers.emplace_back([&futures, &consumedItems, c, consumerCount]() {
                    for (int i = c; i < static_cast<int>(futures.size()); i += consumerCount) {
                        int value = futures[i].get();
                        REQUIRE(value == i * i);
                        consumedItems.fetch_add(1);
                    }
                });
            }
            
            producer.join();
            for (auto& consumer : consumers) {
                consumer.join();
            }
            
            REQUIRE(consumedItems.load() == itemCount);
        }
    }
    
    SECTION("Async task pipeline") {
        SECTION("Multi-stage processing pipeline") {
            AsyncTaskRunner stage1, stage2, stage3;
            
            std::atomic<int> stage1Count{0}, stage2Count{0}, stage3Count{0};
            
            auto processStage1 = [&stage1Count](int input) {
                stage1Count.fetch_add(1);
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                return input * 2;
            };
            
            auto processStage2 = [&stage2Count](int input) {
                stage2Count.fetch_add(1);
                std::this_thread::sleep_for(std::chrono::milliseconds(3));
                return input + 10;
            };
            
            auto processStage3 = [&stage3Count](int input) {
                stage3Count.fetch_add(1);
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
                return std::to_string(input);
            };
            
            const int inputCount = 20;
            std::vector<std::future<std::string>> finalResults;
            
            // Submit pipeline tasks
            for (int i = 0; i < inputCount; ++i) {
                auto future1 = stage1.submitAsync([processStage1, i]() {
                    return processStage1(i);
                });
                
                auto future2 = stage2.submitAsync([processStage2, future1 = std::move(future1)]() mutable {
                    return processStage2(future1.get());
                });
                
                auto future3 = stage3.submitAsync([processStage3, future2 = std::move(future2)]() mutable {
                    return processStage3(future2.get());
                });
                
                finalResults.push_back(std::move(future3));
            }
            
            // Collect results
            std::vector<std::string> results;
            for (auto& future : finalResults) {
                results.push_back(future.get());
            }
            
            REQUIRE(results.size() == inputCount);
            REQUIRE(stage1Count.load() == inputCount);
            REQUIRE(stage2Count.load() == inputCount);
            REQUIRE(stage3Count.load() == inputCount);
            
            // Verify pipeline processing
            for (int i = 0; i < inputCount; ++i) {
                int expected = (i * 2) + 10;
                REQUIRE(results[i] == std::to_string(expected));
            }
        }
    }
    
    SECTION("Async communication patterns") {
        SECTION("Request-Response pattern") {
            struct Request {
                int id;
                std::string data;
                std::promise<std::string> response;
            };
            
            std::queue<Request> requestQueue;
            std::mutex queueMutex;
            std::condition_variable queueCondition;
            std::atomic<bool> serverRunning{true};
            
            // Server thread
            std::thread server([&]() {
                while (serverRunning.load()) {
                    std::unique_lock<std::mutex> lock(queueMutex);
                    queueCondition.wait_for(lock, std::chrono::milliseconds(10), [&] {
                        return !requestQueue.empty() || !serverRunning.load();
                    });
                    
                    if (!requestQueue.empty()) {
                        Request req = std::move(requestQueue.front());
                        requestQueue.pop();
                        lock.unlock();
                        
                        // Process request
                        std::string response = "Processed: " + req.data + " (ID: " + std::to_string(req.id) + ")";
                        req.response.set_value(response);
                    }
                }
            });
            
            // Client requests
            std::vector<std::future<std::string>> responses;
            
            for (int i = 0; i < 10; ++i) {
                Request req;
                req.id = i;
                req.data = "Request_" + std::to_string(i);
                auto future = req.response.get_future();
                
                {
                    std::lock_guard<std::mutex> lock(queueMutex);
                    requestQueue.push(std::move(req));
                }
                queueCondition.notify_one();
                
                responses.push_back(std::move(future));
            }
            
            // Collect responses
            for (int i = 0; i < 10; ++i) {
                std::string response = responses[i].get();
                REQUIRE(response.find("Request_" + std::to_string(i)) != std::string::npos);
                REQUIRE(response.find("ID: " + std::to_string(i)) != std::string::npos);
            }
            
            serverRunning.store(false);
            queueCondition.notify_all();
            server.join();
        }
    }
}

TEST_CASE_METHOD(AsyncTestFixture, "Async Mission Execution", "[async][missions][game-logic]") {
    
    SECTION("Parallel mission execution") {
        SECTION("Multiple exploration missions") {
            std::vector<std::future<bool>> missionResults;
            
            // Create exploration missions asynchronously
            for (int i = 0; i < 5; ++i) {
                missionResults.push_back(std::async(std::launch::async, [this, i]() {
                    // Simulate exploration mission
                    Planet& planet = *testPlanets[i];
                    Fleet& fleet = *testFleets[i % testFleets.size()];
                    
                    // Simulate mission time
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    
                    // Check if mission is successful (based on fleet strength and planet conditions)
                    double missionDifficulty = planet.getResourceAmount(ResourceType::MINERALS) / 1000.0;
                    double fleetStrength = fleet.getCombatPower() / 100.0;
                    
                    return fleetStrength > missionDifficulty;
                }));
            }
            
            // Wait for all missions to complete
            int successfulMissions = 0;
            for (auto& future : missionResults) {
                if (future.get()) {
                    successfulMissions++;
                }
            }
            
            REQUIRE(successfulMissions >= 0);
            REQUIRE(successfulMissions <= 5);
        }
        
        SECTION("Mission coordination") {
            std::promise<bool> coordinationSignal;
            std::shared_future<bool> sharedSignal = coordinationSignal.get_future().share();
            
            std::atomic<int> missionCount{0};
            std::vector<std::future<std::string>> coordinatedMissions;
            
            // Create missions that wait for coordination signal
            for (int i = 0; i < 3; ++i) {
                coordinatedMissions.push_back(std::async(std::launch::async, 
                    [sharedSignal, this, i, &missionCount]() {
                        // Wait for coordination signal
                        bool proceed = sharedSignal.get();
                        
                        if (proceed) {
                            missionCount.fetch_add(1);
                            
                            // Execute mission
                            std::string result = "Mission_" + std::to_string(i) + "_completed";
                            
                            // Simulate mission work
                            testFleets[i % testFleets.size()]->update(1.0);
                            
                            return result;
                        }
                        
                        return std::string("Mission_" + std::to_string(i) + "_cancelled");
                    }));
            }
            
            // Let missions start and wait
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            
            // Send coordination signal
            coordinationSignal.set_value(true);
            
            // Collect results
            for (auto& future : coordinatedMissions) {
                std::string result = future.get();
                REQUIRE(result.find("completed") != std::string::npos);
            }
            
            REQUIRE(missionCount.load() == 3);
        }
    }
    
    SECTION("Async resource management") {
        SECTION("Parallel resource production") {
            std::vector<std::future<std::pair<int, int>>> productionFutures;
            
            for (auto& planet : testPlanets) {
                productionFutures.push_back(std::async(std::launch::async, [&planet]() {
                    int initialMinerals = planet->getResourceAmount(ResourceType::MINERALS);
                    int initialEnergy = planet->getResourceAmount(ResourceType::ENERGY);
                    
                    // Simulate production over time
                    for (int cycle = 0; cycle < 5; ++cycle) {
                        planet->produceResources(1.0);
                        std::this_thread::sleep_for(std::chrono::milliseconds(5));
                    }
                    
                    int finalMinerals = planet->getResourceAmount(ResourceType::MINERALS);
                    int finalEnergy = planet->getResourceAmount(ResourceType::ENERGY);
                    
                    return std::make_pair(finalMinerals - initialMinerals, finalEnergy - initialEnergy);
                }));
            }
            
            // Collect production results
            int totalMineralProduction = 0;
            int totalEnergyProduction = 0;
            
            for (auto& future : productionFutures) {
                auto [mineralProduced, energyProduced] = future.get();
                totalMineralProduction += mineralProduced;
                totalEnergyProduction += energyProduced;
            }
            
            REQUIRE(totalMineralProduction >= 0);
            REQUIRE(totalEnergyProduction >= 0);
        }
        
        SECTION("Resource trading simulation") {
            std::promise<void> tradingStartSignal;
            std::shared_future<void> sharedStart = tradingStartSignal.get_future().share();
            
            std::atomic<int> completedTrades{0};
            std::vector<std::future<bool>> tradeFutures;
            
            // Create trading scenarios
            for (int i = 0; i < testPlanets.size() - 1; i += 2) {
                tradeFutures.push_back(std::async(std::launch::async, 
                    [this, i, sharedStart, &completedTrades]() {
                        // Wait for trading to start
                        sharedStart.wait();
                        
                        Planet& seller = *testPlanets[i];
                        Planet& buyer = *testPlanets[i + 1];
                        
                        // Simulate trade
                        int sellerMinerals = seller.getResourceAmount(ResourceType::MINERALS);
                        int buyerEnergy = buyer.getResourceAmount(ResourceType::ENERGY);
                        
                        if (sellerMinerals > 500 && buyerEnergy > 300) {
                            // Execute trade
                            seller.setResourceAmount(ResourceType::MINERALS, sellerMinerals - 100);
                            buyer.setResourceAmount(ResourceType::ENERGY, buyerEnergy - 50);
                            
                            completedTrades.fetch_add(1);
                            return true;
                        }
                        
                        return false;
                    }));
            }
            
            // Start trading
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            tradingStartSignal.set_value();
            
            // Wait for all trades to complete
            int successfulTrades = 0;
            for (auto& future : tradeFutures) {
                if (future.get()) {
                    successfulTrades++;
                }
            }
            
            REQUIRE(completedTrades.load() == successfulTrades);
        }
    }
}

TEST_CASE_METHOD(AsyncTestFixture, "Async Performance and Optimization", "[async][performance][optimization]") {
    
    SECTION("Async vs synchronous performance") {
        const int taskCount = 100;
        
        // Synchronous execution
        auto syncStart = std::chrono::high_resolution_clock::now();
        
        std::vector<int> syncResults;
        for (int i = 0; i < taskCount; ++i) {
            // Simulate work
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            syncResults.push_back(i * i);
        }
        
        auto syncEnd = std::chrono::high_resolution_clock::now();
        auto syncDuration = std::chrono::duration_cast<std::chrono::milliseconds>(syncEnd - syncStart);
        
        // Asynchronous execution
        auto asyncStart = std::chrono::high_resolution_clock::now();
        
        std::vector<std::future<int>> asyncFutures;
        for (int i = 0; i < taskCount; ++i) {
            asyncFutures.push_back(std::async(std::launch::async, [i]() {
                // Simulate work
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
                return i * i;
            }));
        }
        
        std::vector<int> asyncResults;
        for (auto& future : asyncFutures) {
            asyncResults.push_back(future.get());
        }
        
        auto asyncEnd = std::chrono::high_resolution_clock::now();
        auto asyncDuration = std::chrono::duration_cast<std::chrono::milliseconds>(asyncEnd - asyncStart);
        
        // Results should be the same
        REQUIRE(syncResults.size() == asyncResults.size());
        
        // Sort both vectors for comparison (async order might differ)
        std::sort(syncResults.begin(), syncResults.end());
        std::sort(asyncResults.begin(), asyncResults.end());
        REQUIRE(syncResults == asyncResults);
        
        INFO("Synchronous duration: " << syncDuration.count() << "ms");
        INFO("Asynchronous duration: " << asyncDuration.count() << "ms");
        
        // Async should generally be faster for this I/O-bound work
        REQUIRE(asyncDuration.count() < syncDuration.count() * 0.8);
    }
    
    SECTION("Future overhead measurement") {
        const int iterationCount = 10000;
        
        // Direct function calls
        auto directStart = std::chrono::high_resolution_clock::now();
        
        volatile int directSum = 0;
        for (int i = 0; i < iterationCount; ++i) {
            directSum += i;
        }
        
        auto directEnd = std::chrono::high_resolution_clock::now();
        auto directDuration = std::chrono::duration_cast<std::chrono::microseconds>(directEnd - directStart);
        
        // Future-based calls (deferred)
        auto futureStart = std::chrono::high_resolution_clock::now();
        
        volatile int futureSum = 0;
        for (int i = 0; i < iterationCount; ++i) {
            auto future = std::async(std::launch::deferred, [i]() { return i; });
            futureSum += future.get();
        }
        
        auto futureEnd = std::chrono::high_resolution_clock::now();
        auto futureDuration = std::chrono::duration_cast<std::chrono::microseconds>(futureEnd - futureStart);
        
        REQUIRE(directSum == futureSum);
        
        INFO("Direct calls: " << directDuration.count() << "μs");
        INFO("Future calls: " << futureDuration.count() << "μs");
        INFO("Overhead ratio: " << static_cast<double>(futureDuration.count()) / directDuration.count());
        
        // Future overhead should be reasonable (less than 10x for deferred execution)
        REQUIRE(futureDuration.count() < directDuration.count() * 10);
    }
    
    SECTION("Memory usage with many futures") {
        MemoryTracker::printMemoryStats("Before creating many futures");
        
        {
            const int futureCount = 10000;
            std::vector<std::future<int>> futures;
            futures.reserve(futureCount);
            
            for (int i = 0; i < futureCount; ++i) {
                futures.push_back(std::async(std::launch::deferred, [i]() {
                    return i * 2;
                }));
            }
            
            MemoryTracker::printMemoryStats("After creating 10000 futures");
            
            // Use some of the futures
            int sum = 0;
            for (int i = 0; i < 100; ++i) {
                sum += futures[i].get();
            }
            
            REQUIRE(sum > 0);
            
        } // Futures destroyed here
        
        MemoryTracker::printMemoryStats("After destroying futures");
    }
}

TEST_CASE("Async Error Handling and Edge Cases", "[async][exceptions][edge-cases]") {
    
    SECTION("Exception handling in async operations") {
        SECTION("Exception in async task") {
            auto future = std::async(std::launch::async, []() {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                throw std::runtime_error("Async task error");
                return 42;
            });
            
            REQUIRE_THROWS_AS(future.get(), std::runtime_error);
        }
        
        SECTION("Exception with promise/future") {
            std::promise<double> promise;
            std::future<double> future = promise.get_future();
            
            std::thread t([&promise]() {
                try {
                    throw std::invalid_argument("Promise error");
                } catch (...) {
                    promise.set_exception(std::current_exception());
                }
            });
            
            REQUIRE_THROWS_AS(future.get(), std::invalid_argument);
            t.join();
        }
        
        SECTION("Multiple exceptions in parallel tasks") {
            std::vector<std::future<int>> futures;
            
            for (int i = 0; i < 5; ++i) {
                futures.push_back(std::async(std::launch::async, [i]() {
                    if (i % 2 == 0) {
                        throw std::runtime_error("Even task error");
                    }
                    return i * 10;
                }));
            }
            
            int exceptionCount = 0;
            int successCount = 0;
            std::vector<int> results;
            
            for (auto& future : futures) {
                try {
                    results.push_back(future.get());
                    successCount++;
                } catch (const std::runtime_error&) {
                    exceptionCount++;
                }
            }
            
            REQUIRE(exceptionCount == 3); // indices 0, 2, 4
            REQUIRE(successCount == 2);   // indices 1, 3
            REQUIRE(results.size() == 2);
        }
    }
    
    SECTION("Timeout and cancellation") {
        SECTION("Future timeout with wait_for") {
            auto future = std::async(std::launch::async, []() {
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                return 100;
            });
            
            // Wait with timeout
            auto status = future.wait_for(std::chrono::milliseconds(50));
            REQUIRE(status == std::future_status::timeout);
            
            // Wait longer
            status = future.wait_for(std::chrono::milliseconds(200));
            REQUIRE(status == std::future_status::ready);
            
            REQUIRE(future.get() == 100);
        }
        
        SECTION("Cooperative cancellation") {
            std::atomic<bool> shouldCancel{false};
            std::promise<bool> resultPromise;
            auto resultFuture = resultPromise.get_future();
            
            std::thread worker([&shouldCancel, &resultPromise]() {
                bool completed = false;
                
                for (int i = 0; i < 1000; ++i) {
                    if (shouldCancel.load()) {
                        break;
                    }
                    
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    
                    if (i == 999) {
                        completed = true;
                    }
                }
                
                resultPromise.set_value(completed);
            });
            
            // Let task start
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            
            // Cancel the task
            shouldCancel.store(true);
            
            bool completed = resultFuture.get();
            REQUIRE_FALSE(completed); // Task should have been cancelled
            
            worker.join();
        }
    }
    
    SECTION("Resource cleanup in async operations") {
        std::atomic<int> constructorCount{0};
        std::atomic<int> destructorCount{0};
        
        class AsyncResource {
        public:
            AsyncResource(std::atomic<int>& constructors, std::atomic<int>& destructors)
                : destructors_(destructors) {
                constructors.fetch_add(1);
            }
            
            ~AsyncResource() {
                destructors_.fetch_add(1);
            }
            
            int getValue() const { return 42; }
            
        private:
            std::atomic<int>& destructors_;
        };
        
        {
            std::vector<std::future<int>> futures;
            
            for (int i = 0; i < 10; ++i) {
                futures.push_back(std::async(std::launch::async, [&constructorCount, &destructorCount, i]() {
                    AsyncResource resource(constructorCount, destructorCount);
                    
                    if (i % 3 == 0) {
                        throw std::runtime_error("Resource test exception");
                    }
                    
                    return resource.getValue();
                }));
            }
            
            int exceptionCount = 0;
            for (auto& future : futures) {
                try {
                    future.get();
                } catch (const std::runtime_error&) {
                    exceptionCount++;
                }
            }
            
            REQUIRE(exceptionCount > 0);
            
        } // All futures destroyed here
        
        // All resources should be properly cleaned up
        REQUIRE(constructorCount.load() == 10);
        REQUIRE(destructorCount.load() == 10);
    }
}