#include "modern/LambdaExpressions.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <numeric>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

using namespace CppVerseHub::Modern::LambdaExpressions;
using Catch::Approx;

TEST_CASE("compose applies right-to-left, pipeline left-to-right", "[modern][lambda]") {
    auto inc = [](int x) { return x + 1; };
    auto dbl = [](int x) { return x * 2; };
    auto sq = [](int x) { return x * x; };
    CHECK(compose(inc, dbl, sq)(3) == 19);  // inc(dbl(sq(3)))
    CHECK(pipeline(inc, dbl, sq)(3) == 64); // sq(dbl(inc(3)))
    CHECK(compose(inc)(0) == 1);
    auto concat = [](const std::string& a, const std::string& b) { return a + b; };
    auto len = [](const std::string& s) { return s.size(); };
    CHECK(compose(len, concat)("ab", "cde") == 5); // inner function may take several arguments
}

TEST_CASE("curry supports any argument grouping", "[modern][lambda]") {
    auto f = [](int a, int b, int c, int d) { return ((a * 10 + b) * 10 + c) * 10 + d; };
    auto c = curry(f);
    CHECK(c(1)(2)(3)(4) == 1234);
    CHECK(c(1, 2)(3, 4) == 1234);
    CHECK(c(1, 2, 3, 4) == 1234);
    auto partial = c(9);
    CHECK(partial(8, 7, 6) == 9876);
    CHECK(partial(1)(1)(1) == 9111); // the partial application is reusable
}

TEST_CASE("curry stores bound arguments by value", "[modern][lambda]") {
    auto greet = curry([](const std::string& a, const std::string& b) { return a + ", " + b; });
    std::string name = "Hello";
    auto bound = greet(name);
    name = "Changed";
    CHECK(bound(std::string("world")) == "Hello, world");
}

TEST_CASE("Overloaded builds visitors", "[modern][lambda]") {
    std::variant<int, std::string, double> v = std::string("abc");
    auto visitor = Overloaded{[](int) { return 1; }, [](const std::string&) { return 2; },
                              [](double) { return 3; }};
    CHECK(std::visit(visitor, v) == 2);
    v = 2.0;
    CHECK(std::visit(visitor, v) == 3);
    v = 7;
    CHECK(std::visit(visitor, v) == 1);
}

TEST_CASE("Fix enables recursive lambdas", "[modern][lambda]") {
    const Fix gcdFix{[](const auto& self, int a, int b) -> int { return b == 0 ? a : self(b, a % b); }};
    CHECK(gcdFix(84, 36) == 12);
    CHECK(factorialFix(0) == 1);
    CHECK(factorialFix(20) == 2432902008176640000ULL);
}

TEST_CASE("Memoized caches results and counts hits", "[modern][lambda]") {
    int calls = 0;
    Memoized<int, int> m([&calls](const int& x) {
        ++calls;
        return x * x;
    });
    CHECK(m(4) == 16);
    CHECK(m(4) == 16);
    CHECK(m(5) == 25);
    CHECK(calls == 2);
    CHECK(m.misses() == 2);
    CHECK(m.hits() == 1);
    CHECK(m.size() == 2);
}

TEST_CASE("makeCounter keeps independent state per closure", "[modern][lambda]") {
    auto a = makeCounter(10);
    auto b = makeCounter();
    CHECK(a() == 10);
    CHECK(a() == 11);
    CHECK(b() == 0);
    auto copy = a; // copying a closure copies its state
    CHECK(copy() == 12);
    CHECK(a() == 12);
}

TEST_CASE("makeRunningAverage tracks the mean", "[modern][lambda]") {
    auto avg = makeRunningAverage();
    CHECK(avg(10.0) == Approx(10.0));
    CHECK(avg(20.0) == Approx(15.0));
    CHECK(avg(0.0) == Approx(10.0));
}

TEST_CASE("makeRangeValidator is inclusive", "[modern][lambda]") {
    auto valid = makeRangeValidator(1, 5);
    const int v = GENERATE(0, 1, 3, 5, 6);
    CHECK(valid(v) == (v >= 1 && v <= 5));
}

