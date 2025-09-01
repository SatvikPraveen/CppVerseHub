/**
 * @file MemoryPools.hpp
 * @brief Advanced memory pool implementations for efficient memory allocation
 * @details File location: src/memory/MemoryPools.hpp
 * 
 * This file implements various memory pool strategies including fixed-size pools,
 * variable-size pools, thread-safe pools, and object pools with specialized
 * allocation patterns for high-performance applications.
 */

#ifndef MEMORY_POOLS_HPP
#define MEMORY_POOLS_HPP

#include <memory>
#include <vector>
#include <list>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <cstddef>
#include <cassert>
#include <iostream>
#include <algorithm>
#include <thread>
#include <chrono>
#include <type_traits>
#include <bitset>

namespace CppVerseHub::Memory {

    /**
     * @class FixedSizePool
     * @brief Memory pool for fixed-size allocations with O(1) allocation/deallocation
     */
    template<size_t BlockSize, size_t PoolSize = 4096>
    class FixedSizePool {
    private:
        static constexpr size_t BlocksPerChunk = PoolSize / BlockSize;
        static_assert(BlocksPerChunk > 0, "Block size too large for pool size");

        struct FreeBlock {
            FreeBlock* next;
        };

        struct MemoryChunk {
            alignas(std::max_align_t) char data[PoolSize];
            std::bitset<BlocksPerChunk> allocation_mask;
            size_t free_count;
            FreeBlock* free_list;
            std::unique_ptr<MemoryChunk> next_chunk;

            MemoryChunk() : free_count(BlocksPerChunk), free_list(nullptr) {
                // Initialize free list
                for (size_t i = 0; i < BlocksPerChunk; ++i) {
                    FreeBlock* block = reinterpret_cast<FreeBlock*>(&data[i * BlockSize]);
                    block->next = (i < BlocksPerChunk - 1) ? 
                                  reinterpret_cast<FreeBlock*>(&data[(i + 1) * BlockSize]) : 
                                  nullptr;
                    if (i == 0) free_list = block;
                }
            }
        };

    public:
        FixedSizePool() : head_chunk_(std::make_unique<MemoryChunk>()), 
                         total_allocated_(0), total_chunks_(1) {
            std::cout << "FixedSizePool: Created pool with block size " << BlockSize 
                      << ", blocks per chunk: " << BlocksPerChunk << "\n";
        }

        ~FixedSizePool() {
            std::cout << "FixedSizePool: Destroyed pool with " << total_chunks_ 
                      << " chunks, " << total_allocated_ << " allocations\n";
        }

        void* allocate() {
            std::lock_guard<std::mutex> lock(mutex_);
            
            // Find chunk with available blocks
            MemoryChunk* chunk = find_available_chunk();
            if (!chunk) {
                // Create new chunk
                auto new_chunk = std::make_unique<MemoryChunk>();
                new_chunk->next_chunk = std::move(head_chunk_);
                head_chunk_ = std::move(new_chunk);
                chunk = head_chunk_.get();
                ++total_chunks_;
            }

            // Allocate from chunk
            assert(chunk->free_list != nullptr);
            void* result = chunk->free_list;
            chunk->free_list = chunk->free_list->next;
            --chunk->free_count;
            
            // Update allocation mask
            size_t block_index = (static_cast<char*>(result) - chunk->data) / BlockSize;
            chunk->allocation_mask.set(block_index);

            ++total_allocated_;
            
            std::cout << "FixedSizePool: Allocated block " << block_index 
                      << " from chunk (free count: " << chunk->free_count << ")\n";
            
            return result;
        }

        void deallocate(void* ptr) {
            if (!ptr) return;

            std::lock_guard<std::mutex> lock(mutex_);
            
            MemoryChunk* chunk = find_chunk_for_ptr(ptr);
            if (!chunk) {
                std::cout << "FixedSizePool: ERROR - Pointer not from this pool!\n";
                return;
            }

            // Add back to free list
            FreeBlock* block = static_cast<FreeBlock*>(ptr);
            block->next = chunk->free_list;
            chunk->free_list = block;
            ++chunk->free_count;

            // Update allocation mask
            size_t block_index = (static_cast<char*>(ptr) - chunk->data) / BlockSize;
            chunk->allocation_mask.reset(block_index);

            --total_allocated_;
            
            std::cout << "FixedSizePool: Deallocated block " << block_index 
                      << " (free count: " << chunk->free_count << ")\n";
        }

