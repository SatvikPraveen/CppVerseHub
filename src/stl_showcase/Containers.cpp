/**
 * @file Containers.cpp
 * @brief Implementation of the STL container showcase.
 */
#include "stl_showcase/Containers.hpp"

#include <algorithm>
#include <cctype>
#include <deque>
#include <forward_list>
#include <iterator>
#include <numeric>
#include <set>
#include <stack>
#include <unordered_set>

namespace CppVerseHub::STL {

std::vector<Spacecraft> sampleFleet() {
    return {
        {"Enterprise", "Heavy Cruiser", 5000.0, 400, 650.0, 1200.0},
        {"Falcon", "Light Freighter", 1050.0, 6, 1200.0, 400.0},
        {"Serenity", "Transport", 900.0, 9, 800.0, 200.0},
        {"Normandy", "Frigate", 2500.0, 150, 900.0, 800.0},
        {"Galactica", "Battlestar", 15000.0, 5000, 300.0, 2500.0},
    };
}

std::vector<Planet> samplePlanets() {
    return {
        {"Earth", "Sol", 8.0, 7, {"Water", "Iron", "Food"}},
        {"Mars", "Sol", 0.5, 4, {"Iron", "Silicon"}},
        {"Proxima b", "Alpha Centauri", 0.1, 2, {"Ice"}},
        {"Kepler-442b", "Kepler-442", 1.2, 5, {"Water", "Titanium"}},
        {"Alpha Prime", "Alpha Centauri", 2.4, 7, {"Deuterium", "Gold"}},
        {"Europa", "Sol", 0.05, 1, {"Water", "Ice"}},
    };
}

// ---------------------------------------------------------------------------------------------
// FleetRoster
// ---------------------------------------------------------------------------------------------

FleetRoster::FleetRoster(std::vector<Spacecraft> ships) {
    ships_.reserve(ships.size());
    for (auto& ship : ships) {
        add(std::move(ship));
    }
}

bool FleetRoster::add(Spacecraft ship) {
    if (find(ship.name) != nullptr) {
        return false;
    }
    ships_.push_back(std::move(ship));
    return true;
}

bool FleetRoster::remove(std::string_view name) {
    const auto it = std::ranges::find(ships_, name, &Spacecraft::name);
    if (it == ships_.end()) {
        return false;
    }
    ships_.erase(it);
    return true;
}

std::size_t FleetRoster::removeUndercrewed(int min_crew) {
    return std::erase_if(ships_, [min_crew](const Spacecraft& ship) { return ship.crew_size < min_crew; });
}

const Spacecraft* FleetRoster::find(std::string_view name) const noexcept {
    const auto it = std::ranges::find(ships_, name, &Spacecraft::name);
    return it == ships_.end() ? nullptr : &*it;
}

std::vector<Spacecraft> FleetRoster::sortedByCombatRating() const {
    std::vector<Spacecraft> sorted(ships_);
    std::ranges::sort(sorted, [](const Spacecraft& lhs, const Spacecraft& rhs) {
        const double l = lhs.combatRating();
        const double r = rhs.combatRating();
        if (l != r) {
            return l > r;
        }
        return lhs.name < rhs.name;
    });
    return sorted;
}

std::vector<std::string> FleetRoster::namesOfClass(std::string_view class_type) const {
    std::vector<std::string> names;
    for (const auto& ship : ships_) {
        if (ship.class_type == class_type) {
            names.push_back(ship.name);
        }
    }
    return names;
}

long long FleetRoster::totalCrew() const noexcept {
    return std::transform_reduce(ships_.begin(), ships_.end(), 0LL, std::plus<>{},
                                 [](const Spacecraft& ship) { return static_cast<long long>(ship.crew_size); });
}

// ---------------------------------------------------------------------------------------------
// Free functions
// ---------------------------------------------------------------------------------------------

std::vector<int> slidingWindowMaximum(std::span<const int> values, std::size_t window) {
    if (window == 0) {
        throw std::invalid_argument("window must be positive");
    }
    std::vector<int> maxima;
    if (window > values.size()) {
        return maxima;
    }
    maxima.reserve(values.size() - window + 1);
    std::deque<std::size_t> candidates;  // indices whose values are strictly decreasing
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (!candidates.empty() && candidates.front() + window <= i) {
            candidates.pop_front();
        }
        while (!candidates.empty() && values[candidates.back()] <= values[i]) {
            candidates.pop_back();
        }
        candidates.push_back(i);
        if (i + 1 >= window) {
            maxima.push_back(values[candidates.front()]);
        }
    }
    return maxima;
}

