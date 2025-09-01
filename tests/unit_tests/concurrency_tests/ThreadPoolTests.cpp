// File: tests/unit_tests/concurrency_tests/ThreadPoolTests.cpp
// Threading tests for CppVerseHub concurrency showcase

#include <catch2/catch.hpp>
#include <thread>
#include <future>
#include <atomic>
#include <chrono>
#include <vector>
#include <queue>
#include <functional>
#include <memory>
#include <mutex>
#include <condition_variable>

// Include the concurrency headers
#include "ThreadPool.hpp"
#include "Planet.hpp"
#include "Fleet.hpp"
#include "Entity.hpp"

using namespace CppVerseHub::Concurrency;
using namespace CppVerseHub::Core;

/**
 * @brief Test fixture for thread pool tests
 */
class ThreadPoolTestFixture {
public:
    ThreadPoolTestFixture() {
        setupTestData();
    }
    
    ~ThreadPoolTestFixture() {
        testPlanets.clear();
        testFleets.clear();
    }
    
protected:
    void setupTestData() {
        // Create test entities for concurrent processing
        for (int i = 0; i < 20; ++i) {
            auto planet = std::make_unique<Planet>(
                "Planet_" + std::to_string(i),
                Vector3D{i * 50.0, i * 50.0, i * 50.0}
            );
            planet->setResourceAmount(ResourceType::MINERALS, 1000 + i * 100);
            testPlanets.push_back(std::move(planet));
        }
        
        for (int i = 0; i < 10; ++i) {
            auto fleet = std::make_unique<Fleet>(
                "Fleet_" + std::to_string(i),
                Vector3D{i * 100.0, i * 100.0, i * 100.0}
            );
            fleet->addShips(ShipType::FIGHTER, 5 + i * 2);
            testFleets.push_back(std::move(fleet));
        }
    }
    
    std::vector<std::unique_ptr<Planet>> testPlanets;
    std::vector<std::unique_ptr<Fleet>> testFleets;
};

/**
 * @brief Simple thread pool implementation for testing
 */
class SimpleThreadPool {
private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex queueMutex_;
    std::condition_variable condition_;
    std::atomic<bool> stop_;
    
public:
    explicit SimpleThreadPool(size_t threadCount) : stop_(false) {
        for (size_t i = 0; i < threadCount; ++i) {
            workers_.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    
                    {
                        std::unique_lock<std::mutex> lock(queueMutex_);
                        condition_.wait(lock, [this] { 
                            return stop_.load() || !tasks_.empty(); 
                        });
                        
                        if (stop_.load() && tasks_.empty()) {
                            return;
                        }
                        
                        task = std::move(tasks_.front());
                        tasks_.pop();
                    }
                    
                    task();
                }
            });
        }
    }
    
    ~SimpleThreadPool() {
        stop_.store(true);
        condition_.notify_all();
        
        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }
    
    template<typename F, typename... Args>
    auto enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type> {
        using ReturnType = typename std::result_of<F(Args...)>::type;
        
        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        
        std::future<ReturnType> result = task->get_future();
        
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            
            if (stop_.load()) {
                throw std::runtime_error("enqueue on stopped ThreadPool");
            }
            
            tasks_.emplace([task]() { (*task)(); });
        }
        
        condition_.notify_one();
        return result;
    }
    
    size_t size() const {
        return workers_.size();
    }
    
    size_t pending_tasks() const {
        std::lock_guard<std::mutex> lock(queueMutex_);
        return tasks_.size();
    }
};

