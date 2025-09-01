// File: tests/benchmark_tests/ConcurrencyBenchmarks.cpp
// Threading performance benchmarks for CppVerseHub showcase

#include <catch2/catch.hpp>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <condition_variable>
#include <future>
#include <chrono>
#include <vector>
#include <random>
#include <functional>

// Include concurrency components
#include "ThreadPool.hpp"
#include "LockFreeQueue.hpp"
#include "AtomicOperations.hpp"
#include "Planet.hpp"
#include "Fleet.hpp"

using namespace CppVerseHub::Core;
using namespace CppVerseHub::Concurrency;

/**
 * @brief Benchmark fixture for concurrency performance tests
 */
class ConcurrencyBenchmarkFixture {
public:
    ConcurrencyBenchmarkFixture() {
        setupBenchmarkData();
    }
    
protected:
    void setupBenchmarkData() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> posDis(0.0, 1000.0);
        std::uniform_int_distribution<> resDis(100, 5000);
        
        // Generate test planets
        testPlanets.reserve(1000);
        for (int i = 0; i < 1000; ++i) {
            testPlanets.emplace_back(
                "ConcurrencyPlanet_" + std::to_string(i),
                Vector3D{posDis(gen), posDis(gen), posDis(gen)}
            );
            testPlanets.back().setResourceAmount(ResourceType::MINERALS, resDis(gen));
            testPlanets.back().setResourceAmount(ResourceType::ENERGY, resDis(gen));
        }
        
        // Generate test fleets
        testFleets.reserve(500);
        for (int i = 0; i < 500; ++i) {
            testFleets.emplace_back(
                "ConcurrencyFleet_" + std::to_string(i),
                Vector3D{posDis(gen), posDis(gen), posDis(gen)}
            );
            testFleets.back().addShips(ShipType::FIGHTER, 5 + (i % 20));
            testFleets.back().addShips(ShipType::CRUISER, 1 + (i % 5));
        }
        
        // Generate work items
        workItems.reserve(10000);
        std::uniform_int_distribution<> workDis(1, 1000);
        for (int i = 0; i < 10000; ++i) {
            workItems.push_back(workDis(gen));
        }
    }
    
    template<typename BenchmarkFunction>
    double benchmarkConcurrency(const std::string& name, BenchmarkFunction func, int iterations = 1) {
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < iterations; ++i) {
            func();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        return duration.count() / static_cast<double>(iterations);
    }
    
    std::vector<Planet> testPlanets;
    std::vector<Fleet> testFleets;
    std::vector<int> workItems;
};

