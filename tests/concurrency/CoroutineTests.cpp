// Behavioural tests for CoroutinesDemo.hpp (Generator, Task, sync_wait, schedulers).

#include "concurrency/CoroutinesDemo.hpp"

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <memory>
#include <ranges>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace CppVerseHub::Concurrency;

namespace {

struct LifetimeProbe {
    explicit LifetimeProbe(int& counter) : counter_(&counter) { ++*counter_; }
    LifetimeProbe(const LifetimeProbe&) = delete;
    LifetimeProbe& operator=(const LifetimeProbe&) = delete;
    LifetimeProbe(LifetimeProbe&&) = delete;
    LifetimeProbe& operator=(LifetimeProbe&&) = delete;
    ~LifetimeProbe() { --*counter_; }
    int* counter_;
};

Generator<int> counting_with_probe(int& alive, bool& started) {
    started = true;
    LifetimeProbe probe(alive);
    for (int i = 0;; ++i) {
        co_yield i;
    }
}

Generator<int> throws_after(int n) {
    for (int i = 0; i < n; ++i) {
        co_yield i;
    }
    throw std::runtime_error("generator failure");
}

Generator<std::string> words() {
    co_yield "alpha";
    std::string beta = "beta";
    co_yield beta;
    co_yield std::string("gamma");
}

Generator<int> empty_generator() {
    co_return;
}

Task<int> constant(int v) {
    co_return v;
}

Task<int> add_async(int a, int b) {
    const int x = co_await constant(a);
    const int y = co_await constant(b);
    co_return x + y;
}

Task<void> set_flag(bool& flag) {
    flag = true;
    co_return;
}

Task<int> thrower() {
    throw std::logic_error("task failure");
    co_return 0;
}

Task<int> catches_inner() {
    try {
        co_return co_await thrower();
    } catch (const std::logic_error&) {
        co_return -1;
    }
}

Task<std::unique_ptr<int>> move_only_result() {
    co_return std::make_unique<int>(9);
}

Task<long long> deep_chain(int depth) {
    if (depth == 0) {
        co_return 0;
    }
    co_return 1 + co_await deep_chain(depth - 1);
}

Task<std::thread::id> resume_on(ThreadPool& pool) {
    co_await schedule_on(pool);
    co_return std::this_thread::get_id();
}

Task<int> pool_sum(ThreadPool& pool, int n) {
    int total = 0;
    for (int i = 1; i <= n; ++i) {
        co_await schedule_on(pool); // each iteration may continue on another worker
        total += i;
    }
    co_return total;
}

Task<void> logger(RoundRobinScheduler& s, char id, int steps, std::string& log) {
    for (int i = 0; i < steps; ++i) {
        log.push_back(id);
        co_await s.yield();
    }
}

Task<void> failing_task(RoundRobinScheduler& s) {
    co_await s.yield();
    throw std::runtime_error("boom");
}

Task<void> probe_task(RoundRobinScheduler& s, int& alive) {
    LifetimeProbe probe(alive);
    co_await s.yield();
}

} // namespace

TEST_CASE("Generator models std::ranges::input_range", "[concurrency][coroutines]") {
    STATIC_REQUIRE(std::ranges::input_range<Generator<int>>);
    STATIC_REQUIRE(std::input_iterator<Generator<int>::iterator>);
    STATIC_REQUIRE(std::sentinel_for<std::default_sentinel_t, Generator<int>::iterator>);
}

TEST_CASE("Generator yields lvalues, rvalues and temporaries", "[concurrency][coroutines]") {
    std::vector<std::string> got;
    for (const auto& w : words()) {
        got.push_back(w);
    }
    CHECK(got == std::vector<std::string>{"alpha", "beta", "gamma"});

    std::vector<int> range;
    for (int v : iota_range(3, 8)) {
        range.push_back(v);
    }
    CHECK(range == std::vector<int>{3, 4, 5, 6, 7});

    int count = 0;
    for (int v : empty_generator()) {
        static_cast<void>(v);
        ++count;
    }
    CHECK(count == 0);
    for (int v : iota_range(5, 5)) {
        static_cast<void>(v);
        ++count;
    }
    CHECK(count == 0);
}

TEST_CASE("Generator is lazy and destroys its frame early", "[concurrency][coroutines]") {
    int alive = 0;
    bool started = false;
    {
        auto gen = counting_with_probe(alive, started);
        CHECK_FALSE(started); // nothing runs until begin()
        int sum = 0;
        for (int v : gen) {
            if (v == 5) {
                break;
            }
            sum += v;
        }
        CHECK(started);
        CHECK(sum == 10);
        CHECK(alive == 1); // suspended mid-body: the local is still alive
    }
    CHECK(alive == 0); // destroying the generator ran the local's destructor
}

TEST_CASE("Generator re-throws exceptions from its body", "[concurrency][coroutines]") {
    std::vector<int> got;
    auto consume = [&] {
        for (int v : throws_after(3)) {
            got.push_back(v);
        }
    };
    CHECK_THROWS_AS(consume(), std::runtime_error);
    CHECK(got == std::vector<int>{0, 1, 2});

    auto bad = collatz(0);
    CHECK_THROWS_AS(bad.begin(), std::invalid_argument);
}

TEST_CASE("Generator move semantics", "[concurrency][coroutines]") {
    auto a = iota_range(0, 3);
    auto b = std::move(a);
    std::vector<int> got;
    for (int v : b) {
        got.push_back(v);
    }
    CHECK(got == std::vector<int>{0, 1, 2});
    a = iota_range(10, 12); // moved-from object can be re-assigned
    got.clear();
    for (int v : a) {
        got.push_back(v);
    }
    CHECK(got == std::vector<int>{10, 11});
}

