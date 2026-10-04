/**
 * @file ConditionalVariables.cpp
 * @brief Implementation of the condition-variable based primitives and their showcase.
 * @details File location: src/concurrency/ConditionalVariables.cpp
 */

#include "concurrency/ConditionalVariables.hpp"

#include <algorithm>
#include <atomic>
#include <string>
#include <thread>

namespace CppVerseHub::Concurrency {

// ---------------------------------------------------------------- CountingSemaphore

void CountingSemaphore::acquire() {
    std::unique_lock lock(mutex_);
    cv_.wait(lock, [this] { return count_ > 0; });
    --count_;
}

bool CountingSemaphore::try_acquire() {
    std::lock_guard lock(mutex_);
    if (count_ == 0) {
        return false;
    }
    --count_;
    return true;
}

bool CountingSemaphore::try_acquire_for(std::chrono::nanoseconds timeout) {
    std::unique_lock lock(mutex_);
    if (!cv_.wait_for(lock, timeout, [this] { return count_ > 0; })) {
        return false;
    }
    --count_;
    return true;
}

void CountingSemaphore::release(std::size_t permits) {
    {
        std::lock_guard lock(mutex_);
        count_ += permits;
    }
    if (permits == 1) {
        cv_.notify_one();
    } else if (permits > 1) {
        cv_.notify_all();
    }
}

std::size_t CountingSemaphore::available() const {
    std::lock_guard lock(mutex_);
    return count_;
}

// ---------------------------------------------------------------- CountDownLatch

void CountDownLatch::count_down(std::size_t n) {
    bool opened = false;
    {
        std::lock_guard lock(mutex_);
        if (count_ == 0) {
            return;
        }
        count_ -= std::min(n, count_);
        opened = (count_ == 0);
    }
    if (opened) {
        cv_.notify_all();
    }
}

void CountDownLatch::wait() const {
    std::unique_lock lock(mutex_);
    cv_.wait(lock, [this] { return count_ == 0; });
}

bool CountDownLatch::wait_for(std::chrono::nanoseconds timeout) const {
    std::unique_lock lock(mutex_);
    return cv_.wait_for(lock, timeout, [this] { return count_ == 0; });
}

bool CountDownLatch::try_wait() const {
    std::lock_guard lock(mutex_);
    return count_ == 0;
}

void CountDownLatch::arrive_and_wait() {
    count_down();
    wait();
}

std::size_t CountDownLatch::count() const {
    std::lock_guard lock(mutex_);
    return count_;
}

// ---------------------------------------------------------------- CyclicBarrier

CyclicBarrier::CyclicBarrier(std::size_t parties, std::function<void()> on_completion)
    : parties_(parties), on_completion_(std::move(on_completion)) {
    if (parties_ == 0) {
        throw std::invalid_argument("CyclicBarrier requires at least one party");
    }
}

std::size_t CyclicBarrier::arrive_and_wait() {
    std::unique_lock lock(mutex_);
    const std::size_t my_generation = generation_;
    if (++waiting_ == parties_) {
        if (on_completion_) {
            on_completion_();
        }
        waiting_ = 0;
        ++generation_;
        lock.unlock();
        cv_.notify_all();
        return my_generation;
    }
    cv_.wait(lock, [this, my_generation] { return generation_ != my_generation; });
    return my_generation;
}

std::size_t CyclicBarrier::generation() const {
    std::lock_guard lock(mutex_);
    return generation_;
}

// ---------------------------------------------------------------- ManualResetEvent

void ManualResetEvent::set() {
    {
        std::lock_guard lock(mutex_);
        set_ = true;
    }
    cv_.notify_all();
}

void ManualResetEvent::reset() {
    std::lock_guard lock(mutex_);
    set_ = false;
}

void ManualResetEvent::wait() const {
    std::unique_lock lock(mutex_);
    cv_.wait(lock, [this] { return set_; });
}

bool ManualResetEvent::wait_for(std::chrono::nanoseconds timeout) const {
    std::unique_lock lock(mutex_);
    return cv_.wait_for(lock, timeout, [this] { return set_; });
}

bool ManualResetEvent::is_set() const {
    std::lock_guard lock(mutex_);
    return set_;
}

// ---------------------------------------------------------------- showcase

void demonstrate_condition_variables(std::ostream& out) {
    out << "=== Condition variables ===\n";

    // Producer/consumer with back-pressure and graceful close.
    {
        BoundedQueue<int> queue(8);
        constexpr int producers = 3;
        constexpr int per_producer = 200;
        std::atomic<long long> consumed_sum{0};
        std::atomic<int> consumed_count{0};

        std::vector<std::thread> consumers;
        consumers.reserve(2);
        for (int c = 0; c < 2; ++c) {
            consumers.emplace_back([&] {
                while (auto item = queue.pop()) {
                    consumed_sum.fetch_add(*item, std::memory_order_relaxed);
                    consumed_count.fetch_add(1, std::memory_order_relaxed);
                }
            });
        }
        std::vector<std::thread> producer_threads;
        producer_threads.reserve(static_cast<std::size_t>(producers));
        for (int p = 0; p < producers; ++p) {
            producer_threads.emplace_back([&queue, p] {
                for (int i = 1; i <= per_producer; ++i) {
                    static_cast<void>(queue.push(p * 1000 + i));
                }
            });
        }
        for (auto& t : producer_threads) {
            t.join();
        }
        queue.close();
        for (auto& t : consumers) {
            t.join();
        }
        out << "BoundedQueue(capacity 8): 3 producers -> 2 consumers, consumed " << consumed_count.load()
            << " items, checksum " << consumed_sum.load() << '\n';
    }

    // Latch as a start gun, barrier for lock-step phases.
    {
        constexpr std::size_t workers = 4;
        CountDownLatch start(1);
        std::vector<int> phase_totals;
        std::vector<int> contributions(workers, 0);
        CyclicBarrier barrier(workers, [&] {
            int total = 0;
            for (int c : contributions) {
                total += c;
            }
            phase_totals.push_back(total);
        });
        std::vector<std::thread> threads;
        for (std::size_t w = 0; w < workers; ++w) {
            threads.emplace_back([&, w] {
                start.wait();
                for (int phase = 1; phase <= 3; ++phase) {
                    contributions[w] = phase * static_cast<int>(w + 1);
                    barrier.arrive_and_wait();
                }
            });
        }
        start.count_down();
        for (auto& t : threads) {
            t.join();
        }
        out << "CyclicBarrier phases (sum of per-worker contributions):";
        for (int total : phase_totals) {
            out << ' ' << total;
        }
        out << '\n';
    }

    // Resource pool limits concurrency to the number of resources.
    {
        ResourcePool<std::string> pool({"uplink-A", "uplink-B"});
        std::atomic<int> in_use{0};
        std::atomic<int> peak{0};
        std::vector<std::thread> clients;
        clients.reserve(6);
        for (int i = 0; i < 6; ++i) {
            clients.emplace_back([&] {
                auto lease = pool.acquire();
                const int now = in_use.fetch_add(1) + 1;
                int seen = peak.load();
                while (now > seen && !peak.compare_exchange_weak(seen, now)) {}
                std::this_thread::yield();
                in_use.fetch_sub(1);
            });
        }
        for (auto& t : clients) {
            t.join();
        }
        out << "ResourcePool(2 uplinks) served 6 clients, peak concurrent leases <= " << pool.size() << ": "
            << std::boolalpha << (peak.load() <= 2) << '\n';
    }
    out << '\n';
}

} // namespace CppVerseHub::Concurrency
