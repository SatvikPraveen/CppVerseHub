/**
 * @file CoroutinesDemo.hpp
 * @brief C++20 coroutines: a lazy `Generator<T>`, a lazy `Task<T>`, `sync_wait`, a
 *        cooperative scheduler and an awaitable that hops onto a `ThreadPool`.
 * @details File location: src/concurrency/CoroutinesDemo.hpp
 *
 * C++20 ships the coroutine *machinery* but almost no coroutine *types*; this header
 * builds the two fundamental ones and shows the design points that matter:
 *
 *  - `Generator<T>` is a synchronous, lazily evaluated input range. It stores a pointer
 *    to the yielded object (no copy), forbids `co_await` inside the body, and re-throws
 *    exceptions from the body to the consumer on the next increment.
 *  - `Task<T>` is lazy (starts when awaited) and uses *symmetric transfer*
 *    (`await_suspend` returning a handle) so deep `co_await` chains do not grow the stack.
 *  - `sync_wait()` bridges coroutine and blocking worlds; its completion signal is raised
 *    only *after* the wrapper coroutine has suspended, so destroying the frame is safe.
 *  - `RoundRobinScheduler` interleaves tasks deterministically on one thread via `yield()`.
 *  - `schedule_on(pool)` resumes the awaiting coroutine on a `ThreadPool` worker.
 *
 * Portability: requires C++20 coroutine support (GCC 11+, Clang 14+, MSVC 19.28+).
 */

#ifndef CPPVERSEHUB_CONCURRENCY_COROUTINESDEMO_HPP
#define CPPVERSEHUB_CONCURRENCY_COROUTINESDEMO_HPP

#include "concurrency/ThreadPool.hpp"

#include <condition_variable>
#include <coroutine>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <exception>
#include <iostream>
#include <iterator>
#include <memory>
#include <mutex>
#include <optional>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace CppVerseHub::Concurrency {

// =====================================================================================
// Generator
// =====================================================================================

/**
 * @brief Lazily evaluated, move-only, single-pass coroutine generator (an input range).
 *
 * @code
 * Generator<int> count_to(int n) { for (int i = 1; i <= n; ++i) co_yield i; }
 * for (int v : count_to(3)) { ... }   // 1 2 3
 * @endcode
 * @tparam T Yielded value type.
 */
template <typename T>
class [[nodiscard]] Generator {
public:
    using value_type = std::remove_cvref_t<T>; ///< Element type.
    using reference = const value_type&;       ///< Type returned by dereferencing.

    /** @brief Coroutine promise: holds a pointer to the current yielded object. */
    struct promise_type {
        const value_type* current = nullptr;
        std::exception_ptr error;

