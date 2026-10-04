// Behavioural tests for ThreadPool, PriorityThreadPool, WorkStealingThreadPool and UniqueTask.
// No test depends on timing: ordering is forced with promises/gates, never with sleeps.

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <atomic>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <numeric>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "concurrency/ThreadPool.hpp"

using namespace CppVerseHub::Concurrency;

namespace {

    // Blocks a worker until opened; lets tests fill a queue deterministically.
    struct Gate {
        std::promise<void> promise;
        std::shared_future<void> future = promise.get_future().share();
        void open() { promise.set_value(); }
        [[nodiscard]] auto waiter() const {
            return [f = future] { f.wait(); };
        }
    };

    void spin_until(const std::function<bool()>& predicate) {
        while (!predicate()) {
            std::this_thread::yield();
        }
    }

}  // namespace

TEST_CASE("UniqueTask wraps move-only callables", "[concurrency][threadpool]") {
    UniqueTask empty;
    CHECK_FALSE(static_cast<bool>(empty));

    auto value = std::make_unique<int>(41);
    int observed = 0;
    UniqueTask task([p = std::move(value), &observed] { observed = *p + 1; });
    REQUIRE(static_cast<bool>(task));
    UniqueTask moved = std::move(task);
    moved();
    CHECK(observed == 42);
}

TEST_CASE("ThreadPool submit returns results through futures", "[concurrency][threadpool]") {
    const std::size_t threads = GENERATE(1U, 2U, 4U);
    ThreadPool pool(threads);
    REQUIRE(pool.thread_count() == threads);

    std::vector<std::future<int>> futures;
    for (int i = 0; i < 64; ++i) {
        futures.push_back(pool.submit([](int a, int b) { return a * b; }, i, 3));
    }
    for (int i = 0; i < 64; ++i) {
        CHECK(futures[static_cast<std::size_t>(i)].get() == 3 * i);
    }
}

TEST_CASE("ThreadPool accepts move-only arguments and void tasks", "[concurrency][threadpool]") {
    ThreadPool pool(2);
    auto f = pool.submit([](std::unique_ptr<std::string> s) { return *s + "!"; },
                         std::make_unique<std::string>("liftoff"));
    CHECK(f.get() == "liftoff!");

    std::atomic<int> flag{0};
    auto v = pool.submit([&flag] { flag.store(7); });
    v.get();
    CHECK(flag.load() == 7);
}

TEST_CASE("ThreadPool propagates exceptions through the future", "[concurrency][threadpool]") {
    ThreadPool pool(2);
    auto f = pool.submit([]() -> int { throw std::domain_error("bad orbit"); });
    CHECK_THROWS_AS(f.get(), std::domain_error);
    // The worker survives the exception.
    CHECK(pool.submit([] { return 5; }).get() == 5);
}

TEST_CASE("ThreadPool zero threads means one worker", "[concurrency][threadpool]") {
    ThreadPool pool(0);
    CHECK(pool.thread_count() == 1);
    CHECK(pool.submit([] { return 1; }).get() == 1);
}

TEST_CASE("ThreadPool pending_tasks counts queued work behind a blocked worker", "[concurrency][threadpool]") {
    ThreadPool pool(1);
    Gate gate;
    std::atomic<bool> started{false};
    auto blocker = pool.submit([&started, w = gate.waiter()] {
        started.store(true);
        w();
    });
    spin_until([&] { return started.load(); });
    std::vector<std::future<void>> queued;
    for (int i = 0; i < 5; ++i) {
        queued.push_back(pool.submit([] {}));
    }
    CHECK(pool.pending_tasks() == 5);
    gate.open();
    blocker.get();
    for (auto& q : queued) {
        q.get();
    }
    pool.wait_idle();
    CHECK(pool.pending_tasks() == 0);
    CHECK(pool.completed_tasks() == 6);
}

TEST_CASE("ThreadPool wait_idle waits for all submitted work", "[concurrency][threadpool]") {
    ThreadPool pool(4);
    std::atomic<int> counter{0};
    for (int i = 0; i < 500; ++i) {
        pool.post(UniqueTask([&counter] { counter.fetch_add(1); }));
    }
    pool.wait_idle();
    CHECK(counter.load() == 500);
    CHECK(pool.pending_tasks() == 0);
}

