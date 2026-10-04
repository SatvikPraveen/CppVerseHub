/**
 * @file Atomics.hpp
 * @brief `std::atomic`, memory ordering and lock-free data structures.
 * @details File location: src/concurrency/Atomics.hpp
 *
 * Every atomic operation in this file states *why* its memory order is sufficient.
 * The rule of thumb used throughout: an index/flag store that *publishes* data written
 * just before it is a `release`; the load that *consumes* that publication is an
 * `acquire`; a value that nobody synchronises through (statistics, a thread's own
 * index) is `relaxed`. Where a proof needs a single total order across several
 * variables (the stack's reclamation scheme), `seq_cst` is used deliberately.
 *
 * Contents:
 *  - `SpinLock`: test-and-test-and-set lock satisfying *Lockable*.
 *  - `SpscRingBuffer<T, N>`: wait-free single-producer/single-consumer ring buffer with
 *    cache-line separated indices and cached opposite indices.
 *  - `LockFreeStack<T>`: Treiber stack with "threads in pop" deferred reclamation
 *    (A. Williams, *C++ Concurrency in Action*, §7.2.2), which also rules out ABA.
 *  - `AtomicStatistics`: lock-free min/max/sum via CAS loops.
 *  - `ConcurrentBloomFilter`: lock-free set-membership sketch using `fetch_or`.
 *  - `OneShotEvent`: C++20 `std::atomic<T>::wait`/`notify_all` (futex-style blocking).
 */

#ifndef CPPVERSEHUB_CONCURRENCY_ATOMICS_HPP
#define CPPVERSEHUB_CONCURRENCY_ATOMICS_HPP

#include <array>
#include <atomic>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <memory>
#include <new>
#include <optional>
#include <string_view>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86))
#include <intrin.h>
#endif

namespace CppVerseHub::Concurrency {

/**
 * @brief Assumed cache-line size used to separate hot atomics (avoids false sharing).
 *
 * `std::hardware_destructive_interference_size` is not used because it is missing
 * from some standard libraries and GCC warns when it is used in headers (ABI concern).
 */
inline constexpr std::size_t kCacheLineSize = 64;

/** @brief Hints the CPU that the caller is busy-waiting (no-op where unsupported). */
inline void cpu_relax() noexcept {
#if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86))
    _mm_pause();
#elif (defined(__GNUC__) || defined(__clang__)) && (defined(__x86_64__) || defined(__i386__))
    __builtin_ia32_pause();
#elif (defined(__GNUC__) || defined(__clang__)) && (defined(__aarch64__) || defined(__arm__))
    __asm__ __volatile__("yield");
#else
    std::this_thread::yield();
#endif
}

/**
 * @brief Test-and-test-and-set spin lock (meets *Lockable*, usable with `std::lock_guard`).
 *
 * Spinning on a relaxed load keeps the cache line shared until it is released, instead of
 * hammering it with RMW operations. Only appropriate for very short critical sections.
 */
class SpinLock {
public:
    /** @brief Acquires the lock, spinning (and eventually yielding) while contended. */
    void lock() noexcept {
        for (;;) {
            // acquire: pairs with the release in unlock(); the critical section of the
            // previous owner happens-before ours.
            if (!locked_.exchange(true, std::memory_order_acquire)) {
                return;
            }
            unsigned spins = 0;
            // relaxed: only a hint; the exchange above re-validates with acquire.
            while (locked_.load(std::memory_order_relaxed)) {
                if (++spins < 64) {
                    cpu_relax();
                } else {
                    std::this_thread::yield();
                }
            }
        }
    }

    /** @brief @return true if the lock was acquired without waiting. */
    [[nodiscard]] bool try_lock() noexcept {
        return !locked_.load(std::memory_order_relaxed) && !locked_.exchange(true, std::memory_order_acquire);
    }

