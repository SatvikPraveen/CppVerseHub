/**
 * @file Atomics.cpp
 * @brief Implementation of comprehensive atomic operations and lock-free programming demonstrations
 * @details File location: src/concurrency/Atomics.cpp
 */

#include "Atomics.hpp"
#include <iomanip>
#include <mutex>
#include <sstream>

namespace CppVerseHub::Concurrency {

    // Static member definitions
    std::vector<std::atomic<void*>*> HazardPointerManager::hazard_pointers_;

    // ========== BasicAtomicsDemo Implementation ==========

    void BasicAtomicsDemo::demonstrate_basic_atomic_types() {
        std::cout << "=== Basic Atomic Types ===\n";
        
        std::atomic<int> atomic_int{42};
        std::atomic<bool> atomic_bool{true};
        std::atomic<double> atomic_double{3.14159};
        
        std::cout << "Initial values:\n";
        std::cout << "  atomic_int: " << atomic_int.load() << std::endl;
        std::cout << "  atomic_bool: " << std::boolalpha << atomic_bool.load() << std::endl;
        std::cout << "  atomic_double: " << atomic_double.load() << std::endl;
        
        // Test atomic operations
        atomic_int.store(100);
        atomic_bool.store(false);
        atomic_double.store(2.718);
        
        std::cout << "\nAfter store operations:\n";
        std::cout << "  atomic_int: " << atomic_int.load() << std::endl;
        std::cout << "  atomic_bool: " << std::boolalpha << atomic_bool.load() << std::endl;
        std::cout << "  atomic_double: " << atomic_double.load() << std::endl;
        
        // Test exchange
        int old_int = atomic_int.exchange(200);
        std::cout << "\nExchanged atomic_int " << old_int << " with 200, new value: " 
                  << atomic_int.load() << std::endl;
        
        std::cout << "\nAtomic types ensure thread-safe access without explicit locking\n\n";
    }

    void BasicAtomicsDemo::demonstrate_atomic_operations() {
        std::cout << "=== Atomic Operations ===\n";
        
        atomic_counter_.store(0);
        const int iterations = 10000;
        const int num_threads = 4;
        
        std::vector<std::thread> threads;
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back(&BasicAtomicsDemo::atomic_increment_worker, this, iterations);
        }
        
        for (auto& t : threads) {
            t.join();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        std::cout << "Expected: " << (iterations * num_threads) << std::endl;
        std::cout << "Actual: " << atomic_counter_.load() << std::endl;
        std::cout << "Time: " << duration.count() << " microseconds\n";
        std::cout << "Atomic operations are naturally thread-safe\n\n";
    }

    void BasicAtomicsDemo::demonstrate_memory_ordering() {
        std::cout << "=== Memory Ordering ===\n";
        
        ready_.store(false);
        atomic_ptr_.store(nullptr);
        
        std::thread producer([this]() {
            memory_ordering_producer();
        });
        
        std::thread consumer([this]() {
            memory_ordering_consumer();
        });
        
        producer.join();
        consumer.join();
        
        std::cout << "Memory ordering ensures proper synchronization\n\n";
    }

    void BasicAtomicsDemo::demonstrate_compare_and_swap() {
        std::cout << "=== Compare and Swap (CAS) ===\n";
        
        atomic_counter_.store(0);
        std::vector<std::thread> workers;
        
        for (int i = 0; i < 4; ++i) {
            workers.emplace_back(&BasicAtomicsDemo::cas_worker, this, i, 5);
        }
        
        for (auto& w : workers) {
            w.join();
        }
        
        std::cout << "Final counter value: " << atomic_counter_.load() << std::endl;
        std::cout << "CAS enables lock-free algorithms\n\n";
    }