        size_t block_size() const { return BlockSize; }
        size_t total_allocated() const { 
            std::lock_guard<std::mutex> lock(mutex_);
            return total_allocated_; 
        }
        size_t total_chunks() const { 
            std::lock_guard<std::mutex> lock(mutex_);
            return total_chunks_; 
        }

        bool is_from_pool(void* ptr) const {
            std::lock_guard<std::mutex> lock(mutex_);
            return find_chunk_for_ptr(ptr) != nullptr;
        }

        void print_statistics() const {
            std::lock_guard<std::mutex> lock(mutex_);
            std::cout << "\n=== FixedSizePool Statistics ===\n";
            std::cout << "Block size: " << BlockSize << " bytes\n";
            std::cout << "Blocks per chunk: " << BlocksPerChunk << "\n";
            std::cout << "Total chunks: " << total_chunks_ << "\n";
            std::cout << "Total allocated: " << total_allocated_ << "\n";
            std::cout << "Memory overhead: " << (total_chunks_ * sizeof(MemoryChunk)) << " bytes\n";
        }

    private:
        mutable std::mutex mutex_;
        std::unique_ptr<MemoryChunk> head_chunk_;
        size_t total_allocated_;
        size_t total_chunks_;

        MemoryChunk* find_available_chunk() {
            MemoryChunk* current = head_chunk_.get();
            while (current) {
                if (current->free_count > 0) {
                    return current;
                }
                current = current->next_chunk.get();
            }
            return nullptr;
        }

        MemoryChunk* find_chunk_for_ptr(void* ptr) const {
            MemoryChunk* current = head_chunk_.get();
            while (current) {
                char* chunk_start = current->data;
                char* chunk_end = chunk_start + PoolSize;
                if (ptr >= chunk_start && ptr < chunk_end) {
                    return current;
                }
                current = current->next_chunk.get();
            }
            return nullptr;
        }
    };

    /**
     * @class VariableSizePool
     * @brief Memory pool for variable-size allocations using segregated free lists
     */
    class VariableSizePool {
    private:
        static constexpr size_t MinBlockSize = 16;
        static constexpr size_t MaxBlockSize = 4096;
        static constexpr size_t NumSizeClasses = 32;
        static constexpr size_t ChunkSize = 64 * 1024; // 64KB chunks

        struct FreeBlock {
            FreeBlock* next;
            size_t size;
        };

        struct MemoryChunk {
            char* data;
            size_t size;
            size_t used;
            std::unique_ptr<MemoryChunk> next;

            MemoryChunk(size_t chunk_size) : size(chunk_size), used(0) {
                data = new char[size];
                std::cout << "VariableSizePool: Created chunk of " << size << " bytes\n";
            }

            ~MemoryChunk() {
                delete[] data;
                std::cout << "VariableSizePool: Destroyed chunk of " << size << " bytes\n";
            }
        };

    public:
        VariableSizePool() : total_allocated_(0), total_chunks_(0) {
            // Initialize free lists
            for (size_t i = 0; i < NumSizeClasses; ++i) {
                free_lists_[i] = nullptr;
            }
            std::cout << "VariableSizePool: Created with " << NumSizeClasses << " size classes\n";
        }

        ~VariableSizePool() {
            std::cout << "VariableSizePool: Destroyed with " << total_chunks_ 
                      << " chunks, " << total_allocated_ << " bytes allocated\n";
        }

