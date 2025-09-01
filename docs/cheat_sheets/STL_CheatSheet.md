# Standard Template Library (STL) Cheat Sheet

**Location:** `CppVerseHub/docs/cheat_sheets/STL_CheatSheet.md`

## 📚 Complete STL Reference for Modern C++

### 🗂️ Containers Overview

#### **Sequence Containers**

##### `std::vector<T>` - Dynamic Array

```cpp
#include <vector>

std::vector<Planet> planets;

// Initialization
std::vector<int> numbers{1, 2, 3, 4, 5};
std::vector<std::string> names(10, "Unknown");  // 10 elements, all "Unknown"

// Adding elements
planets.push_back(Planet("Earth"));
planets.emplace_back("Mars", 500.0);  // Construct in-place

// Accessing elements
Planet& first = planets[0];           // No bounds checking
Planet& first_safe = planets.at(0);   // Bounds checking (throws)
Planet& front = planets.front();
Planet& back = planets.back();

// Iteration
for (const auto& planet : planets) {
    std::cout << planet.getName() << "\n";
}

for (auto it = planets.begin(); it != planets.end(); ++it) {
    it->update();
}

// Capacity and size
planets.reserve(100);                 // Pre-allocate space
planets.resize(50);                   // Resize container
std::cout << planets.size() << "\n";       // Current size
std::cout << planets.capacity() << "\n";   // Allocated capacity

// Removal
planets.pop_back();                   // Remove last element
planets.erase(planets.begin() + 2);   // Remove element at index 2
planets.clear();                      // Remove all elements
```

##### `std::deque<T>` - Double-Ended Queue

```cpp
#include <deque>

std::deque<Mission> mission_queue;

// Efficient insertion/removal at both ends
mission_queue.push_front(ExplorationMission());
mission_queue.push_back(CombatMission());
mission_queue.pop_front();
mission_queue.pop_back();

// Random access (like vector)
Mission& current = mission_queue[0];
```

##### `std::list<T>` - Doubly Linked List

```cpp
#include <list>

std::list<Fleet> fleets;

// Efficient insertion/removal anywhere
auto it = fleets.begin();
std::advance(it, 2);
fleets.insert(it, Fleet("Patrol Fleet"));

// Splice operations
std::list<Fleet> backup_fleets;
backup_fleets.splice(backup_fleets.end(), fleets, fleets.begin());
```

##### `std::array<T, N>` - Fixed-Size Array

```cpp
#include <array>

std::array<double, 3> coordinates{10.0, 20.0, 30.0};

// Array-like access with STL benefits
coordinates.fill(0.0);
std::sort(coordinates.begin(), coordinates.end());
```

#### **Associative Containers**

##### `std::map<Key, Value>` - Ordered Key-Value Pairs

```cpp
#include <map>

std::map<std::string, Planet> planet_registry;

// Insertion
planet_registry["Earth"] = Planet("Earth", 1000.0);
planet_registry.emplace("Mars", "Mars", 500.0);
planet_registry.insert({"Venus", Planet("Venus", 200.0)});

// Access
Planet& earth = planet_registry["Earth"];        // Creates if not exists
auto it = planet_registry.find("Mars");          // Returns iterator
if (it != planet_registry.end()) {
    Planet& mars = it->second;
}

// Iteration (sorted by key)
for (const auto& [name, planet] : planet_registry) {
    std::cout << name << ": " << planet.getResources() << "\n";
}
```

##### `std::unordered_map<Key, Value>` - Hash Table

```cpp
#include <unordered_map>

std::unordered_map<std::string, Fleet> fleet_registry;

// Same interface as map, but O(1) average access
fleet_registry["Alpha"] = Fleet("Alpha Squadron");

// Custom hash for user-defined types
struct FleetHash {
    std::size_t operator()(const Fleet& fleet) const {
        return std::hash<std::string>{}(fleet.getName());
    }
};

std::unordered_map<Fleet, int, FleetHash> fleet_sizes;
```

##### `std::set<T>` - Ordered Unique Elements

