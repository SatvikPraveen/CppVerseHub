/**
 * @file MemoryPools.cpp
 * @brief Implementation of advanced memory pool demonstrations
 * @details File location: src/memory/MemoryPools.cpp
 */

#include "MemoryPools.hpp"
#include <random>
#include <iomanip>

namespace CppVerseHub::Memory {

    void MemoryPoolDemo::demonstrateFixedSizePool() {
        std::cout << "\n=== Fixed Size Pool Demonstration ===\n";
        
        FixedSizePool<64> pool;
        std::vector<void*> allocated_blocks;
        
        // Allocate several blocks
        for (int i = 0; i < 10; ++i) {
            void* block = pool.allocate();
            if (block) {
                allocated_blocks.push_back(block);
                
                // Write some test data
                *static_cast<int*>(block) = i * 42;
            }
        }
        
        pool.print_statistics();
        
        // Verify data integrity
        std::cout << "\nData verification:\n";
        for (size_t i = 0; i < allocated_blocks.size(); ++i) {
            int value = *static_cast<int*>(allocated_blocks[i]);
            std::cout << "Block " << i << ": " << value << " (expected: " << (i * 42) << ")\n";
        }
        
        // Deallocate some blocks
        for (size_t i = 0; i < allocated_blocks.size() / 2; ++i) {
            pool.deallocate(allocated_blocks[i]);
        }
        
        // Allocate more blocks (should reuse deallocated ones)
        std::cout << "\nAllocating more blocks (should reuse freed blocks):\n";
        for (int i = 0; i < 3; ++i) {
            void* block = pool.allocate();
            if (block) {
                *static_cast<int*>(block) = 999 + i;
                allocated_blocks.push_back(block);
            }
        }
        
        pool.print_statistics();
        
        // Test pool exhaustion and expansion
        std::cout << "\nTesting pool expansion:\n";
        std::vector<void*> expansion_blocks;
        for (int i = 0; i < 100; ++i) {
            void* block = pool.allocate();
            if (block) {
                expansion_blocks.push_back(block);
                *static_cast<int*>(block) = i;
            }
        }
        
        pool.print_statistics();
        
        // Clean up remaining blocks
        for (void* block : allocated_blocks) {
            pool.deallocate(block);
        }
        for (void* block : expansion_blocks) {
            pool.deallocate(block);
        }
        
        std::cout << "Fixed size pool demonstration complete\n";
    }

    void MemoryPoolDemo::demonstrateVariableSizePool() {
        std::cout << "\n=== Variable Size Pool Demonstration ===\n";
        
        VariableSizePool pool;
        std::vector<std::pair<void*, size_t>> allocations;
        
        // Allocate various sizes
        std::vector<size_t> sizes = {16, 32, 64, 128, 256, 512, 1024, 48, 96, 200};
        
        for (size_t size : sizes) {
            void* ptr = pool.allocate(size);
            if (ptr) {
                allocations.emplace_back(ptr, size);
                
                // Write test pattern
                memset(ptr, static_cast<int>(size & 0xFF), size);
                std::cout << "Allocated " << size << " bytes\n";
            }
        }
        
        pool.print_statistics();
        
        // Verify data integrity
        std::cout << "\nData integrity check:\n";
        for (const auto& [ptr, size] : allocations) {
            unsigned char* data = static_cast<unsigned char*>(ptr);
            unsigned char expected = static_cast<unsigned char>(size & 0xFF);
            bool valid = true;
            
            for (size_t i = 0; i < size; ++i) {
                if (data[i] != expected) {
                    valid = false;
                    break;
                }
            }
            
            std::cout << "Block of " << size << " bytes: " << (valid ? "OK" : "CORRUPTED") << "\n";
        }
        
        // Deallocate some blocks
        std::cout << "\nDeallocating some blocks:\n";
        for (size_t i = 0; i < allocations.size() / 2; ++i) {
            pool.deallocate(allocations[i].first, allocations[i].second);
        }
        
        pool.print_statistics();
        
        // Allocate more to test free list reuse
        std::cout << "\nReallocating to test free list reuse:\n";
        for (size_t i = 0; i < 5; ++i) {
            void* ptr = pool.allocate(64);
            if (ptr) {
                std::cout << "Reallocated 64 bytes\n";
                allocations.emplace_back(ptr, 64);
            }
        }
        
        pool.print_statistics();
        
        // Clean up all allocations
        for (const auto& [ptr, size] : allocations) {
            pool.deallocate(ptr, size);
        }
        
        std::cout << "Variable size pool demonstration complete\n";
    }

