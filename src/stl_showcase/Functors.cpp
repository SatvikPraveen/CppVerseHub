/**
 * @file Functors.cpp
 * @brief Implementation of the callable-object showcase.
 */
#include "stl_showcase/Functors.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <iterator>
#include <limits>
#include <numeric>
#include <set>
#include <stdexcept>

namespace CppVerseHub::STL {

std::vector<Starship> sampleStarships() {
    return {
        {"Enterprise", "Constitution", 85.0, 8.0, 430, 950.0}, {"Defiant", "Escort", 95.0, 9.5, 50, 1100.0},
        {"Reliant", "Miranda", 40.0, 6.0, 300, 600.0},         {"Voyager", "Intrepid", 70.0, 9.9, 150, 800.0},
        {"Rocinante", "Corvette", 60.0, 3.0, 4, 400.0},
    };
}

FleetStats FleetStatsAccumulator::operator()(FleetStats stats, const Starship& ship) const noexcept {
    ++stats.ship_count;
    stats.total_crew += ship.crew;
    stats.total_firepower += ship.firepower;
    stats.max_warp = std::max(stats.max_warp, ship.warp_speed);
    return stats;
}

std::string StarshipNameGenerator::operator()() {
    const int serial = next_++;
    ++generated_;
    std::string digits = std::to_string(serial);
    if (digits.size() < 3) {
        digits.insert(0, 3 - digits.size(), '0');
    }
    return prefix_ + "-" + digits;
}

std::uint64_t memoizedFibonacci(unsigned n) {
    constexpr unsigned max_index = 93; // fib(94) overflows std::uint64_t
    if (n > max_index) {
        throw std::out_of_range("memoizedFibonacci: n > 93 overflows 64 bits");
    }
    std::array<std::uint64_t, max_index + 1> memo{};
    std::array<bool, max_index + 1> known{};
    // A generic lambda receives itself as a parameter to recurse (pre-C++23 "deducing this").
    const auto fib = [&memo, &known](const auto& self, unsigned k) -> std::uint64_t {
        if (k < 2) {
            return k;
        }
        if (!known[k]) {
            memo[k] = self(self, k - 1) + self(self, k - 2);
            known[k] = true;
        }
        return memo[k];
    };
    return fib(fib, n);
}

double calculateDistance(double x1, double y1, double x2, double y2) noexcept {
    return std::hypot(x2 - x1, y2 - y1);
}

double calculateWarpTime(const Starship& ship, double distance) noexcept {
    if (ship.warp_speed <= 0.0) {
        return std::numeric_limits<double>::infinity();
    }
    return distance / (ship.warp_speed * ship.warp_speed * ship.warp_speed);
}

std::vector<double> warpTimesTo(std::span<const Starship> ships, double distance) {
    using std::placeholders::_1;
    const auto time_for = std::bind(calculateWarpTime, _1, distance);
    std::vector<double> times;
    times.reserve(ships.size());
    std::ranges::transform(ships, std::back_inserter(times), time_for);
    return times;
}

std::vector<std::string> namesNotReady(std::span<const Starship> ships, const IsCombatReady& ready) {
    std::vector<std::string> names;
    const auto get_name = std::mem_fn(&Starship::name);
    for (const auto& ship : ships) {
        if (std::not_fn(ready)(ship)) {
            names.push_back(get_name(ship));
        }
    }
    return names;
}

EventDispatcher::Token EventDispatcher::subscribe(std::string event, Handler handler) {
    if (!handler) {
        throw std::invalid_argument("EventDispatcher: empty handler");
    }
    const Token token = next_token_++;
    handlers_[std::move(event)].emplace_back(token, std::move(handler));
    return token;
}

bool EventDispatcher::unsubscribe(Token token) {
    for (auto it = handlers_.begin(); it != handlers_.end(); ++it) {
        auto& list = it->second;
        const auto erased = std::erase_if(list, [token](const auto& entry) { return entry.first == token; });
        if (erased > 0) {
            if (list.empty()) {
                handlers_.erase(it);
            }
            return true;
        }
    }
    return false;
}

std::size_t EventDispatcher::dispatch(std::string_view event, const Starship& ship) const {
    const auto it = handlers_.find(event);
    if (it == handlers_.end()) {
        return 0;
    }
    for (const auto& [token, handler] : it->second) {
        std::invoke(handler, ship);
    }
    return it->second.size();
}

std::size_t EventDispatcher::handlerCount(std::string_view event) const {
    const auto it = handlers_.find(event);
    return it == handlers_.end() ? 0 : it->second.size();
}

// ------------------------------------------------------------------------------ demonstrations

void demonstrateFunctionObjects(std::ostream& out) {
    out << "\n=== Function Objects ===\n";
    auto ships = sampleStarships();
    const IsCombatReady ready;
    out << "combat ready (functor predicate): " << std::ranges::count_if(ships, ready) << " of "
        << ships.size() << '\n';

    std::ranges::sort(ships, ByCombatEffectivenessDesc{});
    out << "sorted by effectiveness (comparator functor):";
    for (const auto& ship : ships) {
        out << ' ' << ship.name;
    }
    out << '\n';

    const auto stats = std::accumulate(ships.begin(), ships.end(), FleetStats{}, FleetStatsAccumulator{});
    out << "accumulator functor: " << stats.ship_count << " ships, crew " << stats.total_crew << ", avg "
        << stats.averageCrew() << ", max warp " << stats.max_warp << '\n';

    StarshipNameGenerator generator("NCC");
    std::vector<std::string> names;
    std::generate_n(std::back_inserter(names), 3, std::ref(generator)); // std::ref keeps the state
    out << "generator via std::ref produced " << generator.generated() << " names:";
    for (const auto& name : names) {
        out << ' ' << name;
    }
    out << '\n';

    const SharedCallCounter counter([](const Starship& s) { return s.crew > 100; });
    const auto big_crews = std::ranges::count_if(ships, counter); // copied, yet the count is shared
    out << "shared-state predicate counted " << counter.calls() << " calls, " << big_crews << " matches\n";
}

void demonstrateLambdas(std::ostream& out) {
    out << "\n=== Lambda Expressions ===\n";
    constexpr auto triple = makeMultiplier(3);
    static_assert(triple(14) == 42);
    out << "constexpr capture-by-value closure: triple(14) = " << triple(14) << '\n';

    auto counter = makeCounter(10);
    const int first = counter();
    const int second = counter();
    out << "mutable init-capture counter: " << first << ", " << second << '\n';

    int total_crew = 0;
    const auto ships = sampleStarships();
    std::ranges::for_each(ships, [&total_crew](const Starship& s) { total_crew += s.crew; });
    out << "capture-by-reference accumulation: crew = " << total_crew << '\n';

    const auto is_fast = [](const auto& s) { return s.warp_speed > 9.0; }; // generic lambda
    out << "generic lambda: fast ships = " << countWhere(ships, is_fast) << '\n';

    const auto inc = [](int x) { return x + 1; };
    const auto square = [](int x) { return x * x; };
    out << "compose(square, inc)(4) = " << compose(square, inc)(4) << '\n';

    Memoized<std::uint64_t, unsigned> slow_square([](const unsigned& x) { return std::uint64_t{x} * x; });
    (void)slow_square(12);
    (void)slow_square(12);
    out << "Memoized: hits " << slow_square.hits() << ", misses " << slow_square.misses() << '\n';
    out << "recursive generic lambda fib(50) = " << memoizedFibonacci(50) << '\n';
}

void demonstrateStandardFunctionObjects(std::ostream& out) {
    out << "\n=== Standard Function Objects ===\n";
    std::vector<int> values{5, 2, 9, 1, 7};
    out << "accumulate with std::plus<>: " << std::accumulate(values.begin(), values.end(), 0, std::plus<>{})
        << ", std::multiplies<>: " << std::accumulate(values.begin(), values.end(), 1, std::multiplies<>{})
        << '\n';
    std::ranges::sort(values, std::greater<>{});
    out << "sort with std::greater<>:";
    for (const int v : values) {
        out << ' ' << v;
    }
    out << '\n';
    std::vector<int> negated(values.size());
    std::ranges::transform(values, negated.begin(), std::negate<>{});
    out << "transform with std::negate<>: first = " << negated.front() << '\n';

    const std::set<std::string, std::less<>> registry{"Defiant", "Enterprise", "Voyager"};
    out << std::boolalpha << "std::less<> heterogeneous find(string_view): "
        << (registry.find(std::string_view{"Voyager"}) != registry.end()) << '\n';

    const auto ships = sampleStarships();
    out << "std::not_fn(IsCombatReady): not ready =";
    for (const auto& name : namesNotReady(ships, IsCombatReady{})) {
        out << ' ' << name;
    }
    out << '\n';
    const auto effectiveness = std::mem_fn(&Starship::combatEffectiveness);
    out << "std::mem_fn on member function: " << ships.front().name << " -> " << effectiveness(ships.front())
        << '\n';
}

void demonstrateFunctionBinding(std::ostream& out) {
    out << "\n=== Function Binding ===\n";
    using std::placeholders::_1;
    using std::placeholders::_2;
    const auto from_origin = std::bind(calculateDistance, 0.0, 0.0, _1, _2);
    out << "std::bind distance from origin to (3,4): " << from_origin(3.0, 4.0) << '\n';
    const auto from_base = std::bind_front(calculateDistance, 1.0, 1.0);
    out << "std::bind_front distance from (1,1) to (4,5): " << from_base(4.0, 5.0) << '\n';

    const auto ships = sampleStarships();
    const auto times = warpTimesTo(ships, 1000.0);
    out << "bound warp times to 1000 ly: " << ships.front().name << " = " << times.front() << '\n';

    EventDispatcher dispatcher;
    int alerts = 0;
    std::vector<std::string> log;
    dispatcher.subscribe("red_alert", [&alerts](const Starship&) { ++alerts; });
    const auto token = dispatcher.subscribe("red_alert",
                                            [&log](const Starship& s) { log.push_back(s.name); });
    const auto invoked = dispatcher.dispatch("red_alert", ships.front());
    dispatcher.unsubscribe(token);
    out << "std::function dispatcher invoked " << invoked << " handlers; after unsubscribe "
        << dispatcher.handlerCount("red_alert") << " remain\n";

    out << "std::invoke on data member: " << std::invoke(&Starship::crew, ships.back()) << " crew aboard "
        << ships.back().name << '\n';
}

void runFunctorsDemo(std::ostream& out) {
    demonstrateFunctionObjects(out);
    demonstrateLambdas(out);
    demonstrateStandardFunctionObjects(out);
    demonstrateFunctionBinding(out);
}

} // namespace CppVerseHub::STL
