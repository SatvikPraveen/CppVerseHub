// Behavioural tests for AsyncMissions.hpp, AsyncComms.hpp and the module demo.
// Catch2 assertions run only on the main thread.

#include "concurrency/AsyncComms.hpp"
#include "concurrency/AsyncMissions.hpp"
#include "concurrency/Demo.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <atomic>
#include <future>
#include <map>
#include <mutex>
#include <numeric>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <variant>
#include <vector>

using namespace CppVerseHub::Concurrency;

// =========================================================================== missions

TEST_CASE("MissionStatus to_string", "[concurrency][async]") {
    CHECK(to_string(MissionStatus::Pending) == "pending");
    CHECK(to_string(MissionStatus::Running) == "running");
    CHECK(to_string(MissionStatus::Succeeded) == "succeeded");
    CHECK(to_string(MissionStatus::Failed) == "failed");
    CHECK(to_string(MissionStatus::Cancelled) == "cancelled");
}

TEST_CASE("CancellationSource and CancellationToken", "[concurrency][async]") {
    CancellationToken never;
    CHECK_FALSE(never.is_cancelled());
    CHECK_NOTHROW(never.throw_if_cancelled());

    CancellationSource source;
    auto token = source.token();
    auto copy = token;
    CHECK_FALSE(token.is_cancelled());
    source.cancel();
    source.cancel();
    CHECK(source.is_cancelled());
    CHECK(token.is_cancelled());
    CHECK(copy.is_cancelled());
    CHECK_THROWS_AS(copy.throw_if_cancelled(), MissionCancelled);
}

TEST_CASE("AsyncMission reports success, failure and cancellation", "[concurrency][async]") {
    SECTION("success") {
        AsyncMission<int> m("sum", [](const CancellationToken&) { return 6 * 7; });
        CHECK(m.status() == MissionStatus::Pending);
        auto f = m.start();
        const auto r = f.get();
        CHECK(r.ok());
        CHECK(r.value == 42);
        CHECK(m.status() == MissionStatus::Succeeded);
        CHECK(m.name() == "sum");
        CHECK_THROWS_AS(m.start(), std::logic_error);
    }
    SECTION("failure captures the message") {
        AsyncMission<int> m("fail",
                            [](const CancellationToken&) -> int { throw std::runtime_error("no fuel"); });
        const auto r = m.start().get();
        CHECK(r.status == MissionStatus::Failed);
        CHECK(r.error == "no fuel");
        CHECK_FALSE(r.value.has_value());
    }
    SECTION("cancel before start") {
        bool ran = false;
        AsyncMission<int> m("cancelled", [&ran](const CancellationToken&) {
            ran = true;
            return 1;
        });
        m.cancel();
        const auto r = m.start().get();
        CHECK(r.status == MissionStatus::Cancelled);
        CHECK_FALSE(ran);
    }
    SECTION("cooperative cancellation while running") {
        std::promise<void> running;
        auto running_future = running.get_future();
        AsyncMission<int> m("loop", [&running](const CancellationToken& token) -> int {
            running.set_value();
            for (;;) {
                token.throw_if_cancelled();
                std::this_thread::yield();
            }
        });
        auto f = m.start();
        running_future.wait();
        m.cancel();
        CHECK(f.get().status == MissionStatus::Cancelled);
    }
    SECTION("deferred launch runs on get()") {
        std::thread::id runner;
        AsyncMission<int> m("lazy", [&runner](const CancellationToken&) {
            runner = std::this_thread::get_id();
            return 3;
        });
        auto f = m.start(std::launch::deferred);
        CHECK(m.status() == MissionStatus::Pending);
        CHECK(f.get().value == 3);
        CHECK(runner == std::this_thread::get_id());
    }
}

