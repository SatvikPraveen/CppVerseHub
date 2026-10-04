/**
 * @file MutexExamples.cpp
 * @brief Hierarchical mutex, bank transfers, dining philosophers and the mutex showcase.
 * @details File location: src/concurrency/MutexExamples.cpp
 */

#include "concurrency/MutexExamples.hpp"

#include <chrono>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <thread>

namespace CppVerseHub::Concurrency {

// ---------------------------------------------------------------- HierarchicalMutex

namespace {
thread_local std::uint64_t this_thread_hierarchy_level = std::numeric_limits<std::uint64_t>::max();
}

std::uint64_t HierarchicalMutex::current_thread_level() noexcept {
    return this_thread_hierarchy_level;
}

void HierarchicalMutex::check_for_violation() const {
    if (this_thread_hierarchy_level <= level_) {
        throw std::logic_error("mutex hierarchy violated: attempted to lock level " + std::to_string(level_) +
                               " while holding level " + std::to_string(this_thread_hierarchy_level));
    }
}

void HierarchicalMutex::update_level() noexcept {
    previous_level_ = this_thread_hierarchy_level;
    this_thread_hierarchy_level = level_;
}

void HierarchicalMutex::lock() {
    check_for_violation();
    mutex_.lock();
    update_level();
}

bool HierarchicalMutex::try_lock() {
    check_for_violation();
    if (!mutex_.try_lock()) {
        return false;
    }
    update_level();
    return true;
}

void HierarchicalMutex::unlock() noexcept {
    this_thread_hierarchy_level = previous_level_;
    mutex_.unlock();
}

// ---------------------------------------------------------------- BankAccount

std::int64_t BankAccount::balance() const {
    std::lock_guard lock(mutex_);
    return balance_;
}

void BankAccount::deposit(std::int64_t amount) {
    if (amount < 0) {
        throw std::invalid_argument("deposit amount must be non-negative");
    }
    std::lock_guard lock(mutex_);
    balance_ += amount;
}

bool transfer(BankAccount& from, BankAccount& to, std::int64_t amount) {
    if (amount <= 0 || &from == &to) {
        return false;
    }
    std::scoped_lock lock(from.mutex_, to.mutex_); // std::lock's deadlock-avoidance algorithm
    if (from.balance_ < amount) {
        return false;
    }
    from.balance_ -= amount;
    to.balance_ += amount;
    return true;
}

// ---------------------------------------------------------------- Dining philosophers

std::vector<int> run_dining_philosophers(std::size_t philosophers, int meals) {
    if (philosophers < 2) {
        throw std::invalid_argument("dining philosophers needs at least two seats");
    }
    std::vector<std::unique_ptr<std::mutex>> forks;
    forks.reserve(philosophers);
    for (std::size_t i = 0; i < philosophers; ++i) {
        forks.push_back(std::make_unique<std::mutex>());
    }
    std::vector<int> eaten(philosophers, 0);
    std::vector<std::thread> diners;
    diners.reserve(philosophers);
    for (std::size_t p = 0; p < philosophers; ++p) {
        diners.emplace_back([&, p] {
            std::mutex& left = *forks[p];
            std::mutex& right = *forks[(p + 1) % philosophers];
            for (int m = 0; m < meals; ++m) {
                // Naively locking left then right deadlocks when everyone grabs their
                // left fork at once; scoped_lock acquires both or neither.
                std::scoped_lock both(left, right);
                ++eaten[p]; // each philosopher owns its own slot
            }
        });
    }
    for (auto& d : diners) {
        d.join();
    }
    return eaten;
}

// ---------------------------------------------------------------- showcase

void demonstrate_mutexes(std::ostream& out) {
    out << "=== Mutexes and deadlock avoidance ===\n";

    {
        std::mutex m;
        long long counter = 0;
        std::vector<std::thread> threads;
        threads.reserve(4);
        for (int t = 0; t < 4; ++t) {
            threads.emplace_back([&] {
                for (int i = 0; i < 10000; ++i) {
                    std::lock_guard lock(m);
                    ++counter;
                }
            });
        }
        for (auto& t : threads) {
            t.join();
        }
        out << "std::mutex + lock_guard: 4 x 10000 increments = " << counter << '\n';
    }

    {
        std::timed_mutex tm;
        std::unique_lock held(tm);
        bool acquired = true;
        std::thread other([&] {
            std::unique_lock attempt(tm, std::defer_lock);
            acquired = attempt.try_lock_for(std::chrono::milliseconds(1));
        });
        other.join();
        out << "std::timed_mutex::try_lock_for while held elsewhere -> " << std::boolalpha << acquired
            << '\n';
    }

    {
        std::recursive_mutex rm;
        std::function<int(int)> depth = [&](int n) -> int {
            std::lock_guard lock(rm); // re-locking by the owner is allowed
            return n == 0 ? 0 : 1 + depth(n - 1);
        };
        out << "std::recursive_mutex re-entered " << depth(5) << " times\n";
    }

    {
        HierarchicalMutex high(1000);
        HierarchicalMutex low(100);
        {
            std::scoped_lock ordered(high);
            std::lock_guard inner(low);
            out << "HierarchicalMutex: locked 1000 then 100 (allowed)\n";
        }
        try {
            std::lock_guard first(low);
            std::lock_guard second(high);
        } catch (const std::logic_error& e) {
            out << "HierarchicalMutex: " << e.what() << '\n';
        }
    }

    {
        BankAccount a("ACC-A", 100000);
        BankAccount b("ACC-B", 100000);
        std::thread ab([&] {
            for (int i = 0; i < 1000; ++i) {
                static_cast<void>(transfer(a, b, 7));
            }
        });
        std::thread ba([&] {
            for (int i = 0; i < 1000; ++i) {
                static_cast<void>(transfer(b, a, 3));
            }
        });
        ab.join();
        ba.join();
        out << "Opposing transfers with scoped_lock: A=" << a.balance() << " B=" << b.balance()
            << " total=" << a.balance() + b.balance() << " (conserved)\n";
    }

    {
        const auto meals = run_dining_philosophers(5, 100);
        out << "Dining philosophers (5 seats, 100 meals each), total meals: "
            << std::accumulate(meals.begin(), meals.end(), 0) << '\n';
    }

    {
        ThreadSafeMap<std::string, int> registry;
        std::vector<std::thread> writers;
        writers.reserve(4);
        for (int t = 0; t < 4; ++t) {
            writers.emplace_back([&registry] {
                for (int i = 0; i < 250; ++i) {
                    registry.update("docking-requests", [](int& v) { ++v; });
                }
            });
        }
        for (auto& w : writers) {
            w.join();
        }
        out << "ThreadSafeMap (shared_mutex) docking-requests = "
            << registry.find("docking-requests").value_or(0) << '\n';

        int computations = 0;
        LazyValue<std::string> config([&computations] {
            ++computations;
            return std::string("orbital-parameters-v2");
        });
        std::vector<std::thread> readers;
        readers.reserve(4);
        for (int t = 0; t < 4; ++t) {
            readers.emplace_back([&config] { static_cast<void>(config.get()); });
        }
        for (auto& r : readers) {
            r.join();
        }
        out << "LazyValue via std::call_once: \"" << config.get() << "\" computed " << computations
            << " time(s)\n";
    }
    out << '\n';
}

} // namespace CppVerseHub::Concurrency
