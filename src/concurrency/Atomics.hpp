/**
 * @file Atomics.hpp
 * @brief Comprehensive atomic operations and lock-free programming demonstrations
 * @details File location: src/concurrency/Atomics.hpp
 * 
 * This file demonstrates atomic operations, memory ordering, lock-free data structures,
 * and advanced concurrent programming techniques without traditional locks.
 */

#ifndef ATOMICS_HPP
#define ATOMICS_HPP

#include <atomic>
#include <thread>
#include <vector>
#include <queue>
#include <stack>
#include <memory>
#include <chrono>
#include <iostream>
#include <string>
#include <random>
#include <functional>
#include <array>
#include <algorithm>
#include <cassert>

namespace CppVerseHub::Concurrency {

    /**
     * @class BasicAtomicsDemo
     * @brief Demonstrates fundamental atomic operations and memory ordering
     */
    class BasicAtomicsDemo {
    public:
        void demonstrate_basic_atomic_types();
        void demonstrate_atomic_operations();
        void demonstrate_memory_ordering();
        void demonstrate_compare_and_swap();
        void demonstrate_atomic_flag();
        void demonstrate_atomic_vs_mutex_performance();

    private:
        std::atomic<int> atomic_counter_{0};
        std::atomic<bool> ready_{false};
        std::atomic<std::string*> atomic_ptr_{nullptr};
        std::atomic_flag spin_lock_ = ATOMIC_FLAG_INIT;
        
        void atomic_increment_worker(int iterations);
        void memory_ordering_producer();
        void memory_ordering_consumer();
        void cas_worker(int worker_id, int attempts);
    };

    /**
     * @class MemoryOrderingDemo
     * @brief Detailed demonstrations of different memory ordering semantics
     */
    class MemoryOrderingDemo {
    public:
        void demonstrate_relaxed_ordering();
        void demonstrate_acquire_release();
        void demonstrate_sequential_consistency();
        void demonstrate_consume_ordering();
        void demonstrate_memory_fences();

    private:
        std::atomic<int> data_{0};
        std::atomic<bool> flag_{false};
        std::atomic<int> x_{0}, y_{0};
        std::atomic<int> r1_{0}, r2_{0};
        
        void relaxed_producer();
        void relaxed_consumer();
        void acquire_release_producer();
        void acquire_release_consumer();
    };

    /**
     * @class LockFreeStack
     * @brief Lock-free stack implementation using atomic operations
     */
    template<typename T>
    class LockFreeStack {
    private:
        struct Node {
            std::atomic<T*> data;
            std::atomic<Node*> next;
            
            Node() : data(nullptr), next(nullptr) {}
        };

        std::atomic<Node*> head_;
        std::atomic<size_t> size_{0};

    public:
        LockFreeStack() : head_(nullptr) {}
        
        ~LockFreeStack() {
            while (Node* old_head = head_.load()) {
                head_.store(old_head->next.load());
                delete old_head->data.load();
                delete old_head;
            }
        }

        void push(T item) {
            Node* new_node = new Node;
            T* data = new T(std::move(item));
            new_node->data.store(data);
            
            Node* old_head = head_.load();
            do {
                new_node->next.store(old_head);
            } while (!head_.compare_exchange_weak(old_head, new_node));
            
            size_++;
        }

        std::shared_ptr<T> pop() {
            Node* old_head = head_.load();
            while (old_head && !head_.compare_exchange_weak(old_head, old_head->next.load())) {
                // Retry if CAS failed
            }
            
            if (old_head == nullptr) {
                return std::shared_ptr<T>();
            }
            
            std::shared_ptr<T> result = std::make_shared<T>(*old_head->data.load());
            delete old_head->data.load();
            delete old_head;
            size_--;
            
            return result;
        }

        bool empty() const {
            return head_.load() == nullptr;
        }

        size_t size() const {
            return size_.load();
        }
    };

    /**
     * @class LockFreeQueue
     * @brief Lock-free queue implementation using atomic operations
     */
    template<typename T>
    class LockFreeQueue {
    private:
        struct Node {
            std::atomic<T*> data;
            std::atomic<Node*> next;
            
            Node() : data(nullptr), next(nullptr) {}
        };

        std::atomic<Node*> head_;
        std::atomic<Node*> tail_;
        std::atomic<size_t> size_{0};

