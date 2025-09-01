# Concurrency & Multithreading Cheat Sheet

**Location:** `CppVerseHub/docs/cheat_sheets/Concurrency_CheatSheet.md`

## 📚 Complete C++ Concurrency Reference

### 🧵 Thread Basics

#### **Creating and Managing Threads**

```cpp
#include <thread>
#include <iostream>

// Function to run in thread
void mission_worker(const std::string& mission_name, int duration) {
    std::cout << "Starting mission: " << mission_name << "\n";
    std::this_thread::sleep_for(std::chrono::seconds(duration));
    std::cout << "Mission completed: " << mission_name << "\n";
}

// Class member function as thread
class Fleet {
public:
    void patrol(const std::string& sector) {
        std::cout << "Patrolling sector: " << sector << "\n";
        // Patrol logic...
    }
};

int main() {
    // Create threads with different callables

    // 1. Function pointer
    std::thread t1(mission_worker, "Exploration Alpha", 3);

    // 2. Lambda function
    std::thread t2([]() {
        std::cout << "Lambda thread executing\n";
    });

    // 3. Function object
    auto mission_func = [](const std::string& name) {
        std::cout << "Function object mission: " << name << "\n";
    };
    std::thread t3(mission_func, "Colonization Beta");

    // 4. Member function
    Fleet fleet;
    std::thread t4(&Fleet::patrol, &fleet, "Outer Rim");

    // Join threads (wait for completion)
    t1.join();
    t2.join();
    t3.join();
    t4.join();

    // Alternative: detach threads (run independently)
    // t1.detach();  // Thread runs independently

    std::cout << "All missions completed\n";
    return 0;
}
```

#### **Thread Properties and Control**

```cpp
#include <thread>
#include <chrono>

void thread_info_demo() {
    // Get thread information
    std::cout << "Hardware concurrency: "
              << std::thread::hardware_concurrency() << "\n";

    std::thread worker([]() {
        auto thread_id = std::this_thread::get_id();
        std::cout << "Thread ID: " << thread_id << "\n";

        // Thread sleep
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Yield to other threads
        std::this_thread::yield();
    });

    // Check thread properties
    std::cout << "Thread joinable: " << worker.joinable() << "\n";
    std::cout << "Thread ID: " << worker.get_id() << "\n";

    worker.join();

    // After join, thread is no longer joinable
    std::cout << "Thread joinable after join: " << worker.joinable() << "\n";
}

// RAII thread wrapper
class ThreadGuard {
private:
    std::thread& thread_;

public:
    explicit ThreadGuard(std::thread& t) : thread_(t) {}

    ~ThreadGuard() {
        if (thread_.joinable()) {
            thread_.join();
        }
    }

    // Non-copyable
    ThreadGuard(const ThreadGuard&) = delete;
    ThreadGuard& operator=(const ThreadGuard&) = delete;
};

// Usage
void safe_thread_usage() {
    std::thread worker([]() {
        // Some work
    });

    ThreadGuard guard(worker);  // Ensures thread is joined

    // Even if exception occurs, thread will be properly joined
    // in ThreadGuard destructor
}
```

### 🔒 Synchronization Primitives

#### **Mutexes and Locks**

