/**
 * @file CustomAllocators.cpp
 * @brief Implementation of custom allocator demonstrations
 * @details File location: src/memory/CustomAllocators.cpp
 */

#include "CustomAllocators.hpp"
#include <random>
#include <algorithm>
#include <numeric>
#include <iomanip>

namespace CppVerseHub::Memory {

    void CustomAllocatorDemo::demonstrateStackAllocator() {
        std::cout << "\n=== Stack Allocator Demonstration ===\n";
        
        StackAllocator<1024> stack_alloc;
        
        std::cout << "Initial state - Used: " << stack_alloc.bytes_used() 
                  << ", Remaining: " << stack_alloc.bytes_remaining() << "\n";
        
        // Allocate some memory
        void* ptr1 = stack_alloc.allocate(64);
        void* ptr2 = stack_alloc.allocate(128);
        void* ptr3 = stack_alloc.allocate(256);
        
        std::cout << "After allocations - Used: " << stack_alloc.bytes_used() 
                  << ", Remaining: " << stack_alloc.bytes_remaining() << "\n";
        
        // Deallocate in reverse order (LIFO)
        stack_alloc.deallocate(ptr3, 256);
        stack_alloc.deallocate(ptr2, 128);
        stack_alloc.deallocate(ptr1, 64);
        
        std::cout << "After deallocations - Used: " << stack_alloc.bytes_used() 
                  << ", Remaining: " << stack_alloc.bytes_remaining() << "\n";
        
        // Try to allocate too much
        try {
            void* large_ptr = stack_alloc.allocate(2048);
            (void)large_ptr; // Suppress unused variable warning
        } catch (const std::bad_alloc& e) {
            std::cout << "Expected exception caught: allocation too large\n";
        }
        
        // Reset allocator
        stack_alloc.reset();
        std::cout << "After reset - Used: " << stack_alloc.bytes_used() << "\n";
    }

    void CustomAllocatorDemo::demonstratePoolAllocator() {
        std::cout << "\n=== Pool Allocator Demonstration ===\n";
        
        PoolAllocator<64, 10> pool_alloc;
        
        // Allocate several blocks
        std::vector<void*> allocated_blocks;
        
        for (int i = 0; i < 5; ++i) {
            void* block = pool_alloc.allocate();
            allocated_blocks.push_back(block);
            
            // Use the block (write some data)
            *static_cast<int*>(block) = i * 42;
        }
        
        std::cout << "Allocated blocks: " << pool_alloc.allocated_count() 
                  << ", Available: " << pool_alloc.available_count() << "\n";
        
        // Verify data integrity
        for (size_t i = 0; i < allocated_blocks.size(); ++i) {
            int value = *static_cast<int*>(allocated_blocks[i]);
            std::cout << "Block " << i << " contains: " << value << "\n";
        }
        
        // Deallocate some blocks
        for (size_t i = 0; i < 3; ++i) {
            pool_alloc.deallocate(allocated_blocks[i]);
        }
        allocated_blocks.erase(allocated_blocks.begin(), allocated_blocks.begin() + 3);
        
        std::cout << "After partial deallocation - Allocated: " << pool_alloc.allocated_count() 
                  << ", Available: " << pool_alloc.available_count() << "\n";
        
        // Allocate more blocks (should reuse deallocated ones)
        for (int i = 0; i < 3; ++i) {
            void* block = pool_alloc.allocate();
            allocated_blocks.push_back(block);
        }
        
        // Try to exceed pool capacity
        try {
            for (int i = 0; i < 10; ++i) {
                void* block = pool_alloc.allocate();
                allocated_blocks.push_back(block);
            }
        } catch (const std::bad_alloc& e) {
            std::cout << "Pool exhausted, exception caught as expected\n";
        }
        
        // Clean up remaining blocks
        for (void* block : allocated_blocks) {
            pool_alloc.deallocate(block);
        }
        
        std::cout << "Final state - Allocated: " << pool_alloc.allocated_count() << "\n";
    }

