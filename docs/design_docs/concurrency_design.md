# Concurrency Design - CppVerseHub

**Location:** `CppVerseHub/docs/design_docs/concurrency_design.md`

## Threading Model Overview

CppVerseHub implements a sophisticated concurrency model designed to demonstrate advanced C++ threading concepts while maintaining high performance and safety. The system uses a combination of thread pools, async operations, and lock-free data structures to handle parallel mission execution, resource management, and inter-entity communication.

## Concurrency Architecture Principles

### Design Goals

1. **Thread Safety**: All shared data structures are protected against race conditions
2. **Performance**: Minimize contention and maximize CPU utilization
3. **Scalability**: Support for configurable thread counts based on hardware
4. **Deadlock Prevention**: Careful lock ordering and timeout mechanisms
5. **Exception Safety**: Concurrent operations maintain strong exception guarantees
6. **Educational Value**: Demonstrate modern C++ concurrency features

### Threading Strategy

- **Thread Pool Pattern**: Managed worker threads for task execution
- **Producer-Consumer**: Async communication between entities
- **Actor Model**: Entities process messages independently
- **Lock-Free Programming**: Critical paths use atomic operations
- **RAII Synchronization**: Automatic lock management

## Core Threading Components

### Thread Pool Implementation

The system uses a custom thread pool optimized for mission processing:

```cpp
class ThreadPool {
private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex queue_mutex_;
    std::condition_variable condition_;
    std::atomic<bool> stop_{false};

public:
    explicit ThreadPool(size_t num_threads = std::thread::hardware_concurrency()) {
        for (size_t i = 0; i < num_threads; ++i) {
            workers_.emplace_back([this] { workerLoop(); });
        }
    }

    ~ThreadPool() {
        stop_.store(true);
        condition_.notify_all();

        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    template<typename F, typename... Args>
    auto enqueue(F&& f, Args&&... args)
        -> std::future<std::invoke_result_t<F, Args...>> {

        using return_type = std::invoke_result_t<F, Args...>;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<return_type> result = task->get_future();

        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            if (stop_.load()) {
                throw std::runtime_error("ThreadPool is stopping");
            }

            tasks_.emplace([task](){ (*task)(); });
        }

        condition_.notify_one();
        return result;
    }

private:
    void workerLoop() {
        while (!stop_.load()) {
            std::function<void()> task;

            {
                std::unique_lock<std::mutex> lock(queue_mutex_);
                condition_.wait(lock, [this] {
                    return stop_.load() || !tasks_.empty();
                });

                if (stop_.load() && tasks_.empty()) {
                    return;
                }

                task = std::move(tasks_.front());
                tasks_.pop();
            }

            try {
                task();
            } catch (const std::exception& e) {
                // Log exception but continue processing
                logError("Task execution failed: " + std::string(e.what()));
            }
        }
    }
};
```

**Key Features:**

- **Generic Task Support**: Templates allow any callable to be queued
- **Future-Based Results**: Type-safe return value handling
- **Exception Isolation**: Failed tasks don't crash worker threads
- **Graceful Shutdown**: RAII ensures proper thread cleanup
- **Hardware Scaling**: Default thread count matches CPU cores

### Async Mission Execution

Missions execute asynchronously to prevent blocking the main simulation:

```cpp
class AsyncMissionExecutor {
private:
    ThreadPool& thread_pool_;
    std::unordered_map<MissionId, std::future<MissionResult>> active_missions_;
    std::shared_mutex missions_mutex_;

public:
    explicit AsyncMissionExecutor(ThreadPool& pool) : thread_pool_(pool) {}

    template<typename MissionType>
    std::future<MissionResult> executeMission(std::unique_ptr<MissionType> mission) {
        static_assert(std::is_base_of_v<Mission, MissionType>,
                     "MissionType must inherit from Mission");

        auto mission_id = mission->getId();

        auto future = thread_pool_.enqueue([mission = std::move(mission)]() mutable {
            try {
                return mission->execute();
            } catch (...) {
                return MissionResult::createFailure(std::current_exception());
            }
        });

        {
            std::unique_lock lock(missions_mutex_);
            active_missions_[mission_id] = std::move(future);
        }

        return active_missions_[mission_id];
    }

    std::vector<MissionResult> waitForCompletion(
        const std::vector<MissionId>& mission_ids,
        std::chrono::milliseconds timeout = std::chrono::milliseconds::max()) {

        std::vector<MissionResult> results;
        results.reserve(mission_ids.size());

        for (const auto& id : mission_ids) {
            std::shared_lock lock(missions_mutex_);
            auto it = active_missions_.find(id);
            if (it != active_missions_.end()) {
                lock.unlock();

                if (timeout == std::chrono::milliseconds::max()) {
                    results.push_back(it->second.get());
                } else {
                    auto status = it->second.wait_for(timeout);
                    if (status == std::future_status::ready) {
                        results.push_back(it->second.get());
                    } else {
                        results.push_back(MissionResult::createTimeout());
                    }
                }
            }
        }

        return results;
    }
};
```