    /** @brief Releases the lock. @pre Held by the caller. */
    void unlock() noexcept {
        // release: publishes the critical section to the next acquirer.
        locked_.store(false, std::memory_order_release);
    }

private:
    std::atomic<bool> locked_{false};
};

/**
 * @brief Bounded wait-free single-producer/single-consumer ring buffer.
 *
 * Exactly one thread may call the producer side (`try_push`) and exactly one thread the
 * consumer side (`try_pop`, `front_ready`) at a time. Indices grow monotonically and are
 * masked on access, so "full" (`tail - head == N`) and "empty" (`tail == head`) are
 * distinguishable without wasting a slot.
 *
 * Memory-ordering contract:
 *  - Producer: constructs the element, then `tail_.store(release)`. Consumer:
 *    `tail_.load(acquire)` before reading the element. => the element's construction
 *    happens-before its consumption.
 *  - Consumer: moves out and destroys the element, then `head_.store(release)`. Producer:
 *    `head_.load(acquire)` before reusing the slot. => destruction happens-before reuse.
 *  - Each side loads its *own* index `relaxed` (only it ever writes it).
 *  - Each side caches the other side's index and only re-reads it (acquire) when the
 *    cached value says full/empty, which removes most cross-core cache traffic.
 *
 * @tparam T Element type.
 * @tparam Capacity Number of slots; must be a power of two.
 */
template <typename T, std::size_t Capacity>
class SpscRingBuffer {
    static_assert(Capacity > 0 && std::has_single_bit(Capacity), "Capacity must be a power of two");

public:
    SpscRingBuffer() = default;
    SpscRingBuffer(const SpscRingBuffer&) = delete;
    SpscRingBuffer& operator=(const SpscRingBuffer&) = delete;
    SpscRingBuffer(SpscRingBuffer&&) = delete;
    SpscRingBuffer& operator=(SpscRingBuffer&&) = delete;

    /** @brief Destroys any elements still buffered. Must not race with push/pop. */
    ~SpscRingBuffer() {
        const std::size_t tail = tail_.load(std::memory_order_relaxed);
        for (std::size_t i = head_.load(std::memory_order_relaxed); i != tail; ++i) {
            std::destroy_at(slot(i));
        }
    }

    /**
     * @brief Producer: enqueues if there is space.
     * @param value Element to enqueue (not consumed on failure).
     * @return false if the buffer is full.
     */
    template <typename U = T>
    [[nodiscard]] bool try_push(U&& value) {
        const std::size_t tail = tail_.load(std::memory_order_relaxed);
        if (tail - producer_cached_head_ == Capacity) {
            producer_cached_head_ = head_.load(std::memory_order_acquire);
            if (tail - producer_cached_head_ == Capacity) {
                return false;
            }
        }
        std::construct_at(slot_storage(tail), std::forward<U>(value));
        tail_.store(tail + 1, std::memory_order_release);
        return true;
    }

    /**
     * @brief Consumer: dequeues the oldest element if any.
     * @return The element, or `std::nullopt` if empty.
     */
    [[nodiscard]] std::optional<T> try_pop() {
        const std::size_t head = head_.load(std::memory_order_relaxed);
        if (head == consumer_cached_tail_) {
            consumer_cached_tail_ = tail_.load(std::memory_order_acquire);
            if (head == consumer_cached_tail_) {
                return std::nullopt;
            }
        }
        T* element = slot(head);
        std::optional<T> result(std::move(*element));
        std::destroy_at(element);
        head_.store(head + 1, std::memory_order_release);
        return result;
    }

    /**
     * @brief Approximate number of buffered elements (exact when called by either endpoint
     *        while the other is quiescent).
     * @return Element count.
     */
    [[nodiscard]] std::size_t size_approx() const noexcept {
        const std::size_t head = head_.load(std::memory_order_acquire);
        const std::size_t tail = tail_.load(std::memory_order_acquire);
        return tail >= head ? tail - head : 0;
    }

    /** @brief @return true if `size_approx() == 0`. */
    [[nodiscard]] bool empty_approx() const noexcept { return size_approx() == 0; }

    /** @brief @return The fixed capacity `Capacity`. */
    [[nodiscard]] static constexpr std::size_t capacity() noexcept { return Capacity; }

private:
    struct alignas(T) Slot {
        std::byte bytes[sizeof(T)];
    };

    static constexpr std::size_t kMask = Capacity - 1;

    T* slot_storage(std::size_t index) noexcept {
        // The slot's object was created by placement new into these bytes; launder recovers a usable pointer.
        return std::launder(reinterpret_cast<T*>(buffer_[index & kMask].bytes));
    }
    T* slot(std::size_t index) noexcept { return std::launder(slot_storage(index)); }