    void CustomAllocatorDemo::demonstrateTrackingAllocator() {
        std::cout << "\n=== Tracking Allocator Demonstration ===\n";
        
        TrackingAllocator<int>::reset_statistics();
        
        // Create containers with tracking allocator
        {
            TrackedVector vec;
            vec.reserve(100);
            
            for (int i = 0; i < 50; ++i) {
                vec.push_back(i * i);
            }
            
            TrackedList list;
            for (int i = 0; i < 20; ++i) {
                list.push_back("String " + std::to_string(i));
            }
            
            TrackedMap map;
            for (int i = 0; i < 30; ++i) {
                map[i] = "Value " + std::to_string(i * 2);
            }
            
            std::cout << "Containers created and populated\n";
            TrackingAllocator<int>::print_statistics();
            
        } // Containers go out of scope here
        
        std::cout << "\nAfter containers destruction:\n";
        TrackingAllocator<int>::print_statistics();
    }

    void CustomAllocatorDemo::demonstrateMonotonicAllocator() {
        std::cout << "\n=== Monotonic Allocator Demonstration ===\n";
        
        MonotonicAllocator<1024> mono_alloc;
        
        // Allocate various sizes
        std::vector<void*> allocations;
        std::vector<size_t> sizes = {32, 64, 128, 256, 512, 100, 200};
        
        for (size_t size : sizes) {
            void* ptr = mono_alloc.allocate(size);
            allocations.push_back(ptr);
            std::cout << "Total allocated so far: " << mono_alloc.total_allocated() << " bytes\n";
        }
        
        // Try to allocate something that requires a new chunk
        void* large_ptr = mono_alloc.allocate(800);
        allocations.push_back(large_ptr);
        
        std::cout << "After large allocation: " << mono_alloc.total_allocated() << " bytes\n";
        
        // No individual deallocation - only reset
        mono_alloc.reset();
        std::cout << "After reset: " << mono_alloc.total_allocated() << " bytes\n";
        
        // Can allocate again after reset
        void* new_ptr = mono_alloc.allocate(256);
        (void)new_ptr;
        std::cout << "After new allocation: " << mono_alloc.total_allocated() << " bytes\n";
    }

    void CustomAllocatorDemo::demonstrateAllocatorPerformance() {
        std::cout << "\n=== Allocator Performance Demonstration ===\n";
        
        AllocatorBenchmark::compare_allocators(1000);
    }

    void CustomAllocatorDemo::demonstrateSTLContainersWithCustomAllocators() {
        std::cout << "\n=== STL Containers with Custom Allocators ===\n";
        
        // Reset tracking statistics
        TrackingAllocator<int>::reset_statistics();
        
        {
            std::cout << "Creating vector with tracking allocator...\n";
            std::vector<int, TrackingAllocator<int>> tracked_vector;
            
            // Demonstrate growth patterns
            for (int i = 0; i < 1000; ++i) {
                tracked_vector.push_back(i);
                if (i % 100 == 99) {
                    std::cout << "Vector size: " << tracked_vector.size() 
                              << ", capacity: " << tracked_vector.capacity() << "\n";
                }
            }
            
            std::cout << "Creating list with tracking allocator...\n";
            std::list<std::string, TrackingAllocator<std::string>> tracked_list;
            
            for (int i = 0; i < 100; ++i) {
                tracked_list.push_back("Element " + std::to_string(i));
            }
            
            std::cout << "List size: " << tracked_list.size() << "\n";
            
            std::cout << "Creating map with tracking allocator...\n";
            std::map<int, std::string, std::less<int>, 
                     TrackingAllocator<std::pair<const int, std::string>>> tracked_map;
            
            for (int i = 0; i < 50; ++i) {
                tracked_map[i] = "Value " + std::to_string(i * i);
            }
            
            std::cout << "Map size: " << tracked_map.size() << "\n";
            
            // Print statistics while containers are alive
            TrackingAllocator<int>::print_statistics();
        }
        
        std::cout << "\nAfter all containers destroyed:\n";
        TrackingAllocator<int>::print_statistics();
    }

