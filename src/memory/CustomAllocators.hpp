/**
 * @file CustomAllocators.hpp
 * @brief Custom memory allocator implementations for advanced memory management
 * @details File location: src/memory/CustomAllocators.hpp
 * 
 * This file demonstrates various custom allocator implementations including
 * stack allocators, pool allocators, and tracking allocators.
 */

#ifndef CUSTOM_ALLOCATORS_HPP
#define CUSTOM_ALLOCATORS_HPP

#include <memory>
#include <vector>
#include <list>
#include <map>
#include <cstddef>
#include <cassert>
#include <iostream>
#include <type_traits>
#include <algorithm>
#include <chrono>
#include <unordered_map>
#include <mutex>
#include <atomic>

namespace CppVerseHub::Memory {

    /**
     * @class StackAllocator
     * @brief Stack-based allocator for fast allocation/deallocation
     * @details Allocates memory from a pre-allocated stack buffer
     */
    template<size_t Size>
    class StackAllocator {
    public:
        StackAllocator() : top_(0) {}

        void* allocate(size_t size, size_t alignment = alignof(std::max_align_t)) {
            // Align the allocation
            size_t aligned_top = align(top_, alignment);
            
            if (aligned_top + size > Size) {
                throw std::bad_alloc{};
            }
            
            void* result = buffer_ + aligned_top;
            top_ = aligned_top + size;
            
            std::cout << "StackAllocator: Allocated " << size 
                      << " bytes at offset " << aligned_top << "\n";
            
            return result;
        }

        void deallocate(void* ptr, size_t size) noexcept {
            // Stack allocator: can only deallocate in reverse order
            if (static_cast<char*>(ptr) + size == buffer_ + top_) {
                top_ -= size;
                std::cout << "StackAllocator: Deallocated " << size 
                          << " bytes, top now at " << top_ << "\n";
            } else {
                std::cout << "StackAllocator: Cannot deallocate out of order!\n";
            }
        }

        void reset() {
            top_ = 0;
            std::cout << "StackAllocator: Reset to beginning\n";
        }

        size_t bytes_used() const { return top_; }
        size_t bytes_remaining() const { return Size - top_; }

    private:
        alignas(std::max_align_t) char buffer_[Size];
        size_t top_;

        size_t align(size_t n, size_t alignment) {
            return (n + alignment - 1) & ~(alignment - 1);
        }
    };

    /**
     * @class PoolAllocator
     * @brief Fixed-size block allocator for efficient allocation of same-sized objects
     */
    template<size_t BlockSize, size_t BlockCount>
    class PoolAllocator {
    private:
        struct Block {
            Block* next;
        };

    public:
        PoolAllocator() {
            // Initialize the free list
            for (size_t i = 0; i < BlockCount - 1; ++i) {
                reinterpret_cast<Block*>(&buffer_[i * BlockSize])->next =
                    reinterpret_cast<Block*>(&buffer_[(i + 1) * BlockSize]);
            }
            reinterpret_cast<Block*>(&buffer_[(BlockCount - 1) * BlockSize])->next = nullptr;
            
            free_head_ = reinterpret_cast<Block*>(buffer_);
            allocated_count_ = 0;
            
            std::cout << "PoolAllocator: Initialized with " << BlockCount 
                      << " blocks of " << BlockSize << " bytes each\n";
        }

        void* allocate() {
            if (!free_head_) {
                throw std::bad_alloc{};
            }

            void* result = free_head_;
            free_head_ = free_head_->next;
            ++allocated_count_;

            std::cout << "PoolAllocator: Allocated block (" 
                      << allocated_count_ << "/" << BlockCount << " used)\n";

            return result;
        }

        void deallocate(void* ptr) noexcept {
            if (!ptr) return;

            // Add back to free list
            Block* block = static_cast<Block*>(ptr);
            block->next = free_head_;
            free_head_ = block;
            --allocated_count_;

            std::cout << "PoolAllocator: Deallocated block (" 
                      << allocated_count_ << "/" << BlockCount << " used)\n";
        }

        bool is_from_pool(void* ptr) const {
            return ptr >= buffer_ && ptr < buffer_ + (BlockCount * BlockSize);
        }