        Generator get_return_object() noexcept {
            return Generator{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_always initial_suspend() const noexcept { return {}; }
        std::suspend_always final_suspend() const noexcept { return {}; }
        // The yielded object (even a temporary) lives until the coroutine is resumed,
        // so storing its address is safe and avoids a copy.
        std::suspend_always yield_value(const value_type& value) noexcept {
            current = std::addressof(value);
            return {};
        }
        std::suspend_always yield_value(value_type&& value) noexcept {
            current = std::addressof(value);
            return {};
        }
        void return_void() const noexcept {}
        void unhandled_exception() noexcept { error = std::current_exception(); }
        /// Generators are synchronous: `co_await` inside a generator body is ill-formed.
        template <typename U>
        std::suspend_never await_transform(U&&) = delete;

        void rethrow_if_failed() {
            if (error) {
                std::rethrow_exception(std::exchange(error, nullptr));
            }
        }
    };

    using handle_type = std::coroutine_handle<promise_type>; ///< Underlying handle type.

    /** @brief Input iterator over the generated sequence. */
    class iterator {
    public:
        using iterator_concept = std::input_iterator_tag;  ///< C++20 iterator concept.
        using iterator_category = std::input_iterator_tag; ///< Legacy iterator category.
        using difference_type = std::ptrdiff_t;            ///< Required by input_iterator.
        using value_type = Generator::value_type;          ///< Element type.
        using reference = Generator::reference;            ///< Dereference type.
        using pointer = const value_type*;                 ///< Arrow type.

        /** @brief Singular iterator (compares equal to the end sentinel). */
        iterator() noexcept = default;
        /** @brief @param handle Generator coroutine. */
        explicit iterator(handle_type handle) noexcept : handle_(handle) {}

        /** @brief @return The current element. */
        [[nodiscard]] reference operator*() const noexcept { return *handle_.promise().current; }
        /** @brief @return Pointer to the current element. */
        [[nodiscard]] pointer operator->() const noexcept { return handle_.promise().current; }

        /** @brief Resumes the coroutine; re-throws if its body threw. @return *this. */
        iterator& operator++() {
            handle_.resume();
            if (handle_.done()) {
                handle_.promise().rethrow_if_failed();
            }
            return *this;
        }
        /** @brief Post-increment (input iterators return void). */
        void operator++(int) { ++*this; }

        /** @brief @return true when the coroutine has run to completion. */
        friend bool operator==(const iterator& it, std::default_sentinel_t) noexcept {
            return !it.handle_ || it.handle_.done();
        }

    private:
        handle_type handle_{};
    };

    Generator(const Generator&) = delete;
    Generator& operator=(const Generator&) = delete;
    /** @brief Transfers ownership of the coroutine frame. */
    Generator(Generator&& other) noexcept : handle_(std::exchange(other.handle_, {})) {}
    /** @brief Destroys the current frame and takes ownership of `other`'s. */
    Generator& operator=(Generator&& other) noexcept {
        if (this != &other) {
            if (handle_) {
                handle_.destroy();
            }
            handle_ = std::exchange(other.handle_, {});
        }
        return *this;
    }
    /** @brief Destroys the coroutine frame (and every local in it). */
    ~Generator() {
        if (handle_) {
            handle_.destroy();
        }
    }

    /**
     * @brief Starts the coroutine and returns an iterator to the first element.
     * @return Iterator; call at most once (single pass).
     */
    [[nodiscard]] iterator begin() {
        if (handle_) {
            handle_.resume();
            if (handle_.done()) {
                handle_.promise().rethrow_if_failed();
            }
        }
        return iterator{handle_};
    }
    /** @brief @return End sentinel. */
    [[nodiscard]] std::default_sentinel_t end() const noexcept { return {}; }

private:
    explicit Generator(handle_type handle) noexcept : handle_(handle) {}
    handle_type handle_;
};

/**
 * @brief Yields the half-open integer range [first, last).
 * @param first First value.
 * @param last One past the last value.
 * @return Generator of the range.
 */
[[nodiscard]] Generator<int> iota_range(int first, int last);

/**
 * @brief Infinite Fibonacci sequence 0, 1, 1, 2, 3, ... (stops before overflowing 64 bits).
 * @return Generator of Fibonacci numbers.
 */
[[nodiscard]] Generator<std::uint64_t> fibonacci();

/**
 * @brief Collatz trajectory of `start` down to 1 (inclusive).
 * @param start Starting value (> 0).
 * @return Generator of the trajectory.
 * @throws std::invalid_argument (on first iteration) if `start == 0`.
 */
[[nodiscard]] Generator<std::uint64_t> collatz(std::uint64_t start);

/**
 * @brief Yields at most `count` elements of `source`.
 * @param source Generator to draw from (consumed).
 * @param count Maximum number of elements.
 * @return Truncated generator.
 */
template <typename T>
[[nodiscard]] Generator<T> take(Generator<T> source, std::size_t count) {
    if (count == 0) {
        co_return;
    }
    std::size_t produced = 0;
    for (const auto& value : source) {
        co_yield value;
        if (++produced == count) {
            co_return;
        }
    }
}

/**
 * @brief Yields the elements of `source` that satisfy `pred`.
 * @param source Generator to draw from (consumed).
 * @param pred Predicate (copied into the coroutine frame).
 * @return Filtered generator.
 */
template <typename T, typename Pred>
[[nodiscard]] Generator<T> filter(Generator<T> source, Pred pred) {
    for (const auto& value : source) {
        if (pred(value)) {
            co_yield value;
        }
    }
}

// =====================================================================================
// Task
// =====================================================================================

template <typename T = void>
class Task;

namespace detail {

/** @brief State shared by every `Task` promise: continuation + captured exception. */
struct TaskPromiseBase {
    std::coroutine_handle<> continuation = std::noop_coroutine();
    std::exception_ptr error;