TEST_CASE("MissionCoordinator respects dependencies", "[concurrency][async]") {
    ThreadPool pool(4);
    MissionCoordinator c;
    std::mutex m;
    std::vector<std::size_t> ran;
    auto body = [&](std::size_t id) {
        return [&, id](const CancellationToken&) {
            std::lock_guard lock(m);
            ran.push_back(id);
        };
    };
    // Diamond plus a tail: 0 -> {1, 2} -> 3 -> 4, and an independent 5.
    const auto a = c.add_mission("a", body(0));
    const auto b = c.add_mission("b", body(1), {a});
    const auto d = c.add_mission("c", body(2), {a});
    const auto e = c.add_mission("d", body(3), {b, d});
    const auto f = c.add_mission("e", body(4), {e});
    const auto g = c.add_mission("f", body(5));
    CHECK(c.size() == 6);
    CHECK(c.topological_order() == std::vector<std::size_t>{0, 1, 2, 3, 4, 5});
    c.run(pool);

    REQUIRE(ran.size() == 6);
    auto pos = [&](std::size_t id) { return std::find(ran.begin(), ran.end(), id) - ran.begin(); };
    CHECK(pos(0) < pos(1));
    CHECK(pos(0) < pos(2));
    CHECK(pos(1) < pos(3));
    CHECK(pos(2) < pos(3));
    CHECK(pos(3) < pos(4));
    for (auto id : {a, b, d, e, f, g}) {
        CHECK(c.status(id) == MissionStatus::Succeeded);
    }
    CHECK(c.completion_order().size() == 6);
    CHECK_THROWS_AS(c.run(pool), std::logic_error);
    CHECK_THROWS_AS(c.add_mission("late", body(9)), std::logic_error);
}

TEST_CASE("MissionCoordinator cancels transitive dependents of a failure", "[concurrency][async]") {
    ThreadPool pool(2);
    MissionCoordinator c;
    std::atomic<int> executed{0};
    auto ok = [&executed](const CancellationToken&) { executed.fetch_add(1); };
    const auto root = c.add_mission("root", ok);
    const auto bad = c.add_mission("bad", [](const CancellationToken&) { throw std::runtime_error("jam"); },
                                   {root});
    const auto child = c.add_mission("child", ok, {bad});
    const auto grandchild = c.add_mission("grandchild", ok, {child});
    const auto join = c.add_mission("join", ok, {root, grandchild});
    const auto sibling = c.add_mission("sibling", ok, {root});
    c.run(pool);
    CHECK(c.status(root) == MissionStatus::Succeeded);
    CHECK(c.status(bad) == MissionStatus::Failed);
    CHECK(c.error(bad) == "jam");
    CHECK(c.status(child) == MissionStatus::Cancelled);
    CHECK(c.status(grandchild) == MissionStatus::Cancelled);
    CHECK(c.status(join) == MissionStatus::Cancelled);
    CHECK(c.error(child).find("bad") != std::string::npos);
    CHECK(c.status(sibling) == MissionStatus::Succeeded);
    CHECK(executed.load() == 2);
    CHECK(c.name(sibling) == "sibling");
}

TEST_CASE("MissionCoordinator detects cycles and invalid ids", "[concurrency][async]") {
    MissionCoordinator c;
    auto noop = [](const CancellationToken&) {};
    const auto a = c.add_mission("a", noop);
    const auto b = c.add_mission("b", noop, {a});
    CHECK_THROWS_AS(c.add_mission("x", noop, {42}), std::out_of_range);
    CHECK_THROWS_AS(c.add_dependency(a, 99), std::out_of_range);
    c.add_dependency(a, b); // a -> b -> a
    CHECK_FALSE(c.topological_order().has_value());
    ThreadPool pool(1);
    CHECK_THROWS_AS(c.run(pool), std::logic_error);
    CHECK_THROWS_AS(c.status(17), std::out_of_range);
}

TEST_CASE("MissionCoordinator cancel_all stops pending missions", "[concurrency][async]") {
    ThreadPool pool(2);
    MissionCoordinator c;
    std::atomic<int> executed{0};
    const auto first = c.add_mission("first", [&](const CancellationToken&) {
        executed.fetch_add(1);
        c.cancel_all();
    });
    const auto second = c.add_mission("second", [&](const CancellationToken&) { executed.fetch_add(1); },
                                      {first});
    const auto third = c.add_mission("third", [&](const CancellationToken&) { executed.fetch_add(1); },
                                     {second});
    c.run(pool);
    CHECK(c.status(first) == MissionStatus::Succeeded);
    CHECK(c.status(second) == MissionStatus::Cancelled);
    CHECK(c.status(third) == MissionStatus::Cancelled);
    CHECK(executed.load() == 1);
}

