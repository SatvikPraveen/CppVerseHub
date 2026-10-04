/**
 * @file MemoryPools.hpp
 * @brief Fixed-size block pools, segregated small-object allocation and typed object pools.
 *
 * Demonstrates the memory-pool family of techniques:
 *  - FixedSizePool: a growable pool of equally-sized, equally-aligned blocks threaded on an
 *    intrusive free list. O(1) allocate/deallocate, no per-block header, no fragmentation.
 *  - PoolAllocator<T>: an STL allocator that serves single-object requests (list/map/set
 *    nodes) from a FixedSizePool and falls back to the heap for arrays.
 *  - ThreadSafePool: a mutex-protected FixedSizePool for concurrent use.
 *  - SmallObjectAllocator: size-segregated pools (16-byte classes up to 256 bytes) exposed
 *    as a `std::pmr::memory_resource`, the design used by most general-purpose allocators.
 *  - ObjectPool<T, N>: typed, fixed-capacity pool that constructs objects in place and
 *    hands them out as `std::unique_ptr` with a pool-returning deleter (exception safe).
 *
 * Why: node-based containers and game/simulation entities allocate many small objects of
 * one size; pools make that allocation cheap, cache friendly and predictable.
 */

#ifndef CPPVERSEHUB_MEMORY_MEMORY_POOLS_HPP
#define CPPVERSEHUB_MEMORY_MEMORY_POOLS_HPP

#include "memory/CustomAllocators.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <memory_resource>
#include <mutex>
#include <new>
#include <type_traits>
#include <utility>
#include <vector>

namespace CppVerseHub::Memory {

/**
 * @class FixedSizePool
 * @brief Growable pool of fixed-size blocks with an intrusive free list.
 *
 * Blocks are carved out of chunks obtained from an upstream `std::pmr::memory_resource`.
 * The stride is rounded up so every block satisfies the requested alignment. Not
 * thread-safe (see ThreadSafePool).
 */
class FixedSizePool {
public:
    /**
     * @brief Construct a pool. No memory is acquired until the first allocate().
     * @param block_size Usable bytes per block (> 0).
     * @param block_alignment Power-of-two alignment of every block.
     * @param blocks_per_chunk Number of blocks obtained per upstream request (> 0).
     * @param max_chunks Maximum number of chunks (0 = unlimited).
     * @param upstream Resource providing chunk memory.
     * @throws std::invalid_argument on invalid parameters.
     */
    explicit FixedSizePool(std::size_t block_size, std::size_t block_alignment = alignof(std::max_align_t),
                           std::size_t blocks_per_chunk = 64, std::size_t max_chunks = 0,
                           std::pmr::memory_resource* upstream = std::pmr::get_default_resource());

    FixedSizePool(const FixedSizePool&) = delete;
    FixedSizePool& operator=(const FixedSizePool&) = delete;
    FixedSizePool(FixedSizePool&&) = delete;
    FixedSizePool& operator=(FixedSizePool&&) = delete;

    /** @brief Return all chunks to upstream. Outstanding blocks become invalid. */
    ~FixedSizePool();

    /**
     * @brief Obtain one block.
     * @return Pointer to block_size() bytes aligned to block_alignment().
     * @throws std::bad_alloc if the chunk limit is reached or upstream fails (pool unchanged).
     */
    [[nodiscard]] void* allocate();

    /**
     * @brief Obtain one block without throwing.
     * @return Block pointer, or nullptr if no memory is available.
     */
    [[nodiscard]] void* try_allocate() noexcept;

    /**
     * @brief Return a block to the pool.
     * @param p Pointer obtained from this pool (nullptr is ignored).
     */
    void deallocate(void* p) noexcept;

    /**
     * @brief Check whether @p p lies inside one of this pool's chunks.
     * @param p Pointer to test.
     * @return True if owned by the pool.
     */
    [[nodiscard]] bool owns(const void* p) const noexcept;

    /**
     * @brief Return every chunk to upstream (all outstanding blocks become invalid).
     */
    void release() noexcept;

    /** @brief @return Usable bytes per block as requested. */
    [[nodiscard]] std::size_t block_size() const noexcept { return block_size_; }
    /** @brief @return Distance in bytes between consecutive blocks. */
    [[nodiscard]] std::size_t stride() const noexcept { return stride_; }
    /** @brief @return Alignment guaranteed for every block. */
    [[nodiscard]] std::size_t block_alignment() const noexcept { return alignment_; }
    /** @brief @return Blocks currently handed out. */
    [[nodiscard]] std::size_t blocks_in_use() const noexcept { return in_use_; }
    /** @brief @return Blocks currently owned (free + in use). */
    [[nodiscard]] std::size_t capacity() const noexcept { return chunks_.size() * blocks_per_chunk_; }
    /** @brief @return Number of chunks obtained from upstream. */
    [[nodiscard]] std::size_t chunk_count() const noexcept { return chunks_.size(); }

private:
    struct FreeNode {
        FreeNode* next;
    };

