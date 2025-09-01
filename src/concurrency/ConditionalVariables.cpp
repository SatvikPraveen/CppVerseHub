/**
 * @file ConditionVariables.cpp
 * @brief Implementation of comprehensive condition variable demonstrations
 * @details File location: src/concurrency/ConditionVariables.cpp
 */

#include "ConditionVariables.hpp"
#include <iomanip>
#include <sstream>

namespace CppVerseHub::Concurrency {

    // ========== BasicConditionVariableDemo Implementation ==========

    void BasicConditionVariableDemo::demonstrate_basic_wait_notify() {
        std::cout << "=== Basic Wait/Notify Pattern ===\n";
        
        ready_ = false;
        shared_data_ = "";
        
        std::thread consumer([this]() {
            std::unique_lock<std::mutex> lock(mutex_);
            std::cout << "Consumer: Waiting for data...\n";
            
            cv_.wait(lock, [this] { return ready_; });
            
            std::cout << "Consumer: Received data: " << shared_data_ << std::endl;
        });
        
        std::thread producer([this]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            
            {
                std::lock_guard<std::mutex> lock(mutex_);
                shared_data_ = "Hello from producer!";
                ready_ = true;
                std::cout << "Producer: Data prepared\n";
            }
            
            cv_.notify_one();
        });
        
        producer.join();
        consumer.join();
        
