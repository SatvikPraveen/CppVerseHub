// Tests for stl_showcase/Containers.hpp
#include "stl_showcase/Containers.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <algorithm>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using namespace CppVerseHub::STL;

TEST_CASE("Spacecraft combat rating and name-based hashing", "[containers][spacecraft]") {
    const Spacecraft ship{"Test", "Frigate", 100.0, 10, 200.0, 50.0};
    REQUIRE(ship.combatRating() == 50.0 * 2.0 * 2.0);
    const Spacecraft same_name{"Test", "Other", 1.0, 1, 1.0, 1.0};
    REQUIRE(SpacecraftNameHash{}(ship) == SpacecraftNameHash{}(same_name));
    REQUIRE(SpacecraftNameEqual{}(ship, same_name));
    REQUIRE_FALSE(ship == same_name);
}

TEST_CASE("FleetRoster rejects duplicate names and preserves insertion order", "[containers][roster]") {
    FleetRoster roster(sampleFleet());
    REQUIRE(roster.size() == 5);
    REQUIRE_FALSE(roster.add({"Enterprise", "Dup", 0, 0, 0, 0}));
    REQUIRE(roster.add({"Rocinante", "Corvette", 500.0, 4, 1500.0, 300.0}));
    REQUIRE(roster.size() == 6);
    REQUIRE(roster.ships().front().name == "Enterprise");
    REQUIRE(roster.ships().back().name == "Rocinante");

    FleetRoster with_dups({{"A", "x", 0, 1, 0, 0}, {"A", "y", 0, 2, 0, 0}});
    REQUIRE(with_dups.size() == 1);
    REQUIRE(with_dups.ships()[0].class_type == "x");
}

TEST_CASE("FleetRoster find, remove and removeUndercrewed", "[containers][roster]") {
    FleetRoster roster(sampleFleet());
    const Spacecraft* falcon = roster.find("Falcon");
    REQUIRE(falcon != nullptr);
    REQUIRE(falcon->crew_size == 6);
    REQUIRE(roster.find("Nope") == nullptr);

    REQUIRE(roster.remove("Falcon"));
    REQUIRE_FALSE(roster.remove("Falcon"));
    REQUIRE(roster.find("Falcon") == nullptr);

    REQUIRE(roster.removeUndercrewed(10) == 1); // Serenity (9)
    REQUIRE(roster.size() == 3);
    REQUIRE(roster.totalCrew() == 400 + 150 + 5000);
}

TEST_CASE("FleetRoster sorted copies and class filtering", "[containers][roster]") {
    FleetRoster roster(sampleFleet());
    const auto ranked = roster.sortedByCombatRating();
    REQUIRE(ranked.size() == roster.size());
    for (std::size_t i = 1; i < ranked.size(); ++i) {
        REQUIRE(ranked[i - 1].combatRating() >= ranked[i].combatRating());
    }
    REQUIRE(ranked.front().name == "Galactica");
    REQUIRE(roster.ships().front().name == "Enterprise"); // original order untouched
    REQUIRE(roster.namesOfClass("Frigate") == std::vector<std::string>{"Normandy"});
    REQUIRE(roster.namesOfClass("Dreadnought").empty());
    REQUIRE(FleetRoster{}.empty());
    REQUIRE(FleetRoster{}.totalCrew() == 0);
}

TEST_CASE("LruCache evicts the least recently used entry", "[containers][lru]") {
    LruCache<std::string, int> cache(2);
    REQUIRE(cache.empty());
    REQUIRE_FALSE(cache.put("a", 1).has_value());
    REQUIRE_FALSE(cache.put("b", 2).has_value());
    REQUIRE(cache.get("a") == 1); // a becomes most recent
    const auto evicted = cache.put("c", 3);
    REQUIRE(evicted == "b");
    REQUIRE_FALSE(cache.contains("b"));
    REQUIRE(cache.keysByRecency() == std::vector<std::string>{"c", "a"});
    REQUIRE(cache.size() == 2);
    REQUIRE(cache.capacity() == 2);
}

TEST_CASE("LruCache update, erase, misses and invalid capacity", "[containers][lru]") {
    LruCache<int, std::string> cache(3);
    cache.put(1, "one");
    cache.put(2, "two");
    REQUIRE_FALSE(cache.put(1, "uno").has_value()); // update, no eviction
    REQUIRE(cache.get(1) == "uno");
    REQUIRE(cache.keysByRecency().front() == 1);
    REQUIRE_FALSE(cache.get(42).has_value());
    REQUIRE(cache.erase(2));
    REQUIRE_FALSE(cache.erase(2));
    REQUIRE(cache.size() == 1);
    REQUIRE_THROWS_AS((LruCache<int, int>(0)), std::invalid_argument);
}

TEST_CASE("LruCache copies are independent and moves keep working", "[containers][lru]") {
    LruCache<int, int> original(2);
    original.put(1, 10);
    original.put(2, 20);

    LruCache<int, int> copy(original);
    REQUIRE(copy.get(1) == 10); // splices inside copy only
    copy.put(3, 30);            // evicts 2 from copy
    REQUIRE(original.contains(2));
    REQUIRE(original.keysByRecency() == std::vector<int>{2, 1});
    REQUIRE(copy.keysByRecency() == std::vector<int>{3, 1});

    LruCache<int, int> assigned(5);
    assigned = original;
    REQUIRE(assigned.capacity() == 2);
    REQUIRE(assigned.get(2) == 20);

    LruCache<int, int> moved(std::move(copy));
    REQUIRE(moved.get(3) == 30);
    moved.put(4, 40);
    REQUIRE(moved.keysByRecency() == std::vector<int>{4, 3});
}

