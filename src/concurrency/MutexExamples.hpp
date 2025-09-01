/**
 * @file MutexExamples.hpp
 * @brief Comprehensive mutex and synchronization demonstrations
 * @details File location: src/concurrency/MutexExamples.hpp
 * 
 * This file demonstrates various mutex types, locking strategies,
 * deadlock prevention, and synchronization patterns.
 */

#ifndef MUTEXEXAMPLES_HPP
#define MUTEXEXAMPLES_HPP

#include <mutex>
#include <shared_mutex>
#include <timed_mutex>
#include <recursive_mutex>
#include <thread>
#include <vector>
#include <queue>
#include <unordered_map>
#include <atomic>
#include <chrono>
#include <iostream>
#include <string>
#include <memory>
#include <functional>
#include <random>
#include <condition_variable>
#include <future>
#include <array>

namespace CppVerseHub::Concurrency {

    /**
     * @class BasicMutexDemo
     * @brief Demonstrates basic mutex usage and race condition prevention
     */
    class BasicMutexDemo {
    public:
        BasicMutexDemo() : counter_(0) {}

        void demonstrate_race_condition();
        void demonstrate_mutex_protection();
        void demonstrate_lock_guard();
        void demonstrate_unique_lock();
        void demonstrate_scoped_lock();

    private:
        int counter_;
        std::mutex counter_mutex_;

        void unsafe_increment(int iterations);
        void safe_increment(int iterations);
    };

    /**
     * @class AdvancedMutexDemo
     * @brief Demonstrates different mutex types and advanced locking
     */
    class AdvancedMutexDemo {
    public:
        void demonstrate_recursive_mutex();
        void demonstrate_timed_mutex();
        void demonstrate_shared_mutex();
        void demonstrate_try_lock_strategies();

    private:
        std::recursive_mutex recursive_mtx_;
        std::timed_mutex timed_mtx_;
        std::shared_mutex shared_mtx_;
        std::string shared_data_ = "Initial shared data";
        int recursive_depth_ = 0;

        void recursive_function(int depth);
        void reader_function(int reader_id);
        void writer_function(int writer_id);
    };

    /**
     * @class DeadlockPrevention
     * @brief Demonstrates deadlock scenarios and prevention techniques
     */
    class DeadlockPrevention {
    public:
        void demonstrate_deadlock_scenario();
        void demonstrate_ordered_locking();
        void demonstrate_timeout_prevention();
        void demonstrate_scoped_lock_prevention();
        void demonstrate_lock_hierarchy();

    private:
        std::mutex mutex_a_;
        std::mutex mutex_b_;
        std::timed_mutex timed_a_;
        std::timed_mutex timed_b_;

        // Lock hierarchy for deadlock prevention
        class HierarchicalMutex {
        public:
            explicit HierarchicalMutex(unsigned level) : hierarchy_level_(level) {}

            void lock() {
                check_for_hierarchy_violation();
                internal_mutex_.lock();
                update_hierarchy_value();
            }

            void unlock() {
                if (this_thread_hierarchy_value_ != hierarchy_level_) {
                    throw std::logic_error("Mutex hierarchy violation on unlock");
                }
                this_thread_hierarchy_value_ = previous_hierarchy_value_;
                internal_mutex_.unlock();
            }

            bool try_lock() {
                check_for_hierarchy_violation();
                if (!internal_mutex_.try_lock()) {
                    return false;
                }
                update_hierarchy_value();
                return true;
            }

        private:
            std::mutex internal_mutex_;
            unsigned const hierarchy_level_;
            unsigned previous_hierarchy_value_;
            static thread_local unsigned this_thread_hierarchy_value_;

            void check_for_hierarchy_violation() {
                if (this_thread_hierarchy_value_ <= hierarchy_level_) {
                    throw std::logic_error("Mutex hierarchy violated");
                }
            }

            void update_hierarchy_value() {
                previous_hierarchy_value_ = this_thread_hierarchy_value_;
                this_thread_hierarchy_value_ = hierarchy_level_;
            }
        };

        static HierarchicalMutex high_level_mutex_;
        static HierarchicalMutex low_level_mutex_;
    };

    /**
     * @class ThreadSafeQueue
     * @brief Thread-safe container implementation using mutexes
     */
    template<typename T>
    class ThreadSafeQueue {
    public:
        void push(T item) {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push(std::move(item));
            condition_.notify_one();
        }

        bool try_pop(T& item) {
            std::lock_guard<std::mutex> lock(mutex_);
            if (queue_.empty()) {
                return false;
            }
            item = std::move(queue_.front());
            queue_.pop();
            return true;
        }

