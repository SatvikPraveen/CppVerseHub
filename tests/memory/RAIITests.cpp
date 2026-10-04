/**
 * @file RAIITests.cpp
 * @brief Tests for the RAII wrappers and scope guards.
 */

#include "memory/RAII_Examples.hpp"

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace CppVerseHub::Memory;

namespace {
std::filesystem::path temp_file(const std::string& name) {
    return std::filesystem::temp_directory_path() / ("cppversehub_memory_tests_" + name);
}

struct RemoveOnExit {
    std::filesystem::path path;
    RemoveOnExit(const RemoveOnExit&) = delete;
    RemoveOnExit& operator=(const RemoveOnExit&) = delete;
    explicit RemoveOnExit(std::filesystem::path p) : path(std::move(p)) {}
    ~RemoveOnExit() {
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }
};

struct CountingMutex {
    int locks = 0;
    int unlocks = 0;
    void lock() { ++locks; }
    void unlock() { ++unlocks; }
};

struct Tracked {
    static inline int live = 0;
    static inline int copies_until_throw = -1;
    int value = 0;
    Tracked() { ++live; }
    explicit Tracked(int v) : value(v) { ++live; }
    Tracked(const Tracked& other) : value(other.value) {
        if (copies_until_throw == 0) {
            throw std::runtime_error("copy failed");
        }
        if (copies_until_throw > 0) {
            --copies_until_throw;
        }
        ++live;
    }
    Tracked& operator=(const Tracked&) = default;
    ~Tracked() { --live; }
};

struct Widget {
    static inline int created = 0;
    int uses = 0;
    Widget() { ++created; }
};
} // namespace

TEST_CASE("UniqueHandle owns, moves, releases and resets", "[memory][raii][handle]") {
    const int before = MockOsHandleTraits::open_count();
    {
        MockOsHandle empty;
        CHECK_FALSE(empty);
        CHECK(empty.get() == MockOsHandleTraits::invalid());

        MockOsHandle a(MockOsHandleTraits::open());
        CHECK(a.valid());
        CHECK(MockOsHandleTraits::open_count() == before + 1);

        MockOsHandle b(std::move(a));
        CHECK_FALSE(a.valid()); // NOLINT(bugprone-use-after-move)
        CHECK(b.valid());
        CHECK(MockOsHandleTraits::open_count() == before + 1);

        MockOsHandle c(MockOsHandleTraits::open());
        c = std::move(b); // c's original handle closed
        CHECK(MockOsHandleTraits::open_count() == before + 1);

        const int raw = c.release();
        CHECK_FALSE(c.valid());
        CHECK(MockOsHandleTraits::open_count() == before + 1);
        c.reset(raw); // re-adopt
        c.reset();
        CHECK(MockOsHandleTraits::open_count() == before);
        c.reset(MockOsHandleTraits::open());
    }
    CHECK(MockOsHandleTraits::open_count() == before);
}

TEST_CASE("FileRAII writes, reads and closes", "[memory][raii][file]") {
    const RemoveOnExit guard(temp_file("file.txt"));
    {
        FileRAII file(guard.path, "w+");
        CHECK(file.is_open());
        CHECK(file.path() == guard.path);
        CHECK(file.write("line one\n"));
        CHECK(file.write("line two\n"));
        CHECK(file.flush());
        CHECK(file.read_all() == "line one\nline two\n");
        CHECK(file.close());
        CHECK_FALSE(file.is_open());
        CHECK(file.close()); // idempotent
        CHECK_FALSE(file.write("ignored"));
        CHECK_FALSE(file.flush());
        CHECK_THROWS_AS(file.read_all(), std::runtime_error);
    }
    FileRAII reopened(guard.path, "r");
    CHECK(reopened.read_all() == "line one\nline two\n");
}

TEST_CASE("FileRAII is movable and throws on open failure", "[memory][raii][file]") {
    const RemoveOnExit guard(temp_file("move.txt"));
    FileRAII a(guard.path, "w+");
    static_cast<void>(a.write("moved"));
    FileRAII b(std::move(a));
    CHECK(b.is_open());
    CHECK(b.read_all() == "moved");
    const auto missing = std::filesystem::temp_directory_path() / "cppversehub_no_such_dir" / "x.txt";
    CHECK_THROWS_AS(FileRAII(missing, "r"), std::runtime_error);
}

TEST_CASE("TimerRAII reports elapsed time through its callback", "[memory][raii][timer]") {
    std::chrono::nanoseconds reported{-1};
    int calls = 0;
    {
        const TimerRAII timer([&](std::chrono::nanoseconds d) {
            reported = d;
            ++calls;
        });
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        CHECK(timer.elapsed() >= std::chrono::milliseconds(2));
    }
    CHECK(calls == 1);
    CHECK(reported >= std::chrono::milliseconds(2));

    // A throwing callback must not escape the destructor.
    CHECK_NOTHROW(
        [] { const TimerRAII t([](std::chrono::nanoseconds) { throw std::runtime_error("x"); }); }());
    { const TimerRAII silent; }

    std::chrono::nanoseconds measured{};
    const int result = RAIIUtils::measure(measured, [] { return 6 * 7; });
    CHECK(result == 42);
    CHECK(measured.count() >= 0);
}

