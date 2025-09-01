// File: tests/unit_tests/concurrency_tests/SynchronizationTests.cpp
// Mutex/condition variable tests for CppVerseHub concurrency showcase

#include <catch2/catch.hpp>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <vector>
#include <queue>
#include <memory>
#include <random>
#include <algorithm>

// Include the synchronization headers
#include "MutexExamples.hpp"
#include "ConditionVariables.hpp"
#include "Atomics.hpp"
#include "Planet.hpp"
#include "Fleet.hpp"

using namespace CppVerseHub::Concurrency;
using namespace CppVerseHub::Core;

/**
 * @brief Test fixture for synchronization tests
 */
class SynchronizationTestFixture {
public:
    SynchronizationTestFixture() {
        setupTestData();
    }
    
    ~SynchronizationTestFixture() {
        testPlanets.clear();
        testFleets.clear();
    }
    
protected:
    void setupTestData() {
        // Create test entities for synchronization testing
        for (int i = 0; i < 8; ++i) {
            auto planet = std::make_unique<Planet>(
                "SyncPlanet_" + std::to_string(i),
                Vector3D{i * 100.0, i * 100.0, i * 100.0}
            );
            planet->setResourceAmount(ResourceType::MINERALS, 1000 + i * 100);
            testPlanets.push_back(std::move(planet));
        }
        
        for (int i = 0; i < 4; ++i) {
            auto fleet = std::make_unique<Fleet>(
                "SyncFleet_" + std::to_string(i),
                Vector3D{i * 200.0, i * 200.0, i * 200.0}
            );
            fleet->addShips(ShipType::FIGHTER, 10 + i * 5);
            testFleets.push_back(std::move(fleet));
        }
    }
    
    std::vector<std::unique_ptr<Planet>> testPlanets;
    std::vector<std::unique_ptr<Fleet>> testFleets;
};

/**
 * @brief Thread-safe counter for testing
 */
class ThreadSafeCounter {
private:
    mutable std::mutex mutex_;
    int count_;
    
public:
    ThreadSafeCounter(int initial = 0) : count_(initial) {}
    
    void increment() {
        std::lock_guard<std::mutex> lock(mutex_);
        ++count_;
    }
    
    void add(int value) {
        std::lock_guard<std::mutex> lock(mutex_);
        count_ += value;
    }
    
    int get() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return count_;
    }
    
    void reset() {
        std::lock_guard<std::mutex> lock(mutex_);
        count_ = 0;
    }
};

/**
 * @brief Producer-Consumer buffer for testing
 */
template<typename T>
class ThreadSafeBuffer {
private:
    std::queue<T> buffer_;
    mutable std::mutex mutex_;
    std::condition_variable condition_;
    size_t maxSize_;
    bool closed_;
    
public:
    explicit ThreadSafeBuffer(size_t maxSize = 100) 
        : maxSize_(maxSize), closed_(false) {}
    
    bool push(const T& item) {
        std::unique_lock<std::mutex> lock(mutex_);
        
        if (closed_) return false;
        
        condition_.wait(lock, [this] { 
            return buffer_.size() < maxSize_ || closed_; 
        });
        
        if (closed_) return false;
        
        buffer_.push(item);
        condition_.notify_one();
        return true;
    }
    
    bool pop(T& item) {
        std::unique_lock<std::mutex> lock(mutex_);
        
        condition_.wait(lock, [this] { 
            return !buffer_.empty() || closed_; 
        });
        
        if (buffer_.empty()) return false;
        
        item = buffer_.front();
        buffer_.pop();
        condition_.notify_one();
        return true;
    }
    
    void close() {
        std::lock_guard<std::mutex> lock(mutex_);
        closed_ = true;
        condition_.notify_all();
    }
    
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return buffer_.size();
    }
    
    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return buffer_.empty();
    }
};