TEST_CASE("ThreadPool graceful shutdown executes every accepted task", "[concurrency][threadpool]") {
    ThreadPool pool(1);
    Gate gate;
    std::atomic<bool> started{false};
    std::atomic<int> executed{0};
    pool.post(UniqueTask([&started, w = gate.waiter()] {
        started.store(true);
        w();
    }));
    spin_until([&] { return started.load(); });
    for (int i = 0; i < 50; ++i) {
        pool.post(UniqueTask([&executed] { executed.fetch_add(1); }));
    }
    std::thread closer([&pool] { pool.shutdown(); });
    spin_until([&] { return pool.is_shutdown(); });
    CHECK_THROWS_AS(pool.submit([] {}), PoolShutdownError);
    gate.open();
    closer.join();
    CHECK(executed.load() == 50);
}

TEST_CASE("ThreadPool rejects work after shutdown and shutdown is idempotent", "[concurrency][threadpool]") {
    ThreadPool pool(2);
    pool.shutdown();
    pool.shutdown();
    CHECK(pool.is_shutdown());
    CHECK_THROWS_AS(pool.submit([] { return 1; }), PoolShutdownError);
    CHECK_THROWS_AS(pool.post(UniqueTask([] {})), PoolShutdownError);
}

TEST_CASE("ThreadPool loses no task when submitters race with shutdown", "[concurrency][threadpool]") {
    std::atomic<int> accepted{0};
    std::atomic<int> executed{0};
    {
        ThreadPool pool(3);
        std::atomic<bool> go{false};
        std::vector<std::thread> submitters;
        for (int t = 0; t < 4; ++t) {
            submitters.emplace_back([&] {
                spin_until([&] { return go.load(); });
                for (int i = 0; i < 500; ++i) {
                    try {
                        pool.post(UniqueTask([&executed] { executed.fetch_add(1); }));
                        accepted.fetch_add(1);
                    } catch (const PoolShutdownError&) {
                        return;
                    }
                }
            });
        }
        go.store(true);
        pool.shutdown();
        for (auto& s : submitters) {
            s.join();
        }
    }
    CHECK(executed.load() == accepted.load());
}

TEST_CASE("ThreadPool supports concurrent submitters", "[concurrency][threadpool]") {
    ThreadPool pool(4);
    std::vector<std::thread> clients;
    std::vector<long long> sums(4, 0);
    for (std::size_t c = 0; c < 4; ++c) {
        clients.emplace_back([&pool, &sums, c] {
            std::vector<std::future<int>> fs;
            for (int i = 1; i <= 100; ++i) {
                fs.push_back(pool.submit([i] { return i; }));
            }
            for (auto& f : fs) {
                sums[c] += f.get();
            }
        });
    }
    for (auto& c : clients) {
        c.join();
    }
    CHECK(std::accumulate(sums.begin(), sums.end(), 0LL) == 4 * 5050);
}

TEST_CASE("PriorityThreadPool runs higher priorities first, FIFO within a priority", "[concurrency][threadpool]") {
    PriorityThreadPool pool(1);
    Gate gate;
    std::atomic<bool> started{false};
    auto blocker = pool.submit(TaskPriority::Critical, [&started, w = gate.waiter()] {
        started.store(true);
        w();
    });
    spin_until([&] { return started.load(); });

    std::mutex m;
    std::vector<std::string> order;
    auto record = [&](std::string s) {
        std::lock_guard lock(m);
        order.push_back(std::move(s));
    };
    std::vector<std::future<void>> fs;
    fs.push_back(pool.submit(TaskPriority::Low, record, std::string("low-1")));
    fs.push_back(pool.submit(TaskPriority::High, record, std::string("high-1")));
    fs.push_back(pool.submit(TaskPriority::Normal, record, std::string("normal-1")));
    fs.push_back(pool.submit(TaskPriority::High, record, std::string("high-2")));
    fs.push_back(pool.submit(TaskPriority::Critical, record, std::string("critical-1")));
    fs.push_back(pool.submit(TaskPriority::Low, record, std::string("low-2")));
    CHECK(pool.pending_tasks() == 6);
    gate.open();
    blocker.get();
    for (auto& f : fs) {
        f.get();
    }
    const std::vector<std::string> expected{"critical-1", "high-1", "high-2", "normal-1", "low-1", "low-2"};
    CHECK(order == expected);
}

TEST_CASE("PriorityThreadPool shutdown drains and then rejects", "[concurrency][threadpool]") {
    std::atomic<int> executed{0};
    PriorityThreadPool pool(2);
    for (int i = 0; i < 100; ++i) {
        static_cast<void>(pool.submit(i % 2 == 0 ? TaskPriority::Low : TaskPriority::High,
                                      [&executed] { executed.fetch_add(1); }));
    }
    pool.shutdown();
    CHECK(executed.load() == 100);
    CHECK(pool.completed_tasks() == 100);
    CHECK_THROWS_AS(pool.submit(TaskPriority::Normal, [] {}), PoolShutdownError);
}

