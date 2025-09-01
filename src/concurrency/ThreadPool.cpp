/**
 * @file ThreadPool.cpp
 * @brief Implementation of thread pool classes and demonstrations
 * @details File location: src/concurrency/ThreadPool.cpp
 */

#include "ThreadPool.hpp"
#include <unordered_map>
#include <sstream>
#include <iomanip>

namespace CppVerseHub::Concurrency {

    // BasicThreadPool Implementation
    BasicThreadPool::BasicThreadPool(size_t num_threads) {
        threads_.reserve(num_threads);
        for (size_t i = 0; i < num_threads; ++i) {
            threads_.emplace_back(&BasicThreadPool::worker_thread, this);
        }
        std::cout << "BasicThreadPool: Created with " << num_threads << " threads\n";
    }

    BasicThreadPool::~BasicThreadPool() {
        shutdown();
    }

    void BasicThreadPool::worker_thread() {
        while (true) {
            std::function<void()> task;
            
            {
                std::unique_lock<std::mutex> lock(queue_mutex_);
                cv_.wait(lock, [this] { return shutdown_.load() || !tasks_.empty(); });

                if (shutdown_.load() && tasks_.empty()) {
                    return;
                }

                task = std::move(tasks_.front());
                tasks_.pop();
            }

            try {
                task();
            } catch (const std::exception& e) {
                std::cout << "BasicThreadPool: Task exception - " << e.what() << "\n";
            }
        }
    }

    size_t BasicThreadPool::pending_tasks() const {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        return tasks_.size();
    }

    void BasicThreadPool::shutdown() {
        if (!shutdown_.exchange(true)) {
            cv_.notify_all();
            for (auto& thread : threads_) {
                if (thread.joinable()) {
                    thread.join();
                }
            }
            std::cout << "BasicThreadPool: Shutdown complete\n";
        }
    }

    // PriorityThreadPool Implementation
    PriorityThreadPool::PriorityThreadPool(size_t num_threads) {
        threads_.reserve(num_threads);
        for (size_t i = 0; i < num_threads; ++i) {
            threads_.emplace_back(&PriorityThreadPool::worker_thread, this);
        }
        std::cout << "PriorityThreadPool: Created with " << num_threads << " threads\n";
    }

    PriorityThreadPool::~PriorityThreadPool() {
        shutdown();
    }

    void PriorityThreadPool::worker_thread() {
        while (true) {
            Task task({}, Priority::LOW, 0);
            
            {
                std::unique_lock<std::mutex> lock(queue_mutex_);
                cv_.wait(lock, [this] { return shutdown_.load() || !task_queue_.empty(); });

                if (shutdown_.load() && task_queue_.empty()) {
                    return;
                }

                task = std::move(const_cast<Task&>(task_queue_.top()));
                task_queue_.pop();
            }

            try {
                task.function();
            } catch (const std::exception& e) {
                std::cout << "PriorityThreadPool: Task " << task.id << " exception - " << e.what() << "\n";
            }
        }
    }

    size_t PriorityThreadPool::pending_tasks() const {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        return task_queue_.size();
    }

    void PriorityThreadPool::shutdown() {
        if (!shutdown_.exchange(true)) {
            cv_.notify_all();
            for (auto& thread : threads_) {
                if (thread.joinable()) {
                    thread.join();
                }
            }
            std::cout << "PriorityThreadPool: Shutdown complete\n";
        }
    }

    // WorkStealingThreadPool Implementation
    WorkStealingThreadPool::WorkStealingThreadPool(size_t num_threads) {
        queues_.reserve(num_threads);
        threads_.reserve(num_threads);

        for (size_t i = 0; i < num_threads; ++i) {
            queues_.emplace_back(std::make_unique<WorkerQueue>());
        }

        for (size_t i = 0; i < num_threads; ++i) {
            threads_.emplace_back(&WorkStealingThreadPool::worker_thread, this, i);
        }

        std::cout << "WorkStealingThreadPool: Created with " << num_threads << " threads and queues\n";
    }

    WorkStealingThreadPool::~WorkStealingThreadPool() {
        shutdown();
    }

