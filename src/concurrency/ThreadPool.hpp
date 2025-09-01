/**
 * @file ThreadPool.hpp
 * @brief Advanced thread pool implementation for worker thread management
 * @details File location: src/concurrency/ThreadPool.hpp
 * 
 * This file demonstrates comprehensive thread pool patterns including
 * work stealing, priority queues, and dynamic thread management.
 */

#ifndef THREADPOOL_HPP
#define THREADPOOL_HPP

#include <thread>
#include <vector>
#include <queue>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <future>
#include <memory>
#include <iostream>
#include <chrono>
#include <random>
#include <algorithm>
#include <type_traits>

namespace CppVerseHub::Concurrency {

    /**
     * @class BasicThreadPool
     * @brief Simple thread pool with work queue
     */
    class BasicThreadPool {
    public:
        explicit BasicThreadPool(size_t num_threads = std::thread::hardware_concurrency());
        ~BasicThreadPool();

        // Submit work to the pool
        template<typename F, typename... Args>
        auto submit(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>>;

        // Get pool statistics
        size_t active_threads() const { return threads_.size(); }
        size_t pending_tasks() const;
        
        void shutdown();
        bool is_shutdown() const { return shutdown_.load(); }

    private:
        std::vector<std::thread> threads_;
        std::queue<std::function<void()>> tasks_;
        mutable std::mutex queue_mutex_;
        std::condition_variable cv_;
        std::atomic<bool> shutdown_{false};

        void worker_thread();
    };

    /**
     * @class PriorityThreadPool
     * @brief Thread pool with priority-based task scheduling
     */
    class PriorityThreadPool {
    public:
        enum class Priority { LOW = 1, NORMAL = 2, HIGH = 3, CRITICAL = 4 };

        struct Task {
            std::function<void()> function;
            Priority priority;
            std::chrono::steady_clock::time_point submit_time;
            size_t id;

            Task(std::function<void()> f, Priority p, size_t task_id)
                : function(std::move(f)), priority(p), submit_time(std::chrono::steady_clock::now()), id(task_id) {}

            bool operator<(const Task& other) const {
                if (priority != other.priority) {
                    return priority < other.priority; // Lower priority value = lower priority
                }
                return submit_time > other.submit_time; // Earlier submission = higher priority
            }
        };

        explicit PriorityThreadPool(size_t num_threads = std::thread::hardware_concurrency());
        ~PriorityThreadPool();

        template<typename F, typename... Args>
        auto submit(Priority priority, F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>>;

        size_t pending_tasks() const;
        void shutdown();

    private:
        std::vector<std::thread> threads_;
        std::priority_queue<Task> task_queue_;
        mutable std::mutex queue_mutex_;
        std::condition_variable cv_;
        std::atomic<bool> shutdown_{false};
        std::atomic<size_t> next_task_id_{0};

        void worker_thread();
    };

    /**
     * @class WorkStealingThreadPool
     * @brief Advanced thread pool with work stealing for load balancing
     */
    class WorkStealingThreadPool {
    public:
        explicit WorkStealingThreadPool(size_t num_threads = std::thread::hardware_concurrency());
        ~WorkStealingThreadPool();

        template<typename F, typename... Args>
        auto submit(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>>;

        size_t total_pending_tasks() const;
        void print_queue_status() const;
        void shutdown();

    private:
        struct WorkerQueue {
            std::deque<std::function<void()>> tasks;
            mutable std::mutex mutex;
        };

        std::vector<std::thread> threads_;
        std::vector<std::unique_ptr<WorkerQueue>> queues_;
        std::atomic<bool> shutdown_{false};
        std::atomic<size_t> next_queue_{0};

        void worker_thread(size_t thread_id);
        bool try_steal_work(size_t my_id);
        void push_task_to_queue(size_t queue_id, std::function<void()> task);
    };

    /**
     * @class ThreadPoolManager
     * @brief Manager for different thread pool implementations
     */
    class ThreadPoolManager {
    public:
        enum class PoolType { BASIC, PRIORITY, WORK_STEALING };

        static ThreadPoolManager& instance();
        
        void create_pool(PoolType type, const std::string& name, size_t num_threads);
        void shutdown_pool(const std::string& name);
        void shutdown_all();

        BasicThreadPool* get_basic_pool(const std::string& name);
        PriorityThreadPool* get_priority_pool(const std::string& name);
        WorkStealingThreadPool* get_work_stealing_pool(const std::string& name);

        void print_pool_statistics() const;

    private:
        ThreadPoolManager() = default;
        ~ThreadPoolManager() { shutdown_all(); }

        struct PoolInfo {
            PoolType type;
            std::unique_ptr<BasicThreadPool> basic_pool;
            std::unique_ptr<PriorityThreadPool> priority_pool;
            std::unique_ptr<WorkStealingThreadPool> work_stealing_pool;
        };

        std::unordered_map<std::string, PoolInfo> pools_;
        mutable std::mutex pools_mutex_;
    };

    /**
     * @class ThreadPoolDemo
     * @brief Comprehensive demonstration of thread pool patterns
     */
    class ThreadPoolDemo {
    public:
        static void demonstrate_basic_pool();
        static void demonstrate_priority_pool();
        static void demonstrate_work_stealing();
        static void demonstrate_performance_comparison();
        static void demonstrate_space_mission_simulation();
        static void run_all_demonstrations();

    private:
        // Simulated space mission tasks
        struct SpaceMissionTask {
            std::string mission_name;
            std::chrono::milliseconds duration;
            PriorityThreadPool::Priority priority;
        };

        static std::vector<SpaceMissionTask> generate_mission_tasks();
        static void simulate_cpu_work(std::chrono::milliseconds duration);
    };

    // Template implementations

    template<typename F, typename... Args>
    auto BasicThreadPool::submit(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>> {
        using return_type = std::invoke_result_t<F, Args...>;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        auto future = task->get_future();

        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            if (shutdown_.load()) {
                throw std::runtime_error("Cannot submit task to shutdown thread pool");
            }
            tasks_.emplace([task]() { (*task)(); });
        }

        cv_.notify_one();
        return future;
    }

    template<typename F, typename... Args>
    auto PriorityThreadPool::submit(Priority priority, F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>> {
        using return_type = std::invoke_result_t<F, Args...>;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        auto future = task->get_future();

        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            if (shutdown_.load()) {
                throw std::runtime_error("Cannot submit task to shutdown thread pool");
            }

            size_t task_id = next_task_id_.fetch_add(1);
            task_queue_.emplace([task]() { (*task)(); }, priority, task_id);
        }

        cv_.notify_one();
        return future;
    }

    template<typename F, typename... Args>
    auto WorkStealingThreadPool::submit(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>> {
        using return_type = std::invoke_result_t<F, Args...>;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        auto future = task->get_future();

        if (shutdown_.load()) {
            throw std::runtime_error("Cannot submit task to shutdown thread pool");
        }

        // Round-robin distribution to worker queues
        size_t queue_id = next_queue_.fetch_add(1) % queues_.size();
        push_task_to_queue(queue_id, [task]() { (*task)(); });

        return future;
    }

} // namespace CppVerseHub::Concurrency

#endif // THREADPOOL_HPP