TEST_CASE_METHOD(ConcurrencyBenchmarkFixture, "Thread Creation and Management Benchmarks", "[benchmark][concurrency][threads]") {
    
    SECTION("Thread creation overhead benchmark") {
        const int threadCount = 100;
        const int iterations = 5;
        
        auto threadCreationTime = benchmarkConcurrency("thread creation", [&]() {
            std::vector<std::thread> threads;
            threads.reserve(threadCount);
            
            for (int i = 0; i < threadCount; ++i) {
                threads.emplace_back([i]() {
                    // Minimal work
                    volatile int result = i * i;
                });
            }
            
            for (auto& thread : threads) {
                thread.join();
            }
        }, iterations);
        
        INFO("Thread creation (" << threadCount << " threads): " << threadCreationTime << "μs avg");
        REQUIRE(threadCreationTime > 0);
    }
    
    SECTION("Thread pool vs raw threads benchmark") {
        const int taskCount = 1000;
        const int iterations = 3;
        
        // Raw thread benchmark
        auto rawThreadTime = benchmarkConcurrency("raw threads", [&]() {
            const int threadsPerBatch = 10;
            std::vector<std::thread> threads;
            threads.reserve(threadsPerBatch);
            
            for (int batch = 0; batch < taskCount / threadsPerBatch; ++batch) {
                for (int i = 0; i < threadsPerBatch; ++i) {
                    int taskId = batch * threadsPerBatch + i;
                    threads.emplace_back([this, taskId]() {
                        // Simulate work
                        volatile int result = 0;
                        for (int j = 0; j < workItems[taskId % workItems.size()]; ++j) {
                            result += j;
                        }
                    });
                }
                
                for (auto& thread : threads) {
                    thread.join();
                }
                threads.clear();
            }
        }, iterations);
        
        // Thread pool benchmark
        auto threadPoolTime = benchmarkConcurrency("thread pool", [&]() {
            ThreadPool pool(std::thread::hardware_concurrency());
            std::vector<std::future<void>> futures;
            
            for (int i = 0; i < taskCount; ++i) {
                futures.push_back(pool.enqueue([this, i]() {
                    // Simulate work
                    volatile int result = 0;
                    for (int j = 0; j < workItems[i % workItems.size()]; ++j) {
                        result += j;
                    }
                }));
            }
            
            for (auto& future : futures) {
                future.wait();
            }
            
            pool.shutdown();
        }, iterations);
        
        INFO("Threading comparison (" << taskCount << " tasks):");
        INFO("Raw threads: " << rawThreadTime << "μs avg");
        INFO("Thread pool: " << threadPoolTime << "μs avg");
        INFO("Thread pool speedup: " << (rawThreadTime / threadPoolTime) << "x");
        
        REQUIRE(rawThreadTime > 0);
        REQUIRE(threadPoolTime > 0);
        // Thread pool should generally be more efficient
        REQUIRE(threadPoolTime <= rawThreadTime);
    }
    
    SECTION("Thread scaling benchmark") {
        const int workPerThread = 1000;
        std::vector<int> threadCounts = {1, 2, 4, 8, 16};
        
        for (int numThreads : threadCounts) {
            if (numThreads > static_cast<int>(std::thread::hardware_concurrency() * 2)) continue;
            
            auto scalingTime = benchmarkConcurrency("thread scaling", [&]() {
                std::vector<std::thread> threads;
                std::atomic<int> totalWork{0};
                
                for (int i = 0; i < numThreads; ++i) {
                    threads.emplace_back([&totalWork, workPerThread]() {
                        volatile int localWork = 0;
                        for (int j = 0; j < workPerThread; ++j) {
                            localWork += j * j;
                        }
                        totalWork.fetch_add(localWork, std::memory_order_relaxed);
                    });
                }
                
                for (auto& thread : threads) {
                    thread.join();
                }
            }, 5);
            
            double efficiency = (1000.0 / scalingTime) * numThreads;
            
            INFO("Threads: " << numThreads << ", Time: " << scalingTime << "μs, Efficiency: " << efficiency);
            
            REQUIRE(scalingTime > 0);
        }
    }
}

