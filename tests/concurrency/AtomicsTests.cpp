// Behavioural tests for Atomics.hpp. Catch2 assertions run only on the main thread.

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <algorithm>
#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "concurrency/Atomics.hpp"

using namespace CppVerseHub::Concurrency;

TEST_CASE("SpinLock provides mutual exclusion and try_lock semantics", "[concurrency][atomics]") {
    SpinLock lock;
    CHECK(lock.try_lock());
    CHECK_FALSE(lock.try_lock());
    lock.unlock();

    long long counter = 0;
    std::vector<std::thread> threads;
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([&] {
            for (int i = 0; i < 5000; ++i) {
                std::lock_guard guard(lock);
                ++counter;
            }
        });
    }
    for (auto& t : threads) {
        t.join();
    }
    CHECK(counter == 20000);
}

TEST_CASE("SpscRingBuffer single-threaded FIFO, full and empty", "[concurrency][atomics][spsc]") {
    SpscRingBuffer<int, 4> ring;
    STATIC_REQUIRE(SpscRingBuffer<int, 4>::capacity() == 4);
    CHECK(ring.empty_approx());
    CHECK_FALSE(ring.try_pop().has_value());
    for (int i = 0; i < 4; ++i) {
        CHECK(ring.try_push(i));
    }
    CHECK_FALSE(ring.try_push(99));
    CHECK(ring.size_approx() == 4);
    for (int i = 0; i < 4; ++i) {
        CHECK(ring.try_pop() == i);
    }
    CHECK_FALSE(ring.try_pop().has_value());

    // Wrap the indices around many times.
    for (int round = 0; round < 1000; ++round) {
        REQUIRE(ring.try_push(round));
        REQUIRE(ring.try_push(round + 1));
        REQUIRE(ring.try_pop() == round);
        REQUIRE(ring.try_pop() == round + 1);
    }
    CHECK(ring.empty_approx());
}

TEST_CASE("SpscRingBuffer manages non-trivial element lifetimes", "[concurrency][atomics][spsc]") {
    auto tracker = std::make_shared<int>(0);
    {
        SpscRingBuffer<std::shared_ptr<int>, 8> ring;
        for (int i = 0; i < 5; ++i) {
            REQUIRE(ring.try_push(tracker));
        }
        CHECK(tracker.use_count() == 6);
        auto popped = ring.try_pop();
        REQUIRE(popped.has_value());
        popped.reset();
        CHECK(tracker.use_count() == 5);  // the slot was destroyed on pop
        auto rejected = tracker;
        for (int i = 0; i < 4; ++i) {
            REQUIRE(ring.try_push(tracker));
        }
        CHECK_FALSE(ring.try_push(std::move(rejected)));
        CHECK(rejected != nullptr);  // failed push did not consume the argument
    }
    CHECK(tracker.use_count() == 1);  // destructor destroyed the remaining elements

    SpscRingBuffer<std::string, 2> strings;
    REQUIRE(strings.try_push(std::string(100, 'x')));
    CHECK(strings.try_pop()->size() == 100);
}

TEST_CASE("SpscRingBuffer transfers a stream between two threads in order", "[concurrency][atomics][spsc]") {
    SpscRingBuffer<std::uint64_t, 128> ring;
    constexpr std::uint64_t items = 100000;
    std::uint64_t mismatches = 0;
    std::uint64_t received = 0;
    std::thread consumer([&] {
        std::uint64_t expected = 0;
        while (expected < items) {
            if (auto v = ring.try_pop()) {
                mismatches += (*v != expected) ? 1 : 0;
                ++expected;
            } else {
                cpu_relax();
            }
        }
        received = expected;
    });
    for (std::uint64_t i = 0; i < items; ++i) {
        while (!ring.try_push(i)) {
            cpu_relax();
        }
    }
    consumer.join();
    CHECK(received == items);
    CHECK(mismatches == 0);
    CHECK(ring.empty_approx());
}

TEST_CASE("LockFreeStack is LIFO single-threaded", "[concurrency][atomics][stack]") {
    LockFreeStack<std::unique_ptr<int>> stack;
    CHECK(stack.empty());
    CHECK_FALSE(stack.pop().has_value());
    for (int i = 0; i < 5; ++i) {
        stack.push(std::make_unique<int>(i));
    }
    CHECK_FALSE(stack.empty());
    for (int i = 4; i >= 0; --i) {
        auto v = stack.pop();
        REQUIRE(v.has_value());
        CHECK(**v == i);
    }
    CHECK(stack.empty());
    CHECK(stack.pending_reclamation() == 0);  // a lone popper frees nodes immediately
}

