#include "core/EventSystem.hpp"

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using namespace CppVerseHub::Core;

namespace {
struct Ping {
    int value;
};
struct Pong {
    std::string text;
};
} // namespace

TEST_CASE("EventBus dispatches only to handlers of the published type", "[core][events]") {
    EventBus bus;
    int pingSum = 0;
    std::string pongs;
    auto s1 = bus.subscribe<Ping>([&](const Ping& p) { pingSum += p.value; });
    auto s2 = bus.subscribe<Pong>([&](const Pong& p) { pongs += p.text; });
    REQUIRE(bus.publish(Ping{3}) == 1);
    REQUIRE(bus.publish(Pong{"a"}) == 1);
    REQUIRE(bus.publish(Ping{4}) == 1);
    REQUIRE(pingSum == 7);
    REQUIRE(pongs == "a");
    REQUIRE(bus.subscriberCount<Ping>() == 1);
    REQUIRE(bus.totalSubscribers() == 2);
    REQUIRE(bus.publishedCount() == 3);
}

TEST_CASE("EventBus invokes handlers in subscription order", "[core][events]") {
    EventBus bus;
    std::vector<int> order;
    auto a = bus.subscribe<Ping>([&](const Ping&) { order.push_back(1); });
    auto b = bus.subscribe<Ping>([&](const Ping&) { order.push_back(2); });
    auto c = bus.subscribe<Ping>([&](const Ping&) { order.push_back(3); });
    REQUIRE(bus.publish(Ping{0}) == 3);
    REQUIRE(order == std::vector<int>{1, 2, 3});
}

TEST_CASE("Subscription unsubscribes on destruction, reset and move-assignment", "[core][events][raii]") {
    EventBus bus;
    int calls = 0;
    {
        auto s = bus.subscribe<Ping>([&](const Ping&) { ++calls; });
        REQUIRE(s.active());
        bus.publish(Ping{1});
    }
    bus.publish(Ping{1});
    REQUIRE(calls == 1);
    REQUIRE(bus.subscriberCount<Ping>() == 0);

    auto s = bus.subscribe<Ping>([&](const Ping&) { ++calls; });
    Subscription moved = std::move(s);
    REQUIRE_FALSE(s.active()); // NOLINT(bugprone-use-after-move): moved-from state is specified
    REQUIRE(moved.active());
    bus.publish(Ping{1});
    REQUIRE(calls == 2);
    moved = bus.subscribe<Ping>([&](const Ping&) { calls += 10; }); // old handler released
    bus.publish(Ping{1});
    REQUIRE(calls == 12);
    moved.reset();
    moved.reset(); // idempotent
    REQUIRE_FALSE(static_cast<bool>(moved));
    REQUIRE(bus.publish(Ping{1}) == 0);
}

TEST_CASE("Subscription safely outlives its bus", "[core][events][raii]") {
    Subscription s;
    {
        auto bus = std::make_unique<EventBus>();
        s = bus->subscribe<Ping>([](const Ping&) {});
        REQUIRE(s.active());
    }
    REQUIRE_FALSE(s.active());
    s.reset(); // must not touch the destroyed bus
}

TEST_CASE("EventBus handlers may publish and unsubscribe re-entrantly", "[core][events]") {
    EventBus bus;
    std::vector<std::string> log;
    Subscription self;
    auto pong = bus.subscribe<Pong>([&](const Pong& p) { log.push_back(p.text); });
    self = bus.subscribe<Ping>([&](const Ping& p) {
        log.push_back("ping" + std::to_string(p.value));
        bus.publish(Pong{"pong"});
        self.reset(); // unsubscribe from inside the handler
    });
    bus.publish(Ping{1});
    bus.publish(Ping{2});
    REQUIRE(log == std::vector<std::string>{"ping1", "pong"});
}

TEST_CASE("EventBus supports concurrent publishers", "[core][events][threads]") {
    EventBus bus;
    std::atomic<int> received{0};
    auto s = bus.subscribe<Ping>(
        [&](const Ping& p) { received.fetch_add(p.value, std::memory_order_relaxed); });
    std::vector<std::thread> threads;
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([&bus] {
            for (int i = 0; i < 1000; ++i) {
                bus.publish(Ping{1});
            }
        });
    }
    for (auto& t : threads) {
        t.join();
    }
    REQUIRE(received.load() == 4000);
    REQUIRE(bus.publishedCount() == 4000);
}