```cpp
#include <set>

std::set<std::string> visited_systems;

// Insertion (automatically sorted, no duplicates)
visited_systems.insert("Sol");
visited_systems.insert("Alpha Centauri");
visited_systems.insert("Sol");  // Duplicate ignored

// Search
if (visited_systems.count("Sol") > 0) {
    std::cout << "Sol system already visited\n";
}

auto it = visited_systems.find("Alpha Centauri");
if (it != visited_systems.end()) {
    std::cout << "Found: " << *it << "\n";
}
```

##### `std::unordered_set<T>` - Hash Set

```cpp
#include <unordered_set>

std::unordered_set<int> active_mission_ids;

// O(1) average insertion and lookup
active_mission_ids.insert(12345);
active_mission_ids.insert(67890);

bool is_active = active_mission_ids.count(12345) > 0;
```

#### **Container Adaptors**

##### `std::stack<T>` - LIFO Stack

```cpp
#include <stack>

std::stack<Command> command_history;

command_history.push(MoveCommand(10, 20));
command_history.push(AttackCommand("Enemy"));

if (!command_history.empty()) {
    Command last_command = command_history.top();
    command_history.pop();
}
```

##### `std::queue<T>` - FIFO Queue

```cpp
#include <queue>

std::queue<Mission> mission_queue;

mission_queue.push(ExplorationMission());
mission_queue.push(CombatMission());

while (!mission_queue.empty()) {
    Mission current = mission_queue.front();
    mission_queue.pop();
    current.execute();
}
```

##### `std::priority_queue<T>` - Priority Heap

```cpp
#include <queue>

// Max heap by default
std::priority_queue<int> high_priority;

// Custom comparator for min heap
auto cmp = [](const Mission& a, const Mission& b) {
    return a.getPriority() > b.getPriority();
};
std::priority_queue<Mission, std::vector<Mission>, decltype(cmp)>
    mission_priorities(cmp);

mission_priorities.push(ExplorationMission(5));
mission_priorities.push(CombatMission(10));

Mission highest = mission_priorities.top();  // CombatMission (priority 10)
```

### 🔄 Iterators

#### **Iterator Types**

```cpp
// Input Iterator (read-only, single pass)
template<typename InputIt>
void process_read_only(InputIt first, InputIt last) {
    for (auto it = first; it != last; ++it) {
        std::cout << *it << " ";  // Read only
    }
}

// Output Iterator (write-only)
template<typename OutputIt, typename T>
void fill_range(OutputIt first, OutputIt last, const T& value) {
    for (auto it = first; it != last; ++it) {
        *it = value;  // Write only
    }
}

// Forward Iterator (read/write, multi-pass)
template<typename ForwardIt>
ForwardIt find_duplicate(ForwardIt first, ForwardIt last) {
    for (auto it1 = first; it1 != last; ++it1) {
        for (auto it2 = std::next(it1); it2 != last; ++it2) {
            if (*it1 == *it2) return it1;
        }
    }
    return last;
}

// Bidirectional Iterator (can go backwards)
template<typename BidirIt>
void reverse_traverse(BidirIt first, BidirIt last) {
    while (last != first) {
        --last;
        std::cout << *last << " ";
    }
}

// Random Access Iterator (jump to any position)
template<typename RandomIt>
void binary_search_example(RandomIt first, RandomIt last,
                          const typename RandomIt::value_type& value) {
    auto mid = first + std::distance(first, last) / 2;
    if (*mid == value) {
        std::cout << "Found at position " << std::distance(first, mid) << "\n";
    }
}
```

#### **Iterator Adaptors**

```cpp
#include <iterator>

std::vector<int> numbers{1, 2, 3, 4, 5};

// Reverse Iterator
for (auto rit = numbers.rbegin(); rit != numbers.rend(); ++rit) {
    std::cout << *rit << " ";  // Prints: 5 4 3 2 1
}

// Insert Iterators
std::vector<int> destination;
std::copy(numbers.begin(), numbers.end(),
          std::back_inserter(destination));  // Insert at end

std::list<int> target;
std::copy(numbers.begin(), numbers.end(),
          std::front_inserter(target));     // Insert at front

// Stream Iterators
std::vector<int> data{10, 20, 30};
std::copy(data.begin(), data.end(),
          std::ostream_iterator<int>(std::cout, " "));  // Output: 10 20 30

// istream_iterator for input
std::vector<int> input_data{std::istream_iterator<int>(std::cin),
                           std::istream_iterator<int>()};
```