```cpp
#include <mutex>
#include <shared_mutex>
#include <lock_guard>
#include <unique_lock>

class ResourceManager {
private:
    std::mutex mutex_;
    std::shared_mutex shared_mutex_;
    double total_resources_ = 1000.0;
    std::vector<std::string> allocation_log_;

public:
    // Basic mutex with lock_guard (RAII)
    void allocate_resources(double amount, const std::string& entity_id) {
        std::lock_guard<std::mutex> lock(mutex_);  // Automatic lock/unlock

        if (total_resources_ >= amount) {
            total_resources_ -= amount;
            allocation_log_.push_back("Allocated " + std::to_string(amount) +
                                     " to " + entity_id);
        }
    }

    // unique_lock for more flexibility
    bool try_allocate_with_timeout(double amount, const std::string& entity_id) {
        std::unique_lock<std::mutex> lock(mutex_, std::defer_lock);

        // Try to acquire lock with timeout
        if (lock.try_lock_for(std::chrono::milliseconds(100))) {
            if (total_resources_ >= amount) {
                total_resources_ -= amount;
                return true;
            }
        }
        return false;
    }

    // Shared mutex for reader-writer scenarios
    double get_total_resources() const {
        std::shared_lock<std::shared_mutex> lock(shared_mutex_);  // Shared read lock
        return total_resources_;
    }

    void reset_resources() {
        std::unique_lock<std::shared_mutex> lock(shared_mutex_);  // Exclusive write lock
        total_resources_ = 1000.0;
        allocation_log_.clear();
    }

    // Multiple lock acquisition
    void transfer_resources(ResourceManager& other, double amount) {
        // Acquire both mutexes without deadlock
        std::lock(mutex_, other.mutex_);

        // Adopt the locks
        std::lock_guard<std::mutex> lock1(mutex_, std::adopt_lock);
        std::lock_guard<std::mutex> lock2(other.mutex_, std::adopt_lock);

        if (total_resources_ >= amount) {
            total_resources_ -= amount;
            other.total_resources_ += amount;
        }
    }
};

// Scoped locking utilities
void advanced_locking_example() {
    std::mutex m1, m2, m3;

    // Lock multiple mutexes simultaneously (deadlock-free)
    std::lock(m1, m2, m3);
    std::lock_guard<std::mutex> lock1(m1, std::adopt_lock);
    std::lock_guard<std::mutex> lock2(m2, std::adopt_lock);
    std::lock_guard<std::mutex> lock3(m3, std::adopt_lock);

    // Critical section with all three mutexes locked
}

// Once flag for initialization
std::once_flag init_flag;
std::unique_ptr<ResourceManager> resource_manager;

void initialize_resource_manager() {
    resource_manager = std::make_unique<ResourceManager>();
}

ResourceManager& get_resource_manager() {
    std::call_once(init_flag, initialize_resource_manager);
    return *resource_manager;
}
```

#### **Condition Variables**

```cpp
#include <condition_variable>
#include <queue>

template<typename T>
class ThreadSafeQueue {
private:
    mutable std::mutex mutex_;
    std::queue<T> queue_;
    std::condition_variable condition_;

public:
    void push(T item) {
        std::unique_lock<std::mutex> lock(mutex_);
        queue_.push(std::move(item));
        condition_.notify_one();  // Wake up one waiting thread
    }

    // Blocking pop
    T pop() {
        std::unique_lock<std::mutex> lock(mutex_);

        // Wait until queue is not empty
        condition_.wait(lock, [this] { return !queue_.empty(); });

        T result = std::move(queue_.front());
        queue_.pop();
        return result;
    }

    // Non-blocking pop with timeout
    bool try_pop(T& item, std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock(mutex_);

        if (condition_.wait_for(lock, timeout, [this] { return !queue_.empty(); })) {
            item = std::move(queue_.front());
            queue_.pop();
            return true;
        }
        return false;
    }

    // Non-blocking pop
    bool try_pop(T& item) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) {
            return false;
        }

        item = std::move(queue_.front());
        queue_.pop();
        return true;
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }
};

// Producer-Consumer example
class MissionQueue {
private:
    ThreadSafeQueue<std::string> mission_queue_;
    std::atomic<bool> running_{true};

public:
    // Producer
    void add_mission(const std::string& mission) {
        if (running_) {
            mission_queue_.push(mission);
            std::cout << "Mission added: " << mission << "\n";
        }
    }

    // Consumer
    void process_missions() {
        while (running_) {
            std::string mission;
            if (mission_queue_.try_pop(mission, std::chrono::milliseconds(100))) {
                std::cout << "Processing mission: " << mission << "\n";
                std::this_thread::sleep_for(std::chrono::seconds(1));  // Simulate work
                std::cout << "Mission completed: " << mission << "\n";
            }
        }
    }

    void shutdown() {
        running_ = false;
    }
};

// Usage example
void producer_consumer_demo() {
    MissionQueue queue;

    // Start consumer thread
    std::thread consumer(&MissionQueue::process_missions, &queue);

    // Producer thread
    std::thread producer([&queue]() {
        for (int i = 0; i < 5; ++i) {
            queue.add_mission("Mission " + std::to_string(i));
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    });

    producer.join();
    std::this_thread::sleep_for(std::chrono::seconds(3));  // Let consumer finish
    queue.shutdown();
    consumer.join();
}
```