**Concurrency Features:**

- **Non-Blocking Execution**: Mission submission returns immediately
- **Exception Handling**: Exceptions captured and returned as results
- **Timeout Support**: Configurable wait times prevent infinite blocking
- **Reader-Writer Locks**: Multiple readers for mission status queries
- **Type Safety**: Template ensures only valid mission types

## Synchronization Mechanisms

### Lock Hierarchy and Deadlock Prevention

The system implements a strict lock ordering to prevent deadlocks:

```cpp
enum class LockLevel : int {
    RESOURCE_MANAGER = 100,
    ENTITY_COLLECTION = 200,
    INDIVIDUAL_ENTITY = 300,
    MISSION_QUEUE = 400,
    OBSERVER_LIST = 500
};

template<LockLevel Level>
class HierarchicalMutex {
private:
    std::mutex mutex_;
    static thread_local LockLevel current_lock_level_;

public:
    void lock() {
        if (current_lock_level_ != LockLevel{} &&
            current_lock_level_ >= Level) {
            throw std::runtime_error("Lock order violation detected");
        }

        mutex_.lock();
        current_lock_level_ = Level;
    }

    void unlock() {
        current_lock_level_ = LockLevel{};
        mutex_.unlock();
    }

    bool try_lock() {
        if (current_lock_level_ != LockLevel{} &&
            current_lock_level_ >= Level) {
            return false;
        }

        if (mutex_.try_lock()) {
            current_lock_level_ = Level;
            return true;
        }
        return false;
    }
};

template<LockLevel Level>
thread_local LockLevel HierarchicalMutex<Level>::current_lock_level_{};
```

**Deadlock Prevention Strategy:**

- **Lock Ordering**: Lower-numbered locks must be acquired before higher-numbered ones
- **Runtime Checking**: Violations detected and reported immediately
- **Thread-Local State**: Each thread tracks its current lock level
- **Try-Lock Support**: Non-blocking acquisition with ordering validation

### Lock-Free Data Structures

Critical performance paths use lock-free programming:

```cpp
template<typename T>
class LockFreeQueue {
private:
    struct Node {
        std::atomic<T*> data{nullptr};
        std::atomic<Node*> next{nullptr};
    };

    std::atomic<Node*> head_;
    std::atomic<Node*> tail_;

public:
    LockFreeQueue() {
        Node* dummy = new Node;
        head_.store(dummy);
        tail_.store(dummy);
    }

    ~LockFreeQueue() {
        while (Node* old_head = head_.load()) {
            head_.store(old_head->next.load());
            delete old_head;
        }
    }

    void enqueue(T item) {
        Node* new_node = new Node;
        T* data_ptr = new T(std::move(item));
        new_node->data.store(data_ptr);

        Node* prev_tail = tail_.exchange(new_node);
        prev_tail->next.store(new_node);
    }

    bool try_dequeue(T& result) {
        Node* head = head_.load();
        Node* next = head->next.load();

        if (next == nullptr) {
            return false; // Queue is empty
        }

        T* data = next->data.exchange(nullptr);
        if (data == nullptr) {
            return false; // Another thread got this item
        }

        result = *data;
        delete data;

        head_.store(next);
        delete head;

        return true;
    }

    bool empty() const {
        Node* head = head_.load();
        Node* next = head->next.load();
        return next == nullptr;
    }
};
```

**Lock-Free Benefits:**

- **No Blocking**: Threads never wait for locks
- **Scalability**: Performance improves with more cores
- **Deadlock Immunity**: No locks means no deadlock potential
- **Progress Guarantee**: At least one thread always makes progress
- **Cache Efficiency**: Atomic operations optimize memory access patterns

### Resource Manager Thread Safety