std::map<std::string, std::vector<std::string>> groupPlanetsBySystem(std::span<const Planet> planets) {
    std::map<std::string, std::vector<std::string>> groups;
    for (const auto& planet : planets) {
        groups[planet.system].push_back(planet.name);
    }
    for (auto& [system, names] : groups) {
        std::ranges::sort(names);
    }
    return groups;
}

std::multimap<int, std::string> indexPlanetsByDefense(std::span<const Planet> planets) {
    std::multimap<int, std::string> index;
    for (const auto& planet : planets) {
        index.emplace(planet.defense_level, planet.name);
    }
    return index;
}

std::vector<std::string> planetsWithDefenseBetween(const std::multimap<int, std::string>& index, int low,
                                                   int high) {
    std::vector<std::string> names;
    if (low > high) {
        return names;
    }
    const auto first = index.lower_bound(low);
    const auto last = index.upper_bound(high);
    std::transform(first, last, std::back_inserter(names), [](const auto& entry) { return entry.second; });
    return names;
}

std::unordered_map<std::string, std::size_t> wordFrequencies(std::string_view text) {
    std::unordered_map<std::string, std::size_t> counts;
    std::string word;
    const auto flush = [&] {
        if (!word.empty()) {
            ++counts[word];
            word.clear();
        }
    };
    for (const char raw : text) {
        const auto ch = static_cast<unsigned char>(raw);
        if (std::isalpha(ch) != 0) {
            word.push_back(static_cast<char>(std::tolower(ch)));
        } else {
            flush();
        }
    }
    flush();
    return counts;
}

std::vector<std::pair<std::string, std::size_t>> topWords(
    const std::unordered_map<std::string, std::size_t>& frequencies, std::size_t k) {
    std::vector<std::pair<std::string, std::size_t>> entries(frequencies.begin(), frequencies.end());
    const auto by_count = [](const auto& lhs, const auto& rhs) {
        if (lhs.second != rhs.second) {
            return lhs.second > rhs.second;
        }
        return lhs.first < rhs.first;
    };
    const std::size_t n = std::min(k, entries.size());
    std::partial_sort(entries.begin(), entries.begin() + static_cast<std::ptrdiff_t>(n), entries.end(), by_count);
    entries.resize(n);
    return entries;
}

bool isBalanced(std::string_view text) {
    std::stack<char> open;
    for (const char ch : text) {
        switch (ch) {
            case '(':
            case '[':
            case '{':
                open.push(ch);
                break;
            case ')':
            case ']':
            case '}': {
                const char expected = ch == ')' ? '(' : (ch == ']' ? '[' : '{');
                if (open.empty() || open.top() != expected) {
                    return false;
                }
                open.pop();
                break;
            }
            default:
                break;
        }
    }
    return open.empty();
}

std::vector<int> breadthFirstOrder(const std::map<int, std::vector<int>>& graph, int start) {
    std::vector<int> order;
    std::unordered_set<int> visited{start};
    std::queue<int> frontier;
    frontier.push(start);
    while (!frontier.empty()) {
        const int vertex = frontier.front();
        frontier.pop();
        order.push_back(vertex);
        if (const auto it = graph.find(vertex); it != graph.end()) {
            for (const int neighbour : it->second) {
                if (visited.insert(neighbour).second) {
                    frontier.push(neighbour);
                }
            }
        }
    }
    return order;
}

// ---------------------------------------------------------------------------------------------
// TaskScheduler
// ---------------------------------------------------------------------------------------------

void TaskScheduler::submit(std::string name, int priority) {
    queue_.push(Task{std::move(name), priority, next_sequence_++});
}

std::optional<TaskScheduler::Task> TaskScheduler::next() {
    if (queue_.empty()) {
        return std::nullopt;
    }
    Task top = queue_.top();
    queue_.pop();
    return top;
}

const TaskScheduler::Task* TaskScheduler::peek() const noexcept {
    return queue_.empty() ? nullptr : &queue_.top();
}

// ---------------------------------------------------------------------------------------------
// Demonstrations
// ---------------------------------------------------------------------------------------------

namespace {

template <typename Range>
void printSequence(std::ostream& out, std::string_view label, const Range& range) {
    out << label << ":";
    for (const auto& value : range) {
        out << ' ' << value;
    }
    out << '\n';
}

}  // namespace