TEST_CASE("Generator combinators and sequences", "[concurrency][coroutines]") {
    std::vector<std::uint64_t> fib;
    for (auto v : take(fibonacci(), 12)) {
        fib.push_back(v);
    }
    CHECK(fib == std::vector<std::uint64_t>{0, 1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89});

    std::uint64_t last = 0;
    std::size_t n = 0;
    for (auto v : fibonacci()) {
        last = v;
        ++n;
    }
    CHECK(n == 94); // F(0)..F(93) fit in 64 bits
    CHECK(last == 12200160415121876738ULL);

    std::vector<std::uint64_t> c;
    for (auto v : collatz(6)) {
        c.push_back(v);
    }
    CHECK(c == std::vector<std::uint64_t>{6, 3, 10, 5, 16, 8, 4, 2, 1});

    std::vector<int> evens;
    for (int v : filter(iota_range(0, 20), [](int x) { return x % 3 == 0; })) {
        evens.push_back(v);
    }
    CHECK(evens == std::vector<int>{0, 3, 6, 9, 12, 15, 18});

    int none = 0;
    for (int v : take(iota_range(0, 100), 0)) {
        none += v + 1;
    }
    CHECK(none == 0);

    std::vector<int> short_take;
    for (int v : take(iota_range(0, 2), 10)) {
        short_take.push_back(v);
    }
    CHECK(short_take == std::vector<int>{0, 1});
}

TEST_CASE("Generator works with std::views", "[concurrency][coroutines]") {
    std::vector<int> got;
    for (int v : iota_range(0, 10) | std::views::transform([](int x) { return x * x; })) {
        got.push_back(v);
    }
    CHECK(got == std::vector<int>{0, 1, 4, 9, 16, 25, 36, 49, 64, 81});
}

TEST_CASE("Task composes and sync_wait returns the result", "[concurrency][coroutines]") {
    CHECK(sync_wait(constant(5)) == 5);
    CHECK(sync_wait(add_async(20, 22)) == 42);
    auto p = sync_wait(move_only_result());
    REQUIRE(p != nullptr);
    CHECK(*p == 9);
    bool flag = false;
    sync_wait(set_flag(flag));
    CHECK(flag);
}

TEST_CASE("Task is lazy until awaited", "[concurrency][coroutines]") {
    bool flag = false;
    auto task = set_flag(flag);
    CHECK_FALSE(flag);
    CHECK_FALSE(task.done());
    sync_wait(std::move(task));
    CHECK(flag);
}

TEST_CASE("Task propagates exceptions to the awaiter", "[concurrency][coroutines]") {
    CHECK_THROWS_AS(sync_wait(thrower()), std::logic_error);
    CHECK(sync_wait(catches_inner()) == -1);
}

TEST_CASE("Task symmetric transfer supports deep await chains", "[concurrency][coroutines]") {
    CHECK(sync_wait(deep_chain(5000)) == 5000);
}

TEST_CASE("schedule_on resumes coroutines on pool threads", "[concurrency][coroutines]") {
    ThreadPool pool(2);
    const auto worker_id = sync_wait(resume_on(pool));
    CHECK(worker_id != std::this_thread::get_id());
    CHECK(sync_wait(pool_sum(pool, 100)) == 5050);

    // Several sync_waits from different threads concurrently.
    std::vector<std::thread> callers;
    std::vector<int> results(4, 0);
    for (std::size_t i = 0; i < results.size(); ++i) {
        callers.emplace_back([&pool, &results, i] { results[i] = sync_wait(pool_sum(pool, 50)); });
    }
    for (auto& c : callers) {
        c.join();
    }
    CHECK(results == std::vector<int>(4, 1275));

    pool.shutdown();
    CHECK_THROWS_AS(sync_wait(resume_on(pool)), PoolShutdownError);
}

TEST_CASE("RoundRobinScheduler interleaves tasks deterministically", "[concurrency][coroutines]") {
    RoundRobinScheduler scheduler;
    std::string log;
    scheduler.spawn(logger(scheduler, 'a', 3, log));
    scheduler.spawn(logger(scheduler, 'b', 2, log));
    scheduler.spawn(logger(scheduler, 'c', 1, log));
    CHECK(log.empty()); // nothing runs before run()
    CHECK(scheduler.live_tasks() == 3);
    const std::size_t resumptions = scheduler.run();
    CHECK(log == "abcaba");
    CHECK(resumptions == 3 + 6);
    CHECK(scheduler.live_tasks() == 0);
    CHECK(scheduler.failed_tasks() == 0);
}

TEST_CASE("RoundRobinScheduler counts failures and cleans up unfinished tasks", "[concurrency][coroutines]") {
    {
        RoundRobinScheduler scheduler;
        scheduler.spawn(failing_task(scheduler));
        std::string log;
        scheduler.spawn(logger(scheduler, 'x', 2, log));
        static_cast<void>(scheduler.run());
        CHECK(scheduler.failed_tasks() == 1);
        CHECK(log == "xx");
    }
    int alive = 0;
    {
        RoundRobinScheduler scheduler;
        scheduler.spawn(probe_task(scheduler, alive));
        CHECK(alive == 0); // not started
    }
    CHECK(alive == 0);
    {
        RoundRobinScheduler scheduler;
        scheduler.spawn(probe_task(scheduler, alive));
        CHECK(scheduler.run() == 2);
        CHECK(alive == 0);
    }
}
