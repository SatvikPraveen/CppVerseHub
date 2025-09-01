/**
 * @file ConditionVariables.hpp
 * @brief Comprehensive condition variable demonstrations for thread coordination
 * @details File location: src/concurrency/ConditionVariables.hpp
 * 
 * This file demonstrates various condition variable patterns, thread coordination
 * strategies, and synchronization mechanisms including barriers, semaphores,
 * and complex multi-threaded workflows.
 */

#ifndef CONDITIONVARIABLES_HPP
#define CONDITIONVARIABLES_HPP

#include <condition_variable>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <vector>
#include <queue>
#include <deque>
#include <stack>
#include <unordered_map>
#include <atomic>
#include <chrono>
#include <iostream>
#include <string>
#include <memory>
#include <functional>
#include <random>
#include <future>
#include <optional>
#include <array>
#include <algorithm>

namespace CppVerseHub::Concurrency {

    /**
     * @class BasicConditionVariableDemo
     * @brief Demonstrates fundamental condition variable usage patterns
     */
    class BasicConditionVariableDemo {
    public:
        void demonstrate_basic_wait_notify();
        void demonstrate_predicate_wait();
        void demonstrate_spurious_wakeup_handling();
        void demonstrate_timeout_operations();
        void demonstrate_notify_all_vs_notify_one();

    private:
        std::mutex mutex_;
        std::condition_variable cv_;
        bool ready_ = false;
        std::string shared_data_;
        int shared_counter_ = 0;
        
        void producer_task(const std::string& data);
        void consumer_task();
        void worker_task(int worker_id);
    };

    /**
     * @class ProducerConsumerBuffer
     * @brief Thread-safe bounded buffer using condition variables
     */
    template<typename T>
    class ProducerConsumerBuffer {
    public:
        explicit ProducerConsumerBuffer(size_t capacity) 
            : capacity_(capacity), buffer_(capacity) {}

        void produce(T item) {
            std::unique_lock<std::mutex> lock(mutex_);
            
            // Wait until buffer is not full
            not_full_.wait(lock, [this] { return count_ < capacity_; });
            
            buffer_[write_index_] = std::move(item);
            write_index_ = (write_index_ + 1) % capacity_;
            count_++;
            
            not_empty_.notify_one();
        }

        T consume() {
            std::unique_lock<std::mutex> lock(mutex_);
            
            // Wait until buffer is not empty
            not_empty_.wait(lock, [this] { return count_ > 0; });
            
            T item = std::move(buffer_[read_index_]);
            read_index_ = (read_index_ + 1) % capacity_;
            count_--;
            
            not_full_.notify_one();
            return item;
        }

        bool try_produce(T item, std::chrono::milliseconds timeout = std::chrono::milliseconds(100)) {
            std::unique_lock<std::mutex> lock(mutex_);
            
            if (!not_full_.wait_for(lock, timeout, [this] { return count_ < capacity_; })) {
                return false;
            }
            
            buffer_[write_index_] = std::move(item);
            write_index_ = (write_index_ + 1) % capacity_;
            count_++;
            
            not_empty_.notify_one();
            return true;
        }

        std::optional<T> try_consume(std::chrono::milliseconds timeout = std::chrono::milliseconds(100)) {
            std::unique_lock<std::mutex> lock(mutex_);
            
            if (!not_empty_.wait_for(lock, timeout, [this] { return count_ > 0; })) {
                return std::nullopt;
            }
            
            T item = std::move(buffer_[read_index_]);
            read_index_ = (read_index_ + 1) % capacity_;
            count_--;
            
            not_full_.notify_one();
            return item;
        }

        size_t size() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return count_;
        }

        size_t capacity() const { return capacity_; }