        void* allocate(size_t size) {
            if (size == 0) return nullptr;
            if (size > MaxBlockSize) {
                // Fall back to standard allocation for large sizes
                std::cout << "VariableSizePool: Large allocation " << size 
                          << " bytes, using standard allocator\n";
                return std::malloc(size);
            }

            std::lock_guard<std::mutex> lock(mutex_);
            
            size_t aligned_size = align_size(size);
            size_t size_class = get_size_class(aligned_size);

            // Try to allocate from free list
            if (free_lists_[size_class]) {
                FreeBlock* block = free_lists_[size_class];
                free_lists_[size_class] = block->next;
                
                std::cout << "VariableSizePool: Allocated " << aligned_size 
                          << " bytes from free list (class " << size_class << ")\n";
                
                return block;
            }

            // Allocate from chunk
            void* result = allocate_from_chunk(aligned_size);
            if (result) {
                total_allocated_ += aligned_size;
                std::cout << "VariableSizePool: Allocated " << aligned_size 
                          << " bytes from chunk\n";
            }

            return result;
        }

        void deallocate(void* ptr, size_t size) {
            if (!ptr) return;
            
            if (size > MaxBlockSize) {
                std::cout << "VariableSizePool: Large deallocation, using standard deallocator\n";
                std::free(ptr);
                return;
            }

            std::lock_guard<std::mutex> lock(mutex_);
            
            size_t aligned_size = align_size(size);
            size_t size_class = get_size_class(aligned_size);

            // Add to free list
            FreeBlock* block = static_cast<FreeBlock*>(ptr);
            block->next = free_lists_[size_class];
            block->size = aligned_size;
            free_lists_[size_class] = block;

            total_allocated_ -= aligned_size;
            
            std::cout << "VariableSizePool: Deallocated " << aligned_size 
                      << " bytes to free list (class " << size_class << ")\n";
        }

        size_t total_allocated() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return total_allocated_;
        }