### ⚛️ Atomic Operations

#### **Basic Atomic Types**

```cpp
#include <atomic>

class AtomicCounter {
private:
    std::atomic<int> count_{0};
    std::atomic<bool> ready_{false};
    std::atomic<double> value_{0.0};

public:
    // Basic atomic operations
    void increment() {
        count_.fetch_add(1);  // Atomic increment
        // or: ++count_;        // Also atomic
    }

    void decrement() {
        count_.fetch_sub(1);  // Atomic decrement
        // or: --count_;        // Also atomic
    }

    int get_count() const {
        return count_.load();  // Atomic read
    }

    void set_count(int new_count) {
        count_.store(new_count);  // Atomic write
    }

    // Compare and swap
    bool try_set_count(int expected, int desired) {
        return count_.compare_exchange_strong(expected, desired);
    }

    // Exchange (set new value, return old)
    int reset_and_get_old() {
        return count_.exchange(0);
    }

    // Flag operations
    void set_ready() {
        ready_.store(true, std::memory_order_release);
    }

    bool is_ready() const {
        return ready_.load(std::memory_order_acquire);
    }

    // Atomic operations with memory ordering
    void safe_publish_value(double new_value) {
        value_.store(new_value, std::memory_order_relaxed);
        ready_.store(true, std::memory_order_release);  // Release barrier
    }

    double safe_consume_value() {
        while (!ready_.load(std::memory_order_acquire)) {  // Acquire barrier
            std::this_thread::yield();
        }
        return value_.load(std::memory_order_relaxed);
    }
};

// Atomic shared pointer (C++20)
class AtomicResourceManager {
private:
    std::atomic<std::shared_ptr<ResourceManager>> manager_;

public:
    void set_manager(std::shared_ptr<ResourceManager> new_manager) {
        manager_.store(std::move(new_manager));
    }

    std::shared_ptr<ResourceManager> get_manager() const {
        return manager_.load();
    }

    bool try_replace_manager(std::shared_ptr<ResourceManager> expected,
                            std::shared_ptr<ResourceManager> desired) {
        return manager_.compare_exchange_strong(expected, desired);
    }
};

// Lock-free stack example
template<typename T>
class LockFreeStack {
private:
    struct Node {
        T data;
        Node* next;

        Node(const T& data) : data(data), next(nullptr) {}
    };

    std::atomic<Node*> head_{nullptr};

public:
    void push(const T& data) {
        Node* new_node = new Node(data);
        new_node->next = head_.load();

        // Compare-and-swap loop
        while (!head_.compare_exchange_weak(new_node->next, new_node)) {
            // Loop continues if CAS failed
        }
    }

    bool pop(T& result) {
        Node* old_head = head_.load();

        do {
            if (!old_head) {
                return false;  // Stack is empty
            }
        } while (!head_.compare_exchange_weak(old_head, old_head->next));

        result = old_head->data;
        delete old_head;
        return true;
    }

    ~LockFreeStack() {
        while (Node* old_head = head_.load()) {
            head_ = old_head->next;
            delete old_head;
        }
    }
};
```

#### **Memory Ordering**