    // Consumer-owned line.
    alignas(kCacheLineSize) std::atomic<std::size_t> head_{0};
    std::size_t consumer_cached_tail_ = 0;
    // Producer-owned line.
    alignas(kCacheLineSize) std::atomic<std::size_t> tail_{0};
    std::size_t producer_cached_head_ = 0;
    // Shared payload.
    alignas(kCacheLineSize) std::array<Slot, Capacity> buffer_{};
};

/**
 * @brief Lock-free (Treiber) stack with safe memory reclamation.
 *
 * Reclamation: a node popped while other threads are also inside `pop()` might still be
 * dereferenced by them, so it is put on a `to_be_deleted_` list instead of being freed.
 * Whenever a thread observes that it is the *only* one in `pop()` it frees its node and
 * the pending list. Because no node is freed while a concurrent popper may hold it, an
 * address cannot be recycled under a popper's feet, so the ABA problem cannot occur.
 * Under sustained contention the pending list may grow until a quiescent moment.
 *
 * Nodes are necessarily raw pointers inside the atomic links; ownership is created with
 * `std::make_unique` and released only once a node is published.
 *
 * @tparam T Element type (move-constructible).
 */
template <typename T>
class LockFreeStack {
public:
    LockFreeStack() = default;
    LockFreeStack(const LockFreeStack&) = delete;
    LockFreeStack& operator=(const LockFreeStack&) = delete;
    LockFreeStack(LockFreeStack&&) = delete;
    LockFreeStack& operator=(LockFreeStack&&) = delete;

    /** @brief Frees all remaining nodes. Must not race with push/pop. */
    ~LockFreeStack() {
        delete_chain(head_.load(std::memory_order_relaxed));
        delete_chain(to_be_deleted_.load(std::memory_order_relaxed));
    }

    /**
     * @brief Pushes a value (lock-free).
     * @param value Element to push.
     */
    template <typename U = T>
    void push(U&& value) {
        auto owned = std::make_unique<Node>(std::forward<U>(value));
        Node* node = owned.get();
        Node* expected = head_.load(std::memory_order_relaxed);
        do {
            // relaxed: the node is not yet visible to anyone; the release CAS publishes it.
            node->next.store(expected, std::memory_order_relaxed);
            // release on success: the node's contents happen-before any pop that acquires it.
        } while (!head_.compare_exchange_weak(expected, node, std::memory_order_release,
                                              std::memory_order_relaxed));
        static_cast<void>(owned.release());
    }

    /**
     * @brief Pops the most recently pushed value (lock-free).
     * @return The value or `std::nullopt` if the stack was empty.
     */
    [[nodiscard]] std::optional<T> pop() {
        // seq_cst: the reclamation argument needs "increment before reading head" and
        // "unlink before reading the counter" to be ordered in one total order.
        threads_in_pop_.fetch_add(1);
        Node* old_head = head_.load();
        // `next` is atomic because a stale `old_head` may concurrently be re-linked onto the
        // pending list by the thread that really popped it. The value read then is garbage,
        // but the CAS fails (the node cannot reappear as head while we are in pop), so it is
        // never used. relaxed suffices: when the CAS succeeds, our acquiring load of `head_`
        // synchronised with the push that wrote `next`.
        while (old_head != nullptr &&
               !head_.compare_exchange_weak(old_head, old_head->next.load(std::memory_order_relaxed))) {}
        std::optional<T> result;
        if (old_head != nullptr) {
            result.emplace(std::move(old_head->value));
        }
        try_reclaim(old_head);
        return result;
    }

    /** @brief @return true if the stack was empty at the moment of the load. */
    [[nodiscard]] bool empty() const noexcept { return head_.load(std::memory_order_acquire) == nullptr; }

