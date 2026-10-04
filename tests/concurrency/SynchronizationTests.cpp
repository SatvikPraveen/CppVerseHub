// Behavioural tests for MutexExamples.hpp and ConditionalVariables.hpp.
// Catch2 assertions are only used on the main test thread (they are not thread safe).

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <numeric>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "concurrency/ConditionalVariables.hpp"
#include "concurrency/MutexExamples.hpp"

using namespace CppVerseHub::Concurrency;
using namespace std::chrono_literals;

// =========================================================================== mutexes

TEST_CASE("HierarchicalMutex enforces lock ordering", "[concurrency][mutex]") {
    HierarchicalMutex high(1000);
    HierarchicalMutex mid(500);
    HierarchicalMutex low(100);
    const auto base = HierarchicalMutex::current_thread_level();

    SECTION("descending order is allowed and levels are restored") {
        {
            std::lock_guard a(high);
            CHECK(HierarchicalMutex::current_thread_level() == 1000);
            std::lock_guard b(mid);
            std::lock_guard c(low);
            CHECK(HierarchicalMutex::current_thread_level() == 100);
        }
        CHECK(HierarchicalMutex::current_thread_level() == base);
    }
    SECTION("ascending order throws and leaves the higher mutex unlocked") {
        std::lock_guard a(low);
        CHECK_THROWS_AS(high.lock(), std::logic_error);
        CHECK_THROWS_AS(static_cast<void>(mid.try_lock()), std::logic_error);
        bool acquired_elsewhere = false;
        std::thread other([&] {
            acquired_elsewhere = high.try_lock();  // was never acquired by the failed attempt
            if (acquired_elsewhere) {
                high.unlock();
            }
        });
        other.join();
        CHECK(acquired_elsewhere);
    }
    SECTION("same level cannot be nested") {
        HierarchicalMutex peer(1000);
        std::lock_guard a(high);
        CHECK_THROWS_AS(peer.lock(), std::logic_error);
    }
}

TEST_CASE("HierarchicalMutex provides mutual exclusion", "[concurrency][mutex]") {
    HierarchicalMutex m(10);
    int counter = 0;
    std::vector<std::thread> threads;
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([&] {
            for (int i = 0; i < 2000; ++i) {
                std::lock_guard lock(m);
                ++counter;
            }
        });
    }
    for (auto& t : threads) {
        t.join();
    }
    CHECK(counter == 8000);
}

TEST_CASE("Synchronized guards its value", "[concurrency][mutex]") {
    Synchronized<std::vector<int>> values;
    std::vector<std::thread> writers;
    for (int t = 0; t < 4; ++t) {
        writers.emplace_back([&values, t] {
            for (int i = 0; i < 250; ++i) {
                values.with_lock([&](std::vector<int>& v) { v.push_back(t); });
            }
        });
    }
    for (auto& w : writers) {
        w.join();
    }
    CHECK(values.with_shared_lock([](const std::vector<int>& v) { return v.size(); }) == 1000);
    const auto copy = values.copy();
    CHECK(std::count(copy.begin(), copy.end(), 2) == 250);
}

TEST_CASE("ThreadSafeQueue FIFO, close and blocking pop", "[concurrency][mutex]") {
    ThreadSafeQueue<int> q;
    CHECK(q.empty());
    CHECK_FALSE(q.try_pop().has_value());
    CHECK(q.push(1));
    CHECK(q.push(2));
    CHECK(q.size() == 2);
    CHECK(q.try_pop() == 1);

    std::vector<int> received;
    std::thread consumer([&] {
        while (auto v = q.wait_and_pop()) {
            received.push_back(*v);
        }
    });
    for (int i = 3; i <= 10; ++i) {
        CHECK(q.push(i));
    }
    q.close();
    consumer.join();
    CHECK(received == std::vector<int>{2, 3, 4, 5, 6, 7, 8, 9, 10});
    CHECK_FALSE(q.push(11));
}

TEST_CASE("ThreadSafeMap basic operations and concurrent updates", "[concurrency][mutex]") {
    ThreadSafeMap<std::string, int> map;
    CHECK(map.insert_or_assign("mars", 4));
    CHECK_FALSE(map.insert_or_assign("mars", 5));
    CHECK(map.find("mars") == 5);
    CHECK_FALSE(map.find("pluto").has_value());
    CHECK(map.contains("mars"));
    CHECK(map.erase("mars"));
    CHECK_FALSE(map.erase("mars"));
    CHECK(map.size() == 0);

    std::vector<std::thread> threads;
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([&map, t] {
            for (int i = 0; i < 100; ++i) {
                map.update("shared", [](int& v) { v += 1; });
                map.insert_or_assign("t" + std::to_string(t), i);
                static_cast<void>(map.find("shared"));
            }
        });
    }
    for (auto& t : threads) {
        t.join();
    }
    const auto snap = map.snapshot();
    CHECK(snap.size() == 5);
    CHECK(snap.at("shared") == 400);
    CHECK(snap.at("t3") == 99);
}