    [[nodiscard]] bool grow() noexcept;

    std::size_t block_size_;
    std::size_t alignment_;
    std::size_t stride_;
    std::size_t blocks_per_chunk_;
    std::size_t max_chunks_;
    std::pmr::memory_resource* upstream_;
    std::vector<std::byte*> chunks_;
    FreeNode* free_list_ = nullptr;
    std::size_t in_use_ = 0;
};

/**
 * @class PoolAllocator
 * @brief STL allocator serving single-object requests from a FixedSizePool.
 *
 * Requests for exactly one object that fits the pool's block size and alignment come
 * from the pool; everything else (e.g. vector buffers, hash bucket arrays) goes to the
 * heap. This makes it a drop-in node allocator for list, map, set and unordered_*.
 *
 * @tparam T Value type.
 */
template <typename T>
class PoolAllocator {
public:
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using propagate_on_container_copy_assignment = std::true_type;
    using propagate_on_container_move_assignment = std::true_type;
    using propagate_on_container_swap = std::true_type;
    using is_always_equal = std::false_type;

    /** @brief Rebind to another value type. */
    template <typename U>
    struct rebind {
        using other = PoolAllocator<U>;
    };

    /**
     * @brief Bind to a pool.
     * @param pool Pool that must outlive every container using this allocator.
     */
    explicit PoolAllocator(FixedSizePool& pool) noexcept : pool_(&pool) {}

    /**
     * @brief Rebinding converting constructor.
     * @param other Allocator for another type sharing the pool.
     */
    template <typename U>
    PoolAllocator(const PoolAllocator<U>& other) noexcept // NOLINT(google-explicit-constructor)
        : pool_(other.pool()) {}

    /**
     * @brief Allocate storage for @p n objects.
     * @param n Number of objects.
     * @return Pointer aligned for T.
     * @throws std::bad_array_new_length on overflow; std::bad_alloc on exhaustion.
     */
    [[nodiscard]] T* allocate(size_type n) {
        if (n > std::numeric_limits<size_type>::max() / sizeof(T)) {
            throw std::bad_array_new_length();
        }
        if (uses_pool(n)) {
            return static_cast<T*>(pool_->allocate());
        }
        return static_cast<T*>(detail::allocate_bytes(n * sizeof(T), alignof(T)));
    }

    /**
     * @brief Release storage from allocate().
     * @param p Pointer from allocate(n).
     * @param n Same @p n passed to allocate().
     */
    void deallocate(T* p, size_type n) noexcept {
        if (uses_pool(n)) {
            pool_->deallocate(p);
        } else {
            detail::deallocate_bytes(p, alignof(T));
        }
    }

    /** @brief @return The bound pool. */
    [[nodiscard]] FixedSizePool* pool() const noexcept { return pool_; }

private:
    [[nodiscard]] bool uses_pool(size_type n) const noexcept {
        return n == 1 && sizeof(T) <= pool_->block_size() && alignof(T) <= pool_->block_alignment();
    }

    FixedSizePool* pool_;
};

/**
 * @brief Equality: allocators are interchangeable iff they share the same pool.
 * @param lhs First allocator.
 * @param rhs Second allocator.
 * @return True if both use the same pool.
 */
template <typename T, typename U>
[[nodiscard]] bool operator==(const PoolAllocator<T>& lhs, const PoolAllocator<U>& rhs) noexcept {
    return lhs.pool() == rhs.pool();
}

static_assert(AllocatorUtils::Allocator<PoolAllocator<long>>);

/**
 * @class ThreadSafePool
 * @brief Mutex-protected FixedSizePool that may be shared between threads.
 */
class ThreadSafePool {
public:
    /**
     * @brief Construct the underlying pool.
     * @param block_size Usable bytes per block.
     * @param block_alignment Power-of-two block alignment.
     * @param blocks_per_chunk Blocks per upstream request.
     */
    explicit ThreadSafePool(std::size_t block_size, std::size_t block_alignment = alignof(std::max_align_t),
                            std::size_t blocks_per_chunk = 64)
        : pool_(block_size, block_alignment, blocks_per_chunk) {}

