/**
 * @file SingletonTests.cpp
 * @brief Tests demonstrating thread-safe, exactly-once construction of Meyers singletons.
 */

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <set>
#include <thread>
#include <type_traits>
#include <vector>

#include "patterns/Singleton.hpp"

using namespace CppVerseHub::Patterns;

namespace {
std::atomic<int> g_constructions{0};

/// Deliberately slow constructor to widen the race window.
class SlowService final : public Singleton<SlowService> {
    friend class Singleton<SlowService>;
    SlowService() {
        g_constructions.fetch_add(1);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        ready = true;
    }

public:
    bool ready = false;
};
}  // namespace

TEST_CASE("Singleton is not copyable, movable or publicly constructible", "[singleton]") {
    STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<ConfigManager>);
    STATIC_REQUIRE_FALSE(std::is_move_constructible_v<ConfigManager>);
    STATIC_REQUIRE_FALSE(std::is_copy_assignable_v<ConfigManager>);
    STATIC_REQUIRE_FALSE(std::is_default_constructible_v<ConfigManager>);
    STATIC_REQUIRE_FALSE(std::is_default_constructible_v<IdGenerator>);
}

TEST_CASE("Concurrent first access constructs exactly once and yields one address", "[singleton][threads]") {
    constexpr int kThreads = 16;
    std::vector<SlowService*> seen(kThreads, nullptr);
    std::vector<int> readyFlags(kThreads, 0);
    std::atomic<bool> go{false};
    std::vector<std::thread> threads;
    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&, i] {
            while (!go.load()) {
                std::this_thread::yield();
            }
            auto& inst = SlowService::instance();
            seen[static_cast<std::size_t>(i)] = &inst;
            readyFlags[static_cast<std::size_t>(i)] = inst.ready ? 1 : 0;  // never observe a half-built object
        });
    }
    go.store(true);
    for (auto& t : threads) {
        t.join();
    }
    CHECK(g_constructions.load() == 1);
    CHECK(std::all_of(seen.begin(), seen.end(), [&](SlowService* p) { return p == seen.front(); }));
    CHECK(std::all_of(readyFlags.begin(), readyFlags.end(), [](int r) { return r == 1; }));
}

TEST_CASE("ConfigManager stores, overwrites and resets values", "[singleton][config]") {
    auto& config = ConfigManager::instance();
    config.reset();
    CHECK(config.size() == 0);
    CHECK_FALSE(config.get("k").has_value());
    CHECK(config.getOr("k", "fallback") == "fallback");
    config.set("k", "v1");
    config.set("k", "v2");
    CHECK(ConfigManager::instance().get("k") == "v2");
    CHECK(config.contains("k"));
    CHECK(config.size() == 1);
    config.reset();
    CHECK_FALSE(config.contains("k"));
}

TEST_CASE("ConfigManager tolerates concurrent readers and writers", "[singleton][config][threads]") {
    auto& config = ConfigManager::instance();
    config.reset();
    std::vector<std::thread> threads;
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([t, &config] {
            for (int i = 0; i < 200; ++i) {
                config.set("key" + std::to_string(t) + "_" + std::to_string(i), std::to_string(i));
                (void)config.get("key0_0");
            }
        });
    }
    for (auto& th : threads) {
        th.join();
    }
    CHECK(config.size() == 800);
    CHECK(config.get("key3_199") == "199");
    config.reset();
}

TEST_CASE("IdGenerator issues unique ids across threads", "[singleton][ids][threads]") {
    auto& gen = IdGenerator::instance();
    const auto before = gen.issued();
    constexpr int kThreads = 8;
    constexpr int kPerThread = 1000;
    std::vector<std::vector<std::uint64_t>> ids(kThreads);
    std::vector<std::thread> threads;
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t] {
            for (int i = 0; i < kPerThread; ++i) {
                ids[static_cast<std::size_t>(t)].push_back(gen.next());
            }
        });
    }
    for (auto& th : threads) {
        th.join();
    }
    std::set<std::uint64_t> unique;
    for (const auto& v : ids) {
        unique.insert(v.begin(), v.end());
        CHECK(std::is_sorted(v.begin(), v.end()));  // monotonic per thread
    }
    CHECK(unique.size() == static_cast<std::size_t>(kThreads * kPerThread));
    CHECK(gen.issued() - before == static_cast<std::uint64_t>(kThreads * kPerThread));
}

TEST_CASE("LogManager filters by level and bounds its buffer", "[singleton][log]") {
    auto& log = LogManager::instance();
    log.reset();
    log.setMinLevel(LogLevel::Warning);
    log.log(LogLevel::Info, "ignored");
    log.log(LogLevel::Error, "kept");
    REQUIRE(log.size() == 1);
    CHECK(log.records().front().message == "kept");
    log.reset();
    for (std::size_t i = 0; i < LogManager::kCapacity + 10; ++i) {
        log.log(LogLevel::Debug, std::to_string(i));
    }
    CHECK(log.size() == LogManager::kCapacity);
    CHECK(log.records().front().message == "10");
    log.reset();
}
