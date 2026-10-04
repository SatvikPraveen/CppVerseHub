/**
 * @file ConditionalVariables.hpp
 * @brief Blocking coordination primitives built from `std::mutex` + `std::condition_variable`.
 * @details File location: src/concurrency/ConditionalVariables.hpp
 *
 * Condition variables are the general-purpose "wait until a predicate over shared
 * state becomes true" primitive. Every type here follows the same discipline:
 * the predicate's state is only touched under the mutex, every wait uses the
 * predicate overload (robust against spurious wake-ups), and notification happens
 * after the state change. The module shows:
 *
 *  - `BoundedQueue<T>`: a closable bounded multi-producer/multi-consumer queue
 *    (back-pressure on producers, graceful drain for consumers).
 *  - `CountingSemaphore`, `CountDownLatch`, `CyclicBarrier`, `ManualResetEvent`:
 *    hand-rolled equivalents of the C++20 `<semaphore>`, `<latch>`, `<barrier>` types,
 *    which makes their semantics explicit and works on every standard library.
 *  - `ResourcePool<T>`: a blocking object pool that hands out RAII leases.
 */

#ifndef CPPVERSEHUB_CONCURRENCY_CONDITIONALVARIABLES_HPP
#define CPPVERSEHUB_CONCURRENCY_CONDITIONALVARIABLES_HPP

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <functional>
#include <iostream>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace CppVerseHub::Concurrency {

/**
 * @brief Closable, bounded, blocking multi-producer/multi-consumer FIFO queue.
 *
 * - `push` blocks while the queue is full (back-pressure); `pop` blocks while empty.
 * - After `close()`, pushes fail immediately and pops drain the remaining elements,
 *   then return `std::nullopt`. This gives consumers a clean termination signal.
 * - A failed `try_push`/`push_for` does not move from its argument.
 *
 * @tparam T Element type; must be move-constructible.
 */
template <typename T>
class BoundedQueue {
public:
    /**
     * @brief Creates an empty queue.
     * @param capacity Maximum number of buffered elements (must be > 0).
     * @throws std::invalid_argument if `capacity == 0`.
     */
    explicit BoundedQueue(std::size_t capacity) : capacity_(capacity) {
        if (capacity_ == 0) {
            throw std::invalid_argument("BoundedQueue capacity must be positive");
        }
    }

    /**
     * @brief Blocks until there is space, then enqueues.
     * @param value Element to enqueue.
     * @return false if the queue was closed (the value is not consumed).
     */
    template <typename U = T>
    bool push(U&& value) {
        std::unique_lock lock(mutex_);
        not_full_.wait(lock, [this] { return closed_ || items_.size() < capacity_; });
        return emplace_locked(lock, std::forward<U>(value));
    }

    /**
     * @brief Enqueues without blocking.
     * @param value Element to enqueue.
     * @return false if full or closed (the value is not consumed).
     */
    template <typename U = T>
    bool try_push(U&& value) {
        std::unique_lock lock(mutex_);
        if (items_.size() >= capacity_) {
            return false;
        }
        return emplace_locked(lock, std::forward<U>(value));
    }

    /**
     * @brief Waits at most `timeout` for space, then enqueues.
     * @param value Element to enqueue.
     * @param timeout Maximum wait.
     * @return false on timeout or if closed.
     */
    template <typename U, typename Rep, typename Period>
    bool push_for(U&& value, const std::chrono::duration<Rep, Period>& timeout) {
        std::unique_lock lock(mutex_);
        if (!not_full_.wait_for(lock, timeout, [this] { return closed_ || items_.size() < capacity_; })) {
            return false;
        }
        return emplace_locked(lock, std::forward<U>(value));
    }

    /**
     * @brief Blocks until an element is available or the queue is closed and drained.
     * @return The front element, or `std::nullopt` once closed and empty.
     */
    [[nodiscard]] std::optional<T> pop() {
        std::unique_lock lock(mutex_);
        not_empty_.wait(lock, [this] { return closed_ || !items_.empty(); });
        return take_locked(lock);
    }

    /** @brief Non-blocking pop. @return Front element or `std::nullopt` if empty. */
    [[nodiscard]] std::optional<T> try_pop() {
        std::unique_lock lock(mutex_);
        return take_locked(lock);
    }

    /**
     * @brief Waits at most `timeout` for an element.
     * @param timeout Maximum wait.
     * @return Front element, or `std::nullopt` on timeout / closed-and-empty.
     */
    template <typename Rep, typename Period>
    [[nodiscard]] std::optional<T> pop_for(const std::chrono::duration<Rep, Period>& timeout) {
        std::unique_lock lock(mutex_);
        not_empty_.wait_for(lock, timeout, [this] { return closed_ || !items_.empty(); });
        return take_locked(lock);
    }

    /** @brief Closes the queue and wakes every waiter. Idempotent. */
    void close() {
        {
            std::lock_guard lock(mutex_);
            closed_ = true;
        }
        not_full_.notify_all();
        not_empty_.notify_all();
    }