    void WorkStealingThreadPool::worker_thread(size_t thread_id) {
        auto& my_queue = *queues_[thread_id];
        
        while (!shutdown_.load()) {
            std::function<void()> task;
            bool found_task = false;

            // Try to get work from own queue
            {
                std::lock_guard<std::mutex> lock(my_queue.mutex);
                if (!my_queue.tasks.empty()) {
                    task = std::move(my_queue.tasks.front());
                    my_queue.tasks.pop_front();
                    found_task = true;
                }
            }

            // If no work in own queue, try to steal
            if (!found_task) {
                found_task = try_steal_work(thread_id);
                if (found_task) {
                    // try_steal_work updates task if successful
                    {
                        std::lock_guard<std::mutex> lock(my_queue.mutex);
                        if (!my_queue.tasks.empty()) {
                            task = std::move(my_queue.tasks.front());
                            my_queue.tasks.pop_front();
                        } else {
                            found_task = false;
                        }
                    }
                }
            }

            if (found_task) {
                try {
                    task();
                } catch (const std::exception& e) {
                    std::cout << "WorkStealingThreadPool: Thread " << thread_id << " task exception - " << e.what() << "\n";
                }
            } else {
                // No work available, sleep briefly
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    }

    bool WorkStealingThreadPool::try_steal_work(size_t my_id) {
        // Try to steal from other queues
        for (size_t i = 0; i < queues_.size(); ++i) {
            if (i == my_id) continue;

            auto& other_queue = *queues_[i];
            std::lock_guard<std::mutex> lock(other_queue.mutex);
            
            if (other_queue.tasks.size() > 1) { // Only steal if queue has multiple tasks
                auto stolen_task = std::move(other_queue.tasks.back());
                other_queue.tasks.pop_back();
                
                // Add stolen task to my queue
                auto& my_queue = *queues_[my_id];
                std::lock_guard<std::mutex> my_lock(my_queue.mutex);
                my_queue.tasks.push_back(std::move(stolen_task));
                return true;
            }
        }
        return false;
    }

    void WorkStealingThreadPool::push_task_to_queue(size_t queue_id, std::function<void()> task) {
        auto& queue = *queues_[queue_id];
        std::lock_guard<std::mutex> lock(queue.mutex);
        queue.tasks.push_back(std::move(task));
    }

    size_t WorkStealingThreadPool::total_pending_tasks() const {
        size_t total = 0;
        for (const auto& queue : queues_) {
            std::lock_guard<std::mutex> lock(queue->mutex);
            total += queue->tasks.size();
        }
        return total;
    }

    void WorkStealingThreadPool::print_queue_status() const {
        std::cout << "WorkStealingThreadPool queue status:\n";
        for (size_t i = 0; i < queues_.size(); ++i) {
            std::lock_guard<std::mutex> lock(queues_[i]->mutex);
            std::cout << "  Queue " << i << ": " << queues_[i]->tasks.size() << " tasks\n";
        }
    }

    void WorkStealingThreadPool::shutdown() {
        if (!shutdown_.exchange(true)) {
            for (auto& thread : threads_) {
                if (thread.joinable()) {
                    thread.join();
                }
            }
            std::cout << "WorkStealingThreadPool: Shutdown complete\n";
        }
    }

    // ThreadPoolManager Implementation
    ThreadPoolManager& ThreadPoolManager::instance() {
        static ThreadPoolManager instance;
        return instance;
    }

    void ThreadPoolManager::create_pool(PoolType type, const std::string& name, size_t num_threads) {
        std::lock_guard<std::mutex> lock(pools_mutex_);
        
        if (pools_.find(name) != pools_.end()) {
            std::cout << "ThreadPoolManager: Pool '" << name << "' already exists\n";
            return;
        }

        PoolInfo info;
        info.type = type;

        switch (type) {
            case PoolType::BASIC:
                info.basic_pool = std::make_unique<BasicThreadPool>(num_threads);
                break;
            case PoolType::PRIORITY:
                info.priority_pool = std::make_unique<PriorityThreadPool>(num_threads);
                break;
            case PoolType::WORK_STEALING:
                info.work_stealing_pool = std::make_unique<WorkStealingThreadPool>(num_threads);
                break;
        }

        pools_[name] = std::move(info);
        std::cout << "ThreadPoolManager: Created " << 
                     (type == PoolType::BASIC ? "Basic" : 
                      type == PoolType::PRIORITY ? "Priority" : "WorkStealing") 
                  << " pool '" << name << "'\n";
    }

    void ThreadPoolManager::shutdown_pool(const std::string& name) {
        std::lock_guard<std::mutex> lock(pools_mutex_);
        auto it = pools_.find(name);
        if (it != pools_.end()) {
            std::cout << "ThreadPoolManager: Shutting down pool '" << name << "'\n";
            pools_.erase(it);
        }
    }

    void ThreadPoolManager::shutdown_all() {
        std::lock_guard<std::mutex> lock(pools_mutex_);
        pools_.clear();
        std::cout << "ThreadPoolManager: All pools shut down\n";
    }

    BasicThreadPool* ThreadPoolManager::get_basic_pool(const std::string& name) {
        std::lock_guard<std::mutex> lock(pools_mutex_);
        auto it = pools_.find(name);
        return (it != pools_.end() && it->second.type == PoolType::BASIC) ? it->second.basic_pool.get() : nullptr;
    }

    PriorityThreadPool* ThreadPoolManager::get_priority_pool(const std::string& name) {
        std::lock_guard<std::mutex> lock(pools_mutex_);
        auto it = pools_.find(name);
        return (it != pools_.end() && it->second.type == PoolType::PRIORITY) ? it->second.priority_pool.get() : nullptr;
    }

    WorkStealingThreadPool* ThreadPoolManager::get_work_stealing_pool(const std::string& name) {
        std::lock_guard<std::mutex> lock(pools_mutex_);
        auto it = pools_.find(name);
        return (it != pools_.end() && it->second.type == PoolType::WORK_STEALING) ? it->second.work_stealing_pool.get() : nullptr;
    }

    void ThreadPoolManager::print_pool_statistics() const {
        std::lock_guard<std::mutex> lock(pools_mutex_);
        std::cout << "\n=== ThreadPool Manager Statistics ===\n";
        std::cout << "Total pools: " << pools_.size() << "\n";
        
        for (const auto& [name, info] : pools_) {
            std::cout << "Pool '" << name << "' - Type: ";
            switch (info.type) {
                case PoolType::BASIC:
                    std::cout << "Basic, Pending: " << info.basic_pool->pending_tasks();
                    break;
                case PoolType::PRIORITY:
                    std::cout << "Priority, Pending: " << info.priority_pool->pending_tasks();
                    break;
                case PoolType::WORK_STEALING:
                    std::cout << "WorkStealing, Pending: " << info.work_stealing_pool->total_pending_tasks();
                    break;
            }
            std::cout << "\n";
        }
    }

    // ThreadPoolDemo Implementation
    void ThreadPoolDemo::demonstrate_basic_pool() {
        std::cout << "\n=== Basic ThreadPool Demonstration ===\n";
        
        BasicThreadPool pool(4);
        std::vector<std::future<int>> futures;
        
        // Submit various tasks
        for (int i = 0; i < 10; ++i) {
            futures.push_back(pool.submit([i]() {
                simulate_cpu_work(std::chrono::milliseconds(100));
                std::cout << "Basic pool task " << i << " completed by thread " 
                          << std::this_thread::get_id() << "\n";
                return i * i;
            }));
        }
        
        // Collect results
        std::cout << "Basic pool results: ";
        for (auto& future : futures) {
            std::cout << future.get() << " ";
        }
        std::cout << "\n";
        
        std::cout << "Basic pool pending tasks: " << pool.pending_tasks() << "\n";
    }

    void ThreadPoolDemo::demonstrate_priority_pool() {
        std::cout << "\n=== Priority ThreadPool Demonstration ===\n";
        
        PriorityThreadPool pool(3);
        std::vector<std::future<std::string>> futures;
        
        // Submit tasks with different priorities
        std::vector<std::pair<PriorityThreadPool::Priority, std::string>> tasks = {
            {PriorityThreadPool::Priority::LOW, "Low Priority Task 1"},
            {PriorityThreadPool::Priority::HIGH, "High Priority Task 1"},
            {PriorityThreadPool::Priority::NORMAL, "Normal Priority Task 1"},
            {PriorityThreadPool::Priority::CRITICAL, "Critical Priority Task"},
            {PriorityThreadPool::Priority::LOW, "Low Priority Task 2"},
            {PriorityThreadPool::Priority::HIGH, "High Priority Task 2"}
        };

        for (const auto& [priority, name] : tasks) {
            futures.push_back(pool.submit(priority, [name]() {
                simulate_cpu_work(std::chrono::milliseconds(150));
                std::string result = name + " completed by thread " + 
                                   std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id()));
                std::cout << result << "\n";
                return result;
            }));
        }
        
        // Wait for all tasks
        for (auto& future : futures) {
            future.wait();
        }
        
        std::cout << "Priority pool pending tasks: " << pool.pending_tasks() << "\n";
    }