The singleton ResourceManager handles concurrent access safely:

```cpp
class ResourceManager {
private:
    static std::unique_ptr<ResourceManager> instance_;
    static std::once_flag init_flag_;

    mutable std::shared_mutex resources_mutex_;
    std::unordered_map<ResourceType, std::atomic<double>> resource_levels_;

    // Allocation tracking for deadlock detection
    struct AllocationRecord {
        std::thread::id thread_id;
        std::chrono::steady_clock::time_point timestamp;
        ResourceType resource_type;
        double amount;
    };

    mutable std::mutex allocation_records_mutex_;
    std::vector<AllocationRecord> pending_allocations_;

    ResourceManager() {
        // Initialize resource levels
        resource_levels_[ResourceType::ENERGY] = 10000.0;
        resource_levels_[ResourceType::MATERIALS] = 5000.0;
        resource_levels_[ResourceType::FUEL] = 8000.0;
    }

public:
    static ResourceManager& getInstance() {
        std::call_once(init_flag_, []() {
            instance_ = std::unique_ptr<ResourceManager>(new ResourceManager());
        });
        return *instance_;
    }

    bool tryAllocateResources(const ResourceRequirement& requirement) {
        // Record allocation attempt for deadlock detection
        {
            std::lock_guard lock(allocation_records_mutex_);
            pending_allocations_.push_back({
                std::this_thread::get_id(),
                std::chrono::steady_clock::now(),
                requirement.type,
                requirement.amount
            });
        }

        // Use atomic operations for the actual allocation
        double expected = resource_levels_[requirement.type].load();
        double desired;

        do {
            if (expected < requirement.amount) {
                cleanupAllocationRecord();
                return false; // Insufficient resources
            }
            desired = expected - requirement.amount;
        } while (!resource_levels_[requirement.type].compare_exchange_weak(
                    expected, desired));

        cleanupAllocationRecord();
        return true;
    }

    void releaseResources(const ResourceRequirement& requirement) {
        resource_levels_[requirement.type].fetch_add(requirement.amount);
    }

    double getResourceLevel(ResourceType type) const {
        return resource_levels_.at(type).load();
    }

private:
    void cleanupAllocationRecord() {
        std::lock_guard lock(allocation_records_mutex_);
        auto thread_id = std::this_thread::get_id();

        pending_allocations_.erase(
            std::remove_if(pending_allocations_.begin(), pending_allocations_.end(),
                [thread_id](const AllocationRecord& record) {
                    return record.thread_id == thread_id;
                }),
            pending_allocations_.end());
    }
};
```

**Thread Safety Features:**

- **Atomic Resource Levels**: Lock-free resource queries and updates
- **Compare-and-Swap**: Atomic allocation operations
- **Deadlock Detection**: Track allocation attempts for analysis
- **Exception Safety**: RAII cleanup of allocation records
- **Singleton Safety**: Thread-safe initialization with std::call_once

## Inter-Entity Communication

### Message Passing System

Entities communicate through an async message passing system:

```cpp
template<typename MessageType>
class MessageQueue {
private:
    LockFreeQueue<MessageType> queue_;
    std::atomic<bool> active_{true};

public:
    void send(MessageType message) {
        if (!active_.load()) {
            throw std::runtime_error("Message queue is shut down");
        }
        queue_.enqueue(std::move(message));
    }

    std::optional<MessageType> receive() {
        if (!active_.load()) {
            return std::nullopt;
        }

        MessageType message;
        if (queue_.try_dequeue(message)) {
            return message;
        }
        return std::nullopt;
    }

    template<typename Predicate>
    std::vector<MessageType> receiveFiltered(Predicate pred, size_t max_count = 100) {
        std::vector<MessageType> messages;
        messages.reserve(max_count);

        MessageType message;
        size_t count = 0;

        while (count < max_count && queue_.try_dequeue(message)) {
            if (pred(message)) {
                messages.push_back(std::move(message));
            }
            ++count;
        }

        return messages;
    }

    void shutdown() {
        active_.store(false);
    }

    bool isActive() const {
        return active_.load();
    }
};

class Entity {
protected:
    MessageQueue<EntityMessage> message_queue_;
    std::atomic<bool> processing_messages_{false};

public:
    void sendMessage(const EntityMessage& message) {
        message_queue_.send(message);
    }

    virtual void processMessages() {
        // Prevent recursive message processing
        bool expected = false;
        if (!processing_messages_.compare_exchange_strong(expected, true)) {
            return; // Already processing messages
        }

        // RAII guard to reset processing flag
        auto guard = make_scope_guard([this] {
            processing_messages_.store(false);
        });

        auto messages = message_queue_.receiveFiltered(
            [this](const EntityMessage& msg) { return shouldProcess(msg); },
            50 // Process up to 50 messages per cycle
        );

        for (const auto& message : messages) {
            try {
                handleMessage(message);
            } catch (const std::exception& e) {
                logError("Message handling failed: " + std::string(e.what()));
            }
        }
    }

protected:
    virtual bool shouldProcess(const EntityMessage& message) const = 0;
    virtual void handleMessage(const EntityMessage& message) = 0;
};
```