```cpp
// Memory ordering examples
class MemoryOrderingDemo {
private:
    std::atomic<int> data_{0};
    std::atomic<bool> ready_{false};

public:
    // Relaxed ordering - no synchronization constraints
    void relaxed_operations() {
        data_.store(42, std::memory_order_relaxed);
        ready_.store(true, std::memory_order_relaxed);
    }

    // Acquire-Release ordering
    void producer() {
        data_.store(42, std::memory_order_relaxed);
        ready_.store(true, std::memory_order_release);  // Release barrier
    }

    int consumer() {
        while (!ready_.load(std::memory_order_acquire)) {  // Acquire barrier
            std::this_thread::yield();
        }
        return data_.load(std::memory_order_relaxed);
    }

    // Sequential consistency (strongest ordering)
    void sequential_operations() {
        data_.store(42);  // Default is memory_order_seq_cst
        ready_.store(true);
    }
};

// Dekker's algorithm using atomics
class DekkerMutex {
private:
    std::atomic<bool> flag0_{false};
    std::atomic<bool> flag1_{false};
    std::atomic<int> turn_{0};

public:
    void lock0() {
        flag0_.store(true);
        while (flag1_.load()) {
            if (turn_.load() != 0) {
                flag0_.store(false);
                while (turn_.load() != 0) {
                    std::this_thread::yield();
                }
                flag0_.store(true);
            }
        }
    }

    void unlock0() {
        turn_.store(1);
        flag0_.store(false);
    }

    void lock1() {
        flag1_.store(true);
        while (flag0_.load()) {
            if (turn_.load() != 1) {
                flag1_.store(false);
                while (turn_.load() != 1) {
                    std::this_thread::yield();
                }
                flag1_.store(true);
            }
        }
    }

    void unlock1() {
        turn_.store(0);
        flag1_.store(false);
    }
};
```

### 🚀 Asynchronous Programming

#### **std::async and std::future**

```cpp
#include <future>
#include <numeric>

// Async function execution
class AsyncMissionController {
public:
    // Async with automatic thread creation
    std::future<int> calculate_mission_cost_async(const std::vector<int>& resources) {
        return std::async(std::launch::async, [](const std::vector<int>& res) {
            return std::accumulate(res.begin(), res.end(), 0);
        }, resources);
    }

    // Deferred execution (lazy evaluation)
    std::future<std::string> generate_mission_report_deferred() {
        return std::async(std::launch::deferred, []() {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            return std::string("Mission Report Generated");
        });
    }

    // Multiple async operations
    void parallel_mission_analysis() {
        std::vector<std::future<double>> futures;

        // Launch multiple async operations
        for (int i = 0; i < 5; ++i) {
            futures.push_back(std::async(std::launch::async, [i]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(100 * i));
                return i * 3.14;
            }));
        }

        // Collect results
        std::vector<double> results;
        for (auto& future : futures) {
            results.push_back(future.get());  // Block until result available
        }

        // Process results
        for (size_t i = 0; i < results.size(); ++i) {
            std::cout << "Result " << i << ": " << results[i] << "\n";
        }
    }

    // Exception handling with futures
    std::future<int> risky_calculation() {
        return std::async(std::launch::async, []() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            if (std::rand() % 2 == 0) {
                throw std::runtime_error("Calculation failed!");
            }
            return 42;
        });
    }

    void handle_async_exceptions() {
        auto future = risky_calculation();

        try {
            int result = future.get();
            std::cout << "Result: " << result << "\n";
        } catch (const std::exception& e) {
            std::cout << "Exception caught: " << e.what() << "\n";
        }
    }
};
```

#### **std::promise and std::packaged_task**