    /**
     * @brief Obtain a block.
     * @return Block pointer.
     * @throws std::bad_alloc if upstream fails.
     */
    [[nodiscard]] void* allocate() {
        const std::lock_guard lock(mutex_);
        return pool_.allocate();
    }

    /**
     * @brief Return a block.
     * @param p Pointer from allocate().
     */
    void deallocate(void* p) noexcept {
        const std::lock_guard lock(mutex_);
        pool_.deallocate(p);
    }

    /** @brief @return Blocks currently in use. */
    [[nodiscard]] std::size_t blocks_in_use() const {
        const std::lock_guard lock(mutex_);
        return pool_.blocks_in_use();
    }

    /** @brief @return Block capacity currently owned. */
    [[nodiscard]] std::size_t capacity() const {
        const std::lock_guard lock(mutex_);
        return pool_.capacity();
    }

private:
    mutable std::mutex mutex_;
    FixedSizePool pool_;
};

/**
 * @class SmallObjectAllocator
 * @brief Size-segregated pool allocator exposed as a `std::pmr::memory_resource`.
 *
 * Requests up to kMaxSmallSize bytes with alignment <= kGranularity are rounded up to a
 * multiple of kGranularity and served from the matching FixedSizePool; larger or
 * over-aligned requests are forwarded to the upstream resource. Not thread-safe.
 */
class SmallObjectAllocator final : public std::pmr::memory_resource {
public:
    /** @brief Size-class granularity and maximum supported small alignment. */
    static constexpr std::size_t kGranularity = 16;
    /** @brief Largest request served from a pool. */
    static constexpr std::size_t kMaxSmallSize = 256;
    /** @brief Number of size classes. */
    static constexpr std::size_t kClassCount = kMaxSmallSize / kGranularity;

    /**
     * @brief Construct the size-class pools (no memory acquired yet).
     * @param upstream Resource for chunks and large requests.
     */
    explicit SmallObjectAllocator(std::pmr::memory_resource* upstream = std::pmr::get_default_resource());

    SmallObjectAllocator(const SmallObjectAllocator&) = delete;
    SmallObjectAllocator& operator=(const SmallObjectAllocator&) = delete;
    SmallObjectAllocator(SmallObjectAllocator&&) = delete;
    SmallObjectAllocator& operator=(SmallObjectAllocator&&) = delete;
    ~SmallObjectAllocator() override = default;

    /**
     * @brief Map a request size to its size class.
     * @param bytes Request size.
     * @return Class index, or kClassCount if the request is "large".
     */
    [[nodiscard]] static constexpr std::size_t size_class(std::size_t bytes) noexcept {
        if (bytes > kMaxSmallSize) {
            return kClassCount;
        }
        return bytes == 0 ? 0 : (bytes - 1) / kGranularity;
    }

    /** @brief @return Requests served by the size-class pools. */
    [[nodiscard]] std::size_t small_allocations() const noexcept { return small_allocations_; }
    /** @brief @return Requests forwarded upstream. */
    [[nodiscard]] std::size_t large_allocations() const noexcept { return large_allocations_; }
    /**
     * @brief Blocks in use in one size class.
     * @param index Size-class index (< kClassCount).
     * @return Blocks in use.
     */
    [[nodiscard]] std::size_t blocks_in_use(std::size_t index) const noexcept {
        return index < kClassCount ? pools_[index]->blocks_in_use() : 0;
    }
    /** @brief @return Blocks in use across all classes. */
    [[nodiscard]] std::size_t total_blocks_in_use() const noexcept;

private:
    void* do_allocate(std::size_t bytes, std::size_t alignment) override;
    void do_deallocate(void* p, std::size_t bytes, std::size_t alignment) override;
    [[nodiscard]] bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }

    std::pmr::memory_resource* upstream_;
    std::array<std::unique_ptr<FixedSizePool>, kClassCount> pools_;
    std::size_t small_allocations_ = 0;
    std::size_t large_allocations_ = 0;
};

/**
 * @class ObjectPool
 * @brief Fixed-capacity typed pool with in-object storage and RAII handles.
 *
 * acquire() constructs a T in a free slot and returns a `std::unique_ptr` whose deleter
 * destroys the object and returns the slot. If T's constructor throws, the slot is
 * returned before the exception propagates (strong guarantee). Handles must not
 * outlive the pool. Not thread-safe.
 *
 * @tparam T Object type.
 * @tparam Capacity Maximum number of live objects.
 */
