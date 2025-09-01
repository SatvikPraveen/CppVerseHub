/**
 * @file MutexExamples.cpp
 * @brief Implementation of comprehensive mutex and synchronization demonstrations
 * @details File location: src/concurrency/MutexExamples.cpp
 */

#include "MutexExamples.hpp"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <optional>

namespace CppVerseHub::Concurrency {

    // Static member definitions
    thread_local unsigned DeadlockPrevention::HierarchicalMutex::this_thread_hierarchy_value_ = UINT_MAX;
    DeadlockPrevention::HierarchicalMutex DeadlockPrevention::high_level_mutex_(1000);
    DeadlockPrevention::HierarchicalMutex DeadlockPrevention::low_level_mutex_(100);

    // ========== BasicMutexDemo Implementation ==========

    void BasicMutexDemo::demonstrate_race_condition() {
        std::cout << "=== Demonstrating Race Condition ===\n";
        
        counter_ = 0;
        const int iterations = 10000;
        const int num_threads = 4;
        
        std::vector<std::thread> threads;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back(&BasicMutexDemo::unsafe_increment, this, iterations);
        }
        
        for (auto& t : threads) {
            t.join();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        std::cout << "Expected: " << (iterations * num_threads) << std::endl;
        std::cout << "Actual: " << counter_ << std::endl;
        std::cout << "Time: " << duration.count() << " microseconds\n";
        std::cout << "Race condition " << (counter_ == iterations * num_threads ? "NOT " : "") << "detected!\n\n";
    }

    void BasicMutexDemo::demonstrate_mutex_protection() {
        std::cout << "=== Demonstrating Mutex Protection ===\n";
        
        counter_ = 0;
        const int iterations = 10000;
        const int num_threads = 4;
        
        std::vector<std::thread> threads;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back(&BasicMutexDemo::safe_increment, this, iterations);
        }
        
        for (auto& t : threads) {
            t.join();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        std::cout << "Expected: " << (iterations * num_threads) << std::endl;
        std::cout << "Actual: " << counter_ << std::endl;
        std::cout << "Time: " << duration.count() << " microseconds\n";
        std::cout << "Protection " << (counter_ == iterations * num_threads ? "SUCCESS" : "FAILED") << "!\n\n";
    }

    void BasicMutexDemo::demonstrate_lock_guard() {
        std::cout << "=== Demonstrating std::lock_guard ===\n";
        
        std::mutex data_mutex;
        std::vector<int> shared_data;
        const int num_threads = 3;
        
        std::vector<std::thread> threads;
        
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([&shared_data, &data_mutex, i]() {
                for (int j = 0; j < 5; ++j) {
                    {
                        std::lock_guard<std::mutex> lock(data_mutex);
                        shared_data.push_back(i * 10 + j);
                        std::cout << "Thread " << i << " added: " << (i * 10 + j) << std::endl;
                    } // lock automatically released here
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            });
        }
        
        for (auto& t : threads) {
            t.join();
        }
        
        std::cout << "Final data size: " << shared_data.size() << std::endl;
        std::cout << "lock_guard ensures RAII and exception safety\n\n";
    }

    void BasicMutexDemo::demonstrate_unique_lock() {
        std::cout << "=== Demonstrating std::unique_lock ===\n";
        
        std::mutex data_mutex;
        std::condition_variable cv;
        bool ready = false;
        std::string data;
        
        std::thread producer([&]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            {
                std::lock_guard<std::mutex> lock(data_mutex);
                data = "Producer data ready";
                ready = true;
                std::cout << "Producer: Data prepared\n";
            }
            cv.notify_one();
        });
        
        std::thread consumer([&]() {
            std::unique_lock<std::mutex> lock(data_mutex);
            cv.wait(lock, [&]() { return ready; });
            std::cout << "Consumer: Received - " << data << std::endl;
            
            // unique_lock allows manual unlock/lock
            lock.unlock();
            std::cout << "Consumer: Processing data (mutex unlocked)\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            
            lock.lock();
            data += " - processed";
            std::cout << "Consumer: Processing complete - " << data << std::endl;
        });
        
        producer.join();
        consumer.join();
        