    void MemoryPoolDemo::demonstrateObjectPool() {
        std::cout << "\n=== Object Pool Demonstration ===\n";
        
        ObjectPool<TestObject, 5> pool;
        std::vector<TestObject*> objects;
        
        // Acquire objects with different constructor arguments
        for (int i = 0; i < 7; ++i) {
            TestObject* obj = pool.acquire(i, "Object-" + std::to_string(i));
            if (obj) {
                objects.push_back(obj);
                obj->process();
            } else {
                std::cout << "Failed to acquire object " << i << " - pool exhausted\n";
            }
        }
        
        std::cout << "\nPool status: " << pool.objects_in_use() << " in use, " 
                  << pool.available_objects() << " available\n";
        
        // Release some objects
        std::cout << "\nReleasing some objects:\n";
        for (size_t i = 0; i < 3 && i < objects.size(); ++i) {
            pool.release(objects[i]);
            objects[i] = nullptr;
        }
        
        std::cout << "Pool status: " << pool.objects_in_use() << " in use, " 
                  << pool.available_objects() << " available\n";
        
        // Acquire more objects (should reuse released ones)
        std::cout << "\nReacquiring objects (should reuse slots):\n";
        for (int i = 10; i < 13; ++i) {
            TestObject* obj = pool.acquire(i, "Reused-" + std::to_string(i));
            if (obj) {
                // Replace nullptr entries
                for (size_t j = 0; j < objects.size(); ++j) {
                    if (objects[j] == nullptr) {
                        objects[j] = obj;
                        break;
                    }
                }
                obj->process();
            }
        }
        
        // Test double release protection
        std::cout << "\nTesting double release protection:\n";
        if (!objects.empty() && objects[0]) {
            TestObject* obj_to_double_release = objects[0];
            pool.release(obj_to_double_release);
            pool.release(obj_to_double_release); // Should detect this
        }
        
        // Clean up remaining objects
        for (TestObject* obj : objects) {
            if (obj) {
                pool.release(obj);
            }
        }
        
        std::cout << "Final pool status: " << pool.objects_in_use() << " in use\n";
        std::cout << "Object pool demonstration complete\n";
    }

    void MemoryPoolDemo::demonstrateThreadSafePool() {
        std::cout << "\n=== Thread-Safe Pool Demonstration ===\n";
        
        ThreadSafeMemoryPool<128> pool;
        constexpr int num_threads = 4;
        constexpr int allocations_per_thread = 50;
        
        std::vector<std::thread> threads;
        std::atomic<int> total_allocations{0};
        std::atomic<int> total_deallocations{0};
        
        // Launch worker threads
        for (int t = 0; t < num_threads; ++t) {
            threads.emplace_back([&pool, &total_allocations, &total_deallocations, 
                                 t, allocations_per_thread]() {
                std::vector<void*> thread_allocations;
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<> delay_dist(1, 10);
                
                std::cout << "Thread " << t << " starting\n";
                
                // Allocate blocks
                for (int i = 0; i < allocations_per_thread; ++i) {
                    void* ptr = pool.allocate();
                    if (ptr) {
                        thread_allocations.push_back(ptr);
                        
                        // Write thread-specific data
                        *static_cast<int*>(ptr) = t * 1000 + i;
                        
                        total_allocations.fetch_add(1);
                        
                        // Random delay to simulate work
                        std::this_thread::sleep_for(std::chrono::microseconds(delay_dist(gen)));
                    }
                }
                
                std::cout << "Thread " << t << " allocated " << thread_allocations.size() << " blocks\n";
                
                // Verify data integrity
                bool data_valid = true;
                for (size_t i = 0; i < thread_allocations.size(); ++i) {
                    int expected = t * 1000 + static_cast<int>(i);
                    int actual = *static_cast<int*>(thread_allocations[i]);
                    if (actual != expected) {
                        std::cout << "Thread " << t << " data corruption at index " << i 
                                  << ": expected " << expected << ", got " << actual << "\n";
                        data_valid = false;
                    }
                }
                
                if (data_valid) {
                    std::cout << "Thread " << t << " data integrity check passed\n";
                }
                
                // Deallocate blocks with random pattern
                std::shuffle(thread_allocations.begin(), thread_allocations.end(), gen);
                
                for (void* ptr : thread_allocations) {
                    pool.deallocate(ptr);
                    total_deallocations.fetch_add(1);
                    
                    std::this_thread::sleep_for(std::chrono::microseconds(delay_dist(gen)));
                }
                
                std::cout << "Thread " << t << " completed\n";
            });
        }
        
        // Wait for all threads
        for (auto& thread : threads) {
            thread.join();
        }
        
        std::cout << "\nAll threads completed\n";
        std::cout << "Total allocations: " << total_allocations.load() << "\n";
        std::cout << "Total deallocations: " << total_deallocations.load() << "\n";
        
        pool.print_statistics();
        
        std::cout << "Thread-safe pool demonstration complete\n";
    }

