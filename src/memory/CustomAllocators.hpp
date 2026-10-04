/**
 * @file CustomAllocators.hpp
 * @brief Standard-conforming custom allocators and memory resources.
 *
 * Demonstrates how to plug custom memory strategies into the standard library:
 *  - AllocatorUtils: alignment arithmetic and an `Allocator` concept that checks the
 *    minimal standard Allocator requirements at compile time.
 *  - AllocationStats / TrackingAllocator: a stateful, rebindable STL allocator that counts
 *    allocations and bytes so tests can prove containers release everything (leak detection).
 *  - TrackingMemoryResource: the same idea for `std::pmr`, wrapping an upstream resource.
 *  - StackAllocator: a fixed in-object LIFO arena with markers (rewind to a checkpoint).
 *  - MonotonicArena / ArenaAllocator: a chunked bump-pointer arena that is both a
 *    `std::pmr::memory_resource` and the backing store of a classic STL allocator.
 *
 * Why: allocation is often the dominant cost of container-heavy code. Arenas turn many
 * small heap allocations into pointer bumps, and tracking allocators make memory behaviour
 * observable. Every allocator here honours alignment (via `std::align` or aligned
 * `operator new`) and provides the strong exception guarantee on allocation failure.
 */

#ifndef CPPVERSEHUB_MEMORY_CUSTOM_ALLOCATORS_HPP
#define CPPVERSEHUB_MEMORY_CUSTOM_ALLOCATORS_HPP

#include <atomic>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <memory_resource>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace CppVerseHub::Memory {

/**
 * @brief Alignment helpers and allocator concepts.
 */
namespace AllocatorUtils {

/**
 * @brief Check whether a value is a (non-zero) power of two.
 * @param value Value to test.
 * @return True if exactly one bit is set.
 */
[[nodiscard]] constexpr bool is_power_of_two(std::size_t value) noexcept {
    return value != 0 && (value & (value - 1)) == 0;
}

/**
 * @brief Round @p value up to the next multiple of @p alignment.
 * @param value Value to round.
 * @param alignment Power-of-two alignment.
 * @return Smallest multiple of @p alignment that is >= @p value.
 */
[[nodiscard]] constexpr std::size_t align_up(std::size_t value, std::size_t alignment) noexcept {
    return (value + alignment - 1) & ~(alignment - 1);
}

/**
 * @brief Check whether a pointer is aligned to @p alignment.
 * @param ptr Pointer to test.
 * @param alignment Power-of-two alignment.
 * @return True if the address is a multiple of @p alignment.
 */
[[nodiscard]] inline bool is_aligned(const void* ptr, std::size_t alignment) noexcept {
    return reinterpret_cast<std::uintptr_t>(ptr) % alignment == 0;
}

/**
 * @brief Minimal standard Allocator requirements (value_type, allocate, deallocate,
 *        rebinding conversion and equality).
 */
template <typename A>
concept Allocator = requires(A a, typename A::value_type* p, std::size_t n) {
    typename A::value_type;
    { a.allocate(n) } -> std::same_as<typename A::value_type*>;
    { a.deallocate(p, n) };
    { a == a } -> std::convertible_to<bool>;
    { a != a } -> std::convertible_to<bool>;
    requires std::copy_constructible<A>;
    requires std::is_nothrow_copy_constructible_v<A>;
};

} // namespace AllocatorUtils

namespace detail {

/**
 * @brief Allocate raw bytes from the global heap honouring an arbitrary alignment.
 * @param bytes Number of bytes.
 * @param alignment Power-of-two alignment.
 * @return Pointer to at least @p bytes of suitably aligned storage.
 */
[[nodiscard]] inline void* allocate_bytes(std::size_t bytes, std::size_t alignment) {
    if (alignment > __STDCPP_DEFAULT_NEW_ALIGNMENT__) {
        return ::operator new(bytes, std::align_val_t{alignment});
    }
    return ::operator new(bytes);
}

/**
 * @brief Release storage obtained from allocate_bytes().
 * @param ptr Pointer previously returned by allocate_bytes().
 * @param alignment Same alignment that was passed to allocate_bytes().
 */
inline void deallocate_bytes(void* ptr, std::size_t alignment) noexcept {
    if (alignment > __STDCPP_DEFAULT_NEW_ALIGNMENT__) {
        ::operator delete(ptr, std::align_val_t{alignment});
    } else {
        ::operator delete(ptr);
    }
}

} // namespace detail