```cpp
// Promise-future communication
class MissionCommunication {
public:
    void promise_future_demo() {
        std::promise<std::string> mission_promise;
        std::future<std::string> mission_future = mission_promise.get_future();

        // Thread that sets the promise
        std::thread sender([promise = std::move(mission_promise)]() mutable {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            promise.set_value("Mission data received");
        });

        // Thread that waits for the promise
        std::thread receiver([&mission_future]() {
            std::cout << "Waiting for mission data...\n";
            std::string data = mission_future.get();
            std::cout << "Received: " << data << "\n";
        });

        sender.join();
        receiver.join();
    }

    // Exception through promise
    void promise_exception_demo() {
        std::promise<int> calc_promise;
        std::future<int> calc_future = calc_promise.get_future();

        std::thread worker([promise = std::move(calc_promise)]() mutable {
            try {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                throw std::runtime_error("Calculation error");
                promise.set_value(42);  // This won't be reached
            } catch (...) {
                promise.set_exception(std::current_exception());
            }
        });

        try {
            int result = calc_future.get();
            std::cout << "Result: " << result << "\n";
        } catch (const std::exception& e) {
            std::cout << "Exception: " << e.what() << "\n";
        }

        worker.join();
    }

    // Packaged task
    void packaged_task_demo() {
        // Package a function for async execution
        std::packaged_task<int(int, int)> task([](int a, int b) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            return a + b;
        });

        std::future<int> result_future = task.get_future();

        // Execute task in another thread
        std::thread worker(std::move(task), 10, 20);

        // Get result
        std::cout << "Calculating...\n";
        int result = result_future.get();
        std::cout << "Result: " << result << "\n";

        worker.join();
    }
};
```

#### **std::shared_future**

```cpp
// Shared future for multiple consumers
class SharedMissionData {
public:
    void shared_future_demo() {
        std::promise<std::string> data_promise;
        std::shared_future<std::string> shared_data = data_promise.get_future().share();

        // Multiple consumers
        std::vector<std::thread> consumers;
        for (int i = 0; i < 3; ++i) {
            consumers.emplace_back([shared_data, i]() {
                std::string data = shared_data.get();  // All can call get()
                std::cout << "Consumer " << i << " received: " << data << "\n";
            });
        }

        // Producer
        std::thread producer([promise = std::move(data_promise)]() mutable {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            promise.set_value("Shared mission data");
        });

        producer.join();
        for (auto& consumer : consumers) {
            consumer.join();
        }
    }

    // Future with timeout
    void future_timeout_demo() {
        auto future = std::async(std::launch::async, []() {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            return 42;
        });

        // Wait with timeout
        auto status = future.wait_for(std::chrono::seconds(2));

        if (status == std::future_status::ready) {
            std::cout << "Result: " << future.get() << "\n";
        } else if (status == std::future_status::timeout) {
            std::cout << "Operation timed out\n";
        } else {
            std::cout << "Operation deferred\n";
        }
    }
};
```

### 🏊 Thread Pools

#### **Basic Thread Pool Implementation**

```cpp
#include <functional>

class ThreadPool {
private:
    std::vector<std::thread> workers_;
    ThreadSafeQueue<std::function<void()>> task_queue_;
    std::atomic<bool> stop_{false};

public:
    explicit ThreadPool(size_t num_threads = std::thread::hardware_concurrency()) {
        for (size_t i = 0; i < num_threads; ++i) {
            workers_.emplace_back([this] {
                worker_thread();
            });
        }
    }

    ~ThreadPool() {
        stop_ = true;
        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    template<typename F, typename... Args>
    auto submit(F&& func, Args&&... args)
        -> std::future<typename std::result_of<F(Args...)>::type> {

        using return_type = typename std::result_of<F(Args...)>::type;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(func), std::forward<Args>(args)...)
        );

        std::future<return_type> result = task->get_future();

        if (!stop_) {
            task_queue_.push([task] { (*task)(); });
        }

        return result;
    }

    void shutdown() {
        stop_ = true;
    }

private:
    void worker_thread() {
        while (!stop_) {
            std::function<void()> task;
            if (task_queue_.try_pop(task, std::chrono::milliseconds(100))) {
                task();
            }
        }
    }
};

// Usage example
void thread_pool_demo() {
    ThreadPool pool(4);

    std::vector<std::future<int>> futures;

    // Submit tasks
    for (int i = 0; i < 10; ++i) {
        futures.push_back(pool.submit([i]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100 * i));
            return i * i;
        }));
    }

    // Collect results
    for (auto& future : futures) {
        std::cout << "Result: " << future.get() << "\n";
    }
}
```