### ⚙️ Algorithms

#### **Non-Modifying Algorithms**

```cpp
#include <algorithm>

std::vector<Planet> planets = getPlanets();

// Finding elements
auto it = std::find(planets.begin(), planets.end(), target_planet);
auto it2 = std::find_if(planets.begin(), planets.end(),
    [](const Planet& p) { return p.getResources() > 1000; });

// Counting
int habitable_count = std::count_if(planets.begin(), planets.end(),
    [](const Planet& p) { return p.isHabitable(); });

// Checking conditions
bool all_explored = std::all_of(planets.begin(), planets.end(),
    [](const Planet& p) { return p.isExplored(); });

bool any_hostile = std::any_of(planets.begin(), planets.end(),
    [](const Planet& p) { return p.isHostile(); });

bool none_destroyed = std::none_of(planets.begin(), planets.end(),
    [](const Planet& p) { return p.isDestroyed(); });

// For each
std::for_each(planets.begin(), planets.end(),
    [](Planet& p) { p.update(); });

// Min/Max elements
auto richest = std::max_element(planets.begin(), planets.end(),
    [](const Planet& a, const Planet& b) {
        return a.getResources() < b.getResources();
    });

auto [min_planet, max_planet] = std::minmax_element(
    planets.begin(), planets.end(),
    [](const Planet& a, const Planet& b) {
        return a.getPopulation() < b.getPopulation();
    });
```

#### **Modifying Algorithms**

```cpp
// Copy and transform
std::vector<Planet> planets = getPlanets();
std::vector<std::string> planet_names;

std::transform(planets.begin(), planets.end(),
               std::back_inserter(planet_names),
               [](const Planet& p) { return p.getName(); });

// Remove operations (erase-remove idiom)
planets.erase(
    std::remove_if(planets.begin(), planets.end(),
                   [](const Planet& p) { return p.isDestroyed(); }),
    planets.end());

// Replace
std::replace_if(planets.begin(), planets.end(),
                [](const Planet& p) { return p.getResources() == 0; },
                Planet("Barren World"));

// Fill and generate
std::vector<int> mission_ids(10);
std::iota(mission_ids.begin(), mission_ids.end(), 1000);  // 1000, 1001, 1002...

std::generate(mission_ids.begin(), mission_ids.end(),
              []() { return rand() % 10000; });

// Reverse and rotate
std::reverse(planets.begin(), planets.end());
std::rotate(planets.begin(), planets.begin() + 2, planets.end());

// Random operations
std::random_device rd;
std::mt19937 g(rd());
std::shuffle(planets.begin(), planets.end(), g);

// Partition
auto partition_point = std::partition(planets.begin(), planets.end(),
    [](const Planet& p) { return p.isHabitable(); });
// Now habitable planets are before partition_point
```

#### **Sorting and Related Operations**

```cpp
std::vector<Fleet> fleets = getFleets();

// Sorting
std::sort(fleets.begin(), fleets.end(),
    [](const Fleet& a, const Fleet& b) {
        return a.getFirepower() > b.getFirepower();
    });

// Partial sort (only sort first n elements)
std::partial_sort(fleets.begin(), fleets.begin() + 5, fleets.end(),
    [](const Fleet& a, const Fleet& b) {
        return a.getSize() > b.getSize();
    });

// Stable sort (maintains relative order of equal elements)
std::stable_sort(fleets.begin(), fleets.end(),
    [](const Fleet& a, const Fleet& b) {
        return a.getFaction() < b.getFaction();
    });

// nth_element (partial sorting)
std::nth_element(fleets.begin(), fleets.begin() + fleets.size()/2, fleets.end());
Fleet& median_fleet = fleets[fleets.size()/2];

// Binary search (requires sorted range)
std::sort(fleets.begin(), fleets.end());
bool found = std::binary_search(fleets.begin(), fleets.end(), target_fleet);

auto lower = std::lower_bound(fleets.begin(), fleets.end(), target_fleet);
auto upper = std::upper_bound(fleets.begin(), fleets.end(), target_fleet);
auto range = std::equal_range(fleets.begin(), fleets.end(), target_fleet);

// Merge sorted ranges
std::vector<Fleet> combined_fleets;
std::merge(fleet_alpha.begin(), fleet_alpha.end(),
          fleet_beta.begin(), fleet_beta.end(),
          std::back_inserter(combined_fleets));
```