/**
 * @class AllocationStats
 * @brief Thread-safe counters describing allocation activity.
 *
 * Shared by TrackingAllocator instances (and TrackingMemoryResource). All counters are
 * atomics, so allocators may be used from several threads at once.
 */
class AllocationStats {
public:
    /** @brief Construct zeroed counters. */
    AllocationStats() noexcept = default;
    AllocationStats(const AllocationStats&) = delete;
    AllocationStats& operator=(const AllocationStats&) = delete;
    AllocationStats(AllocationStats&&) = delete;
    AllocationStats& operator=(AllocationStats&&) = delete;
    ~AllocationStats() = default;

    /**
     * @brief Record a successful allocation.
     * @param bytes Size of the allocation.
     */
    void record_allocation(std::size_t bytes) noexcept {
        allocations_.fetch_add(1, std::memory_order_relaxed);
        bytes_allocated_.fetch_add(bytes, std::memory_order_relaxed);
        const std::size_t now = in_use_.fetch_add(bytes, std::memory_order_relaxed) + bytes;
        std::size_t peak = peak_.load(std::memory_order_relaxed);
        while (now > peak && !peak_.compare_exchange_weak(peak, now, std::memory_order_relaxed)) {}
    }

    /**
     * @brief Record a deallocation.
     * @param bytes Size of the released block.
     */
    void record_deallocation(std::size_t bytes) noexcept {
        deallocations_.fetch_add(1, std::memory_order_relaxed);
        bytes_deallocated_.fetch_add(bytes, std::memory_order_relaxed);
        in_use_.fetch_sub(bytes, std::memory_order_relaxed);
    }

    /** @brief @return Number of allocate calls. */
    [[nodiscard]] std::size_t allocations() const noexcept { return allocations_.load(); }
    /** @brief @return Number of deallocate calls. */
    [[nodiscard]] std::size_t deallocations() const noexcept { return deallocations_.load(); }
    /** @brief @return Total bytes ever allocated. */
    [[nodiscard]] std::size_t bytes_allocated() const noexcept { return bytes_allocated_.load(); }
    /** @brief @return Total bytes ever deallocated. */
    [[nodiscard]] std::size_t bytes_deallocated() const noexcept { return bytes_deallocated_.load(); }
    /** @brief @return Bytes currently allocated and not yet released. */
    [[nodiscard]] std::size_t bytes_in_use() const noexcept { return in_use_.load(); }
    /** @brief @return Highest value bytes_in_use() has reached. */
    [[nodiscard]] std::size_t peak_bytes() const noexcept { return peak_.load(); }
    /** @brief @return Number of allocations not yet matched by a deallocation. */
    [[nodiscard]] std::size_t outstanding() const noexcept { return allocations() - deallocations(); }
    /** @brief @return True if any allocation is still outstanding. */
    [[nodiscard]] bool has_leaks() const noexcept { return outstanding() != 0 || bytes_in_use() != 0; }

    /** @brief Zero all counters (not synchronised with concurrent allocation). */
    void reset() noexcept {
        allocations_ = 0;
        deallocations_ = 0;
        bytes_allocated_ = 0;
        bytes_deallocated_ = 0;
        in_use_ = 0;
        peak_ = 0;
    }