#### **Advanced Thread Pool with Priority**

```cpp
template<typename T>
class PriorityTask {
private:
    int priority_;
    T task_;

public:
    PriorityTask(int priority, T task)
        : priority_(priority), task_(std::move(task)) {}

    int get_priority() const { return priority_; }

    void operator()() { task_(); }

    bool operator<(const PriorityTask& other) const {
        return priority_ < other.priority_;  // Higher priority = higher number
    }
};

class PriorityThreadPool {
private:
    std::vector<std::thread> workers_;
    std::priority_queue<PriorityTask<std::function<void()>>> task_queue_;
    std::mutex queue_mutex_;
    std::condition_variable condition_;
    std::atomic<bool> stop_{false};

public:
    explicit PriorityThreadPool(size_t num_threads) {
        for (size_t i = 0; i < num_threads; ++i) {
            workers_.emplace_back([this] {
                worker_thread();
            });
        }
    }

    ~PriorityThreadPool() {
        stop_ = true;
        condition_.notify_all();
        for (auto& worker : workers_) {
            worker.join();
        }
    }

    template<typename F>
    void submit(int priority, F&& func) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            task_queue_.emplace(priority, std::forward<F>(func));
        }
        condition_.notify_one();
    }

private:
    void worker_thread() {
        while (!stop_) {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            condition_.wait(lock, [this] { return stop_ || !task_queue_.empty(); });

            if (!task_queue_.empty()) {
                auto task = std::move(const_cast<PriorityTask<std::function<void()>>&>(task_queue_.top()));
                task_queue_.pop();
                lock.unlock();

                task();
            }
        }
    }
};
```

### 🔄 C++20 Coroutines

#### **Basic Coroutine Implementation**

```cpp
#include <coroutine>
#include <exception>

// Simple generator coroutine
template<typename T>
class Generator {
public:
    struct promise_type {
        T current_value;

        Generator get_return_object() {
            return Generator{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::suspend_always initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }

        std::suspend_always yield_value(T value) {
            current_value = value;
            return {};
        }

        void return_void() {}
        void unhandled_exception() { std::terminate(); }
    };

    std::coroutine_handle<promise_type> coro;

    Generator(std::coroutine_handle<promise_type> h) : coro(h) {}
    ~Generator() { if (coro) coro.destroy(); }

    // Move-only
    Generator(const Generator&) = delete;
    Generator& operator=(const Generator&) = delete;
    Generator(Generator&& other) : coro(other.coro) { other.coro = nullptr; }
    Generator& operator=(Generator&& other) {
        if (this != &other) {
            if (coro) coro.destroy();
            coro = other.coro;
            other.coro = nullptr;
        }
        return *this;
    }

    bool next() {
        coro.resume();
        return !coro.done();
    }

    T value() {
        return coro.promise().current_value;
    }
};

// Coroutine function
Generator<int> mission_sequence() {
    co_yield 1;  // Preparation
    co_yield 2;  // Launch
    co_yield 3;  // Travel
    co_yield 4;  // Execute
    co_yield 5;  // Return
}

// Async coroutine (C++20)
#ifdef __cpp_lib_coroutine
#include <iostream>

struct Task {
    struct promise_type {
        Task get_return_object() {
            return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_never initial_suspend() { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }
        void return_void() {}
        void unhandled_exception() { std::terminate(); }
    };

    std::coroutine_handle<promise_type> coro;
    Task(std::coroutine_handle<promise_type> h) : coro(h) {}
    ~Task() { if (coro) coro.destroy(); }
};

Task async_mission() {
    std::cout << "Mission started\n";
    co_await std::suspend_always{};  // Suspend point
    std::cout << "Mission resumed\n";
    co_await std::suspend_always{};  // Another suspend point
    std::cout << "Mission completed\n";
}
#endif
```