TEST_CASE_METHOD(ConcurrencyBenchmarkFixture, "Synchronization Primitive Benchmarks", "[benchmark][concurrency][synchronization]") {
    
    SECTION("Mutex contention benchmark") {
        const int threadCount = 8;
        const int operationsPerThread = 10000;
        const int iterations = 3;
        
        auto mutexContentionTime = benchmarkConcurrency("mutex contention", [&]() {
            std::mutex mtx;
            std::vector<std::thread> threads;
            std::atomic<int> counter{0};
            
            for (int i = 0; i < threadCount; ++i) {
                threads.emplace_back([&mtx, &counter, operationsPerThread]() {
                    for (int j = 0; j < operationsPerThread; ++j) {
                        std::lock_guard<std::mutex> lock(mtx);
                        counter.fetch_add(1, std::memory_order_relaxed);
                    }
                });
            }
            
            for (auto& thread : threads) {
                thread.join();
            }
        }, iterations);
        
        INFO("Mutex contention (" << threadCount << " threads, " << operationsPerThread << " ops each): " << mutexContentionTime << "μs avg");
        REQUIRE(mutexContentionTime > 0);
    }
    
    SECTION("Atomic vs mutex performance comparison") {
        const int threadCount = 6;
        const int operationsPerThread = 50000;
        const int iterations = 5;
        
        // Atomic increment benchmark
        auto atomicTime = benchmarkConcurrency("atomic increment", [&]() {
            std::atomic<int> counter{0};
            std::vector<std::thread> threads;
            
            for (int i = 0; i < threadCount; ++i) {
                threads.emplace_back([&counter, operationsPerThread]() {
                    for (int j = 0; j < operationsPerThread; ++j) {
                        counter.fetch_add(1, std::memory_order_relaxed);
                    }
                });
            }
            
            for (auto& thread : threads) {
                thread.join();
            }
        }, iterations);
        
        // Mutex-protected increment benchmark
        auto mutexTime = benchmarkConcurrency("mutex increment", [&]() {
            std::mutex mtx;
            int counter = 0;
            std::vector<std::thread> threads;
            
            for (int i = 0; i < threadCount; ++i) {
                threads.emplace_back([&mtx, &counter, operationsPerThread]() {
                    for (int j = 0; j < operationsPerThread; ++j) {
                        std::lock_guard<std::mutex> lock(mtx);
                        ++counter;
                    }
                });
            }
            
            for (auto& thread : threads) {
                thread.join();
            }
        }, iterations);
        
        INFO("Atomic vs Mutex comparison (" << threadCount << " threads, " << operationsPerThread << " ops each):");
        INFO("Atomic operations: " << atomicTime << "μs avg");
        INFO("Mutex operations: " << mutexTime << "μs avg");
        INFO("Atomic speedup: " << (mutexTime / atomicTime) << "x");
        
        REQUIRE(atomicTime > 0);
        REQUIRE(mutexTime > 0);
        REQUIRE(atomicTime <= mutexTime); // Atomic should be faster or equal
    }
    
    SECTION("Reader-writer lock benchmark") {
        const int readerThreads = 8;
        const int writerThreads = 2;
        const int operationsPerThread = 1000;
        const int iterations = 3;
        
        auto readerWriterTime = benchmarkConcurrency("reader-writer lock", [&]() {
            std::shared_mutex rwMutex;
            std::vector<int> sharedData(1000, 42);
            std::vector<std::thread> threads;
            
            // Reader threads
            for (int i = 0; i < readerThreads; ++i) {
                threads.emplace_back([&rwMutex, &sharedData, operationsPerThread]() {
                    for (int j = 0; j < operationsPerThread; ++j) {
                        std::shared_lock<std::shared_mutex> lock(rwMutex);
                        volatile int sum = 0;
                        for (int value : sharedData) {
                            sum += value;
                        }
                    }
                });
            }
            
            // Writer threads
            for (int i = 0; i < writerThreads; ++i) {
                threads.emplace_back([&rwMutex, &sharedData, operationsPerThread, i]() {
                    for (int j = 0; j < operationsPerThread; ++j) {
                        std::unique_lock<std::shared_mutex> lock(rwMutex);
                        sharedData[j % sharedData.size()] += i;
                    }
                });
            }
            
            for (auto& thread : threads) {
                thread.join();
            }
        }, iterations);
        
        INFO("Reader-writer lock (" << readerThreads << " readers, " << writerThreads << " writers): " << readerWriterTime << "μs avg");
        REQUIRE(readerWriterTime > 0);
    }
    
    SECTION("Condition variable performance") {
        const int producerThreads = 2;
        const int consumerThreads = 4;
        const int itemsPerProducer = 500;
        const int iterations = 3;
        
        auto conditionVariableTime = benchmarkConcurrency("condition variable", [&]() {
            std::mutex mtx;
            std::condition_variable cv;
            std::queue<int> queue;
            std::atomic<int> itemsProduced{0};
            std::atomic<int> itemsConsumed{0};
            std::atomic<bool> finished{false};
            
            std::vector<std::thread> threads;
            
            // Producer threads
            for (int i = 0; i < producerThreads; ++i) {
                threads.emplace_back([&, i]() {
                    for (int j = 0; j < itemsPerProducer; ++j) {
                        {
                            std::lock_guard<std::mutex> lock(mtx);
                            queue.push(i * itemsPerProducer + j);
                            itemsProduced.fetch_add(1);
                        }
                        cv.notify_one();
                    }
                });
            }
            
            // Consumer threads
            for (int i = 0; i < consumerThreads; ++i) {
                threads.emplace_back([&]() {
                    while (true) {
                        std::unique_lock<std::mutex> lock(mtx);
                        cv.wait(lock, [&]() { return !queue.empty() || finished.load(); });
                        
                        if (queue.empty() && finished.load()) break;
                        
                        if (!queue.empty()) {
                            volatile int item = queue.front();
                            queue.pop();
                            itemsConsumed.fetch_add(1);
                        }
                    }
                });
            }
            
            // Wait for producers to finish
            for (int i = 0; i < producerThreads; ++i) {
                threads[i].join();
            }
            
            // Signal consumers to finish
            finished.store(true);
            cv.notify_all();
            
            // Wait for consumers
            for (int i = producerThreads; i < threads.size(); ++i) {
                threads[i].join();
            }
        }, iterations);
        
        INFO("Condition variable (" << producerThreads << " producers, " << consumerThreads << " consumers): " << conditionVariableTime << "μs avg");
        REQUIRE(conditionVariableTime > 0);
    }
}