    void MemoryPoolDemo::demonstratePoolPerformance() {
        std::cout << "\n=== Pool Performance Demonstration ===\n";
        
        benchmark_allocation_performance();
    }

    void MemoryPoolDemo::demonstrateRealWorldScenario() {
        std::cout << "\n=== Real-World Scenario Demonstration ===\n";
        
        // Simulate a game object system with frequent allocation/deallocation
        ObjectPool<TestObject, 20> game_object_pool;
        FixedSizePool<256> buffer_pool;
        VariableSizePool message_pool;
        
        std::vector<TestObject*> active_objects;
        std::vector<void*> active_buffers;
        std::vector<std::pair<void*, size_t>> active_messages;
        
        std::cout << "Simulating game loop with object creation, messaging, and cleanup...\n";
        
        // Simulate 10 game frames
        for (int frame = 0; frame < 10; ++frame) {
            std::cout << "\n--- Frame " << frame << " ---\n";
            
            // Create some game objects
            for (int i = 0; i < 3; ++i) {
                TestObject* obj = game_object_pool.acquire(frame * 100 + i, "GameObject");
                if (obj) {
                    active_objects.push_back(obj);
                }
            }
            
            // Allocate some buffers for rendering/physics
            for (int i = 0; i < 2; ++i) {
                void* buffer = buffer_pool.allocate();
                if (buffer) {
                    active_buffers.push_back(buffer);
                    // Simulate buffer usage
                    memset(buffer, frame & 0xFF, 256);
                }
            }
            
            // Create some messages of varying sizes
            std::vector<size_t> message_sizes = {32, 64, 128, 96};
            for (size_t size : message_sizes) {
                void* msg = message_pool.allocate(size);
                if (msg) {
                    active_messages.emplace_back(msg, size);
                    // Write message header
                    *static_cast<int*>(msg) = frame;
                }
            }
            
            // Process objects
            for (TestObject* obj : active_objects) {
                if (obj) {
                    obj->process();
                }
            }
            
            // Randomly destroy some objects (simulate object lifecycle)
            if (frame > 3) {
                size_t objects_to_remove = std::min(active_objects.size(), size_t(2));
                for (size_t i = 0; i < objects_to_remove; ++i) {
                    if (!active_objects.empty()) {
                        game_object_pool.release(active_objects.back());
                        active_objects.pop_back();
                    }
                }
            }
            
            // Free some buffers
            if (frame > 1) {
                size_t buffers_to_free = std::min(active_buffers.size(), size_t(1));
                for (size_t i = 0; i < buffers_to_free; ++i) {
                    if (!active_buffers.empty()) {
                        buffer_pool.deallocate(active_buffers.back());
                        active_buffers.pop_back();
                    }
                }
            }
            
            // Process and free old messages
            if (frame > 0) {
                size_t messages_to_process = std::min(active_messages.size(), size_t(2));
                for (size_t i = 0; i < messages_to_process; ++i) {
                    if (!active_messages.empty()) {
                        auto [ptr, size] = active_messages.back();
                        // Process message
                        int frame_id = *static_cast<int*>(ptr);
                        std::cout << "Processing message from frame " << frame_id << "\n";
                        
                        message_pool.deallocate(ptr, size);
                        active_messages.pop_back();
                    }
                }
            }
            
            // Print frame statistics
            std::cout << "Frame " << frame << " stats - Objects: " << active_objects.size()
                      << ", Buffers: " << active_buffers.size() 
                      << ", Messages: " << active_messages.size() << "\n";
        }
        
        // Cleanup remaining resources
        std::cout << "\nCleaning up remaining resources...\n";
        
        for (TestObject* obj : active_objects) {
            game_object_pool.release(obj);
        }
        
        for (void* buffer : active_buffers) {
            buffer_pool.deallocate(buffer);
        }
        
        for (const auto& [ptr, size] : active_messages) {
            message_pool.deallocate(ptr, size);
        }
        
        std::cout << "\nFinal pool statistics:\n";
        std::cout << "Game objects in use: " << game_object_pool.objects_in_use() << "\n";
        buffer_pool.print_statistics();
        message_pool.print_statistics();
        
        std::cout << "Real-world scenario demonstration complete\n";
    }

    void MemoryPoolDemo::runAllDemonstrations() {
        std::cout << "\n========== MEMORY POOL COMPREHENSIVE DEMONSTRATION ==========\n";
        
        demonstrateFixedSizePool();
        demonstrateVariableSizePool();
        demonstrateObjectPool();
        demonstrateThreadSafePool();
        demonstratePoolPerformance();
        demonstrateRealWorldScenario();
        
        std::cout << "\n========== MEMORY POOL DEMONSTRATION COMPLETE ==========\n";
    }