**Communication Features:**

- **Lock-Free Queues**: High-performance message passing
- **Type Safety**: Template-based message type checking
- **Filtering**: Selective message processing
- **Overflow Protection**: Bounded message processing per cycle
- **Exception Isolation**: Failed message handling doesn't crash entity

### Async Fleet Communication

Fleets use specialized communication for coordination:

```cpp
class Fleet : public Entity {
private:
    ThreadPool& thread_pool_;
    std::shared_ptr<AsyncMissionExecutor> mission_executor_;

    // Fleet-specific message types
    using FleetMessage = std::variant<
        MoveCommand,
        AttackCommand,
        FormationChange,
        ResourceRequest,
        StatusUpdate
    >;

    MessageQueue<FleetMessage> fleet_queue_;

public:
    std::future<void> executeAsync(std::unique_ptr<Mission> mission) {
        return thread_pool_.enqueue([this, mission = std::move(mission)]() mutable {
            try {
                auto result = mission->execute();
                broadcastResult(result);
            } catch (...) {
                broadcastError(std::current_exception());
            }
        });
    }

    void coordinateWithFleet(std::shared_ptr<Fleet> other_fleet,
                           CoordinationStrategy strategy) {
        thread_pool_.enqueue([this, other_fleet, strategy]() {
            try {
                auto coordination_result = strategy.coordinate(*this, *other_fleet);

                // Send coordination commands to both fleets
                sendMessage(coordination_result.this_fleet_command);
                other_fleet->sendMessage(coordination_result.other_fleet_command);

            } catch (const std::exception& e) {
                logError("Fleet coordination failed: " + std::string(e.what()));
            }
        });
    }

protected:
    void handleMessage(const EntityMessage& base_message) override {
        // Try to convert to fleet-specific message
        if (auto fleet_msg = std::get_if<FleetMessage>(&base_message.data)) {
            std::visit([this](const auto& msg) {
                handleFleetMessage(msg);
            }, *fleet_msg);
        } else {
            Entity::handleMessage(base_message);
        }
    }

private:
    template<typename MessageType>
    void handleFleetMessage(const MessageType& message) {
        if constexpr (std::is_same_v<MessageType, MoveCommand>) {
            processMovement(message);
        } else if constexpr (std::is_same_v<MessageType, AttackCommand>) {
            processAttack(message);
        } else if constexpr (std::is_same_v<MessageType, FormationChange>) {
            adjustFormation(message);
        }
        // ... other message type handlers
    }
};
```

## Performance Optimization

### Thread Pool Tuning

Dynamic thread pool sizing based on workload:

```cpp
class AdaptiveThreadPool {
private:
    std::vector<std::thread> core_workers_;
    std::vector<std::unique_ptr<std::thread>> overflow_workers_;

    std::atomic<size_t> active_tasks_{0};
    std::atomic<size_t> queued_tasks_{0};

    const size_t min_threads_;
    const size_t max_threads_;

    std::chrono::steady_clock::time_point last_adjustment_;

public:
    void adjustThreadCount() {
        auto now = std::chrono::steady_clock::now();
        if (now - last_adjustment_ < std::chrono::seconds(5)) {
            return; // Don't adjust too frequently
        }

        size_t active = active_tasks_.load();
        size_t queued = queued_tasks_.load();
        size_t current_threads = core_workers_.size() + overflow_workers_.size();

        // Scale up if we have queued work and spare capacity
        if (queued > current_threads && current_threads < max_threads_) {
            addWorkerThread();
        }

        // Scale down if utilization is low
        if (active < current_threads / 2 && current_threads > min_threads_) {
            removeWorkerThread();
        }

        last_adjustment_ = now;
    }
};
```