        std::cout << "unique_lock provides flexibility for condition variables\n\n";
    }

    void BasicMutexDemo::demonstrate_scoped_lock() {
        std::cout << "=== Demonstrating std::scoped_lock (C++17) ===\n";
        
        std::mutex mutex_a, mutex_b;
        int counter_a = 0, counter_b = 0;
        
        std::vector<std::thread> threads;
        
        // Function that needs both mutexes - prevents deadlock
        auto transfer_work = [&](int thread_id) {
            for (int i = 0; i < 5; ++i) {
                {
                    std::scoped_lock lock(mutex_a, mutex_b); // Locks both atomically
                    counter_a++;
                    counter_b++;
                    std::cout << "Thread " << thread_id << " - Transfer " << i 
                              << " (A: " << counter_a << ", B: " << counter_b << ")\n";
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        };
        
        for (int i = 0; i < 3; ++i) {
            threads.emplace_back(transfer_work, i);
        }
        
        for (auto& t : threads) {
            t.join();
        }
        
        std::cout << "Final: A=" << counter_a << ", B=" << counter_b << std::endl;
        std::cout << "scoped_lock prevents deadlock with multiple mutexes\n\n";
    }

    void BasicMutexDemo::unsafe_increment(int iterations) {
        for (int i = 0; i < iterations; ++i) {
            counter_++;
        }
    }

    void BasicMutexDemo::safe_increment(int iterations) {
        for (int i = 0; i < iterations; ++i) {
            std::lock_guard<std::mutex> lock(counter_mutex_);
            counter_++;
        }
    }

    // ========== AdvancedMutexDemo Implementation ==========

    void AdvancedMutexDemo::demonstrate_recursive_mutex() {
        std::cout << "=== Demonstrating std::recursive_mutex ===\n";
        
        std::thread t1([this]() {
            std::cout << "Thread starting recursive function\n";
            recursive_function(5);
        });
        
        t1.join();
        std::cout << "recursive_mutex allows same thread to lock multiple times\n\n";
    }

    void AdvancedMutexDemo::demonstrate_timed_mutex() {
        std::cout << "=== Demonstrating std::timed_mutex ===\n";
        
        std::thread holder([this]() {
            std::lock_guard<std::timed_mutex> lock(timed_mtx_);
            std::cout << "Holder: Acquired lock, sleeping for 2 seconds\n";
            std::this_thread::sleep_for(std::chrono::seconds(2));
            std::cout << "Holder: Releasing lock\n";
        });
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        std::thread waiter([this]() {
            std::cout << "Waiter: Trying to acquire lock with timeout\n";
            
            if (timed_mtx_.try_lock_for(std::chrono::milliseconds(500))) {
                std::cout << "Waiter: Lock acquired within timeout\n";
                timed_mtx_.unlock();
            } else {
                std::cout << "Waiter: Timeout - couldn't acquire lock\n";
            }
            
            std::this_thread::sleep_for(std::chrono::seconds(2));
            
            if (timed_mtx_.try_lock_for(std::chrono::milliseconds(500))) {
                std::cout << "Waiter: Lock acquired on second attempt\n";
                timed_mtx_.unlock();
            } else {
                std::cout << "Waiter: Second timeout\n";
            }
        });
        
        holder.join();
        waiter.join();
        std::cout << "timed_mutex allows timeout-based lock attempts\n\n";
    }

    void AdvancedMutexDemo::demonstrate_shared_mutex() {
        std::cout << "=== Demonstrating std::shared_mutex ===\n";
        
        std::vector<std::thread> readers;
        std::vector<std::thread> writers;
        
        // Start multiple readers
        for (int i = 0; i < 3; ++i) {
            readers.emplace_back(&AdvancedMutexDemo::reader_function, this, i);
        }
        
        // Start a writer
        writers.emplace_back(&AdvancedMutexDemo::writer_function, this, 1);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        // Start another writer
        writers.emplace_back(&AdvancedMutexDemo::writer_function, this, 2);
        
        for (auto& r : readers) {
            r.join();
        }
        for (auto& w : writers) {
            w.join();
        }
        
        std::cout << "shared_mutex allows multiple readers or single writer\n\n";
    }

    void AdvancedMutexDemo::demonstrate_try_lock_strategies() {
        std::cout << "=== Demonstrating try_lock Strategies ===\n";
        
        std::mutex resource_mutex;
        std::atomic<int> successful_acquisitions{0};
        std::atomic<int> failed_attempts{0};
        
        std::vector<std::thread> threads;
        
        for (int i = 0; i < 5; ++i) {
            threads.emplace_back([&, i]() {
                for (int attempt = 0; attempt < 3; ++attempt) {
                    if (resource_mutex.try_lock()) {
                        successful_acquisitions++;
                        std::cout << "Thread " << i << " - Attempt " << attempt << " SUCCESS\n";
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        resource_mutex.unlock();
                    } else {
                        failed_attempts++;
                        std::cout << "Thread " << i << " - Attempt " << attempt << " FAILED\n";
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    }
                }
            });
        }
        
        for (auto& t : threads) {
            t.join();
        }
        
        std::cout << "Successful acquisitions: " << successful_acquisitions << std::endl;
        std::cout << "Failed attempts: " << failed_attempts << std::endl;
        std::cout << "try_lock allows non-blocking mutex attempts\n\n";
    }

    void AdvancedMutexDemo::recursive_function(int depth) {
        std::lock_guard<std::recursive_mutex> lock(recursive_mtx_);
        recursive_depth_++;
        
        std::cout << "Recursive depth: " << recursive_depth_ << std::endl;
        
        if (depth > 0) {
            recursive_function(depth - 1);
        }
        
        recursive_depth_--;
    }

    void AdvancedMutexDemo::reader_function(int reader_id) {
        for (int i = 0; i < 3; ++i) {
            std::shared_lock<std::shared_mutex> lock(shared_mtx_);
            std::cout << "Reader " << reader_id << " reading: " << shared_data_ << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    void AdvancedMutexDemo::writer_function(int writer_id) {
        for (int i = 0; i < 2; ++i) {
            std::unique_lock<std::shared_mutex> lock(shared_mtx_);
            shared_data_ = "Data modified by writer " + std::to_string(writer_id) + " iteration " + std::to_string(i);
            std::cout << "Writer " << writer_id << " wrote: " << shared_data_ << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
        }
    }

    // ========== DeadlockPrevention Implementation ==========

    void DeadlockPrevention::demonstrate_deadlock_scenario() {
        std::cout << "=== Demonstrating Deadlock Scenario ===\n";
        std::cout << "WARNING: This may deadlock - timeout implemented\n";
        
        std::atomic<bool> deadlock_detected{false};
        
        std::thread t1([this, &deadlock_detected]() {
            try {
                std::lock_guard<std::mutex> lock_a(mutex_a_);
                std::cout << "Thread 1: Acquired mutex A\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                
                // Try to acquire mutex B (this may cause deadlock)
                if (!deadlock_detected.load()) {
                    std::lock_guard<std::mutex> lock_b(mutex_b_);
                    std::cout << "Thread 1: Acquired mutex B\n";
                }
            } catch (const std::exception& e) {
                std::cout << "Thread 1 exception: " << e.what() << std::endl;
            }
        });
        
        std::thread t2([this, &deadlock_detected]() {
            try {
                std::lock_guard<std::mutex> lock_b(mutex_b_);
                std::cout << "Thread 2: Acquired mutex B\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                
                // Try to acquire mutex A (this may cause deadlock)
                if (!deadlock_detected.load()) {
                    std::lock_guard<std::mutex> lock_a(mutex_a_);
                    std::cout << "Thread 2: Acquired mutex A\n";
                }
            } catch (const std::exception& e) {
                std::cout << "Thread 2 exception: " << e.what() << std::endl;
            }
        });
        
        // Timeout mechanism to detect deadlock
        std::thread timeout_thread([&deadlock_detected, &t1, &t2]() {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            deadlock_detected = true;
            // Note: In a real scenario, you'd need more sophisticated deadlock detection
        });
        
        t1.join();
        t2.join();
        timeout_thread.join();
        
        if (deadlock_detected) {
            std::cout << "Potential deadlock scenario demonstrated\n";
        }
        std::cout << "\n";
    }

    void DeadlockPrevention::demonstrate_ordered_locking() {
        std::cout << "=== Demonstrating Ordered Locking ===\n";
        
        // Always acquire mutexes in the same order
        auto ordered_work = [this](int thread_id, bool reverse_work = false) {
            // Always lock in order: mutex_a then mutex_b
            std::lock_guard<std::mutex> lock_a(mutex_a_);
            std::cout << "Thread " << thread_id << ": Acquired mutex A\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            
            std::lock_guard<std::mutex> lock_b(mutex_b_);
            std::cout << "Thread " << thread_id << ": Acquired mutex B\n";
            
            if (reverse_work) {
                std::cout << "Thread " << thread_id << ": Doing reverse work\n";
            } else {
                std::cout << "Thread " << thread_id << ": Doing normal work\n";
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            std::cout << "Thread " << thread_id << ": Work complete\n";
        };
        
        std::thread t1(ordered_work, 1, false);
        std::thread t2(ordered_work, 2, true);
        
        t1.join();
        t2.join();
        
        std::cout << "Ordered locking prevents deadlock\n\n";
    }

    void DeadlockPrevention::demonstrate_timeout_prevention() {
        std::cout << "=== Demonstrating Timeout Prevention ===\n";
        
        std::thread t1([this]() {
            std::unique_lock<std::timed_mutex> lock_a(timed_a_);
            std::cout << "Thread 1: Acquired timed mutex A\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            if (timed_b_.try_lock_for(std::chrono::milliseconds(200))) {
                std::cout << "Thread 1: Acquired timed mutex B\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                timed_b_.unlock();
            } else {
                std::cout << "Thread 1: Timeout on mutex B - avoiding deadlock\n";
            }
        });
        
        std::thread t2([this]() {
            std::unique_lock<std::timed_mutex> lock_b(timed_b_);
            std::cout << "Thread 2: Acquired timed mutex B\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            if (timed_a_.try_lock_for(std::chrono::milliseconds(200))) {
                std::cout << "Thread 2: Acquired timed mutex A\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                timed_a_.unlock();
            } else {
                std::cout << "Thread 2: Timeout on mutex A - avoiding deadlock\n";
            }
        });
        
        t1.join();
        t2.join();
        
        std::cout << "Timeout-based locking prevents indefinite deadlock\n\n";
    }

    void DeadlockPrevention::demonstrate_scoped_lock_prevention() {
        std::cout << "=== Demonstrating std::scoped_lock Prevention ===\n";
        
        std::thread t1([this]() {
            std::scoped_lock lock(mutex_a_, mutex_b_);
            std::cout << "Thread 1: Acquired both mutexes atomically\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        });
        
        std::thread t2([this]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            std::scoped_lock lock(mutex_b_, mutex_a_);  // Different order, but safe
            std::cout << "Thread 2: Acquired both mutexes atomically\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        });
        
        t1.join();
        t2.join();
        
        std::cout << "scoped_lock acquires multiple mutexes atomically\n\n";
    }

    void DeadlockPrevention::demonstrate_lock_hierarchy() {
        std::cout << "=== Demonstrating Lock Hierarchy ===\n";
        
        try {
            std::thread t1([]() {
                try {
                    std::lock_guard<HierarchicalMutex> lock_high(high_level_mutex_);
                    std::cout << "Thread 1: Acquired high-level mutex\n";
                    
                    std::lock_guard<HierarchicalMutex> lock_low(low_level_mutex_);
                    std::cout << "Thread 1: Acquired low-level mutex (correct order)\n";
                } catch (const std::exception& e) {
                    std::cout << "Thread 1 error: " << e.what() << std::endl;
                }
            });
            
            std::thread t2([]() {
                try {
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    std::lock_guard<HierarchicalMutex> lock_low(low_level_mutex_);
                    std::cout << "Thread 2: Acquired low-level mutex\n";
                    
                    // This should throw an exception
                    std::lock_guard<HierarchicalMutex> lock_high(high_level_mutex_);
                    std::cout << "Thread 2: This should not print\n";
                } catch (const std::exception& e) {
                    std::cout << "Thread 2 error (expected): " << e.what() << std::endl;
                }
            });
            
            t1.join();
            t2.join();
        } catch (const std::exception& e) {
            std::cout << "Hierarchy demonstration error: " << e.what() << std::endl;
        }
        
        std::cout << "Hierarchical mutexes enforce lock ordering\n\n";
    }

    // ========== ResourceManager Implementation ==========

    ResourceManager::ResourceManager() {
        initialize_resources();
    }

    ResourceManager::~ResourceManager() {
        std::unique_lock<std::shared_mutex> lock(resources_mutex_);
        resources_.clear();
    }

    void ResourceManager::initialize_resources() {
        resources_.reserve(5);
        resources_.emplace_back(std::make_shared<Resource>(1, "Database Connection"));
        resources_.emplace_back(std::make_shared<Resource>(2, "File Handle"));
        resources_.emplace_back(std::make_shared<Resource>(3, "Network Socket"));
        resources_.emplace_back(std::make_shared<Resource>(4, "Memory Pool"));
        resources_.emplace_back(std::make_shared<Resource>(5, "GPU Context"));
    }

    std::shared_ptr<ResourceManager::Resource> ResourceManager::acquire_resource(const std::string& requester) {
        std::unique_lock<std::shared_mutex> resources_lock(resources_mutex_);
        
        for (auto& resource : resources_) {
            if (!resource->in_use) {
                resource->in_use = true;
                resource->last_accessed = std::chrono::steady_clock::now();
                
                {
                    std::lock_guard<std::mutex> owners_lock(owners_mutex_);
                    resource_owners_[resource->id] = requester;
                }
                
                std::cout << "Resource '" << resource->name << "' acquired by " << requester << std::endl;
                return resource;
            }
        }
        
        std::cout << "No resources available for " << requester << std::endl;
        return nullptr;
    }

    void ResourceManager::release_resource(std::shared_ptr<Resource> resource, const std::string& releaser) {
        if (!resource) {
            std::cout << "Invalid resource release attempt by " << releaser << std::endl;
            return;
        }
        
        std::shared_lock<std::shared_mutex> resources_lock(resources_mutex_);
        
        {
            std::lock_guard<std::mutex> owners_lock(owners_mutex_);
            auto owner_it = resource_owners_.find(resource->id);
            if (owner_it == resource_owners_.end() || owner_it->second != releaser) {
                std::cout << "Unauthorized release attempt by " << releaser 
                          << " for resource '" << resource->name << "'\n";
                return;
            }
            resource_owners_.erase(owner_it);
        }
        
        resource->in_use = false;
        resource->last_accessed = std::chrono::steady_clock::now();
        
        std::cout << "Resource '" << resource->name << "' released by " << releaser << std::endl;
    }

    void ResourceManager::print_resource_status() const {
        std::shared_lock<std::shared_mutex> resources_lock(resources_mutex_);
        std::lock_guard<std::mutex> owners_lock(owners_mutex_);
        
        std::cout << "\n=== Resource Status ===\n";
        for (const auto& resource : resources_) {
            std::cout << "ID: " << resource->id << " | Name: " << resource->name 
                      << " | Status: " << (resource->in_use ? "IN USE" : "AVAILABLE");
            
            if (resource->in_use) {
                auto owner_it = resource_owners_.find(resource->id);
                if (owner_it != resource_owners_.end()) {
                    std::cout << " | Owner: " << owner_it->second;
                }
            }
            
            auto time_since_access = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - resource->last_accessed
            );
            std::cout << " | Last Access: " << time_since_access.count() << "s ago\n";
        }
        std::cout << "=======================\n\n";
    }

    size_t ResourceManager::available_resource_count() const {
        std::shared_lock<std::shared_mutex> lock(resources_mutex_);
        return std::count_if(resources_.begin(), resources_.end(),
                           [](const std::shared_ptr<Resource>& r) { return !r->in_use; });
    }

    std::vector<std::string> ResourceManager::get_resource_names() const {
        std::shared_lock<std::shared_mutex> lock(resources_mutex_);
        std::vector<std::string> names;
        names.reserve(resources_.size());
        
        for (const auto& resource : resources_) {
            names.push_back(resource->name);
        }
        
        return names;
    }

    // ========== MutexPerformanceTest Implementation ==========

    std::vector<MutexPerformanceTest::PerformanceResult> 
    MutexPerformanceTest::run_performance_comparison(size_t operations) {
        std::vector<PerformanceResult> results;
        
        results.push_back(test_basic_mutex(operations));
        results.push_back(test_recursive_mutex(operations));
        results.push_back(test_shared_mutex_read(operations));
        results.push_back(test_shared_mutex_write(operations));
        results.push_back(test_atomic_operations(operations));
        
        return results;
    }

    void MutexPerformanceTest::print_performance_results(const std::vector<PerformanceResult>& results) {
        std::cout << "\n=== Mutex Performance Comparison ===\n";
        std::cout << std::left << std::setw(20) << "Mutex Type" 
                  << std::setw(15) << "Total Time(μs)" 
                  << std::setw(15) << "Operations" 
                  << std::setw(15) << "Ops/Second\n";
        std::cout << std::string(65, '-') << std::endl;
        
        for (const auto& result : results) {
            std::cout << std::left << std::setw(20) << result.mutex_type
                      << std::setw(15) << result.total_time.count()
                      << std::setw(15) << result.operations_count
                      << std::setw(15) << std::fixed << std::setprecision(0) << result.ops_per_second << std::endl;
        }
        std::cout << std::string(65, '-') << std::endl << std::endl;
    }

    MutexPerformanceTest::PerformanceResult 
    MutexPerformanceTest::test_basic_mutex(size_t operations) {
        std::mutex mtx;
        int counter = 0;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<std::thread> threads;
        const int num_threads = 4;
        const size_t ops_per_thread = operations / num_threads;
        
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([&mtx, &counter, ops_per_thread]() {
                for (size_t j = 0; j < ops_per_thread; ++j) {
                    std::lock_guard<std::mutex> lock(mtx);
                    counter++;
                }
            });
        }
        
        for (auto& t : threads) {
            t.join();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        return {"Basic Mutex", duration, operations, 
                static_cast<double>(operations) / duration.count() * 1000000};
    }

    MutexPerformanceTest::PerformanceResult 
    MutexPerformanceTest::test_recursive_mutex(size_t operations) {
        std::recursive_mutex mtx;
        int counter = 0;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<std::thread> threads;
        const int num_threads = 4;
        const size_t ops_per_thread = operations / num_threads;
        
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([&mtx, &counter, ops_per_thread]() {
                for (size_t j = 0; j < ops_per_thread; ++j) {
                    std::lock_guard<std::recursive_mutex> lock(mtx);
                    counter++;
                }
            });
        }
        
        for (auto& t : threads) {
            t.join();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        return {"Recursive Mutex", duration, operations, 
                static_cast<double>(operations) / duration.count() * 1000000};
    }

    MutexPerformanceTest::PerformanceResult 
    MutexPerformanceTest::test_shared_mutex_read(size_t operations) {
        std::shared_mutex mtx;
        int data = 42;
        int sum = 0;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<std::thread> threads;
        const int num_threads = 4;
        const size_t ops_per_thread = operations / num_threads;
        
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([&mtx, &data, &sum, ops_per_thread]() {
                int local_sum = 0;
                for (size_t j = 0; j < ops_per_thread; ++j) {
                    std::shared_lock<std::shared_mutex> lock(mtx);
                    local_sum += data;
                }
                sum += local_sum; // This is not thread-safe, but for performance testing only
            });
        }
        
        for (auto& t : threads) {
            t.join();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        return {"Shared Read", duration, operations, 
                static_cast<double>(operations) / duration.count() * 1000000};
    }

    MutexPerformanceTest::PerformanceResult 
    MutexPerformanceTest::test_shared_mutex_write(size_t operations) {
        std::shared_mutex mtx;
        int counter = 0;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<std::thread> threads;
        const int num_threads = 4;
        const size_t ops_per_thread = operations / num_threads;
        
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([&mtx, &counter, ops_per_thread]() {
                for (size_t j = 0; j < ops_per_thread; ++j) {
                    std::unique_lock<std::shared_mutex> lock(mtx);
                    counter++;
                }
            });
        }
        
        for (auto& t : threads) {
            t.join();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        return {"Shared Write", duration, operations, 
                static_cast<double>(operations) / duration.count() * 1000000};
    }

    MutexPerformanceTest::PerformanceResult 
    MutexPerformanceTest::test_atomic_operations(size_t operations) {
        std::atomic<int> counter{0};
        
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<std::thread> threads;
        const int num_threads = 4;
        const size_t ops_per_thread = operations / num_threads;
        
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([&counter, ops_per_thread]() {
                for (size_t j = 0; j < ops_per_thread; ++j) {
                    counter++;
                }
            });
        }
        
        for (auto& t : threads) {
            t.join();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        return {"Atomic", duration, operations, 
                static_cast<double>(operations) / duration.count() * 1000000};
    }

    // ========== SpaceStationSimulation Implementation ==========

    SpaceStationSimulation::SpaceStationSimulation() {
        // Initialize all systems
        systems_[SystemType::LIFE_SUPPORT] = {SystemType::LIFE_SUPPORT, true, 0.95, 
                                              std::chrono::steady_clock::now(), ""};
        systems_[SystemType::POWER_MANAGEMENT] = {SystemType::POWER_MANAGEMENT, true, 0.92, 
                                                  std::chrono::steady_clock::now(), ""};
        systems_[SystemType::COMMUNICATIONS] = {SystemType::COMMUNICATIONS, true, 0.88, 
                                               std::chrono::steady_clock::now(), ""};
        systems_[SystemType::NAVIGATION] = {SystemType::NAVIGATION, true, 0.90, 
                                           std::chrono::steady_clock::now(), ""};
        systems_[SystemType::SCIENCE_LAB] = {SystemType::SCIENCE_LAB, true, 0.85, 
                                            std::chrono::steady_clock::now(), ""};
    }

    void SpaceStationSimulation::start_simulation(std::chrono::seconds duration) {
        safe_print("🚀 Starting Space Station Simulation for " + std::to_string(duration.count()) + " seconds");
        
        simulation_running_ = true;
        
        // Start operator threads
        std::vector<std::string> operators = {"Alice", "Bob", "Charlie", "Diana"};
        std::vector<SystemType> preferred_systems = {
            SystemType::LIFE_SUPPORT, SystemType::POWER_MANAGEMENT,
            SystemType::COMMUNICATIONS, SystemType::NAVIGATION
        };
        
        for (size_t i = 0; i < operators.size(); ++i) {
            operator_threads_.emplace_back(&SpaceStationSimulation::operator_routine, 
                                         this, operators[i], preferred_systems[i]);
        }
        
        // Start maintenance and emergency threads
        std::thread maintenance_thread(&SpaceStationSimulation::maintenance_routine, this);
        std::thread emergency_thread(&SpaceStationSimulation::emergency_response_routine, this);
        
        // Let simulation run
        std::this_thread::sleep_for(duration);
        
        // Stop simulation
        simulation_running_ = false;
        
        // Join all threads
        for (auto& t : operator_threads_) {
            if (t.joinable()) {
                t.join();
            }
        }
        
        if (maintenance_thread.joinable()) {
            maintenance_thread.join();
        }
        
        if (emergency_thread.joinable()) {
            emergency_thread.join();
        }
        
        operator_threads_.clear();
        safe_print("🛑 Space Station Simulation Complete");
    }

    void SpaceStationSimulation::stop_simulation() {
        simulation_running_ = false;
    }

    void SpaceStationSimulation::print_station_status() const {
        std::shared_lock<std::shared_mutex> lock(systems_mutex_);
        
        safe_print("\n📊 === SPACE STATION STATUS ===");
        for (const auto& [type, status] : systems_) {
            std::ostringstream oss;
            oss << system_type_to_string(type) << " | "
                << (status.operational ? "✅ ONLINE" : "❌ OFFLINE") << " | "
                << "Efficiency: " << std::fixed << std::setprecision(1) << (status.efficiency * 100) << "% | "
                << "Operator: " << (status.current_operator.empty() ? "None" : status.current_operator);
            safe_print(oss.str());
        }
        safe_print("===============================\n");
    }

    void SpaceStationSimulation::operator_routine(const std::string& operator_name, SystemType preferred_system) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> work_time(500, 2000);
        std::uniform_int_distribution<> efficiency_change(1, 5);
        
        while (simulation_running_) {
            {
                std::unique_lock<std::shared_mutex> lock(systems_mutex_);
                
                // Try to work on preferred system first, then any available system
                std::vector<SystemType> systems_to_try = {preferred_system};
                for (const auto& [type, _] : systems_) {
                    if (type != preferred_system) {
                        systems_to_try.push_back(type);
                    }
                }
                
                for (auto system_type : systems_to_try) {
                    auto& system = systems_[system_type];
                    if (system.current_operator.empty() && system.operational) {
                        system.current_operator = operator_name;
                        
                        safe_print("👨‍🚀 " + operator_name + " started working on " + 
                                  system_type_to_string(system_type));
                        
                        lock.unlock();
                        
                        // Simulate work
                        std::this_thread::sleep_for(std::chrono::milliseconds(work_time(gen)));
                        
                        lock.lock();
                        
                        // Update efficiency
                        double change = efficiency_change(gen) / 100.0;
                        system.efficiency = std::min(1.0, system.efficiency + change);
                        system.last_maintenance = std::chrono::steady_clock::now();
                        system.current_operator.clear();
                        
                        safe_print("✅ " + operator_name + " completed work on " + 
                                  system_type_to_string(system_type) + 
                                  " (Efficiency: " + std::to_string(int(system.efficiency * 100)) + "%)");
                        break;
                    }
                }
            }
            
            // Rest between work sessions
            std::this_thread::sleep_for(std::chrono::milliseconds(1000 + work_time(gen) / 2));
        }
    }

    void SpaceStationSimulation::maintenance_routine() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> maintenance_interval(3000, 8000);
        std::uniform_int_distribution<> efficiency_degradation(2, 8);
        
        while (simulation_running_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(maintenance_interval(gen)));
            
            std::unique_lock<std::shared_mutex> lock(systems_mutex_);
            
            // Random efficiency degradation
            for (auto& [type, system] : systems_) {
                if (system.current_operator.empty()) { // Only degrade if not being worked on
                    double degradation = efficiency_degradation(gen) / 100.0;
                    system.efficiency = std::max(0.1, system.efficiency - degradation);
                    
                    if (system.efficiency < 0.5) {
                        safe_print("⚠️  MAINTENANCE WARNING: " + system_type_to_string(type) + 
                                  " efficiency dropped to " + std::to_string(int(system.efficiency * 100)) + "%");
                    }
                }
            }
        }
    }

    void SpaceStationSimulation::emergency_response_routine() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> emergency_interval(5000, 15000);
        std::uniform_int_distribution<> system_selector(0, 4);
        
        while (simulation_running_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(emergency_interval(gen)));
            
            std::unique_lock<std::shared_mutex> lock(systems_mutex_);
            
            // Random emergency
            std::vector<SystemType> system_types = {
                SystemType::LIFE_SUPPORT, SystemType::POWER_MANAGEMENT,
                SystemType::COMMUNICATIONS, SystemType::NAVIGATION, SystemType::SCIENCE_LAB
            };
            
            SystemType emergency_system = system_types[system_selector(gen)];
            auto& system = systems_[emergency_system];
            
            if (system.operational && system.efficiency > 0.3) {
                system.efficiency *= 0.7; // Emergency reduces efficiency
                safe_print("🚨 EMERGENCY: " + system_type_to_string(emergency_system) + 
                          " experienced malfunction! Efficiency: " + 
                          std::to_string(int(system.efficiency * 100)) + "%");
                
                if (system.efficiency < 0.2) {
                    system.operational = false;
                    safe_print("💥 CRITICAL: " + system_type_to_string(emergency_system) + " OFFLINE!");
                }
            }
        }
    }

    std::string SpaceStationSimulation::system_type_to_string(SystemType type) const {
        switch (type) {
            case SystemType::LIFE_SUPPORT: return "Life Support";
            case SystemType::POWER_MANAGEMENT: return "Power Management";
            case SystemType::COMMUNICATIONS: return "Communications";
            case SystemType::NAVIGATION: return "Navigation";
            case SystemType::SCIENCE_LAB: return "Science Lab";
            default: return "Unknown System";
        }
    }

    void SpaceStationSimulation::safe_print(const std::string& message) const {
        std::lock_guard<std::mutex> lock(console_mutex_);
        std::cout << "[" << std::chrono::duration_cast<std::chrono::seconds>(
                         std::chrono::steady_clock::now().time_since_epoch()).count() % 1000 
                  << "s] " << message << std::endl;
    }

    // ========== ProducerConsumerDemo Implementation ==========

    ProducerConsumerDemo::ProducerConsumerDemo(size_t buffer_size) {
        // Constructor - buffer size is handled by ThreadSafeQueue internally
    }

    void ProducerConsumerDemo::start_demo(size_t num_producers, size_t num_consumers, std::chrono::seconds duration) {
        std::cout << "🏭 Starting Producer-Consumer Demo\n";
        std::cout << "Producers: " << num_producers << ", Consumers: " << num_consumers 
                  << ", Duration: " << duration.count() << "s\n\n";
        
        demo_running_ = true;
        task_counter_ = 0;
        tasks_produced_ = 0;
        tasks_consumed_ = 0;
        
        // Start producer threads
        for (size_t i = 0; i < num_producers; ++i) {
            producer_threads_.emplace_back(&ProducerConsumerDemo::producer_routine, this, static_cast<int>(i));
        }
        
        // Start consumer threads
        for (size_t i = 0; i < num_consumers; ++i) {
            consumer_threads_.emplace_back(&ProducerConsumerDemo::consumer_routine, this, static_cast<int>(i));
        }
        
        // Let demo run
        std::this_thread::sleep_for(duration);
        
        // Stop demo
        stop_demo();
    }

    void ProducerConsumerDemo::stop_demo() {
        demo_running_ = false;
        
        // Join all threads
        for (auto& t : producer_threads_) {
            if (t.joinable()) {
                t.join();
            }
        }
        
        for (auto& t : consumer_threads_) {
            if (t.joinable()) {
                t.join();
            }
        }
        
        producer_threads_.clear();
        consumer_threads_.clear();
        
        std::cout << "\n🛑 Producer-Consumer Demo Stopped\n";
    }

    void ProducerConsumerDemo::print_statistics() const {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        std::cout << "\n📊 === PRODUCER-CONSUMER STATISTICS ===\n";
        std::cout << "Tasks Produced: " << tasks_produced_.load() << std::endl;
        std::cout << "Tasks Consumed: " << tasks_consumed_.load() << std::endl;
        std::cout << "Queue Size: " << task_queue_.size() << std::endl;
        std::cout << "======================================\n\n";
    }

    void ProducerConsumerDemo::producer_routine(int producer_id) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> delay(100, 500);
        
        while (demo_running_) {
            int task_id = ++task_counter_;
            Task task{
                task_id,
                "Task from Producer " + std::to_string(producer_id),
                std::chrono::steady_clock::now()
            };
            
            task_queue_.push(std::move(task));
            tasks_produced_++;
            
            std::cout << "📦 Producer " << producer_id << " created Task " << task_id << std::endl;
            
            std::this_thread::sleep_for(std::chrono::milliseconds(delay(gen)));
        }
    }

    void ProducerConsumerDemo::consumer_routine(int consumer_id) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> processing_time(200, 800);
        
        while (demo_running_ || !task_queue_.empty()) {
            Task task;
            if (task_queue_.try_pop(task)) {
                tasks_consumed_++;
                
                auto processing_duration = std::chrono::milliseconds(processing_time(gen));
                std::cout << "⚙️  Consumer " << consumer_id << " processing Task " << task.id 
                          << " (processing for " << processing_duration.count() << "ms)" << std::endl;
                
                std::this_thread::sleep_for(processing_duration);
                
                std::cout << "✅ Consumer " << consumer_id << " completed Task " << task.id << std::endl;
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        }
    }

    // ========== ReadersWritersDemo Implementation ==========

    ReadersWritersDemo::ReadersWritersDemo() : shared_document_("Initial document content") {}

    void ReadersWritersDemo::start_demo(size_t num_readers, size_t num_writers, std::chrono::seconds duration) {
        std::cout << "📚 Starting Readers-Writers Demo\n";
        std::cout << "Readers: " << num_readers << ", Writers: " << num_writers 
                  << ", Duration: " << duration.count() << "s\n\n";
        
        demo_running_ = true;
        active_readers_ = 0;
        total_reads_ = 0;
        total_writes_ = 0;
        
        // Start reader threads
        for (size_t i = 0; i < num_readers; ++i) {
            reader_threads_.emplace_back(&ReadersWritersDemo::reader_routine, this, static_cast<int>(i));
        }
        
        // Start writer threads
        for (size_t i = 0; i < num_writers; ++i) {
            writer_threads_.emplace_back(&ReadersWritersDemo::writer_routine, this, static_cast<int>(i));
        }
        
        // Let demo run
        std::this_thread::sleep_for(duration);
        
        // Stop demo
        stop_demo();
    }

    void ReadersWritersDemo::stop_demo() {
        demo_running_ = false;
        
        // Join all threads
        for (auto& t : reader_threads_) {
            if (t.joinable()) {
                t.join();
            }
        }
        
        for (auto& t : writer_threads_) {
            if (t.joinable()) {
                t.join();
            }
        }
        
        reader_threads_.clear();
        writer_threads_.clear();
        
        safe_console_print("\n🛑 Readers-Writers Demo Stopped");
    }

    void ReadersWritersDemo::print_statistics() const {
        std::cout << "\n📊 === READERS-WRITERS STATISTICS ===\n";
        std::cout << "Total Reads: " << total_reads_.load() << std::endl;
        std::cout << "Total Writes: " << total_writes_.load() << std::endl;
        std::cout << "Active Readers: " << active_readers_.load() << std::endl;
        std::cout << "Final Document Length: " << shared_document_.length() << " characters\n";
        std::cout << "====================================\n\n";
    }

    void ReadersWritersDemo::reader_routine(int reader_id) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> delay(300, 1000);
        
        while (demo_running_) {
            {
                std::shared_lock<std::shared_mutex> lock(document_mutex_);
                active_readers_++;
                
                safe_console_print("📖 Reader " + std::to_string(reader_id) + 
                                  " reading document (length: " + std::to_string(shared_document_.length()) + 
                                  " chars, active readers: " + std::to_string(active_readers_.load()) + ")");
                
                // Simulate reading time
                std::this_thread::sleep_for(std::chrono::milliseconds(delay(gen)));
                
                total_reads_++;
                active_readers_--;
            }
            
            // Break between reads
            std::this_thread::sleep_for(std::chrono::milliseconds(delay(gen)));
        }
    }

    void ReadersWritersDemo::writer_routine(int writer_id) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> delay(1000, 3000);
        
        while (demo_running_) {
            {
                std::unique_lock<std::shared_mutex> lock(document_mutex_);
                
                std::string addition = " [Edit by Writer " + std::to_string(writer_id) + 
                                      " at time " + std::to_string(total_writes_.load()) + "]";
                shared_document_ += addition;
                
                safe_console_print("✏️  Writer " + std::to_string(writer_id) + 
                                  " modified document (new length: " + std::to_string(shared_document_.length()) + 
                                  " chars)");
                
                total_writes_++;
                
                // Simulate writing time
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
            
            // Break between writes
            std::this_thread::sleep_for(std::chrono::milliseconds(delay(gen)));
        }
    }

    void ReadersWritersDemo::safe_console_print(const std::string& message) const {
        std::lock_guard<std::mutex> lock(console_mutex_);
        std::cout << message << std::endl;
    }

    // ========== DiningPhilosophersDemo Implementation ==========

    DiningPhilosophersDemo::DiningPhilosophersDemo(size_t num_philosophers) 
        : num_philosophers_(num_philosophers), forks_(num_philosophers), philosophers_(num_philosophers) {
        
        for (size_t i = 0; i < num_philosophers_; ++i) {
            philosophers_[i].id = static_cast<int>(i);
        }
    }

    void DiningPhilosophersDemo::start_demo(std::chrono::seconds duration) {
        std::cout << "🍝 Starting Dining Philosophers Demo\n";
        std::cout << "Philosophers: " << num_philosophers_ << ", Duration: " << duration.count() << "s\n\n";
        
        demo_running_ = true;
        
        // Start philosopher threads
        for (size_t i = 0; i < num_philosophers_; ++i) {
            philosopher_threads_.emplace_back(&DiningPhilosophersDemo::philosopher_routine, this, static_cast<int>(i));
        }
        
        // Let demo run
        std::this_thread::sleep_for(duration);
        
        // Stop demo
        stop_demo();
    }

    void DiningPhilosophersDemo::stop_demo() {
        demo_running_ = false;
        
        // Join all threads
        for (auto& t : philosopher_threads_) {
            if (t.joinable()) {
                t.join();
            }
        }
        
        philosopher_threads_.clear();
        
        safe_console_print("\n🛑 Dining Philosophers Demo Stopped");
    }

    void DiningPhilosophersDemo::print_statistics() const {
        std::cout << "\n📊 === DINING PHILOSOPHERS STATISTICS ===\n";
        for (size_t i = 0; i < num_philosophers_; ++i) {
            const auto& phil = philosophers_[i];
            std::cout << "Philosopher " << i << ": "
                      << "Meals: " << phil.meals_eaten.load()
                      << ", Thinking: " << phil.thinking_time_ms.load() << "ms"
                      << ", Eating: " << phil.eating_time_ms.load() << "ms"
                      << ", Waiting: " << phil.waiting_time_ms.load() << "ms\n";
        }
        std::cout << "========================================\n\n";
    }

    void DiningPhilosophersDemo::philosopher_routine(int philosopher_id) {
        auto& philosopher = philosophers_[philosopher_id];
        
        while (demo_running_) {
            // Think
            auto thinking_time = get_random_time(500, 2000);
            safe_console_print("🤔 Philosopher " + std::to_string(philosopher_id) + " is thinking for " + 
                              std::to_string(thinking_time.count()) + "ms");
            std::this_thread::sleep_for(thinking_time);
            philosopher.thinking_time_ms += static_cast<int>(thinking_time.count());
            
            // Try to eat - avoid deadlock by ordering fork acquisition
            int left_fork = philosopher_id;
            int right_fork = (philosopher_id + 1) % num_philosophers_;
            
            // Always acquire lower numbered fork first to prevent deadlock
            int first_fork = std::min(left_fork, right_fork);
            int second_fork = std::max(left_fork, right_fork);
            
            auto wait_start = std::chrono::steady_clock::now();
            
            safe_console_print("🍴 Philosopher " + std::to_string(philosopher_id) + " is hungry, trying to get forks");
            
            {
                std::lock_guard<std::mutex> lock1(forks_[first_fork]);
                std::lock_guard<std::mutex> lock2(forks_[second_fork]);
                
                auto wait_end = std::chrono::steady_clock::now();
                auto wait_time = std::chrono::duration_cast<std::chrono::milliseconds>(wait_end - wait_start);
                philosopher.waiting_time_ms += static_cast<int>(wait_time.count());
                
                // Eat
                auto eating_time = get_random_time(800, 1500);
                safe_console_print("🍽️  Philosopher " + std::to_string(philosopher_id) + " is eating for " + 
                                  std::to_string(eating_time.count()) + "ms");
                
                std::this_thread::sleep_for(eating_time);
                philosopher.eating_time_ms += static_cast<int>(eating_time.count());
                philosopher.meals_eaten++;
                
                safe_console_print("✅ Philosopher " + std::to_string(philosopher_id) + 
                                  " finished eating (meal #" + std::to_string(philosopher.meals_eaten.load()) + ")");
            }
        }
    }

    void DiningPhilosophersDemo::safe_console_print(const std::string& message) const {
        std::lock_guard<std::mutex> lock(console_mutex_);
        std::cout << message << std::endl;
    }

    std::chrono::milliseconds DiningPhilosophersDemo::get_random_time(int min_ms, int max_ms) const {
        static thread_local std::random_device rd;
        static thread_local std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(min_ms, max_ms);
        return std::chrono::milliseconds(dis(gen));
    }

    // ========== MutexDemo Implementation ==========

    void MutexDemo::demonstrate_basic_mutex() {
        print_section_header("Basic Mutex Demonstrations");
        
        BasicMutexDemo demo;
        demo.demonstrate_race_condition();
        demo.demonstrate_mutex_protection();
        demo.demonstrate_lock_guard();
        demo.demonstrate_unique_lock();
        demo.demonstrate_scoped_lock();
        
        print_section_footer();
    }

    void MutexDemo::demonstrate_advanced_mutex() {
        print_section_header("Advanced Mutex Demonstrations");
        
        AdvancedMutexDemo demo;
        demo.demonstrate_recursive_mutex();
        demo.demonstrate_timed_mutex();
        demo.demonstrate_shared_mutex();
        demo.demonstrate_try_lock_strategies();
        
        print_section_footer();
    }

    void MutexDemo::demonstrate_deadlock_prevention() {
        print_section_header("Deadlock Prevention Demonstrations");
        
        DeadlockPrevention demo;
        demo.demonstrate_ordered_locking();
        demo.demonstrate_timeout_prevention();
        demo.demonstrate_scoped_lock_prevention();
        demo.demonstrate_lock_hierarchy();
        
        print_section_footer();
    }

    void MutexDemo::demonstrate_thread_safe_containers() {
        print_section_header("Thread-Safe Container Demonstrations");
        
        std::cout << "=== Thread-Safe Queue Demo ===\n";
        ThreadSafeQueue<int> queue;
        
        std::vector<std::thread> producers;
        std::vector<std::thread> consumers;
        
        // Start producers
        for (int i = 0; i < 2; ++i) {
            producers.emplace_back([&queue, i]() {
                for (int j = 0; j < 5; ++j) {
                    int value = i * 10 + j;
                    queue.push(value);
                    std::cout << "Producer " << i << " pushed: " << value << std::endl;
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            });
        }
        
        // Start consumers
        for (int i = 0; i < 2; ++i) {
            consumers.emplace_back([&queue, i]() {
                for (int j = 0; j < 5; ++j) {
                    int value;
                    if (queue.try_pop(value)) {
                        std::cout << "Consumer " << i << " popped: " << value << std::endl;
                    } else {
                        std::cout << "Consumer " << i << " found empty queue" << std::endl;
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                        j--; // Retry
                    }
                }
            });
        }
        
        for (auto& p : producers) p.join();
        for (auto& c : consumers) c.join();
        
        std::cout << "Final queue size: " << queue.size() << std::endl;
        
        std::cout << "\n=== Thread-Safe Map Demo ===\n";
        ThreadSafeMap<std::string, int> safe_map;
        
        std::vector<std::thread> map_threads;
        
        // Writers
        for (int i = 0; i < 3; ++i) {
            map_threads.emplace_back([&safe_map, i]() {
                for (int j = 0; j < 3; ++j) {
                    std::string key = "key" + std::to_string(i) + "_" + std::to_string(j);
                    int value = i * 10 + j;
                    safe_map.insert(key, value);
                    std::cout << "Thread " << i << " inserted: " << key << " = " << value << std::endl;
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                }
            });
        }
        
        // Readers
        for (int i = 0; i < 2; ++i) {
            map_threads.emplace_back([&safe_map, i]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                auto keys = safe_map.keys();
                for (const auto& key : keys) {
                    auto value = safe_map.find(key);
                    if (value) {
                        std::cout << "Reader " << i << " found: " << key << " = " << *value << std::endl;
                    }
                }
            });
        }
        
        for (auto& t : map_threads) t.join();
        
        std::cout << "Final map size: " << safe_map.size() << std::endl;
        
        print_section_footer();
    }

    void MutexDemo::demonstrate_resource_management() {
        print_section_header("Resource Management Demonstration");
        
        ResourceManager manager;
        
        std::vector<std::thread> workers;
        
        for (int i = 0; i < 3; ++i) {
            workers.emplace_back([&manager, i]() {
                std::string worker_name = "Worker" + std::to_string(i);
                
                for (int j = 0; j < 2; ++j) {
                    auto resource = manager.acquire_resource(worker_name);
                    if (resource) {
                        simulate_work(std::chrono::milliseconds(500 + i * 100));
                        manager.release_resource(resource, worker_name);
                    } else {
                        std::cout << worker_name << " couldn't acquire resource\n";
                    }
                    
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                }
            });
        }
        
        // Print status periodically
        std::thread status_thread([&manager]() {
            for (int i = 0; i < 5; ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(400));
                manager.print_resource_status();
            }
        });
        
        for (auto& w : workers) w.join();
        status_thread.join();
        
        print_section_footer();
    }

    void MutexDemo::demonstrate_performance_comparison() {
        print_section_header("Mutex Performance Comparison");
        
        auto results = MutexPerformanceTest::run_performance_comparison(50000);
        MutexPerformanceTest::print_performance_results(results);
        
        print_section_footer();
    }

    void MutexDemo::demonstrate_space_station_simulation() {
        print_section_header("Space Station Simulation");
        
        SpaceStationSimulation simulation;
        
        // Start simulation in a separate thread so we can monitor it
        std::thread sim_thread([&simulation]() {
            simulation.start_simulation(std::chrono::seconds(8));
        });
        
        // Print status updates
        std::thread status_thread([&simulation]() {
            for (int i = 0; i < 4; ++i) {
                std::this_thread::sleep_for(std::chrono::seconds(2));
                simulation.print_station_status();
            }
        });
        
        sim_thread.join();
        status_thread.join();
        
        print_section_footer();
    }

    void MutexDemo::demonstrate_producer_consumer() {
        print_section_header("Producer-Consumer Pattern");
        
        ProducerConsumerDemo demo(10);
        demo.start_demo(2, 3, std::chrono::seconds(5));
        demo.print_statistics();
        
        print_section_footer();
    }

    void MutexDemo::demonstrate_readers_writers() {
        print_section_header("Readers-Writers Problem");
        
        ReadersWritersDemo demo;
        demo.start_demo(4, 2, std::chrono::seconds(6));
        demo.print_statistics();
        
        print_section_footer();
    }

    void MutexDemo::demonstrate_dining_philosophers() {
        print_section_header("Dining Philosophers Problem");
        
        DiningPhilosophersDemo demo(5);
        demo.start_demo(std::chrono::seconds(8));
        demo.print_statistics();
        
        print_section_footer();
    }

    void MutexDemo::run_all_demonstrations() {
        std::cout << "\n🎯 ======================================================\n";
        std::cout << "🎯 COMPREHENSIVE MUTEX AND SYNCHRONIZATION DEMONSTRATIONS\n";
        std::cout << "🎯 ======================================================\n\n";
        
        demonstrate_basic_mutex();
        demonstrate_advanced_mutex();
        demonstrate_deadlock_prevention();
        demonstrate_thread_safe_containers();
        demonstrate_resource_management();
        demonstrate_performance_comparison();
        demonstrate_producer_consumer();
        demonstrate_readers_writers();
        demonstrate_dining_philosophers();
        demonstrate_space_station_simulation();
        
        std::cout << "\n🎉 ======================================\n";
        std::cout << "🎉 ALL DEMONSTRATIONS COMPLETED!\n";
        std::cout << "🎉 ======================================\n\n";
    }

    void MutexDemo::simulate_work(std::chrono::milliseconds duration) {
        std::this_thread::sleep_for(duration);
    }

    void MutexDemo::print_section_header(const std::string& title) {
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "🔒 " << title << std::endl;
        std::cout << std::string(60, '=') << std::endl << std::endl;
    }

    void MutexDemo::print_section_footer() {
        std::cout << std::string(60, '-') << std::endl;
        std::cout << "✅ Section Complete\n" << std::endl;
    }

} // namespace CppVerseHub::Concurrency