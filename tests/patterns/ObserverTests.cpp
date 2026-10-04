/**
 * @file ObserverTests.cpp
 * @brief Tests for the classic Subject/IObserver and the Signal/Connection observer.
 */

#include "patterns/Observer.hpp"

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <stdexcept>
#include <thread>
#include <vector>

using namespace CppVerseHub::Patterns;

namespace {
struct IntRecorder final : IObserver<int> {
    std::vector<int> seen;
    void onNotify(const int& e) override { seen.push_back(e); }
};
struct Thrower final : IObserver<int> {
    void onNotify(const int&) override { throw std::runtime_error("boom"); }
};
} // namespace

TEST_CASE("Subject delivers events to every attached observer", "[observer][subject]") {
    Subject<int> subject;
    auto a = std::make_shared<IntRecorder>();
    auto b = std::make_shared<IntRecorder>();
    REQUIRE(subject.attach(a));
    REQUIRE(subject.attach(b));
    CHECK(subject.notify(7) == 2);
    CHECK(subject.notify(9) == 2);
    CHECK(a->seen == std::vector<int>{7, 9});
    CHECK(b->seen == std::vector<int>{7, 9});
}

TEST_CASE("Subject rejects null and duplicate observers", "[observer][subject]") {
    Subject<int> subject;
    auto a = std::make_shared<IntRecorder>();
    CHECK_FALSE(subject.attach(nullptr));
    CHECK(subject.attach(a));
    CHECK_FALSE(subject.attach(a));
    CHECK(subject.observerCount() == 1);
    CHECK(subject.notify(1) == 1);
    CHECK(a->seen.size() == 1);
}

TEST_CASE("Subject detach removes only the requested observer", "[observer][subject]") {
    Subject<int> subject;
    auto a = std::make_shared<IntRecorder>();
    auto b = std::make_shared<IntRecorder>();
    subject.attach(a);
    subject.attach(b);
    CHECK(subject.detach(a.get()));
    CHECK_FALSE(subject.detach(a.get()));
    CHECK_FALSE(subject.detach(nullptr));
    subject.notify(3);
    CHECK(a->seen.empty());
    CHECK(b->seen == std::vector<int>{3});
}

TEST_CASE("Subject does not extend observer lifetime (weak_ptr)", "[observer][subject][lifetime]") {
    Subject<int> subject;
    auto keep = std::make_shared<IntRecorder>();
    subject.attach(keep);
    {
        auto temp = std::make_shared<IntRecorder>();
        subject.attach(temp);
        CHECK(subject.observerCount() == 2);
        std::weak_ptr<IntRecorder> weak = temp;
        temp.reset();
        CHECK(weak.expired()); // the subject held no strong reference
    }
    CHECK(subject.observerCount() == 1);
    CHECK(subject.notify(5) == 1);
}

TEST_CASE("Subject keeps notifying after an observer throws and rethrows afterwards", "[observer][subject]") {
    Subject<int> subject;
    auto first = std::make_shared<IntRecorder>();
    auto bad = std::make_shared<Thrower>();
    auto last = std::make_shared<IntRecorder>();
    subject.attach(first);
    subject.attach(bad);
    subject.attach(last);
    CHECK_THROWS_AS(subject.notify(4), std::runtime_error);
    CHECK(first->seen == std::vector<int>{4});
    CHECK(last->seen == std::vector<int>{4});
}

TEST_CASE("Signal invokes connected slots and disconnect stops delivery", "[observer][signal]") {
    Signal<int, int> sig;
    int sum = 0;
    auto c1 = sig.connect([&](int a, int b) { sum += a + b; });
    auto c2 = sig.connect([&](int a, int) { sum += a * 100; });
    CHECK(sig.slotCount() == 2);
    CHECK(sig.emit(1, 2) == 2);
    CHECK(sum == 103);
    CHECK(c1.connected());
    c1.disconnect();
    CHECK_FALSE(c1.connected());
    c1.disconnect(); // idempotent
    CHECK(sig.emit(1, 2) == 1);
    CHECK(sum == 203);
    CHECK(c2.connected());
}

TEST_CASE("ScopedConnection unsubscribes on scope exit (RAII)", "[observer][signal][raii]") {
    Signal<> sig;
    int calls = 0;
    {
        ScopedConnection scoped = sig.connectScoped([&] { ++calls; });
        CHECK(scoped.connected());
        sig.emit();
    }
    sig.emit();
    CHECK(calls == 1);
    CHECK(sig.slotCount() == 0);
}

TEST_CASE("ScopedConnection move transfers ownership; release keeps the slot", "[observer][signal][raii]") {
    Signal<> sig;
    int calls = 0;
    ScopedConnection outer;
    {
        ScopedConnection inner = sig.connectScoped([&] { ++calls; });
        outer = std::move(inner);
    }
    sig.emit();
    CHECK(calls == 1);
    Connection raw = outer.release();
    outer.reset(); // nothing managed anymore
    sig.emit();
    CHECK(calls == 2);
    raw.disconnect();
    sig.emit();
    CHECK(calls == 2);
}