    /** @brief @return Number of buffered elements (a snapshot). */
    [[nodiscard]] std::size_t size() const {
        std::lock_guard lock(mutex_);
        return items_.size();
    }
    /** @brief @return true if no element is buffered (a snapshot). */
    [[nodiscard]] bool empty() const { return size() == 0; }
    /** @brief @return Maximum number of buffered elements. */
    [[nodiscard]] std::size_t capacity() const noexcept { return capacity_; }
    /** @brief @return true once `close()` has been called. */
    [[nodiscard]] bool is_closed() const {
        std::lock_guard lock(mutex_);
        return closed_;
    }

private:
    template <typename U>
    bool emplace_locked(std::unique_lock<std::mutex>& lock, U&& value) {
        if (closed_) {
            return false;
        }
        items_.emplace_back(std::forward<U>(value));
        lock.unlock();
        not_empty_.notify_one();
        return true;
    }

    std::optional<T> take_locked(std::unique_lock<std::mutex>& lock) {
        if (items_.empty()) {
            return std::nullopt;
        }
        std::optional<T> value(std::move(items_.front()));
        items_.pop_front();
        lock.unlock();
        not_full_.notify_one();
        return value;
    }

    const std::size_t capacity_;
    mutable std::mutex mutex_;
    std::condition_variable not_full_;
    std::condition_variable not_empty_;
    std::deque<T> items_;
    bool closed_ = false;
};

/**
 * @brief Counting semaphore (cf. `std::counting_semaphore`) built on a condition variable.
 */
class CountingSemaphore {
public:
    /**
     * @brief Creates the semaphore.
     * @param initial Initial number of permits.
     */
    explicit CountingSemaphore(std::size_t initial = 0) noexcept : count_(initial) {}

    /** @brief Blocks until a permit is available and takes it. */
    void acquire();
    /** @brief @return true if a permit was taken without blocking. */
    [[nodiscard]] bool try_acquire();
    /**
     * @brief Waits at most `timeout` for a permit.
     * @param timeout Maximum wait.
     * @return true if a permit was taken.
     */
    [[nodiscard]] bool try_acquire_for(std::chrono::nanoseconds timeout);
    /**
     * @brief Returns permits and wakes waiters.
     * @param permits Number of permits to add.
     */
    void release(std::size_t permits = 1);
    /** @brief @return Currently available permits (a snapshot). */
    [[nodiscard]] std::size_t available() const;

private:
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::size_t count_;
};

/**
 * @brief Single-use countdown latch (cf. `std::latch`).
 */
class CountDownLatch {
public:
    /**
     * @brief Creates the latch.
     * @param count Number of `count_down()` calls needed to open it.
     */
    explicit CountDownLatch(std::size_t count) noexcept : count_(count) {}

    /**
     * @brief Decrements the counter (saturating at zero); opens the latch at zero.
     * @param n Amount to subtract.
     */
    void count_down(std::size_t n = 1);
    /** @brief Blocks until the counter reaches zero. */
    void wait() const;
    /**
     * @brief Waits at most `timeout` for the latch to open.
     * @param timeout Maximum wait.
     * @return true if open.
     */
    [[nodiscard]] bool wait_for(std::chrono::nanoseconds timeout) const;
    /** @brief @return true if the counter is zero. */
    [[nodiscard]] bool try_wait() const;
    /** @brief Equivalent to `count_down(); wait();`. */
    void arrive_and_wait();
    /** @brief @return Remaining count (a snapshot). */
    [[nodiscard]] std::size_t count() const;

private:
    mutable std::mutex mutex_;
    mutable std::condition_variable cv_;
    std::size_t count_;
};

/**
 * @brief Reusable barrier for a fixed number of parties (cf. `std::barrier`).
 *
 * Uses a generation counter so that a fast thread re-entering the next phase cannot
 * be confused with the stragglers of the previous phase.
 */
class CyclicBarrier {
public:
    /**
     * @brief Creates the barrier.
     * @param parties Number of threads that must arrive per phase (must be > 0).
     * @param on_completion Run exactly once per phase by the last arriving thread,
     *        before any waiter is released.
     * @throws std::invalid_argument if `parties == 0`.
     */
    explicit CyclicBarrier(std::size_t parties, std::function<void()> on_completion = {});

    /**
     * @brief Arrives at the barrier and blocks until all parties of this phase have arrived.
     * @return The index of the completed phase (0, 1, 2, ...).
     */
    std::size_t arrive_and_wait();
    /** @brief @return Number of parties per phase. */
    [[nodiscard]] std::size_t parties() const noexcept { return parties_; }
    /** @brief @return Number of completed phases. */
    [[nodiscard]] std::size_t generation() const;

private:
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    const std::size_t parties_;
    std::size_t waiting_ = 0;
    std::size_t generation_ = 0;
    std::function<void()> on_completion_;
};

/**
 * @brief Manual-reset event: once set, all current and future waiters pass until `reset()`.
 */