#### **Set Operations (on sorted ranges)**

```cpp
std::vector<int> system_ids_a{1, 3, 5, 7, 9};
std::vector<int> system_ids_b{2, 3, 6, 7, 10};
std::vector<int> result;

// Set union
std::set_union(system_ids_a.begin(), system_ids_a.end(),
               system_ids_b.begin(), system_ids_b.end(),
               std::back_inserter(result));
// Result: {1, 2, 3, 5, 6, 7, 9, 10}

// Set intersection
result.clear();
std::set_intersection(system_ids_a.begin(), system_ids_a.end(),
                     system_ids_b.begin(), system_ids_b.end(),
                     std::back_inserter(result));
// Result: {3, 7}

// Set difference
result.clear();
std::set_difference(system_ids_a.begin(), system_ids_a.end(),
                   system_ids_b.begin(), system_ids_b.end(),
                   std::back_inserter(result));
// Result: {1, 5, 9}

// Symmetric difference
result.clear();
std::set_symmetric_difference(system_ids_a.begin(), system_ids_a.end(),
                             system_ids_b.begin(), system_ids_b.end(),
                             std::back_inserter(result));
// Result: {1, 2, 5, 6, 9, 10}
```

#### **Heap Operations**

```cpp
std::vector<int> mission_priorities{3, 1, 4, 1, 5, 9, 2, 6};

// Create heap
std::make_heap(mission_priorities.begin(), mission_priorities.end());
// Max element is now at front

// Add element to heap
mission_priorities.push_back(8);
std::push_heap(mission_priorities.begin(), mission_priorities.end());

// Remove max element
std::pop_heap(mission_priorities.begin(), mission_priorities.end());
int max_priority = mission_priorities.back();
mission_priorities.pop_back();

// Sort heap
std::sort_heap(mission_priorities.begin(), mission_priorities.end());
// Now sorted in ascending order
```

### 🧮 Numeric Algorithms

```cpp
#include <numeric>

std::vector<double> resource_amounts{100.0, 250.0, 175.0, 300.0, 125.0};

// Accumulate (sum, product, etc.)
double total_resources = std::accumulate(
    resource_amounts.begin(), resource_amounts.end(), 0.0);

double product = std::accumulate(
    resource_amounts.begin(), resource_amounts.end(), 1.0,
    std::multiplies<double>());

// Reduce (parallel version of accumulate in C++17)
double total_parallel = std::reduce(
    resource_amounts.begin(), resource_amounts.end(), 0.0);

// Inner product
std::vector<double> prices{10.0, 20.0, 15.0, 25.0, 12.0};
double total_value = std::inner_product(
    resource_amounts.begin(), resource_amounts.end(),
    prices.begin(), 0.0);

// Transform reduce
double weighted_sum = std::transform_reduce(
    resource_amounts.begin(), resource_amounts.end(),
    prices.begin(), 0.0,
    std::plus<>(),           // Reduction operation
    std::multiplies<>()      // Transform operation
);

// Partial sum and differences
std::vector<double> cumulative_resources;
std::partial_sum(resource_amounts.begin(), resource_amounts.end(),
                std::back_inserter(cumulative_resources));

std::vector<double> daily_changes;
std::adjacent_difference(resource_amounts.begin(), resource_amounts.end(),
                        std::back_inserter(daily_changes));

// iota (fill with sequence)
std::vector<int> mission_ids(10);
std::iota(mission_ids.begin(), mission_ids.end(), 1000);
// Creates: 1000, 1001, 1002, ..., 1009
```