TEST_CASE("Connection outliving its Signal is safe", "[observer][signal][lifetime]") {
    Connection c;
    {
        Signal<int> sig;
        c = sig.connect([](int) {});
        CHECK(c.connected());
    }
    CHECK_FALSE(c.connected());
    c.disconnect(); // must not crash
    ScopedConnection late;
    {
        Signal<int> sig;
        late = sig.connectScoped([](int) {});
    }
    late.reset();
    SUCCEED();
}

TEST_CASE("Slots may disconnect themselves and others during emission", "[observer][signal][reentrancy]") {
    Signal<> sig;
    int a = 0;
    int b = 0;
    Connection ca;
    Connection cb;
    ca = sig.connect([&] {
        ++a;
        ca.disconnect(); // self
        cb.disconnect(); // a later slot: must not be called in this emission
    });
    cb = sig.connect([&] { ++b; });
    CHECK(sig.emit() == 1);
    CHECK(sig.emit() == 0);
    CHECK(a == 1);
    CHECK(b == 0);
}

TEST_CASE("Signal disconnectAll clears every slot", "[observer][signal]") {
    Signal<int> sig;
    auto c = sig.connect([](int) {});
    auto d = sig.connect([](int) {});
    sig.disconnectAll();
    CHECK(sig.slotCount() == 0);
    CHECK_FALSE(c.connected());
    CHECK(sig.emit(1) == 0);
}

TEST_CASE("Signal is safe under concurrent connect/emit/disconnect", "[observer][signal][threads]") {
    Signal<int> sig;
    std::atomic<long> total{0};
    auto base = sig.connectScoped([&](int v) { total += v; });
    std::vector<std::thread> threads;
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([&] {
            for (int i = 0; i < 500; ++i) {
                auto c = sig.connectScoped([](int) {});
                sig.emit(1);
            }
        });
    }
    for (auto& t : threads) {
        t.join();
    }
    CHECK(total.load() == 2000);
    CHECK(sig.slotCount() == 1);
}

TEST_CASE("ObservablePlanet publishes only real changes", "[observer][planet]") {
    ObservablePlanet planet("Terra");
    auto logger = std::make_shared<EventLogger>();
    planet.subject().attach(logger);
    std::vector<PlanetEventKind> kinds;
    auto conn = planet.changed().connectScoped([&](const PlanetEvent& e) { kinds.push_back(e.kind); });

    planet.setResource("ore", 10.0);
    planet.setResource("ore", 10.0); // unchanged -> silent
    planet.setPopulation(5.0);
    planet.setDefense(-3.0); // clamped to 0 == current -> silent
    planet.setDefense(50.0);
    CHECK(planet.resource("ore") == 10.0);
    CHECK(planet.resource("unknown") == 0.0);
    REQUIRE(logger->entries().size() == 3);
    CHECK(logger->entries()[0] == "Terra: ResourceChanged ore 0 -> 10");
    CHECK(kinds == std::vector<PlanetEventKind>{PlanetEventKind::ResourceChanged,
                                                PlanetEventKind::PopulationChanged,
                                                PlanetEventKind::DefenseChanged});
}

TEST_CASE("ResourceMonitor alerts once when crossing below the threshold", "[observer][planet]") {
    ObservablePlanet planet("Mars");
    auto monitor = std::make_shared<ResourceMonitor>(50.0);
    planet.subject().attach(monitor);
    planet.setResource("water", 100.0);
    planet.setResource("water", 40.0); // crosses -> alert
    planet.setResource("water", 30.0); // already below -> no new alert
    planet.setResource("water", 80.0);
    planet.setResource("water", 10.0); // crosses again
    REQUIRE(monitor->alerts().size() == 2);
    CHECK(monitor->alerts()[0] == "LOW water on Mars");
}

TEST_CASE("DefenseMonitor accumulates damage and flags critical defence", "[observer][planet]") {
    ObservablePlanet planet("Io");
    auto monitor = std::make_shared<DefenseMonitor>(10.0);
    planet.subject().attach(monitor);
    planet.setDefense(50.0);
    planet.attack(15.0);
    CHECK_FALSE(monitor->critical());
    planet.attack(100.0); // clamped to remaining 35
    CHECK(planet.defense() == 0.0);
    CHECK(monitor->attacksObserved() == 2);
    CHECK(monitor->totalDamage() == 50.0);
    CHECK(monitor->critical());
}

TEST_CASE("toString covers every PlanetEventKind", "[observer]") {
    CHECK(toString(PlanetEventKind::ResourceChanged) == "ResourceChanged");
    CHECK(toString(PlanetEventKind::PopulationChanged) == "PopulationChanged");
    CHECK(toString(PlanetEventKind::DefenseChanged) == "DefenseChanged");
    CHECK(toString(PlanetEventKind::UnderAttack) == "UnderAttack");
}
