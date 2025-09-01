/**
 * @file RAII_Examples.cpp
 * @brief Implementation of RAII pattern demonstrations
 * @details File location: src/memory/RAII_Examples.cpp
 */

#include "RAII_Examples.hpp"
#include <algorithm>
#include <sstream>
#include <random>

namespace CppVerseHub::Memory {

    // Static member initialization
    int RAIIDemo::TestResource::counter_ = 0;

    void RAIIDemo::demonstrateFileRAII() {
        std::cout << "\n=== File RAII Demonstration ===\n";
        
        const std::string test_filename = "raii_test.txt";
        const std::string test_content = "Hello, RAII world!\nThis is a test file.\n";
        
        // Write to file - automatic close on scope exit
        {
            try {
                FileRAII file(test_filename, "w");
                file.write(test_content);
                file.flush();
                std::cout << "File written successfully\n";
            } catch (const std::exception& e) {
                std::cout << "File write error: " << e.what() << "\n";
                return;
            }
        } // File automatically closed here
        
        // Read from file - automatic close on scope exit
        {
            try {
                FileRAII file(test_filename, "r");
                std::string content = file.read_all();
                std::cout << "File content read: " << content.size() << " bytes\n";
                std::cout << "Content: " << content;
            } catch (const std::exception& e) {
                std::cout << "File read error: " << e.what() << "\n";
            }
        } // File automatically closed here
        
        // Move semantics demonstration
        {
            FileRAII file1(test_filename, "r");
            FileRAII file2 = std::move(file1); // Transfer ownership
            
            std::string content = file2.read_all();
            std::cout << "File moved successfully, content size: " << content.size() << "\n";
        }
        
        // Cleanup
        std::remove(test_filename.c_str());
        std::cout << "Temporary file cleaned up\n";
    }

    void RAIIDemo::demonstrateTimerRAII() {
        std::cout << "\n=== Timer RAII Demonstration ===\n";
        
        // Simple timing
        {
            TimerRAII timer("Simple operation");
            
            // Simulate some work
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            
            std::cout << "Intermediate elapsed time: " << timer.elapsed().count() << " microseconds\n";
            
            // More work
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
        } // Timer automatically prints total time
        
        // Nested timing
        {
            TimerRAII outer_timer("Outer operation");
            
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            
            {
                TimerRAII inner_timer("Inner operation");
                std::this_thread::sleep_for(std::chrono::milliseconds(40));
            } // Inner timer completes
            
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        } // Outer timer completes
        
        // Function timing using utility
        auto result = RAIIUtils::measure_execution([]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
            return 42;
        }, "Lambda execution");
        
