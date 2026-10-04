/**
 * @file ThreadPool.hpp
 * @brief Thread pools: FIFO, priority-ordered and work-stealing executors.
 * @details File location: src/concurrency/ThreadPool.hpp
 *
 * Demonstrates how to build a correct general-purpose executor on top of
 * `std::thread`, `std::mutex` and `std::condition_variable`:
 *
 *  - `submit()` type-erases any callable into a move-only task and returns a
 *    `std::future` that carries either the result or the thrown exception.
 *  - Shutdown is *graceful*: once `shutdown()` is called no new work is accepted,
 *    but every task that was accepted before is executed before the workers exit.
 *    No accepted task is ever silently dropped.
 *  - `PriorityThreadPool` orders work by priority and breaks ties FIFO with a
 *    monotonically increasing sequence number (not a timestamp, which can collide).
 *  - `WorkStealingThreadPool` gives each worker its own deque: the owner pushes and
 *    pops at the back (LIFO, cache friendly), idle workers steal from the front
 *    (FIFO, takes the oldest and typically largest piece of work).
 */

#ifndef CPPVERSEHUB_CONCURRENCY_THREADPOOL_HPP
#define CPPVERSEHUB_CONCURRENCY_THREADPOOL_HPP

#include <algorithm>
#include <atomic>
#include <concepts>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace CppVerseHub::Concurrency {

    /**
     * @brief Thrown when work is submitted to a pool that has been shut down.
     */
    class PoolShutdownError : public std::runtime_error {
    public:
        /** @brief Constructs the error with a fixed diagnostic message. */
        PoolShutdownError() : std::runtime_error("thread pool has been shut down") {}
    };

    /**
     * @brief Move-only, type-erased `void()` callable (a minimal `std::move_only_function`).
     *
     * `std::function` requires copyable targets, which rules out `std::packaged_task`
     * and lambdas that capture `std::unique_ptr`. This wrapper only requires move.
     */
    class UniqueTask {
    public:
        /** @brief Constructs an empty task. */
        UniqueTask() noexcept = default;

        /**
         * @brief Wraps an arbitrary callable.
         * @param fn Callable invocable as `fn()`; it is moved or copied into the task.
         */
        template <typename F>
            requires(!std::same_as<std::remove_cvref_t<F>, UniqueTask> && std::invocable<std::decay_t<F>&>)
        explicit UniqueTask(F&& fn) : impl_(std::make_unique<Model<std::decay_t<F>>>(std::forward<F>(fn))) {}

        /**
         * @brief Invokes the wrapped callable.
         * @pre The task is non-empty.
         */
        void operator()() { impl_->call(); }

        /** @brief @return true if the task holds a callable. */
        [[nodiscard]] explicit operator bool() const noexcept { return impl_ != nullptr; }

    private:
        struct Concept {
            Concept() = default;
            Concept(const Concept&) = delete;
            Concept& operator=(const Concept&) = delete;
            Concept(Concept&&) = delete;
            Concept& operator=(Concept&&) = delete;
            virtual ~Concept() = default;
            virtual void call() = 0;
        };

        template <typename F>
        struct Model final : Concept {
            template <typename G>
            explicit Model(G&& g) : fn(std::forward<G>(g)) {}
            void call() override { std::invoke(fn); }
            F fn;
        };

        std::unique_ptr<Concept> impl_;
    };

    namespace detail {

        /**
         * @brief Binds a callable and its arguments into a `UniqueTask` plus a matching future.
         * @param fn Callable.
         * @param args Arguments, decay-copied (like `std::thread`).
         * @return Pair of (task to run, future for its result).
         */
        template <typename F, typename... Args>
        [[nodiscard]] auto package_task(F&& fn, Args&&... args) {
            using Result = std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>;
            std::packaged_task<Result()> task(
                [f = std::forward<F>(fn), ... bound = std::forward<Args>(args)]() mutable -> Result {
                    return std::invoke(std::move(f), std::move(bound)...);
                });
            std::future<Result> future = task.get_future();
            return std::pair<UniqueTask, std::future<Result>>{UniqueTask(std::move(task)), std::move(future)};
        }

        /** @brief @return `max(1, std::thread::hardware_concurrency())`. */
        [[nodiscard]] inline std::size_t default_thread_count() noexcept {
            return std::max<std::size_t>(1, std::thread::hardware_concurrency());
        }

        /**
         * @brief Shared worker machinery parameterised on the queue discipline.
         *
         * `Queue` must provide `push(Ts...)`, `pop() -> UniqueTask`, `empty()` and `size()`.
         * All queue operations happen under `mutex_`, so the queue need not be thread safe.
         */
        template <typename Queue>
        class PoolCore {
        public:
            explicit PoolCore(std::size_t thread_count) {
                const std::size_t n = std::max<std::size_t>(1, thread_count);
                workers_.reserve(n);
                try {
                    for (std::size_t i = 0; i < n; ++i) {
                        workers_.emplace_back([this] { worker_loop(); });
                    }
                } catch (...) {
                    shutdown();
                    throw;
                }
            }

            PoolCore(const PoolCore&) = delete;
            PoolCore& operator=(const PoolCore&) = delete;
            PoolCore(PoolCore&&) = delete;
            PoolCore& operator=(PoolCore&&) = delete;
            ~PoolCore() { shutdown(); }

            template <typename... QArgs>
            void push(QArgs&&... qargs) {
                {
                    std::lock_guard lock(mutex_);
                    if (stopping_) {
                        throw PoolShutdownError{};
                    }
                    queue_.push(std::forward<QArgs>(qargs)...);
                }
                work_cv_.notify_one();
            }

            void wait_idle() {
                std::unique_lock lock(mutex_);
                idle_cv_.wait(lock, [this] { return active_ == 0 && queue_.empty(); });
            }

            void shutdown() {
                {
                    std::lock_guard lock(mutex_);
                    stopping_ = true;
                }
                work_cv_.notify_all();
                std::lock_guard join_lock(join_mutex_);
                for (auto& worker : workers_) {
                    if (worker.joinable()) {
                        worker.join();
                    }
                }
            }

            [[nodiscard]] std::size_t thread_count() const noexcept { return workers_.size(); }

            [[nodiscard]] std::size_t pending() const {
                std::lock_guard lock(mutex_);
                return queue_.size();
            }

            [[nodiscard]] bool is_shutdown() const {
                std::lock_guard lock(mutex_);
                return stopping_;
            }

            [[nodiscard]] std::uint64_t completed() const noexcept {
                return completed_.load(std::memory_order_relaxed);
            }

        private:
            void worker_loop() {
                for (;;) {
                    UniqueTask task;
                    {
                        std::unique_lock lock(mutex_);
                        work_cv_.wait(lock, [this] { return stopping_ || !queue_.empty(); });
                        if (queue_.empty()) {
                            return;  // stopping_ and fully drained: graceful exit
                        }
                        task = queue_.pop();
                        ++active_;
                    }
                    try {
                        task();
                    } catch (...) {
                        // Tasks created by submit() never throw (packaged_task captures the
                        // exception into the future); anything else is swallowed so a single
                        // faulty task cannot terminate the process.
                    }
                    completed_.fetch_add(1, std::memory_order_relaxed);
                    {
                        std::lock_guard lock(mutex_);
                        --active_;
                        if (active_ == 0 && queue_.empty()) {
                            idle_cv_.notify_all();
                        }
                    }
                }
            }

            mutable std::mutex mutex_;
            std::condition_variable work_cv_;
            std::condition_variable idle_cv_;
            Queue queue_;
            std::size_t active_ = 0;
            bool stopping_ = false;
            std::atomic<std::uint64_t> completed_{0};
            std::mutex join_mutex_;
            std::vector<std::thread> workers_;
        };

        /** @brief FIFO queue discipline for `PoolCore`. */
        class FifoQueue {
        public:
            void push(UniqueTask task) { tasks_.push_back(std::move(task)); }
            [[nodiscard]] UniqueTask pop() {
                UniqueTask task = std::move(tasks_.front());
                tasks_.pop_front();
                return task;
            }
            [[nodiscard]] bool empty() const noexcept { return tasks_.empty(); }
            [[nodiscard]] std::size_t size() const noexcept { return tasks_.size(); }

        private:
            std::deque<UniqueTask> tasks_;
        };

    }  // namespace detail

    /**
     * @brief Fixed-size FIFO thread pool with future-returning `submit()` and graceful shutdown.
     *
     * Thread-safety: all member functions may be called concurrently, except that
     * `shutdown()`, `wait_idle()` and the destructor must not be called from one of the
     * pool's own worker threads (that would deadlock waiting for itself).
     */
    class ThreadPool {
    public:
        /**
         * @brief Starts the worker threads.
         * @param thread_count Number of workers; 0 is treated as 1.
         */
        explicit ThreadPool(std::size_t thread_count = detail::default_thread_count()) : core_(thread_count) {}

        /**
         * @brief Schedules `fn(args...)` for execution.
         * @param fn Callable to run on a worker.
         * @param args Arguments, decay-copied into the task.
         * @return Future that receives the return value or the exception thrown by `fn`.
         * @throws PoolShutdownError if `shutdown()` has already been called.
         */
        template <typename F, typename... Args>
            requires std::invocable<std::decay_t<F>, std::decay_t<Args>...>
        [[nodiscard]] auto submit(F&& fn, Args&&... args)
            -> std::future<std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>> {
            auto [task, future] = detail::package_task(std::forward<F>(fn), std::forward<Args>(args)...);
            core_.push(std::move(task));
            return std::move(future);
        }

        /**
         * @brief Fire-and-forget scheduling of a raw task (used e.g. to resume coroutines).
         * @param task Task to execute; exceptions escaping it are swallowed.
         * @throws PoolShutdownError if the pool is shut down.
         */
        void post(UniqueTask task) { core_.push(std::move(task)); }

        /** @brief Blocks until the queue is empty and no worker is running a task. */
        void wait_idle() { core_.wait_idle(); }

        /**
         * @brief Stops accepting work, runs every already-accepted task, joins the workers.
         *
         * Idempotent and safe to call concurrently from several non-worker threads.
         */
        void shutdown() { core_.shutdown(); }

        /** @brief @return Number of worker threads. */
        [[nodiscard]] std::size_t thread_count() const noexcept { return core_.thread_count(); }
        /** @brief @return Number of queued tasks not yet picked up by a worker. */
        [[nodiscard]] std::size_t pending_tasks() const { return core_.pending(); }
        /** @brief @return true once `shutdown()` has been initiated. */
        [[nodiscard]] bool is_shutdown() const { return core_.is_shutdown(); }
        /** @brief @return Number of tasks that have finished executing. */
        [[nodiscard]] std::uint64_t completed_tasks() const noexcept { return core_.completed(); }

    private:
        detail::PoolCore<detail::FifoQueue> core_;
    };

    /** @brief Scheduling priority for `PriorityThreadPool`. Higher values run first. */
    enum class TaskPriority : std::uint8_t { Low = 0, Normal = 1, High = 2, Critical = 3 };

    namespace detail {

        /** @brief Max-heap by priority, FIFO among equal priorities (stable via sequence number). */
        class PriorityQueue {
        public:
            void push(TaskPriority priority, UniqueTask task) {
                heap_.push_back(Entry{priority, next_sequence_++, std::move(task)});
                std::push_heap(heap_.begin(), heap_.end(), Compare{});
            }
            [[nodiscard]] UniqueTask pop() {
                std::pop_heap(heap_.begin(), heap_.end(), Compare{});
                UniqueTask task = std::move(heap_.back().task);
                heap_.pop_back();
                return task;
            }
            [[nodiscard]] bool empty() const noexcept { return heap_.empty(); }
            [[nodiscard]] std::size_t size() const noexcept { return heap_.size(); }

        private:
            struct Entry {
                TaskPriority priority;
                std::uint64_t sequence;
                UniqueTask task;
            };
            struct Compare {
                // "a has lower precedence than b": lower priority, or same priority but submitted later.
                bool operator()(const Entry& a, const Entry& b) const noexcept {
                    if (a.priority != b.priority) {
                        return a.priority < b.priority;
                    }
                    return a.sequence > b.sequence;
                }
            };
            std::vector<Entry> heap_;
            std::uint64_t next_sequence_ = 0;
        };

    }  // namespace detail

    /**
     * @brief Thread pool that always dequeues the highest-priority pending task.
     *
     * Equal-priority tasks run in submission order. Same threading contract as `ThreadPool`.
     */
    class PriorityThreadPool {
    public:
        /**
         * @brief Starts the worker threads.
         * @param thread_count Number of workers; 0 is treated as 1.
         */
        explicit PriorityThreadPool(std::size_t thread_count = detail::default_thread_count())
            : core_(thread_count) {}

        /**
         * @brief Schedules `fn(args...)` with the given priority.
         * @param priority Scheduling priority.
         * @param fn Callable.
         * @param args Arguments, decay-copied.
         * @return Future for the result.
         * @throws PoolShutdownError if the pool is shut down.
         */
        template <typename F, typename... Args>
            requires std::invocable<std::decay_t<F>, std::decay_t<Args>...>
        [[nodiscard]] auto submit(TaskPriority priority, F&& fn, Args&&... args)
            -> std::future<std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>> {
            auto [task, future] = detail::package_task(std::forward<F>(fn), std::forward<Args>(args)...);
            core_.push(priority, std::move(task));
            return std::move(future);
        }

        /** @brief Blocks until the queue is empty and all workers are idle. */
        void wait_idle() { core_.wait_idle(); }
        /** @brief Graceful shutdown; see `ThreadPool::shutdown()`. */
        void shutdown() { core_.shutdown(); }
        /** @brief @return Number of worker threads. */
        [[nodiscard]] std::size_t thread_count() const noexcept { return core_.thread_count(); }
        /** @brief @return Number of queued tasks. */
        [[nodiscard]] std::size_t pending_tasks() const { return core_.pending(); }
        /** @brief @return true once shutdown has been initiated. */
        [[nodiscard]] bool is_shutdown() const { return core_.is_shutdown(); }
        /** @brief @return Number of finished tasks. */
        [[nodiscard]] std::uint64_t completed_tasks() const noexcept { return core_.completed(); }

    private:
        detail::PoolCore<detail::PriorityQueue> core_;
    };

    /**
     * @brief Work-stealing thread pool with one deque per worker.
     *
     * Tasks submitted from outside the pool are distributed round-robin; tasks submitted
     * from inside a worker go to that worker's own deque (back). A worker pops its own
     * deque LIFO and, when empty, steals FIFO from the others.
     *
     * Correctness argument for "no lost task / no lost wake-up":
     *  - `pending_` counts tasks that have been accepted but not yet popped. It is
     *    incremented under `sleep_mutex_` together with the `stopping_` check, so a task is
     *    either rejected or counted before shutdown can be observed.
     *  - A worker only sleeps after re-checking `pending_ == 0` under `sleep_mutex_`, and only
     *    exits when `stopping_ && pending_ == 0`, so every accepted task is executed.
     *
     * The per-deque locks keep this implementation simple and ThreadSanitizer-clean; a
     * Chase-Lev lock-free deque would be the next step for very fine-grained tasks.
     */
    class WorkStealingThreadPool {
    public:
        /**
         * @brief Starts the workers.
         * @param thread_count Number of workers; 0 is treated as 1.
         */
        explicit WorkStealingThreadPool(std::size_t thread_count = detail::default_thread_count());
        WorkStealingThreadPool(const WorkStealingThreadPool&) = delete;
        WorkStealingThreadPool& operator=(const WorkStealingThreadPool&) = delete;
        WorkStealingThreadPool(WorkStealingThreadPool&&) = delete;
        WorkStealingThreadPool& operator=(WorkStealingThreadPool&&) = delete;
        /** @brief Performs a graceful `shutdown()`. */
        ~WorkStealingThreadPool();

        /**
         * @brief Schedules `fn(args...)`.
         * @param fn Callable.
         * @param args Arguments, decay-copied.
         * @return Future for the result.
         * @throws PoolShutdownError if the pool is shut down.
         */
        template <typename F, typename... Args>
            requires std::invocable<std::decay_t<F>, std::decay_t<Args>...>
        [[nodiscard]] auto submit(F&& fn, Args&&... args)
            -> std::future<std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>> {
            auto [task, future] = detail::package_task(std::forward<F>(fn), std::forward<Args>(args)...);
            enqueue(std::move(task));
            return std::move(future);
        }

        /** @brief Graceful shutdown: executes every accepted task, then joins the workers. */
        void shutdown();

        /** @brief @return Number of worker threads. */
        [[nodiscard]] std::size_t thread_count() const noexcept { return queues_.size(); }
        /** @brief @return Number of accepted tasks not yet picked up. */
        [[nodiscard]] std::size_t pending_tasks() const noexcept { return pending_.load(); }
        /** @brief @return Number of tasks a worker took from another worker's deque. */
        [[nodiscard]] std::uint64_t steal_count() const noexcept {
            return steals_.load(std::memory_order_relaxed);
        }
        /** @brief @return Number of finished tasks. */
        [[nodiscard]] std::uint64_t completed_tasks() const noexcept {
            return completed_.load(std::memory_order_relaxed);
        }

    private:
        struct WorkerQueue {
            std::mutex mutex;
            std::deque<UniqueTask> tasks;
        };

        void enqueue(UniqueTask task);
        void worker_loop(std::size_t index);
        [[nodiscard]] UniqueTask try_acquire(std::size_t index);

        std::vector<std::unique_ptr<WorkerQueue>> queues_;
        std::atomic<std::size_t> pending_{0};
        std::atomic<std::size_t> next_queue_{0};
        std::atomic<std::uint64_t> steals_{0};
        std::atomic<std::uint64_t> completed_{0};
        std::mutex sleep_mutex_;
        std::condition_variable sleep_cv_;
        bool stopping_ = false;  // guarded by sleep_mutex_
        std::mutex join_mutex_;
        std::vector<std::thread> workers_;
    };

    /**
     * @brief Showcase: FIFO pool, priorities, work stealing, exceptions.
     * @param out Destination stream.
     */
    void demonstrate_thread_pools(std::ostream& out = std::cout);

}  // namespace CppVerseHub::Concurrency

#endif  // CPPVERSEHUB_CONCURRENCY_THREADPOOL_HPP