TEST_CASE("LruCache supports move-only values", "[containers][lru]") {
    LruCache<int, std::shared_ptr<int>> cache(1);
    cache.put(1, std::make_shared<int>(5));
    const auto value = cache.get(1);
    REQUIRE(value.has_value());
    REQUIRE(**value == 5);
}

TEST_CASE("slidingWindowMaximum matches a brute-force reference", "[containers][deque]") {
    const std::vector<int> data{3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5, 8, 9, 7, 9};
    const std::size_t k = GENERATE(1U, 2U, 3U, 5U, 15U);
    const auto fast = slidingWindowMaximum(data, k);
    std::vector<int> brute;
    for (std::size_t i = 0; i + k <= data.size(); ++i) {
        brute.push_back(*std::max_element(data.begin() + static_cast<std::ptrdiff_t>(i),
                                          data.begin() + static_cast<std::ptrdiff_t>(i + k)));
    }
    REQUIRE(fast == brute);
}

TEST_CASE("slidingWindowMaximum edge cases", "[containers][deque]") {
    const std::vector<int> data{1, 2, 3};
    REQUIRE(slidingWindowMaximum(data, 4).empty());
    REQUIRE(slidingWindowMaximum(std::vector<int>{}, 1).empty());
    REQUIRE_THROWS_AS(slidingWindowMaximum(data, 0), std::invalid_argument);
}

TEST_CASE("makeSquares is usable at compile time", "[containers][array]") {
    constexpr auto squares = makeSquares<5>();
    STATIC_REQUIRE(squares.size() == 5);
    STATIC_REQUIRE(squares[4] == 16);
    REQUIRE(squares[0] == 0);
    REQUIRE(makeSquares<0>().empty());
}

TEST_CASE("groupPlanetsBySystem orders systems and names", "[containers][map]") {
    const auto groups = groupPlanetsBySystem(samplePlanets());
    REQUIRE(groups.size() == 3);
    REQUIRE(groups.begin()->first == "Alpha Centauri");
    REQUIRE(groups.at("Sol") == std::vector<std::string>{"Earth", "Europa", "Mars"});
    REQUIRE(groups.at("Alpha Centauri") == std::vector<std::string>{"Alpha Prime", "Proxima b"});
}

TEST_CASE("multimap defense index supports duplicate keys and range queries", "[containers][multimap]") {
    const auto index = indexPlanetsByDefense(samplePlanets());
    REQUIRE(index.size() == 6);
    REQUIRE(index.count(7) == 2);
    REQUIRE(planetsWithDefenseBetween(index, 4, 7) ==
            std::vector<std::string>{"Mars", "Kepler-442b", "Earth", "Alpha Prime"});
    REQUIRE(planetsWithDefenseBetween(index, 8, 10).empty());
    REQUIRE(planetsWithDefenseBetween(index, 7, 1).empty());
}

TEST_CASE("wordFrequencies and topWords", "[containers][unordered_map]") {
    const auto freq = wordFrequencies("The fleet, the FLEET and the station!");
    REQUIRE(freq.at("the") == 3);
    REQUIRE(freq.at("fleet") == 2);
    REQUIRE(freq.at("station") == 1);
    REQUIRE(freq.size() == 4);
    const auto top = topWords(freq, 3);
    REQUIRE(top.size() == 3);
    REQUIRE(top[0] == std::pair<std::string, std::size_t>{"the", 3});
    REQUIRE(top[1].first == "fleet");
    REQUIRE(top[2].first == "and"); // ties broken alphabetically: and < station
    REQUIRE(topWords(freq, 100).size() == 4);
    REQUIRE(wordFrequencies("  123 ...").empty());
}

TEST_CASE("isBalanced detects nesting errors", "[containers][stack]") {
    REQUIRE(isBalanced(""));
    REQUIRE(isBalanced("a(b[c]{d})e"));
    REQUIRE(isBalanced("{[()()]}"));
    REQUIRE_FALSE(isBalanced("([)]"));
    REQUIRE_FALSE(isBalanced("(("));
    REQUIRE_FALSE(isBalanced("())"));
    REQUIRE_FALSE(isBalanced("}"));
}

TEST_CASE("breadthFirstOrder visits level by level without repeats", "[containers][queue]") {
    const std::map<int, std::vector<int>> graph{{0, {1, 2}}, {1, {3, 0}}, {2, {3, 4}}, {3, {5}}, {4, {5}}};
    REQUIRE(breadthFirstOrder(graph, 0) == std::vector<int>{0, 1, 2, 3, 4, 5});
    REQUIRE(breadthFirstOrder(graph, 4) == std::vector<int>{4, 5});
    REQUIRE(breadthFirstOrder(graph, 99) == std::vector<int>{99});
}

TEST_CASE("TaskScheduler serves by priority with FIFO tie-breaking", "[containers][priority_queue]") {
    TaskScheduler scheduler;
    REQUIRE(scheduler.empty());
    REQUIRE(scheduler.peek() == nullptr);
    REQUIRE_FALSE(scheduler.next().has_value());

    scheduler.submit("low-1", 1);
    scheduler.submit("high", 9);
    scheduler.submit("low-2", 1);
    scheduler.submit("mid", 5);
    scheduler.submit("low-3", 1);
    REQUIRE(scheduler.size() == 5);
    REQUIRE(scheduler.peek()->name == "high");

    std::vector<std::string> order;
    while (auto task = scheduler.next()) {
        order.push_back(task->name);
    }
    REQUIRE(order == std::vector<std::string>{"high", "mid", "low-1", "low-2", "low-3"});
}