        size_t allocated_count() const { return allocated_count_; }
        size_t available_count() const { return BlockCount - allocated_count_; }

    private:
        alignas(std::max_align_t) char buffer_[BlockCount * BlockSize];
        Block* free_head_;
        size_t allocated_count_;
    };

    /**
     * @class TrackingAllocator
     * @brief STL-compatible allocator that tracks memory usage
     */
    template<typename T>
    class TrackingAllocator {
    public:
        using value_type = T;
        using pointer = T*;
        using const_pointer = const T*;
        using reference = T&;
        using const_reference = const T&;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;

        template<typename U>
        struct rebind {
            using other = TrackingAllocator<U>;
        };

        TrackingAllocator() noexcept = default;

        template<typename U>
        TrackingAllocator(const TrackingAllocator<U>&) noexcept {}

        pointer allocate(size_type n) {
            if (n > std::numeric_limits<size_type>::max() / sizeof(T)) {
                throw std::bad_alloc{};
            }

            size_t bytes = n * sizeof(T);
            pointer result = static_cast<pointer>(std::malloc(bytes));
            
            if (!result) {
                throw std::bad_alloc{};
            }

            // Track allocation
            {
                std::lock_guard<std::mutex> lock(stats_mutex_);
                allocations_[result] = bytes;
                total_allocated_ += bytes;
                ++allocation_count_;
                peak_usage_ = std::max(peak_usage_, total_allocated_ - total_deallocated_);
            }

            std::cout << "TrackingAllocator: Allocated " << bytes << " bytes for " 
                      << n << " objects of type " << typeid(T).name() << "\n";

            return result;
        }

        void deallocate(pointer ptr, size_type n) noexcept {
            if (!ptr) return;

            size_t bytes = 0;
            {
                std::lock_guard<std::mutex> lock(stats_mutex_);
                auto it = allocations_.find(ptr);
                if (it != allocations_.end()) {
                    bytes = it->second;
                    total_deallocated_ += bytes;
                    allocations_.erase(it);
                    ++deallocation_count_;
                }
            }

            std::free(ptr);

            std::cout << "TrackingAllocator: Deallocated " << bytes << " bytes for " 
                      << n << " objects of type " << typeid(T).name() << "\n";
        }

        // Static methods for statistics
        static void print_statistics() {
            std::lock_guard<std::mutex> lock(stats_mutex_);
            std::cout << "\n=== TrackingAllocator Statistics ===\n";
            std::cout << "Total allocations: " << allocation_count_ << "\n";
            std::cout << "Total deallocations: " << deallocation_count_ << "\n";
            std::cout << "Active allocations: " << allocations_.size() << "\n";
            std::cout << "Total allocated: " << total_allocated_ << " bytes\n";
            std::cout << "Total deallocated: " << total_deallocated_ << " bytes\n";
            std::cout << "Currently allocated: " << (total_allocated_ - total_deallocated_) << " bytes\n";
            std::cout << "Peak usage: " << peak_usage_ << " bytes\n";
        }

        static void reset_statistics() {
            std::lock_guard<std::mutex> lock(stats_mutex_);
            allocations_.clear();
            total_allocated_ = 0;
            total_deallocated_ = 0;
            allocation_count_ = 0;
            deallocation_count_ = 0;
            peak_usage_ = 0;
        }

        // Comparison operators
        template<typename U>
        bool operator==(const TrackingAllocator<U>&) const noexcept {
            return true;
        }

        template<typename U>
        bool operator!=(const TrackingAllocator<U>&) const noexcept {
            return false;
        }

    private:
        static std::mutex stats_mutex_;
        static std::unordered_map<void*, size_t> allocations_;
        static std::atomic<size_t> total_allocated_;
        static std::atomic<size_t> total_deallocated_;
        static std::atomic<size_t> allocation_count_;
        static std::atomic<size_t> deallocation_count_;
        static std::atomic<size_t> peak_usage_;
    };

    // Static member definitions
    template<typename T>
    std::mutex TrackingAllocator<T>::stats_mutex_;