TEST_CASE("transfer validates its arguments", "[concurrency][mutex]") {
    BankAccount a("A", 100);
    BankAccount b("B", 0);
    CHECK_FALSE(transfer(a, b, 0));
    CHECK_FALSE(transfer(a, b, -5));
    CHECK_FALSE(transfer(a, a, 10));
    CHECK_FALSE(transfer(a, b, 101));
    CHECK(transfer(a, b, 60));
    CHECK(a.balance() == 40);
    CHECK(b.balance() == 60);
    CHECK_THROWS_AS(a.deposit(-1), std::invalid_argument);
    a.deposit(10);
    CHECK(a.balance() == 50);
    CHECK(a.id() == "A");
}

TEST_CASE("Opposing concurrent transfers neither deadlock nor lose money", "[concurrency][mutex]") {
    std::vector<std::unique_ptr<BankAccount>> accounts;
    for (int i = 0; i < 4; ++i) {
        accounts.push_back(std::make_unique<BankAccount>("acc" + std::to_string(i), 1000));
    }
    std::vector<std::thread> threads;
    for (std::size_t t = 0; t < 4; ++t) {
        threads.emplace_back([&accounts, t] {
            for (std::size_t i = 0; i < 500; ++i) {
                auto& from = *accounts[(t + i) % 4];
                auto& to = *accounts[(t + i + 1 + t % 2) % 4];
                static_cast<void>(transfer(from, to, static_cast<std::int64_t>(1 + i % 7)));
            }
        });
    }
    for (auto& t : threads) {
        t.join();
    }
    std::int64_t total = 0;
    for (const auto& acc : accounts) {
        total += acc->balance();
        CHECK(acc->balance() >= 0);
    }
    CHECK(total == 4000);
}

TEST_CASE("Dining philosophers all finish their meals", "[concurrency][mutex]") {
    const std::size_t seats = GENERATE(2U, 5U, 8U);
    const auto meals = run_dining_philosophers(seats, 50);
    REQUIRE(meals.size() == seats);
    CHECK(std::all_of(meals.begin(), meals.end(), [](int m) { return m == 50; }));
    CHECK_THROWS_AS(run_dining_philosophers(1, 1), std::invalid_argument);
}

TEST_CASE("LazyValue initialises exactly once under contention", "[concurrency][mutex]") {
    std::atomic<int> calls{0};
    LazyValue<std::vector<int>> lazy([&calls] {
        calls.fetch_add(1);
        return std::vector<int>{1, 2, 3};
    });
    CHECK_FALSE(lazy.has_value());
    std::vector<std::thread> threads;
    std::atomic<int> sizes{0};
    for (int t = 0; t < 8; ++t) {
        threads.emplace_back([&] { sizes.fetch_add(static_cast<int>(lazy.get().size())); });
    }
    for (auto& t : threads) {
        t.join();
    }
    CHECK(calls.load() == 1);
    CHECK(sizes.load() == 24);
    CHECK(lazy.has_value());
}

// =========================================================================== condition variables

TEST_CASE("BoundedQueue single-threaded semantics", "[concurrency][condvar]") {
    CHECK_THROWS_AS(BoundedQueue<int>(0), std::invalid_argument);
    BoundedQueue<int> q(3);
    CHECK(q.capacity() == 3);
    CHECK(q.empty());
    CHECK(q.try_push(1));
    CHECK(q.push(2));
    CHECK(q.try_push(3));
    CHECK_FALSE(q.try_push(4));  // full
    CHECK(q.size() == 3);
    CHECK(q.try_pop() == 1);
    CHECK(q.pop() == 2);
    CHECK(q.try_pop() == 3);
    CHECK_FALSE(q.try_pop().has_value());
}

TEST_CASE("BoundedQueue close semantics", "[concurrency][condvar]") {
    BoundedQueue<std::unique_ptr<int>> q(2);
    CHECK(q.push(std::make_unique<int>(1)));
    q.close();
    CHECK(q.is_closed());
    auto rejected = std::make_unique<int>(2);
    CHECK_FALSE(q.push(std::move(rejected)));
    REQUIRE(rejected != nullptr);  // a failed push does not consume its argument
    CHECK(*rejected == 2);
    auto drained = q.pop();
    REQUIRE(drained.has_value());
    CHECK(**drained == 1);
    CHECK_FALSE(q.pop().has_value());  // closed and empty: does not block
}