        size_t total_chunks() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return total_chunks_;
        }

        void print_statistics() const {
            std::lock_guard<std::mutex> lock(mutex_);
            std::cout << "\n=== VariableSizePool Statistics ===\n";
            std::cout << "Total chunks: " << total_chunks_ << "\n";
            std::cout << "Total allocated: " << total_allocated_ << " bytes\n";
            
            std::cout << "Free lists:\n";
            for (size_t i = 0; i < NumSizeClasses; ++i) {
                size_t count = 0;
                FreeBlock* current = free_lists_[i];
                while (current) {
                    ++count;
                    current = current->next;
                }
                if (count > 0) {
                    std::cout << "  Size class " << i << " (" << size_class_to_size(i) 
                              << " bytes): " << count << " blocks\n";
                }
            }
        }

    private:
        mutable std::mutex mutex_;
        FreeBlock* free_lists_[NumSizeClasses];
        std::unique_ptr<MemoryChunk> head_chunk_;
        size_t total_allocated_;
        size_t total_chunks_;

        size_t align_size(size_t size) {
            return (size + sizeof(void*) - 1) & ~(sizeof(void*) - 1);
        }

        size_t get_size_class(size_t size) {
            // Simple power-of-2 size classes
            size_t class_size = MinBlockSize;
            for (size_t i = 0; i < NumSizeClasses; ++i) {
                if (size <= class_size) {
                    return i;
                }
                class_size *= 2;
                if (class_size > MaxBlockSize) {
                    return NumSizeClasses - 1;
                }
            }
            return NumSizeClasses - 1;
        }

        size_t size_class_to_size(size_t size_class) {
            return MinBlockSize << size_class;
        }

        void* allocate_from_chunk(size_t size) {
            // Find chunk with enough space
            MemoryChunk* chunk = find_chunk_with_space(size);
            if (!chunk) {
                // Create new chunk
                auto new_chunk = std::make_unique<MemoryChunk>(ChunkSize);
                new_chunk->next = std::move(head_chunk_);
                head_chunk_ = std::move(new_chunk);
                chunk = head_chunk_.get();
                ++total_chunks_;
            }

            void* result = chunk->data + chunk->used;
            chunk->used += size;
            return result;
        }

        MemoryChunk* find_chunk_with_space(size_t size) {
            MemoryChunk* current = head_chunk_.get();
            while (current) {
                if (current->size - current->used >= size) {
                    return current;
                }
                current = current->next.get();
            }
            return nullptr;
        }
    };

    /**
     * @class ObjectPool
     * @brief Template-based object pool for specific types
     */
    template<typename T, size_t PoolSize = 100>
    class ObjectPool {
    private:
        struct ObjectSlot {
            alignas(T) char storage[sizeof(T)];
            bool in_use;
            
            ObjectSlot() : in_use(false) {}
        };

    public:
        ObjectPool() : next_free_(0), objects_in_use_(0) {
            std::cout << "ObjectPool<" << typeid(T).name() << ">: Created pool with " 
                      << PoolSize << " slots\n";
        }

        ~ObjectPool() {
            // Destroy any remaining objects
            for (size_t i = 0; i < PoolSize; ++i) {
                if (slots_[i].in_use) {
                    reinterpret_cast<T*>(slots_[i].storage)->~T();
                }
            }
            std::cout << "ObjectPool<" << typeid(T).name() << ">: Destroyed pool\n";
        }

        template<typename... Args>
        T* acquire(Args&&... args) {
            std::lock_guard<std::mutex> lock(mutex_);
            
            // Find free slot
            for (size_t i = 0; i < PoolSize; ++i) {
                size_t index = (next_free_ + i) % PoolSize;
                if (!slots_[index].in_use) {
                    slots_[index].in_use = true;
                    next_free_ = (index + 1) % PoolSize;
                    ++objects_in_use_;
                    
                    // Construct object in place
                    T* object = new (slots_[index].storage) T(std::forward<Args>(args)...);
                    
                    std::cout << "ObjectPool: Acquired object at slot " << index 
                              << " (" << objects_in_use_ << "/" << PoolSize << " in use)\n";
                    
                    return object;
                }
            }
            
            std::cout << "ObjectPool: Pool exhausted, returning nullptr\n";
            return nullptr;
        }

        void release(T* object) {
            if (!object) return;

            std::lock_guard<std::mutex> lock(mutex_);
            
            // Find the slot for this object
            for (size_t i = 0; i < PoolSize; ++i) {
                if (reinterpret_cast<void*>(slots_[i].storage) == object) {
                    if (slots_[i].in_use) {
                        object->~T();
                        slots_[i].in_use = false;
                        --objects_in_use_;
                        
                        std::cout << "ObjectPool: Released object at slot " << i 
                                  << " (" << objects_in_use_ << "/" << PoolSize << " in use)\n";
                    } else {
                        std::cout << "ObjectPool: ERROR - Double release detected!\n";
                    }
                    return;
                }
            }
            
            std::cout << "ObjectPool: ERROR - Object not from this pool!\n";
        }

        size_t objects_in_use() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return objects_in_use_;
        }

        size_t available_objects() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return PoolSize - objects_in_use_;
        }

        bool is_full() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return objects_in_use_ == PoolSize;
        }

        bool is_empty() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return objects_in_use_ == 0;
        }

    private:
        mutable std::mutex mutex_;
        ObjectSlot slots_[PoolSize];
        size_t next_free_;
        size_t objects_in_use_;
    };

    /**
     * @class ThreadSafeMemoryPool
     * @brief Thread-safe memory pool with per-thread caches
     */
    template<size_t BlockSize>
    class ThreadSafeMemoryPool {
    private:
        static constexpr size_t CacheSize = 32;
        
        struct ThreadCache {
            void* blocks[CacheSize];
            size_t count;
            
            ThreadCache() : count(0) {
                for (size_t i = 0; i < CacheSize; ++i) {
                    blocks[i] = nullptr;
                }
            }
        };

    public:
        ThreadSafeMemoryPool() : global_pool_(std::make_unique<FixedSizePool<BlockSize>>()) {
            std::cout << "ThreadSafeMemoryPool: Created with block size " << BlockSize << "\n";
        }

        ~ThreadSafeMemoryPool() {
            // Clean up thread caches
            std::lock_guard<std::mutex> lock(cache_mutex_);
            for (auto& [thread_id, cache] : thread_caches_) {
                for (size_t i = 0; i < cache->count; ++i) {
                    global_pool_->deallocate(cache->blocks[i]);
                }
            }
            std::cout << "ThreadSafeMemoryPool: Destroyed\n";
        }

        void* allocate() {
            ThreadCache* cache = get_thread_cache();
            
            // Try to allocate from thread cache
            if (cache->count > 0) {
                --cache->count;
                void* result = cache->blocks[cache->count];
                cache->blocks[cache->count] = nullptr;
                
                thread_local_allocations_.fetch_add(1);
                return result;
            }

            // Refill cache from global pool
            refill_cache(cache);
            
            if (cache->count > 0) {
                --cache->count;
                void* result = cache->blocks[cache->count];
                cache->blocks[cache->count] = nullptr;
                
                thread_local_allocations_.fetch_add(1);
                return result;
            }

            // Global pool allocation failed
            return nullptr;
        }

        void deallocate(void* ptr) {
            if (!ptr) return;

            ThreadCache* cache = get_thread_cache();
            
            // Add to thread cache if there's space
            if (cache->count < CacheSize) {
                cache->blocks[cache->count] = ptr;
                ++cache->count;
                
                thread_local_deallocations_.fetch_add(1);
                return;
            }

            // Cache is full, return to global pool
            global_pool_->deallocate(ptr);
            thread_local_deallocations_.fetch_add(1);
        }

        size_t get_thread_allocations() const {
            return thread_local_allocations_.load();
        }

        size_t get_thread_deallocations() const {
            return thread_local_deallocations_.load();
        }

        void print_statistics() const {
            global_pool_->print_statistics();
            
            std::lock_guard<std::mutex> lock(cache_mutex_);
            std::cout << "Thread caches: " << thread_caches_.size() << "\n";
            std::cout << "Total thread allocations: " << thread_local_allocations_.load() << "\n";
            std::cout << "Total thread deallocations: " << thread_local_deallocations_.load() << "\n";
        }

    private:
        std::unique_ptr<FixedSizePool<BlockSize>> global_pool_;
        mutable std::mutex cache_mutex_;
        std::unordered_map<std::thread::id, std::unique_ptr<ThreadCache>> thread_caches_;
        
        // Thread-local statistics
        thread_local std::atomic<size_t> thread_local_allocations_{0};
        thread_local std::atomic<size_t> thread_local_deallocations_{0};

        ThreadCache* get_thread_cache() {
            std::thread::id current_id = std::this_thread::get_id();
            
            std::lock_guard<std::mutex> lock(cache_mutex_);
            auto it = thread_caches_.find(current_id);
            if (it == thread_caches_.end()) {
                auto cache = std::make_unique<ThreadCache>();
                ThreadCache* cache_ptr = cache.get();
                thread_caches_[current_id] = std::move(cache);
                return cache_ptr;
            }
            return it->second.get();
        }

        void refill_cache(ThreadCache* cache) {
            // Allocate multiple blocks from global pool
            size_t blocks_to_allocate = std::min(CacheSize - cache->count, CacheSize / 2);
            
            for (size_t i = 0; i < blocks_to_allocate; ++i) {
                void* block = global_pool_->allocate();
                if (block) {
                    cache->blocks[cache->count] = block;
                    ++cache->count;
                } else {
                    break;
                }
            }
        }
    };

    /**
     * @class MemoryPoolDemo
     * @brief Comprehensive demonstration of memory pool implementations
     */
    class MemoryPoolDemo {
    public:
        MemoryPoolDemo() = default;

        void demonstrateFixedSizePool();
        void demonstrateVariableSizePool();
        void demonstrateObjectPool();
        void demonstrateThreadSafePool();
        void demonstratePoolPerformance();
        void demonstrateRealWorldScenario();

        void runAllDemonstrations();

    private:
        // Test objects for demonstrations
        class TestObject {
        public:
            TestObject(int id = 0, const std::string& name = "default") 
                : id_(id), name_(name), data_(100, 'X') {
                std::cout << "TestObject " << id_ << " '" << name_ << "' constructed\n";
            }
            
            ~TestObject() {
                std::cout << "TestObject " << id_ << " '" << name_ << "' destroyed\n";
            }
            
            void process() {
                std::cout << "TestObject " << id_ << " processing...\n";
                // Simulate work
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
            
            int get_id() const { return id_; }
            const std::string& get_name() const { return name_; }
            
        private:
            int id_;
            std::string name_;
            std::vector<char> data_; // Some payload
        };

        void benchmark_allocation_performance();
    };

} // namespace CppVerseHub::Memory

#endif // MEMORY_POOLS_HPP