    template<typename T>
    std::unordered_map<void*, size_t> TrackingAllocator<T>::allocations_;

    template<typename T>
    std::atomic<size_t> TrackingAllocator<T>::total_allocated_{0};

    template<typename T>
    std::atomic<size_t> TrackingAllocator<T>::total_deallocated_{0};

    template<typename T>
    std::atomic<size_t> TrackingAllocator<T>::allocation_count_{0};

    template<typename T>
    std::atomic<size_t> TrackingAllocator<T>::deallocation_count_{0};

    template<typename T>
    std::atomic<size_t> TrackingAllocator<T>::peak_usage_{0};

    /**
     * @class MonotonicAllocator
     * @brief Allocator that only grows, never deallocates individual objects
     */
    template<size_t ChunkSize = 4096>
    class MonotonicAllocator {
    private:
        struct Chunk {
            std::unique_ptr<char[]> memory;
            size_t used;
            std::unique_ptr<Chunk> next;

            Chunk() : memory(std::make_unique<char[]>(ChunkSize)), used(0) {}
        };

    public:
        MonotonicAllocator() : current_chunk_(std::make_unique<Chunk>()) {
            std::cout << "MonotonicAllocator: Initialized with chunk size " 
                      << ChunkSize << "\n";
        }

        void* allocate(size_t size, size_t alignment = alignof(std::max_align_t)) {
            size_t aligned_size = align(size, alignment);
            
            // Check if current chunk has enough space
            if (current_chunk_->used + aligned_size > ChunkSize) {
                // Need a new chunk
                auto new_chunk = std::make_unique<Chunk>();
                new_chunk->next = std::move(current_chunk_);
                current_chunk_ = std::move(new_chunk);
                
                std::cout << "MonotonicAllocator: Allocated new chunk\n";
            }

            void* result = current_chunk_->memory.get() + current_chunk_->used;
            current_chunk_->used += aligned_size;
            total_allocated_ += aligned_size;

            std::cout << "MonotonicAllocator: Allocated " << size 
                      << " bytes (aligned to " << aligned_size << ")\n";

            return result;
        }

        // No individual deallocation - only reset all
        void reset() {
            current_chunk_ = std::make_unique<Chunk>();
            total_allocated_ = 0;
            std::cout << "MonotonicAllocator: Reset all allocations\n";
        }

        size_t total_allocated() const { return total_allocated_; }

    private:
        std::unique_ptr<Chunk> current_chunk_;
        size_t total_allocated_ = 0;

        size_t align(size_t n, size_t alignment) {
            return (n + alignment - 1) & ~(alignment - 1);
        }
    };

    /**
     * @class CustomAllocatorDemo
     * @brief Demonstration class for all custom allocators
     */
    class CustomAllocatorDemo {
    public:
        // Type aliases for containers with custom allocators
        using TrackedVector = std::vector<int, TrackingAllocator<int>>;
        using TrackedList = std::list<std::string, TrackingAllocator<std::string>>;
        using TrackedMap = std::map<int, std::string, std::less<int>, 
                                   TrackingAllocator<std::pair<const int, std::string>>>;

        CustomAllocatorDemo() = default;

        void demonstrateStackAllocator();
        void demonstratePoolAllocator();
        void demonstrateTrackingAllocator();
        void demonstrateMonotonicAllocator();
        void demonstrateAllocatorPerformance();
        void demonstrateSTLContainersWithCustomAllocators();
        
        void runAllDemonstrations();

    private:
        void benchmark_allocation_performance();
    };

    /**
     * @class AllocatorBenchmark
     * @brief Performance benchmarking for different allocator types
     */
    class AllocatorBenchmark {
    public:
        struct AllocationPattern {
            std::vector<size_t> sizes;
            std::vector<bool> deallocate_immediately;
        };

        struct BenchmarkResult {
            std::chrono::nanoseconds total_time;
            std::chrono::nanoseconds avg_allocation_time;
            std::chrono::nanoseconds avg_deallocation_time;
            size_t total_memory;
            size_t fragmentation_waste;
        };

        static BenchmarkResult benchmark_standard_allocator(
            const AllocationPattern& pattern, size_t iterations);
        