### 🛡️ Thread Safety Patterns

#### **Singleton Pattern (Thread-Safe)**

```cpp
// Thread-safe singleton with std::call_once
class MissionControlSingleton {
private:
    MissionControlSingleton() = default;
    static std::once_flag initialized_;
    static std::unique_ptr<MissionControlSingleton> instance_;

public:
    static MissionControlSingleton& getInstance() {
        std::call_once(initialized_, []() {
            instance_ = std::make_unique<MissionControlSingleton>();
        });
        return *instance_;
    }

    // Non-copyable, non-movable
    MissionControlSingleton(const MissionControlSingleton&) = delete;
    MissionControlSingleton& operator=(const MissionControlSingleton&) = delete;

    void execute_mission(const std::string& mission) {
        std::cout << "Executing: " << mission << "\n";
    }
};

std::once_flag MissionControlSingleton::initialized_;
std::unique_ptr<MissionControlSingleton> MissionControlSingleton::instance_;

// Meyer's singleton (C++11 thread-safe)
class SimpleSingleton {
public:
    static SimpleSingleton& getInstance() {
        static SimpleSingleton instance;  // Thread-safe in C++11
        return instance;
    }

private:
    SimpleSingleton() = default;
};
```

#### **Reader-Writer Lock Pattern**

```cpp
class ReadWriteData {
private:
    mutable std::shared_mutex mutex_;
    std::map<std::string, int> data_;

public:
    // Multiple readers can access simultaneously
    int read_value(const std::string& key) const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        auto it = data_.find(key);
        return (it != data_.end()) ? it->second : 0;
    }

    // Only one writer can access at a time
    void write_value(const std::string& key, int value) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        data_[key] = value;
    }

    // Read multiple values atomically
    std::vector<int> read_multiple(const std::vector<std::string>& keys) const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        std::vector<int> results;
        for (const auto& key : keys) {
            auto it = data_.find(key);
            results.push_back((it != data_.end()) ? it->second : 0);
        }
        return results;
    }
};
```

#### **Double-Checked Locking (Fixed)**

```cpp
class LazyInitialization {
private:
    mutable std::mutex mutex_;
    mutable std::atomic<bool> initialized_{false};
    mutable std::unique_ptr<ExpensiveResource> resource_;

public:
    const ExpensiveResource& get_resource() const {
        if (!initialized_.load(std::memory_order_acquire)) {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!initialized_.load(std::memory_order_relaxed)) {
                resource_ = std::make_unique<ExpensiveResource>();
                initialized_.store(true, std::memory_order_release);
            }
        }
        return *resource_;
    }
};
```

### 📊 Performance and Best Practices

#### **Performance Guidelines**

```cpp
// Prefer lock-free algorithms when possible
class LockFreeCounter {
private:
    std::atomic<int> count_{0};

public:
    void increment() {
        count_.fetch_add(1, std::memory_order_relaxed);  // Fastest for counters
    }

    int get() const {
        return count_.load(std::memory_order_relaxed);
    }
};

// Minimize lock contention
class OptimizedResourceManager {
private:
    std::array<std::mutex, 16> mutexes_;  // Multiple mutexes to reduce contention
    std::array<double, 16> resources_;

    size_t get_index(const std::string& key) const {
        return std::hash<std::string>{}(key) % mutexes_.size();
    }

public:
    void allocate(const std::string& entity, double amount) {
        size_t index = get_index(entity);
        std::lock_guard<std::mutex> lock(mutexes_[index]);
        resources_[index] -= amount;
    }

    double get_resources(const std::string& entity) const {
        size_t index = get_index(entity);
        std::lock_guard<std::mutex> lock(mutexes_[index]);
        return resources_[index];
    }
};

// Use RAII for exception safety
class RAIILock {
private:
    std::mutex& mutex_;
    bool locked_;

public:
    explicit RAIILock(std::mutex& m) : mutex_(m), locked_(false) {
        mutex_.lock();
        locked_ = true;
    }

    ~RAIILock() {
        if (locked_) {
            mutex_.unlock();
        }
    }

    void unlock() {
        if (locked_) {
            mutex_.unlock();
            locked_ = false;
        }
    }
};
```