    void CustomAllocatorDemo::runAllDemonstrations() {
        std::cout << "\n========== CUSTOM ALLOCATOR COMPREHENSIVE DEMO ==========\n";
        
        demonstrateStackAllocator();
        demonstratePoolAllocator();
        demonstrateTrackingAllocator();
        demonstrateMonotonicAllocator();
        demonstrateSTLContainersWithCustomAllocators();
        demonstrateAllocatorPerformance();
        
        std::cout << "\n========== CUSTOM ALLOCATOR DEMO COMPLETE ==========\n";
    }

    void CustomAllocatorDemo::benchmark_allocation_performance() {
        std::cout << "\n=== Internal Allocation Performance Benchmark ===\n";
        
        constexpr size_t iterations = 10000;
        constexpr size_t allocation_size = 64;
        
        // Benchmark standard allocator
        auto start = std::chrono::high_resolution_clock::now();
        std::vector<void*> standard_ptrs;
        standard_ptrs.reserve(iterations);
        
        for (size_t i = 0; i < iterations; ++i) {
            standard_ptrs.push_back(std::malloc(allocation_size));
        }
        
        for (void* ptr : standard_ptrs) {
            std::free(ptr);
        }
        
        auto standard_time = std::chrono::high_resolution_clock::now() - start;
        
        // Benchmark pool allocator
        start = std::chrono::high_resolution_clock::now();
        PoolAllocator<allocation_size, iterations> pool_alloc;
        std::vector<void*> pool_ptrs;
        pool_ptrs.reserve(iterations);
        
        for (size_t i = 0; i < iterations; ++i) {
            pool_ptrs.push_back(pool_alloc.allocate());
        }
        
        for (void* ptr : pool_ptrs) {
            pool_alloc.deallocate(ptr);
        }
        
        auto pool_time = std::chrono::high_resolution_clock::now() - start;
        
        std::cout << "Standard allocator: " 
                  << std::chrono::duration_cast<std::chrono::microseconds>(standard_time).count() 
                  << " microseconds\n";
        std::cout << "Pool allocator: " 
                  << std::chrono::duration_cast<std::chrono::microseconds>(pool_time).count() 
                  << " microseconds\n";
        
        double speedup = static_cast<double>(standard_time.count()) / pool_time.count();
        std::cout << "Pool allocator speedup: " << std::fixed << std::setprecision(2) 
                  << speedup << "x\n";
    }