TEST_CASE("MissionCoordinator with an empty graph returns immediately", "[concurrency][async]") {
    ThreadPool pool(1);
    MissionCoordinator c;
    c.run(pool);
    CHECK(c.size() == 0);
    CHECK(c.completion_order().empty());
}

TEST_CASE("parallel_transform preserves order and propagates exceptions", "[concurrency][async]") {
    ThreadPool pool(4);
    std::vector<int> in(1000);
    std::iota(in.begin(), in.end(), 0);
    const auto out = parallel_transform(pool, in, [](int x) { return std::to_string(x * 2); });
    REQUIRE(out.size() == in.size());
    for (std::size_t i = 0; i < in.size(); ++i) {
        REQUIRE(out[i] == std::to_string(in[i] * 2));
    }
    CHECK(parallel_transform(pool, std::vector<int>{}, [](int x) { return x; }).empty());
    const auto chunked = parallel_transform(pool, in, [](int x) { return x + 1; }, 7);
    CHECK(std::accumulate(chunked.begin(), chunked.end(), 0LL) == 1000LL * 1001 / 2);
    CHECK_THROWS_AS(parallel_transform(pool, in,
                                       [](int x) {
                                           if (x == 500) {
                                               throw std::range_error("bad sample");
                                           }
                                           return x;
                                       }),
                    std::range_error);
}

TEST_CASE("when_all collects values in order and rethrows the first error", "[concurrency][async]") {
    ThreadPool pool(3);
    std::vector<std::future<int>> fs;
    for (int i = 0; i < 10; ++i) {
        fs.push_back(pool.submit([i] { return i * i; }));
    }
    const auto values = when_all(std::move(fs));
    CHECK(values == std::vector<int>{0, 1, 4, 9, 16, 25, 36, 49, 64, 81});

    std::vector<std::future<int>> failing;
    failing.push_back(pool.submit([] { return 1; }));
    failing.push_back(pool.submit([]() -> int { throw std::invalid_argument("first"); }));
    failing.push_back(pool.submit([]() -> int { throw std::out_of_range("second"); }));
    CHECK_THROWS_AS(when_all(std::move(failing)), std::invalid_argument);
}

TEST_CASE("Pipeline processes items through threaded stages in order", "[concurrency][async]") {
    Pipeline<int> identity;
    CHECK(identity.process({3, 1, 2}) == std::vector<int>{3, 1, 2});

    Pipeline<std::string> p;
    p.add_stage("trim", [](std::string s) { return s.substr(1); }).add_stage("tag", [](std::string s) {
        return "[" + s + "]";
    });
    CHECK(p.stage_count() == 2);
    CHECK(p.stage_names() == std::vector<std::string>{"trim", "tag"});
    std::vector<std::string> in;
    for (int i = 0; i < 300; ++i) {
        in.push_back("x" + std::to_string(i));
    }
    const auto out = p.process(in, 2);
    REQUIRE(out.size() == 300);
    for (std::size_t i = 0; i < out.size(); ++i) {
        REQUIRE(out[i] == "[" + std::to_string(i) + "]");
    }

    Pipeline<int> failing;
    failing.add_stage("ok", [](int x) { return x; }).add_stage("explode", [](int x) {
        if (x == 50) {
            throw std::runtime_error("stage failure");
        }
        return x;
    });
    std::vector<int> nums(200);
    std::iota(nums.begin(), nums.end(), 0);
    CHECK_THROWS_AS(failing.process(nums, 4), std::runtime_error);
}

// =========================================================================== communication