    public:
        LockFreeQueue() {
            Node* dummy = new Node;
            head_.store(dummy);
            tail_.store(dummy);
        }
        
        ~LockFreeQueue() {
            while (Node* old_head = head_.load()) {
                head_.store(old_head->next.load());
                delete old_head->data.load();
                delete old_head;
            }
        }

        void enqueue(T item) {
            Node* new_node = new Node;
            T* data = new T(std::move(item));
            new_node->data.store(data);
            
            while (true) {
                Node* last = tail_.load();
                Node* next = last->next.load();
                
                if (last == tail_.load()) {
                    if (next == nullptr) {
                        if (last->next.compare_exchange_weak(next, new_node)) {
                            tail_.compare_exchange_weak(last, new_node);
                            break;
                        }
                    } else {
                        tail_.compare_exchange_weak(last, next);
                    }
                }
            }
            size_++;
        }

        std::shared_ptr<T> dequeue() {
            while (true) {
                Node* first = head_.load();
                Node* last = tail_.load();
                Node* next = first->next.load();
                
                if (first == head_.load()) {
                    if (first == last) {
                        if (next == nullptr) {
                            return std::shared_ptr<T>();
                        }
                        tail_.compare_exchange_weak(last, next);
                    } else {
                        if (next == nullptr) {
                            continue;
                        }
                        
                        T* data = next->data.load();
                        if (head_.compare_exchange_weak(first, next)) {
                            std::shared_ptr<T> result = std::make_shared<T>(*data);
                            delete data;
                            delete first;
                            size_--;
                            return result;
                        }
                    }
                }
            }
        }

        bool empty() const {
            Node* first = head_.load();
            Node* last = tail_.load();
            return (first == last) && (first->next.load() == nullptr);
        }

        size_t size() const {
            return size_.load();
        }
    };

    /**
     * @class AtomicCounter
     * @brief High-performance atomic counter with statistics
     */
    class AtomicCounter {
    public:
        AtomicCounter() : value_(0), increments_(0), decrements_(0) {}

        int64_t increment() {
            increments_++;
            return ++value_;
        }

        int64_t decrement() {
            decrements_++;
            return --value_;
        }

        int64_t add(int64_t n) {
            increments_++;
            return value_.fetch_add(n) + n;
        }

        int64_t subtract(int64_t n) {
            decrements_++;
            return value_.fetch_sub(n) - n;
        }

        bool compare_and_set(int64_t expected, int64_t desired) {
            return value_.compare_exchange_strong(expected, desired);
        }

        int64_t get() const {
            return value_.load();
        }

        int64_t get_increments() const {
            return increments_.load();
        }

        int64_t get_decrements() const {
            return decrements_.load();
        }

        void reset() {
            value_.store(0);
            increments_.store(0);
            decrements_.store(0);
        }

    private:
        std::atomic<int64_t> value_;
        std::atomic<int64_t> increments_;
        std::atomic<int64_t> decrements_;
    };

    /**
     * @class SpinLock
     * @brief Simple spinlock implementation using atomic_flag
     */
    class SpinLock {
    public:
        void lock() {
            while (flag_.test_and_set(std::memory_order_acquire)) {
                // Spin until lock is acquired
                std::this_thread::yield();
            }
        }

        void unlock() {
            flag_.clear(std::memory_order_release);
        }

        bool try_lock() {
            return !flag_.test_and_set(std::memory_order_acquire);
        }

    private:
        std::atomic_flag flag_ = ATOMIC_FLAG_INIT;
    };

    /**
     * @class RWSpinLock
     * @brief Reader-writer spinlock using atomic operations
     */
    class RWSpinLock {
    public:
        void read_lock() {
            while (true) {
                while (writer_count_.load(std::memory_order_acquire) != 0) {
                    std::this_thread::yield();
                }
                
                reader_count_.fetch_add(1, std::memory_order_acquire);
                
                if (writer_count_.load(std::memory_order_acquire) == 0) {
                    break;
                }
                
                reader_count_.fetch_sub(1, std::memory_order_release);
            }
        }

        void read_unlock() {
            reader_count_.fetch_sub(1, std::memory_order_release);
        }

        void write_lock() {
            while (writer_count_.exchange(1, std::memory_order_acquire) != 0) {
                std::this_thread::yield();
            }
            
            while (reader_count_.load(std::memory_order_acquire) != 0) {
                std::this_thread::yield();
            }
        }