    /**
     * @brief Process-wide statistics used by default-constructed TrackingAllocators.
     * @return Reference to the global instance.
     */
    [[nodiscard]] static AllocationStats& global() noexcept {
        static AllocationStats instance;
        return instance;
    }

private:
    std::atomic<std::size_t> allocations_{0};
    std::atomic<std::size_t> deallocations_{0};
    std::atomic<std::size_t> bytes_allocated_{0};
    std::atomic<std::size_t> bytes_deallocated_{0};
    std::atomic<std::size_t> in_use_{0};
    std::atomic<std::size_t> peak_{0};
};

/**
 * @class TrackingAllocator
 * @brief Stateful STL allocator that forwards to the global heap and records statistics.
 *
 * Over-aligned types (alignof(T) > __STDCPP_DEFAULT_NEW_ALIGNMENT__) are served through
 * aligned `operator new`. Two allocators compare equal iff they share the same
 * AllocationStats, which is what a container needs to know to free memory it did not
 * allocate itself.
 *
 * @tparam T Value type.
 */
template <typename T>
class TrackingAllocator {
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
        using other = TrackingAllocator<U>;
    };

    /** @brief Construct an allocator reporting to AllocationStats::global(). */
    TrackingAllocator() noexcept : stats_(&AllocationStats::global()) {}

    /**
     * @brief Construct an allocator reporting to @p stats.
     * @param stats Statistics sink; must outlive every copy of this allocator.
     */
    explicit TrackingAllocator(AllocationStats& stats) noexcept : stats_(&stats) {}

    /**
     * @brief Rebinding converting constructor.
     * @param other Allocator for another value type; shares its statistics.
     */
    template <typename U>
    TrackingAllocator(const TrackingAllocator<U>& other) noexcept // NOLINT(google-explicit-constructor)
        : stats_(other.stats()) {}

    /**
     * @brief Allocate storage for @p n objects (uninitialised).
     * @param n Number of objects.
     * @return Pointer to storage aligned for T.
     * @throws std::bad_array_new_length if n * sizeof(T) overflows; std::bad_alloc on failure.
     */
    [[nodiscard]] T* allocate(size_type n) {
        if (n > max_size()) {
            throw std::bad_array_new_length();
        }
        const std::size_t bytes = n * sizeof(T);
        void* p = detail::allocate_bytes(bytes, alignof(T));
        stats_->record_allocation(bytes); // only after success: strong guarantee
        return static_cast<T*>(p);
    }

    /**
     * @brief Release storage obtained from allocate().
     * @param p Pointer returned by allocate(n).
     * @param n The same @p n passed to allocate().
     */
    void deallocate(T* p, size_type n) noexcept {
        if (p == nullptr) {
            return;
        }
        detail::deallocate_bytes(p, alignof(T));
        stats_->record_deallocation(n * sizeof(T));
    }

    /** @brief @return Largest @p n that allocate() can accept. */
    [[nodiscard]] static constexpr size_type max_size() noexcept {
        return std::numeric_limits<size_type>::max() / sizeof(T);
    }

    /** @brief @return The statistics sink shared by this allocator and its copies. */
    [[nodiscard]] AllocationStats* stats() const noexcept { return stats_; }

private:
    AllocationStats* stats_;
};

/**
 * @brief Equality: allocators are interchangeable iff they share statistics.
 * @param lhs First allocator.
 * @param rhs Second allocator.
 * @return True if memory from one may be released through the other.
 */
template <typename T, typename U>
[[nodiscard]] bool operator==(const TrackingAllocator<T>& lhs, const TrackingAllocator<U>& rhs) noexcept {
    return lhs.stats() == rhs.stats();
}

/**
 * @class TrackingMemoryResource
 * @brief `std::pmr::memory_resource` decorator that records statistics and verifies alignment.
 */
class TrackingMemoryResource final : public std::pmr::memory_resource {
public:
    /**
     * @brief Wrap an upstream resource.
     * @param upstream Resource that performs the real allocations.
     */
    explicit TrackingMemoryResource(
        std::pmr::memory_resource* upstream = std::pmr::new_delete_resource()) noexcept
        : upstream_(upstream) {}