        static BenchmarkResult benchmark_pool_allocator(
            const AllocationPattern& pattern, size_t iterations);
        
        static BenchmarkResult benchmark_stack_allocator(
            const AllocationPattern& pattern, size_t iterations);

        static void compare_allocators(size_t iterations = 1000);
        
        static AllocationPattern create_random_pattern(size_t count);
        static AllocationPattern create_sequential_pattern(size_t count, size_t size);
        static AllocationPattern create_mixed_pattern(size_t count);
    };

    /**
     * @brief Utility functions for allocator management
     */
    namespace AllocatorUtils {
        
        /**
         * @brief Calculate memory fragmentation
         */
        double calculate_fragmentation(const std::vector<void*>& allocations,
                                     const std::vector<size_t>& sizes);

        /**
         * @brief Align memory address
         */
        template<typename T>
        constexpr T align_up(T value, size_t alignment) {
            return (value + alignment - 1) & ~(alignment - 1);
        }

        /**
         * @brief Check if pointer is aligned
         */
        template<typename T>
        constexpr bool is_aligned(T* ptr, size_t alignment) {
            return (reinterpret_cast<uintptr_t>(ptr) % alignment) == 0;
        }

        /**
         * @brief Memory usage tracker
         */
        class MemoryTracker {
        public:
            static MemoryTracker& instance() {
                static MemoryTracker tracker;
                return tracker;
            }

            void record_allocation(void* ptr, size_t size);
            void record_deallocation(void* ptr);
            void print_report() const;
            void reset();

        private:
            MemoryTracker() = default;
            
            mutable std::mutex mutex_;
            std::unordered_map<void*, size_t> active_allocations_;
            size_t total_allocated_ = 0;
            size_t total_deallocated_ = 0;
            size_t peak_usage_ = 0;
            size_t allocation_count_ = 0;
            size_t deallocation_count_ = 0;
        };
    }

    /**
     * @class SmallObjectAllocator
     * @brief Specialized allocator for small objects using multiple pools
     */
    template<size_t MaxObjectSize = 256, size_t PoolSize = 4096>
    class SmallObjectAllocator {
    private:
        static constexpr size_t NumPools = MaxObjectSize / sizeof(void*);
        
        struct Pool {
            char* memory;
            void* free_list;
            size_t block_size;
            size_t blocks_per_pool;
            
            Pool(size_t size) : block_size(size) {
                blocks_per_pool = PoolSize / block_size;
                memory = new char[PoolSize];
                
                // Initialize free list
                free_list = memory;
                for (size_t i = 0; i < blocks_per_pool - 1; ++i) {
                    void** current = reinterpret_cast<void**>(memory + i * block_size);
                    *current = memory + (i + 1) * block_size;
                }
                void** last = reinterpret_cast<void**>(memory + (blocks_per_pool - 1) * block_size);
                *last = nullptr;
            }
            
            ~Pool() {
                delete[] memory;
            }
        };

    public:
        SmallObjectAllocator() {
            for (size_t i = 0; i < NumPools; ++i) {
                size_t block_size = (i + 1) * sizeof(void*);
                pools_[i] = std::make_unique<Pool>(block_size);
            }
        }

        void* allocate(size_t size) {
            if (size > MaxObjectSize) {
                return std::malloc(size); // Fall back to standard allocator
            }

            size_t pool_index = (size - 1) / sizeof(void*);
            Pool& pool = *pools_[pool_index];

            if (!pool.free_list) {
                throw std::bad_alloc{};
            }

            void* result = pool.free_list;
            pool.free_list = *static_cast<void**>(pool.free_list);

            return result;
        }

        void deallocate(void* ptr, size_t size) {
            if (size > MaxObjectSize) {
                std::free(ptr);
                return;
            }

            size_t pool_index = (size - 1) / sizeof(void*);
            Pool& pool = *pools_[pool_index];

            *static_cast<void**>(ptr) = pool.free_list;
            pool.free_list = ptr;
        }

    private:
        std::unique_ptr<Pool> pools_[NumPools];
    };

} // namespace CppVerseHub::Memory

#endif // CUSTOM_ALLOCATORS_HPP