### 🔧 Functional Objects (Functors)

#### **Standard Function Objects**

```cpp
#include <functional>

// Arithmetic operations
std::plus<int> add;
std::minus<int> subtract;
std::multiplies<int> multiply;
std::divides<int> divide;

int result = add(10, 5);  // 15

// Comparison operations
std::greater<int> gt;
std::less<int> lt;
std::equal_to<int> eq;

// Use with algorithms
std::vector<int> values{3, 1, 4, 1, 5, 9};
std::sort(values.begin(), values.end(), std::greater<int>());  // Descending

// Logical operations
std::logical_and<bool> and_op;
std::logical_or<bool> or_op;
std::logical_not<bool> not_op;
```

#### **Custom Function Objects**

```cpp
// Functor class
struct PlanetComparator {
    bool operator()(const Planet& a, const Planet& b) const {
        return a.getResources() < b.getResources();
    }
};

// Lambda expressions (C++11+)
auto planet_sorter = [](const Planet& a, const Planet& b) {
    return a.getPopulation() > b.getPopulation();
};

// Generic lambda (C++14+)
auto generic_printer = [](const auto& item) {
    std::cout << item << "\n";
};

// Capture examples
int threshold = 1000;
auto filter_rich_planets = [threshold](const Planet& p) {
    return p.getResources() > threshold;
};

// Capture by reference
auto increment_counter = [&threshold]() {
    ++threshold;
};

// Capture by copy
auto use_threshold = [=](int value) {
    return value > threshold;
};

// std::function wrapper
std::function<bool(const Planet&)> planet_filter =
    [](const Planet& p) { return p.isHabitable(); };

// Bind operations
using namespace std::placeholders;
auto bound_function = std::bind(planet_comparator, _1, Planet("Reference"));
```

### 📊 Utility Functions

#### **std::pair and std::tuple**

```cpp
#include <utility>
#include <tuple>

// Pair operations
std::pair<std::string, double> planet_info{"Mars", 500.0};
auto [name, resources] = planet_info;  // Structured binding (C++17)

// make_pair
auto coordinates = std::make_pair(10.5, 20.3);

// Tuple operations
std::tuple<std::string, int, double, bool> mission_data{
    "Exploration", 12345, 1500.0, true
};

auto mission_name = std::get<0>(mission_data);
auto mission_id = std::get<int>(mission_data);

// Structured bindings with tuple
auto [name, id, cost, active] = mission_data;

// tie for unpacking
std::string extracted_name;
int extracted_id;
std::tie(extracted_name, extracted_id, std::ignore, std::ignore) = mission_data;

// make_tuple
auto fleet_status = std::make_tuple("Active", 15, 98.5);

// tuple_size and tuple_element
constexpr std::size_t tuple_size = std::tuple_size_v<decltype(mission_data)>;
using first_type = std::tuple_element_t<0, decltype(mission_data)>;
```

#### **std::optional (C++17)**

```cpp
#include <optional>

std::optional<Planet> findPlanet(const std::string& name) {
    // Search logic...
    if (planet_found) {
        return Planet(name, resources);
    }
    return std::nullopt;  // or {}
}

// Usage
auto planet = findPlanet("Unknown");
if (planet) {  // or planet.has_value()
    std::cout << "Found: " << planet->getName() << "\n";
    std::cout << "Resources: " << planet.value().getResources() << "\n";
} else {
    std::cout << "Planet not found\n";
}

// value_or for default values
double resources = planet.value_or(Planet("Default", 0)).getResources();
```

#### **std::variant (C++17)**