    void BasicAtomicsDemo::demonstrate_atomic_flag() {
        std::cout << "=== Atomic Flag ===\n";
        
        spin_lock_.clear();
        std::vector<std::thread> threads;
        std::string shared_resource = "";
        
        for (int i = 0; i < 5; ++i) {
            threads.emplace_back([this, &shared_resource, i]() {
                for (int j = 0; j < 3; ++j) {
                    while (spin_lock_.test_and_set(std::memory_order_acquire)) {
                        // Spin wait
                    }
                    
                    // Critical section
                    shared_resource += "T" + std::to_string(i) + "(" + std::to_string(j) + ") ";
                    std::cout << "Thread " << i << " modified resource: " << shared_resource << std::endl;
                    
                    spin_lock_.clear(std::memory_order_release);
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            });
        }
        
        for (auto& t : threads) {
            t.join();
        }
        
        std::cout << "Final resource: " << shared_resource << std::endl;
        std::cout << "atomic_flag provides a simple spinlock mechanism\n\n";
    }

    void BasicAtomicsDemo::demonstrate_atomic_vs_mutex_performance() {
        std::cout << "=== Atomic vs Mutex Performance ===\n";
        
        const int iterations = 100000;
        const int num_threads = 4;
        
        // Test atomic performance
        atomic_counter_.store(0);
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<std::thread> atomic_threads;
        for (int i = 0; i < num_threads; ++i) {
            atomic_threads.emplace_back([this, iterations]() {
                for (int j = 0; j < iterations; ++j) {
                    atomic_counter_.fetch_add(1, std::memory_order_relaxed);
                }
            });
        }
        
        for (auto& t : atomic_threads) {
            t.join();
        }
        
        auto atomic_end = std::chrono::high_resolution_clock::now();
        auto atomic_duration = std::chrono::duration_cast<std::chrono::microseconds>(atomic_end - start);
        
        // Test mutex performance
        int mutex_counter = 0;
        std::mutex counter_mutex;
        start = std::chrono::high_resolution_clock::now();
        
        std::vector<std::thread> mutex_threads;
        for (int i = 0; i < num_threads; ++i) {
            mutex_threads.emplace_back([&mutex_counter, &counter_mutex, iterations]() {
                for (int j = 0; j < iterations; ++j) {
                    std::lock_guard<std::mutex> lock(counter_mutex);
                    mutex_counter++;
                }
            });
        }
        
        for (auto& t : mutex_threads) {
            t.join();
        }
        
        auto mutex_end = std::chrono::high_resolution_clock::now();
        auto mutex_duration = std::chrono::duration_cast<std::chrono::microseconds>(mutex_end - start);
        
        std::cout << "Atomic operations: " << atomic_duration.count() << " μs (result: " 
                  << atomic_counter_.load() << ")\n";
        std::cout << "Mutex operations: " << mutex_duration.count() << " μs (result: " 
                  << mutex_counter << ")\n";
        std::cout << "Speedup: " << std::fixed << std::setprecision(2) 
                  << (static_cast<double>(mutex_duration.count()) / atomic_duration.count()) << "x\n\n";
    }

    void BasicAtomicsDemo::atomic_increment_worker(int iterations) {
        for (int i = 0; i < iterations; ++i) {
            atomic_counter_.fetch_add(1, std::memory_order_relaxed);
        }
    }

    void BasicAtomicsDemo::memory_ordering_producer() {
        std::string* data = new std::string("Hello from producer!");
        atomic_ptr_.store(data, std::memory_order_relaxed);
        ready_.store(true, std::memory_order_release);
        std::cout << "Producer: Data published with release semantics\n";
    }

    void BasicAtomicsDemo::memory_ordering_consumer() {
        while (!ready_.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        
        std::string* data = atomic_ptr_.load(std::memory_order_relaxed);
        if (data) {
            std::cout << "Consumer: Received data with acquire semantics: " << *data << std::endl;
            delete data;
        }
    }

    void BasicAtomicsDemo::cas_worker(int worker_id, int attempts) {
        for (int i = 0; i < attempts; ++i) {
            int expected = atomic_counter_.load();
            int desired;
            
            do {
                desired = expected + 1;
            } while (!atomic_counter_.compare_exchange_weak(expected, desired));
            
            std::cout << "Worker " << worker_id << " CAS success: " << expected 
                      << " -> " << desired << std::endl;
        }
    }

    // ========== MemoryOrderingDemo Implementation ==========

    void MemoryOrderingDemo::demonstrate_relaxed_ordering() {
        std::cout << "=== Relaxed Memory Ordering ===\n";
        
        data_.store(0);
        flag_.store(false);
        
        std::thread producer([this]() {
            relaxed_producer();
        });
        
        std::thread consumer([this]() {
            relaxed_consumer();
        });
        
        producer.join();
        consumer.join();
        
        std::cout << "Relaxed ordering allows reordering but maintains atomicity\n\n";
    }

    void MemoryOrderingDemo::demonstrate_acquire_release() {
        std::cout << "=== Acquire-Release Ordering ===\n";
        
        data_.store(0);
        flag_.store(false);
        
        std::thread producer([this]() {
            acquire_release_producer();
        });
        
        std::thread consumer([this]() {
            acquire_release_consumer();
        });
        
        producer.join();
        consumer.join();
        
        std::cout << "Acquire-release provides synchronization guarantees\n\n";
    }

    void MemoryOrderingDemo::demonstrate_sequential_consistency() {
        std::cout << "=== Sequential Consistency ===\n";
        
        x_.store(0);
        y_.store(0);
        r1_.store(0);
        r2_.store(0);
        
        std::thread t1([this]() {
            x_.store(1, std::memory_order_seq_cst);
            r1_.store(y_.load(std::memory_order_seq_cst), std::memory_order_seq_cst);
        });
        
        std::thread t2([this]() {
            y_.store(1, std::memory_order_seq_cst);
            r2_.store(x_.load(std::memory_order_seq_cst), std::memory_order_seq_cst);
        });
        
        t1.join();
        t2.join();
        
        std::cout << "Sequential consistency results: r1=" << r1_.load() 
                  << ", r2=" << r2_.load() << std::endl;
        std::cout << "With seq_cst, at least one read should see the write\n\n";
    }

    void MemoryOrderingDemo::demonstrate_consume_ordering() {
        std::cout << "=== Consume Ordering (Deprecated) ===\n";
        std::cout << "memory_order_consume is deprecated in C++17\n";
        std::cout << "Most implementations treat it as memory_order_acquire\n\n";
    }

    void MemoryOrderingDemo::demonstrate_memory_fences() {
        std::cout << "=== Memory Fences ===\n";
        
        data_.store(0);
        flag_.store(false);
        
        std::thread producer([this]() {
            data_.store(42, std::memory_order_relaxed);
            std::atomic_thread_fence(std::memory_order_release);
            flag_.store(true, std::memory_order_relaxed);
            std::cout << "Producer: Used release fence for synchronization\n";
        });
        
        std::thread consumer([this]() {
            while (!flag_.load(std::memory_order_relaxed)) {
                std::this_thread::yield();
            }
            std::atomic_thread_fence(std::memory_order_acquire);
            int value = data_.load(std::memory_order_relaxed);
            std::cout << "Consumer: Read value " << value << " after acquire fence\n";
        });
        
        producer.join();
        consumer.join();
        
        std::cout << "Memory fences provide ordering guarantees without atomic variables\n\n";
    }

    void MemoryOrderingDemo::relaxed_producer() {
        data_.store(100, std::memory_order_relaxed);
        flag_.store(true, std::memory_order_relaxed);
        std::cout << "Relaxed producer: Data and flag set with relaxed ordering\n";
    }

    void MemoryOrderingDemo::relaxed_consumer() {
        while (!flag_.load(std::memory_order_relaxed)) {
            std::this_thread::yield();
        }
        int value = data_.load(std::memory_order_relaxed);
        std::cout << "Relaxed consumer: Read value " << value << std::endl;
    }

    void MemoryOrderingDemo::acquire_release_producer() {
        data_.store(200, std::memory_order_relaxed);
        flag_.store(true, std::memory_order_release);
        std::cout << "Acquire-release producer: Used release semantics\n";
    }

    void MemoryOrderingDemo::acquire_release_consumer() {
        while (!flag_.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        int value = data_.load(std::memory_order_relaxed);
        std::cout << "Acquire-release consumer: Read value " << value << " with acquire semantics\n";
    }

    // ========== PerformanceAnalyzer Implementation ==========

    std::vector<PerformanceAnalyzer::PerformanceResult> 
    PerformanceAnalyzer::compare_atomic_vs_mutex(size_t operations) {
        std::vector<PerformanceResult> results;
        
        results.push_back(test_atomic_increment(operations, 1));
        results.push_back(test_atomic_increment(operations, 4));
        results.push_back(test_mutex_increment(operations, 1));
        results.push_back(test_mutex_increment(operations, 4));
        
        return results;
    }

    std::vector<PerformanceAnalyzer::PerformanceResult> 
    PerformanceAnalyzer::test_lock_free_structures(size_t operations) {
        std::vector<PerformanceResult> results;
        
        results.push_back(test_lock_free_stack(operations, 4));
        results.push_back(test_lock_free_queue(operations, 4));
        results.push_back(test_atomic_hash_map(operations, 4));
        
        return results;
    }

    void PerformanceAnalyzer::print_results(const std::vector<PerformanceResult>& results) {
        std::cout << "\n=== PERFORMANCE ANALYSIS RESULTS ===\n";
        std::cout << std::left << std::setw(25) << "Operation"
                  << std::setw(10) << "Threads"
                  << std::setw(15) << "Time (μs)"
                  << std::setw(15) << "Operations"
                  << std::setw(15) << "Ops/Second\n";
        std::cout << std::string(80, '-') << std::endl;
        
        for (const auto& result : results) {
            std::cout << std::left << std::setw(25) << result.operation_type
                      << std::setw(10) << result.thread_count
                      << std::setw(15) << result.duration.count()
                      << std::setw(15) << result.operations_count
                      << std::setw(15) << std::fixed << std::setprecision(0) << result.ops_per_second
                      << std::endl;
        }
        std::cout << std::string(80, '-') << std::endl << std::endl;
    }

    PerformanceAnalyzer::PerformanceResult 
    PerformanceAnalyzer::test_atomic_increment(size_t operations, size_t threads) {
        std::atomic<int64_t> counter{0};
        
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<std::thread> workers;
        size_t ops_per_thread = operations / threads;
        
        for (size_t i = 0; i < threads; ++i) {
            workers.emplace_back([&counter, ops_per_thread]() {
                for (size_t j = 0; j < ops_per_thread; ++j) {
                    counter.fetch_add(1, std::memory_order_relaxed);
                }
            });
        }
        
        for (auto& worker : workers) {
            worker.join();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        return {
            "Atomic Increment",
            duration,
            operations,
            static_cast<double>(operations) / duration.count() * 1000000,
            threads
        };
    }

    PerformanceAnalyzer::PerformanceResult 
    PerformanceAnalyzer::test_mutex_increment(size_t operations, size_t threads) {
        int64_t counter = 0;
        std::mutex counter_mutex;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<std::thread> workers;
        size_t ops_per_thread = operations / threads;
        
        for (size_t i = 0; i < threads; ++i) {
            workers.emplace_back([&counter, &counter_mutex, ops_per_thread]() {
                for (size_t j = 0; j < ops_per_thread; ++j) {
                    std::lock_guard<std::mutex> lock(counter_mutex);
                    counter++;
                }
            });
        }
        
        for (auto& worker : workers) {
            worker.join();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        return {
            "Mutex Increment",
            duration,
            operations,
            static_cast<double>(operations) / duration.count() * 1000000,
            threads
        };
    }

    PerformanceAnalyzer::PerformanceResult 
    PerformanceAnalyzer::test_lock_free_stack(size_t operations, size_t threads) {
        LockFreeStack<int> stack;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<std::thread> workers;
        size_t ops_per_thread = operations / threads;
        
        for (size_t i = 0; i < threads; ++i) {
            workers.emplace_back([&stack, ops_per_thread, i]() {
                // Push operations
                for (size_t j = 0; j < ops_per_thread / 2; ++j) {
                    stack.push(static_cast<int>(i * 1000 + j));
                }
                // Pop operations
                for (size_t j = 0; j < ops_per_thread / 2; ++j) {
                    auto item = stack.pop();
                }
            });
        }
        
        for (auto& worker : workers) {
            worker.join();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        return {
            "Lock-Free Stack",
            duration,
            operations,
            static_cast<double>(operations) / duration.count() * 1000000,
            threads
        };
    }

    PerformanceAnalyzer::PerformanceResult 
    PerformanceAnalyzer::test_lock_free_queue(size_t operations, size_t threads) {
        LockFreeQueue<int> queue;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<std::thread> workers;
        size_t ops_per_thread = operations / threads;
        
        for (size_t i = 0; i < threads; ++i) {
            workers.emplace_back([&queue, ops_per_thread, i]() {
                // Enqueue operations
                for (size_t j = 0; j < ops_per_thread / 2; ++j) {
                    queue.enqueue(static_cast<int>(i * 1000 + j));
                }
                // Dequeue operations
                for (size_t j = 0; j < ops_per_thread / 2; ++j) {
                    auto item = queue.dequeue();
                }
            });
        }
        
        for (auto& worker : workers) {
            worker.join();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        return {
            "Lock-Free Queue",
            duration,
            operations,
            static_cast<double>(operations) / duration.count() * 1000000,
            threads
        };
    }

    PerformanceAnalyzer::PerformanceResult 
    PerformanceAnalyzer::test_atomic_hash_map(size_t operations, size_t threads) {
        AtomicHashMap<int, int> hash_map;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<std::thread> workers;
        size_t ops_per_thread = operations / threads;
        
        for (size_t i = 0; i < threads; ++i) {
            workers.emplace_back([&hash_map, ops_per_thread, i]() {
                for (size_t j = 0; j < ops_per_thread; ++j) {
                    int key = static_cast<int>(i * 1000 + j);
                    int value = key * 2;
                    hash_map.insert(key, value);
                    
                    int retrieved_value;
                    hash_map.find(key, retrieved_value);
                }
            });
        }
        
        for (auto& worker : workers) {
            worker.join();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        return {
            "Atomic HashMap",
            duration,
            operations,
            static_cast<double>(operations) / duration.count() * 1000000,
            threads
        };
    }

    // ========== HazardPointerManager Implementation ==========

    HazardPointerManager& HazardPointerManager::instance() {
        static HazardPointerManager instance;
        return instance;
    }

    // ========== ConcurrentBloomFilter Implementation ==========

    ConcurrentBloomFilter::ConcurrentBloomFilter(size_t size, size_t hash_count) 
        : bit_array_(size), hash_count_(hash_count) {
        for (auto& bit : bit_array_) {
            bit.store(false);
        }
    }

    void ConcurrentBloomFilter::insert(const std::string& item) {
        auto hash_values = get_hash_values(item);
        for (auto hash : hash_values) {
            bit_array_[hash % bit_array_.size()].store(true, std::memory_order_relaxed);
        }
        insert_count_.fetch_add(1, std::memory_order_relaxed);
    }

    bool ConcurrentBloomFilter::might_contain(const std::string& item) const {
        auto hash_values = get_hash_values(item);
        for (auto hash : hash_values) {
            if (!bit_array_[hash % bit_array_.size()].load(std::memory_order_relaxed)) {
                return false;
            }
        }
        return true;
    }

    void ConcurrentBloomFilter::clear() {
        for (auto& bit : bit_array_) {
            bit.store(false, std::memory_order_relaxed);
        }
        insert_count_.store(0, std::memory_order_relaxed);
    }

    double ConcurrentBloomFilter::estimated_fill_ratio() const {
        size_t set_bits = 0;
        for (const auto& bit : bit_array_) {
            if (bit.load(std::memory_order_relaxed)) {
                set_bits++;
            }
        }
        return static_cast<double>(set_bits) / bit_array_.size();
    }

    std::vector<size_t> ConcurrentBloomFilter::get_hash_values(const std::string& item) const {
        std::vector<size_t> hashes;
        std::hash<std::string> hasher;
        
        for (size_t i = 0; i < hash_count_; ++i) {
            std::string modified_item = item + std::to_string(i);
            hashes.push_back(hasher(modified_item));
        }
        
        return hashes;
    }

    // ========== AtomicsDemo Implementation ==========

    void AtomicsDemo::demonstrate_basic_atomics() {
        print_section_header("Basic Atomic Operations");
        
        BasicAtomicsDemo demo;
        demo.demonstrate_basic_atomic_types();
        demo.demonstrate_atomic_operations();
        demo.demonstrate_compare_and_swap();
        demo.demonstrate_atomic_flag();
        demo.demonstrate_atomic_vs_mutex_performance();
        
        print_section_footer();
    }

    void AtomicsDemo::demonstrate_memory_ordering() {
        print_section_header("Memory Ordering");
        
        MemoryOrderingDemo demo;
        demo.demonstrate_relaxed_ordering();
        demo.demonstrate_acquire_release();
        demo.demonstrate_sequential_consistency();
        demo.demonstrate_consume_ordering();
        demo.demonstrate_memory_fences();
        
        print_section_footer();
    }

    void AtomicsDemo::demonstrate_lock_free_stack() {
        print_section_header("Lock-Free Stack");
        
        LockFreeStack<std::string> stack;
        
        std::vector<std::thread> producers;
        std::vector<std::thread> consumers;
        
        // Start producers
        for (int i = 0; i < 3; ++i) {
            producers.emplace_back([&stack, i]() {
                for (int j = 0; j < 5; ++j) {
                    std::string item = "Item-" + std::to_string(i) + "-" + std::to_string(j);
                    stack.push(item);
                    std::cout << "Producer " << i << " pushed: " << item 
                              << " (stack size: ~" << stack.size() << ")\n";
                    simulate_work(std::chrono::milliseconds(50));
                }
            });
        }
        
        // Start consumers
        for (int i = 0; i < 2; ++i) {
            consumers.emplace_back([&stack, i]() {
                for (int j = 0; j < 7; ++j) {
                    auto item = stack.pop();
                    if (item) {
                        std::cout << "Consumer " << i << " popped: " << *item 
                                  << " (stack size: ~" << stack.size() << ")\n";
                    } else {
                        std::cout << "Consumer " << i << " found empty stack\n";
                        j--; // Retry
                    }
                    simulate_work(std::chrono::milliseconds(100));
                }
            });
        }
        
        for (auto& p : producers) p.join();
        for (auto& c : consumers) c.join();
        
        std::cout << "Final stack size: " << stack.size() << std::endl;
        std::cout << "Lock-free stack provides thread-safe operations without locks\n";
        
        print_section_footer();
    }

    void AtomicsDemo::demonstrate_lock_free_queue() {
        print_section_header("Lock-Free Queue");
        
        LockFreeQueue<int> queue;
        
        std::vector<std::thread> threads;
        
        // Producer threads
        for (int i = 0; i < 2; ++i) {
            threads.emplace_back([&queue, i]() {
                for (int j = 0; j < 8; ++j) {
                    int value = i * 100 + j;
                    queue.enqueue(value);
                    std::cout << "Producer " << i << " enqueued: " << value 
                              << " (queue size: ~" << queue.size() << ")\n";
                    simulate_work(std::chrono::milliseconds(75));
                }
            });
        }
        
        // Consumer threads
        for (int i = 0; i < 3; ++i) {
            threads.emplace_back([&queue, i]() {
                for (int j = 0; j < 5; ++j) {
                    auto item = queue.dequeue();
                    if (item) {
                        std::cout << "Consumer " << i << " dequeued: " << *item 
                                  << " (queue size: ~" << queue.size() << ")\n";
                    } else {
                        std::cout << "Consumer " << i << " found empty queue\n";
                        j--; // Retry
                    }
                    simulate_work(std::chrono::milliseconds(120));
                }
            });
        }
        
        for (auto& t : threads) t.join();
        
        std::cout << "Final queue size: " << queue.size() << std::endl;
        std::cout << "Lock-free queue enables efficient producer-consumer patterns\n";
        
        print_section_footer();
    }

    void AtomicsDemo::demonstrate_atomic_counter() {
        print_section_header("Atomic Counter");
        
        AtomicCounter counter;
        std::vector<std::thread> threads;
        
        // Increment workers
        for (int i = 0; i < 3; ++i) {
            threads.emplace_back([&counter, i]() {
                for (int j = 0; j < 10; ++j) {
                    int64_t value = counter.increment();
                    std::cout << "Worker " << i << " incremented to: " << value << std::endl;
                    simulate_work(std::chrono::milliseconds(20));
                }
            });
        }
        
        // Decrement workers
        for (int i = 0; i < 2; ++i) {
            threads.emplace_back([&counter, i]() {
                for (int j = 0; j < 8; ++j) {
                    int64_t value = counter.decrement();
                    std::cout << "Worker " << (i + 3) << " decremented to: " << value << std::endl;
                    simulate_work(std::chrono::milliseconds(30));
                }
            });
        }
        
        for (auto& t : threads) t.join();
        
        std::cout << "\nFinal Statistics:\n";
        std::cout << "Value: " << counter.get() << std::endl;
        std::cout << "Increments: " << counter.get_increments() << std::endl;
        std::cout << "Decrements: " << counter.get_decrements() << std::endl;
        
        print_section_footer();
    }

    void AtomicsDemo::demonstrate_spinlocks() {
        print_section_header("Spinlocks");
        
        SpinLock spin_lock;
        RWSpinLock rw_spin_lock;
        std::string shared_data = "";
        
        std::vector<std::thread> threads;
        
        // Test basic spinlock
        std::cout << "Testing basic spinlock:\n";
        for (int i = 0; i < 4; ++i) {
            threads.emplace_back([&spin_lock, &shared_data, i]() {
                for (int j = 0; j < 3; ++j) {
                    spin_lock.lock();
                    shared_data += std::to_string(i);
                    std::cout << "Thread " << i << " modified data: " << shared_data << std::endl;
                    simulate_work(std::chrono::milliseconds(10));
                    spin_lock.unlock();
                }
            });
        }
        
        for (auto& t : threads) t.join();
        threads.clear();
        
        std::cout << "\nTesting RW spinlock:\n";
        shared_data = "Initial RW data";
        
        // Reader threads
        for (int i = 0; i < 3; ++i) {
            threads.emplace_back([&rw_spin_lock, &shared_data, i]() {
                for (int j = 0; j < 4; ++j) {
                    rw_spin_lock.read_lock();
                    std::cout << "Reader " << i << " read: " << shared_data << std::endl;
                    simulate_work(std::chrono::milliseconds(50));
                    rw_spin_lock.read_unlock();
                }
            });
        }
        
        // Writer threads
        for (int i = 0; i < 2; ++i) {
            threads.emplace_back([&rw_spin_lock, &shared_data, i]() {
                for (int j = 0; j < 2; ++j) {
                    rw_spin_lock.write_lock();
                    shared_data = "Modified by writer " + std::to_string(i) + 
                                 " iteration " + std::to_string(j);
                    std::cout << "Writer " << i << " wrote: " << shared_data << std::endl;
                    simulate_work(std::chrono::milliseconds(100));
                    rw_spin_lock.write_unlock();
                }
            });
        }
        
        for (auto& t : threads) t.join();
        
        std::cout << "Spinlocks provide low-latency synchronization\n";
        
        print_section_footer();
    }

    void AtomicsDemo::demonstrate_atomic_hash_map() {
        print_section_header("Lock-Free Hash Map");
        
        AtomicHashMap<std::string, int> hash_map;
        std::vector<std::thread> threads;
        
        // Insert workers
        for (int i = 0; i < 3; ++i) {
            threads.emplace_back([&hash_map, i]() {
                for (int j = 0; j < 5; ++j) {
                    std::string key = "key" + std::to_string(i) + "_" + std::to_string(j);
                    int value = i * 100 + j;
                    
                    if (hash_map.insert(key, value)) {
                        std::cout << "Thread " << i << " inserted: " << key 
                                  << " = " << value << std::endl;
                    } else {
                        std::cout << "Thread " << i << " failed to insert: " << key << std::endl;
                    }
                }
            });
        }
        
        // Update workers
        for (int i = 0; i < 2; ++i) {
            threads.emplace_back([&hash_map, i]() {
                simulate_work(std::chrono::milliseconds(100)); // Wait for some inserts
                
                for (int j = 0; j < 3; ++j) {
                    std::string key = "key0_" + std::to_string(j);
                    int new_value = 999 + i * 10 + j;
                    
                    if (hash_map.update(key, new_value)) {
                        std::cout << "Updater " << i << " updated: " << key 
                                  << " = " << new_value << std::endl;
                    }
                }
            });
        }
        
        // Reader workers
        for (int i = 0; i < 2; ++i) {
            threads.emplace_back([&hash_map, i]() {
                simulate_work(std::chrono::milliseconds(200)); // Wait for inserts/updates
                
                for (int j = 0; j < 5; ++j) {
                    std::string key = "key" + std::to_string(j % 3) + "_" + std::to_string(j);
                    int value;
                    
                    if (hash_map.find(key, value)) {
                        std::cout << "Reader " << i << " found: " << key 
                                  << " = " << value << std::endl;
                    } else {
                        std::cout << "Reader " << i << " not found: " << key << std::endl;
                    }
                }
            });
        }
        
        for (auto& t : threads) t.join();
        
        std::cout << "Lock-free hash map enables concurrent access without locks\n";
        
        print_section_footer();
    }

    void AtomicsDemo::demonstrate_performance_comparison() {
        print_section_header("Performance Comparison");
        
        auto atomic_vs_mutex = PerformanceAnalyzer::compare_atomic_vs_mutex(100000);
        PerformanceAnalyzer::print_results(atomic_vs_mutex);
        
        auto lock_free_structures = PerformanceAnalyzer::test_lock_free_structures(50000);
        PerformanceAnalyzer::print_results(lock_free_structures);
        
        print_section_footer();
    }

    void AtomicsDemo::demonstrate_atomic_statistics() {
        print_section_header("Atomic Statistics");
        
        AtomicStatistics stats;
        std::vector<std::thread> threads;
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(0.0, 100.0);
        
        for (int i = 0; i < 5; ++i) {
            threads.emplace_back([&stats, &gen, &dis, i]() {
                for (int j = 0; j < 20; ++j) {
                    double value = dis(gen);
                    stats.record_value(value);
                    std::cout << "Thread " << i << " recorded value: " 
                              << std::fixed << std::setprecision(2) << value << std::endl;
                    simulate_work(std::chrono::milliseconds(10));
                }
            });
        }
        
        for (auto& t : threads) t.join();
        
        std::cout << "\nFinal Statistics:\n";
        std::cout << "Count: " << stats.get_count() << std::endl;
        std::cout << "Sum: " << std::fixed << std::setprecision(2) << stats.get_sum() << std::endl;
        std::cout << "Mean: " << std::fixed << std::setprecision(2) << stats.get_mean() << std::endl;
        std::cout << "Min: " << std::fixed << std::setprecision(2) << stats.get_min() << std::endl;
        std::cout << "Max: " << std::fixed << std::setprecision(2) << stats.get_max() << std::endl;
        
        print_section_footer();
    }

    void AtomicsDemo::demonstrate_bloom_filter() {
        print_section_header("Concurrent Bloom Filter");
        
        ConcurrentBloomFilter bloom_filter(1000, 3);
        std::vector<std::thread> threads;
        std::vector<std::string> test_items;
        
        // Generate test items
        for (int i = 0; i < 50; ++i) {
            test_items.push_back("item_" + std::to_string(i));
        }
        
        // Insert workers
        for (int i = 0; i < 3; ++i) {
            threads.emplace_back([&bloom_filter, &test_items, i]() {
                for (size_t j = i * 15; j < (i + 1) * 15 && j < test_items.size(); ++j) {
                    bloom_filter.insert(test_items[j]);
                    std::cout << "Thread " << i << " inserted: " << test_items[j] << std::endl;
                    simulate_work(std::chrono::milliseconds(20));
                }
            });
        }
        
        // Query workers
        for (int i = 0; i < 2; ++i) {
            threads.emplace_back([&bloom_filter, &test_items, i]() {
                simulate_work(std::chrono::milliseconds(100)); // Wait for some insertions
                
                for (size_t j = 0; j < test_items.size(); j += 5) {
                    bool might_contain = bloom_filter.might_contain(test_items[j]);
                    std::cout << "Query " << i << " - " << test_items[j] 
                              << ": " << (might_contain ? "MAYBE" : "NO") << std::endl;
                }
                
                // Test false items
                for (int k = 0; k < 3; ++k) {
                    std::string false_item = "false_item_" + std::to_string(i * 10 + k);
                    bool might_contain = bloom_filter.might_contain(false_item);
                    std::cout << "Query " << i << " - " << false_item 
                              << ": " << (might_contain ? "MAYBE" : "NO") << std::endl;
                }
            });
        }
        
        for (auto& t : threads) t.join();
        
        std::cout << "\nBloom Filter Statistics:\n";
        std::cout << "Fill Ratio: " << std::fixed << std::setprecision(3) 
                  << bloom_filter.estimated_fill_ratio() << std::endl;
        std::cout << "Bloom filters provide probabilistic membership testing\n";
        
        print_section_footer();
    }

    void AtomicsDemo::run_all_demonstrations() {
        std::cout << "\n=====================================================\n";
        std::cout << "COMPREHENSIVE ATOMIC OPERATIONS DEMONSTRATIONS\n";
        std::cout << "=====================================================\n\n";
        
        demonstrate_basic_atomics();
        demonstrate_memory_ordering();
        demonstrate_lock_free_stack();
        demonstrate_lock_free_queue();
        demonstrate_atomic_counter();
        demonstrate_spinlocks();
        demonstrate_atomic_hash_map();
        demonstrate_performance_comparison();
        demonstrate_atomic_statistics();
        demonstrate_bloom_filter();
        
        std::cout << "\n=======================================\n";
        std::cout << "ALL ATOMIC DEMONSTRATIONS COMPLETED!\n";
        std::cout << "=======================================\n\n";
    }

    void AtomicsDemo::print_section_header(const std::string& title) {
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << title << std::endl;
        std::cout << std::string(60, '=') << std::endl << std::endl;
    }

    void AtomicsDemo::print_section_footer() {
        std::cout << std::string(60, '-') << std::endl;
        std::cout << "Section Complete\n" << std::endl;
    }

    void AtomicsDemo::simulate_work(std::chrono::milliseconds duration) {
        std::this_thread::sleep_for(duration);
    }

    // ========== AtomicCounter Implementation ==========

    AtomicCounter::AtomicCounter() : value_(0), increment_count_(0), decrement_count_(0) {}

    int64_t AtomicCounter::increment() {
        increment_count_.fetch_add(1, std::memory_order_relaxed);
        return value_.fetch_add(1, std::memory_order_acq_rel) + 1;
    }

    int64_t AtomicCounter::decrement() {
        decrement_count_.fetch_add(1, std::memory_order_relaxed);
        return value_.fetch_sub(1, std::memory_order_acq_rel) - 1;
    }

    int64_t AtomicCounter::get() const {
        return value_.load(std::memory_order_acquire);
    }

    void AtomicCounter::reset() {
        value_.store(0, std::memory_order_release);
        increment_count_.store(0, std::memory_order_relaxed);
        decrement_count_.store(0, std::memory_order_relaxed);
    }

    int64_t AtomicCounter::get_increments() const {
        return increment_count_.load(std::memory_order_relaxed);
    }

    int64_t AtomicCounter::get_decrements() const {
        return decrement_count_.load(std::memory_order_relaxed);
    }

    // ========== SpinLock Implementation ==========

    SpinLock::SpinLock() : flag_(ATOMIC_FLAG_INIT) {}

    void SpinLock::lock() {
        while (flag_.test_and_set(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
    }

    void SpinLock::unlock() {
        flag_.clear(std::memory_order_release);
    }

    bool SpinLock::try_lock() {
        return !flag_.test_and_set(std::memory_order_acquire);
    }

    // ========== RWSpinLock Implementation ==========

    RWSpinLock::RWSpinLock() : readers_(0), writer_flag_(ATOMIC_FLAG_INIT) {}

    void RWSpinLock::read_lock() {
        while (true) {
            // Wait for writer to finish
            while (writer_flag_.test(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            
            // Increment reader count
            readers_.fetch_add(1, std::memory_order_acquire);
            
            // Check if writer started while we were incrementing
            if (!writer_flag_.test(std::memory_order_acquire)) {
                break; // Success
            }
            
            // Writer started, back off
            readers_.fetch_sub(1, std::memory_order_release);
        }
    }

    void RWSpinLock::read_unlock() {
        readers_.fetch_sub(1, std::memory_order_release);
    }

    void RWSpinLock::write_lock() {
        // Acquire writer flag
        while (writer_flag_.test_and_set(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        
        // Wait for all readers to finish
        while (readers_.load(std::memory_order_acquire) > 0) {
            std::this_thread::yield();
        }
    }

    void RWSpinLock::write_unlock() {
        writer_flag_.clear(std::memory_order_release);
    }

    // ========== AtomicStatistics Implementation ==========

    AtomicStatistics::AtomicStatistics() : count_(0), sum_(0.0), min_(std::numeric_limits<double>::max()),
                                           max_(std::numeric_limits<double>::lowest()) {}

    void AtomicStatistics::record_value(double value) {
        count_.fetch_add(1, std::memory_order_relaxed);
        
        // Update sum atomically
        double current_sum = sum_.load(std::memory_order_relaxed);
        while (!sum_.compare_exchange_weak(current_sum, current_sum + value, std::memory_order_relaxed)) {
            // Retry with updated current_sum
        }
        
        // Update min atomically
        double current_min = min_.load(std::memory_order_relaxed);
        while (value < current_min && 
               !min_.compare_exchange_weak(current_min, value, std::memory_order_relaxed)) {
            // Retry with updated current_min
        }
        
        // Update max atomically
        double current_max = max_.load(std::memory_order_relaxed);
        while (value > current_max && 
               !max_.compare_exchange_weak(current_max, value, std::memory_order_relaxed)) {
            // Retry with updated current_max
        }
    }

    size_t AtomicStatistics::get_count() const {
        return count_.load(std::memory_order_relaxed);
    }

    double AtomicStatistics::get_sum() const {
        return sum_.load(std::memory_order_relaxed);
    }

    double AtomicStatistics::get_mean() const {
        size_t count = get_count();
        return count > 0 ? get_sum() / count : 0.0;
    }

    double AtomicStatistics::get_min() const {
        double min_val = min_.load(std::memory_order_relaxed);
        return min_val == std::numeric_limits<double>::max() ? 0.0 : min_val;
    }

    double AtomicStatistics::get_max() const {
        double max_val = max_.load(std::memory_order_relaxed);
        return max_val == std::numeric_limits<double>::lowest() ? 0.0 : max_val;
    }

    void AtomicStatistics::reset() {
        count_.store(0, std::memory_order_relaxed);
        sum_.store(0.0, std::memory_order_relaxed);
        min_.store(std::numeric_limits<double>::max(), std::memory_order_relaxed);
        max_.store(std::numeric_limits<double>::lowest(), std::memory_order_relaxed);
    }

} // namespace CppVerseHub::Concurrency