    // AllocatorBenchmark implementations
    AllocatorBenchmark::BenchmarkResult 
    AllocatorBenchmark::benchmark_standard_allocator(const AllocationPattern& pattern, size_t iterations) {
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<void*> allocations;
        size_t total_memory = 0;
        auto alloc_start = std::chrono::high_resolution_clock::now();
        
        for (size_t iter = 0; iter < iterations; ++iter) {
            allocations.clear();
            
            for (size_t i = 0; i < pattern.sizes.size(); ++i) {
                void* ptr = std::malloc(pattern.sizes[i]);
                allocations.push_back(ptr);
                total_memory += pattern.sizes[i];
                
                if (pattern.deallocate_immediately[i]) {
                    std::free(ptr);
                    allocations.back() = nullptr;
                }
            }
            
            // Deallocate remaining
            for (void* ptr : allocations) {
                if (ptr) std::free(ptr);
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto total_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        
        return {
            total_time,
            total_time / (iterations * pattern.sizes.size()),
            total_time / (iterations * pattern.sizes.size()),
            total_memory,
            0 // Standard allocator fragmentation is managed by OS
        };
    }

    AllocatorBenchmark::BenchmarkResult 
    AllocatorBenchmark::benchmark_pool_allocator(const AllocationPattern& pattern, size_t iterations) {
        // Simplified pool allocator benchmark
        auto start = std::chrono::high_resolution_clock::now();
        
        PoolAllocator<64, 1000> pool_alloc;
        std::vector<void*> allocations;
        size_t total_memory = 0;
        
        for (size_t iter = 0; iter < iterations; ++iter) {
            allocations.clear();
            
            for (size_t i = 0; i < pattern.sizes.size() && i < 1000; ++i) {
                if (pattern.sizes[i] <= 64) { // Pool can only handle fixed size
                    try {
                        void* ptr = pool_alloc.allocate();
                        allocations.push_back(ptr);
                        total_memory += 64;
                        
                        if (pattern.deallocate_immediately[i]) {
                            pool_alloc.deallocate(ptr);
                            allocations.back() = nullptr;
                        }
                    } catch (const std::bad_alloc&) {
                        break; // Pool exhausted
                    }
                }
            }
            
            // Deallocate remaining
            for (void* ptr : allocations) {
                if (ptr) pool_alloc.deallocate(ptr);
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto total_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        
        return {
            total_time,
            total_time / (iterations * std::min(pattern.sizes.size(), size_t(1000))),
            total_time / (iterations * std::min(pattern.sizes.size(), size_t(1000))),
            total_memory,
            0
        };
    }

    AllocatorBenchmark::BenchmarkResult 
    AllocatorBenchmark::benchmark_stack_allocator(const AllocationPattern& pattern, size_t iterations) {
        auto start = std::chrono::high_resolution_clock::now();
        
        StackAllocator<65536> stack_alloc;
        size_t total_memory = 0;
        
        for (size_t iter = 0; iter < iterations; ++iter) {
            stack_alloc.reset();
            
            for (size_t i = 0; i < pattern.sizes.size(); ++i) {
                try {
                    void* ptr = stack_alloc.allocate(pattern.sizes[i]);
                    total_memory += pattern.sizes[i];
                    
                    // Stack allocator can only deallocate in LIFO order
                    // So we simulate this constraint
                    (void)ptr;
                } catch (const std::bad_alloc&) {
                    break; // Stack full
                }
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto total_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        
        return {
            total_time,
            total_time / (iterations * pattern.sizes.size()),
            std::chrono::nanoseconds(0), // No individual deallocation
            total_memory,
            0
        };
    }

    void AllocatorBenchmark::compare_allocators(size_t iterations) {
        std::cout << "\n=== Allocator Performance Comparison ===\n";
        
        auto pattern = create_mixed_pattern(100);
        
        std::cout << "Running benchmarks with " << iterations << " iterations...\n";
        
        auto standard_result = benchmark_standard_allocator(pattern, iterations);
        auto pool_result = benchmark_pool_allocator(pattern, iterations);
        auto stack_result = benchmark_stack_allocator(pattern, iterations);
        
        std::cout << "\nResults:\n";
        std::cout << std::setw(15) << "Allocator" 
                  << std::setw(15) << "Total Time (ms)" 
                  << std::setw(20) << "Avg Alloc (ns)" 
                  << std::setw(15) << "Memory (KB)" << "\n";
        
        std::cout << std::setw(15) << "Standard" 
                  << std::setw(15) << std::chrono::duration_cast<std::chrono::milliseconds>(standard_result.total_time).count()
                  << std::setw(20) << standard_result.avg_allocation_time.count()
                  << std::setw(15) << (standard_result.total_memory / 1024) << "\n";
        
        std::cout << std::setw(15) << "Pool" 
                  << std::setw(15) << std::chrono::duration_cast<std::chrono::milliseconds>(pool_result.total_time).count()
                  << std::setw(20) << pool_result.avg_allocation_time.count()
                  << std::setw(15) << (pool_result.total_memory / 1024) << "\n";
        
        std::cout << std::setw(15) << "Stack" 
                  << std::setw(15) << std::chrono::duration_cast<std::chrono::milliseconds>(stack_result.total_time).count()
                  << std::setw(20) << stack_result.avg_allocation_time.count()
                  << std::setw(15) << (stack_result.total_memory / 1024) << "\n";
    }

    AllocatorBenchmark::AllocationPattern 
    AllocatorBenchmark::create_random_pattern(size_t count) {
        AllocationPattern pattern;
        pattern.sizes.reserve(count);
        pattern.deallocate_immediately.reserve(count);
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<size_t> size_dist(16, 512);
        std::uniform_int_distribution<int> dealloc_dist(0, 1);
        
        for (size_t i = 0; i < count; ++i) {
            pattern.sizes.push_back(size_dist(gen));
            pattern.deallocate_immediately.push_back(dealloc_dist(gen) == 1);
        }
        
        return pattern;
    }

    AllocatorBenchmark::AllocationPattern 
    AllocatorBenchmark::create_sequential_pattern(size_t count, size_t size) {
        AllocationPattern pattern;
        pattern.sizes.assign(count, size);
        pattern.deallocate_immediately.assign(count, false);
        return pattern;
    }

    AllocatorBenchmark::AllocationPattern 
    AllocatorBenchmark::create_mixed_pattern(size_t count) {
        AllocationPattern pattern;
        pattern.sizes.reserve(count);
        pattern.deallocate_immediately.reserve(count);
        
        // Mix of small, medium, and large allocations
        std::vector<size_t> sizes = {16, 32, 64, 128, 256, 512};
        
        for (size_t i = 0; i < count; ++i) {
            pattern.sizes.push_back(sizes[i % sizes.size()]);
            pattern.deallocate_immediately.push_back(i % 3 == 0); // Every 3rd allocation
        }
        
        return pattern;
    }

    // AllocatorUtils implementations
    namespace AllocatorUtils {
        
        double calculate_fragmentation(const std::vector<void*>& allocations,
                                     const std::vector<size_t>& sizes) {
            if (allocations.size() != sizes.size() || allocations.empty()) {
                return 0.0;
            }
            
            // Simple fragmentation calculation based on address gaps
            std::vector<std::pair<void*, size_t>> sorted_allocs;
            for (size_t i = 0; i < allocations.size(); ++i) {
                sorted_allocs.emplace_back(allocations[i], sizes[i]);
            }
            
            std::sort(sorted_allocs.begin(), sorted_allocs.end());
            
            size_t total_allocated = std::accumulate(sizes.begin(), sizes.end(), 0ULL);
            size_t address_span = 0;
            
            if (sorted_allocs.size() > 1) {
                char* first = static_cast<char*>(sorted_allocs.front().first);
                char* last = static_cast<char*>(sorted_allocs.back().first) + sorted_allocs.back().second;
                address_span = last - first;
            }
            
            return address_span > 0 ? 1.0 - (static_cast<double>(total_allocated) / address_span) : 0.0;
        }
        
        void MemoryTracker::record_allocation(void* ptr, size_t size) {
            std::lock_guard<std::mutex> lock(mutex_);
            active_allocations_[ptr] = size;
            total_allocated_ += size;
            ++allocation_count_;
            
            size_t current_usage = total_allocated_ - total_deallocated_;
            peak_usage_ = std::max(peak_usage_, current_usage);
        }
        
        void MemoryTracker::record_deallocation(void* ptr) {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = active_allocations_.find(ptr);
            if (it != active_allocations_.end()) {
                total_deallocated_ += it->second;
                active_allocations_.erase(it);
                ++deallocation_count_;
            }
        }
        
        void MemoryTracker::print_report() const {
            std::lock_guard<std::mutex> lock(mutex_);
            
            std::cout << "\n=== Memory Tracker Report ===\n";
            std::cout << "Total allocations: " << allocation_count_ << "\n";
            std::cout << "Total deallocations: " << deallocation_count_ << "\n";
            std::cout << "Active allocations: " << active_allocations_.size() << "\n";
            std::cout << "Total allocated: " << total_allocated_ << " bytes\n";
            std::cout << "Total deallocated: " << total_deallocated_ << " bytes\n";
            std::cout << "Currently allocated: " << (total_allocated_ - total_deallocated_) << " bytes\n";
            std::cout << "Peak usage: " << peak_usage_ << " bytes\n";
            
            if (!active_allocations_.empty()) {
                std::cout << "Memory leaks detected:\n";
                for (const auto& [ptr, size] : active_allocations_) {
                    std::cout << "  " << ptr << ": " << size << " bytes\n";
                }
            }
        }
        
        void MemoryTracker::reset() {
            std::lock_guard<std::mutex> lock(mutex_);
            active_allocations_.clear();
            total_allocated_ = 0;
            total_deallocated_ = 0;
            peak_usage_ = 0;
            allocation_count_ = 0;
            deallocation_count_ = 0;
        }
    }

} // namespace CppVerseHub::Memory