        void write_unlock() {
            writer_count_.store(0, std::memory_order_release);
        }

    private:
        std::atomic<int> reader_count_{0};
        std::atomic<int> writer_count_{0};
    };

    /**
     * @class AtomicHashMap
     * @brief Simple lock-free hash map using atomic operations
     */
    template<typename Key, typename Value, size_t BucketCount = 1024>
    class AtomicHashMap {
    private:
        struct Node {
            Key key;
            std::atomic<Value> value;
            std::atomic<Node*> next;
            
            Node(const Key& k, const Value& v) : key(k), value(v), next(nullptr) {}
        };

        std::array<std::atomic<Node*>, BucketCount> buckets_;
        std::hash<Key> hasher_;

        size_t get_bucket_index(const Key& key) const {
            return hasher_(key) % BucketCount;
        }

    public:
        AtomicHashMap() {
            for (auto& bucket : buckets_) {
                bucket.store(nullptr);
            }
        }

        ~AtomicHashMap() {
            for (auto& bucket : buckets_) {
                Node* current = bucket.load();
                while (current) {
                    Node* next = current->next.load();
                    delete current;
                    current = next;
                }
            }
        }

        bool insert(const Key& key, const Value& value) {
            size_t bucket_idx = get_bucket_index(key);
            Node* new_node = new Node(key, value);
            
            Node* head = buckets_[bucket_idx].load();
            do {
                // Check if key already exists
                Node* current = head;
                while (current) {
                    if (current->key == key) {
                        delete new_node;
                        return false; // Key already exists
                    }
                    current = current->next.load();
                }
                
                new_node->next.store(head);
            } while (!buckets_[bucket_idx].compare_exchange_weak(head, new_node));
            
            return true;
        }

        bool find(const Key& key, Value& value) const {
            size_t bucket_idx = get_bucket_index(key);
            Node* current = buckets_[bucket_idx].load();
            
            while (current) {
                if (current->key == key) {
                    value = current->value.load();
                    return true;
                }
                current = current->next.load();
            }
            
            return false;
        }

        bool update(const Key& key, const Value& value) {
            size_t bucket_idx = get_bucket_index(key);
            Node* current = buckets_[bucket_idx].load();
            
            while (current) {
                if (current->key == key) {
                    current->value.store(value);
                    return true;
                }
                current = current->next.load();
            }
            
            return false;
        }

        bool remove(const Key& key) {
            size_t bucket_idx = get_bucket_index(key);
            
            while (true) {
                Node* head = buckets_[bucket_idx].load();
                Node* current = head;
                Node* prev = nullptr;
                
                while (current) {
                    if (current->key == key) {
                        Node* next = current->next.load();
                        
                        if (prev == nullptr) {
                            if (buckets_[bucket_idx].compare_exchange_weak(head, next)) {
                                delete current;
                                return true;
                            }
                            break; // Retry
                        } else {
                            if (prev->next.compare_exchange_weak(current, next)) {
                                delete current;
                                return true;
                            }
                            break; // Retry
                        }
                    }
                    prev = current;
                    current = current->next.load();
                }
                
                if (current == nullptr) {
                    return false; // Key not found
                }
            }
        }
    };

    /**
     * @class PerformanceAnalyzer
     * @brief Analyzes performance of atomic vs mutex operations
     */
    class PerformanceAnalyzer {
    public:
        struct PerformanceResult {
            std::string operation_type;
            std::chrono::microseconds duration;
            size_t operations_count;
            double ops_per_second;
            size_t thread_count;
        };

        static std::vector<PerformanceResult> compare_atomic_vs_mutex(size_t operations = 100000);
        static std::vector<PerformanceResult> test_lock_free_structures(size_t operations = 50000);
        static void print_results(const std::vector<PerformanceResult>& results);

    private:
        static PerformanceResult test_atomic_increment(size_t operations, size_t threads);
        static PerformanceResult test_mutex_increment(size_t operations, size_t threads);
        static PerformanceResult test_lock_free_stack(size_t operations, size_t threads);
        static PerformanceResult test_lock_free_queue(size_t operations, size_t threads);
        static PerformanceResult test_atomic_hash_map(size_t operations, size_t threads);
    };