TEST_CASE("MessageBus delivers to topic subscribers in publication order", "[concurrency][comms]") {
    std::vector<std::string> a_log;
    std::vector<std::string> b_log;
    MessageBus bus(4);
    const auto a = bus.subscribe("nav", [&](const Message& m) { a_log.push_back(m.payload); });
    static_cast<void>(bus.subscribe("nav", [&](const Message& m) { b_log.push_back(m.payload); }));
    static_cast<void>(bus.subscribe("comms", [&](const Message& m) { b_log.push_back("c:" + m.payload); }));
    CHECK(bus.subscriber_count("nav") == 2);
    CHECK(bus.subscriber_count("none") == 0);
    CHECK(bus.topics() == std::vector<std::string>{"comms", "nav"});

    for (int i = 0; i < 20; ++i) {
        CHECK(bus.publish(Message{i % 2 == 0 ? "nav" : "comms", std::to_string(i), 1, std::nullopt}));
    }
    static_cast<void>(bus.publish(Message{"nobody-listens", "x", 1, std::nullopt}));
    bus.flush();
    CHECK(a_log == std::vector<std::string>{"0", "2", "4", "6", "8", "10", "12", "14", "16", "18"});
    REQUIRE(b_log.size() == 20);
    CHECK(b_log[0] == "0");
    CHECK(b_log[1] == "c:1");
    CHECK(bus.deliveries() == 30);

    CHECK(bus.unsubscribe(a));
    CHECK_FALSE(bus.unsubscribe(a));
    static_cast<void>(bus.publish(Message{"nav", "after", 1, std::nullopt}));
    bus.flush();
    CHECK(a_log.size() == 10);
    CHECK(b_log.back() == "after");
}

TEST_CASE("MessageBus handlers may publish, failures are contained, shutdown drains",
          "[concurrency][comms]") {
    std::atomic<int> pongs{0};
    MessageBus bus;
    static_cast<void>(bus.subscribe("ping", [&bus](const Message& m) {
        static_cast<void>(bus.publish(Message{"pong", m.payload, 2, m.correlation_id})); // re-entrant publish
    }));
    static_cast<void>(bus.subscribe("pong", [&pongs](const Message&) { pongs.fetch_add(1); }));
    static_cast<void>(
        bus.subscribe("faulty", [](const Message&) { throw std::runtime_error("handler bug"); }));
    for (int i = 0; i < 10; ++i) {
        static_cast<void>(bus.publish(Message{"ping", std::to_string(i), 1, static_cast<std::uint64_t>(i)}));
    }
    static_cast<void>(bus.publish(Message{"faulty", "x", 1, std::nullopt}));
    bus.shutdown(); // delivers the backlog; pongs published during shutdown may be rejected
    CHECK(bus.handler_failures() == 1);
    CHECK(pongs.load() <= 10);
    CHECK_FALSE(bus.publish(Message{"ping", "late", 1, std::nullopt}));
    bus.flush(); // returns immediately after shutdown
}

TEST_CASE("MessageBus flush observes re-entrant publications", "[concurrency][comms]") {
    std::atomic<int> pongs{0};
    MessageBus bus;
    static_cast<void>(bus.subscribe("ping", [&bus](const Message& m) {
        static_cast<void>(bus.publish(Message{"pong", m.payload, 2, std::nullopt}));
    }));
    static_cast<void>(bus.subscribe("pong", [&pongs](const Message&) { pongs.fetch_add(1); }));
    for (int i = 0; i < 10; ++i) {
        static_cast<void>(bus.publish(Message{"ping", std::to_string(i), 1, std::nullopt}));
    }
    bus.flush(); // waits for quiescence, which includes the pongs published by handlers
    CHECK(pongs.load() == 10);
    CHECK(bus.deliveries() == 20);
}

namespace {
struct Add {
    int value;
};
struct Get {
    std::promise<std::vector<int>> reply;
};
struct Fail {};
using Command = std::variant<Add, Get, Fail>;
} // namespace