    void MemoryPoolDemo::benchmark_allocation_performance() {
        std::cout << "\n=== Allocation Performance Benchmark ===\n";
        
        constexpr size_t iterations = 10000;
        constexpr size_t block_size = 64;
        
        // Benchmark standard allocator
        std::cout << "Benchmarking standard allocator...\n";
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<void*> standard_ptrs;
        standard_ptrs.reserve(iterations);
        
        for (size_t i = 0; i < iterations; ++i) {
            standard_ptrs.push_back(std::malloc(block_size));
        }
        
        for (void* ptr : standard_ptrs) {
            std::free(ptr);
        }
        
        auto standard_time = std::chrono::high_resolution_clock::now() - start;
        
        // Benchmark fixed size pool
        std::cout << "Benchmarking fixed size pool...\n";
        start = std::chrono::high_resolution_clock::now();
        
        FixedSizePool<block_size> pool;
        std::vector<void*> pool_ptrs;
        pool_ptrs.reserve(iterations);
        
        for (size_t i = 0; i < iterations; ++i) {
            pool_ptrs.push_back(pool.allocate());
        }
        
        for (void* ptr : pool_ptrs) {
            pool.deallocate(ptr);
        }
        
        auto pool_time = std::chrono::high_resolution_clock::now() - start;
        
        // Benchmark thread-safe pool
        std::cout << "Benchmarking thread-safe pool...\n";
        start = std::chrono::high_resolution_clock::now();
        
        ThreadSafeMemoryPool<block_size> ts_pool;
        std::vector<void*> ts_pool_ptrs;
        ts_pool_ptrs.reserve(iterations);
        
        for (size_t i = 0; i < iterations; ++i) {
            ts_pool_ptrs.push_back(ts_pool.allocate());
        }
        
        for (void* ptr : ts_pool_ptrs) {
            ts_pool.deallocate(ptr);
        }
        
        auto ts_pool_time = std::chrono::high_resolution_clock::now() - start;
        
        // Print results
        auto to_microseconds = [](const std::chrono::nanoseconds& duration) {
            return std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
        };
        
        std::cout << "\nBenchmark Results (" << iterations << " iterations):\n";
        std::cout << std::setw(20) << "Allocator" << std::setw(15) << "Time (μs)" << std::setw(15) << "Speedup" << "\n";
        std::cout << std::string(50, '-') << "\n";
        
        auto standard_us = to_microseconds(standard_time);
        auto pool_us = to_microseconds(pool_time);
        auto ts_pool_us = to_microseconds(ts_pool_time);
        
        std::cout << std::setw(20) << "Standard" << std::setw(15) << standard_us << std::setw(15) << "1.00x" << "\n";
        std::cout << std::setw(20) << "Fixed Pool" << std::setw(15) << pool_us 
                  << std::setw(15) << std::fixed << std::setprecision(2) 
                  << (static_cast<double>(standard_us) / pool_us) << "x" << "\n";
        std::cout << std::setw(20) << "Thread-Safe Pool" << std::setw(15) << ts_pool_us 
                  << std::setw(15) << std::fixed << std::setprecision(2) 
                  << (static_cast<double>(standard_us) / ts_pool_us) << "x" << "\n";
        
        // Memory fragmentation test
        std::cout << "\n=== Memory Fragmentation Test ===\n";
        std::cout << "Allocating and deallocating in random pattern...\n";
        
        std::vector<void*> fragmentation_ptrs;
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> action_dist(0, 1);
        
        FixedSizePool<block_size> frag_pool;
        
        for (int i = 0; i < 1000; ++i) {
            if (action_dist(gen) == 0 || fragmentation_ptrs.empty()) {
                // Allocate
                void* ptr = frag_pool.allocate();
                if (ptr) {
                    fragmentation_ptrs.push_back(ptr);
                }
            } else {
                // Deallocate random element
                std::uniform_int_distribution<> index_dist(0, static_cast<int>(fragmentation_ptrs.size() - 1));
                size_t index = index_dist(gen);
                frag_pool.deallocate(fragmentation_ptrs[index]);
                fragmentation_ptrs.erase(fragmentation_ptrs.begin() + index);
            }
        }
        
        std::cout << "Fragmentation test complete - " << fragmentation_ptrs.size() 
                  << " blocks still allocated\n";
        frag_pool.print_statistics();
        
        // Clean up
        for (void* ptr : fragmentation_ptrs) {
            frag_pool.deallocate(ptr);
        }
    }

} // namespace CppVerseHub::Memory