    /**
     * @class AtomicStatistics
     * @brief Thread-safe statistics collector using atomic operations
     */
    class AtomicStatistics {
    public:
        void record_value(double value) {
            count_.fetch_add(1, std::memory_order_relaxed);
            
            // Update sum atomically
            double current_sum = sum_.load(std::memory_order_relaxed);
            while (!sum_.compare_exchange_weak(current_sum, current_sum + value, 
                                             std::memory_order_relaxed)) {
                // Retry if CAS failed
            }
            
            // Update min atomically
            double current_min = min_.load(std::memory_order_relaxed);
            while (value < current_min && 
                   !min_.compare_exchange_weak(current_min, value, std::memory_order_relaxed)) {
                // Retry if CAS failed
            }
            
            // Update max atomically
            double current_max = max_.load(std::memory_order_relaxed);
            while (value > current_max && 
                   !max_.compare_exchange_weak(current_max, value, std::memory_order_relaxed)) {
                // Retry if CAS failed
            }
        }

        double get_mean() const {
            int64_t count = count_.load(std::memory_order_acquire);
            if (count == 0) return 0.0;
            return sum_.load(std::memory_order_acquire) / count;
        }

        double get_min() const {
            return min_.load(std::memory_order_acquire);
        }

        double get_max() const {
            return max_.load(std::memory_order_acquire);
        }

        int64_t get_count() const {
            return count_.load(std::memory_order_acquire);
        }

        double get_sum() const {
            return sum_.load(std::memory_order_acquire);
        }

        void reset() {
            count_.store(0, std::memory_order_release);
            sum_.store(0.0, std::memory_order_release);
            min_.store(std::numeric_limits<double>::max(), std::memory_order_release);
            max_.store(std::numeric_limits<double>::lowest(), std::memory_order_release);
        }

    private:
        std::atomic<int64_t> count_{0};
        std::atomic<double> sum_{0.0};
        std::atomic<double> min_{std::numeric_limits<double>::max()};
        std::atomic<double> max_{std::numeric_limits<double>::lowest()};
    };

    /**
     * @class HazardPointerManager
     * @brief Simple hazard pointer implementation for memory management
     */
    class HazardPointerManager {
    public:
        static HazardPointerManager& instance();
        
        template<typename T>
        void protect(std::atomic<T*>& atomic_ptr, T*& local_ptr) {
            thread_local static std::atomic<void*> hazard_ptr{nullptr};
            
            T* ptr = atomic_ptr.load();
            do {
                local_ptr = ptr;
                hazard_ptr.store(ptr);
                ptr = atomic_ptr.load();
            } while (ptr != local_ptr);
        }

        template<typename T>
        void retire(T* ptr) {
            // Simplified retirement - in real implementation would check hazard pointers
            delete ptr;
        }

    private:
        HazardPointerManager() = default;
        static std::vector<std::atomic<void*>*> hazard_pointers_;
    };

    /**
     * @class ConcurrentBloomFilter
     * @brief Thread-safe Bloom filter using atomic operations
     */
    class ConcurrentBloomFilter {
    public:
        explicit ConcurrentBloomFilter(size_t size = 1024, size_t hash_count = 3);
        
        void insert(const std::string& item);
        bool might_contain(const std::string& item) const;
        void clear();
        
        size_t size() const { return bit_array_.size(); }
        size_t hash_functions() const { return hash_count_; }
        
        double estimated_fill_ratio() const;

    private:
        std::vector<std::atomic<bool>> bit_array_;
        size_t hash_count_;
        mutable std::atomic<size_t> insert_count_{0};
        
        std::vector<size_t> get_hash_values(const std::string& item) const;
    };

    /**
     * @class AtomicsDemo
     * @brief Main demonstration coordinator for atomic operations examples
     */
    class AtomicsDemo {
    public:
        static void demonstrate_basic_atomics();
        static void demonstrate_memory_ordering();
        static void demonstrate_lock_free_stack();
        static void demonstrate_lock_free_queue();
        static void demonstrate_atomic_counter();
        static void demonstrate_spinlocks();
        static void demonstrate_atomic_hash_map();
        static void demonstrate_performance_comparison();
        static void demonstrate_atomic_statistics();
        static void demonstrate_bloom_filter();
        static void run_all_demonstrations();

    private:
        static void print_section_header(const std::string& title);
        static void print_section_footer();
        static void simulate_work(std::chrono::milliseconds duration);
    };

} // namespace CppVerseHub::Concurrency

#endif // ATOMICS_HPP