TEST_CASE_METHOD(ConcurrencyBenchmarkFixture, "Lock-Free Data Structure Benchmarks", "[benchmark][concurrency][lock-free]") {
    
    SECTION("Lock-free queue vs mutex queue benchmark") {
        const int producerThreads = 3;
        const int consumerThreads = 3;
        const int itemsPerProducer = 1000;
        const int iterations = 3;
        
        // Lock-free queue benchmark
        auto lockFreeTime = benchmarkConcurrency("lock-free queue", [&]() {
            LockFreeQueue<int> queue;
            std::vector<std::thread> threads;
            std::atomic<int> itemsProduced{0};
            std::atomic<int> itemsConsumed{0};
            
            // Producer threads
            for (int i = 0; i < producerThreads; ++i) {
                threads.emplace_back([&queue, &itemsProduced, itemsPerProducer, i]() {
                    for (int j = 0; j < itemsPerProducer; ++j) {
                        queue.push(i * itemsPerProducer + j);
                        itemsProduced.fetch_add(1);
                    }
                });
            }
            
            // Consumer threads
            for (int i = 0; i < consumerThreads; ++i) {
                threads.emplace_back([&queue, &itemsConsumed, &itemsProduced, itemsPerProducer, producerThreads]() {
                    int item;
                    while (itemsConsumed.load() < producerThreads * itemsPerProducer) {
                        if (queue.pop(item)) {
                            itemsConsumed.fetch_add(1);
                        } else {
                            std::this_thread::yield();
                        }
                    }
                });
            }
            
            for (auto& thread : threads) {
                thread.join();
            }
        }, iterations);
        
        // Mutex-based queue benchmark
        auto mutexQueueTime = benchmarkConcurrency("mutex queue", [&]() {
            std::mutex mtx;
            std::queue<int> queue;
            std::vector<std::thread> threads;
            std::atomic<int> itemsProduced{0};
            std::atomic<int> itemsConsumed{0};
            
            // Producer threads
            for (int i = 0; i < producerThreads; ++i) {
                threads.emplace_back([&mtx, &queue, &itemsProduced, itemsPerProducer, i]() {
                    for (int j = 0; j < itemsPerProducer; ++j) {
                        {
                            std::lock_guard<std::mutex> lock(mtx);
                            queue.push(i * itemsPerProducer + j);
                        }
                        itemsProduced.fetch_add(1);
                    }
                });
            }
            
            // Consumer threads
            for (int i = 0; i < consumerThreads; ++i) {
                threads.emplace_back([&mtx, &queue, &itemsConsumed, &itemsProduced, itemsPerProducer, producerThreads]() {
                    while (itemsConsumed.load() < producerThreads * itemsPerProducer) {
                        bool found = false;
                        {
                            std::lock_guard<std::mutex> lock(mtx);
                            if (!queue.empty()) {
                                volatile int item = queue.front();
                                queue.pop();
                                found = true;
                            }
                        }
                        if (found) {
                            itemsConsumed.fetch_add(1);
                        } else {
                            std::this_thread::yield();
                        }
                    }
                });
            }
            
            for (auto& thread : threads) {
                thread.join();
            }
        }, iterations);
        
        INFO("Queue comparison (" << producerThreads << " producers, " << consumerThreads << " consumers):");
        INFO("Lock-free queue: " << lockFreeTime << "μs avg");
        INFO("Mutex queue: " << mutexQueueTime << "μs avg");
        INFO("Lock-free speedup: " << (mutexQueueTime / lockFreeTime) << "x");
        
        REQUIRE(lockFreeTime > 0);
        REQUIRE(mutexQueueTime > 0);
    }
    
    SECTION("Atomic operations benchmark") {
        const int threadCount = 8;
        const int operationsPerThread = 100000;
        const int iterations = 5;
        
        // Compare different atomic operations
        std::vector<std::pair<std::string, std::function<void()>>> atomicTests;
        
        // fetch_add benchmark
        atomicTests.emplace_back("fetch_add", [&]() {
            std::atomic<int> counter{0};
            std::vector<std::thread> threads;
            
            for (int i = 0; i < threadCount; ++i) {
                threads.emplace_back([&counter, operationsPerThread]() {
                    for (int j = 0; j < operationsPerThread; ++j) {
                        counter.fetch_add(1, std::memory_order_relaxed);
                    }
                });
            }
            
            for (auto& thread : threads) {
                thread.join();
            }
        });
        
        // compare_exchange_weak benchmark
        atomicTests.emplace_back("compare_exchange_weak", [&]() {
            std::atomic<int> counter{0};
            std::vector<std::thread> threads;
            
            for (int i = 0; i < threadCount; ++i) {
                threads.emplace_back([&counter, operationsPerThread]() {
                    for (int j = 0; j < operationsPerThread; ++j) {
                        int expected = counter.load();
                        while (!counter.compare_exchange_weak(expected, expected + 1, 
                                                             std::memory_order_relaxed)) {
                            // Retry
                        }
                    }
                });
            }
            
            for (auto& thread : threads) {
                thread.join();
            }
        });
        
        // exchange benchmark
        atomicTests.emplace_back("exchange", [&]() {
            std::atomic<int> counter{0};
            std::vector<std::thread> threads;
            
            for (int i = 0; i < threadCount; ++i) {
                threads.emplace_back([&counter, operationsPerThread, i]() {
                    for (int j = 0; j < operationsPerThread; ++j) {
                        volatile int old = counter.exchange(i * operationsPerThread + j, std::memory_order_relaxed);
                    }
                });
            }
            
            for (auto& thread : threads) {
                thread.join();
            }
        });
        
        INFO("Atomic operations comparison (" << threadCount << " threads, " << operationsPerThread << " ops each):");
        
        for (const auto& test : atomicTests) {
            auto time = benchmarkConcurrency(test.first, test.second, iterations);
            INFO(test.first << ": " << time << "μs avg");
            REQUIRE(time > 0);
        }
    }
}