TEST_CASE("ScopedLock locks, unlocks early and relocks", "[memory][raii][lock]") {
    CountingMutex m;
    {
        ScopedLock lock(m);
        CHECK(lock.owns_lock());
        CHECK(m.locks == 1);
        lock.unlock();
        lock.unlock(); // no double unlock
        CHECK(m.unlocks == 1);
        lock.lock();
        lock.lock(); // no double lock
        CHECK(m.locks == 2);
    }
    CHECK(m.unlocks == 2);
}

TEST_CASE("ScopedLock releases on exception and serialises threads", "[memory][raii][lock]") {
    std::mutex m;
    try {
        const ScopedLock lock(m);
        throw std::runtime_error("boom");
    } catch (const std::runtime_error&) {}
    CHECK(m.try_lock());
    m.unlock();

    int counter = 0;
    std::vector<std::thread> threads;
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([&] {
            for (int i = 0; i < 1000; ++i) {
                const ScopedLock lock(m);
                ++counter;
            }
        });
    }
    for (auto& th : threads) {
        th.join();
    }
    CHECK(counter == 4000);
}

TEST_CASE("ScopeGuard runs always, unless dismissed or moved from", "[memory][raii][guard]") {
    int runs = 0;
    {
        const auto g = make_scope_guard([&runs]() noexcept { ++runs; });
    }
    CHECK(runs == 1);
    {
        auto g = make_scope_guard([&runs]() noexcept { ++runs; });
        CHECK(g.active());
        g.dismiss();
        CHECK_FALSE(g.active());
    }
    CHECK(runs == 1);
    {
        auto g1 = make_scope_guard([&runs]() noexcept { ++runs; });
        auto g2 = std::move(g1);
        CHECK_FALSE(g1.active()); // NOLINT(bugprone-use-after-move)
        CHECK(g2.active());
    }
    CHECK(runs == 2);
    try {
        const auto g = make_scope_guard([&runs]() noexcept { ++runs; });
        throw std::logic_error("x");
    } catch (const std::logic_error&) {}
    CHECK(runs == 3);
}

TEST_CASE("ScopeFail and ScopeSuccess distinguish normal exit from unwinding", "[memory][raii][guard]") {
    std::vector<std::string> log;
    {
        const auto fail = make_scope_fail([&log]() noexcept { log.emplace_back("rollback"); });
        const auto ok = make_scope_success([&log]() noexcept { log.emplace_back("commit"); });
    }
    CHECK(log == std::vector<std::string>{"commit"});
    log.clear();
    try {
        const auto fail = make_scope_fail([&log]() noexcept { log.emplace_back("rollback"); });
        const auto ok = make_scope_success([&log]() noexcept { log.emplace_back("commit"); });
        throw std::runtime_error("failure");
    } catch (const std::runtime_error&) {}
    CHECK(log == std::vector<std::string>{"rollback"});
}

TEST_CASE("ScopeSuccess inside a destructor during unwinding still detects normal exit",
          "[memory][raii][guard]") {
    // uncaught_exceptions() is compared against the count at construction, so a guard
    // created and destroyed entirely within a destructor running during unwinding fires
    // as "success".
    struct Inner {
        std::vector<std::string>* log;
        Inner(const Inner&) = delete;
        Inner& operator=(const Inner&) = delete;
        explicit Inner(std::vector<std::string>* l) : log(l) {}
        ~Inner() {
            const auto ok = make_scope_success([this]() noexcept { log->emplace_back("inner-commit"); });
        }
    };
    std::vector<std::string> log;
    try {
        const Inner inner(&log);
        throw std::runtime_error("outer");
    } catch (const std::runtime_error&) {}
    CHECK(log == std::vector<std::string>{"inner-commit"});
}

TEST_CASE("ResourcePool leases and recycles resources", "[memory][raii][pool]") {
    Widget::created = 0;
    ResourcePool<Widget> pool(2);
    CHECK(pool.idle_count() == 2);
    CHECK(Widget::created == 2);
    Widget* first_address = nullptr;
    {
        auto a = pool.acquire();
        auto b = pool.acquire();
        auto c = pool.acquire();
        CHECK(Widget::created == 3);
        CHECK(pool.leased_count() == 3);
        CHECK(pool.idle_count() == 0);
        a->uses = 5;
        first_address = &*a;
        auto moved = std::move(a);
        CHECK_FALSE(a); // NOLINT(bugprone-use-after-move)
        CHECK(moved->uses == 5);
        b = std::move(moved); // b's previous resource returned
        CHECK(pool.leased_count() == 2);
    }
    CHECK(pool.leased_count() == 0);
    CHECK(pool.idle_count() == 3);
    CHECK(pool.created_count() == 3);
    auto again = pool.acquire();
    CHECK(Widget::created == 3); // reused, not created
    static_cast<void>(first_address);
}