TEST_CASE("init-capture can own move-only resources", "[modern][lambda]") {
    auto reporter = makeOwningReporter(std::make_unique<std::string>("12345"));
    CHECK(reporter() == 5);
    STATIC_CHECK_FALSE(std::is_copy_constructible_v<decltype(reporter)>);
    auto moved = std::move(reporter);
    CHECK(moved() == 5);
    CHECK(makeOwningReporter(nullptr)() == 0);
}

TEST_CASE("this vs *this capture", "[modern][lambda]") {
    Beacon b("one");
    auto live = b.liveReporter();
    auto snap = b.snapshotReporter();
    b.relabel("two");
    CHECK(live() == "two");
    CHECK(snap() == "one");
}

TEST_CASE("filterBy and mapTo", "[modern][lambda]") {
    const auto planets = samplePlanets();
    const auto habitable = filterBy(planets, [](const Planet& p) { return p.habitable; });
    REQUIRE(habitable.size() == 3);
    CHECK(habitable[0].name == "Earth");
    const auto names = mapTo(planets, [](const Planet& p) { return p.name; });
    CHECK(names.size() == planets.size());
    CHECK(names.back() == "Proxima-b");
    CHECK(mapTo(std::vector<int>{1, 2}, [](int x) { return x * 0.5; }) == std::vector<double>{0.5, 1.0});
}

TEST_CASE("sizeInBits template lambda", "[modern][lambda]") {
    CHECK(sizeInBits(char{}) == 8);
    CHECK(sizeInBits(std::uint64_t{}) == 64);
}

TEST_CASE("summarizeFleet uses STL algorithms correctly", "[modern][lambda]") {
    const auto s = summarizeFleet(sampleFleet());
    CHECK(s.operationalNames == std::vector<std::string>{"USS Guardian", "USS Explorer", "USS Defender"});
    CHECK(s.activeCrew == 150 + 300 + 200 + 180);
    CHECK(s.meanFuel == Approx((85.5 + 92.0 + 23.1 + 67.8 + 41.2) / 5.0));
    const auto empty = summarizeFleet({});
    CHECK(empty.operationalNames.empty());
    CHECK(empty.meanFuel == 0.0);
}

TEST_CASE("EventBus subscribe, emit and unsubscribe", "[modern][lambda]") {
    EventBus bus;
    std::vector<std::string> received;
    const auto a = bus.subscribe("x", [&](const std::string& p) { received.push_back("a:" + p); });
    const auto b = bus.subscribe("x", [&](const std::string& p) { received.push_back("b:" + p); });
    bus.subscribe("y", [&](const std::string& p) { received.push_back("y:" + p); });
    CHECK(a != b);
    CHECK(bus.handlerCount("x") == 2);
    CHECK(bus.emit("x", "1") == 2);
    CHECK(received == std::vector<std::string>{"a:1", "b:1"});
    CHECK(bus.unsubscribe(a));
    CHECK_FALSE(bus.unsubscribe(a));
    CHECK_FALSE(bus.unsubscribe(12345));
    CHECK(bus.emit("x", "2") == 1);
    CHECK(bus.emit("none", "") == 0);
    CHECK(received.back() == "b:2");
    CHECK(bus.handlerCount("none") == 0);
}

TEST_CASE("parallelSum matches std::accumulate for any chunk count", "[modern][lambda]") {
    std::vector<int> data(1001);
    std::iota(data.begin(), data.end(), -500);
    data.push_back(7777);
    const auto expected = std::accumulate(data.begin(), data.end(), 0LL);
    const std::size_t chunks = GENERATE(0U, 1U, 3U, 8U, 5000U);
    CHECK(parallelSum(data, chunks) == expected);
    CHECK(parallelSum({}, chunks) == 0);
}

TEST_CASE("lambda showcases write to the stream", "[modern][lambda]") {
    std::ostringstream os;
    demonstrateAllLambdas(os);
    const auto text = os.str();
    CHECK(text.find("[this] sees 'Omega', [*this] kept 'Alpha'") != std::string::npos);
    CHECK(text.find("parallelSum(1..1000, 4 tasks) = 500500") != std::string::npos);
    CHECK(text.find("product of 1..10 on another thread = 3628800") != std::string::npos);
    CHECK(text.find("Memoized: 2 computations, 3 cache hits") != std::string::npos);
}
