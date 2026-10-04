/**
 * @file LambdaExpressions.cpp
 * @brief Implementation of the lambda showcase (see LambdaExpressions.hpp).
 */
#include "modern/LambdaExpressions.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <future>
#include <iterator>
#include <numeric>
#include <set>
#include <string_view>
#include <variant>

namespace CppVerseHub::Modern::LambdaExpressions {

std::vector<SpaceShip> sampleFleet() {
    return {
        {1, "USS Explorer", "Science", 85.5, 150, true},
        {2, "USS Guardian", "Battleship", 92.0, 300, true},
        {3, "USS Voyager", "Scout", 23.1, 50, false},
        {4, "USS Defender", "Destroyer", 67.8, 200, true},
        {5, "USS Discovery", "Research", 41.2, 180, true},
    };
}

std::vector<Planet> samplePlanets() {
    return {
        {1, "Mercury", 0.39, 0, false},
        {2, "Venus", 0.72, 0, false},
        {3, "Earth", 1.0, 8'000'000'000LL, true},
        {4, "Mars", 1.52, 0, false},
        {5, "Jupiter", 5.20, 0, false},
        {6, "Kepler-442b", 112.0, 50'000'000LL, true},
        {7, "Proxima-b", 42400.0, 0, true},
    };
}

// ===== EventBus =====

EventBus::SubscriptionId EventBus::subscribe(const std::string& event, Handler handler) {
    const SubscriptionId id = nextId_++;
    handlers_[event].push_back(Entry{id, std::move(handler)});
    return id;
}

bool EventBus::unsubscribe(SubscriptionId id) {
    for (auto& [event, entries] : handlers_) {
        auto it = std::find_if(entries.begin(), entries.end(), [id](const Entry& e) { return e.id == id; });
        if (it != entries.end()) {
            entries.erase(it);
            return true;
        }
    }
    return false;
}

std::size_t EventBus::emit(const std::string& event, const std::string& payload) const {
    auto it = handlers_.find(event);
    if (it == handlers_.end()) {
        return 0;
    }
    for (const auto& entry : it->second) {
        entry.handler(payload);
    }
    return it->second.size();
}

std::size_t EventBus::handlerCount(const std::string& event) const {
    auto it = handlers_.find(event);
    return it == handlers_.end() ? 0 : it->second.size();
}

// ===== parallelSum =====

long long parallelSum(const std::vector<int>& data, std::size_t chunks) {
    if (data.empty()) {
        return 0;
    }
    chunks = std::clamp<std::size_t>(chunks, 1, data.size());
    const std::size_t chunkSize = (data.size() + chunks - 1) / chunks;

    std::vector<std::future<long long>> futures;
    futures.reserve(chunks);
    for (std::size_t begin = 0; begin < data.size(); begin += chunkSize) {
        const std::size_t end = std::min(begin + chunkSize, data.size());
        // The lambda captures the bounds by value and the data by reference; the futures are joined
        // before `data` can go out of scope, so the reference never dangles.
        futures.push_back(std::async(std::launch::async, [&data, begin, end] {
            return std::accumulate(data.begin() + static_cast<std::ptrdiff_t>(begin),
                                   data.begin() + static_cast<std::ptrdiff_t>(end), 0LL);
        }));
    }
    long long total = 0;
    for (auto& f : futures) {
        total += f.get();
    }
    return total;
}

// ===== summarizeFleet =====

FleetSummary summarizeFleet(const std::vector<SpaceShip>& fleet) {
    FleetSummary summary;

    std::vector<SpaceShip> operational;
    std::copy_if(fleet.begin(), fleet.end(), std::back_inserter(operational),
                 [](const SpaceShip& s) { return s.isActive && s.fuelLevel > 50.0; });
    std::sort(operational.begin(), operational.end(),
              [](const SpaceShip& a, const SpaceShip& b) { return a.fuelLevel > b.fuelLevel; });
    std::transform(operational.begin(), operational.end(), std::back_inserter(summary.operationalNames),
                   [](const SpaceShip& s) { return s.name; });

    summary.activeCrew = std::accumulate(fleet.begin(), fleet.end(), 0, [](int sum, const SpaceShip& s) {
        return s.isActive ? sum + s.crewSize : sum;
    });
    if (!fleet.empty()) {
        const double totalFuel = std::accumulate(fleet.begin(), fleet.end(), 0.0,
                                                 [](double acc, const SpaceShip& s) { return acc + s.fuelLevel; });
        summary.meanFuel = totalFuel / static_cast<double>(fleet.size());
    }
    return summary;
}

// ===== SHOWCASES =====

void demonstrateBasicLambdas(std::ostream& out) {
    out << "\n--- Basic lambdas ---\n";
    auto greet = [] { return std::string_view{"Welcome to CppVerseHub!"}; };
    out << greet() << '\n';

    auto distance = [](double x1, double y1, double x2, double y2) {
        return std::hypot(x2 - x1, y2 - y1);
    };
    out << "distance((0,0),(3,4)) = " << distance(0.0, 0.0, 3.0, 4.0) << '\n';

    auto clampPercent = [](double v) -> int {  // explicit trailing return type
        return static_cast<int>(std::clamp(v, 0.0, 100.0));
    };
    out << "clampPercent(140.7) = " << clampPercent(140.7) << '\n';

    int totalScore = 0;
    auto addScore = [&totalScore](int points) { totalScore += points; };
    for (int p : {150, 200, 75}) {
        addScore(p);
    }
    out << "score after three additions: " << totalScore << '\n';

    int (*fnPtr)(int) = [](int x) { return x * 3; };  // capture-less lambdas convert to function pointers
    out << "function-pointer conversion: fnPtr(7) = " << fnPtr(7) << '\n';
}

void demonstrateCaptureModes(std::ostream& out) {
    out << "\n--- Capture modes ---\n";
    int fleetCount = 5;
    double fuelReserve = 1000.0;
    const std::string commander = "Admiral Zhang";

    auto byValue = [=] { return fleetCount * 10; };          // copies taken now
    auto byReference = [&] { fuelReserve -= 150.0; ++fleetCount; };
    auto mixed = [=, &fuelReserve](double cost) {            // everything by value except fuelReserve
        fuelReserve -= cost;
        return commander + " has " + std::to_string(static_cast<int>(fuelReserve)) + " fuel";
    };
    byReference();
    out << "[=] saw fleetCount when created: " << byValue() << " (now " << fleetCount * 10 << ")\n";
    out << "[=, &fuelReserve]: " << mixed(200.0) << '\n';

    auto reporter = makeOwningReporter(std::make_unique<std::string>("cargo manifest"));
    out << "init-capture moved a unique_ptr in; reporter() = " << reporter() << '\n';

    Beacon beacon("Alpha");
    auto live = beacon.liveReporter();
    auto snapshot = beacon.snapshotReporter();
    beacon.relabel("Omega");
    out << "[this] sees '" << live() << "', [*this] kept '" << snapshot() << "'\n";
}

void demonstrateStlLambdas(std::ostream& out) {
    out << "\n--- Lambdas with STL algorithms ---\n";
    const auto fleet = sampleFleet();
    const auto summary = summarizeFleet(fleet);
    out << "Operational (>50% fuel, by fuel):";
    for (const auto& n : summary.operationalNames) {
        out << ' ' << n << ';';
    }
    out << "\nActive crew: " << summary.activeCrew << ", mean fuel: " << summary.meanFuel << "%\n";

    auto upper = mapTo(fleet, [](const SpaceShip& s) {
        std::string name = s.name;
        std::transform(name.begin(), name.end(), name.begin(),
                       [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
        return name;
    });
    out << "Upper-cased first ship: " << upper.front() << '\n';

    const auto planets = samplePlanets();
    const auto habitable = filterBy(planets, [](const Planet& p) { return p.habitable; });
    out << "Habitable planets:";
    for (const auto& p : habitable) {
        out << ' ' << p.name;
    }
    out << '\n';
    const double limitAu = 10.0;
    const auto nearby = std::count_if(planets.begin(), planets.end(),
                                      [limitAu](const Planet& p) { return p.distanceAu <= limitAu; });
    out << "Planets within " << limitAu << " AU: " << nearby << '\n';
}

void demonstrateGenericLambdas(std::ostream& out) {
    out << "\n--- Generic, template and constexpr lambdas ---\n";
    auto greater = [](const auto& a, const auto& b) { return a > b; };
    out << std::boolalpha << "greater(5, 3) = " << greater(5, 3)
        << ", greater(\"zebra\"s, \"apple\"s) = " << greater(std::string("zebra"), std::string("apple")) << '\n';

    auto joinMapped = [](const auto& container, auto fn) {
        std::string s;
        for (const auto& item : container) {
            s += std::to_string(fn(item)) + ' ';
        }
        return s;
    };
    out << "squares: " << joinMapped(std::vector{1, 2, 3, 4}, [](int n) { return n * n; }) << '\n';
    out << "lengths: "
        << joinMapped(std::vector<std::string>{"space", "game", "lambda"}, [](const std::string& w) {
               return w.size();
           })
        << '\n';

    // Template lambda: the explicit type parameter lets us name the element type.
    auto sumAs = []<typename T>(const std::vector<T>& v) {
        return std::accumulate(v.begin(), v.end(), T{});
    };
    out << "sumAs<double>({0.5, 1.5, 2.0}) = " << sumAs(std::vector{0.5, 1.5, 2.0}) << '\n';
    out << "sizeInBits(double) = " << sizeInBits(1.0) << '\n';

    constexpr auto hypotSq = [](auto a, auto b) constexpr { return a * a + b * b; };
    constexpr auto r = hypotSq(3, 4);
    static_assert(r == 25);
    out << "constexpr lambda hypotSq(3, 4) = " << r << '\n';

    // C++20: stateless lambdas are default-constructible and may appear in unevaluated contexts.
    std::set<int, decltype([](int a, int b) { return a > b; })> descending{3, 1, 4, 1, 5, 9};
    out << "set ordered by a decltype(lambda) comparator:";
    for (int v : descending) {
        out << ' ' << v;
    }
    out << '\n' << std::noboolalpha;
}

void demonstrateStatefulLambdas(std::ostream& out) {
    out << "\n--- Stateful (mutable) lambdas ---\n";
    auto missionId = makeCounter(1);
    for (const char* type : {"Exploration", "Combat", "Trade"}) {
        out << "  Mission-" << missionId() << '-' << type << '\n';
    }

    auto average = makeRunningAverage();
    out << "Running average of fuel readings:";
    for (double reading : {85.5, 92.0, 23.1, 67.8}) {
        out << ' ' << average(reading);
    }
    out << '\n';

    auto fuelOk = makeRangeValidator(20.0, 100.0);
    auto crewOk = makeRangeValidator(10, 500);
    out << std::boolalpha << "fuelOk(75.5)=" << fuelOk(75.5) << " fuelOk(15.2)=" << fuelOk(15.2)
        << " crewOk(150)=" << crewOk(150) << " crewOk(600)=" << crewOk(600) << std::noboolalpha << '\n';
}

void demonstrateFunctionalUtilities(std::ostream& out) {
    out << "\n--- Functional utilities ---\n";
    auto square = [](double x) { return x * x; };
    auto addTen = [](double x) { return x + 10.0; };
    auto halve = [](double x) { return x / 2.0; };
    out << "compose(square, addTen, halve)(8) = " << compose(square, addTen, halve)(8.0) << '\n';
    out << "pipeline(square, addTen, halve)(8) = " << pipeline(square, addTen, halve)(8.0) << '\n';

    auto add3 = [](int a, int b, int c) { return a + b + c; };
    auto add5 = curry(add3)(5);
    out << "curry(add3)(5)(10)(20) = " << add5(10)(20) << ", curry(add3)(5)(10, 20) = " << add5(10, 20) << '\n';

    using Reading = std::variant<int, double, std::string>;
    const std::vector<Reading> readings{42, 3.5, std::string{"offline"}};
    for (const auto& r : readings) {
        out << "  visit: "
            << std::visit(Overloaded{[](int i) { return "int " + std::to_string(i); },
                                     [](double d) { return "double " + std::to_string(d); },
                                     [](const std::string& s) { return "string " + s; }},
                          r)
            << '\n';
    }

    const Fix fib{[](const auto& self, int n) -> long long { return n < 2 ? n : self(n - 1) + self(n - 2); }};
    out << "Fix (Y-combinator) fib(20) = " << fib(20) << ", factorialFix(15) = " << factorialFix(15) << '\n';

    Memoized<int, long long> slowSquare([](const int& n) { return static_cast<long long>(n) * n; });
    for (int n : {12, 7, 12, 12, 7}) {
        (void)slowSquare(n);
    }
    out << "Memoized: " << slowSquare.misses() << " computations, " << slowSquare.hits() << " cache hits\n";
}

void demonstrateEventSystem(std::ostream& out) {
    out << "\n--- Event system with std::function ---\n";
    EventBus bus;
    std::vector<std::string> log;
    int resourceTotal = 0;

    bus.subscribe("ship_launched", [&log](const std::string& p) { log.push_back("launch:" + p); });
    const auto alertId = bus.subscribe("ship_launched", [&out](const std::string& p) {
        out << "  [alert] " << p << " has launched\n";
    });
    bus.subscribe("resource_found", [&resourceTotal](const std::string& p) {
        resourceTotal += static_cast<int>(p.size());
    });

    bus.emit("ship_launched", "USS Explorer");
    bus.unsubscribe(alertId);
    bus.emit("ship_launched", "USS Guardian");
    bus.emit("resource_found", "dilithium");
    const auto unheard = bus.emit("unknown_event", "ignored");
    out << "Logged " << log.size() << " launches, resource score " << resourceTotal << ", unknown event reached "
        << unheard << " handlers\n";
}

void demonstrateAsyncLambdas(std::ostream& out) {
    out << "\n--- Lambdas with std::async ---\n";
    std::vector<int> numbers(1000);
    std::iota(numbers.begin(), numbers.end(), 1);
    out << "parallelSum(1..1000, 4 tasks) = " << parallelSum(numbers, 4) << '\n';

    auto product = std::async(std::launch::async, [first = numbers.begin(), last = numbers.begin() + 10] {
        return std::accumulate(first, last, 1LL, std::multiplies<>{});
    });
    out << "product of 1..10 on another thread = " << product.get() << '\n';
}

void demonstrateAllLambdas(std::ostream& out) {
    out << "\n=== Lambda Expressions ===\n";
    demonstrateBasicLambdas(out);
    demonstrateCaptureModes(out);
    demonstrateStlLambdas(out);
    demonstrateGenericLambdas(out);
    demonstrateStatefulLambdas(out);
    demonstrateFunctionalUtilities(out);
    demonstrateEventSystem(out);
    demonstrateAsyncLambdas(out);
}

}  // namespace CppVerseHub::Modern::LambdaExpressions