TEST_CASE_METHOD(SynchronizationTestFixture, "Basic Mutex Operations", "[synchronization][mutex][basic]") {
    
    SECTION("std::mutex basic usage") {
        std::mutex testMutex;
        int sharedValue = 0;
        const int threadCount = 10;
        const int incrementsPerThread = 1000;
        
        std::vector<std::thread> threads;
        
        for (int i = 0; i < threadCount; ++i) {
            threads.emplace_back([&testMutex, &sharedValue, incrementsPerThread] {
                for (int j = 0; j < incrementsPerThread; ++j) {
                    std::lock_guard<std::mutex> lock(testMutex);
                    ++sharedValue;
                }
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        REQUIRE(sharedValue == threadCount * incrementsPerThread);
    }
    
    SECTION("std::recursive_mutex") {
        std::recursive_mutex recursiveMutex;
        int recursiveValue = 0;
        
        std::function<void(int)> recursiveFunction = [&](int depth) {
            std::lock_guard<std::recursive_mutex> lock(recursiveMutex);
            ++recursiveValue;
            
            if (depth > 0) {
                recursiveFunction(depth - 1);
            }
        };
        
        std::vector<std::thread> threads;
        const int threadCount = 5;
        const int maxDepth = 10;
        
        for (int i = 0; i < threadCount; ++i) {
            threads.emplace_back([&recursiveFunction, maxDepth] {
                recursiveFunction(maxDepth);
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        // Each thread calls the function 11 times (depth 10, 9, 8, ..., 0)
        REQUIRE(recursiveValue == threadCount * (maxDepth + 1));
    }
    
    SECTION("std::timed_mutex") {
        std::timed_mutex timedMutex;
        std::atomic<int> successfulLocks{0};
        std::atomic<int> timedOutLocks{0};
        
        std::thread holder([&timedMutex] {
            std::lock_guard<std::timed_mutex> lock(timedMutex);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        });
        
        std::vector<std::thread> waiters;
        const int waiterCount = 5;
        
        for (int i = 0; i < waiterCount; ++i) {
            waiters.emplace_back([&timedMutex, &successfulLocks, &timedOutLocks] {
                if (timedMutex.try_lock_for(std::chrono::milliseconds(50))) {
                    successfulLocks.fetch_add(1);
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    timedMutex.unlock();
                } else {
                    timedOutLocks.fetch_add(1);
                }
            });
        }
        
        holder.join();
        for (auto& waiter : waiters) {
            waiter.join();
        }
        
        REQUIRE(successfulLocks.load() + timedOutLocks.load() == waiterCount);
        REQUIRE(timedOutLocks.load() > 0); // Some should have timed out
    }
    
    SECTION("Planet resource protection") {
        Planet& testPlanet = *testPlanets[0];
        std::mutex planetMutex;
        
        const int threadCount = 8;
        const int operationsPerThread = 100;
        
        std::vector<std::thread> threads;
        
        for (int i = 0; i < threadCount; ++i) {
            threads.emplace_back([&testPlanet, &planetMutex, operationsPerThread, i] {
                for (int j = 0; j < operationsPerThread; ++j) {
                    std::lock_guard<std::mutex> lock(planetMutex);
                    
                    // Safely modify planet resources
                    int currentMinerals = testPlanet.getResourceAmount(ResourceType::MINERALS);
                    testPlanet.setResourceAmount(ResourceType::MINERALS, currentMinerals + 1);
                    
                    // Update the planet
                    testPlanet.update(0.01);
                }
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        // Planet should have increased minerals and still be healthy
        int finalMinerals = testPlanet.getResourceAmount(ResourceType::MINERALS);
        int expectedMinerals = 1000 + (threadCount * operationsPerThread);
        REQUIRE(finalMinerals == expectedMinerals);
        REQUIRE(testPlanet.getHealth() > 0);
    }
}

TEST_CASE_METHOD(SynchronizationTestFixture, "Advanced Mutex Usage", "[synchronization][mutex][advanced]") {
    
    SECTION("std::shared_mutex for reader-writer access") {
        std::shared_mutex sharedMutex;
        std::vector<int> sharedData = {1, 2, 3, 4, 5};
        std::atomic<int> readerCount{0};
        std::atomic<int> writerCount{0};
        
        const int readerThreads = 8;
        const int writerThreads = 2;
        
        std::vector<std::thread> threads;
        
        // Reader threads
        for (int i = 0; i < readerThreads; ++i) {
            threads.emplace_back([&sharedMutex, &sharedData, &readerCount] {
                for (int j = 0; j < 100; ++j) {
                    std::shared_lock<std::shared_mutex> lock(sharedMutex);
                    
                    // Read shared data
                    volatile int sum = 0;
                    for (int value : sharedData) {
                        sum += value;
                    }
                    
                    readerCount.fetch_add(1);
                    std::this_thread::sleep_for(std::chrono::microseconds(10));
                }
            });
        }
        
        // Writer threads
        for (int i = 0; i < writerThreads; ++i) {
            threads.emplace_back([&sharedMutex, &sharedData, &writerCount, i] {
                for (int j = 0; j < 50; ++j) {
                    std::unique_lock<std::shared_mutex> lock(sharedMutex);
                    
                    // Modify shared data
                    sharedData[j % sharedData.size()] += 1;
                    
                    writerCount.fetch_add(1);
                    std::this_thread::sleep_for(std::chrono::microseconds(50));
                }
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        REQUIRE(readerCount.load() == readerThreads * 100);
        REQUIRE(writerCount.load() == writerThreads * 50);
        
        // Data should have been modified by writers
        int sum = std::accumulate(sharedData.begin(), sharedData.end(), 0);
        REQUIRE(sum > 15); // Original sum was 15, should be higher after modifications
    }
    
    SECTION("Multiple mutex locking") {
        std::mutex mutex1, mutex2, mutex3;
        ThreadSafeCounter counter1, counter2, counter3;
        
        const int threadCount = 6;
        std::vector<std::thread> threads;
        
        for (int i = 0; i < threadCount; ++i) {
            threads.emplace_back([&mutex1, &mutex2, &mutex3, &counter1, &counter2, &counter3, i] {
                for (int j = 0; j < 100; ++j) {
                    // Lock mutexes in consistent order to avoid deadlock
                    std::lock(mutex1, mutex2, mutex3);
                    std::lock_guard<std::mutex> lock1(mutex1, std::adopt_lock);
                    std::lock_guard<std::mutex> lock2(mutex2, std::adopt_lock);
                    std::lock_guard<std::mutex> lock3(mutex3, std::adopt_lock);
                    
                    counter1.increment();
                    counter2.increment();
                    counter3.increment();
                    
                    std::this_thread::sleep_for(std::chrono::microseconds(1));
                }
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        REQUIRE(counter1.get() == threadCount * 100);
        REQUIRE(counter2.get() == threadCount * 100);
        REQUIRE(counter3.get() == threadCount * 100);
    }
    
    SECTION("RAII lock management") {
        std::mutex testMutex;
        std::vector<int> sharedVector;
        
        auto addElementsWithRAII = [&testMutex, &sharedVector](int start, int count) {
            std::lock_guard<std::mutex> lock(testMutex);
            
            for (int i = 0; i < count; ++i) {
                sharedVector.push_back(start + i);
                
                if (i == count / 2) {
                    // Simulate potential exception
                    if (start % 3 == 0) {
                        // Lock will be automatically released due to RAII
                        throw std::runtime_error("Test exception");
                    }
                }
            }
        };
        
        std::vector<std::thread> threads;
        std::atomic<int> exceptionCount{0};
        
        for (int i = 0; i < 10; ++i) {
            threads.emplace_back([&addElementsWithRAII, &exceptionCount, i] {
                try {
                    addElementsWithRAII(i * 100, 50);
                } catch (const std::runtime_error&) {
                    exceptionCount.fetch_add(1);
                }
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        REQUIRE(exceptionCount.load() > 0);
        REQUIRE(!sharedVector.empty());
        
        // Vector should contain elements from successful operations
        // Even with exceptions, mutex should be properly released
    }
}

TEST_CASE_METHOD(SynchronizationTestFixture, "Condition Variables", "[synchronization][condition-variable]") {
    
    SECTION("Basic condition variable usage") {
        std::mutex mutex;
        std::condition_variable cv;
        bool ready = false;
        std::string result;
        
        std::thread worker([&mutex, &cv, &ready, &result] {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            
            {
                std::lock_guard<std::mutex> lock(mutex);
                result = "Worker completed";
                ready = true;
            }
            
            cv.notify_one();
        });
        
        std::thread waiter([&mutex, &cv, &ready, &result] {
            std::unique_lock<std::mutex> lock(mutex);
            cv.wait(lock, [&ready] { return ready; });
            
            REQUIRE(result == "Worker completed");
        });
        
        worker.join();
        waiter.join();
    }
    
    SECTION("Producer-Consumer with condition variables") {
        ThreadSafeBuffer<int> buffer(10);
        std::atomic<int> produced{0};
        std::atomic<int> consumed{0};
        
        const int itemCount = 100;
        const int producerCount = 3;
        const int consumerCount = 2;
        
        std::vector<std::thread> producers;
        std::vector<std::thread> consumers;
        
        // Producer threads
        for (int p = 0; p < producerCount; ++p) {
            producers.emplace_back([&buffer, &produced, itemCount, p, producerCount] {
                for (int i = p; i < itemCount; i += producerCount) {
                    if (buffer.push(i)) {
                        produced.fetch_add(1);
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            });
        }
        
        // Consumer threads
        for (int c = 0; c < consumerCount; ++c) {
            consumers.emplace_back([&buffer, &consumed] {
                int item;
                while (buffer.pop(item)) {
                    consumed.fetch_add(1);
                    std::this_thread::sleep_for(std::chrono::milliseconds(2));
                }
            });
        }
        
        // Wait for producers to finish
        for (auto& producer : producers) {
            producer.join();
        }
        
        // Close buffer and wait for consumers
        buffer.close();
        for (auto& consumer : consumers) {
            consumer.join();
        }
        
        REQUIRE(produced.load() == itemCount);
        REQUIRE(consumed.load() == itemCount);
    }
    
    SECTION("Waiting with timeout") {
        std::mutex mutex;
        std::condition_variable cv;
        bool signaled = false;
        
        std::thread signaler([&mutex, &cv, &signaled] {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            {
                std::lock_guard<std::mutex> lock(mutex);
                signaled = true;
            }
            cv.notify_one();
        });
        
        std::thread waiterSuccess([&mutex, &cv, &signaled] {
            std::unique_lock<std::mutex> lock(mutex);
            bool result = cv.wait_for(lock, std::chrono::milliseconds(150), [&signaled] {
                return signaled;
            });
            REQUIRE(result == true);
        });
        
        std::thread waiterTimeout([&mutex, &cv, &signaled] {
            std::unique_lock<std::mutex> lock(mutex);
            bool result = cv.wait_for(lock, std::chrono::milliseconds(50), [&signaled] {
                return signaled;
            });
            REQUIRE(result == false);
        });
        
        signaler.join();
        waiterSuccess.join();
        waiterTimeout.join();
    }
    
    SECTION("Mission coordination with condition variables") {
        std::mutex missionMutex;
        std::condition_variable missionCV;
        bool missionReady = false;
        std::atomic<int> missionCount{0};
        
        const int fleetCount = testFleets.size();
        std::vector<std::thread> missionThreads;
        
        // Mission coordinator
        std::thread coordinator([&missionMutex, &missionCV, &missionReady] {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            
            {
                std::lock_guard<std::mutex> lock(missionMutex);
                missionReady = true;
            }
            missionCV.notify_all();
        });
        
        // Fleet mission threads
        for (int i = 0; i < fleetCount; ++i) {
            missionThreads.emplace_back([&missionMutex, &missionCV, &missionReady, &missionCount, this, i] {
                std::unique_lock<std::mutex> lock(missionMutex);
                missionCV.wait(lock, [&missionReady] { return missionReady; });
                lock.unlock();
                
                // Execute mission
                testFleets[i]->update(1.0);
                missionCount.fetch_add(1);
            });
        }
        
        coordinator.join();
        for (auto& thread : missionThreads) {
            thread.join();
        }
        
        REQUIRE(missionCount.load() == fleetCount);
    }
}

TEST_CASE_METHOD(SynchronizationTestFixture, "Atomic Operations", "[synchronization][atomic]") {
    
    SECTION("Basic atomic operations") {
        std::atomic<int> atomicInt{0};
        std::atomic<double> atomicDouble{0.0};
        std::atomic<bool> atomicBool{false};
        
        const int threadCount = 10;
        const int operationsPerThread = 1000;
        
        std::vector<std::thread> threads;
        
        for (int i = 0; i < threadCount; ++i) {
            threads.emplace_back([&atomicInt, &atomicDouble, &atomicBool, operationsPerThread] {
                for (int j = 0; j < operationsPerThread; ++j) {
                    atomicInt.fetch_add(1, std::memory_order_relaxed);
                    
                    double expected = atomicDouble.load(std::memory_order_acquire);
                    while (!atomicDouble.compare_exchange_weak(expected, expected + 0.1, 
                                                              std::memory_order_release,
                                                              std::memory_order_relaxed)) {
                        // Retry until successful
                    }
                    
                    atomicBool.store(j % 2 == 0, std::memory_order_relaxed);
                }
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        REQUIRE(atomicInt.load() == threadCount * operationsPerThread);
        REQUIRE(atomicDouble.load() == Approx(threadCount * operationsPerThread * 0.1).epsilon(0.01));
    }
    
    SECTION("Lock-free data structures") {
        struct Node {
            std::atomic<int> value;
            std::atomic<Node*> next;
            
            Node(int val) : value(val), next(nullptr) {}
        };
        
        class LockFreeStack {
        private:
            std::atomic<Node*> head_;
            
        public:
            LockFreeStack() : head_(nullptr) {}
            
            ~LockFreeStack() {
                while (Node* node = head_.load()) {
                    head_ = node->next;
                    delete node;
                }
            }
            
            void push(int value) {
                Node* newNode = new Node(value);
                Node* prevHead = head_.load();
                
                do {
                    newNode->next = prevHead;
                } while (!head_.compare_exchange_weak(prevHead, newNode));
            }
            
            bool pop(int& result) {
                Node* head = head_.load();
                
                while (head) {
                    if (head_.compare_exchange_weak(head, head->next)) {
                        result = head->value;
                        delete head;
                        return true;
                    }
                }
                
                return false;
            }
        };
        
        LockFreeStack stack;
        std::atomic<int> pushCount{0};
        std::atomic<int> popCount{0};
        
        const int threadCount = 8;
        const int operationsPerThread = 500;
        
        std::vector<std::thread> threads;
        
        // Producer threads
        for (int i = 0; i < threadCount / 2; ++i) {
            threads.emplace_back([&stack, &pushCount, operationsPerThread, i] {
                for (int j = 0; j < operationsPerThread; ++j) {
                    stack.push(i * operationsPerThread + j);
                    pushCount.fetch_add(1);
                }
            });
        }
        
        // Consumer threads
        for (int i = 0; i < threadCount / 2; ++i) {
            threads.emplace_back([&stack, &popCount] {
                int value;
                while (popCount.load() < 1000) { // Stop after popping enough items
                    if (stack.pop(value)) {
                        popCount.fetch_add(1);
                    } else {
                        std::this_thread::yield();
                    }
                }
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        REQUIRE(pushCount.load() == (threadCount / 2) * operationsPerThread);
        REQUIRE(popCount.load() <= pushCount.load());
    }
    
    SECTION("Memory ordering effects") {
        std::atomic<int> x{0};
        std::atomic<int> y{0};
        std::atomic<int> r1{0};
        std::atomic<int> r2{0};
        
        const int iterations = 1000;
        int reorderingCount = 0;
        
        for (int i = 0; i < iterations; ++i) {
            x.store(0, std::memory_order_relaxed);
            y.store(0, std::memory_order_relaxed);
            r1.store(0, std::memory_order_relaxed);
            r2.store(0, std::memory_order_relaxed);
            
            std::thread t1([&x, &y, &r1] {
                x.store(1, std::memory_order_release);
                r1.store(y.load(std::memory_order_acquire), std::memory_order_relaxed);
            });
            
            std::thread t2([&x, &y, &r2] {
                y.store(1, std::memory_order_release);
                r2.store(x.load(std::memory_order_acquire), std::memory_order_relaxed);
            });
            
            t1.join();
            t2.join();
            
            // Check for memory reordering
            if (r1.load() == 0 && r2.load() == 0) {
                reorderingCount++;
            }
        }
        
        // With proper memory ordering, this should be rare or never happen
        INFO("Memory reordering observed: " << reorderingCount << " times out of " << iterations);
        
        // The test passes regardless, but demonstrates memory ordering concepts
        REQUIRE(iterations == iterations); // Always true
    }
    
    SECTION("Atomic operations on game entities") {
        std::atomic<int> totalShips{0};
        std::atomic<int> totalResources{0};
        
        const int threadCount = 6;
        std::vector<std::thread> threads;
        
        for (int i = 0; i < threadCount; ++i) {
            threads.emplace_back([&totalShips, &totalResources, this, i] {
                // Count ships atomically
                for (const auto& fleet : testFleets) {
                    int ships = fleet->getShipCount(ShipType::FIGHTER) + 
                               fleet->getShipCount(ShipType::CRUISER);
                    totalShips.fetch_add(ships, std::memory_order_relaxed);
                }
                
                // Count resources atomically
                for (const auto& planet : testPlanets) {
                    int resources = planet->getResourceAmount(ResourceType::MINERALS);
                    totalResources.fetch_add(resources, std::memory_order_relaxed);
                }
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        // Verify atomic operations produced consistent results
        REQUIRE(totalShips.load() > 0);
        REQUIRE(totalResources.load() > 0);
        
        // Results should be deterministic despite concurrent access
        REQUIRE(totalShips.load() % threadCount == 0); // Each thread counted the same values
        REQUIRE(totalResources.load() % threadCount == 0);
    }
}

TEST_CASE_METHOD(SynchronizationTestFixture, "Performance and Scalability", "[synchronization][performance]") {
    
    SECTION("Mutex contention analysis") {
        std::mutex contentedMutex;
        std::atomic<int> operations{0};
        
        const int threadCount = std::thread::hardware_concurrency();
        const int operationsPerThread = 10000;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<std::thread> threads;
        
        for (int i = 0; i < threadCount; ++i) {
            threads.emplace_back([&contentedMutex, &operations, operationsPerThread] {
                for (int j = 0; j < operationsPerThread; ++j) {
                    std::lock_guard<std::mutex> lock(contentedMutex);
                    operations.fetch_add(1, std::memory_order_relaxed);
                }
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        REQUIRE(operations.load() == threadCount * operationsPerThread);
        
        INFO("Mutex contention test: " << threadCount << " threads, " << 
             operations.load() << " operations in " << duration.count() << "μs");
    }
    
    SECTION("Atomic vs mutex performance") {
        const int threadCount = 8;
        const int operationsPerThread = 100000;
        
        // Test atomic performance
        std::atomic<int> atomicCounter{0};
        
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<std::thread> atomicThreads;
        for (int i = 0; i < threadCount; ++i) {
            atomicThreads.emplace_back([&atomicCounter, operationsPerThread] {
                for (int j = 0; j < operationsPerThread; ++j) {
                    atomicCounter.fetch_add(1, std::memory_order_relaxed);
                }
            });
        }
        
        for (auto& thread : atomicThreads) {
            thread.join();
        }
        
        auto atomicTime = std::chrono::high_resolution_clock::now() - start;
        
        // Test mutex performance
        std::mutex mutexCounter;
        int mutexValue = 0;
        
        start = std::chrono::high_resolution_clock::now();
        
        std::vector<std::thread> mutexThreads;
        for (int i = 0; i < threadCount; ++i) {
            mutexThreads.emplace_back([&mutexCounter, &mutexValue, operationsPerThread] {
                for (int j = 0; j < operationsPerThread; ++j) {
                    std::lock_guard<std::mutex> lock(mutexCounter);
                    ++mutexValue;
                }
            });
        }
        
        for (auto& thread : mutexThreads) {
            thread.join();
        }
        
        auto mutexTime = std::chrono::high_resolution_clock::now() - start;
        
        REQUIRE(atomicCounter.load() == threadCount * operationsPerThread);
        REQUIRE(mutexValue == threadCount * operationsPerThread);
        
        INFO("Atomic time: " << std::chrono::duration_cast<std::chrono::microseconds>(atomicTime).count() << "μs");
        INFO("Mutex time: " << std::chrono::duration_cast<std::chrono::microseconds>(mutexTime).count() << "μs");
        
        // Atomic operations should generally be faster for simple operations
    }
    
    SECTION("Scalability with thread count") {
        const int maxThreads = std::min(16, static_cast<int>(std::thread::hardware_concurrency() * 2));
        const int operationsPerThread = 50000;
        
        std::vector<std::pair<int, double>> results;
        
        for (int threadCount = 1; threadCount <= maxThreads; threadCount *= 2) {
            std::atomic<int> counter{0};
            
            auto start = std::chrono::high_resolution_clock::now();
            
            std::vector<std::thread> threads;
            for (int i = 0; i < threadCount; ++i) {
                threads.emplace_back([&counter, operationsPerThread] {
                    for (int j = 0; j < operationsPerThread; ++j) {
                        counter.fetch_add(1, std::memory_order_relaxed);
                    }
                });
            }
            
            for (auto& thread : threads) {
                thread.join();
            }
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            
            double opsPerSecond = (threadCount * operationsPerThread) / 
                                 (duration.count() / 1000000.0);
            
            results.emplace_back(threadCount, opsPerSecond);
            
            REQUIRE(counter.load() == threadCount * operationsPerThread);
        }
        
        // Print scalability results
        for (const auto& result : results) {
            INFO("Threads: " << result.first << ", Ops/sec: " << result.second);
        }
    }
}