        std::cout << "Basic wait/notify completed\n\n";
    }

    void BasicConditionVariableDemo::demonstrate_predicate_wait() {
        std::cout << "=== Predicate-based Wait ===\n";
        
        shared_counter_ = 0;
        
        std::vector<std::thread> workers;
        
        // Start multiple workers that increment counter
        for (int i = 0; i < 3; ++i) {
            workers.emplace_back([this, i]() {
                for (int j = 0; j < 5; ++j) {
                    {
                        std::lock_guard<std::mutex> lock(mutex_);
                        shared_counter_++;
                        std::cout << "Worker " << i << " incremented counter to " << shared_counter_ << std::endl;
                    }
                    cv_.notify_all();
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            });
        }
        
        // Waiter that waits for counter to reach threshold
        std::thread waiter([this]() {
            std::unique_lock<std::mutex> lock(mutex_);
            std::cout << "Waiter: Waiting for counter to reach 10...\n";
            
            cv_.wait(lock, [this] { return shared_counter_ >= 10; });
            
            std::cout << "Waiter: Counter reached " << shared_counter_ << "!\n";
        });
        
        for (auto& w : workers) w.join();
        waiter.join();
        
        std::cout << "Predicate wait completed\n\n";
    }

    void BasicConditionVariableDemo::demonstrate_spurious_wakeup_handling() {
        std::cout << "=== Spurious Wakeup Handling ===\n";
        
        ready_ = false;
        
        std::thread waiter([this]() {
            std::unique_lock<std::mutex> lock(mutex_);
            std::cout << "Waiter: Waiting with spurious wakeup protection...\n";
            
            int wakeup_count = 0;
            cv_.wait(lock, [this, &wakeup_count] {
                wakeup_count++;
                std::cout << "Waiter: Wakeup #" << wakeup_count << " - Ready: " << ready_ << std::endl;
                return ready_;
            });
            
            std::cout << "Waiter: Finally ready after " << wakeup_count << " wakeups\n";
        });
        
        // Simulate spurious wakeups
        std::thread spurious_notifier([this]() {
            for (int i = 0; i < 3; ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                std::cout << "Spurious notify #" << (i + 1) << std::endl;
                cv_.notify_one();
            }
            
            // Finally set ready to true
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            {
                std::lock_guard<std::mutex> lock(mutex_);
                ready_ = true;
            }
            cv_.notify_one();
        });
        
        waiter.join();
        spurious_notifier.join();
        
        std::cout << "Spurious wakeup handling completed\n\n";
    }

    void BasicConditionVariableDemo::demonstrate_timeout_operations() {
        std::cout << "=== Timeout Operations ===\n";
        
        ready_ = false;
        
        std::thread timeout_waiter([this]() {
            std::unique_lock<std::mutex> lock(mutex_);
            std::cout << "Timeout waiter: Waiting for 1 second...\n";
            
            if (cv_.wait_for(lock, std::chrono::seconds(1), [this] { return ready_; })) {
                std::cout << "Timeout waiter: Condition met before timeout\n";
            } else {
                std::cout << "Timeout waiter: Timeout occurred\n";
            }
        });
        
        std::thread delayed_notifier([this]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(1500)); // Longer than timeout
            {
                std::lock_guard<std::mutex> lock(mutex_);
                ready_ = true;
            }
            cv_.notify_one();
        });
        
        timeout_waiter.join();
        delayed_notifier.join();
        
        // Test successful timeout
        ready_ = false;
        
        std::thread successful_timeout_waiter([this]() {
            std::unique_lock<std::mutex> lock(mutex_);
            std::cout << "Success waiter: Waiting for 2 seconds...\n";
            
            if (cv_.wait_for(lock, std::chrono::seconds(2), [this] { return ready_; })) {
                std::cout << "Success waiter: Condition met within timeout\n";
            } else {
                std::cout << "Success waiter: Timeout occurred\n";
            }
        });
        
        std::thread quick_notifier([this]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Shorter than timeout
            {
                std::lock_guard<std::mutex> lock(mutex_);
                ready_ = true;
            }
            cv_.notify_one();
        });
        
        successful_timeout_waiter.join();
        quick_notifier.join();
        
        std::cout << "Timeout operations completed\n\n";
    }

    void BasicConditionVariableDemo::demonstrate_notify_all_vs_notify_one() {
        std::cout << "=== notify_all vs notify_one ===\n";
        
        shared_counter_ = 0;
        
        // Test notify_one
        std::cout << "Testing notify_one:\n";
        std::vector<std::thread> workers_one;
        
        for (int i = 0; i < 3; ++i) {
            workers_one.emplace_back([this, i]() {
                std::unique_lock<std::mutex> lock(mutex_);
                cv_.wait(lock, [this] { return shared_counter_ > 0; });
                shared_counter_--;
                std::cout << "Worker " << i << " processed item (remaining: " << shared_counter_ << ")\n";
            });
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        {
            std::lock_guard<std::mutex> lock(mutex_);
            shared_counter_ = 1; // Only one item available
        }
        cv_.notify_one(); // Only one worker will be awakened
        
        for (auto& w : workers_one) w.join();
        
        std::cout << "\nTesting notify_all:\n";
        shared_counter_ = 0;
        std::vector<std::thread> workers_all;
        
        for (int i = 0; i < 3; ++i) {
            workers_all.emplace_back([this, i]() {
                std::unique_lock<std::mutex> lock(mutex_);
                cv_.wait(lock, [this] { return shared_counter_ > 0; });
                if (shared_counter_ > 0) {
                    shared_counter_--;
                    std::cout << "Worker " << i << " processed item (remaining: " << shared_counter_ << ")\n";
                } else {
                    std::cout << "Worker " << i << " found no items to process\n";
                }
            });
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        {
            std::lock_guard<std::mutex> lock(mutex_);
            shared_counter_ = 3; // Multiple items available
        }
        cv_.notify_all(); // All workers will be awakened
        
        for (auto& w : workers_all) w.join();
        
        std::cout << "notify_all vs notify_one completed\n\n";
    }

    // ========== ThreadPool Implementation ==========

    ThreadPool::ThreadPool(size_t num_threads) {
        workers_.reserve(num_threads);
        
        for (size_t i = 0; i < num_threads; ++i) {
            workers_.emplace_back(&ThreadPool::worker_thread, this);
        }
    }

    ThreadPool::~ThreadPool() {
        shutdown();
    }

    void ThreadPool::shutdown() {
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            stop_ = true;
        }
        
        condition_.notify_all();
        
        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
        
        workers_.clear();
    }

    size_t ThreadPool::active_threads() const {
        return active_count_.load();
    }

    size_t ThreadPool::pending_tasks() const {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        return tasks_.size();
    }

    void ThreadPool::worker_thread() {
        while (true) {
            std::function<void()> task;
            
            {
                std::unique_lock<std::mutex> lock(queue_mutex_);
                
                condition_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
                
                if (stop_ && tasks_.empty()) {
                    return;
                }
                
                task = std::move(tasks_.front());
                tasks_.pop();
            }
            
            active_count_++;
            task();
            active_count_--;
        }
    }

    // ========== WorkflowCoordinator Implementation ==========

    WorkflowCoordinator::WorkflowCoordinator(size_t num_workers_per_stage) {
        // Initialize stage queues and synchronization primitives
        std::vector<Stage> stages = {
            Stage::INITIALIZATION, Stage::DATA_PROCESSING, Stage::VALIDATION,
            Stage::OUTPUT_GENERATION, Stage::CLEANUP
        };
        
        for (auto stage : stages) {
            stage_queues_[stage] = std::queue<std::shared_ptr<Task>>();
            tasks_processed_[stage] = 0;
        }
    }

    WorkflowCoordinator::~WorkflowCoordinator() {
        stop_workflow();
    }

    void WorkflowCoordinator::submit_task(const std::string& name, const std::string& data) {
        auto task = std::make_shared<Task>();
        task->id = ++task_counter_;
        task->name = name;
        task->data = data;
        task->current_stage = Stage::INITIALIZATION;
        task->created_at = std::chrono::steady_clock::now();
        
        {
            std::lock_guard<std::mutex> lock(stage_mutexes_[Stage::INITIALIZATION]);
            stage_queues_[Stage::INITIALIZATION].push(task);
        }
        stage_conditions_[Stage::INITIALIZATION].notify_one();
        
        std::cout << "📋 Submitted task " << task->id << ": " << name << std::endl;
    }

    void WorkflowCoordinator::start_workflow(std::chrono::seconds duration) {
        std::cout << "🏭 Starting Workflow Coordinator\n";
        
        running_ = true;
        
        // Start worker threads for each stage
        std::vector<Stage> stages = {
            Stage::INITIALIZATION, Stage::DATA_PROCESSING, Stage::VALIDATION,
            Stage::OUTPUT_GENERATION, Stage::CLEANUP
        };
        
        for (auto stage : stages) {
            for (int i = 0; i < 2; ++i) { // 2 workers per stage
                worker_threads_.emplace_back(&WorkflowCoordinator::stage_worker, this, stage, i);
            }
        }
        
        // Let workflow run
        std::this_thread::sleep_for(duration);
        
        stop_workflow();
    }

    void WorkflowCoordinator::stop_workflow() {
        running_ = false;
        
        // Notify all workers
        for (auto& [stage, condition] : stage_conditions_) {
            condition.notify_all();
        }
        
        // Join all threads
        for (auto& thread : worker_threads_) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        
        worker_threads_.clear();
        std::cout << "🛑 Workflow stopped\n";
    }

    void WorkflowCoordinator::print_statistics() const {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        
        std::cout << "\n📊 === WORKFLOW STATISTICS ===\n";
        for (const auto& [stage, count] : tasks_processed_) {
            std::cout << stage_to_string(stage) << ": " << count.load() << " tasks processed\n";
        }
        std::cout << "Completed tasks: " << completed_tasks_.size() << std::endl;
        std::cout << "=============================\n\n";
    }

    void WorkflowCoordinator::stage_worker(Stage stage, int worker_id) {
        while (running_) {
            std::shared_ptr<Task> task;
            
            {
                std::unique_lock<std::mutex> lock(stage_mutexes_[stage]);
                stage_conditions_[stage].wait(lock, [this, stage] {
                    return !running_ || !stage_queues_[stage].empty();
                });
                
                if (!running_) break;
                
                if (!stage_queues_[stage].empty()) {
                    task = stage_queues_[stage].front();
                    stage_queues_[stage].pop();
                }
            }
            
            if (task) {
                process_task_at_stage(task, stage);
                advance_task_to_next_stage(task);
                tasks_processed_[stage]++;
            }
        }
    }

    void WorkflowCoordinator::process_task_at_stage(std::shared_ptr<Task> task, Stage stage) {
        std::cout << "⚙️  Worker processing Task " << task->id 
                  << " at " << stage_to_string(stage) << std::endl;
        
        // Simulate processing time
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> delay(200, 800);
        std::this_thread::sleep_for(std::chrono::milliseconds(delay(gen)));
        
        // Modify task data based on stage
        switch (stage) {
            case Stage::INITIALIZATION:
                task->data = "[INIT] " + task->data;
                break;
            case Stage::DATA_PROCESSING:
                task->data = "[PROCESSED] " + task->data;
                break;
            case Stage::VALIDATION:
                task->data = "[VALIDATED] " + task->data;
                break;
            case Stage::OUTPUT_GENERATION:
                task->data = "[OUTPUT] " + task->data;
                break;
            case Stage::CLEANUP:
                task->data = "[CLEAN] " + task->data;
                break;
            default:
                break;
        }
        
        task->current_stage = stage;
    }

    void WorkflowCoordinator::advance_task_to_next_stage(std::shared_ptr<Task> task) {
        Stage next = next_stage(task->current_stage);
        
        if (next == Stage::COMPLETED) {
            task->completed_at = std::chrono::steady_clock::now();
            std::lock_guard<std::mutex> lock(stats_mutex_);
            completed_tasks_.push_back(task);
            std::cout << "✅ Task " << task->id << " completed: " << task->data << std::endl;
        } else {
            {
                std::lock_guard<std::mutex> lock(stage_mutexes_[next]);
                stage_queues_[next].push(task);
            }
            stage_conditions_[next].notify_one();
        }
    }

    std::string WorkflowCoordinator::stage_to_string(Stage stage) const {
        switch (stage) {
            case Stage::INITIALIZATION: return "Initialization";
            case Stage::DATA_PROCESSING: return "Data Processing";
            case Stage::VALIDATION: return "Validation";
            case Stage::OUTPUT_GENERATION: return "Output Generation";
            case Stage::CLEANUP: return "Cleanup";
            case Stage::COMPLETED: return "Completed";
            default: return "Unknown";
        }
    }

    WorkflowCoordinator::Stage WorkflowCoordinator::next_stage(Stage current) const {
        switch (current) {
            case Stage::INITIALIZATION: return Stage::DATA_PROCESSING;
            case Stage::DATA_PROCESSING: return Stage::VALIDATION;
            case Stage::VALIDATION: return Stage::OUTPUT_GENERATION;
            case Stage::OUTPUT_GENERATION: return Stage::CLEANUP;
            case Stage::CLEANUP: return Stage::COMPLETED;
            default: return Stage::COMPLETED;
        }
    }

    // ========== EventNotificationSystem Implementation ==========

    EventNotificationSystem::EventNotificationSystem() {}

    EventNotificationSystem::~EventNotificationSystem() {
        stop_system();
    }

    void EventNotificationSystem::subscribe(EventType type, const std::string& subscriber_id, EventHandler handler) {
        subscribers_[type][subscriber_id] = std::move(handler);
        std::cout << "📝 Subscriber '" << subscriber_id << "' subscribed to " 
                  << event_type_to_string(type) << " events\n";
    }

    void EventNotificationSystem::unsubscribe(EventType type, const std::string& subscriber_id) {
        auto type_it = subscribers_.find(type);
        if (type_it != subscribers_.end()) {
            type_it->second.erase(subscriber_id);
            std::cout << "❌ Subscriber '" << subscriber_id << "' unsubscribed from " 
                      << event_type_to_string(type) << " events\n";
        }
    }

    void EventNotificationSystem::publish_event(const Event& event) {
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            event_queue_.push(event);
            events_published_++;
        }
        event_available_.notify_one();
        
        std::cout << "📢 Published " << event_type_to_string(event.type) 
                  << " event from " << event.source << std::endl;
    }

    void EventNotificationSystem::start_system() {
        std::cout << "🚀 Starting Event Notification System\n";
        running_ = true;
        
        // Start event processor threads
        for (int i = 0; i < 3; ++i) {
            processor_threads_.emplace_back(&EventNotificationSystem::event_processor, this, i);
        }
    }

    void EventNotificationSystem::stop_system() {
        running_ = false;
        event_available_.notify_all();
        
        for (auto& thread : processor_threads_) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        
        processor_threads_.clear();
        std::cout << "🛑 Event Notification System stopped\n";
    }

    void EventNotificationSystem::print_statistics() const {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        std::cout << "\n📊 === EVENT SYSTEM STATISTICS ===\n";
        std::cout << "Events Published: " << events_published_.load() << std::endl;
        std::cout << "Events Processed: " << events_processed_.load() << std::endl;
        std::cout << "================================\n\n";
    }

    void EventNotificationSystem::event_processor(int processor_id) {
        while (running_ || !event_queue_.empty()) {
            Event event;
            
            {
                std::unique_lock<std::mutex> lock(queue_mutex_);
                event_available_.wait(lock, [this] { 
                    return !running_ || !event_queue_.empty(); 
                });
                
                if (!running_ && event_queue_.empty()) break;
                
                if (!event_queue_.empty()) {
                    event = event_queue_.front();
                    event_queue_.pop();
                }
            }
            
            // Process event
            auto type_subscribers = subscribers_.find(event.type);
            if (type_subscribers != subscribers_.end()) {
                for (const auto& [subscriber_id, handler] : type_subscribers->second) {
                    try {
                        handler(event);
                        std::cout << "📨 Processor " << processor_id 
                                  << " delivered event to " << subscriber_id << std::endl;
                    } catch (const std::exception& e) {
                        std::cout << "❌ Error delivering event to " << subscriber_id 
                                  << ": " << e.what() << std::endl;
                    }
                }
            }
            
            events_processed_++;
        }
    }

    std::string EventNotificationSystem::event_type_to_string(EventType type) const {
        switch (type) {
            case EventType::DATA_UPDATED: return "DATA_UPDATED";
            case EventType::USER_ACTION: return "USER_ACTION";
            case EventType::SYSTEM_ALERT: return "SYSTEM_ALERT";
            case EventType::TIMER_EXPIRED: return "TIMER_EXPIRED";
            case EventType::CUSTOM_EVENT: return "CUSTOM_EVENT";
            default: return "UNKNOWN";
        }
    }

    // ========== DatabaseConnectionPool Implementation ==========

    DatabaseConnectionPool::DatabaseConnectionPool(size_t pool_size, size_t max_wait_time_ms) 
        : pool_size_(pool_size), max_wait_time_(max_wait_time_ms) {
        
        connections_.reserve(pool_size);
        
        for (size_t i = 0; i < pool_size; ++i) {
            auto connection = std::make_shared<Connection>(
                static_cast<int>(i), 
                "connection://localhost:5432/db" + std::to_string(i)
            );
            connections_.push_back(connection);
            available_connections_.push(connection);
        }
        
        std::cout << "🗄️  Database Connection Pool initialized with " << pool_size << " connections\n";
    }

    DatabaseConnectionPool::~DatabaseConnectionPool() {
        stop_connection_monitor();
    }

    std::shared_ptr<DatabaseConnectionPool::Connection> 
    DatabaseConnectionPool::acquire_connection(const std::string& client_id) {
        std::unique_lock<std::mutex> lock(pool_mutex_);
        
        if (!connection_available_.wait_for(lock, max_wait_time_, 
                                           [this] { return !available_connections_.empty(); })) {
            failed_acquisitions_++;
            std::cout << "⏰ " << client_id << " timed out waiting for connection\n";
            return nullptr;
        }
        
        auto connection = available_connections_.front();
        available_connections_.pop();
        connection->in_use = true;
        connection->last_used = std::chrono::steady_clock::now();
        
        total_acquisitions_++;
        std::cout << "🔗 " << client_id << " acquired connection " << connection->id << std::endl;
        
        return connection;
    }

    void DatabaseConnectionPool::release_connection(std::shared_ptr<Connection> connection) {
        if (!connection) return;
        
        {
            std::lock_guard<std::mutex> lock(pool_mutex_);
            connection->in_use = false;
            connection->last_used = std::chrono::steady_clock::now();
            available_connections_.push(connection);
        }
        
        connection_available_.notify_one();
        std::cout << "🔓 Released connection " << connection->id << std::endl;
    }

    void DatabaseConnectionPool::simulate_query(std::shared_ptr<Connection> connection, const std::string& query) {
        if (!connection || !connection->in_use) return;
        
        std::cout << "📊 Executing query on connection " << connection->id << ": " << query << std::endl;
        
        // Simulate query execution time
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> delay(50, 300);
        std::this_thread::sleep_for(std::chrono::milliseconds(delay(gen)));
        
        connection->query_count++;
        std::cout << "✅ Query completed on connection " << connection->id 
                  << " (total queries: " << connection->query_count << ")\n";
    }

    void DatabaseConnectionPool::start_connection_monitor() {
        monitor_running_ = true;
        monitor_thread_ = std::thread(&DatabaseConnectionPool::connection_monitor, this);
    }

    void DatabaseConnectionPool::stop_connection_monitor() {
        monitor_running_ = false;
        if (monitor_thread_.joinable()) {
            monitor_thread_.join();
        }
    }

    void DatabaseConnectionPool::print_pool_status() const {
        std::lock_guard<std::mutex> lock(pool_mutex_);
        std::lock_guard<std::mutex> stats_lock(stats_mutex_);
        
        std::cout << "\n📊 === CONNECTION POOL STATUS ===\n";
        std::cout << "Total Connections: " << pool_size_ << std::endl;
        std::cout << "Available Connections: " << available_connections_.size() << std::endl;
        std::cout << "Connections In Use: " << (pool_size_ - available_connections_.size()) << std::endl;
        std::cout << "Total Acquisitions: " << total_acquisitions_.load() << std::endl;
        std::cout << "Failed Acquisitions: " << failed_acquisitions_.load() << std::endl;
        
        std::cout << "\nConnection Details:\n";
        for (const auto& conn : connections_) {
            auto time_since_use = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - conn->last_used
            );
            std::cout << "  Connection " << conn->id 
                      << " | Status: " << (conn->in_use ? "IN_USE" : "AVAILABLE")
                      << " | Queries: " << conn->query_count
                      << " | Last Used: " << time_since_use.count() << "s ago\n";
        }
        std::cout << "================================\n\n";
    }

    size_t DatabaseConnectionPool::available_connections() const {
        std::lock_guard<std::mutex> lock(pool_mutex_);
        return available_connections_.size();
    }

    void DatabaseConnectionPool::connection_monitor() {
        while (monitor_running_) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            cleanup_idle_connections();
        }
    }

    void DatabaseConnectionPool::cleanup_idle_connections() {
        // This would typically close idle connections, but for simulation we just print status
        std::lock_guard<std::mutex> lock(pool_mutex_);
        
        auto now = std::chrono::steady_clock::now();
        for (const auto& conn : connections_) {
            auto idle_time = std::chrono::duration_cast<std::chrono::seconds>(now - conn->last_used);
            if (!conn->in_use && idle_time.count() > 10) {
                std::cout << "🧹 Connection " << conn->id << " has been idle for " 
                          << idle_time.count() << " seconds\n";
            }
        }
    }

    // ========== ConditionVariableDemo Implementation ==========

    void ConditionVariableDemo::demonstrate_basic_condition_variables() {
        print_section_header("Basic Condition Variables");
        
        BasicConditionVariableDemo demo;
        demo.demonstrate_basic_wait_notify();
        demo.demonstrate_predicate_wait();
        demo.demonstrate_spurious_wakeup_handling();
        demo.demonstrate_timeout_operations();
        demo.demonstrate_notify_all_vs_notify_one();
        
        print_section_footer();
    }

    void ConditionVariableDemo::demonstrate_producer_consumer_buffer() {
        print_section_header("Producer-Consumer Buffer");
        
        ProducerConsumerBuffer<std::string> buffer(5);
        
        std::vector<std::thread> producers;
        std::vector<std::thread> consumers;
        
        // Start producers
        for (int i = 0; i < 2; ++i) {
            producers.emplace_back([&buffer, i]() {
                for (int j = 0; j < 8; ++j) {
                    std::string item = "Item-" + std::to_string(i) + "-" + std::to_string(j);
                    buffer.produce(item);
                    std::cout << "📦 Producer " << i << " produced: " << item 
                              << " (buffer size: " << buffer.size() << ")\n";
                    simulate_work(std::chrono::milliseconds(100 + i * 50));
                }
            });
        }
        
        // Start consumers
        for (int i = 0; i < 3; ++i) {
            consumers.emplace_back([&buffer, i]() {
                for (int j = 0; j < 5; ++j) {
                    auto item = buffer.consume();
                    std::cout << "📥 Consumer " << i << " consumed: " << item 
                              << " (buffer size: " << buffer.size() << ")\n";
                    simulate_work(std::chrono::milliseconds(150 + i * 30));
                }
            });
        }
        
        for (auto& p : producers) p.join();
        for (auto& c : consumers) c.join();
        
        std::cout << "Final buffer size: " << buffer.size() << std::endl;
        
        print_section_footer();
    }

    void ConditionVariableDemo::demonstrate_thread_barrier() {
        print_section_header("Thread Barrier");
        
        const size_t num_threads = 4;
        ThreadBarrier barrier(num_threads);
        
        std::vector<std::thread> threads;
        
        for (size_t i = 0; i < num_threads; ++i) {
            threads.emplace_back([&barrier, i]() {
                // Phase 1
                std::cout << "🏃 Thread " << i << " starting phase 1\n";
                simulate_work(std::chrono::milliseconds(100 + i * 50));
                std::cout << "⏸️  Thread " << i << " waiting at barrier (phase 1)\n";
                barrier.wait();
                
                // Phase 2
                std::cout << "🏃 Thread " << i << " starting phase 2\n";
                simulate_work(std::chrono::milliseconds(200 + i * 30));
                std::cout << "⏸️  Thread " << i << " waiting at barrier (phase 2)\n";
                barrier.wait();
                
                // Phase 3
                std::cout << "🏃 Thread " << i << " starting phase 3\n";
                simulate_work(std::chrono::milliseconds(150));
                std::cout << "✅ Thread " << i << " completed all phases\n";
            });
        }
        
        for (auto& t : threads) t.join();
        
        print_section_footer();
    }

    void ConditionVariableDemo::demonstrate_counting_semaphore() {
        print_section_header("Counting Semaphore");
        
        CountingSemaphore semaphore(3); // Allow 3 concurrent operations
        
        std::vector<std::thread> workers;
        
        for (int i = 0; i < 8; ++i) {
            workers.emplace_back([&semaphore, i]() {
                std::cout << "🔄 Worker " << i << " trying to acquire semaphore\n";
                semaphore.acquire();
                
                std::cout << "✅ Worker " << i << " acquired semaphore (available: " 
                          << semaphore.available_count() << ")\n";
                
                simulate_work(std::chrono::milliseconds(500));
                
                std::cout << "🔓 Worker " << i << " releasing semaphore\n";
                semaphore.release();
            });
        }
        
        for (auto& w : workers) w.join();
        
        std::cout << "Final semaphore count: " << semaphore.available_count() << std::endl;
        
        print_section_footer();
    }

    void ConditionVariableDemo::demonstrate_thread_pool() {
        print_section_header("Thread Pool");
        
        ThreadPool pool(4);
        
        std::vector<std::future<int>> futures;
        
        // Submit tasks to the pool
        for (int i = 0; i < 10; ++i) {
            auto future = pool.submit([i]() {
                std::cout << "🛠️  Task " << i << " executing on thread " 
                          << std::this_thread::get_id() << std::endl;
                simulate_work(std::chrono::milliseconds(200 + i * 50));
                std::cout << "✅ Task " << i << " completed\n";
                return i * i;
            });
            futures.push_back(std::move(future));
        }
        
        // Collect results
        std::cout << "\n📊 Task Results:\n";
        for (size_t i = 0; i < futures.size(); ++i) {
            int result = futures[i].get();
            std::cout << "Task " << i << " result: " << result << std::endl;
        }
        
        std::cout << "Active threads: " << pool.active_threads() << std::endl;
        std::cout << "Pending tasks: " << pool.pending_tasks() << std::endl;
        
        print_section_footer();
    }

    void ConditionVariableDemo::demonstrate_workflow_coordination() {
        print_section_header("Workflow Coordination");
        
        WorkflowCoordinator coordinator;
        
        // Start workflow in background
        std::thread workflow_thread([&coordinator]() {
            coordinator.start_workflow(std::chrono::seconds(5));
        });
        
        // Submit tasks
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        for (int i = 1; i <= 6; ++i) {
            coordinator.submit_task("Task " + std::to_string(i), 
                                  "Data for task " + std::to_string(i));
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }
        
        workflow_thread.join();
        coordinator.print_statistics();
        
        print_section_footer();
    }

    void ConditionVariableDemo::demonstrate_event_notification_system() {
        print_section_header("Event Notification System");
        
        EventNotificationSystem event_system;
        event_system.start_system();
        
        // Register subscribers
        event_system.subscribe(EventNotificationSystem::EventType::DATA_UPDATED, "Logger", 
            [](const auto& event) {
                std::cout << "📝 Logger: Data update from " << event.source 
                          << " - " << event.message << std::endl;
            });
        
        event_system.subscribe(EventNotificationSystem::EventType::USER_ACTION, "Analytics", 
            [](const auto& event) {
                std::cout << "📈 Analytics: User action - " << event.message << std::endl;
            });
        
        event_system.subscribe(EventNotificationSystem::EventType::SYSTEM_ALERT, "Monitor", 
            [](const auto& event) {
                std::cout << "🚨 Monitor: System alert from " << event.source 
                          << " - " << event.message << std::endl;
            });
        
        // Publish some events
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        std::vector<std::thread> event_publishers;
        
        event_publishers.emplace_back([&event_system]() {
            for (int i = 0; i < 3; ++i) {
                EventNotificationSystem::Event event{
                    EventNotificationSystem::EventType::DATA_UPDATED,
                    "Database",
                    "Record " + std::to_string(i) + " updated",
                    std::chrono::steady_clock::now()
                };
                event_system.publish_event(event);
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }
        });
        
        event_publishers.emplace_back([&event_system]() {
            for (int i = 0; i < 2; ++i) {
                EventNotificationSystem::Event event{
                    EventNotificationSystem::EventType::USER_ACTION,
                    "WebApp",
                    "User clicked button " + std::to_string(i),
                    std::chrono::steady_clock::now()
                };
                event_system.publish_event(event);
                std::this_thread::sleep_for(std::chrono::milliseconds(300));
            }
        });
        
        for (auto& publisher : event_publishers) {
            publisher.join();
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        event_system.stop_system();
        event_system.print_statistics();
        
        print_section_footer();
    }

    void ConditionVariableDemo::demonstrate_database_connection_pool() {
        print_section_header("Database Connection Pool");
        
        DatabaseConnectionPool pool(3, 2000); // 3 connections, 2 second timeout
        pool.start_connection_monitor();
        
        std::vector<std::thread> clients;
        
        // Start multiple clients
        for (int i = 0; i < 6; ++i) {
            clients.emplace_back([&pool, i]() {
                std::string client_id = "Client-" + std::to_string(i);
                
                for (int j = 0; j < 2; ++j) {
                    auto connection = pool.acquire_connection(client_id);
                    
                    if (connection) {
                        pool.simulate_query(connection, "SELECT * FROM table" + std::to_string(j));
                        simulate_work(std::chrono::milliseconds(300 + i * 100));
                        pool.release_connection(connection);
                    }
                    
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            });
        }
        
        // Monitor pool status
        std::thread status_monitor([&pool]() {
            for (int i = 0; i < 4; ++i) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                pool.print_pool_status();
            }
        });
        
        for (auto& client : clients) {
            client.join();
        }
        
        status_monitor.join();
        pool.stop_connection_monitor();
        
        print_section_footer();
    }

    void ConditionVariableDemo::run_all_demonstrations() {
        std::cout << "\n🎯 ========================================================\n";
        std::cout << "🎯 COMPREHENSIVE CONDITION VARIABLE DEMONSTRATIONS\n";
        std::cout << "🎯 ========================================================\n\n";
        
        demonstrate_basic_condition_variables();
        demonstrate_producer_consumer_buffer();
        demonstrate_thread_barrier();
        demonstrate_counting_semaphore();
        demonstrate_thread_pool();
        demonstrate_workflow_coordination();
        demonstrate_event_notification_system();
        demonstrate_database_connection_pool();
        
        std::cout << "\n🎉 ========================================\n";
        std::cout << "🎉 ALL CONDITION VARIABLE DEMONSTRATIONS COMPLETED!\n";
        std::cout << "🎉 ========================================\n\n";
    }

    void ConditionVariableDemo::print_section_header(const std::string& title) {
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "🔄 " << title << std::endl;
        std::cout << std::string(60, '=') << std::endl << std::endl;
    }

    void ConditionVariableDemo::print_section_footer() {
        std::cout << std::string(60, '-') << std::endl;
        std::cout << "✅ Section Complete\n" << std::endl;
    }

    void ConditionVariableDemo::simulate_work(std::chrono::milliseconds duration) {
        std::this_thread::sleep_for(duration);
    }

} // namespace CppVerseHub::Concurrency