    struct FinalAwaiter {
        [[nodiscard]] bool await_ready() const noexcept { return false; }
        // Symmetric transfer: resume whoever awaited us without growing the stack.
        template <typename Promise>
        std::coroutine_handle<> await_suspend(std::coroutine_handle<Promise> handle) const noexcept {
            return handle.promise().continuation;
        }
        void await_resume() const noexcept {}
    };

    std::suspend_always initial_suspend() const noexcept { return {}; }
    FinalAwaiter final_suspend() const noexcept { return {}; }
    void unhandled_exception() noexcept { error = std::current_exception(); }
};

template <typename T>
struct TaskPromise : TaskPromiseBase {
    std::optional<T> value;

    Task<T> get_return_object() noexcept;
    template <typename U>
        requires std::constructible_from<T, U&&>
    void return_value(U&& v) {
        value.emplace(std::forward<U>(v));
    }
    T take_result() {
        if (error) {
            std::rethrow_exception(error);
        }
        return std::move(*value);
    }
};

template <>
struct TaskPromise<void> : TaskPromiseBase {
    Task<void> get_return_object() noexcept;
    void return_void() const noexcept {}
    void take_result() const {
        if (error) {
            std::rethrow_exception(error);
        }
    }
};

} // namespace detail

/**
 * @brief Lazy, move-only, awaitable unit of asynchronous work producing a `T`.
 *
 * The body starts running only when the task is `co_await`ed (or passed to
 * `sync_wait` / `RoundRobinScheduler::spawn`). A task may be awaited once.
 * Exceptions thrown by the body are re-thrown from `co_await`.
 * @tparam T Result type (`void` allowed, references not).
 */
template <typename T>
class [[nodiscard]] Task {
    static_assert(!std::is_reference_v<T>, "Task<T&> is not supported; use std::reference_wrapper");

public:
    using promise_type = detail::TaskPromise<T>;             ///< Coroutine promise.
    using handle_type = std::coroutine_handle<promise_type>; ///< Underlying handle.

    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;
    /** @brief Transfers ownership of the frame. */
    Task(Task&& other) noexcept : handle_(std::exchange(other.handle_, {})) {}
    /** @brief Destroys the current frame and takes `other`'s. */
    Task& operator=(Task&& other) noexcept {
        if (this != &other) {
            if (handle_) {
                handle_.destroy();
            }
            handle_ = std::exchange(other.handle_, {});
        }
        return *this;
    }
    /** @brief Destroys the frame. */
    ~Task() {
        if (handle_) {
            handle_.destroy();
        }
    }

    /** @brief @return true if the body has finished. */
    [[nodiscard]] bool done() const noexcept { return !handle_ || handle_.done(); }

    /** @brief Awaiter: starts the task and resumes the awaiting coroutine when it finishes. */
    struct Awaiter {
        handle_type handle;
        [[nodiscard]] bool await_ready() const noexcept { return handle.done(); }
        std::coroutine_handle<> await_suspend(std::coroutine_handle<> awaiting) noexcept {
            handle.promise().continuation = awaiting;
            return handle;
        }
        T await_resume() { return handle.promise().take_result(); }
    };