template <typename T, std::size_t Capacity>
class ObjectPool {
    static_assert(Capacity > 0, "ObjectPool needs at least one slot");

public:
    /** @brief Deleter that hands the object back to its pool. */
    class Releaser {
    public:
        /** @brief Construct a detached deleter (used by empty handles). */
        Releaser() noexcept = default;
        /**
         * @brief Construct a deleter bound to @p pool.
         * @param pool Owning pool.
         */
        explicit Releaser(ObjectPool* pool) noexcept : pool_(pool) {}
        /**
         * @brief Destroy @p p and free its slot.
         * @param p Object obtained from the pool.
         */
        void operator()(T* p) const noexcept {
            if (pool_ != nullptr && p != nullptr) {
                pool_->destroy(p);
            }
        }

    private:
        ObjectPool* pool_ = nullptr;
    };

    /** @brief Owning handle to a pooled object. */
    using Handle = std::unique_ptr<T, Releaser>;

    /** @brief Construct an empty pool (no objects constructed). */
    ObjectPool() noexcept {
        for (std::size_t i = 0; i < Capacity; ++i) {
            free_[i] = Capacity - 1 - i; // pop from the back: slot 0 first
        }
    }

    ObjectPool(const ObjectPool&) = delete;
    ObjectPool& operator=(const ObjectPool&) = delete;
    ObjectPool(ObjectPool&&) = delete;
    ObjectPool& operator=(ObjectPool&&) = delete;

    /** @brief Destroy any objects still alive (their handles must already be gone). */
    ~ObjectPool() {
        for (T*& p : live_) {
            if (p != nullptr) {
                std::destroy_at(p);
                p = nullptr;
            }
        }
    }

    /**
     * @brief Construct an object in a free slot.
     * @param args Constructor arguments.
     * @return Owning handle.
     * @throws std::bad_alloc if the pool is full; anything T's constructor throws.
     */
    template <typename... Args>
    [[nodiscard]] Handle acquire(Args&&... args) {
        Handle h = try_acquire(std::forward<Args>(args)...);
        if (!h) {
            throw std::bad_alloc();
        }
        return h;
    }

    /**
     * @brief Construct an object if a slot is free.
     * @param args Constructor arguments.
     * @return Owning handle, or an empty handle if the pool is full.
     * @throws Anything T's constructor throws (the slot is returned first).
     */
    template <typename... Args>
    [[nodiscard]] Handle try_acquire(Args&&... args) {
        if (free_count_ == 0) {
            return Handle(nullptr, Releaser(this));
        }
        const std::size_t index = free_[free_count_ - 1];
        // Construct before committing the slot: if this throws, the pool is unchanged.
        T* object = ::new (static_cast<void*>(slots_[index].bytes)) T(std::forward<Args>(args)...);
        --free_count_;
        live_[index] = object;
        return Handle(object, Releaser(this));
    }

    /** @brief @return Number of live objects. */
    [[nodiscard]] std::size_t in_use() const noexcept { return Capacity - free_count_; }
    /** @brief @return Number of free slots. */
    [[nodiscard]] std::size_t available() const noexcept { return free_count_; }
    /** @brief @return Total slot count. */
    [[nodiscard]] static constexpr std::size_t capacity() noexcept { return Capacity; }

    /**
     * @brief Check whether @p p points at one of this pool's slots.
     * @param p Pointer to test.
     * @return True if owned.
     */
    [[nodiscard]] bool owns(const T* p) const noexcept {
        const auto* b = reinterpret_cast<const std::byte*>(p);
        const auto* first = slots_[0].bytes;
        const auto* last = slots_[Capacity - 1].bytes + sizeof(Slot);
        return !std::less<const std::byte*>{}(b, first) && std::less<const std::byte*>{}(b, last);
    }

private:
    struct Slot {
        alignas(T) std::byte bytes[sizeof(T)];
    };

    void destroy(T* p) noexcept {
        // Integer arithmetic: the slots are distinct arrays, so pointer subtraction would be UB.
        const auto offset = reinterpret_cast<std::uintptr_t>(p) -
                            reinterpret_cast<std::uintptr_t>(&slots_[0]);
        const auto index = static_cast<std::size_t>(offset) / sizeof(Slot);
        std::destroy_at(p);
        live_[index] = nullptr;
        free_[free_count_++] = index;
    }

    std::array<Slot, Capacity> slots_{};
    std::array<T*, Capacity> live_{};
    std::array<std::size_t, Capacity> free_{};
    std::size_t free_count_ = Capacity;
};

/**
 * @brief Showcase: fixed-size pools, node allocators, size classes and object pools.
 * @param out Stream receiving the narration.
 */
void demonstrateMemoryPools(std::ostream& out = std::cout);

} // namespace CppVerseHub::Memory

#endif // CPPVERSEHUB_MEMORY_MEMORY_POOLS_HPP