        std::cout << "Function returned: " << result << "\n";
    }

    void RAIIDemo::demonstrateScopedLock() {
        std::cout << "\n=== Scoped Lock Demonstration ===\n";
        
        // Basic locking
        {
            ScopedLock<std::mutex> lock(demo_mutex_);
            std::cout << "Critical section 1 - mutex is locked\n";
            
            // Simulate critical section work
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        } // Mutex automatically unlocked
        
        // Manual unlock/lock
        {
            ScopedLock<std::mutex> lock(demo_mutex_);
            std::cout << "Critical section 2 start\n";
            
            lock.unlock();
            std::cout << "Mutex temporarily unlocked\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            
            lock.lock();
            std::cout << "Mutex locked again\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        } // Final unlock
        
        // Demonstrate thread safety with multiple threads
        std::vector<std::thread> threads;
        std::atomic<int> counter{0};
        
        for (int i = 0; i < 3; ++i) {
            threads.emplace_back([this, &counter, i]() {
                ScopedLock<std::mutex> lock(demo_mutex_);
                int local_counter = counter.load();
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                counter.store(local_counter + 1);
                std::cout << "Thread " << i << " incremented counter to " << counter.load() << "\n";
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        std::cout << "Final counter value: " << counter.load() << "\n";
    }

    void RAIIDemo::demonstrateResourcePool() {
        std::cout << "\n=== Resource Pool Demonstration ===\n";
        
        ResourcePool<TestResource> pool(3);
        
        std::cout << "Initial pool size: " << pool.size() 
                  << ", available: " << pool.available_count() << "\n";
        
        // Acquire and use resources
        {
            auto resource1 = pool.acquire();
            resource1->do_work();
            
            auto resource2 = pool.acquire();
            resource2->do_work();
            
            std::cout << "Pool size: " << pool.size() 
                      << ", available: " << pool.available_count() << "\n";
            
            {
                auto resource3 = pool.acquire();
                auto resource4 = pool.acquire(); // Should create new resource
                
                resource3->do_work();
                resource4->do_work();
                
                std::cout << "Pool size: " << pool.size() 
                          << ", available: " << pool.available_count() << "\n";
            } // resource3 and resource4 automatically returned
            
            std::cout << "After inner scope - Pool size: " << pool.size() 
                      << ", available: " << pool.available_count() << "\n";
        } // resource1 and resource2 automatically returned
        
        std::cout << "After outer scope - Pool size: " << pool.size() 
                  << ", available: " << pool.available_count() << "\n";
        
        // Move semantics with handles
        {
            auto resource = pool.acquire();
            resource->do_work();
            
            auto moved_resource = std::move(resource);
            moved_resource->do_work();
            
            std::cout << "Resource moved successfully\n";
        } // moved_resource automatically returned
    }

    void RAIIDemo::demonstrateNetworkConnection() {
        std::cout << "\n=== Network Connection Demonstration ===\n";
        
        // Basic connection usage
        {
            NetworkConnection conn("192.168.1.100", 8080);
            
            if (conn.is_connected()) {
                conn.send_data("Hello, server!");
                std::string response = conn.receive_data();
                std::cout << "Received: " << response << "\n";
            }
        } // Connection automatically closed
        
        // Move semantics
        {
            NetworkConnection conn1("10.0.0.1", 9090);
            NetworkConnection conn2 = std::move(conn1);
            
            conn2.send_data("Data from moved connection");
        } // conn2 automatically closes connection
        
        // Exception handling
        try {
            NetworkConnection conn("invalid.host", -1);
            conn.send_data("This won't be sent");
        } catch (const std::exception& e) {
            std::cout << "Connection error handled: " << e.what() << "\n";
        }
    }

    void RAIIDemo::demonstrateScopeGuard() {
        std::cout << "\n=== Scope Guard Demonstration ===\n";
        
        int* resource = nullptr;
        
        {
            resource = new int(42);
            
            // Create scope guard for cleanup
            auto cleanup = make_scope_guard([&resource]() {
                delete resource;
                resource = nullptr;
                std::cout << "ScopeGuard: Resource cleaned up\n";
            });
            
            std::cout << "Resource allocated and guard created\n";
            std::cout << "Resource value: " << *resource << "\n";
            
            // Resource will be cleaned up automatically when cleanup goes out of scope
        } // ScopeGuard destructor called here
        
        // Conditional cleanup
        {
            std::vector<int> data{1, 2, 3, 4, 5};
            bool process_succeeded = false;
            
            auto rollback = make_scope_guard([&data]() {
                data.clear();
                std::cout << "ScopeGuard: Rolling back data changes\n";
            });
            
            // Simulate processing
            for (auto& value : data) {
                value *= 2;
            }
            
            // Simulate success condition
            if (data.size() == 5) {
                process_succeeded = true;
                rollback.dismiss(); // Don't rollback on success
                std::cout << "Processing succeeded, rollback dismissed\n";
            }
            
            std::cout << "Data processing complete, success: " << process_succeeded << "\n";
        } // rollback destructor called, but cleanup is dismissed
        
        // Multiple scope guards
        {
            auto guard1 = make_scope_guard([]() { std::cout << "Guard 1 cleanup\n"; });
            auto guard2 = make_scope_guard([]() { std::cout << "Guard 2 cleanup\n"; });
            auto guard3 = make_scope_guard([]() { std::cout << "Guard 3 cleanup\n"; });
            
            std::cout << "Multiple guards created\n";
        } // Guards execute in reverse order (3, 2, 1)
    }

    void RAIIDemo::demonstrateMemoryMapper() {
        std::cout << "\n=== Memory Mapper Demonstration ===\n";
        
        const std::string filename = "mapped_file.dat";
        const size_t file_size = 1024;
        
        {
            MemoryMapper mapper(filename, file_size);
            
            // Use the mapped memory
            char* data = mapper.data();
            if (data && mapper.size() > 0) {
                // Write some test data
                std::string test_data = "Hello, memory mapped world!";
                std::copy(test_data.begin(), test_data.end(), data);
                
                // Sync to disk
                mapper.sync();
                
                std::cout << "Written " << test_data.size() 
                          << " bytes to mapped memory\n";
            }
        } // Memory automatically unmapped
        
        // Move semantics
        {
            MemoryMapper mapper1("another_file.dat", 512);
            MemoryMapper mapper2 = std::move(mapper1);
            
            std::cout << "Mapper moved, new size: " << mapper2.size() << "\n";
        } // mapper2 automatically unmaps
        
        std::cout << "Memory mapper demonstration complete\n";
    }

    void RAIIDemo::demonstrateExceptionSafety() {
        std::cout << "\n=== Exception Safety with RAII ===\n";
        
        // Test exception safety
        auto test_exception_safety = [](bool throw_exception) {
            TimerRAII timer("Exception safety test");
            
            // Multiple RAII objects
            auto cleanup1 = make_scope_guard([]() {
                std::cout << "Cleanup 1 executed (exception path)\n";
            });
            
            auto cleanup2 = make_scope_guard([]() {
                std::cout << "Cleanup 2 executed (exception path)\n";
            });
            
            if (throw_exception) {
                throw std::runtime_error("Simulated exception");
            }
            
            cleanup1.dismiss(); // Success path
            cleanup2.dismiss();
            std::cout << "Success path - cleanups dismissed\n";
        };
        
        // Normal execution
        try {
            test_exception_safety(false);
            std::cout << "Normal execution completed\n";
        } catch (const std::exception& e) {
            std::cout << "Unexpected exception: " << e.what() << "\n";
        }
        
        // Exception execution
        try {
            test_exception_safety(true);
            std::cout << "This shouldn't print\n";
        } catch (const std::exception& e) {
            std::cout << "Exception caught: " << e.what() << "\n";
            std::cout << "RAII cleanup should have been executed\n";
        }
    }

    void RAIIDemo::demonstrateNestedRAII() {
        std::cout << "\n=== Nested RAII Demonstration ===\n";
        
        {
            TimerRAII outer_timer("Outer operation");
            
            // Network connection with automatic cleanup
            {
                NetworkConnection conn("nested.example.com", 80);
                
                // File operations within network context
                {
                    auto temp_file = RAIIUtils::create_temp_file("Nested RAII content");
                    if (temp_file) {
                        temp_file->write("Additional data from network operation");
                        temp_file->flush();
                        
                        // Memory mapping within file context
                        {
                            MemoryMapper mapper("temp_mapped.dat", 256);
                            char* data = mapper.data();
                            if (data) {
                                std::string network_data = conn.receive_data();
                                std::copy(network_data.begin(), 
                                        std::min(network_data.end(), network_data.begin() + 256), 
                                        data);
                            }
                        } // Memory unmapped
                        
                        std::cout << "Nested file and memory operations complete\n";
                    }
                } // File closed
                
                conn.send_data("Nested operation complete");
            } // Connection closed
        } // Timer reports total time
        
        std::cout << "All nested RAII objects cleaned up properly\n";
    }

    void RAIIDemo::demonstrateRAIIWithSmartPointers() {
        std::cout << "\n=== RAII with Smart Pointers ===\n";
        
        // Combining RAII with smart pointers
        {
            auto temp_file = RAIIUtils::create_temp_file("Smart pointer content");
            
            if (temp_file) {
                temp_file->write("Data written through smart pointer managed RAII\n");
                
                // Create shared ownership of RAII resource
                auto shared_timer = std::make_shared<TimerRAII>("Shared timer operation");
                
                // Multiple owners
                auto timer_copy1 = shared_timer;
                auto timer_copy2 = shared_timer;
                
                std::cout << "Timer shared among " << shared_timer.use_count() << " owners\n";
                
                // Simulate work
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                
                // Timers will be destroyed when all shared_ptr instances go out of scope
            }
        }
        
        // Array RAII with smart pointers
        {
            auto array_raii = std::make_unique<RAIIUtils::ArrayRAII<int>>(100);
            
            // Initialize array
            for (size_t i = 0; i < array_raii->size(); ++i) {
                (*array_raii)[i] = static_cast<int>(i * i);
            }
            
            std::cout << "Array initialized with " << array_raii->size() << " elements\n";
            std::cout << "Sample values: " << (*array_raii)[0] << ", " 
                      << (*array_raii)[10] << ", " << (*array_raii)[50] << "\n";
        }
        
        std::cout << "Smart pointer + RAII demonstration complete\n";
    }

    void RAIIDemo::runAllDemonstrations() {
        std::cout << "\n========== RAII COMPREHENSIVE DEMONSTRATION ==========\n";
        
        demonstrateFileRAII();
        demonstrateTimerRAII();
        demonstrateScopedLock();
        demonstrateResourcePool();
        demonstrateNetworkConnection();
        demonstrateScopeGuard();
        demonstrateMemoryMapper();
        demonstrateExceptionSafety();
        demonstrateNestedRAII();
        demonstrateRAIIWithSmartPointers();
        
        std::cout << "\n========== RAII DEMONSTRATION COMPLETE ==========\n";
    }

    // RAIIUtils implementations
    namespace RAIIUtils {
        
        std::unique_ptr<FileRAII> create_temp_file(const std::string& content) {
            static int temp_counter = 0;
            std::string temp_name = "temp_file_" + std::to_string(++temp_counter) + ".tmp";
            
            try {
                auto file = std::make_unique<FileRAII>(temp_name, "w+");
                if (!content.empty()) {
                    file->write(content);
                    file->flush();
                }
                return file;
            } catch (const std::exception& e) {
                std::cout << "Failed to create temp file: " << e.what() << "\n";
                return nullptr;
            }
        }
    }

} // namespace CppVerseHub::Memory