    /** @brief @return Awaiter for `co_await task`. @pre The task is valid. */
    Awaiter operator co_await() & noexcept { return Awaiter{handle_}; }
    /** @brief @return Awaiter for `co_await std::move(task)`. @pre The task is valid. */
    Awaiter operator co_await() && noexcept { return Awaiter{handle_}; }

private:
    friend promise_type;
    explicit Task(handle_type handle) noexcept : handle_(handle) {}
    handle_type handle_;
};

namespace detail {

template <typename T>
Task<T> TaskPromise<T>::get_return_object() noexcept {
    return Task<T>{std::coroutine_handle<TaskPromise<T>>::from_promise(*this)};
}

inline Task<void> TaskPromise<void>::get_return_object() noexcept {
    return Task<void>{std::coroutine_handle<TaskPromise<void>>::from_promise(*this)};
}

/** @brief One-shot blocking signal; `set()` notifies under the lock so the waiter may
 *         destroy the event as soon as `wait()` returns. */
class SyncWaitEvent {
public:
    void set() noexcept {
        std::lock_guard lock(mutex_);
        done_ = true;
        cv_.notify_all();
    }
    void wait() {
        std::unique_lock lock(mutex_);
        cv_.wait(lock, [this] { return done_; });
    }

private:
    std::mutex mutex_;
    std::condition_variable cv_;
    bool done_ = false;
};

/** @brief Eager wrapper coroutine used by `sync_wait`. */
template <typename R>
class SyncWaitTask {
public:
    struct promise_type {
        SyncWaitEvent* event = nullptr;
        std::optional<R> value;
        std::exception_ptr error;

        SyncWaitTask get_return_object() noexcept {
            return SyncWaitTask{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_always initial_suspend() const noexcept { return {}; }
        auto final_suspend() const noexcept {
            struct Signal {
                [[nodiscard]] bool await_ready() const noexcept { return false; }
                // Runs after the coroutine is suspended: the waiter may now destroy it.
                void await_suspend(std::coroutine_handle<promise_type> h) const noexcept {
                    h.promise().event->set();
                }
                void await_resume() const noexcept {}
            };
            return Signal{};
        }
        template <typename U>
        void return_value(U&& v) {
            value.emplace(std::forward<U>(v));
        }
        void unhandled_exception() noexcept { error = std::current_exception(); }
    };

    SyncWaitTask(const SyncWaitTask&) = delete;
    SyncWaitTask& operator=(const SyncWaitTask&) = delete;
    SyncWaitTask(SyncWaitTask&& other) noexcept : handle_(std::exchange(other.handle_, {})) {}
    SyncWaitTask& operator=(SyncWaitTask&&) = delete;
    ~SyncWaitTask() {
        if (handle_) {
            handle_.destroy();
        }
    }

    R run() {
        SyncWaitEvent event;
        handle_.promise().event = &event;
        handle_.resume();
        event.wait();
        if (handle_.promise().error) {
            std::rethrow_exception(handle_.promise().error);
        }
        return std::move(*handle_.promise().value);
    }

private:
    explicit SyncWaitTask(std::coroutine_handle<promise_type> h) noexcept : handle_(h) {}
    std::coroutine_handle<promise_type> handle_;
};

template <typename T>
using SyncWaitStorage = std::conditional_t<std::is_void_v<T>, std::monostate, T>;

template <typename T>
SyncWaitTask<SyncWaitStorage<T>> make_sync_wait_task(Task<T> task) {
    if constexpr (std::is_void_v<T>) {
        co_await std::move(task);
        co_return std::monostate{};
    } else {
        co_return co_await std::move(task);
    }
}

} // namespace detail

/**
 * @brief Runs a task to completion, blocking the calling thread.
 *
 * The task may hop to other threads (e.g. via `schedule_on`); the caller is woken when
 * it completes. Must not be called from a thread the task needs in order to finish.
 * @param task Task to run (consumed).
 * @return The task's result; its exception is re-thrown.
 */
template <typename T>
T sync_wait(Task<T> task) {
    auto wrapper = detail::make_sync_wait_task(std::move(task));
    if constexpr (std::is_void_v<T>) {
        static_cast<void>(wrapper.run());
    } else {
        return wrapper.run();
    }
}

/**
 * @brief Awaitable that resumes the awaiting coroutine on a `ThreadPool` worker.
 *
 * @code co_await schedule_on(pool);  // execution continues on a pool thread @endcode
 */
class ScheduleOnAwaiter {
public:
    /** @brief @param pool Target pool (must outlive the suspension). */
    explicit ScheduleOnAwaiter(ThreadPool& pool) noexcept : pool_(&pool) {}
    /** @brief @return false: always suspend. */
    [[nodiscard]] bool await_ready() const noexcept { return false; }
    /**
     * @brief Posts the resumption to the pool.
     * @param handle Awaiting coroutine.
     * @throws PoolShutdownError if the pool is shut down (propagates out of `co_await`).
     */
    void await_suspend(std::coroutine_handle<> handle) const {
        pool_->post(UniqueTask([handle] { handle.resume(); }));
    }
    /** @brief Nothing to return. */
    void await_resume() const noexcept {}

private:
    ThreadPool* pool_;
};

/**
 * @brief @param pool Pool to resume on. @return Awaitable that transfers execution to `pool`.
 */
[[nodiscard]] inline ScheduleOnAwaiter schedule_on(ThreadPool& pool) noexcept {
    return ScheduleOnAwaiter{pool};
}

/**
 * @brief Single-threaded cooperative scheduler with deterministic round-robin interleaving.
 *
 * Spawned tasks run only inside `run()`. Inside a task, `co_await scheduler.yield()` moves
 * the task to the back of the ready queue. Exceptions escaping a task are counted, not
 * propagated. Not thread safe: use from one thread.
 */
class RoundRobinScheduler {
public:
    RoundRobinScheduler() = default;
    RoundRobinScheduler(const RoundRobinScheduler&) = delete;
    RoundRobinScheduler& operator=(const RoundRobinScheduler&) = delete;
    RoundRobinScheduler(RoundRobinScheduler&&) = delete;
    RoundRobinScheduler& operator=(RoundRobinScheduler&&) = delete;
    /** @brief Destroys every remaining (possibly unfinished) task. */
    ~RoundRobinScheduler();

