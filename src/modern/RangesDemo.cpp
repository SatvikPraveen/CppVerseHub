/**
 * @file RangesDemo.cpp
 * @brief Implementation of the ranges showcase (see RangesDemo.hpp).
 */
#include "modern/RangesDemo.hpp"

#include <algorithm>
#include <functional>
#include <numeric>
#include <random>
#include <tuple>

namespace CppVerseHub::Modern::Ranges {

namespace views = std::views;
namespace rng = std::ranges;

std::vector<Planet> generatePlanets() {
    return {
        {1, "Mercury", "Sol", 0.39, 0, 120.0, false},
        {2, "Venus", "Sol", 0.72, 0, 200.0, false},
        {3, "Earth", "Sol", 1.00, 8'000'000'000LL, 950.0, true},
        {4, "Mars", "Sol", 1.52, 2'000'000LL, 410.0, true},
        {5, "Jupiter", "Sol", 5.20, 0, 780.0, false},
        {6, "Proxima-b", "Alpha Centauri", 0.05, 15'000'000LL, 520.0, true},
        {7, "Proxima-d", "Alpha Centauri", 0.03, 0, 90.0, false},
        {8, "Kepler-442b", "Kepler-442", 0.41, 50'000'000LL, 640.0, true},
        {9, "Kepler-442c", "Kepler-442", 1.20, 0, 300.0, false},
        {10, "Kepler-442d", "Kepler-442", 2.10, 1'000LL, 150.0, false},
    };
}

std::vector<Fleet> generateFleets() {
    return {
        {1, "Zhang", 12, 85.0, true}, {2, "Okafor", 4, 95.0, true},  {3, "Ivanova", 20, 30.0, true},
        {4, "Reyes", 15, 70.0, false}, {5, "Tanaka", 9, 60.0, true}, {6, "Novak", 18, 99.0, true},
    };
}

std::vector<Mission> generateMissions() {
    return {
        {1, "Exploration", 2, 40.0}, {2, "Combat", 5, 10.0}, {3, "Trade", 1, 90.0},  {4, "Rescue", 5, 70.0},
        {5, "Research", 3, 0.0},     {6, "Patrol", 2, 15.0}, {7, "Colonize", 4, 55.0}, {8, "Combat", 5, 10.0},
    };
}

std::vector<std::string> habitablePlanetNames(const std::vector<Planet>& planets) {
    return toVector(planets | views::filter(&Planet::habitable) | views::transform(&Planet::name));
}

std::vector<std::string> topByPopulation(std::vector<Planet> planets, std::size_t n) {
    rng::sort(planets, rng::greater{}, &Planet::population);
    return toVector(planets | views::take(static_cast<std::ptrdiff_t>(n)) | views::transform(&Planet::name));
}

long long totalPopulation(const std::vector<Planet>& planets) {
    auto pops = planets | views::transform(&Planet::population) | views::common;
    return std::accumulate(pops.begin(), pops.end(), 0LL);
}

std::vector<int> readyFleetIds(const std::vector<Fleet>& fleets, double minFuel, int minShips) {
    auto ready = fleets | views::filter([minFuel, minShips](const Fleet& f) {
                     return f.active && f.fuel >= minFuel && f.ships >= minShips;
                 }) |
                 views::transform(&Fleet::id);
    return toVector(ready);
}

std::vector<int> missionIdsByUrgency(std::vector<Mission> missions) {
    // Stable sort by the secondary key, then by the primary key: equal priorities keep progress order.
    rng::stable_sort(missions, rng::less{}, &Mission::progress);
    rng::stable_sort(missions, rng::greater{}, &Mission::priority);
    return toVector(missions | views::transform(&Mission::id));
}

std::map<std::string, std::vector<std::string>> planetsBySystem(const std::vector<Planet>& planets) {
    std::map<std::string, std::vector<std::string>> groups;
    rng::for_each(planets, [&groups](const Planet& p) { groups[p.system].push_back(p.name); });
    return groups;
}

std::vector<std::string> splitWords(std::string_view text, char delimiter) {
    std::vector<std::string> words;
    for (auto token : text | views::split(delimiter)) {
        if (!rng::empty(token)) {
            words.emplace_back(rng::begin(token), rng::end(token));
        }
    }
    return words;
}

std::vector<int> flatten(const std::vector<std::vector<int>>& nested) {
    return toVector(nested | views::join);
}

std::vector<long long> squaresOfOdds(std::size_t count) {
    auto odds = views::iota(1LL) | views::filter([](long long x) { return x % 2 != 0; }) |
                views::transform([](long long x) { return x * x; }) |
                views::take(static_cast<std::ptrdiff_t>(count));
    return toVector(odds);
}

std::size_t countEvaluationsForFirst(const std::vector<int>& data, std::size_t first) {
    std::size_t evaluations = 0;
    auto pipeline = data | views::transform([&evaluations](int x) {
                        ++evaluations;
                        return x * 10;
                    }) |
                    views::take(static_cast<std::ptrdiff_t>(first));
    for (int v : pipeline) {
        (void)v;
    }
    return evaluations;
}

std::pair<std::vector<int>, std::vector<int>> splitAtFirstNotBelow(const std::vector<int>& data, int limit) {
    auto below = [limit](int x) { return x < limit; };
    return {toVector(data | views::take_while(below)), toVector(data | views::drop_while(below))};
}

std::vector<int> makeRandomValues(std::size_t n, std::uint32_t seed) {
    std::mt19937 gen(seed);
    std::uniform_int_distribution<int> dist(0, 999);
    std::vector<int> values(n);
    rng::generate(values, [&] { return dist(gen); });
    return values;
}

long long sumSquaresOfEvensRanges(const std::vector<int>& data) {
    auto squares = data | views::filter([](int x) { return x % 2 == 0; }) |
                   views::transform([](int x) { return static_cast<long long>(x) * x; }) | views::common;
    return std::accumulate(squares.begin(), squares.end(), 0LL);
}

long long sumSquaresOfEvensLoop(const std::vector<int>& data) {
    long long sum = 0;
    for (int x : data) {
        if (x % 2 == 0) {
            sum += static_cast<long long>(x) * x;
        }
    }
    return sum;
}

// ===== SHOWCASES =====

namespace {

template <typename R>
void printRange(std::ostream& out, std::string_view label, R&& r) {
    out << "  " << label << ':';
    for (auto&& v : r) {
        out << ' ' << v;
    }
    out << '\n';
}

}  // namespace

void demonstrateBasicRanges(std::ostream& out) {
    out << "\n--- Basic view pipelines ---\n";
    const std::vector<int> numbers{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    printRange(out, "evens", numbers | views::filter([](int x) { return x % 2 == 0; }));
    printRange(out, "squares", numbers | views::transform([](int x) { return x * x; }));
    printRange(out, "evens squared, first 3",
               numbers | views::filter([](int x) { return x % 2 == 0; }) |
                   views::transform([](int x) { return x * x; }) | views::take(3));
    printRange(out, "drop 9", numbers | views::drop(9));
    printRange(out, "reversed", numbers | views::reverse | views::take(4));
    printRange(out, "iota(10, 15)", views::iota(10, 15));
    printRange(out, "squares of odds", squaresOfOdds(5));
    out << "  ranges::count_if(> 6) = " << rng::count_if(numbers, [](int x) { return x > 6; })
        << ", ranges::max = " << rng::max(numbers) << '\n';
}

void demonstrateDomainQueries(std::ostream& out) {
    out << "\n--- Domain queries with projections ---\n";
    const auto planets = generatePlanets();
    printRange(out, "habitable", habitablePlanetNames(planets));
    printRange(out, "top 3 by population", topByPopulation(planets, 3));
    out << "  total population: " << totalPopulation(planets) << '\n';
    const auto richest = rng::max_element(planets, {}, &Planet::resourceValue);
    out << "  richest (max_element with projection): " << richest->name << '\n';
    out << "  any planet beyond 5 AU? " << std::boolalpha
        << rng::any_of(planets, [](double d) { return d > 5.0; }, &Planet::distanceAu) << std::noboolalpha
        << '\n';
    for (const auto& [system, names] : planetsBySystem(planets)) {
        out << "  " << system << ": " << names.size() << " planets\n";
    }
    printRange(out, "fleets ready (fuel>=60, ships>=10)", readyFleetIds(generateFleets(), 60.0, 10));
    printRange(out, "missions by urgency", missionIdsByUrgency(generateMissions()));
}

void demonstrateAdvancedAdaptors(std::ostream& out) {
    out << "\n--- keys / values / elements / split / join / take_while ---\n";
    const std::map<std::string, int> shipsPerSector{{"Alpha", 12}, {"Beta", 7}, {"Gamma", 3}};
    printRange(out, "keys", shipsPerSector | views::keys);
    printRange(out, "values", shipsPerSector | views::values);
    const std::vector<std::tuple<int, std::string, double>> logs{{1, "warp", 0.9}, {2, "dock", 0.4}};
    printRange(out, "elements<1>", logs | views::elements<1>);
    printRange(out, "split", splitWords("scan,,mine,trade,colonize", ','));
    printRange(out, "join", flatten({{1, 2}, {}, {3}, {4, 5, 6}}));
    const auto [prefix, rest] = splitAtFirstNotBelow({1, 3, 5, 8, 2, 9}, 6);
    printRange(out, "take_while(<6)", prefix);
    printRange(out, "drop_while(<6)", rest);
}

void demonstrateCustomView(std::ostream& out) {
    out << "\n--- Custom view: EveryNthView ---\n";
    std::vector<int> data(12);
    std::iota(data.begin(), data.end(), 1);
    printRange(out, "every 3rd", data | everyNth(3));
    printRange(out, "every 5th of iota(0,20)", views::iota(0, 20) | everyNth(5));
    printRange(out, "every 2nd, squared",
               data | everyNth(2) | views::transform([](int x) { return x * x; }));
    auto planets = generatePlanets();
    printRange(out, "every 4th planet", planets | everyNth(4) | views::transform(&Planet::name));
}

void demonstrateLaziness(std::ostream& out) {
    out << "\n--- Lazy evaluation ---\n";
    const std::vector<int> data(1000, 1);
    out << "  take(3) over a transform of 1000 elements evaluated the transform "
        << countEvaluationsForFirst(data, 3) << " times\n";
    const auto values = makeRandomValues(10'000, 42);
    out << "  sum of squares of evens (ranges) = " << sumSquaresOfEvensRanges(values)
        << ", (loop) = " << sumSquaresOfEvensLoop(values) << '\n';
}

void demonstrateAllRanges(std::ostream& out) {
    out << "\n=== C++20 Ranges ===\n";
    demonstrateBasicRanges(out);
    demonstrateDomainQueries(out);
    demonstrateAdvancedAdaptors(out);
    demonstrateCustomView(out);
    demonstrateLaziness(out);
}

}  // namespace CppVerseHub::Modern::Ranges