TEST_CASE("BoundedQueue timed operations time out when they cannot succeed", "[concurrency][condvar]") {
    BoundedQueue<int> q(1);
    CHECK_FALSE(q.pop_for(1ms).has_value());
    CHECK(q.push_for(5, 1ms));
    int value = 6;
    CHECK_FALSE(q.push_for(value, 1ms));
    CHECK(q.pop_for(1ms) == 5);
}

TEST_CASE("BoundedQueue back-pressure preserves order with capacity 1", "[concurrency][condvar]") {
    BoundedQueue<int> q(1);
    std::atomic<int> rejected{0};
    std::thread producer([&] {
        for (int i = 0; i < 200; ++i) {
            rejected.fetch_add(q.push(i) ? 0 : 1);
        }
        q.close();
    });
    std::vector<int> got;
    while (auto v = q.pop()) {
        got.push_back(*v);
    }
    producer.join();
    CHECK(rejected.load() == 0);
    std::vector<int> expected(200);
    std::iota(expected.begin(), expected.end(), 0);
    CHECK(got == expected);
}

TEST_CASE("BoundedQueue MPMC delivers every item exactly once", "[concurrency][condvar]") {
    const std::size_t capacity = GENERATE(1U, 4U, 64U);
    BoundedQueue<int> q(capacity);
    constexpr int producers = 4;
    constexpr int per_producer = 1000;
    std::vector<std::atomic<int>> seen(producers * per_producer);
    std::vector<std::thread> consumers;
    for (int c = 0; c < 3; ++c) {
        consumers.emplace_back([&] {
            while (auto v = q.pop()) {
                seen[static_cast<std::size_t>(*v)].fetch_add(1);
            }
        });
    }
    std::vector<std::thread> producer_threads;
    for (int p = 0; p < producers; ++p) {
        producer_threads.emplace_back([&q, p] {
            for (int i = 0; i < per_producer; ++i) {
                static_cast<void>(q.push(p * per_producer + i));  // never closed while producing
            }
        });
    }
    for (auto& t : producer_threads) {
        t.join();
    }
    q.close();
    for (auto& t : consumers) {
        t.join();
    }
    CHECK(std::all_of(seen.begin(), seen.end(), [](const std::atomic<int>& s) { return s.load() == 1; }));
}

TEST_CASE("BoundedQueue close wakes blocked producers and consumers", "[concurrency][condvar]") {
    BoundedQueue<int> full(1);
    REQUIRE(full.push(1));
    BoundedQueue<int> empty(1);
    std::atomic<int> push_result{-1};
    std::atomic<int> pop_result{-1};
    std::thread producer([&] { push_result.store(full.push(2) ? 1 : 0); });
    std::thread consumer([&] { pop_result.store(empty.pop().has_value() ? 1 : 0); });
    full.close();
    empty.close();
    producer.join();
    consumer.join();
    CHECK(push_result.load() == 0);
    CHECK(pop_result.load() == 0);
}

TEST_CASE("CountingSemaphore permits", "[concurrency][condvar]") {
    CountingSemaphore sem(2);
    CHECK(sem.available() == 2);
    CHECK(sem.try_acquire());
    CHECK(sem.try_acquire());
    CHECK_FALSE(sem.try_acquire());
    CHECK_FALSE(sem.try_acquire_for(1ms));
    sem.release(2);
    CHECK(sem.available() == 2);

    CountingSemaphore signal(0);
    std::thread waiter([&] { signal.acquire(); });
    signal.release();
    waiter.join();
    CHECK(signal.available() == 0);
}

TEST_CASE("CountingSemaphore bounds concurrency", "[concurrency][condvar]") {
    CountingSemaphore sem(2);
    std::atomic<int> inside{0};
    std::atomic<int> peak{0};
    std::vector<std::thread> threads;
    for (int t = 0; t < 8; ++t) {
        threads.emplace_back([&] {
            for (int i = 0; i < 50; ++i) {
                sem.acquire();
                const int now = inside.fetch_add(1) + 1;
                int p = peak.load();
                while (now > p && !peak.compare_exchange_weak(p, now)) {
                }
                inside.fetch_sub(1);
                sem.release();
            }
        });
    }
    for (auto& t : threads) {
        t.join();
    }
    CHECK(peak.load() <= 2);
    CHECK(peak.load() >= 1);
    CHECK(sem.available() == 2);
}