    void ThreadPoolDemo::demonstrate_work_stealing() {
        std::cout << "\n=== Work Stealing ThreadPool Demonstration ===\n";
        
        WorkStealingThreadPool pool(4);
        std::vector<std::future<void>> futures;
        
        // Submit uneven workloads to demonstrate work stealing
        for (int i = 0; i < 20; ++i) {
            futures.push_back(pool.submit([i]() {
                // Vary work duration to create imbalance
                auto duration = std::chrono::milliseconds(50 + (i % 5) * 30);
                simulate_cpu_work(duration);
                std::cout << "Work stealing task " << i << " (duration: " << duration.count() 
                          << "ms) completed by thread " << std::this_thread::get_id() << "\n";
            }));
        }
        
        // Print queue status periodically
        std::thread status_thread([&pool]() {
            for (int i = 0; i < 5; ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                pool.print_queue_status();
            }
        });
        
        // Wait for all tasks
        for (auto& future : futures) {
            future.wait();
        }
        
        status_thread.join();
        std::cout << "Work stealing final pending tasks: " << pool.total_pending_tasks() << "\n";
    }

    void ThreadPoolDemo::demonstrate_performance_comparison() {
        std::cout << "\n=== ThreadPool Performance Comparison ===\n";
        
        const size_t num_tasks = 100;
        const auto task_duration = std::chrono::milliseconds(10);
        
        // Test Basic ThreadPool
        {
            auto start = std::chrono::high_resolution_clock::now();
            BasicThreadPool pool(4);
            std::vector<std::future<void>> futures;
            
            for (size_t i = 0; i < num_tasks; ++i) {
                futures.push_back(pool.submit([task_duration]() {
                    simulate_cpu_work(task_duration);
                }));
            }
            
            for (auto& future : futures) {
                future.wait();
            }
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            std::cout << "Basic ThreadPool: " << duration.count() << "ms for " << num_tasks << " tasks\n";
        }
        
        // Test Work Stealing ThreadPool
        {
            auto start = std::chrono::high_resolution_clock::now();
            WorkStealingThreadPool pool(4);
            std::vector<std::future<void>> futures;
            
            for (size_t i = 0; i < num_tasks; ++i) {
                futures.push_back(pool.submit([task_duration]() {
                    simulate_cpu_work(task_duration);
                }));
            }
            
            for (auto& future : futures) {
                future.wait();
            }
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            std::cout << "Work Stealing ThreadPool: " << duration.count() << "ms for " << num_tasks << " tasks\n";
        }
    }

