/**
 * @file CoroutinesDemo.cpp
 * @brief Generators, the round-robin scheduler driver and the coroutine showcase.
 * @details File location: src/concurrency/CoroutinesDemo.cpp
 */

#include "concurrency/CoroutinesDemo.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <string>
#include <thread>

namespace CppVerseHub::Concurrency {

    // ---------------------------------------------------------------- generators

    Generator<int> iota_range(int first, int last) {
        for (int i = first; i < last; ++i) {
            co_yield i;
        }
    }

    Generator<std::uint64_t> fibonacci() {
        std::uint64_t a = 0;
        std::uint64_t b = 1;
        for (;;) {
            co_yield a;
            if (b > std::numeric_limits<std::uint64_t>::max() - a) {
                co_yield b;
                co_return;  // next term would overflow
            }
            const std::uint64_t next = a + b;
            a = b;
            b = next;
        }
    }

    Generator<std::uint64_t> collatz(std::uint64_t start) {
        if (start == 0) {
            throw std::invalid_argument("collatz is undefined for 0");
        }
        std::uint64_t n = start;
        co_yield n;
        while (n != 1) {
            n = (n % 2 == 0) ? n / 2 : 3 * n + 1;
            co_yield n;
        }
    }

    // ---------------------------------------------------------------- scheduler

    namespace {

        /** Eager-on-resume coroutine owned by the scheduler that runs one spawned task. */
        class Driver {
        public:
            struct promise_type {
                Driver get_return_object() noexcept {
                    return Driver{std::coroutine_handle<promise_type>::from_promise(*this)};
                }
                std::suspend_always initial_suspend() const noexcept { return {}; }
                std::suspend_always final_suspend() const noexcept { return {}; }
                void return_void() const noexcept {}
                void unhandled_exception() const noexcept { std::terminate(); }  // body catches all
            };

            Driver(const Driver&) = delete;
            Driver& operator=(const Driver&) = delete;
            Driver(Driver&& other) noexcept : handle_(std::exchange(other.handle_, {})) {}
            Driver& operator=(Driver&&) = delete;
            ~Driver() {
                if (handle_) {
                    handle_.destroy();
                }
            }
            [[nodiscard]] std::coroutine_handle<> release() noexcept { return std::exchange(handle_, {}); }

        private:
            explicit Driver(std::coroutine_handle<promise_type> h) noexcept : handle_(h) {}
            std::coroutine_handle<promise_type> handle_;
        };

        Driver drive(Task<void> task, std::size_t* failures) {
            try {
                co_await std::move(task);
            } catch (...) {
                ++*failures;
            }
        }

    }  // namespace

    RoundRobinScheduler::~RoundRobinScheduler() {
        for (auto handle : drivers_) {
            handle.destroy();
        }
    }

    void RoundRobinScheduler::spawn(Task<void> task) {
        Driver driver = drive(std::move(task), &failures_);
        drivers_.reserve(drivers_.size() + 1);
        ready_.push_back(std::coroutine_handle<>{});  // reserve the slot before releasing ownership
        const std::coroutine_handle<> handle = driver.release();
        drivers_.push_back(handle);
        ready_.back() = handle;
    }

    std::size_t RoundRobinScheduler::run() {
        std::size_t resumptions = 0;
        while (!ready_.empty()) {
            const std::coroutine_handle<> next = ready_.front();
            ready_.pop_front();
            next.resume();
            ++resumptions;
        }
        // Reclaim finished drivers.
        const auto finished = std::remove_if(drivers_.begin(), drivers_.end(), [](std::coroutine_handle<> h) {
            if (h.done()) {
                h.destroy();
                return true;
            }
            return false;
        });
        drivers_.erase(finished, drivers_.end());
        return resumptions;
    }

    // ---------------------------------------------------------------- showcase

    namespace {

        Task<int> square_async(int x) { co_return x * x; }

        Task<int> sum_of_squares_async(int n) {
            int total = 0;
            for (int i = 1; i <= n; ++i) {
                total += co_await square_async(i);
            }
            co_return total;
        }

        Task<std::string> failing_async() {
            throw std::runtime_error("telemetry decode failed");
            co_return std::string{};  // unreachable; makes this a coroutine
        }

        Task<bool> hop_to_pool(ThreadPool& pool, std::thread::id caller) {
            co_await schedule_on(pool);
            co_return std::this_thread::get_id() != caller;
        }

        Task<void> chatter(RoundRobinScheduler& scheduler, std::string name, int turns, std::string& log) {
            for (int i = 0; i < turns; ++i) {
                log += name + std::to_string(i) + ' ';
                co_await scheduler.yield();
            }
        }

    }  // namespace

    void demonstrate_coroutines(std::ostream& out) {
        out << "=== C++20 coroutines ===\n";

        out << "Generator iota_range(1, 6):";
        for (int v : iota_range(1, 6)) {
            out << ' ' << v;
        }
        out << "\nFirst 10 Fibonacci numbers:";
        for (std::uint64_t v : take(fibonacci(), 10)) {
            out << ' ' << v;
        }
        std::size_t fib_count = 0;
        for (std::uint64_t v : fibonacci()) {
            static_cast<void>(v);
            ++fib_count;
        }
        out << "\nFibonacci terms representable in 64 bits: " << fib_count;
        out << "\nEven terms of collatz(27), first 8:";
        for (std::uint64_t v : take(filter(collatz(27), [](std::uint64_t x) { return x % 2 == 0; }), 8)) {
            out << ' ' << v;
        }
        std::size_t steps = 0;
        for (std::uint64_t v : collatz(27)) {
            static_cast<void>(v);
            ++steps;
        }
        out << "\ncollatz(27) trajectory length: " << steps << '\n';

        out << "Task composition: sum of squares 1..10 = " << sync_wait(sum_of_squares_async(10)) << '\n';
        try {
            static_cast<void>(sync_wait(failing_async()));
        } catch (const std::exception& e) {
            out << "Task exception re-thrown by sync_wait: " << e.what() << '\n';
        }

        {
            ThreadPool pool(2);
            const bool hopped = sync_wait(hop_to_pool(pool, std::this_thread::get_id()));
            out << "co_await schedule_on(pool) resumed on a worker thread: " << std::boolalpha << hopped << '\n';
        }

        {
            RoundRobinScheduler scheduler;
            std::string log;
            scheduler.spawn(chatter(scheduler, "A", 3, log));
            scheduler.spawn(chatter(scheduler, "B", 3, log));
            const std::size_t resumptions = scheduler.run();
            out << "Round-robin interleaving: " << log << "(" << resumptions << " resumptions)\n";
        }
        out << '\n';
    }

}  // namespace CppVerseHub::Concurrency