TEST_CASE("ResourcePool uses a custom factory and is thread safe", "[memory][raii][pool]") {
    int made = 0;
    ResourcePool<int> pool(0, [&made] { return std::make_unique<int>(++made); });
    {
        auto lease = pool.acquire();
        CHECK(*lease == 1);
    }
    std::vector<std::thread> threads;
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([&pool] {
            for (int i = 0; i < 200; ++i) {
                auto lease = pool.acquire();
                ++*lease;
            }
        });
    }
    for (auto& th : threads) {
        th.join();
    }
    CHECK(pool.leased_count() == 0);
    CHECK(pool.idle_count() == pool.created_count());
    CHECK(pool.created_count() <= 5);
}

TEST_CASE("NetworkConnection lifetime and move semantics", "[memory][raii][connection]") {
    const int before = NetworkConnection::active_connections();
    {
        NetworkConnection a("mars.example:80");
        CHECK(a.connected());
        CHECK(a.address() == "mars.example:80");
        CHECK(a.send("ping") == 4);
        CHECK(a.bytes_sent() == 4);
        CHECK(NetworkConnection::active_connections() == before + 1);

        NetworkConnection b(std::move(a));
        CHECK_FALSE(a.connected()); // NOLINT(bugprone-use-after-move)
        CHECK_THROWS_AS(a.send("x"), std::logic_error);
        CHECK(b.bytes_sent() == 4);

        NetworkConnection c("venus.example:81");
        c = std::move(b);
        CHECK(NetworkConnection::active_connections() == before + 1);
        c.disconnect();
        CHECK(NetworkConnection::active_connections() == before);
    }
    CHECK(NetworkConnection::active_connections() == before);
}

TEST_CASE("NetworkConnection constructor failure acquires nothing",
          "[memory][raii][connection][exceptions]") {
    const int before = NetworkConnection::active_connections();
    CHECK_THROWS_AS(NetworkConnection("missing-port"), std::invalid_argument);
    CHECK(NetworkConnection::active_connections() == before);
}

TEST_CASE("ArrayRAII value semantics", "[memory][raii][array]") {
    Tracked::live = 0;
    {
        RAIIUtils::ArrayRAII<Tracked> a(4, Tracked(7));
        CHECK(a.size() == 4);
        CHECK(Tracked::live == 4);
        CHECK(a[3].value == 7);
        RAIIUtils::ArrayRAII<Tracked> b = a;
        b[0].value = 1;
        CHECK(a[0].value == 7);
        CHECK(Tracked::live == 8);
        RAIIUtils::ArrayRAII<Tracked> c = std::move(b);
        CHECK(b.empty()); // NOLINT(bugprone-use-after-move)
        CHECK(c[0].value == 1);
        a = c;
        CHECK(a[0].value == 1);
        a = std::move(c);
        CHECK(Tracked::live == 4);
        CHECK_THROWS_AS(a.at(4), std::out_of_range);
        int sum = 0;
        for (const auto& t : a) {
            sum += t.value;
        }
        CHECK(sum == 1 + 7 + 7 + 7);
        RAIIUtils::ArrayRAII<Tracked> empty;
        CHECK(empty.data() == nullptr);
        CHECK(empty.begin() == empty.end());
    }
    CHECK(Tracked::live == 0);
}

TEST_CASE("ArrayRAII provides the strong exception guarantee", "[memory][raii][array][exceptions]") {
    Tracked::live = 0;
    {
        RAIIUtils::ArrayRAII<Tracked> source(5, Tracked(3));
        RAIIUtils::ArrayRAII<Tracked> target(2, Tracked(9));
        CHECK(Tracked::live == 7);

        Tracked::copies_until_throw = 2;
        CHECK_THROWS_AS(target = source, std::runtime_error);
        Tracked::copies_until_throw = -1;
        // target unchanged, partially-copied elements destroyed, nothing leaked
        CHECK(target.size() == 2);
        CHECK(target[0].value == 9);
        CHECK(Tracked::live == 7);

        Tracked::copies_until_throw = 1;
        CHECK_THROWS_AS(RAIIUtils::ArrayRAII<Tracked>(3, Tracked(1)), std::runtime_error);
        Tracked::copies_until_throw = -1;
        CHECK(Tracked::live == 7);
    }
    CHECK(Tracked::live == 0);
}

TEST_CASE("demonstrateRAII leaves no resources behind", "[memory][raii][demo]") {
    const int handles = MockOsHandleTraits::open_count();
    const int connections = NetworkConnection::active_connections();
    std::ostringstream out;
    demonstrateRAII(out);
    const auto text = out.str();
    CHECK(text.find("rollback") != std::string::npos);
    CHECK(text.find("commit") == std::string::npos);
    CHECK(text.find("FileRAII read back: RAII closes files") != std::string::npos);
    CHECK(MockOsHandleTraits::open_count() == handles);
    CHECK(NetworkConnection::active_connections() == connections);
    CHECK_FALSE(
        std::filesystem::exists(std::filesystem::temp_directory_path() / "cppversehub_memory_demo.txt"));
}