TEST_CASE("CountDownLatch opens at zero", "[concurrency][condvar]") {
    CountDownLatch latch(3);
    CHECK_FALSE(latch.try_wait());
    CHECK_FALSE(latch.wait_for(1ms));
    std::vector<std::thread> workers;
    for (int i = 0; i < 3; ++i) {
        workers.emplace_back([&] { latch.count_down(); });
    }
    latch.wait();
    for (auto& w : workers) {
        w.join();
    }
    CHECK(latch.try_wait());
    CHECK(latch.count() == 0);
    latch.count_down(5);  // saturates
    CHECK(latch.count() == 0);

    CountDownLatch big(2);
    big.count_down(10);
    CHECK(big.try_wait());

    CountDownLatch rendezvous(2);
    std::thread partner([&] { rendezvous.arrive_and_wait(); });
    rendezvous.arrive_and_wait();
    partner.join();
    CHECK(rendezvous.try_wait());
}

TEST_CASE("CyclicBarrier synchronises phases and runs the completion once per phase", "[concurrency][condvar]") {
    CHECK_THROWS_AS(CyclicBarrier(0), std::invalid_argument);
    constexpr std::size_t parties = 4;
    constexpr int phases = 20;
    int completions = 0;
    std::vector<int> slots(parties, 0);
    bool consistent = true;
    CyclicBarrier barrier(parties, [&] {
        ++completions;
        // Every party must have written this phase's value before the completion runs.
        consistent = consistent && std::all_of(slots.begin(), slots.end(), [&](int s) { return s == completions; });
    });
    std::vector<std::vector<std::size_t>> generations(parties);
    std::vector<std::thread> threads;
    for (std::size_t p = 0; p < parties; ++p) {
        threads.emplace_back([&, p] {
            for (int phase = 1; phase <= phases; ++phase) {
                slots[p] = phase;
                generations[p].push_back(barrier.arrive_and_wait());
            }
        });
    }
    for (auto& t : threads) {
        t.join();
    }
    CHECK(completions == phases);
    CHECK(consistent);
    CHECK(barrier.generation() == static_cast<std::size_t>(phases));
    CHECK(barrier.parties() == parties);
    for (const auto& g : generations) {
        std::vector<std::size_t> expected(phases);
        std::iota(expected.begin(), expected.end(), std::size_t{0});
        CHECK(g == expected);
    }
}

TEST_CASE("ManualResetEvent set/reset semantics", "[concurrency][condvar]") {
    ManualResetEvent event;
    CHECK_FALSE(event.is_set());
    CHECK_FALSE(event.wait_for(1ms));
    std::vector<std::thread> waiters;
    std::atomic<int> released{0};
    for (int i = 0; i < 3; ++i) {
        waiters.emplace_back([&] {
            event.wait();
            released.fetch_add(1);
        });
    }
    event.set();
    for (auto& w : waiters) {
        w.join();
    }
    CHECK(released.load() == 3);
    CHECK(event.wait_for(0ms));
    event.reset();
    CHECK_FALSE(event.is_set());
    CHECK(ManualResetEvent(true).is_set());
}

TEST_CASE("ResourcePool leases resources and blocks when exhausted", "[concurrency][condvar]") {
    CHECK_THROWS_AS(ResourcePool<int>(std::vector<int>{}), std::invalid_argument);
    ResourcePool<std::string> pool({"alpha", "beta"});
    CHECK(pool.size() == 2);
    {
        auto a = pool.acquire();
        auto b = pool.acquire();
        CHECK(pool.available() == 0);
        CHECK(a.index() != b.index());
        CHECK_FALSE(pool.try_acquire_for(1ms).has_value());
        a->append("-x");
        auto moved = std::move(a);
        CHECK(pool.available() == 0);
        moved.reset();
        CHECK(pool.available() == 1);
        auto again = pool.acquire();
        CHECK((*again == "alpha-x" || *again == "beta-x"));  // the modified resource came back
    }
    CHECK(pool.available() == 2);

    std::atomic<int> inside{0};
    std::atomic<int> peak{0};
    std::vector<std::thread> clients;
    for (int t = 0; t < 6; ++t) {
        clients.emplace_back([&] {
            for (int i = 0; i < 50; ++i) {
                auto lease = pool.acquire();
                const int now = inside.fetch_add(1) + 1;
                int p = peak.load();
                while (now > p && !peak.compare_exchange_weak(p, now)) {
                }
                inside.fetch_sub(1);
            }
        });
    }
    for (auto& c : clients) {
        c.join();
    }
    CHECK(peak.load() <= 2);
    CHECK(pool.available() == 2);
}