    /** @brief @return Number of popped nodes awaiting reclamation (diagnostic snapshot). */
    [[nodiscard]] std::size_t pending_reclamation() const noexcept {
        std::size_t n = 0;
        // Only meaningful when quiescent; traversal is not safe concurrently with pop().
        for (Node* p = to_be_deleted_.load(std::memory_order_acquire); p != nullptr;
             p = p->next.load(std::memory_order_relaxed)) {
            ++n;
        }
        return n;
    }

private:
    struct Node {
        template <typename U>
            requires(!std::is_same_v<std::remove_cvref_t<U>, Node>)
        explicit Node(U&& v) : value(std::forward<U>(v)) {}
        T value;
        std::atomic<Node*> next{nullptr};
    };

    static void delete_chain(Node* node) noexcept {
        while (node != nullptr) {
            std::unique_ptr<Node> owned(node);
            node = node->next.load(std::memory_order_relaxed);
        }
    }

    void chain_pending(Node* first, Node* last) noexcept {
        Node* expected = to_be_deleted_.load();
        do {
            last->next.store(expected, std::memory_order_relaxed);
        } while (!to_be_deleted_.compare_exchange_weak(expected, first)); // seq_cst: publishes the chain
    }

    void chain_pending(Node* nodes) noexcept {
        Node* last = nodes;
        while (Node* next = last->next.load(std::memory_order_relaxed)) {
            last = next;
        }
        chain_pending(nodes, last);
    }

    void try_reclaim(Node* old_head) noexcept {
        if (threads_in_pop_.load() == 1) {
            // We are the only popper: claim the pending list.
            Node* claimed = to_be_deleted_.exchange(nullptr);
            if (threads_in_pop_.fetch_sub(1) == 1) {
                delete_chain(claimed); // still alone: nobody can reference these nodes
            } else if (claimed != nullptr) {
                chain_pending(claimed); // someone joined meanwhile: put them back
            }
            // old_head was unlinked before we saw the count at 1, so no other popper
            // (current or future) can be holding it.
            std::unique_ptr<Node> reclaim(old_head);
        } else {
            if (old_head != nullptr) {
                chain_pending(old_head, old_head);
            }
            threads_in_pop_.fetch_sub(1);
        }
    }

    alignas(kCacheLineSize) std::atomic<Node*> head_{nullptr};
    alignas(kCacheLineSize) std::atomic<std::size_t> threads_in_pop_{0};
    alignas(kCacheLineSize) std::atomic<Node*> to_be_deleted_{nullptr};
};

/**
 * @brief Lock-free running statistics (count/sum/min/max) updated with relaxed atomics.
 *
 * Each field is individually atomic; a `snapshot()` taken while writers are active is not
 * a consistent cut across fields. Take it after joining the writers for exact values.
 */
class AtomicStatistics {
public:
    /** @brief Consistent view of the statistics. */
    struct Snapshot {
        std::uint64_t count = 0; ///< Number of samples.
        std::int64_t sum = 0;    ///< Sum of samples.
        std::int64_t min = 0;    ///< Smallest sample (0 if empty).
        std::int64_t max = 0;    ///< Largest sample (0 if empty).
        /** @brief @return Arithmetic mean, 0 if empty. */
        [[nodiscard]] double mean() const noexcept {
            return count == 0 ? 0.0 : static_cast<double>(sum) / static_cast<double>(count);
        }
    };