TEST_CASE_METHOD(ThreadPoolTestFixture, "Basic Thread Pool Operations", "[threadpool][concurrency][basic]") {
    
    SECTION("Thread pool creation and destruction") {
        SECTION("Create thread pool with specific thread count") {
            const size_t threadCount = 4;
            SimpleThreadPool pool(threadCount);
            
            REQUIRE(pool.size() == threadCount);
            REQUIRE(pool.pending_tasks() == 0);
        }
        
        SECTION("Thread pool with hardware concurrency") {
            const size_t hwConcurrency = std::thread::hardware_concurrency();
            const size_t threadCount = hwConcurrency > 0 ? hwConcurrency : 4;
            
            SimpleThreadPool pool(threadCount);
            REQUIRE(pool.size() == threadCount);
        }
        
        SECTION("Empty thread pool") {
            SimpleThreadPool pool(0);
            REQUIRE(pool.size() == 0);
        }
    }
    
    SECTION("Task submission and execution") {
        SimpleThreadPool pool(4);
        
        SECTION("Simple task execution") {
            std::atomic<int> counter{0};
            
            std::vector<std::future<void>> futures;
            
            // Submit multiple tasks
            for (int i = 0; i < 10; ++i) {
                futures.push_back(pool.enqueue([&counter, i] {
                    counter.fetch_add(1);
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }));
            }
            
            // Wait for all tasks to complete
            for (auto& future : futures) {
                future.wait();
            }
            
            REQUIRE(counter.load() == 10);
        }
        
        SECTION("Task with return values") {
            auto future1 = pool.enqueue([]() -> int {
                return 42;
            });
            
            auto future2 = pool.enqueue([](int a, int b) -> int {
                return a + b;
            }, 10, 20);
            
            auto future3 = pool.enqueue([]() -> std::string {
                return "Hello from thread pool";
            });
            
            REQUIRE(future1.get() == 42);
            REQUIRE(future2.get() == 30);
            REQUIRE(future3.get() == "Hello from thread pool");
        }
        
        SECTION("Exception handling in tasks") {
            auto future = pool.enqueue([]() -> int {
                throw std::runtime_error("Test exception");
                return 0; // Never reached
            });
            
            REQUIRE_THROWS_AS(future.get(), std::runtime_error);
        }
    }
    
    SECTION("Concurrent entity processing") {
        SimpleThreadPool pool(6);
        
        SECTION("Parallel planet updates") {
            std::atomic<int> updateCount{0};
            std::vector<std::future<void>> futures;
            
            // Submit planet update tasks
            for (auto& planet : testPlanets) {
                futures.push_back(pool.enqueue([&planet, &updateCount] {
                    planet->update(1.0);
                    updateCount.fetch_add(1);
                }));
            }
            
            // Wait for all updates to complete
            for (auto& future : futures) {
                future.wait();
            }
            
            REQUIRE(updateCount.load() == static_cast<int>(testPlanets.size()));
            
            // Verify all planets are still valid
            for (const auto& planet : testPlanets) {
                REQUIRE(planet->getHealth() > 0);
                REQUIRE(!planet->getName().empty());
            }
        }
        
        SECTION("Parallel resource calculations") {
            std::atomic<int> totalMinerals{0};
            std::vector<std::future<int>> futures;
            
            // Submit resource calculation tasks
            for (const auto& planet : testPlanets) {
                futures.push_back(pool.enqueue([&planet]() -> int {
                    // Simulate complex calculation
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                    return planet->getResourceAmount(ResourceType::MINERALS);
                }));
            }
            
            // Collect results
            for (auto& future : futures) {
                totalMinerals.fetch_add(future.get());
            }
            
            // Verify total
            int expectedTotal = 0;
            for (const auto& planet : testPlanets) {
                expectedTotal += planet->getResourceAmount(ResourceType::MINERALS);
            }
            
            REQUIRE(totalMinerals.load() == expectedTotal);
        }
        
        SECTION("Parallel fleet operations") {
            std::vector<std::future<double>> combatPowerFutures;
            
            // Calculate combat power in parallel
            for (const auto& fleet : testFleets) {
                combatPowerFutures.push_back(pool.enqueue([&fleet]() -> double {
                    return fleet->getCombatPower();
                }));
            }
            
            // Collect combat powers
            std::vector<double> combatPowers;
            for (auto& future : combatPowerFutures) {
                combatPowers.push_back(future.get());
            }
            
            REQUIRE(combatPowers.size() == testFleets.size());
            
            // Verify all combat powers are positive
            for (double power : combatPowers) {
                REQUIRE(power > 0);
            }
        }
    }
}