    void ThreadPoolDemo::demonstrate_space_mission_simulation() {
        std::cout << "\n=== Space Mission Simulation ===\n";
        
        auto missions = generate_mission_tasks();
        PriorityThreadPool mission_control(6);
        std::vector<std::future<void>> mission_futures;
        
        std::cout << "Launching " << missions.size() << " space missions...\n";
        
        for (const auto& mission : missions) {
            mission_futures.push_back(mission_control.submit(mission.priority, [mission]() {
                std::cout << "Starting mission: " << mission.mission_name << " (Priority: ";
                switch (mission.priority) {
                    case PriorityThreadPool::Priority::CRITICAL: std::cout << "CRITICAL"; break;
                    case PriorityThreadPool::Priority::HIGH: std::cout << "HIGH"; break;
                    case PriorityThreadPool::Priority::NORMAL: std::cout << "NORMAL"; break;
                    case PriorityThreadPool::Priority::LOW: std::cout << "LOW"; break;
                }
                std::cout << ")\n";
                
                simulate_cpu_work(mission.duration);
                
                std::cout << "Mission completed: " << mission.mission_name << " after " 
                          << mission.duration.count() << "ms\n";
            }));
        }
        
        // Monitor mission progress
        std::thread monitor([&mission_control]() {
            while (mission_control.pending_tasks() > 0) {
                std::cout << "Mission Control: " << mission_control.pending_tasks() << " missions in queue\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        });
        
        // Wait for all missions
        for (auto& future : mission_futures) {
            future.wait();
        }
        
        monitor.join();
        std::cout << "All space missions completed successfully!\n";
    }

    void ThreadPoolDemo::run_all_demonstrations() {
        std::cout << "\n========== THREADPOOL COMPREHENSIVE DEMONSTRATION ==========\n";
        
        demonstrate_basic_pool();
        demonstrate_priority_pool();
        demonstrate_work_stealing();
        demonstrate_performance_comparison();
        demonstrate_space_mission_simulation();
        
        // Demonstrate ThreadPoolManager
        std::cout << "\n=== ThreadPool Manager Demonstration ===\n";
        auto& manager = ThreadPoolManager::instance();
        
        manager.create_pool(ThreadPoolManager::PoolType::BASIC, "main_pool", 4);
        manager.create_pool(ThreadPoolManager::PoolType::PRIORITY, "priority_pool", 3);
        manager.create_pool(ThreadPoolManager::PoolType::WORK_STEALING, "stealing_pool", 5);
        
        manager.print_pool_statistics();
        
        // Use managed pools
        if (auto* basic_pool = manager.get_basic_pool("main_pool")) {
            auto future = basic_pool->submit([]() {
                std::cout << "Task executed on managed basic pool\n";
                return 42;
            });
            std::cout << "Managed pool result: " << future.get() << "\n";
        }
        
        manager.shutdown_all();
        
        std::cout << "\n========== THREADPOOL DEMONSTRATION COMPLETE ==========\n";
    }

    std::vector<ThreadPoolDemo::SpaceMissionTask> ThreadPoolDemo::generate_mission_tasks() {
        return {
            {"Emergency Rescue Mission", std::chrono::milliseconds(200), PriorityThreadPool::Priority::CRITICAL},
            {"Satellite Deployment", std::chrono::milliseconds(800), PriorityThreadPool::Priority::HIGH},
            {"Routine Maintenance", std::chrono::milliseconds(400), PriorityThreadPool::Priority::LOW},
            {"Scientific Survey", std::chrono::milliseconds(600), PriorityThreadPool::Priority::NORMAL},
            {"Deep Space Probe", std::chrono::milliseconds(1000), PriorityThreadPool::Priority::NORMAL},
            {"Station Resupply", std::chrono::milliseconds(300), PriorityThreadPool::Priority::HIGH},
            {"Asteroid Mining", std::chrono::milliseconds(1200), PriorityThreadPool::Priority::LOW},
            {"Communications Relay", std::chrono::milliseconds(250), PriorityThreadPool::Priority::HIGH},
            {"Solar Panel Inspection", std::chrono::milliseconds(150), PriorityThreadPool::Priority::LOW},
            {"Navigation Calibration", std::chrono::milliseconds(350), PriorityThreadPool::Priority::NORMAL}
        };
    }

    void ThreadPoolDemo::simulate_cpu_work(std::chrono::milliseconds duration) {
        auto start = std::chrono::high_resolution_clock::now();
        while (std::chrono::high_resolution_clock::now() - start < duration) {
            // Simulate CPU-intensive work
            volatile int dummy = 0;
            for (int i = 0; i < 1000; ++i) {
                dummy += i;
            }
        }
    }

} // namespace CppVerseHub::Concurrency