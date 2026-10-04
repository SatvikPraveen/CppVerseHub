/**
 * @file Atomics.cpp
 * @brief Out-of-line atomics utilities and the atomics showcase.
 * @details File location: src/concurrency/Atomics.cpp
 */

#include "concurrency/Atomics.hpp"

#include <algorithm>
#include <mutex>
#include <string>

namespace CppVerseHub::Concurrency {

// ---------------------------------------------------------------- AtomicStatistics

void AtomicStatistics::record(std::int64_t value) noexcept {
    count_.fetch_add(1, std::memory_order_relaxed);
    sum_.fetch_add(value, std::memory_order_relaxed);
    static_cast<void>(atomic_fetch_min(min_, value));
    static_cast<void>(atomic_fetch_max(max_, value));
}

AtomicStatistics::Snapshot AtomicStatistics::snapshot() const noexcept {
    Snapshot s;
    s.count = count_.load(std::memory_order_relaxed);
    s.sum = sum_.load(std::memory_order_relaxed);
    if (s.count != 0) {
        s.min = min_.load(std::memory_order_relaxed);
        s.max = max_.load(std::memory_order_relaxed);
    }
    return s;
}

void AtomicStatistics::reset() noexcept {
    count_.store(0, std::memory_order_relaxed);
    sum_.store(0, std::memory_order_relaxed);
    min_.store(std::numeric_limits<std::int64_t>::max(), std::memory_order_relaxed);
    max_.store(std::numeric_limits<std::int64_t>::min(), std::memory_order_relaxed);
}

// ---------------------------------------------------------------- ConcurrentBloomFilter

namespace {

constexpr std::uint64_t fnv1a64(std::string_view key) noexcept {
    std::uint64_t hash = 14695981039346656037ULL;
    for (const char c : key) {
        hash ^= static_cast<std::uint8_t>(c);
        hash *= 1099511628211ULL;
    }
    return hash;
}

constexpr std::uint64_t splitmix64(std::uint64_t x) noexcept {
    x += 0x9E3779B97F4A7C15ULL;
    x = (x ^ (x >> 30U)) * 0xBF58476D1CE4E5B9ULL;
    x = (x ^ (x >> 27U)) * 0x94D049BB133111EBULL;
    return x ^ (x >> 31U);
}

} // namespace

ConcurrentBloomFilter::ConcurrentBloomFilter(std::size_t bit_count, std::size_t hash_count)
    : words_(std::max<std::size_t>(1, (bit_count + 63) / 64))
    , hash_count_(std::max<std::size_t>(1, hash_count)) {}

std::size_t ConcurrentBloomFilter::probe(std::uint64_t h1, std::uint64_t h2, std::size_t i) const noexcept {
    // Double hashing (Kirsch-Mitzenmacher): g_i = h1 + i*h2 mod m. The result is < m, so
    // it fits in std::size_t; no cast is written to keep GCC's -Wuseless-cast quiet on LP64.
    const std::uint64_t bits = bit_count();
    const std::uint64_t index = i;
    const std::size_t bit = (h1 + index * h2) % bits;
    return bit;
}

void ConcurrentBloomFilter::insert(std::string_view key) noexcept {
    const std::uint64_t h1 = fnv1a64(key);
    const std::uint64_t h2 = splitmix64(h1) | 1U;
    for (std::size_t i = 0; i < hash_count_; ++i) {
        const std::size_t bit = probe(h1, h2, i);
        words_[bit / 64].fetch_or(std::uint64_t{1} << (bit % 64), std::memory_order_relaxed);
    }
}

bool ConcurrentBloomFilter::possibly_contains(std::string_view key) const noexcept {
    const std::uint64_t h1 = fnv1a64(key);
    const std::uint64_t h2 = splitmix64(h1) | 1U;
    for (std::size_t i = 0; i < hash_count_; ++i) {
        const std::size_t bit = probe(h1, h2, i);
        if ((words_[bit / 64].load(std::memory_order_relaxed) & (std::uint64_t{1} << (bit % 64))) == 0) {
            return false;
        }
    }
    return true;
}

std::size_t ConcurrentBloomFilter::popcount() const noexcept {
    std::size_t total = 0;
    for (const auto& word : words_) {
        total += static_cast<std::size_t>(std::popcount(word.load(std::memory_order_relaxed)));
    }
    return total;
}

// ---------------------------------------------------------------- free functions

int release_acquire_message_passing(int payload) {
    int data = 0; // deliberately non-atomic
    std::atomic<bool> ready{false};
    int observed = -1;

    std::thread reader([&] {
        // acquire: synchronises-with the release store below, so the write to `data`
        // happens-before this read. A relaxed load here would be a data race.
        while (!ready.load(std::memory_order_acquire)) {
            cpu_relax();
        }
        observed = data;
    });
    std::thread writer([&] {
        data = payload;
        ready.store(true, std::memory_order_release);
    });
    writer.join();
    reader.join();
    return observed;
}

std::uint64_t relaxed_counter_total(std::size_t threads, std::uint64_t increments_per_thread) {
    std::atomic<std::uint64_t> counter{0};
    std::vector<std::thread> workers;
    workers.reserve(threads);
    for (std::size_t t = 0; t < threads; ++t) {
        workers.emplace_back([&counter, increments_per_thread] {
            for (std::uint64_t i = 0; i < increments_per_thread; ++i) {
                // relaxed: atomicity alone guarantees no lost update; the final value is
                // read after join(), which provides the happens-before edge.
                counter.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }
    for (auto& w : workers) {
        w.join();
    }
    return counter.load(std::memory_order_relaxed);
}

// ---------------------------------------------------------------- showcase

void demonstrate_atomics(std::ostream& out) {
    out << "=== Atomics and lock-free structures ===\n";
    out << "std::atomic<int> is lock-free: " << std::boolalpha << std::atomic<int>{}.is_lock_free()
        << ", std::atomic<std::uint64_t> always lock-free: "
        << std::atomic<std::uint64_t>::is_always_lock_free << '\n';
    out << "Release/acquire message passing delivered payload " << release_acquire_message_passing(42)
        << '\n';
    out << "Relaxed fetch_add from 4 threads x 10000 = " << relaxed_counter_total(4, 10000) << '\n';

    {
        SpinLock lock;
        long long shared = 0;
        std::vector<std::thread> threads;
        for (int t = 0; t < 4; ++t) {
            threads.emplace_back([&] {
                for (int i = 0; i < 5000; ++i) {
                    std::lock_guard guard(lock);
                    ++shared;
                }
            });
        }
        for (auto& t : threads) {
            t.join();
        }
        out << "SpinLock-protected counter: " << shared << " (expected 20000)\n";
    }

    {
        SpscRingBuffer<int, 64> ring;
        constexpr int items = 10000;
        long long checksum = 0;
        bool in_order = true;
        std::thread consumer([&] {
            int expected = 0;
            while (expected < items) {
                if (auto v = ring.try_pop()) {
                    in_order = in_order && (*v == expected);
                    checksum += *v;
                    ++expected;
                } else {
                    cpu_relax();
                }
            }
        });
        for (int i = 0; i < items; ++i) {
            while (!ring.try_push(i)) {
                cpu_relax();
            }
        }
        consumer.join();
        out << "SPSC ring (64 slots): transferred " << items << " items, FIFO preserved: " << in_order
            << ", checksum " << checksum << '\n';
    }

    {
        LockFreeStack<int> stack;
        std::vector<std::thread> pushers;
        for (int t = 0; t < 4; ++t) {
            pushers.emplace_back([&stack, t] {
                for (int i = 0; i < 1000; ++i) {
                    stack.push(t * 1000 + i);
                }
            });
        }
        for (auto& t : pushers) {
            t.join();
        }
        std::atomic<long long> popped_sum{0};
        std::atomic<int> popped{0};
        std::vector<std::thread> poppers;
        for (int t = 0; t < 4; ++t) {
            poppers.emplace_back([&] {
                while (auto v = stack.pop()) {
                    popped_sum.fetch_add(*v, std::memory_order_relaxed);
                    popped.fetch_add(1, std::memory_order_relaxed);
                }
            });
        }
        for (auto& t : poppers) {
            t.join();
        }
        out << "LockFreeStack: 4 pushers x 1000, popped " << popped.load() << " values, sum "
            << popped_sum.load() << '\n';
    }

    {
        AtomicStatistics stats;
        std::vector<std::thread> threads;
        for (int t = 0; t < 4; ++t) {
            threads.emplace_back([&stats, t] {
                for (int i = 1; i <= 100; ++i) {
                    stats.record(t * 100 + i);
                }
            });
        }
        for (auto& t : threads) {
            t.join();
        }
        const auto s = stats.snapshot();
        out << "AtomicStatistics: count " << s.count << ", min " << s.min << ", max " << s.max << ", mean "
            << s.mean() << '\n';
    }

    {
        ConcurrentBloomFilter filter(4096, 4);
        for (int i = 0; i < 200; ++i) {
            filter.insert("probe-" + std::to_string(i));
        }
        int false_positives = 0;
        for (int i = 1000; i < 2000; ++i) {
            false_positives += filter.possibly_contains("probe-" + std::to_string(i)) ? 1 : 0;
        }
        out << "Bloom filter (4096 bits, k=4, 200 keys): \"probe-7\" present: "
            << filter.possibly_contains("probe-7")
            << ", false positives on 1000 absent keys: " << false_positives << '\n';
    }

    {
        OneShotEvent event;
        int value = 0;
        std::thread waiter([&] {
            event.wait();
            value *= 2;
        });
        value = 21;
        event.set();
        waiter.join();
        out << "std::atomic::wait/notify event delivered value " << value << '\n';
    }
    out << '\n';
}

} // namespace CppVerseHub::Concurrency