    /** @brief @return Statistics collected so far. */
    [[nodiscard]] const AllocationStats& stats() const noexcept { return stats_; }
    /** @brief @return Mutable statistics (e.g. to reset them). */
    [[nodiscard]] AllocationStats& stats() noexcept { return stats_; }
    /** @brief @return Number of allocations whose result violated the requested alignment. */
    [[nodiscard]] std::size_t misaligned_count() const noexcept { return misaligned_.load(); }
    /** @brief @return The wrapped upstream resource. */
    [[nodiscard]] std::pmr::memory_resource* upstream() const noexcept { return upstream_; }

private:
    void* do_allocate(std::size_t bytes, std::size_t alignment) override {
        void* p = upstream_->allocate(bytes, alignment);
        if (!AllocatorUtils::is_aligned(p, alignment)) {
            misaligned_.fetch_add(1, std::memory_order_relaxed);
        }
        stats_.record_allocation(bytes);
        return p;
    }

    void do_deallocate(void* p, std::size_t bytes, std::size_t alignment) override {
        upstream_->deallocate(p, bytes, alignment);
        stats_.record_deallocation(bytes);
    }

    [[nodiscard]] bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }

    std::pmr::memory_resource* upstream_;
    AllocationStats stats_;
    std::atomic<std::size_t> misaligned_{0};
};

/**
 * @class StackAllocator
 * @brief Fixed-capacity LIFO arena whose storage lives inside the object.
 *
 * Allocation bumps a top-of-stack offset (aligned with `std::align`); only the most
 * recent allocation can be freed individually. Markers allow rewinding to a checkpoint,
 * which is the classic "frame allocator" pattern used for per-frame scratch memory.
 *
 * @tparam Capacity Size of the internal buffer in bytes.
 */
template <std::size_t Capacity>
class StackAllocator {
    static_assert(Capacity > 0, "StackAllocator needs a non-empty buffer");

public:
    /** @brief Opaque checkpoint produced by mark(). */
    using Marker = std::size_t;

    /** @brief Construct an empty arena. */
    StackAllocator() noexcept = default;
    StackAllocator(const StackAllocator&) = delete;
    StackAllocator& operator=(const StackAllocator&) = delete;
    StackAllocator(StackAllocator&&) = delete;
    StackAllocator& operator=(StackAllocator&&) = delete;
    ~StackAllocator() = default;

    /**
     * @brief Allocate @p bytes aligned to @p alignment.
     * @param bytes Size in bytes (0 is treated as 1).
     * @param alignment Power-of-two alignment.
     * @return Pointer into the internal buffer.
     * @throws std::invalid_argument for a non power-of-two alignment; std::bad_alloc when full.
     */
    [[nodiscard]] void* allocate(std::size_t bytes, std::size_t alignment = alignof(std::max_align_t)) {
        if (!AllocatorUtils::is_power_of_two(alignment)) {
            throw std::invalid_argument("StackAllocator: alignment must be a power of two");
        }
        bytes = bytes == 0 ? 1 : bytes;
        void* p = buffer_ + top_;
        std::size_t space = Capacity - top_;
        if (std::align(alignment, bytes, p, space) == nullptr) {
            throw std::bad_alloc();
        }
        top_ = static_cast<std::size_t>(static_cast<std::byte*>(p) - buffer_) + bytes;
        ++live_;
        return p;
    }

    /**
     * @brief Free the most recent allocation. Out-of-order frees are ignored
     *        (the memory is reclaimed by reset() or rewind()).
     * @param p Pointer returned by allocate().
     * @param bytes Size passed to allocate().
     * @return True if the stack top moved back.
     */
    bool deallocate(void* p, std::size_t bytes) noexcept {
        if (p == nullptr || !owns(p)) {
            return false;
        }
        bytes = bytes == 0 ? 1 : bytes;
        const auto offset = static_cast<std::size_t>(static_cast<std::byte*>(p) - buffer_);
        if (live_ > 0) {
            --live_;
        }
        if (offset + bytes == top_) {
            top_ = offset;
            return true;
        }
        return false;
    }