        bool empty() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return count_ == 0;
        }

        bool full() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return count_ == capacity_;
        }

    private:
        const size_t capacity_;
        std::vector<T> buffer_;
        size_t read_index_ = 0;
        size_t write_index_ = 0;
        size_t count_ = 0;
        
        mutable std::mutex mutex_;
        std::condition_variable not_empty_;
        std::condition_variable not_full_;
    };

    /**
     * @class ThreadBarrier
     * @brief Custom barrier implementation using condition variables
     */
    class ThreadBarrier {
    public:
        explicit ThreadBarrier(size_t thread_count) 
            : thread_count_(thread_count), waiting_count_(0), barrier_generation_(0) {}

        void wait() {
            std::unique_lock<std::mutex> lock(mutex_);
            
            size_t current_generation = barrier_generation_;
            waiting_count_++;
            
            if (waiting_count_ == thread_count_) {
                // Last thread to arrive - release all
                waiting_count_ = 0;
                barrier_generation_++;
                condition_.notify_all();
            } else {
                // Wait for all threads to arrive
                condition_.wait(lock, [this, current_generation] {
                    return current_generation != barrier_generation_;
                });
            }
        }

        bool wait_for(const std::chrono::milliseconds& timeout) {
            std::unique_lock<std::mutex> lock(mutex_);
            
            size_t current_generation = barrier_generation_;
            waiting_count_++;
            
            if (waiting_count_ == thread_count_) {
                waiting_count_ = 0;
                barrier_generation_++;
                condition_.notify_all();
                return true;
            } else {
                bool result = condition_.wait_for(lock, timeout, [this, current_generation] {
                    return current_generation != barrier_generation_;
                });
                
                if (!result) {
                    waiting_count_--; // Remove from wait count if timeout
                }
                
                return result;
            }
        }

        size_t thread_count() const { return thread_count_; }

    private:
        const size_t thread_count_;
        size_t waiting_count_;
        size_t barrier_generation_;
        std::mutex mutex_;
        std::condition_variable condition_;
    };

    /**
     * @class CountingSemaphore
     * @brief Counting semaphore implementation using condition variables
     */
    class CountingSemaphore {
    public:
        explicit CountingSemaphore(int initial_count = 0) : count_(initial_count) {}

        void acquire() {
            std::unique_lock<std::mutex> lock(mutex_);
            condition_.wait(lock, [this] { return count_ > 0; });
            count_--;
        }

        bool try_acquire() {
            std::lock_guard<std::mutex> lock(mutex_);
            if (count_ > 0) {
                count_--;
                return true;
            }
            return false;
        }

        bool try_acquire_for(const std::chrono::milliseconds& timeout) {
            std::unique_lock<std::mutex> lock(mutex_);
            if (condition_.wait_for(lock, timeout, [this] { return count_ > 0; })) {
                count_--;
                return true;
            }
            return false;
        }

        void release(int count = 1) {
            std::lock_guard<std::mutex> lock(mutex_);
            count_ += count;
            for (int i = 0; i < count; ++i) {
                condition_.notify_one();
            }
        }

        int available_count() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return count_;
        }

    private:
        int count_;
        mutable std::mutex mutex_;
        std::condition_variable condition_;
    };

    /**
     * @class ThreadPool
     * @brief Simple thread pool implementation using condition variables
     */
    class ThreadPool {
    public:
        explicit ThreadPool(size_t num_threads);
        ~ThreadPool();

        template<typename F>
        auto submit(F&& task) -> std::future<decltype(task())> {
            using ReturnType = decltype(task());
            
            auto task_ptr = std::make_shared<std::packaged_task<ReturnType()>>(
                std::forward<F>(task)
            );
            
            std::future<ReturnType> future = task_ptr->get_future();
            
            {
                std::lock_guard<std::mutex> lock(queue_mutex_);
                if (stop_) {
                    throw std::runtime_error("ThreadPool is stopped");
                }
                
                tasks_.emplace([task_ptr] { (*task_ptr)(); });
            }
            
            condition_.notify_one();
            return future;
        }

        void shutdown();
        size_t active_threads() const;
        size_t pending_tasks() const;

    private:
        std::vector<std::thread> workers_;
        std::queue<std::function<void()>> tasks_;
        
        std::mutex queue_mutex_;
        std::condition_variable condition_;
        std::atomic<bool> stop_{false};
        std::atomic<size_t> active_count_{0};
        
        void worker_thread();
    };

    /**
     * @class WorkflowCoordinator
     * @brief Coordinates complex multi-stage workflows using condition variables
     */
    class WorkflowCoordinator {
    public:
        enum class Stage {
            INITIALIZATION,
            DATA_PROCESSING,
            VALIDATION,
            OUTPUT_GENERATION,
            CLEANUP,
            COMPLETED
        };

        struct Task {
            int id;
            std::string name;
            std::string data;
            Stage current_stage;
            std::chrono::steady_clock::time_point created_at;
            std::chrono::steady_clock::time_point completed_at;
        };

        WorkflowCoordinator(size_t num_workers_per_stage = 2);
        ~WorkflowCoordinator();

        void submit_task(const std::string& name, const std::string& data);
        void start_workflow(std::chrono::seconds duration);
        void stop_workflow();
        void print_statistics() const;

    private:
        std::unordered_map<Stage, std::queue<std::shared_ptr<Task>>> stage_queues_;
        std::unordered_map<Stage, std::mutex> stage_mutexes_;
        std::unordered_map<Stage, std::condition_variable> stage_conditions_;
        
        std::vector<std::thread> worker_threads_;
        std::atomic<bool> running_{false};
        std::atomic<int> task_counter_{0};
        
        mutable std::mutex stats_mutex_;
        std::unordered_map<Stage, std::atomic<int>> tasks_processed_;
        std::vector<std::shared_ptr<Task>> completed_tasks_;
        
        void stage_worker(Stage stage, int worker_id);
        void process_task_at_stage(std::shared_ptr<Task> task, Stage stage);
        void advance_task_to_next_stage(std::shared_ptr<Task> task);
        std::string stage_to_string(Stage stage) const;
        Stage next_stage(Stage current) const;
    };

    /**
     * @class EventNotificationSystem
     * @brief Event-driven notification system using condition variables
     */
    class EventNotificationSystem {
    public:
        enum class EventType {
            DATA_UPDATED,
            USER_ACTION,
            SYSTEM_ALERT,
            TIMER_EXPIRED,
            CUSTOM_EVENT
        };

        struct Event {
            EventType type;
            std::string source;
            std::string message;
            std::chrono::steady_clock::time_point timestamp;
            std::unordered_map<std::string, std::string> metadata;
        };

        using EventHandler = std::function<void(const Event&)>;

        EventNotificationSystem();
        ~EventNotificationSystem();

        void subscribe(EventType type, const std::string& subscriber_id, EventHandler handler);
        void unsubscribe(EventType type, const std::string& subscriber_id);
        void publish_event(const Event& event);
        void start_system();
        void stop_system();
        void print_statistics() const;

    private:
        std::unordered_map<EventType, std::unordered_map<std::string, EventHandler>> subscribers_;
        std::queue<Event> event_queue_;
        
        std::mutex queue_mutex_;
        std::condition_variable event_available_;
        std::atomic<bool> running_{false};
        
        std::vector<std::thread> processor_threads_;
        mutable std::mutex stats_mutex_;
        std::atomic<int> events_published_{0};
        std::atomic<int> events_processed_{0};
        
        void event_processor(int processor_id);
        std::string event_type_to_string(EventType type) const;
    };

    /**
     * @class DatabaseConnectionPool
     * @brief Database connection pool simulation using condition variables
     */
    class DatabaseConnectionPool {
    public:
        struct Connection {
            int id;
            std::string connection_string;
            bool in_use;
            std::chrono::steady_clock::time_point last_used;
            int query_count;

            Connection(int id, const std::string& conn_str) 
                : id(id), connection_string(conn_str), in_use(false), 
                  last_used(std::chrono::steady_clock::now()), query_count(0) {}
        };

        explicit DatabaseConnectionPool(size_t pool_size, size_t max_wait_time_ms = 5000);
        ~DatabaseConnectionPool();

        std::shared_ptr<Connection> acquire_connection(const std::string& client_id);
        void release_connection(std::shared_ptr<Connection> connection);
        void simulate_query(std::shared_ptr<Connection> connection, const std::string& query);
        
        void start_connection_monitor();
        void stop_connection_monitor();
        void print_pool_status() const;
        
        size_t available_connections() const;
        size_t total_connections() const { return pool_size_; }

    private:
        std::vector<std::shared_ptr<Connection>> connections_;
        std::queue<std::shared_ptr<Connection>> available_connections_;
        
        const size_t pool_size_;
        const std::chrono::milliseconds max_wait_time_;
        
        std::mutex pool_mutex_;
        std::condition_variable connection_available_;
        std::atomic<bool> monitor_running_{false};
        
        std::thread monitor_thread_;
        mutable std::mutex stats_mutex_;
        std::atomic<int> total_acquisitions_{0};
        std::atomic<int> failed_acquisitions_{0};
        
        void connection_monitor();
        void cleanup_idle_connections();
    };

    /**
     * @class ConditionVariableDemo
     * @brief Main demonstration coordinator for condition variable examples
     */
    class ConditionVariableDemo {
    public:
        static void demonstrate_basic_condition_variables();
        static void demonstrate_producer_consumer_buffer();
        static void demonstrate_thread_barrier();
        static void demonstrate_counting_semaphore();
        static void demonstrate_thread_pool();
        static void demonstrate_workflow_coordination();
        static void demonstrate_event_notification_system();
        static void demonstrate_database_connection_pool();
        static void run_all_demonstrations();

    private:
        static void print_section_header(const std::string& title);
        static void print_section_footer();
        static void simulate_work(std::chrono::milliseconds duration);
    };

} // namespace CppVerseHub::Concurrency

#endif // CONDITIONVARIABLES_HPP