TEST_CASE("LockFreeStack concurrent push/pop delivers every value exactly once", "[concurrency][atomics][stack]") {
    LockFreeStack<int> stack;
    constexpr int threads = 4;
    constexpr int per_thread = 5000;
    std::vector<std::atomic<int>> seen(threads * per_thread);
    std::vector<std::thread> workers;
    for (int t = 0; t < threads; ++t) {
        workers.emplace_back([&, t] {
            for (int i = 0; i < per_thread; ++i) {
                stack.push(t * per_thread + i);
                if (i % 2 == 1) {  // interleave pops with pushes to stress reclamation
                    if (auto v = stack.pop()) {
                        seen[static_cast<std::size_t>(*v)].fetch_add(1);
                    }
                }
            }
        });
    }
    for (auto& w : workers) {
        w.join();
    }
    while (auto v = stack.pop()) {
        seen[static_cast<std::size_t>(*v)].fetch_add(1);
    }
    CHECK(std::all_of(seen.begin(), seen.end(), [](const std::atomic<int>& s) { return s.load() == 1; }));
    CHECK(stack.empty());
}

TEST_CASE("AtomicStatistics aggregates samples from many threads", "[concurrency][atomics]") {
    AtomicStatistics stats;
    CHECK(stats.snapshot().count == 0);
    CHECK(stats.snapshot().mean() == 0.0);
    std::vector<std::thread> threads;
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([&stats, t] {
            for (int i = 0; i < 1000; ++i) {
                stats.record(t * 1000 + i - 1500);
            }
        });
    }
    for (auto& t : threads) {
        t.join();
    }
    const auto s = stats.snapshot();
    CHECK(s.count == 4000);
    CHECK(s.min == -1500);
    CHECK(s.max == 2499);
    CHECK(s.sum == (3999LL * 4000 / 2) - 1500LL * 4000);
    stats.reset();
    CHECK(stats.snapshot().count == 0);
}

TEST_CASE("atomic_fetch_max and atomic_fetch_min", "[concurrency][atomics]") {
    std::atomic<int> hi{5};
    CHECK(atomic_fetch_max(hi, 3) == 5);
    CHECK(hi.load() == 5);
    CHECK(atomic_fetch_max(hi, 9) == 5);
    CHECK(hi.load() == 9);
    std::atomic<int> lo{5};
    CHECK(atomic_fetch_min(lo, 7) == 5);
    CHECK(atomic_fetch_min(lo, -2) == 5);
    CHECK(lo.load() == -2);

    std::atomic<int> shared_max{0};
    std::vector<std::thread> threads;
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([&shared_max, t] {
            for (int i = 0; i < 1000; ++i) {
                static_cast<void>(atomic_fetch_max(shared_max, t * 1000 + i));
            }
        });
    }
    for (auto& t : threads) {
        t.join();
    }
    CHECK(shared_max.load() == 3999);
}

TEST_CASE("ConcurrentBloomFilter has no false negatives and few false positives", "[concurrency][atomics]") {
    ConcurrentBloomFilter filter(1 << 14, 5);
    CHECK(filter.bit_count() == (1 << 14));
    CHECK(filter.popcount() == 0);
    CHECK_FALSE(filter.possibly_contains("anything"));
    std::vector<std::thread> threads;
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([&filter, t] {
            for (int i = 0; i < 250; ++i) {
                filter.insert("ship-" + std::to_string(t * 250 + i));
            }
        });
    }
    for (auto& t : threads) {
        t.join();
    }
    for (int i = 0; i < 1000; ++i) {
        REQUIRE(filter.possibly_contains("ship-" + std::to_string(i)));
    }
    int false_positives = 0;
    for (int i = 0; i < 10000; ++i) {
        false_positives += filter.possibly_contains("rock-" + std::to_string(i)) ? 1 : 0;
    }
    // Theoretical rate for m=16384, n=1000, k=5 is ~0.2%; allow generous slack.
    CHECK(false_positives < 200);
    CHECK(filter.popcount() > 0);
    CHECK(filter.popcount() <= 5000);

    ConcurrentBloomFilter tiny(1, 0);  // clamped to 64 bits and 1 hash
    CHECK(tiny.bit_count() == 64);
    tiny.insert("x");
    CHECK(tiny.possibly_contains("x"));
    CHECK(tiny.popcount() == 1);
}

TEST_CASE("OneShotEvent releases waiters and publishes prior writes", "[concurrency][atomics]") {
    OneShotEvent event;
    CHECK_FALSE(event.is_set());
    std::vector<int> results(4, 0);
    int payload = 0;
    std::vector<std::thread> waiters;
    for (std::size_t i = 0; i < results.size(); ++i) {
        waiters.emplace_back([&, i] {
            event.wait();
            results[i] = payload;
        });
    }
    payload = 77;
    event.set();
    for (auto& w : waiters) {
        w.join();
    }
    CHECK(event.is_set());
    CHECK(std::all_of(results.begin(), results.end(), [](int r) { return r == 77; }));
    event.wait();  // already set: returns immediately
}

TEST_CASE("Release/acquire message passing and relaxed counters", "[concurrency][atomics]") {
    const int payload = GENERATE(0, 1, 42, -7, 123456);
    CHECK(release_acquire_message_passing(payload) == payload);
    CHECK(relaxed_counter_total(4, 2500) == 10000);
    CHECK(relaxed_counter_total(0, 100) == 0);
}