```cpp
#include <variant>

// Union-like type that can hold one of several types
std::variant<int, double, std::string> mission_parameter;

mission_parameter = 42;              // Holds int
mission_parameter = 3.14;            // Now holds double
mission_parameter = "Exploration";   // Now holds string

// Access with std::get
try {
    int int_param = std::get<int>(mission_parameter);
} catch (const std::bad_variant_access& e) {
    // Handle wrong type access
}

// Safe access with std::get_if
if (auto* str_ptr = std::get_if<std::string>(&mission_parameter)) {
    std::cout << "String parameter: " << *str_ptr << "\n";
}

// Visitor pattern
struct ParameterVisitor {
    void operator()(int value) { std::cout << "Integer: " << value << "\n"; }
    void operator()(double value) { std::cout << "Double: " << value << "\n"; }
    void operator()(const std::string& value) { std::cout << "String: " << value << "\n"; }
};

std::visit(ParameterVisitor{}, mission_parameter);

// Generic lambda visitor
std::visit([](const auto& value) {
    std::cout << "Value: " << value << "\n";
}, mission_parameter);
```

#### **std::any (C++17)**

```cpp
#include <any>

std::any mission_data;

mission_data = 42;
mission_data = std::string("Exploration");
mission_data = Planet("Mars", 500.0);

// Type checking
if (mission_data.type() == typeid(std::string)) {
    std::string str = std::any_cast<std::string>(mission_data);
}

// Safe casting
try {
    Planet planet = std::any_cast<Planet>(mission_data);
} catch (const std::bad_any_cast& e) {
    // Handle casting error
}
```

### 🔄 String Operations

#### **String Algorithms**

```cpp
#include <string>
#include <algorithm>
#include <cctype>

std::string planet_name = "  Mars Station Alpha  ";

// Transform case
std::transform(planet_name.begin(), planet_name.end(), planet_name.begin(),
               [](char c) { return std::tolower(c); });

// Remove whitespace
planet_name.erase(planet_name.begin(),
                  std::find_if(planet_name.begin(), planet_name.end(),
                              [](char c) { return !std::isspace(c); }));

planet_name.erase(std::find_if(planet_name.rbegin(), planet_name.rend(),
                              [](char c) { return !std::isspace(c); }).base(),
                  planet_name.end());

// String search
std::string text = "The fleet approached the hostile planet";
auto pos = text.find("fleet");
if (pos != std::string::npos) {
    std::cout << "Found 'fleet' at position " << pos << "\n";
}

// String replace
std::string::size_type start_pos = 0;
while ((start_pos = text.find("planet", start_pos)) != std::string::npos) {
    text.replace(start_pos, 6, "world");
    start_pos += 5;
}
```

### 📋 Performance Tips & Best Practices

#### **Container Selection Guide**

```cpp
// Use vector for:
// - Random access needed
// - Frequent push_back operations
// - Cache-friendly sequential access
std::vector<Planet> planets;

// Use deque for:
// - Frequent insertion/removal at both ends
// - Random access needed
std::deque<Mission> mission_queue;

// Use list for:
// - Frequent insertion/removal in middle
// - No random access needed
std::list<Fleet> active_fleets;

// Use map for:
// - Sorted key-value pairs
// - Range queries needed
std::map<std::string, Planet> sorted_planets;

// Use unordered_map for:
// - Fast key-value lookups
// - Order doesn't matter
std::unordered_map<int, Fleet> fleet_lookup;
```

#### **Algorithm Optimization**

```cpp
// Reserve space to avoid reallocations
std::vector<Planet> planets;
planets.reserve(1000);  // If you know approximate size

// Use move semantics
std::vector<std::unique_ptr<Fleet>> fleets;
fleets.push_back(std::make_unique<Fleet>("Alpha"));

// Prefer algorithms over loops
// Instead of:
for (auto it = vec.begin(); it != vec.end(); ++it) {
    if (condition(*it)) {
        // do something
    }
}

// Use:
std::for_each(vec.begin(), vec.end(), [](const auto& item) {
    if (condition(item)) {
        // do something
    }
});

// Or even better, use ranges (C++20):
// std::ranges::for_each(vec | std::views::filter(condition), action);
```

#### **Memory and Performance**