    /** @brief @return A checkpoint for rewind(). */
    [[nodiscard]] Marker mark() const noexcept { return top_; }

    /**
     * @brief Release everything allocated after @p marker.
     * @param marker Value previously returned by mark().
     */
    void rewind(Marker marker) noexcept {
        if (marker <= top_) {
            top_ = marker;
        }
    }

    /** @brief Release everything. */
    void reset() noexcept {
        top_ = 0;
        live_ = 0;
    }

    /**
     * @brief Check whether @p p points into the internal buffer.
     * @param p Pointer to test.
     * @return True if owned.
     */
    [[nodiscard]] bool owns(const void* p) const noexcept {
        const auto* b = static_cast<const std::byte*>(p);
        return !std::less<const std::byte*>{}(b, buffer_) &&
               std::less<const std::byte*>{}(b, buffer_ + Capacity);
    }

    /** @brief @return Bytes consumed including alignment padding. */
    [[nodiscard]] std::size_t bytes_used() const noexcept { return top_; }
    /** @brief @return Bytes still available (before alignment padding). */
    [[nodiscard]] std::size_t bytes_remaining() const noexcept { return Capacity - top_; }
    /** @brief @return Total capacity in bytes. */
    [[nodiscard]] static constexpr std::size_t capacity() noexcept { return Capacity; }

private:
    alignas(std::max_align_t) std::byte buffer_[Capacity]{};
    std::size_t top_ = 0;
    std::size_t live_ = 0;
};

/**
 * @class MonotonicArena
 * @brief Chunked bump-pointer arena usable as a `std::pmr::memory_resource`.
 *
 * Memory is obtained from an upstream resource in geometrically growing chunks
 * (optionally starting with a caller-supplied buffer) and released all at once by
 * release() or the destructor; individual deallocation is a no-op. Requests of any
 * power-of-two alignment are satisfied with `std::align`.
 */
class MonotonicArena final : public std::pmr::memory_resource {
public:
    /** @brief Default size of the first heap chunk. */
    static constexpr std::size_t kDefaultChunkSize = 1024;

    /**
     * @brief Construct an arena that allocates chunks from @p upstream.
     * @param initial_chunk_size Size of the first chunk (subsequent chunks double).
     * @param upstream Resource providing chunk memory.
     */
    explicit MonotonicArena(std::size_t initial_chunk_size = kDefaultChunkSize,
                            std::pmr::memory_resource* upstream = std::pmr::get_default_resource());

    /**
     * @brief Construct an arena that first consumes a caller-provided buffer.
     * @param buffer Initial buffer (not owned, must outlive the arena).
     * @param size Size of @p buffer in bytes.
     * @param upstream Resource providing overflow chunks.
     */
    MonotonicArena(void* buffer, std::size_t size,
                   std::pmr::memory_resource* upstream = std::pmr::get_default_resource());

    MonotonicArena(const MonotonicArena&) = delete;
    MonotonicArena& operator=(const MonotonicArena&) = delete;
    MonotonicArena(MonotonicArena&&) = delete;
    MonotonicArena& operator=(MonotonicArena&&) = delete;

    /** @brief Release all chunks to the upstream resource. */
    ~MonotonicArena() override;

    /** @brief Return every chunk to upstream and restart from the initial buffer. */
    void release() noexcept;

    /** @brief @return Bytes handed out (excluding padding). */
    [[nodiscard]] std::size_t bytes_allocated() const noexcept { return bytes_allocated_; }
    /** @brief @return Bytes still available in the current chunk. */
    [[nodiscard]] std::size_t bytes_remaining() const noexcept { return remaining_; }
    /** @brief @return Number of heap chunks obtained from upstream. */
    [[nodiscard]] std::size_t chunk_count() const noexcept { return chunks_.size(); }
    /** @brief @return Number of allocate requests served. */
    [[nodiscard]] std::size_t allocation_count() const noexcept { return allocation_count_; }
    /** @brief @return Upstream resource. */
    [[nodiscard]] std::pmr::memory_resource* upstream() const noexcept { return upstream_; }

private:
    struct Chunk {
        void* memory;
        std::size_t size;
        std::size_t alignment;
    };