void demonstrateSequenceContainers(std::ostream& out) {
    out << "\n=== Sequence Containers ===\n";

    FleetRoster roster(sampleFleet());
    out << "vector-backed roster holds " << roster.size() << " ships, total crew " << roster.totalCrew() << '\n';
    const bool duplicate_added = roster.add({"Falcon", "Copy", 1.0, 1, 1.0, 1.0});
    out << "adding a duplicate name succeeded? " << std::boolalpha << duplicate_added << '\n';
    out << "ranked by combat rating:";
    for (const auto& ship : roster.sortedByCombatRating()) {
        out << ' ' << ship.name;
    }
    out << '\n';
    out << "removed " << roster.removeUndercrewed(10) << " undercrewed ships via std::erase_if\n";

    const std::vector<int> readings{3, 1, 4, 1, 5, 9, 2, 6, 5, 3};
    printSequence(out, "deque sliding-window max (k=3)", slidingWindowMaximum(readings, 3));

    std::list<std::string> route{"Sol", "Vega", "Sirius"};
    std::list<std::string> detour{"Altair", "Deneb"};
    route.splice(std::next(route.begin()), detour);  // O(1), no copies, iterators stay valid
    printSequence(out, "list after splice", route);

    std::forward_list<int> sensor_ids{7, 3, 3, 9, 1, 1, 1};
    sensor_ids.unique();
    sensor_ids.push_front(0);
    printSequence(out, "forward_list after unique/push_front", sensor_ids);

    constexpr auto squares = makeSquares<6>();
    static_assert(squares[5] == 25);
    printSequence(out, "constexpr std::array of squares", squares);
}

void demonstrateAssociativeContainers(std::ostream& out) {
    out << "\n=== Associative Containers ===\n";
    const auto planets = samplePlanets();

    for (const auto& [system, names] : groupPlanetsBySystem(planets)) {
        printSequence(out, "map[" + system + "]", names);
    }

    const auto defense_index = indexPlanetsByDefense(planets);
    out << "multimap holds " << defense_index.count(7) << " planets with defense 7\n";
    printSequence(out, "defense in [4,7]", planetsWithDefenseBetween(defense_index, 4, 7));

    std::set<std::string, std::less<>> resources;  // transparent comparator: lookup by string_view
    for (const auto& planet : planets) {
        resources.insert(planet.resources.begin(), planet.resources.end());
    }
    printSequence(out, "set of unique resources", resources);
    out << "contains \"Gold\" (heterogeneous lookup)? " << std::boolalpha
        << resources.contains(std::string_view{"Gold"}) << '\n';

    const auto freq = wordFrequencies("the fleet and the station and the moon");
    out << "unordered_map word counts, top 2:";
    for (const auto& [word, count] : topWords(freq, 2)) {
        out << ' ' << word << '=' << count;
    }
    out << '\n';

    std::unordered_set<Spacecraft, SpacecraftNameHash, SpacecraftNameEqual> registry;
    for (const auto& ship : sampleFleet()) {
        registry.insert(ship);
    }
    const bool inserted = registry.insert({"Enterprise", "Clone", 0.0, 0, 0.0, 0.0}).second;
    out << "unordered_set with custom hash rejects duplicate Enterprise: " << std::boolalpha << !inserted << '\n';

    LruCache<std::string, int> cache(2);
    cache.put("alpha", 1);
    cache.put("beta", 2);
    (void)cache.get("alpha");
    const auto evicted = cache.put("gamma", 3);
    out << "LRU cache (list + unordered_map) evicted: " << evicted.value_or("none") << '\n';
}

void demonstrateContainerAdapters(std::ostream& out) {
    out << "\n=== Container Adapters ===\n";
    out << std::boolalpha << "stack: \"{[()]}\" balanced? " << isBalanced("{[()]}") << ", \"([)]\" balanced? "
        << isBalanced("([)]") << '\n';

    const std::map<int, std::vector<int>> jump_gates{{0, {1, 2}}, {1, {3}}, {2, {3, 4}}, {3, {5}}, {4, {5}}};
    printSequence(out, "queue-driven BFS from gate 0", breadthFirstOrder(jump_gates, 0));

    TaskScheduler scheduler;
    scheduler.submit("refuel", 2);
    scheduler.submit("repel boarders", 9);
    scheduler.submit("resupply", 2);
    scheduler.submit("scan sector", 5);
    out << "priority_queue order:";
    while (auto task = scheduler.next()) {
        out << ' ' << task->name << '(' << task->priority << ')';
    }
    out << '\n';
}

void runContainersDemo(std::ostream& out) {
    demonstrateSequenceContainers(out);
    demonstrateAssociativeContainers(out);
    demonstrateContainerAdapters(out);
}

}  // namespace CppVerseHub::STL