TEST_CASE("WorkStealingThreadPool computes correct results", "[concurrency][threadpool][workstealing]") {
    const std::size_t threads = GENERATE(1U, 3U, 8U);
    WorkStealingThreadPool pool(threads);
    REQUIRE(pool.thread_count() == threads);
    std::vector<std::future<std::uint64_t>> fs;
    for (std::uint64_t i = 0; i < 1000; ++i) {
        fs.push_back(pool.submit([i] { return i * i; }));
    }
    std::uint64_t total = 0;
    for (auto& f : fs) {
        total += f.get();
    }
    CHECK(total == 332833500ULL);  // sum of i^2 for i < 1000
}

TEST_CASE("WorkStealingThreadPool supports tasks spawning subtasks", "[concurrency][threadpool][workstealing]") {
    WorkStealingThreadPool pool(4);
    std::vector<std::future<std::vector<std::future<int>>>> parents;
    for (int p = 0; p < 8; ++p) {
        parents.push_back(pool.submit([&pool, p] {
            std::vector<std::future<int>> children;
            for (int c = 0; c < 16; ++c) {
                children.push_back(pool.submit([p, c] { return p * 100 + c; }));
            }
            return children;
        }));
    }
    long long total = 0;
    for (auto& parent : parents) {
        for (auto& child : parent.get()) {
            total += child.get();
        }
    }
    // sum over p<8, c<16 of (100p + c) = 100*16*28 + 8*120
    CHECK(total == 100LL * 16 * 28 + 8 * 120);
    CHECK(pool.completed_tasks() == 8 + 8 * 16);
}

TEST_CASE("WorkStealingThreadPool propagates exceptions", "[concurrency][threadpool][workstealing]") {
    WorkStealingThreadPool pool(2);
    auto f = pool.submit([]() -> int { throw std::runtime_error("hull breach"); });
    CHECK_THROWS_AS(f.get(), std::runtime_error);
}

TEST_CASE("WorkStealingThreadPool shutdown executes all accepted tasks", "[concurrency][threadpool][workstealing]") {
    std::atomic<int> executed{0};
    WorkStealingThreadPool pool(4);
    for (int i = 0; i < 2000; ++i) {
        static_cast<void>(pool.submit([&executed] { executed.fetch_add(1); }));
    }
    pool.shutdown();
    CHECK(executed.load() == 2000);
    CHECK(pool.pending_tasks() == 0);
    CHECK_THROWS_AS(pool.submit([] {}), PoolShutdownError);
    pool.shutdown();  // idempotent
}

TEST_CASE("WorkStealingThreadPool loses no task when submitters race with shutdown",
          "[concurrency][threadpool][workstealing]") {
    std::atomic<int> accepted{0};
    std::atomic<int> executed{0};
    {
        WorkStealingThreadPool pool(3);
        std::atomic<bool> go{false};
        std::vector<std::thread> submitters;
        for (int t = 0; t < 4; ++t) {
            submitters.emplace_back([&] {
                spin_until([&] { return go.load(); });
                for (int i = 0; i < 500; ++i) {
                    try {
                        static_cast<void>(pool.submit([&executed] { executed.fetch_add(1); }));
                        accepted.fetch_add(1);
                    } catch (const PoolShutdownError&) {
                        return;
                    }
                }
            });
        }
        go.store(true);
        pool.shutdown();
        for (auto& s : submitters) {
            s.join();
        }
    }
    CHECK(executed.load() == accepted.load());
}

TEST_CASE("WorkStealingThreadPool idle workers steal from a busy worker", "[concurrency][threadpool][workstealing]") {
    WorkStealingThreadPool pool(2);
    // One task (on some worker) spawns many children onto its own deque and then blocks
    // until all of them have run. They can only complete if the other worker steals them.
    std::atomic<int> done{0};
    auto parent = pool.submit([&pool, &done] {
        for (int i = 0; i < 32; ++i) {
            static_cast<void>(pool.submit([&done] { done.fetch_add(1); }));
        }
        spin_until([&] { return done.load() == 32; });
    });
    parent.get();
    CHECK(done.load() == 32);
    CHECK(pool.steal_count() >= 32);
}