TEST_CASE_METHOD(ThreadPoolTestFixture, "Thread Pool Performance and Scalability", "[threadpool][performance][scalability]") {
    
    SECTION("Performance comparison") {
        const int taskCount = 1000;
        
        SECTION("Sequential vs parallel execution") {
            // Sequential execution
            auto sequentialStart = std::chrono::high_resolution_clock::now();
            
            std::atomic<int> sequentialSum{0};
            for (int i = 0; i < taskCount; ++i) {
                // Simulate work
                std::this_thread::sleep_for(std::chrono::microseconds(100));
                sequentialSum.fetch_add(i);
            }
            
            auto sequentialEnd = std::chrono::high_resolution_clock::now();
            auto sequentialDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
                sequentialEnd - sequentialStart);
            
            // Parallel execution
            SimpleThreadPool pool(8);
            auto parallelStart = std::chrono::high_resolution_clock::now();
            
            std::atomic<int> parallelSum{0};
            std::vector<std::future<void>> futures;
            
            for (int i = 0; i < taskCount; ++i) {
                futures.push_back(pool.enqueue([&parallelSum, i] {
                    // Simulate work
                    std::this_thread::sleep_for(std::chrono::microseconds(100));
                    parallelSum.fetch_add(i);
                }));
            }
            
            for (auto& future : futures) {
                future.wait();
            }
            
            auto parallelEnd = std::chrono::high_resolution_clock::now();
            auto parallelDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
                parallelEnd - parallelStart);
            
            // Results should be the same
            REQUIRE(sequentialSum.load() == parallelSum.load());
            
            // Parallel should be faster (with some tolerance for overhead)
            INFO("Sequential duration: " << sequentialDuration.count() << "ms");
            INFO("Parallel duration: " << parallelDuration.count() << "ms");
            
            // Allow for some overhead, but parallel should generally be faster
            REQUIRE(parallelDuration.count() < sequentialDuration.count() * 0.8);
        }
        
        SECTION("Scalability with different thread counts") {
            const int workAmount = 500;
            std::vector<size_t> threadCounts = {1, 2, 4, 8};
            std::vector<std::chrono::milliseconds> durations;
            
            for (size_t threadCount : threadCounts) {
                SimpleThreadPool pool(threadCount);
                auto start = std::chrono::high_resolution_clock::now();
                
                std::vector<std::future<void>> futures;
                std::atomic<int> workCounter{0};
                
                for (int i = 0; i < workAmount; ++i) {
                    futures.push_back(pool.enqueue([&workCounter] {
                        // Simulate CPU-bound work
                        volatile int sum = 0;
                        for (int j = 0; j < 10000; ++j) {
                            sum += j;
                        }
                        workCounter.fetch_add(1);
                    }));
                }
                
                for (auto& future : futures) {
                    future.wait();
                }
                
                auto end = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
                durations.push_back(duration);
                
                REQUIRE(workCounter.load() == workAmount);
                
                INFO("Thread count: " << threadCount << ", Duration: " << duration.count() << "ms");
            }
            
            // Generally, more threads should improve performance up to a point
            // (though this depends on the specific hardware and work characteristics)
            REQUIRE(durations[3] <= durations[0]); // 8 threads should be faster than 1
        }
    }
    
    SECTION("Memory usage and resource management") {
        SECTION("Thread pool memory usage") {
            MemoryTracker::printMemoryStats("Before thread pool creation");
            
            {
                SimpleThreadPool pool(16);
                
                MemoryTracker::printMemoryStats("After thread pool creation");
                
                // Submit many tasks
                std::vector<std::future<void>> futures;
                for (int i = 0; i < 10000; ++i) {
                    futures.push_back(pool.enqueue([i] {
                        // Light work
                        volatile int result = i * i;
                        (void)result;
                    }));
                }
                
                MemoryTracker::printMemoryStats("After submitting 10000 tasks");
                
                // Wait for completion
                for (auto& future : futures) {
                    future.wait();
                }
                
                MemoryTracker::printMemoryStats("After task completion");
                
            } // Pool destroyed here
            
            MemoryTracker::printMemoryStats("After thread pool destruction");
        }
        
        SECTION("Task queue management") {
            SimpleThreadPool pool(2); // Intentionally small pool
            
            // Submit many tasks quickly
            std::vector<std::future<void>> futures;
            for (int i = 0; i < 100; ++i) {
                futures.push_back(pool.enqueue([i] {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }));
            }
            
            // Queue should have tasks
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            INFO("Pending tasks: " << pool.pending_tasks());
            
            // Wait for all to complete
            for (auto& future : futures) {
                future.wait();
            }
            
            // Queue should be empty
            REQUIRE(pool.pending_tasks() == 0);
        }
    }
}

