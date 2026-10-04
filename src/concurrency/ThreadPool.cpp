/**
 * @file ThreadPool.cpp
 * @brief Work-stealing pool implementation and the thread-pool showcase.
 * @details File location: src/concurrency/ThreadPool.cpp
 */

#include "concurrency/ThreadPool.hpp"

#include <numeric>
#include <stdexcept>
#include <string>

namespace CppVerseHub::Concurrency {

    namespace {
        // Identifies the pool/worker that owns the current thread, so that tasks spawned
        // from inside a task land on the spawning worker's own deque.
        thread_local const WorkStealingThreadPool* tls_pool = nullptr;
        thread_local std::size_t tls_index = 0;
    }  // namespace

    WorkStealingThreadPool::WorkStealingThreadPool(std::size_t thread_count) {
        const std::size_t n = std::max<std::size_t>(1, thread_count);
        queues_.reserve(n);
        for (std::size_t i = 0; i < n; ++i) {
            queues_.push_back(std::make_unique<WorkerQueue>());
        }
        workers_.reserve(n);
        try {
            for (std::size_t i = 0; i < n; ++i) {
                workers_.emplace_back([this, i] { worker_loop(i); });
            }
        } catch (...) {
            shutdown();
            throw;
        }
    }

    WorkStealingThreadPool::~WorkStealingThreadPool() { shutdown(); }

    void WorkStealingThreadPool::enqueue(UniqueTask task) {
        {
            std::lock_guard lock(sleep_mutex_);
            if (stopping_) {
                throw PoolShutdownError{};
            }
            pending_.fetch_add(1);
        }
        const std::size_t target =
            (tls_pool == this) ? tls_index : next_queue_.fetch_add(1, std::memory_order_relaxed) % queues_.size();
        {
            WorkerQueue& queue = *queues_[target];
            std::lock_guard lock(queue.mutex);
            queue.tasks.push_back(std::move(task));
        }
        // Lock/unlock before notifying so a worker that has just evaluated its wait predicate
        // cannot miss this notification (classic lost wake-up).
        { std::lock_guard lock(sleep_mutex_); }
        sleep_cv_.notify_one();
    }

    UniqueTask WorkStealingThreadPool::try_acquire(std::size_t index) {
        {
            WorkerQueue& own = *queues_[index];
            std::lock_guard lock(own.mutex);
            if (!own.tasks.empty()) {
                UniqueTask task = std::move(own.tasks.back());
                own.tasks.pop_back();
                pending_.fetch_sub(1);
                return task;
            }
        }
        const std::size_t n = queues_.size();
        for (std::size_t offset = 1; offset < n; ++offset) {
            WorkerQueue& victim = *queues_[(index + offset) % n];
            std::lock_guard lock(victim.mutex);
            if (!victim.tasks.empty()) {
                UniqueTask task = std::move(victim.tasks.front());
                victim.tasks.pop_front();
                pending_.fetch_sub(1);
                steals_.fetch_add(1, std::memory_order_relaxed);
                return task;
            }
        }
        return {};
    }

    void WorkStealingThreadPool::worker_loop(std::size_t index) {
        tls_pool = this;
        tls_index = index;
        for (;;) {
            if (UniqueTask task = try_acquire(index)) {
                try {
                    task();
                } catch (...) {
                    // submit() tasks never throw; see PoolCore::worker_loop.
                }
                completed_.fetch_add(1, std::memory_order_relaxed);
                continue;
            }
            std::unique_lock lock(sleep_mutex_);
            if (pending_.load() > 0) {
                // A task is counted but its push into a deque is still in flight.
                lock.unlock();
                std::this_thread::yield();
                continue;
            }
            if (stopping_) {
                break;
            }
            sleep_cv_.wait(lock, [this] { return stopping_ || pending_.load() > 0; });
        }
        tls_pool = nullptr;
    }

    void WorkStealingThreadPool::shutdown() {
        {
            std::lock_guard lock(sleep_mutex_);
            stopping_ = true;
        }
        sleep_cv_.notify_all();
        std::lock_guard join_lock(join_mutex_);
        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    namespace {

        // Sums data[lo, hi); used as the leaf work item of the work-stealing demo.
        std::uint64_t fanout_sum(const std::vector<std::uint64_t>& data, std::size_t lo, std::size_t hi) {
            std::uint64_t sum = 0;
            for (std::size_t i = lo; i < hi; ++i) {
                sum += data[i];
            }
            return sum;
        }

    }  // namespace

    void demonstrate_thread_pools(std::ostream& out) {
        out << "=== Thread pools ===\n";

        {
            ThreadPool pool(4);
            std::vector<std::future<int>> squares;
            for (int i = 1; i <= 8; ++i) {
                squares.push_back(pool.submit([](int x) { return x * x; }, i));
            }
            int total = 0;
            for (auto& f : squares) {
                total += f.get();
            }
            out << "FIFO pool (" << pool.thread_count() << " workers): sum of squares 1..8 = " << total << '\n';

            auto failing = pool.submit([]() -> int { throw std::runtime_error("sensor offline"); });
            try {
                static_cast<void>(failing.get());
            } catch (const std::exception& e) {
                out << "Exception propagated through future: " << e.what() << '\n';
            }

            std::atomic<int> counter{0};
            for (int i = 0; i < 100; ++i) {
                pool.post(UniqueTask([&counter] { counter.fetch_add(1, std::memory_order_relaxed); }));
            }
            pool.shutdown();  // graceful: all 100 posted tasks still run
            out << "Graceful shutdown executed " << counter.load() << "/100 queued tasks\n";
            try {
                static_cast<void>(pool.submit([] { return 0; }));
            } catch (const PoolShutdownError&) {
                out << "Submission after shutdown rejected with PoolShutdownError\n";
            }
        }

        {
            PriorityThreadPool pool(1);
            std::promise<void> gate;
            std::shared_future<void> gate_future = gate.get_future().share();
            auto blocker = pool.submit(TaskPriority::Critical, [gate_future] { gate_future.wait(); });

            std::mutex order_mutex;
            std::vector<std::string> order;
            auto record = [&](std::string name) {
                std::lock_guard lock(order_mutex);
                order.push_back(std::move(name));
            };
            std::vector<std::future<void>> futures;
            futures.push_back(pool.submit(TaskPriority::Low, record, std::string("telemetry-archive")));
            futures.push_back(pool.submit(TaskPriority::Critical, record, std::string("collision-avoidance")));
            futures.push_back(pool.submit(TaskPriority::Normal, record, std::string("course-correction")));
            futures.push_back(pool.submit(TaskPriority::High, record, std::string("life-support-check")));
            gate.set_value();
            blocker.get();
            for (auto& f : futures) {
                f.get();
            }
            out << "Priority pool execution order:";
            for (const auto& name : order) {
                out << ' ' << name;
            }
            out << '\n';
        }

        {
            WorkStealingThreadPool pool(4);
            std::vector<std::uint64_t> data(100000);
            std::iota(data.begin(), data.end(), std::uint64_t{1});
            constexpr std::size_t chunk = 5000;
            std::vector<std::future<std::uint64_t>> parts;
            for (std::size_t lo = 0; lo < data.size(); lo += chunk) {
                parts.push_back(pool.submit(fanout_sum, std::cref(data), lo, std::min(lo + chunk, data.size())));
            }
            std::uint64_t total = 0;
            for (auto& f : parts) {
                total += f.get();
            }
            out << "Work-stealing pool: sum 1..100000 = " << total << " computed in " << parts.size()
                << " chunks\n";
        }
        out << '\n';
    }

}  // namespace CppVerseHub::Concurrency