TEST_CASE_METHOD(ConcurrencyBenchmarkFixture, "Real-World Concurrency Scenarios", "[benchmark][concurrency][real-world]") {
    
    SECTION("Parallel planet processing benchmark") {
        const int planetCount = 500;
        const int iterations = 5;
        
        // Sequential processing
        auto sequentialTime = benchmarkConcurrency("sequential planet processing", [&]() {
            for (int i = 0; i < planetCount; ++i) {
                Planet& planet = testPlanets[i];
                
                // Simulate complex planet update
                planet.update(0.1);
                
                int minerals = planet.getResourceAmount(ResourceType::MINERALS);
                int energy = planet.getResourceAmount(ResourceType::ENERGY);
                
                // Simulate resource processing
                volatile int processed = minerals + energy;
                for (int j = 0; j < 100; ++j) {
                    processed += j;
                }
            }
        }, iterations);
        
        // Parallel processing
        auto parallelTime = benchmarkConcurrency("parallel planet processing", [&]() {
            const int numThreads = std::thread::hardware_concurrency();
            std::vector<std::thread> threads;
            
            int planetsPerThread = planetCount / numThreads;
            
            for (int t = 0; t < numThreads; ++t) {
                threads.emplace_back([&, t, planetsPerThread]() {
                    int start = t * planetsPerThread;
                    int end = (t == numThreads - 1) ? planetCount : start + planetsPerThread;
                    
                    for (int i = start; i < end; ++i) {
                        Planet& planet = testPlanets[i];
                        
                        // Simulate complex planet update
                        planet.update(0.1);
                        
                        int minerals = planet.getResourceAmount(ResourceType::MINERALS);
                        int energy = planet.getResourceAmount(ResourceType::ENERGY);
                        
                        // Simulate resource processing
                        volatile int processed = minerals + energy;
                        for (int j = 0; j < 100; ++j) {
                            processed += j;
                        }
                    }
                });
            }
            
            for (auto& thread : threads) {
                thread.join();
            }
        }, iterations);
        
        INFO("Planet processing comparison (" << planetCount << " planets):");
        INFO("Sequential: " << sequentialTime << "μs avg");
        INFO("Parallel (" << std::thread::hardware_concurrency() << " threads): " << parallelTime << "μs avg");
        INFO("Parallel speedup: " << (sequentialTime / parallelTime) << "x");
        
        REQUIRE(sequentialTime > 0);
        REQUIRE(parallelTime > 0);
        REQUIRE(parallelTime <= sequentialTime); // Parallel should be faster or equal
    }
    
    SECTION("Fleet combat simulation benchmark") {
        const int combatRounds = 1000;
        const int iterations = 3;
        
        auto combatTime = benchmarkConcurrency("fleet combat simulation", [&]() {
            const int numThreads = 4;
            std::vector<std::thread> threads;
            std::atomic<int> totalDamage{0};
            
            for (int t = 0; t < numThreads; ++t) {
                threads.emplace_back([&, t]() {
                    std::random_device rd;
                    std::mt19937 gen(rd());
                    std::uniform_int_distribution<> damageDis(10, 100);
                    
                    for (int round = 0; round < combatRounds / numThreads; ++round) {
                        Fleet& fleet1 = testFleets[(t * 2) % testFleets.size()];
                        Fleet& fleet2 = testFleets[(t * 2 + 1) % testFleets.size()];
                        
                        // Simulate combat calculations
                        int fleet1Power = fleet1.getShipCount(ShipType::FIGHTER) * 10 +
                                        fleet1.getShipCount(ShipType::CRUISER) * 25 +
                                        fleet1.getShipCount(ShipType::BATTLESHIP) * 50;
                        
                        int fleet2Power = fleet2.getShipCount(ShipType::FIGHTER) * 10 +
                                        fleet2.getShipCount(ShipType::CRUISER) * 25 +
                                        fleet2.getShipCount(ShipType::BATTLESHIP) * 50;
                        
                        // Simulate damage calculation
                        int damage = std::abs(fleet1Power - fleet2Power) + damageDis(gen);
                        totalDamage.fetch_add(damage, std::memory_order_relaxed);
                        
                        // Simulate position updates
                        volatile double distance = fleet1.getPosition().distanceTo(fleet2.getPosition());
                    }
                });
            }
            
            for (auto& thread : threads) {
                thread.join();
            }
        }, iterations);
        
        INFO("Fleet combat simulation (" << combatRounds << " rounds): " << combatTime << "μs avg");
        REQUIRE(combatTime > 0);
    }
    
    SECTION("Resource management scalability") {
        const int resourceUpdates = 10000;
        std::vector<int> threadCounts = {1, 2, 4, 8};
        
        for (int numThreads : threadCounts) {
            if (numThreads > static_cast<int>(std::thread::hardware_concurrency())) continue;
            
            auto resourceTime = benchmarkConcurrency("resource management", [&]() {
                std::vector<std::atomic<int>> resources(100);
                std::vector<std::thread> threads;
                
                for (int t = 0; t < numThreads; ++t) {
                    threads.emplace_back([&resources, resourceUpdates, numThreads, t]() {
                        std::random_device rd;
                        std::mt19937 gen(rd());
                        std::uniform_int_distribution<> resourceDis(0, resources.size() - 1);
                        std::uniform_int_distribution<> amountDis(1, 10);
                        
                        for (int i = 0; i < resourceUpdates / numThreads; ++i) {
                            int resourceId = resourceDis(gen);
                            int amount = amountDis(gen);
                            
                            resources[resourceId].fetch_add(amount, std::memory_order_relaxed);
                            
                            // Simulate resource consumption
                            if (i % 10 == 0) {
                                resources[resourceId].fetch_sub(1, std::memory_order_relaxed);
                            }
                        }
                    });
                }
                
                for (auto& thread : threads) {
                    thread.join();
                }
            }, 3);
            
            double throughput = resourceUpdates / (resourceTime / 1000000.0); // operations per second
            
            INFO("Resource management (" << numThreads << " threads): " << resourceTime << "μs, " << throughput << " ops/sec");
            
            REQUIRE(resourceTime > 0);
        }
    }
    
    SECTION("Memory bandwidth benchmark") {
        const int dataSize = 1000000;
        const int iterations = 3;
        
        std::vector<int> data(dataSize);
        std::iota(data.begin(), data.end(), 0);
        
        // Sequential memory access
        auto sequentialMemory = benchmarkConcurrency("sequential memory access", [&]() {
            volatile long sum = 0;
            for (int value : data) {
                sum += value;
            }
        }, iterations);
        
        // Parallel memory access
        auto parallelMemory = benchmarkConcurrency("parallel memory access", [&]() {
            const int numThreads = std::thread::hardware_concurrency();
            std::vector<std::thread> threads;
            std::atomic<long> totalSum{0};
            
            int chunkSize = dataSize / numThreads;
            
            for (int t = 0; t < numThreads; ++t) {
                threads.emplace_back([&data, &totalSum, t, chunkSize, dataSize]() {
                    int start = t * chunkSize;
                    int end = (t == std::thread::hardware_concurrency() - 1) ? dataSize : start + chunkSize;
                    
                    long localSum = 0;
                    for (int i = start; i < end; ++i) {
                        localSum += data[i];
                    }
                    
                    totalSum.fetch_add(localSum, std::memory_order_relaxed);
                });
            }
            
            for (auto& thread : threads) {
                thread.join();
            }
        }, iterations);
        
        double sequentialBandwidth = (dataSize * sizeof(int)) / (sequentialMemory / 1000000.0); // bytes per second
        double parallelBandwidth = (dataSize * sizeof(int)) / (parallelMemory / 1000000.0);
        
        INFO("Memory bandwidth comparison (" << dataSize << " integers):");
        INFO("Sequential: " << sequentialMemory << "μs, " << (sequentialBandwidth / 1e9) << " GB/s");
        INFO("Parallel: " << parallelMemory << "μs, " << (parallelBandwidth / 1e9) << " GB/s");
        INFO("Bandwidth improvement: " << (parallelBandwidth / sequentialBandwidth) << "x");
        
        REQUIRE(sequentialMemory > 0);
        REQUIRE(parallelMemory > 0);
    }
}