    /**
     * @brief Registers a task; it starts at the next `run()`.
     * @param task Task to run (consumed).
     */
    void spawn(Task<void> task);

    /** @brief Awaitable returned by `yield()`. */
    struct YieldAwaiter {
        RoundRobinScheduler* scheduler;
        [[nodiscard]] bool await_ready() const noexcept { return false; }
        void await_suspend(std::coroutine_handle<> handle) const { scheduler->ready_.push_back(handle); }
        void await_resume() const noexcept {}
    };

    /** @brief @return Awaitable that re-queues the current task behind the others. */
    [[nodiscard]] YieldAwaiter yield() noexcept { return YieldAwaiter{this}; }

    /**
     * @brief Resumes ready coroutines until none is left.
     * @return Number of resumptions performed.
     */
    std::size_t run();

    /** @brief @return Number of spawned tasks that have not finished. */
    [[nodiscard]] std::size_t live_tasks() const noexcept { return drivers_.size(); }
    /** @brief @return Number of tasks that finished by throwing. */
    [[nodiscard]] std::size_t failed_tasks() const noexcept { return failures_; }

private:
    std::deque<std::coroutine_handle<>> ready_;
    std::vector<std::coroutine_handle<>> drivers_;
    std::size_t failures_ = 0;
};

/**
 * @brief Showcase: generators and combinators, task composition, sync_wait, thread hop,
 *        round-robin interleaving.
 * @param out Destination stream.
 */
void demonstrate_coroutines(std::ostream& out = std::cout);

} // namespace CppVerseHub::Concurrency

#endif // CPPVERSEHUB_CONCURRENCY_COROUTINESDEMO_HPP