    void* do_allocate(std::size_t bytes, std::size_t alignment) override;
    void do_deallocate(void* p, std::size_t bytes, std::size_t alignment) override;
    [[nodiscard]] bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override;

    [[nodiscard]] void* try_bump(std::size_t bytes, std::size_t alignment) noexcept;
    void grow(std::size_t bytes, std::size_t alignment);

    std::pmr::memory_resource* upstream_;
    std::vector<Chunk> chunks_;
    void* initial_buffer_ = nullptr;
    std::size_t initial_size_ = 0;
    std::size_t next_chunk_size_;
    std::byte* cursor_ = nullptr;
    std::size_t remaining_ = 0;
    std::size_t bytes_allocated_ = 0;
    std::size_t allocation_count_ = 0;
};

/**
 * @class ArenaAllocator
 * @brief Classic (non-polymorphic) STL allocator drawing from a MonotonicArena.
 *
 * deallocate() is a no-op: the arena reclaims memory in bulk. Ideal for build-once,
 * discard-together data structures.
 *
 * @tparam T Value type.
 */
template <typename T>
class ArenaAllocator {
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
        using other = ArenaAllocator<U>;
    };

    /**
     * @brief Bind to an arena.
     * @param arena Arena that must outlive every container using this allocator.
     */
    explicit ArenaAllocator(MonotonicArena& arena) noexcept : arena_(&arena) {}

    /**
     * @brief Rebinding converting constructor.
     * @param other Allocator for another type sharing the same arena.
     */
    template <typename U>
    ArenaAllocator(const ArenaAllocator<U>& other) noexcept // NOLINT(google-explicit-constructor)
        : arena_(other.arena()) {}

    /**
     * @brief Allocate storage for @p n objects from the arena.
     * @param n Number of objects.
     * @return Pointer aligned for T.
     * @throws std::bad_array_new_length on overflow; std::bad_alloc if upstream fails.
     */
    [[nodiscard]] T* allocate(size_type n) {
        if (n > std::numeric_limits<size_type>::max() / sizeof(T)) {
            throw std::bad_array_new_length();
        }
        return static_cast<T*>(arena_->allocate(n * sizeof(T), alignof(T)));
    }

    /**
     * @brief No-op; memory is reclaimed when the arena is released.
     * @param p Pointer from allocate().
     * @param n Count passed to allocate().
     */
    void deallocate(T* p, size_type n) noexcept { arena_->deallocate(p, n * sizeof(T), alignof(T)); }

    /** @brief @return The bound arena. */
    [[nodiscard]] MonotonicArena* arena() const noexcept { return arena_; }

private:
    MonotonicArena* arena_;
};

/**
 * @brief Equality: allocators are interchangeable iff they share the same arena.
 * @param lhs First allocator.
 * @param rhs Second allocator.
 * @return True if both use the same arena.
 */
template <typename T, typename U>
[[nodiscard]] bool operator==(const ArenaAllocator<T>& lhs, const ArenaAllocator<U>& rhs) noexcept {
    return lhs.arena() == rhs.arena();
}

static_assert(AllocatorUtils::Allocator<TrackingAllocator<int>>);
static_assert(AllocatorUtils::Allocator<ArenaAllocator<double>>);

/**
 * @brief Showcase: tracking, stack, arena and pmr allocators with standard containers.
 * @param out Stream receiving the narration.
 */
void demonstrateCustomAllocators(std::ostream& out = std::cout);

} // namespace CppVerseHub::Memory

#endif // CPPVERSEHUB_MEMORY_CUSTOM_ALLOCATORS_HPP