TEST_CASE_METHOD(ThreadPoolTestFixture, "Thread Safety and Synchronization", "[threadpool][thread-safety][synchronization]") {
    
    SECTION("Thread safety of shared data") {
        SimpleThreadPool pool(8);
        
        SECTION("Atomic operations") {
            std::atomic<int> sharedCounter{0};
            const int incrementsPerThread = 1000;
            const int threadCount = 10;
            
            std::vector<std::future<void>> futures;
            
            for (int t = 0; t < threadCount; ++t) {
                futures.push_back(pool.enqueue([&sharedCounter, incrementsPerThread] {
                    for (int i = 0; i < incrementsPerThread; ++i) {
                        sharedCounter.fetch_add(1, std::memory_order_relaxed);
                    }
                }));
            }
            
            for (auto& future : futures) {
                future.wait();
            }
            
            REQUIRE(sharedCounter.load() == threadCount * incrementsPerThread);
        }
        
        SECTION("Mutex-protected operations") {
            std::vector<int> sharedVector;
            std::mutex vectorMutex;
            
            std::vector<std::future<void>> futures;
            const int elementsPerTask = 100;
            const int taskCount = 10;
            
            for (int t = 0; t < taskCount; ++t) {
                futures.push_back(pool.enqueue([&sharedVector, &vectorMutex, t, elementsPerTask] {
                    for (int i = 0; i < elementsPerTask; ++i) {
                        std::lock_guard<std::mutex> lock(vectorMutex);
                        sharedVector.push_back(t * elementsPerTask + i);
                    }
                }));
            }
            
            for (auto& future : futures) {
                future.wait();
            }
            
            REQUIRE(sharedVector.size() == taskCount * elementsPerTask);
            
            // All elements should be unique
            std::sort(sharedVector.begin(), sharedVector.end());
            auto uniqueEnd = std::unique(sharedVector.begin(), sharedVector.end());
            REQUIRE(uniqueEnd == sharedVector.end());
        }
        
        SECTION("Planet concurrent modification") {
            std::mutex planetMutex;
            std::atomic<int> modificationCount{0};
            
            std::vector<std::future<void>> futures;
            
            // Multiple threads modifying the same planet
            Planet& testPlanet = *testPlanets[0];
            
            for (int i = 0; i < 20; ++i) {
                futures.push_back(pool.enqueue([&testPlanet, &planetMutex, &modificationCount, i] {
                    std::lock_guard<std::mutex> lock(planetMutex);
                    
                    // Safely modify planet
                    int currentMinerals = testPlanet.getResourceAmount(ResourceType::MINERALS);
                    testPlanet.setResourceAmount(ResourceType::MINERALS, currentMinerals + i);
                    
                    testPlanet.update(0.1);
                    modificationCount.fetch_add(1);
                }));
            }
            
            for (auto& future : futures) {
                future.wait();
            }
            
            REQUIRE(modificationCount.load() == 20);
            REQUIRE(testPlanet.getHealth() > 0);
            
            // Planet should have been modified
            int finalMinerals = testPlanet.getResourceAmount(ResourceType::MINERALS);
            int expectedIncrease = 0;
            for (int i = 0; i < 20; ++i) {
                expectedIncrease += i;
            }
            
            REQUIRE(finalMinerals == 1000 + expectedIncrease); // 1000 was initial value
        }
    }
    
    SECTION("Deadlock prevention") {
        SimpleThreadPool pool(4);
        
        SECTION("Multiple mutex ordering") {
            std::mutex mutex1, mutex2;
            std::atomic<int> completedTasks{0};
            
            std::vector<std::future<void>> futures;
            
            // Tasks that acquire mutexes in consistent order
            for (int i = 0; i < 10; ++i) {
                futures.push_back(pool.enqueue([&mutex1, &mutex2, &completedTasks] {
                    // Always acquire mutex1 before mutex2
                    std::lock_guard<std::mutex> lock1(mutex1);
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    std::lock_guard<std::mutex> lock2(mutex2);
                    
                    completedTasks.fetch_add(1);
                }));
            }
            
            // All tasks should complete without deadlock
            for (auto& future : futures) {
                auto status = future.wait_for(std::chrono::seconds(5));
                REQUIRE(status == std::future_status::ready);
            }
            
            REQUIRE(completedTasks.load() == 10);
        }
        
        SECTION("Resource contention handling") {
            const int resourceCount = 5;
            std::vector<std::mutex> resourceMutexes(resourceCount);
            std::atomic<int> accessCount{0};
            
            std::vector<std::future<void>> futures;
            
            // Tasks that need multiple resources
            for (int i = 0; i < 20; ++i) {
                futures.push_back(pool.enqueue([&resourceMutexes, &accessCount, i] {
                    // Use deterministic ordering to prevent deadlocks
                    int resource1 = i % resourceCount;
                    int resource2 = (i + 1) % resourceCount;
                    
                    if (resource1 > resource2) {
                        std::swap(resource1, resource2);
                    }
                    
                    if (resource1 != resource2) {
                        std::lock_guard<std::mutex> lock1(resourceMutexes[resource1]);
                        std::lock_guard<std::mutex> lock2(resourceMutexes[resource2]);
                        
                        // Simulate work with resources
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                        accessCount.fetch_add(1);
                    } else {
                        std::lock_guard<std::mutex> lock(resourceMutexes[resource1]);
                        accessCount.fetch_add(1);
                    }
                }));
            }
            
            // All tasks should complete
            for (auto& future : futures) {
                auto status = future.wait_for(std::chrono::seconds(10));
                REQUIRE(status == std::future_status::ready);
            }
            
            REQUIRE(accessCount.load() == 20);
        }
    }
}