    /**
     * @brief Records one sample.
     * @param value Sample value.
     */
    void record(std::int64_t value) noexcept;
    /** @brief @return Current values of all fields. */
    [[nodiscard]] Snapshot snapshot() const noexcept;
    /** @brief Resets to the empty state. Must not race with `record()`. */
    void reset() noexcept;

private:
    // relaxed everywhere: the statistics are not used to publish other data.
    std::atomic<std::uint64_t> count_{0};
    std::atomic<std::int64_t> sum_{0};
    std::atomic<std::int64_t> min_{std::numeric_limits<std::int64_t>::max()};
    std::atomic<std::int64_t> max_{std::numeric_limits<std::int64_t>::min()};
};

/**
 * @brief Atomically raises `target` to at least `value` (a portable `fetch_max`).
 * @param target Atomic to update.
 * @param value Candidate value.
 * @param order Memory order used on a successful update.
 * @return The previous value.
 */
template <typename T>
T atomic_fetch_max(std::atomic<T>& target, T value,
                   std::memory_order order = std::memory_order_relaxed) noexcept {
    T current = target.load(std::memory_order_relaxed);
    while (current < value &&
           !target.compare_exchange_weak(current, value, order, std::memory_order_relaxed)) {}
    return current;
}

/**
 * @brief Atomically lowers `target` to at most `value` (a portable `fetch_min`).
 * @param target Atomic to update.
 * @param value Candidate value.
 * @param order Memory order used on a successful update.
 * @return The previous value.
 */
template <typename T>
T atomic_fetch_min(std::atomic<T>& target, T value,
                   std::memory_order order = std::memory_order_relaxed) noexcept {
    T current = target.load(std::memory_order_relaxed);
    while (value < current &&
           !target.compare_exchange_weak(current, value, order, std::memory_order_relaxed)) {}
    return current;
}

/**
 * @brief Thread-safe Bloom filter: no false negatives, tunable false-positive rate.
 *
 * Bits are set with relaxed `fetch_or`; queries use relaxed loads. Relaxed suffices
 * because the bits are the only shared state; if a caller needs "inserted => visible",
 * that ordering must come from the caller's own synchronisation (e.g. a join).
 * Hashing uses FNV-1a + double hashing so results are identical on every platform.
 */
class ConcurrentBloomFilter {
public:
    /**
     * @brief Creates the filter.
     * @param bit_count Number of bits (rounded up to a multiple of 64, at least 64).
     * @param hash_count Number of hash probes per key (at least 1).
     */
    ConcurrentBloomFilter(std::size_t bit_count, std::size_t hash_count);

    /**
     * @brief Inserts a key.
     * @param key Key bytes.
     */
    void insert(std::string_view key) noexcept;
    /**
     * @brief Membership query.
     * @param key Key bytes.
     * @return false => definitely absent; true => probably present.
     */
    [[nodiscard]] bool possibly_contains(std::string_view key) const noexcept;
    /** @brief @return Number of bits in the filter. */
    [[nodiscard]] std::size_t bit_count() const noexcept { return words_.size() * 64; }
    /** @brief @return Number of set bits (snapshot). */
    [[nodiscard]] std::size_t popcount() const noexcept;

private:
    [[nodiscard]] std::size_t probe(std::uint64_t h1, std::uint64_t h2, std::size_t i) const noexcept;
    std::vector<std::atomic<std::uint64_t>> words_;
    std::size_t hash_count_;
};

/**
 * @brief One-shot event using C++20 `std::atomic<bool>::wait/notify_all`.
 *
 * Unlike a spin loop, `wait()` blocks in the kernel (futex/ulock/WaitOnAddress).
 */
class OneShotEvent {
public:
    /** @brief Signals the event (release: writes before `set()` are visible after `wait()`). */
    void set() noexcept {
        flag_.store(true, std::memory_order_release);
        flag_.notify_all();
    }
    /** @brief Blocks until `set()` has been called (acquire). */
    void wait() const noexcept {
        while (!flag_.load(std::memory_order_acquire)) {
            flag_.wait(false, std::memory_order_acquire);
        }
    }
    /** @brief @return true if set. */
    [[nodiscard]] bool is_set() const noexcept { return flag_.load(std::memory_order_acquire); }

private:
    std::atomic<bool> flag_{false};
};

/**
 * @brief Classic release/acquire message passing: a writer fills a *non-atomic* payload
 *        and raises a flag with `release`; a reader spins with `acquire`, then reads the payload.
 * @param payload Value the writer publishes.
 * @return The value observed by the reader (always equals `payload`; data-race free).
 */
[[nodiscard]] int release_acquire_message_passing(int payload);

/**
 * @brief Increments a shared counter from several threads with `fetch_add(relaxed)`.
 * @param threads Number of threads.
 * @param increments_per_thread Increments per thread.
 * @return Final counter value (always `threads * increments_per_thread`).
 */
[[nodiscard]] std::uint64_t relaxed_counter_total(std::size_t threads, std::uint64_t increments_per_thread);

/**
 * @brief Showcase: memory ordering, spin lock, SPSC ring, lock-free stack, statistics, Bloom filter.
 * @param out Destination stream.
 */
void demonstrate_atomics(std::ostream& out = std::cout);

} // namespace CppVerseHub::Concurrency

#endif // CPPVERSEHUB_CONCURRENCY_ATOMICS_HPP