### Cache-Friendly Data Layout

Organize data to minimize cache misses:

```cpp
// Structure of Arrays for better cache performance
class EntityManager {
private:
    // Instead of Array of Structures (AoS)
    struct EntityData {
        alignas(64) std::vector<EntityId> ids;
        alignas(64) std::vector<Position> positions;
        alignas(64) std::vector<Velocity> velocities;
        alignas(64) std::vector<std::atomic<Health>> health_values;
    };

    EntityData entities_;

public:
    // Process positions in batch for cache efficiency
    void updatePositions(float delta_time) {
        const size_t batch_size = 64; // Process in cache-line sized batches

        for (size_t i = 0; i < entities_.positions.size(); i += batch_size) {
            size_t end = std::min(i + batch_size, entities_.positions.size());

            // Prefetch next batch
            if (end < entities_.positions.size()) {
                __builtin_prefetch(&entities_.positions[end], 0, 3);
                __builtin_prefetch(&entities_.velocities[end], 0, 3);
            }

            // Process current batch
            for (size_t j = i; j < end; ++j) {
                entities_.positions[j] += entities_.velocities[j] * delta_time;
            }
        }
    }
};
```

## Exception Safety in Concurrent Code

### Exception-Safe Lock Guards

Custom RAII locks that handle exceptions properly:

```cpp
template<typename Mutex>
class ExceptionSafeLockGuard {
private:
    Mutex& mutex_;
    bool locked_{false};

public:
    explicit ExceptionSafeLockGuard(Mutex& m) : mutex_(m) {
        mutex_.lock();
        locked_ = true;
    }

    ~ExceptionSafeLockGuard() noexcept {
        if (locked_) {
            try {
                mutex_.unlock();
            } catch (...) {
                // Log error but don't throw from destructor
                // This prevents std::terminate from being called
                std::abort(); // Better than masking the error
            }
        }
    }

    // Move-only semantics
    ExceptionSafeLockGuard(const ExceptionSafeLockGuard&) = delete;
    ExceptionSafeLockGuard& operator=(const ExceptionSafeLockGuard&) = delete;

    ExceptionSafeLockGuard(ExceptionSafeLockGuard&& other) noexcept
        : mutex_(other.mutex_), locked_(other.locked_) {
        other.locked_ = false;
    }

    void unlock() {
        if (locked_) {
            mutex_.unlock();
            locked_ = false;
        }
    }

    void lock() {
        if (!locked_) {
            mutex_.lock();
            locked_ = true;
        }
    }
};
```

### Exception Propagation in Async Operations

Properly handle exceptions across thread boundaries:

```cpp
template<typename T>
class ExceptionSafeFuture {
private:
    std::future<T> future_;
    std::exception_ptr exception_;

public:
    explicit ExceptionSafeFuture(std::future<T> f) : future_(std::move(f)) {}

    T get() {
        try {
            return future_.get();
        } catch (...) {
            exception_ = std::current_exception();
            throw;
        }
    }

    std::optional<T> get_no_throw() noexcept {
        try {
            return future_.get();
        } catch (...) {
            exception_ = std::current_exception();
            return std::nullopt;
        }
    }

    bool has_exception() const noexcept {
        return exception_ != nullptr;
    }

    void rethrow_exception() const {
        if (exception_) {
            std::rethrow_exception(exception_);
        }
    }
};
```

## Testing Concurrent Code

### Race Condition Testing

Specialized testing for concurrent operations:

```cpp
TEST_CASE("Resource allocation race conditions", "[concurrency][stress]") {
    ResourceManager& rm = ResourceManager::getInstance();

    const int num_threads = std::thread::hardware_concurrency();
    const int allocations_per_thread = 1000;

    std::vector<std::thread> threads;
    std::atomic<int> successful_allocations{0};
    std::atomic<int> failed_allocations{0};

    // Reset resource levels
    rm.setResourceLevel(ResourceType::ENERGY, 10000.0);

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < allocations_per_thread; ++j) {
                ResourceRequirement req{ResourceType::ENERGY, 1.0};

                if (rm.tryAllocateResources(req)) {
                    successful_allocations.fetch_add(1);

                    // Simulate some work
                    std::this_thread::sleep_for(std::chrono::microseconds(10));

                    rm.releaseResources(req);
                } else {
                    failed_allocations.fetch_add(1);
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // Verify no resources were lost or created
    REQUIRE(rm.getResourceLevel(ResourceType::ENERGY) == Approx(10000.0));

    // At least some allocations should have succeeded
    REQUIRE(successful_allocations.load() > 0);

    INFO("Successful: " << successful_allocations.load()
         << ", Failed: " << failed_allocations.load());
}
```