TEST_CASE("Actor processes messages sequentially and supports ask", "[concurrency][comms]") {
    std::vector<int> state; // owned by the actor thread
    Actor<Command> actor(
        [&state](Command& c) {
            if (auto* add = std::get_if<Add>(&c)) {
                state.push_back(add->value);
            } else if (auto* get = std::get_if<Get>(&c)) {
                get->reply.set_value(state);
            } else {
                throw std::runtime_error("fail command");
            }
        },
        8);
    for (int i = 0; i < 100; ++i) {
        CHECK(actor.tell(Command{Add{i}}));
    }
    CHECK(actor.tell(Command{Fail{}}));
    Get get;
    auto reply = get.reply.get_future();
    CHECK(actor.tell(Command{std::move(get)}));
    const auto snapshot = reply.get();
    std::vector<int> expected(100);
    std::iota(expected.begin(), expected.end(), 0);
    CHECK(snapshot == expected); // mailbox order preserved
    actor.stop();
    CHECK(actor.processed() == 102);
    CHECK(actor.failures() == 1);
    CHECK_FALSE(actor.tell(Command{Add{1}}));
    actor.stop(); // idempotent
}

TEST_CASE("Actor with concurrent senders and drain on destruction", "[concurrency][comms]") {
    std::atomic<long long> sum{0};
    {
        Actor<int> actor([&sum](int& v) { sum.fetch_add(v); }, 4);
        std::vector<std::thread> senders;
        for (int t = 0; t < 4; ++t) {
            senders.emplace_back([&actor] {
                for (int i = 1; i <= 250; ++i) {
                    static_cast<void>(actor.tell(i));
                }
            });
        }
        for (auto& s : senders) {
            s.join();
        }
    } // destructor drains the mailbox
    CHECK(sum.load() == 4LL * 250 * 251 / 2);
}

TEST_CASE("RequestResponseServer answers requests through futures", "[concurrency][comms]") {
    RequestResponseServer server(3);
    server.register_handler("double", [](const std::string& s) { return s + s; });
    server.register_handler("len", [](const std::string& s) { return std::to_string(s.size()); });

    std::vector<RequestResponseServer::Ticket> tickets;
    for (int i = 0; i < 50; ++i) {
        tickets.push_back(
            server.request(i % 2 == 0 ? "double" : "len", std::string(static_cast<std::size_t>(i), 'z')));
    }
    std::set<std::uint64_t> ids;
    for (std::size_t i = 0; i < tickets.size(); ++i) {
        auto response = tickets[i].response.get();
        CHECK(response.correlation_id == tickets[i].correlation_id);
        ids.insert(response.correlation_id);
        if (i % 2 == 0) {
            CHECK(response.payload == std::string(2 * i, 'z'));
        } else {
            CHECK(response.payload == std::to_string(i));
        }
    }
    CHECK(ids.size() == 50); // correlation ids are unique

    auto unknown = server.request("teleport", "now");
    CHECK_THROWS_AS(unknown.response.get(), UnknownRequestError);
    CHECK(server.unregister_handler("len"));
    CHECK_FALSE(server.unregister_handler("len"));
    auto gone = server.request("len", "abc");
    CHECK_THROWS_AS(gone.response.get(), UnknownRequestError);
    server.register_handler("double", [](const std::string& s) { return s + "|" + s; }); // replace
    CHECK(server.request("double", "a").response.get().payload == "a|a");
    server.shutdown();
    CHECK_THROWS_AS(server.request("double", "a"), PoolShutdownError);
}

// =========================================================================== demo

TEST_CASE("runDemo runs every showcase and writes to the given stream", "[concurrency][demo]") {
    std::ostringstream oss;
    CHECK_NOTHROW(runDemo(oss));
    const std::string text = oss.str();
    CHECK_FALSE(text.empty());
    for (const char* section :
         {"=== Thread pools ===", "=== Mutexes", "=== Condition variables ===", "=== Atomics",
          "=== Async missions", "=== Asynchronous communication ===", "=== C++20 coroutines ==="}) {
        CHECK(text.find(section) != std::string::npos);
    }
    CHECK(text.find("aborted") == std::string::npos);
    CHECK(text.find("Graceful shutdown executed 100/100") != std::string::npos);
    CHECK(text.find("A0 B0 A1 B1 A2 B2") != std::string::npos);
}