        std::shared_ptr<T> try_pop() {
            std::lock_guard<std::mutex> lock(mutex_);
            if (queue_.empty()) {
                return std::shared_ptr<T>();
            }
            std::shared_ptr<T> result = std::make_shared<T>(std::move(queue_.front()));
            queue_.pop();
            return result;
        }

        void wait_and_pop(T& item) {
            std::unique_lock<std::mutex> lock(mutex_);
            while (queue_.empty()) {
                condition_.wait(lock);
            }
            item = std::move(queue_.front());
            queue_.pop();
        }

        std::shared_ptr<T> wait_and_pop() {
            std::unique_lock<std::mutex> lock(mutex_);
            while (queue_.empty()) {
                condition_.wait(lock);
            }
            std::shared_ptr<T> result = std::make_shared<T>(std::move(queue_.front()));
            queue_.pop();
            return result;
        }

        bool empty() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return queue_.empty();
        }

        size_t size() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return queue_.size();
        }

    private:
        mutable std::mutex mutex_;
        std::queue<T> queue_;
        std::condition_variable condition_;
    };

    /**
     * @class ThreadSafeMap
     * @brief Thread-safe map implementation with read-write locks
     */
    template<typename Key, typename Value>
    class ThreadSafeMap {
    public:
        void insert(const Key& key, const Value& value) {
            std::unique_lock<std::shared_mutex> lock(mutex_);
            map_[key] = value;
        }

        bool find(const Key& key, Value& value) const {
            std::shared_lock<std::shared_mutex> lock(mutex_);
            auto it = map_.find(key);
            if (it != map_.end()) {
                value = it->second;
                return true;
            }
            return false;
        }

        std::optional<Value> find(const Key& key) const {
            std::shared_lock<std::shared_mutex> lock(mutex_);
            auto it = map_.find(key);
            if (it != map_.end()) {
                return it->second;
            }
            return std::nullopt;
        }

        bool erase(const Key& key) {
            std::unique_lock<std::shared_mutex> lock(mutex_);
            return map_.erase(key) > 0;
        }

        size_t size() const {
            std::shared_lock<std::shared_mutex> lock(mutex_);
            return map_.size();
        }

        std::vector<Key> keys() const {
            std::shared_lock<std::shared_mutex> lock(mutex_);
            std::vector<Key> result;
            result.reserve(map_.size());
            for (const auto& pair : map_) {
                result.push_back(pair.first);
            }
            return result;
        }

    private:
        mutable std::shared_mutex mutex_;
        std::unordered_map<Key, Value> map_;
    };

    /**
     * @class ResourceManager
     * @brief Demonstrates resource management with multiple mutexes
     */
    class ResourceManager {
    public:
        struct Resource {
            int id;
            std::string name;
            bool in_use;
            std::chrono::steady_clock::time_point last_accessed;

            Resource(int id, const std::string& name) 
                : id(id), name(name), in_use(false), last_accessed(std::chrono::steady_clock::now()) {}
        };

        ResourceManager();
        ~ResourceManager();

        std::shared_ptr<Resource> acquire_resource(const std::string& requester);
        void release_resource(std::shared_ptr<Resource> resource, const std::string& releaser);
        void print_resource_status() const;
        
        size_t available_resource_count() const;
        std::vector<std::string> get_resource_names() const;

    private:
        std::vector<std::shared_ptr<Resource>> resources_;
        mutable std::shared_mutex resources_mutex_;
        std::unordered_map<int, std::string> resource_owners_;
        mutable std::mutex owners_mutex_;

        void initialize_resources();
    };

    /**
     * @class MutexPerformanceTest
     * @brief Performance comparison of different mutex types
     */
    class MutexPerformanceTest {
    public:
        struct PerformanceResult {
            std::string mutex_type;
            std::chrono::microseconds total_time;
            size_t operations_count;
            double ops_per_second;
        };

        static std::vector<PerformanceResult> run_performance_comparison(size_t operations = 100000);
        static void print_performance_results(const std::vector<PerformanceResult>& results);

    private:
        static PerformanceResult test_basic_mutex(size_t operations);
        static PerformanceResult test_recursive_mutex(size_t operations);
        static PerformanceResult test_shared_mutex_read(size_t operations);
        static PerformanceResult test_shared_mutex_write(size_t operations);
        static PerformanceResult test_atomic_operations(size_t operations);
    };

    /**
     * @class SpaceStationSimulation
     * @brief Complex simulation using multiple synchronization primitives
     */
    class SpaceStationSimulation {
    public:
        enum class SystemType {
            LIFE_SUPPORT,
            POWER_MANAGEMENT,
            COMMUNICATIONS,
            NAVIGATION,
            SCIENCE_LAB
        };

        struct SystemStatus {
            SystemType type;
            bool operational;
            double efficiency;
            std::chrono::steady_clock::time_point last_maintenance;
            std::string current_operator;
        };

        SpaceStationSimulation();
        void start_simulation(std::chrono::seconds duration);
        void stop_simulation();
        void print_station_status() const;

    private:
        std::unordered_map<SystemType, SystemStatus> systems_;
        mutable std::shared_mutex systems_mutex_;
        
        std::mutex console_mutex_;  // For thread-safe console output
        std::atomic<bool> simulation_running_{false};
        std::vector<std::thread> operator_threads_;
        
        void operator_routine(const std::string& operator_name, SystemType preferred_system);
        void maintenance_routine();
        void emergency_response_routine();
        
        std::string system_type_to_string(SystemType type) const;
        void safe_print(const std::string& message) const;
    };

    /**
     * @class ProducerConsumerDemo
     * @brief Demonstrates producer-consumer pattern with condition variables
     */
    class ProducerConsumerDemo {
    public:
        struct Task {
            int id;
            std::string description;
            std::chrono::steady_clock::time_point created_at;
        };

        ProducerConsumerDemo(size_t buffer_size = 10);
        void start_demo(size_t num_producers = 2, size_t num_consumers = 3, 
                       std::chrono::seconds duration = std::chrono::seconds(10));
        void stop_demo();
        void print_statistics() const;

    private:
        ThreadSafeQueue<Task> task_queue_;
        std::atomic<bool> demo_running_{false};
        std::atomic<int> task_counter_{0};
        std::atomic<int> tasks_produced_{0};
        std::atomic<int> tasks_consumed_{0};
        
        std::vector<std::thread> producer_threads_;
        std::vector<std::thread> consumer_threads_;
        mutable std::mutex stats_mutex_;
        
        void producer_routine(int producer_id);
        void consumer_routine(int consumer_id);
    };

    /**
     * @class ReadersWritersDemo
     * @brief Demonstrates readers-writers problem solution
     */
    class ReadersWritersDemo {
    public:
        ReadersWritersDemo();
        void start_demo(size_t num_readers = 5, size_t num_writers = 2,
                       std::chrono::seconds duration = std::chrono::seconds(15));
        void stop_demo();
        void print_statistics() const;

    private:
        std::string shared_document_;
        mutable std::shared_mutex document_mutex_;
        
        std::atomic<bool> demo_running_{false};
        std::atomic<int> active_readers_{0};
        std::atomic<int> total_reads_{0};
        std::atomic<int> total_writes_{0};
        
        std::vector<std::thread> reader_threads_;
        std::vector<std::thread> writer_threads_;
        mutable std::mutex console_mutex_;
        
        void reader_routine(int reader_id);
        void writer_routine(int writer_id);
        void safe_console_print(const std::string& message) const;
    };

    /**
     * @class DiningPhilosophersDemo
     * @brief Demonstrates classic dining philosophers problem
     */
    class DiningPhilosophersDemo {
    public:
        explicit DiningPhilosophersDemo(size_t num_philosophers = 5);
        void start_demo(std::chrono::seconds duration = std::chrono::seconds(20));
        void stop_demo();
        void print_statistics() const;

    private:
        struct Philosopher {
            int id;
            std::atomic<int> meals_eaten{0};
            std::atomic<int> thinking_time_ms{0};
            std::atomic<int> eating_time_ms{0};
            std::atomic<int> waiting_time_ms{0};
        };

        size_t num_philosophers_;
        std::vector<std::mutex> forks_;
        std::vector<Philosopher> philosophers_;
        std::atomic<bool> demo_running_{false};
        std::vector<std::thread> philosopher_threads_;
        mutable std::mutex console_mutex_;
        
        void philosopher_routine(int philosopher_id);
        void safe_console_print(const std::string& message) const;
        std::chrono::milliseconds get_random_time(int min_ms, int max_ms) const;
    };

    /**
     * @class MutexDemo
     * @brief Main demonstration coordinator for all mutex examples
     */
    class MutexDemo {
    public:
        static void demonstrate_basic_mutex();
        static void demonstrate_advanced_mutex();
        static void demonstrate_deadlock_prevention();
        static void demonstrate_thread_safe_containers();
        static void demonstrate_resource_management();
        static void demonstrate_performance_comparison();
        static void demonstrate_space_station_simulation();
        static void demonstrate_producer_consumer();
        static void demonstrate_readers_writers();
        static void demonstrate_dining_philosophers();
        static void run_all_demonstrations();

    private:
        static void simulate_work(std::chrono::milliseconds duration);
        static void print_section_header(const std::string& title);
        static void print_section_footer();
    };

} // namespace CppVerseHub::Concurrency

#endif // MUTEXEXAMPLES_HPP