```cpp
// Use emplace instead of push for complex objects
planets.emplace_back("Mars", 500.0);  // Construct in place
// Instead of:
planets.push_back(Planet("Mars", 500.0));  // Construct then copy/move

// Use const references in range-based loops
for (const auto& planet : planets) {  // No copy
    std::cout << planet.getName();
}

// Reserve iterators when possible
auto it = std::find(planets.begin(), planets.end(), target);

// Use appropriate container member functions
planets.clear();           // Instead of erase(begin(), end())
if (!planets.empty()) {}   // Instead of size() > 0
```

### 🎯 Common Patterns & Idioms

#### **Erase-Remove Idiom**

```cpp
// Remove all destroyed planets
planets.erase(
    std::remove_if(planets.begin(), planets.end(),
                   [](const Planet& p) { return p.isDestroyed(); }),
    planets.end());

// For std::string
std::string text = "Hello, World!";
text.erase(std::remove(text.begin(), text.end(), ','), text.end());
```

#### **Copy-and-Swap Idiom**

```cpp
class Fleet {
private:
    std::vector<Ship> ships_;

public:
    Fleet& operator=(Fleet other) {  // Pass by value
        swap(*this, other);          // Swap with temporary
        return *this;                // Temporary destroyed automatically
    }

    friend void swap(Fleet& first, Fleet& second) noexcept {
        using std::swap;
        swap(first.ships_, second.ships_);
    }
};
```

#### **RAII with STL Containers**

```cpp
class ResourceManager {
private:
    std::vector<std::unique_ptr<Resource>> resources_;

public:
    ~ResourceManager() = default;  // Automatic cleanup via smart pointers

    void addResource(std::unique_ptr<Resource> resource) {
        resources_.push_back(std::move(resource));
    }

    // Resources automatically cleaned up when vector is destroyed
};
```

### 📊 STL Quick Reference Table

| Need                   | Container        | Time Complexity                | Notes            |
| ---------------------- | ---------------- | ------------------------------ | ---------------- |
| Dynamic array          | `vector`         | Access: O(1), Insert end: O(1) | Default choice   |
| Queue                  | `deque`          | Insert/remove ends: O(1)       | Double-ended     |
| Linked list            | `list`           | Insert/remove anywhere: O(1)   | No random access |
| Key-value (sorted)     | `map`            | All operations: O(log n)       | Ordered keys     |
| Key-value (fast)       | `unordered_map`  | Average: O(1), Worst: O(n)     | Hash table       |
| Unique values (sorted) | `set`            | All operations: O(log n)       | No duplicates    |
| Unique values (fast)   | `unordered_set`  | Average: O(1), Worst: O(n)     | Hash set         |
| Stack                  | `stack`          | Push/pop: O(1)                 | LIFO             |
| Queue                  | `queue`          | Push/pop: O(1)                 | FIFO             |
| Priority queue         | `priority_queue` | Push/pop: O(log n)             | Heap             |

### 🚀 Modern STL Features (C++17/20/23)

#### **Parallel Algorithms (C++17)**

```cpp
#include <execution>

std::vector<int> large_data(1000000);

// Parallel sort
std::sort(std::execution::par, large_data.begin(), large_data.end());

// Parallel for_each
std::for_each(std::execution::par_unseq, large_data.begin(), large_data.end(),
              [](int& value) { value = value * value; });
```

#### **Ranges Library (C++20)**

```cpp
#include <ranges>

std::vector<Planet> planets = getPlanets();

// Range-based filtering and transformation
auto habitable_names = planets
    | std::views::filter([](const Planet& p) { return p.isHabitable(); })
    | std::views::transform([](const Planet& p) { return p.getName(); });

// Lazy evaluation - only computed when needed
for (const auto& name : habitable_names) {
    std::cout << name << "\n";
}
```

### 🎯 Key Takeaways

1. **Choose the right container** for your use case
2. **Use algorithms** instead of hand-written loops
3. **Prefer const references** in range-based loops
4. **Reserve space** when you know container size
5. **Use move semantics** for expensive objects
6. **Embrace modern STL features** (optional, variant, ranges)
7. **Follow RAII principles** with smart pointers
8. **Use iterators correctly** and understand their categories

---

_This cheat sheet covers the essential STL components used throughout CppVerseHub. For practical examples, see the `src/stl_showcase/` directory._