TEST_CASE_METHOD(ThreadPoolTestFixture, "Advanced Thread Pool Features", "[threadpool][advanced][features]") {
    
    SECTION("Priority task scheduling") {
        // This would require a more advanced thread pool implementation
        // For now, we'll simulate priority by submission order
        
        SimpleThreadPool pool(4);
        std::atomic<int> executionOrder{0};
        std::vector<int> completionOrder;
        std::mutex orderMutex;
        
        std::vector<std::future<void>> futures;
        
        // Submit high priority tasks first
        for (int i = 0; i < 5; ++i) {
            futures.push_back(pool.enqueue([&executionOrder, &completionOrder, &orderMutex, i] {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                
                int order = executionOrder.fetch_add(1);
                {
                    std::lock_guard<std::mutex> lock(orderMutex);
                    completionOrder.push_back(i);
                }
            }));
        }
        
        for (auto& future : futures) {
            future.wait();
        }
        
        REQUIRE(completionOrder.size() == 5);
        // In a simple FIFO thread pool, tasks complete roughly in submission order
    }
    
    SECTION("Task cancellation simulation") {
        SimpleThreadPool pool(2);
        
        std::atomic<bool> shouldCancel{false};
        std::atomic<int> completedTasks{0};
        std::atomic<int> cancelledTasks{0};
        
        std::vector<std::future<void>> futures;
        
        // Submit tasks that check for cancellation
        for (int i = 0; i < 10; ++i) {
            futures.push_back(pool.enqueue([&shouldCancel, &completedTasks, &cancelledTasks, i] {
                for (int j = 0; j < 100; ++j) {
                    if (shouldCancel.load()) {
                        cancelledTasks.fetch_add(1);
                        return;
                    }
                    
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
                
                completedTasks.fetch_add(1);
            }));
        }
        
        // Let some tasks start
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        
        // Signal cancellation
        shouldCancel.store(true);
        
        // Wait for all futures to complete
        for (auto& future : futures) {
            future.wait();
        }
        
        INFO("Completed tasks: " << completedTasks.load());
        INFO("Cancelled tasks: " << cancelledTasks.load());
        
        REQUIRE(completedTasks.load() + cancelledTasks.load() == 10);
        REQUIRE(cancelledTasks.load() > 0); // Some tasks should have been cancelled
    }
    
    SECTION("Work stealing simulation") {
        // Simulate work stealing by having tasks generate more work
        SimpleThreadPool pool(4);
        
        std::atomic<int> totalWork{0};
        std::atomic<int> workGenerated{0};
        
        std::function<void(int)> recursiveWork = [&](int depth) {
            totalWork.fetch_add(1);
            
            if (depth > 0) {
                // Generate more work
                pool.enqueue(recursiveWork, depth - 1);
                pool.enqueue(recursiveWork, depth - 1);
                workGenerated.fetch_add(2);
            }
        };
        
        // Start with initial work
        auto future = pool.enqueue(recursiveWork, 3);
        future.wait();
        
        // Wait a bit for all generated work to complete
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        INFO("Total work done: " << totalWork.load());
        INFO("Work generated: " << workGenerated.load());
        
        // Should have done exponential amount of work
        REQUIRE(totalWork.load() > 10);
    }
}

TEST_CASE("Thread Pool Exception Handling", "[threadpool][exceptions][error-handling]") {
    
    SECTION("Task exception propagation") {
        SimpleThreadPool pool(4);
        
        SECTION("Exception in single task") {
            auto future = pool.enqueue([]() -> int {
                throw std::runtime_error("Task exception");
                return 42;
            });
            
            REQUIRE_THROWS_AS(future.get(), std::runtime_error);
        }
        
        SECTION("Multiple tasks with exceptions") {
            std::vector<std::future<int>> futures;
            
            for (int i = 0; i < 5; ++i) {
                futures.push_back(pool.enqueue([i]() -> int {
                    if (i % 2 == 0) {
                        throw std::runtime_error("Even task exception");
                    }
                    return i;
                }));
            }
            
            int exceptionCount = 0;
            int successCount = 0;
            
            for (auto& future : futures) {
                try {
                    int result = future.get();
                    successCount++;
                    REQUIRE(result % 2 == 1); // Only odd numbers should succeed
                } catch (const std::runtime_error&) {
                    exceptionCount++;
                }
            }
            
            REQUIRE(exceptionCount == 3); // Even indices: 0, 2, 4
            REQUIRE(successCount == 2);   // Odd indices: 1, 3
        }
        
        SECTION("Thread pool stability after exceptions") {
            // Submit tasks that throw exceptions
            for (int i = 0; i < 10; ++i) {
                auto future = pool.enqueue([]() {
                    throw std::runtime_error("Stability test exception");
                });
                
                REQUIRE_THROWS_AS(future.get(), std::runtime_error);
            }
            
            // Pool should still be functional
            auto normalFuture = pool.enqueue([]() -> int {
                return 42;
            });
            
            REQUIRE(normalFuture.get() == 42);
        }
    }
    
    SECTION("Resource cleanup on exceptions") {
        SimpleThreadPool pool(2);
        
        std::atomic<int> constructorCalls{0};
        std::atomic<int> destructorCalls{0};
        
        class RAII_Resource {
        public:
            RAII_Resource(std::atomic<int>& constructors, std::atomic<int>& destructors)
                : constructors_(constructors), destructors_(destructors) {
                constructors_.fetch_add(1);
            }
            
            ~RAII_Resource() {
                destructors_.fetch_add(1);
            }
            
        private:
            std::atomic<int>& constructors_;
            std::atomic<int>& destructors_;
        };
        
        std::vector<std::future<void>> futures;
        
        for (int i = 0; i < 10; ++i) {
            futures.push_back(pool.enqueue([&constructorCalls, &destructorCalls, i]() {
                RAII_Resource resource(constructorCalls, destructorCalls);
                
                if (i % 3 == 0) {
                    throw std::runtime_error("RAII test exception");
                }
                
                // Do some work
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
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
        
        // Should have had exceptions for indices 0, 3, 6, 9
        REQUIRE(exceptionCount == 4);
        
        // All resources should be properly cleaned up
        REQUIRE(constructorCalls.load() == 10);
        REQUIRE(destructorCalls.load() == 10);
    }
}