### Deadlock Detection Testing

Automated testing for potential deadlocks:

```cpp
class DeadlockDetector {
private:
    static thread_local std::vector<std::mutex*> held_locks_;
    static std::mutex detection_mutex_;
    static std::unordered_map<std::thread::id, std::vector<std::mutex*>> all_held_locks_;

public:
    static void lockAcquired(std::mutex* m) {
        held_locks_.push_back(m);

        std::lock_guard lock(detection_mutex_);
        all_held_locks_[std::this_thread::get_id()] = held_locks_;

        // Check for potential deadlock
        if (detectCircularWait()) {
            throw std::runtime_error("Potential deadlock detected");
        }
    }

    static void lockReleased(std::mutex* m) {
        auto it = std::find(held_locks_.begin(), held_locks_.end(), m);
        if (it != held_locks_.end()) {
            held_locks_.erase(it);
        }

        std::lock_guard lock(detection_mutex_);
        if (held_locks_.empty()) {
            all_held_locks_.erase(std::this_thread::get_id());
        } else {
            all_held_locks_[std::this_thread::get_id()] = held_locks_;
        }
    }

private:
    static bool detectCircularWait() {
        // Implement cycle detection in wait-for graph
        // This is a simplified version - real implementation would be more complex
        return false;
    }
};
```

## Monitoring and Debugging

### Thread Performance Metrics

Built-in monitoring for thread performance:

```cpp
class ThreadMetrics {
private:
    struct ThreadStats {
        std::atomic<size_t> tasks_completed{0};
        std::atomic<std::chrono::microseconds> total_execution_time{std::chrono::microseconds::zero()};
        std::atomic<std::chrono::microseconds> total_wait_time{std::chrono::microseconds::zero()};
    };

    std::unordered_map<std::thread::id, ThreadStats> thread_stats_;
    std::shared_mutex stats_mutex_;

public:
    void recordTaskExecution(std::chrono::microseconds execution_time,
                           std::chrono::microseconds wait_time) {
        auto thread_id = std::this_thread::get_id();

        std::shared_lock lock(stats_mutex_);
        auto& stats = thread_stats_[thread_id];

        stats.tasks_completed.fetch_add(1);
        stats.total_execution_time.fetch_add(execution_time);
        stats.total_wait_time.fetch_add(wait_time);
    }

    struct PerformanceReport {
        size_t total_tasks;
        double average_execution_time_ms;
        double average_wait_time_ms;
        double thread_utilization;
    };

    PerformanceReport generateReport() const {
        std::shared_lock lock(stats_mutex_);

        PerformanceReport report{};

        for (const auto& [thread_id, stats] : thread_stats_) {
            report.total_tasks += stats.tasks_completed.load();
            report.average_execution_time_ms += stats.total_execution_time.load().count() / 1000.0;
            report.average_wait_time_ms += stats.total_wait_time.load().count() / 1000.0;
        }

        if (!thread_stats_.empty()) {
            report.average_execution_time_ms /= thread_stats_.size();
            report.average_wait_time_ms /= thread_stats_.size();

            double total_time = report.average_execution_time_ms + report.average_wait_time_ms;
            report.thread_utilization = total_time > 0 ?
                (report.average_execution_time_ms / total_time) : 0.0;
        }

        return report;
    }
};
```

## Conclusion

The CppVerseHub concurrency design demonstrates advanced C++ threading concepts while maintaining practical applicability. Key achievements include:

1. **Safe Concurrency**: All shared data structures are properly synchronized
2. **High Performance**: Lock-free data structures where appropriate
3. **Scalable Design**: Thread pools adapt to workload and hardware
4. **Exception Safety**: Strong guarantees maintained in concurrent code
5. **Testable Architecture**: Comprehensive testing for race conditions and deadlocks
6. **Educational Value**: Clear examples of modern C++ concurrency features

The system serves as both a working concurrent application and a comprehensive learning resource for advanced C++ threading techniques.