### 🎯 Concurrency Anti-Patterns to Avoid

#### **Common Mistakes**

```cpp
// ❌ Bad: Race condition
class BadCounter {
private:
    int count_ = 0;  // Not atomic!

public:
    void increment() {
        ++count_;  // Race condition!
    }

    int get() const {
        return count_;  // Race condition!
    }
};

// ✅ Good: Thread-safe counter
class GoodCounter {
private:
    std::atomic<int> count_{0};

public:
    void increment() {
        count_.fetch_add(1);
    }

    int get() const {
        return count_.load();
    }
};

// ❌ Bad: Deadlock potential
void bad_transfer(BankAccount& from, BankAccount& to, double amount) {
    std::lock_guard<std::mutex> lock1(from.mutex_);
    std::lock_guard<std::mutex> lock2(to.mutex_);  // Potential deadlock!

    from.balance_ -= amount;
    to.balance_ += amount;
}

// ✅ Good: Deadlock-free transfer
void good_transfer(BankAccount& from, BankAccount& to, double amount) {
    std::lock(from.mutex_, to.mutex_);  // Lock both atomically
    std::lock_guard<std::mutex> lock1(from.mutex_, std::adopt_lock);
    std::lock_guard<std::mutex> lock2(to.mutex_, std::adopt_lock);

    from.balance_ -= amount;
    to.balance_ += amount;
}

// ❌ Bad: Lock held too long
void bad_processing() {
    std::lock_guard<std::mutex> lock(global_mutex_);

    // Long computation while holding lock
    expensive_computation();  // Bad!

    shared_data_.update();
}

// ✅ Good: Minimize lock duration
void good_processing() {
    auto result = expensive_computation();  // Compute outside lock

    {
        std::lock_guard<std::mutex> lock(global_mutex_);
        shared_data_.update(result);  // Quick update under lock
    }
}
```

### 📋 Concurrency Quick Reference

| Primitive                 | Use Case                | Performance         | Notes                |
| ------------------------- | ----------------------- | ------------------- | -------------------- |
| `std::mutex`              | Basic mutual exclusion  | Medium              | Use with RAII        |
| `std::shared_mutex`       | Reader-writer scenarios | Good for read-heavy | C++17                |
| `std::atomic`             | Simple data types       | Excellent           | Lock-free            |
| `std::condition_variable` | Thread coordination     | Good                | Use with unique_lock |
| `std::future/async`       | Async operations        | Good                | Easy to use          |
| `std::thread`             | Basic threading         | Good                | Manual management    |
| Thread pools              | Task parallelism        | Excellent           | Reuse threads        |
| Lock-free structures      | High performance        | Excellent           | Complex to implement |

### 🎯 Key Takeaways

1. **Prefer atomic operations** over mutexes for simple data
2. **Use RAII** for automatic lock management
3. **Minimize lock duration** and scope
4. **Avoid deadlocks** with consistent lock ordering or `std::lock`
5. **Choose appropriate synchronization** primitives for your use case
6. **Consider thread pools** instead of creating threads manually
7. **Use async/future** for simple parallel tasks
8. **Test thoroughly** - concurrency bugs are hard to reproduce
9. **Profile and measure** - don't assume performance benefits
10. **Start simple** - add complexity only when needed

---

_This cheat sheet covers essential concurrency concepts used throughout CppVerseHub. For practical examples, see the `src/concurrency/` directory._