class ManualResetEvent {
public:
    /**
     * @brief Creates the event.
     * @param initially_set Initial state.
     */
    explicit ManualResetEvent(bool initially_set = false) noexcept : set_(initially_set) {}
    /** @brief Signals the event and wakes all waiters. */
    void set();
    /** @brief Returns the event to the non-signalled state. */
    void reset();
    /** @brief Blocks until the event is set. */
    void wait() const;
    /**
     * @brief Waits at most `timeout`.
     * @param timeout Maximum wait.
     * @return true if the event is set.
     */
    [[nodiscard]] bool wait_for(std::chrono::nanoseconds timeout) const;
    /** @brief @return Current state. */
    [[nodiscard]] bool is_set() const;

private:
    mutable std::mutex mutex_;
    mutable std::condition_variable cv_;
    bool set_;
};

/**
 * @brief Blocking pool of reusable objects (connections, buffers...) handed out as RAII leases.
 *
 * The pool must outlive every lease obtained from it.
 * @tparam T Pooled object type.
 */
template <typename T>
class ResourcePool {
public:
    /**
     * @brief Move-only RAII handle; returns the resource to the pool on destruction.
     */
    class Lease {
    public:
        Lease(const Lease&) = delete;
        Lease& operator=(const Lease&) = delete;
        /** @brief Transfers ownership of the lease. */
        Lease(Lease&& other) noexcept : pool_(std::exchange(other.pool_, nullptr)), index_(other.index_) {}
        /** @brief Releases the current resource, then takes over `other`'s. */
        Lease& operator=(Lease&& other) noexcept {
            if (this != &other) {
                reset();
                pool_ = std::exchange(other.pool_, nullptr);
                index_ = other.index_;
            }
            return *this;
        }
        /** @brief Returns the resource to the pool. */
        ~Lease() { reset(); }

        /** @brief @return Reference to the leased resource. */
        [[nodiscard]] T& operator*() const noexcept { return pool_->resources_[index_]; }
        /** @brief @return Pointer to the leased resource. */
        [[nodiscard]] T* operator->() const noexcept { return &pool_->resources_[index_]; }
        /** @brief @return Slot index of the resource inside the pool. */
        [[nodiscard]] std::size_t index() const noexcept { return index_; }
        /** @brief Returns the resource early; the lease becomes empty. */
        void reset() noexcept {
            if (pool_ != nullptr) {
                pool_->give_back(index_);
                pool_ = nullptr;
            }
        }

    private:
        friend class ResourcePool;
        Lease(ResourcePool* pool, std::size_t index) noexcept : pool_(pool), index_(index) {}
        ResourcePool* pool_;
        std::size_t index_;
    };

    /**
     * @brief Takes ownership of the pooled objects.
     * @param resources Objects to pool (must be non-empty).
     * @throws std::invalid_argument if empty.
     */
    explicit ResourcePool(std::vector<T> resources) : resources_(std::move(resources)) {
        if (resources_.empty()) {
            throw std::invalid_argument("ResourcePool requires at least one resource");
        }
        free_.reserve(resources_.size());
        for (std::size_t i = resources_.size(); i-- > 0;) {
            free_.push_back(i);
        }
    }

    ResourcePool(const ResourcePool&) = delete;
    ResourcePool& operator=(const ResourcePool&) = delete;
    ResourcePool(ResourcePool&&) = delete;
    ResourcePool& operator=(ResourcePool&&) = delete;
    ~ResourcePool() = default;

    /** @brief Blocks until a resource is free. @return Lease on it. */
    [[nodiscard]] Lease acquire() {
        std::unique_lock lock(mutex_);
        cv_.wait(lock, [this] { return !free_.empty(); });
        return take_locked();
    }

    /**
     * @brief Waits at most `timeout` for a resource.
     * @param timeout Maximum wait.
     * @return Lease, or `std::nullopt` on timeout.
     */
    template <typename Rep, typename Period>
    [[nodiscard]] std::optional<Lease> try_acquire_for(const std::chrono::duration<Rep, Period>& timeout) {
        std::unique_lock lock(mutex_);
        if (!cv_.wait_for(lock, timeout, [this] { return !free_.empty(); })) {
            return std::nullopt;
        }
        return take_locked();
    }

    /** @brief @return Number of free resources (a snapshot). */
    [[nodiscard]] std::size_t available() const {
        std::lock_guard lock(mutex_);
        return free_.size();
    }
    /** @brief @return Total number of pooled resources. */
    [[nodiscard]] std::size_t size() const noexcept { return resources_.size(); }

private:
    Lease take_locked() {
        const std::size_t index = free_.back();
        free_.pop_back();
        return Lease(this, index);
    }

    void give_back(std::size_t index) noexcept {
        {
            std::lock_guard lock(mutex_);
            free_.push_back(index); // capacity reserved up front: cannot throw
        }
        cv_.notify_one();
    }

    std::vector<T> resources_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::vector<std::size_t> free_;
};

/**
 * @brief Showcase: producer/consumer pipeline, latch start signal, barrier phases, resource pool.
 * @param out Destination stream.
 */
void demonstrate_condition_variables(std::ostream& out = std::cout);

} // namespace CppVerseHub::Concurrency

#endif // CPPVERSEHUB_CONCURRENCY_CONDITIONALVARIABLES_HPP
