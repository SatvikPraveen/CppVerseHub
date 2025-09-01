# Modern C++ Features Cheat Sheet

**Location:** `CppVerseHub/docs/cheat_sheets/ModernCpp_CheatSheet.md`

## 📚 C++11/14/17/20/23 Features Reference

### 🎯 C++11 Revolutionary Features

#### **Auto Type Deduction**

```cpp
// Basic auto usage
auto x = 42;              // int
auto y = 3.14;            // double
auto z = "hello";         // const char*
auto w = std::string{"world"};  // std::string

// Auto with functions
auto add = [](int a, int b) { return a + b; };  // lambda
auto vec = std::vector<int>{1, 2, 3, 4, 5};     // vector<int>

// Auto with iterators (very useful!)
std::map<std::string, Planet> planets;
auto it = planets.find("Earth");  // std::map<std::string, Planet>::iterator

// Auto with templates
template<typename T, typename U>
auto multiply(T a, U b) -> decltype(a * b) {    // Trailing return type
    return a * b;
}

// Auto in range-based loops
std::vector<Planet> planet_list;
for (const auto& planet : planet_list) {        // Much cleaner!
    std::cout << planet.getName() << "\n";
}
```

#### **Range-Based For Loops**

```cpp
std::vector<int> numbers{1, 2, 3, 4, 5};
std::map<std::string, double> resources;

// Basic range-based loops
for (const int& num : numbers) {
    std::cout << num << " ";
}

for (const auto& [name, amount] : resources) {  // C++17 structured bindings
    std::cout << name << ": " << amount << "\n";
}

// Modify elements
for (auto& num : numbers) {
    num *= 2;  // Double each element
}

// Custom range-based loop support
class Planet {
private:
    std::vector<std::string> moons_;

public:
    // Iterator support for range-based loops
    auto begin() { return moons_.begin(); }
    auto end() { return moons_.end(); }
    auto begin() const { return moons_.begin(); }
    auto end() const { return moons_.end(); }
};

Planet earth;
for (const auto& moon : earth) {  // Works with our custom class!
    std::cout << moon << "\n";
}
```

#### **Lambda Expressions**

```cpp
#include <algorithm>
#include <functional>

// Basic lambda syntax
auto simple_lambda = []() {
    std::cout << "Hello from lambda!\n";
};

// Lambda with parameters and return type
auto add_lambda = [](int a, int b) -> int {
    return a + b;
};

// Capture by value
int multiplier = 10;
auto multiply_by_value = [multiplier](int x) {
    return x * multiplier;  // multiplier is copied
};

// Capture by reference
auto multiply_by_ref = [&multiplier](int x) {
    multiplier += 1;        // Modifies original multiplier
    return x * multiplier;
};

// Capture everything by value/reference
auto capture_all_value = [=](int x) { return x * multiplier; };  // Capture all by value
auto capture_all_ref = [&](int x) { return x * multiplier; };    // Capture all by reference

// Mixed captures
int base = 5;
auto mixed_capture = [multiplier, &base](int x) {
    base += x;              // Modify original base
    return x * multiplier;  // Use copied multiplier
};

// Mutable lambdas
auto mutable_lambda = [multiplier](int x) mutable {
    multiplier += 1;        // Can modify captured copy
    return x * multiplier;
};

// Lambda with STL algorithms
std::vector<Planet> planets = getPlanets();

// Sort planets by resource amount
std::sort(planets.begin(), planets.end(),
          [](const Planet& a, const Planet& b) {
              return a.getResources() > b.getResources();
          });

// Find planets with resources > 1000
auto rich_planet = std::find_if(planets.begin(), planets.end(),
                               [](const Planet& p) {
                                   return p.getResources() > 1000.0;
                               });

// Transform planet names to uppercase
std::vector<std::string> planet_names;
std::transform(planets.begin(), planets.end(),
               std::back_inserter(planet_names),
               [](const Planet& p) {
                   std::string name = p.getName();
                   std::transform(name.begin(), name.end(), name.begin(), ::toupper);
                   return name;
               });

// Generic lambda (C++14)
auto generic_printer = [](const auto& item) {
    std::cout << item << "\n";
};

generic_printer(42);           // Works with int
generic_printer("hello");      // Works with string
generic_printer(3.14);         // Works with double
```

#### **Smart Pointers**

```cpp
#include <memory>

// unique_ptr - Exclusive ownership
class Fleet {
public:
    Fleet(const std::string& name) : name_(name) {
        std::cout << "Fleet " << name_ << " created\n";
    }

    ~Fleet() {
        std::cout << "Fleet " << name_ << " destroyed\n";
    }

private:
    std::string name_;
};

// Create unique_ptr
std::unique_ptr<Fleet> fleet1 = std::make_unique<Fleet>("Alpha");
auto fleet2 = std::make_unique<Fleet>("Beta");  // Preferred C++14

// Transfer ownership
std::unique_ptr<Fleet> fleet3 = std::move(fleet1);  // fleet1 is now nullptr

// Custom deleter
auto custom_deleter = [](Fleet* f) {
    std::cout << "Custom deletion\n";
    delete f;
};
std::unique_ptr<Fleet, decltype(custom_deleter)> fleet4(new Fleet("Gamma"), custom_deleter);

// shared_ptr - Shared ownership
std::shared_ptr<Fleet> shared_fleet1 = std::make_shared<Fleet>("Shared Alpha");
std::shared_ptr<Fleet> shared_fleet2 = shared_fleet1;  // Reference count = 2

std::cout << "Reference count: " << shared_fleet1.use_count() << "\n";

// weak_ptr - Non-owning observer
std::weak_ptr<Fleet> weak_fleet = shared_fleet1;

if (auto locked_fleet = weak_fleet.lock()) {  // Check if still valid
    // Use locked_fleet as shared_ptr
    std::cout << "Fleet still exists\n";
} else {
    std::cout << "Fleet has been destroyed\n";
}

// RAII with smart pointers
class ResourceManager {
private:
    std::unique_ptr<ExpensiveResource> resource_;

public:
    ResourceManager() : resource_(std::make_unique<ExpensiveResource>()) {}

    // Rule of Zero - no need for custom destructor, copy, or move operations
    // Smart pointer handles everything automatically
};
```

#### **Move Semantics and Perfect Forwarding**

```cpp
#include <utility>

class Planet {
private:
    std::string name_;
    std::vector<std::string> moons_;

public:
    // Move constructor
    Planet(Planet&& other) noexcept
        : name_(std::move(other.name_)), moons_(std::move(other.moons_)) {
        std::cout << "Planet moved\n";
    }

    // Move assignment operator
    Planet& operator=(Planet&& other) noexcept {
        if (this != &other) {
            name_ = std::move(other.name_);
            moons_ = std::move(other.moons_);
        }
        return *this;
    }

    // Copy constructor and assignment (Rule of Five)
    Planet(const Planet& other)
        : name_(other.name_), moons_(other.moons_) {
        std::cout << "Planet copied\n";
    }

    Planet& operator=(const Planet& other) {
        if (this != &other) {
            name_ = other.name_;
            moons_ = other.moons_;
        }
        return *this;
    }

    ~Planet() = default;

    // Move-aware member functions
    void add_moon(std::string moon_name) {
        moons_.push_back(std::move(moon_name));  // Move the string
    }
};

// Perfect forwarding template
template<typename T, typename... Args>
std::unique_ptr<T> make_unique_custom(Args&&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

// Universal references and perfect forwarding
template<typename T>
void process_value(T&& value) {  // Universal reference
    // Forward value preserving its value category
    actual_processor(std::forward<T>(value));
}

// Move vs Copy demonstration
void move_semantics_demo() {
    Planet earth("Earth");

    std::vector<Planet> planets;

    planets.push_back(earth);              // Copy
    planets.push_back(std::move(earth));   // Move

    planets.emplace_back("Mars");          // Construct in place (best!)
}
```

### 🚀 C++14 Enhancements

#### **Generic Lambdas and Improved Auto**

```cpp
// Generic lambdas
auto generic_lambda = [](auto a, auto b) {
    return a + b;
};

int result1 = generic_lambda(1, 2);        // int + int
double result2 = generic_lambda(1.5, 2.5); // double + double
std::string result3 = generic_lambda(std::string("Hello "), std::string("World"));

// Auto return type deduction
auto calculate_distance(double x1, double y1, double x2, double y2) {
    return std::sqrt((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1));  // Return type deduced as double
}

// Variable templates
template<typename T>
constexpr T pi_v = T(3.1415926535897932385);

double circle_area = pi_v<double> * radius * radius;
float circle_area_f = pi_v<float> * radius * radius;

// Binary literals and digit separators
int binary = 0b1010'1100;      // Binary with separator
int decimal = 1'000'000;       // Million with separators
int hex = 0xFF'FF'FF'FF;       // Hex with separators

// Improved make_unique (now standard)
auto planet = std::make_unique<Planet>("Venus", 800.0);
```

#### **Return Type Deduction**

```cpp
// Function template with deduced return type
template<typename Container>
auto get_first_element(Container&& c) {  // Return type deduced
    return *c.begin();
}

// Conditional return type deduction
template<typename T>
auto process_data(T&& data) {
    if constexpr (std::is_arithmetic_v<T>) {
        return data * 2;     // Arithmetic types get doubled
    } else {
        return data;         // Others returned as-is
    }
}

// Recursive function with deduced return type
auto fibonacci(int n) {
    if (n <= 1) return n;
    return fibonacci(n-1) + fibonacci(n-2);
}
```

### 🎨 C++17 Modern Features

#### **Structured Bindings**

```cpp
#include <tuple>
#include <map>

// Tuple unpacking
std::tuple<std::string, int, double> get_planet_info() {
    return {"Mars", 687, 6.39e23};
}

auto [name, orbit_days, mass] = get_planet_info();  // Structured binding
std::cout << name << " orbits in " << orbit_days << " days\n";

// Pair unpacking
std::map<std::string, double> resources;
resources["Gold"] = 100.5;
resources["Platinum"] = 50.2;

for (const auto& [resource_name, amount] : resources) {
    std::cout << resource_name << ": " << amount << "\n";
}

// Array unpacking
int coordinates[3] = {10, 20, 30};
auto [x, y, z] = coordinates;

// Custom class structured bindings
class Point3D {
private:
    double x_, y_, z_;

public:
    Point3D(double x, double y, double z) : x_(x), y_(y), z_(z) {}

    // Enable structured bindings
    template<size_t I>
    auto& get() & {
        if constexpr (I == 0) return x_;
        else if constexpr (I == 1) return y_;
        else if constexpr (I == 2) return z_;
    }

    template<size_t I>
    const auto& get() const & {
        if constexpr (I == 0) return x_;
        else if constexpr (I == 1) return y_;
        else if constexpr (I == 2) return z_;
    }
};

// Specializations needed for structured bindings
namespace std {
    template<>
    struct tuple_size<Point3D> : std::integral_constant<size_t, 3> {};

    template<size_t I>
    struct tuple_element<I, Point3D> {
        using type = double;
    };
}

Point3D point(1.0, 2.0, 3.0);
auto [px, py, pz] = point;  // Now works!
```

#### **If and Switch with Initializers**

```cpp
// If with initializer
if (auto it = planets.find("Earth"); it != planets.end()) {
    // Use 'it' here - scope limited to if statement
    std::cout << "Found Earth: " << it->second.getName() << "\n";
}

// Traditional equivalent would pollute outer scope:
// auto it = planets.find("Earth");  // 'it' lives beyond if
// if (it != planets.end()) { ... }

// Switch with initializer
switch (auto status = mission.getStatus(); status) {
    case MissionStatus::Active:
        std::cout << "Mission is active\n";
        break;
    case MissionStatus::Completed:
        std::cout << "Mission completed\n";
        break;
    default:
        std::cout << "Unknown status: " << static_cast<int>(status) << "\n";
}

// Constexpr if for template metaprogramming
template<typename T>
void process_type() {
    if constexpr (std::is_integral_v<T>) {
        std::cout << "Processing integer type\n";
    } else if constexpr (std::is_floating_point_v<T>) {
        std::cout << "Processing floating point type\n";
    } else {
        std::cout << "Processing other type\n";
    }
}
```

#### **Optional, Variant, and Any**

```cpp
#include <optional>
#include <variant>
#include <any>

// std::optional - represents a value that may or may not exist
std::optional<Planet> find_planet(const std::string& name) {
    // Search logic...
    if (found) {
        return Planet(name, resources);
    }
    return std::nullopt;  // or return {};
}

void optional_demo() {
    auto planet = find_planet("Unknown");

    if (planet) {  // or planet.has_value()
        std::cout << "Found: " << planet->getName() << "\n";
        std::cout << "Resources: " << planet.value().getResources() << "\n";
    } else {
        std::cout << "Planet not found\n";
    }

    // value_or provides default
    auto default_planet = planet.value_or(Planet("Default", 0.0));

    // Transform optional values
    auto resource_amount = planet.transform([](const Planet& p) {
        return p.getResources();
    });  // Returns optional<double>
}

// std::variant - type-safe union
using MissionParameter = std::variant<int, double, std::string, Planet>;

void variant_demo() {
    MissionParameter param = 42;           // Holds int
    param = 3.14;                         // Now holds double
    param = std::string("Exploration");   // Now holds string
    param = Planet("Mars", 500.0);       // Now holds Planet

    // Visit variant with visitor pattern
    std::visit([](const auto& value) {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, int>) {
            std::cout << "Integer: " << value << "\n";
        } else if constexpr (std::is_same_v<T, double>) {
            std::cout << "Double: " << value << "\n";
        } else if constexpr (std::is_same_v<T, std::string>) {
            std::cout << "String: " << value << "\n";
        } else if constexpr (std::is_same_v<T, Planet>) {
            std::cout << "Planet: " << value.getName() << "\n";
        }
    }, param);

    // Access specific type
    if (std::holds_alternative<Planet>(param)) {
        Planet& planet = std::get<Planet>(param);
        std::cout << "Planet resources: " << planet.getResources() << "\n";
    }

    // Safe access with get_if
    if (auto* planet_ptr = std::get_if<Planet>(&param)) {
        std::cout << "Planet name: " << planet_ptr->getName() << "\n";
    }
}

// std::any - can hold any type
void any_demo() {
    std::any data;

    data = 42;
    data = std::string("Hello");
    data = Planet("Earth", 1000.0);

    // Type checking
    if (data.type() == typeid(Planet)) {
        std::cout << "Contains a Planet\n";
    }

    // Access with any_cast
    try {
        Planet planet = std::any_cast<Planet>(data);
        std::cout << "Planet: " << planet.getName() << "\n";
    } catch (const std::bad_any_cast& e) {
        std::cout << "Wrong type: " << e.what() << "\n";
    }

    // Safe pointer access
    if (auto* planet_ptr = std::any_cast<Planet>(&data)) {
        std::cout << "Planet resources: " << planet_ptr->getResources() << "\n";
    }
}
```

#### **String View**

```cpp
#include <string_view>

// Efficient string processing without copies
void process_string(std::string_view sv) {  // No copy, just a view
    std::cout << "Processing: " << sv << " (length: " << sv.size() << ")\n";

    // String operations
    if (sv.starts_with("Planet_")) {
        std::cout << "This is a planet identifier\n";
    }

    if (sv.ends_with("_Active")) {
        std::cout << "This entity is active\n";
    }

    auto pos = sv.find("Mars");
    if (pos != std::string_view::npos) {
        std::cout << "Found Mars at position " << pos << "\n";
    }
}

void string_view_demo() {
    // Works with string literals
    process_string("Planet_Mars_Active");

    // Works with std::string
    std::string planet_name = "Planet_Earth_Inactive";
    process_string(planet_name);

    // Works with char arrays
    char buffer[] = "Planet_Venus_Active";
    process_string(buffer);

    // Substring views (no allocation)
    std::string full_name = "Planet_Jupiter_Moon_Europa";
    std::string_view planet_part = std::string_view(full_name).substr(0, 15);
    process_string(planet_part);  // "Planet_Jupiter"
}

// Efficient string parsing
std::vector<std::string_view> split_string(std::string_view sv, char delimiter) {
    std::vector<std::string_view> parts;
    size_t start = 0;

    while (true) {
        size_t end = sv.find(delimiter, start);
        if (end == std::string_view::npos) {
            parts.push_back(sv.substr(start));
            break;
        }

        parts.push_back(sv.substr(start, end - start));
        start = end + 1;
    }

    return parts;
}
```

#### **Parallel Algorithms**

```cpp
#include <execution>
#include <algorithm>
#include <numeric>

void parallel_algorithms_demo() {
    std::vector<int> large_dataset(1000000);
    std::iota(large_dataset.begin(), large_dataset.end(), 1);  // Fill 1 to 1000000

    // Sequential execution (default)
    std::sort(std::execution::seq, large_dataset.begin(), large_dataset.end());

    // Parallel execution
    std::sort(std::execution::par, large_dataset.begin(), large_dataset.end());

    // Parallel unsequenced execution (vectorization allowed)
    std::sort(std::execution::par_unseq, large_dataset.begin(), large_dataset.end());

    // Parallel for_each
    std::for_each(std::execution::par, large_dataset.begin(), large_dataset.end(),
                  [](int& value) {
                      value = value * value;  // Square each element
                  });

    // Parallel reduce
    long long sum = std::reduce(std::execution::par,
                               large_dataset.begin(), large_dataset.end(), 0LL);

    // Parallel transform
    std::vector<double> sqrt_values(large_dataset.size());
    std::transform(std::execution::par,
                   large_dataset.begin(), large_dataset.end(),
                   sqrt_values.begin(),
                   [](int value) { return std::sqrt(value); });

    // Parallel find
    auto it = std::find_if(std::execution::par,
                          large_dataset.begin(), large_dataset.end(),
                          [](int value) { return value > 500000; });
}
```

### 🔮 C++20 Revolutionary Features

#### **Concepts**

```cpp
#include <concepts>

// Basic concepts
template<typename T>
concept Numeric = std::integral<T> || std::floating_point<T>;

template<typename T>
concept Drawable = requires(T obj) {
    obj.draw();
    obj.move(0, 0);
    { obj.getPosition() } -> std::convertible_to<std::pair<int, int>>;
};

// Function using concepts
template<Numeric T>
T square(T value) {
    return value * value;
}

// Concept for space entities
template<typename T>
concept SpaceEntity = requires(T entity) {
    { entity.getId() } -> std::convertible_to<std::string>;
    { entity.getPosition() } -> std::convertible_to<Point3D>;
    entity.update();
    entity.render();
} && std::is_base_of_v<Entity, T>;

// Using concept with abbreviated function template
void update_entity(SpaceEntity auto& entity) {  // Abbreviated syntax
    entity.update();
}

// Concept subsumption (more specific concepts are preferred)
template<typename T>
concept Integral = std::integral<T>;

template<typename T>
concept SignedIntegral = Integral<T> && std::signed_integral<T>;

template<Integral T>
void process(T value) {
    std::cout << "Processing integral\n";
}

template<SignedIntegral T>  // More constrained, will be chosen
void process(T value) {
    std::cout << "Processing signed integral\n";
}
```

#### **Ranges Library**

```cpp
#include <ranges>
#include <algorithm>

void ranges_demo() {
    std::vector<Planet> planets = getPlanets();

    // Range-based algorithms
    std::ranges::sort(planets, {}, &Planet::getResources);  // Sort by resources

    // Range views (lazy evaluation)
    auto habitable_planets = planets
        | std::views::filter([](const Planet& p) { return p.isHabitable(); })
        | std::views::transform([](const Planet& p) { return p.getName(); });

    // Consume the view
    for (const auto& name : habitable_planets) {
        std::cout << "Habitable planet: " << name << "\n";
    }

    // Complex range pipeline
    std::vector<int> numbers{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    auto result = numbers
        | std::views::filter([](int n) { return n % 2 == 0; })  // Even numbers
        | std::views::transform([](int n) { return n * n; })     // Square them
        | std::views::take(3);                                   // Take first 3

    std::vector<int> final_result(result.begin(), result.end());  // {4, 16, 36}

    // Range adaptors
    auto reversed = numbers | std::views::reverse;
    auto first_five = numbers | std::views::take(5);
    auto skip_three = numbers | std::views::drop(3);

    // Generate ranges
    auto infinite_numbers = std::views::iota(1);  // 1, 2, 3, 4, ...
    auto first_ten = infinite_numbers | std::views::take(10);
}
```

#### **Coroutines**

```cpp
#include <coroutine>
#include <iostream>

// Simple generator
template<typename T>
class Generator {
public:
    struct promise_type {
        T current_value;

        Generator get_return_object() {
            return Generator{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::suspend_always initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }

        std::suspend_always yield_value(T value) {
            current_value = value;
            return {};
        }

        void return_void() {}
        void unhandled_exception() { std::terminate(); }
    };

    std::coroutine_handle<promise_type> coro;

    Generator(std::coroutine_handle<promise_type> h) : coro(h) {}
    ~Generator() { if (coro) coro.destroy(); }

    // Iterator interface
    class iterator {
        std::coroutine_handle<promise_type> coro_;
        bool done_;

    public:
        iterator(std::coroutine_handle<promise_type> coro, bool done)
            : coro_(coro), done_(done) {}

        iterator& operator++() {
            coro_.resume();
            done_ = coro_.done();
            return *this;
        }

        T operator*() const {
            return coro_.promise().current_value;
        }

        bool operator==(const iterator& other) const {
            return done_ == other.done_;
        }

        bool operator!=(const iterator& other) const {
            return !(*this == other);
        }
    };

    iterator begin() {
        coro.resume();
        return iterator{coro, coro.done()};
    }

    iterator end() {
        return iterator{coro, true};
    }
};

// Coroutine function
Generator<int> fibonacci() {
    int a = 0, b = 1;
    co_yield a;
    co_yield b;

    while (true) {
        int next = a + b;
        co_yield next;
        a = b;
        b = next;
    }
}

void coroutine_demo() {
    auto fib = fibonacci();

    int count = 0;
    for (auto value : fib) {
        std::cout << value << " ";
        if (++count >= 10) break;  // First 10 Fibonacci numbers
    }
    std::cout << "\n";
}
```

#### **Modules (Basic Example)**

```cpp
// planet_module.ixx - Module interface
export module planet;

import <string>;
import <iostream>;

export class Planet {
private:
    std::string name_;
    double resources_;

public:
    Planet(const std::string& name, double resources)
        : name_(name), resources_(resources) {}

    const std::string& getName() const { return name_; }
    double getResources() const { return resources_; }

    void display() const {
        std::cout << "Planet: " << name_ << ", Resources: " << resources_ << "\n";
    }
};

export void explorePlanet(const Planet& planet) {
    std::cout << "Exploring " << planet.getName() << "...\n";
}

// main.cpp - Using the module
import planet;

int main() {
    Planet mars("Mars", 500.0);
    mars.display();
    explorePlanet(mars);
    return 0;
}
```

### ⚡ C++23 Latest Features

#### **Multidimensional Subscript Operator**

```cpp
// C++23 allows multiple arguments in operator[]
class Matrix3D {
private:
    std::vector<std::vector<std::vector<double>>> data_;

public:
    Matrix3D(size_t x, size_t y, size_t z)
        : data_(x, std::vector<std::vector<double>>(y, std::vector<double>(z))) {}

    // C++23: Multi-dimensional subscript
    double& operator[](size_t x, size_t y, size_t z) {
        return data_[x][y][z];
    }

    const double& operator[](size_t x, size_t y, size_t z) const {
        return data_[x][y][z];
    }
};

void multidim_subscript_demo() {
    Matrix3D space(10, 10, 10);
    space[5, 3, 7] = 42.0;  // C++23 syntax!

    std::cout << "Value at [5,3,7]: " << space[5, 3, 7] << "\n";
}
```

#### **if consteval**

```cpp
#include <type_traits>

consteval int compile_time_factorial(int n) {
    return (n <= 1) ? 1 : n * compile_time_factorial(n - 1);
}

constexpr int runtime_factorial(int n) {
    int result = 1;
    for (int i = 2; i <= n; ++i) {
        result *= i;
    }
    return result;
}

constexpr int smart_factorial(int n) {
    if consteval {  // C++23: Executed at compile time
        return compile_time_factorial(n);
    } else {        // Executed at runtime
        return runtime_factorial(n);
    }
}
```

#### **Deducing this**

```cpp
// C++23: Explicit object parameter (deducing this)
class FluentInterface {
private:
    std::string data_;

public:
    // C++23: Explicit object parameter
    template<typename Self>
    auto&& set_data(this Self&& self, std::string data) {
        self.data_ = std::move(data);
        return std::forward<Self>(self);
    }

    template<typename Self>
    auto&& process(this Self&& self) {
        // Process data...
        return std::forward<Self>(self);
    }

    const std::string& get_data() const { return data_; }
};

void deducing_this_demo() {
    FluentInterface obj;

    // Method chaining works with both lvalues and rvalues
    obj.set_data("Hello").process();

    auto result = FluentInterface{}
        .set_data("World")
        .process()
        .get_data();
}
```

### 🎯 Modern C++ Best Practices

#### **RAII and Resource Management**

```cpp
// Modern resource management
class FileManager {
private:
    std::unique_ptr<FILE, decltype(&fclose)> file_;

public:
    FileManager(const std::string& filename)
        : file_(fopen(filename.c_str(), "r"), &fclose) {
        if (!file_) {
            throw std::runtime_error("Failed to open file");
        }
    }

    // Rule of Zero - no need for destructor, copy, or move
    // unique_ptr handles everything

    void process() {
        // File operations...
        // File automatically closed when FileManager is destroyed
    }
};

// RAII with custom resources
class MissionLock {
private:
    std::function<void()> cleanup_;

public:
    template<typename Setup, typename Cleanup>
    MissionLock(Setup&& setup, Cleanup&& cleanup)
        : cleanup_(std::forward<Cleanup>(cleanup)) {
        setup();
    }

    ~MissionLock() {
        if (cleanup_) cleanup_();
    }

    // Non-copyable, movable
    MissionLock(const MissionLock&) = delete;
    MissionLock& operator=(const MissionLock&) = delete;

    MissionLock(MissionLock&& other) noexcept
        : cleanup_(std::move(other.cleanup_)) {
        other.cleanup_ = nullptr;
    }
};

void raii_demo() {
    MissionLock lock(
        []() { std::cout << "Mission started\n"; },      // Setup
        []() { std::cout << "Mission cleanup\n"; }       // Cleanup
    );

    // Mission operations...
    // Cleanup automatically called when lock goes out of scope
}
```

#### **Type Safety and Strong Types**

```cpp
// Strong type wrappers for type safety
template<typename T, typename Tag>
class StrongType {
private:
    T value_;

public:
    explicit StrongType(const T& value) : value_(value) {}
    explicit StrongType(T&& value) : value_(std::move(value)) {}

    const T& get() const { return value_; }
    T& get() { return value_; }

    // Comparison operators
    bool operator==(const StrongType& other) const { return value_ == other.value_; }
    bool operator<(const StrongType& other) const { return value_ < other.value_; }
};

// Define strong types
using PlanetId = StrongType<std::string, struct PlanetIdTag>;
using FleetId = StrongType<std::string, struct FleetIdTag>;
using ResourceAmount = StrongType<double, struct ResourceAmountTag>;

// Type-safe function parameters
void transfer_resources(PlanetId from, PlanetId to, ResourceAmount amount) {
    std::cout << "Transferring " << amount.get()
              << " from " << from.get()
              << " to " << to.get() << "\n";
}

void strong_types_demo() {
    PlanetId earth{"Earth"};
    PlanetId mars{"Mars"};
    ResourceAmount gold{100.5};

    transfer_resources(earth, mars, gold);

    // This would be a compile-time error:
    // transfer_resources(mars, gold, earth);  // Wrong parameter order caught!
}
```

#### **Functional Programming Style**

```cpp
#include <functional>

// Higher-order functions
template<typename Container, typename Predicate>
auto filter(const Container& container, Predicate pred) {
    Container result;
    std::copy_if(container.begin(), container.end(),
                 std::back_inserter(result), pred);
    return result;
}

template<typename Container, typename Transform>
auto map(const Container& container, Transform trans) {
    using ValueType = decltype(trans(*container.begin()));
    std::vector<ValueType> result;

    std::transform(container.begin(), container.end(),
                   std::back_inserter(result), trans);
    return result;
}

template<typename Container, typename BinaryOp>
auto reduce(const Container& container, BinaryOp op) {
    return std::accumulate(std::next(container.begin()),
                          container.end(),
                          *container.begin(),
                          op);
}

void functional_demo() {
    std::vector<int> numbers{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    // Filter even numbers
    auto evens = filter(numbers, [](int n) { return n % 2 == 0; });

    // Square each number
    auto squares = map(numbers, [](int n) { return n * n; });

    // Sum all numbers
    auto sum = reduce(numbers, std::plus<int>{});

    // Chained operations
    auto result = numbers
        | std::views::filter([](int n) { return n % 2 == 0; })  // C++20 ranges
        | std::views::transform([](int n) { return n * n; });

    std::vector<int> final_result(result.begin(), result.end());
}

// Currying and partial application
template<typename F>
class Curried {
private:
    F func_;

public:
    Curried(F func) : func_(func) {}

    template<typename... Args>
    auto operator()(Args... args) {
        if constexpr (std::is_invocable_v<F, Args...>) {
            return func_(args...);
        } else {
            return [=](auto... rest_args) {
                return func_(args..., rest_args...);
            };
        }
    }
};

template<typename F>
auto curry(F func) {
    return Curried<F>{func};
}

void currying_demo() {
    auto add = curry([](int a, int b, int c) { return a + b + c; });

    auto add_5 = add(5);        // Partially applied
    auto add_5_3 = add_5(3);    // More partial application

    int result = add_5_3(2);    // Final application: 5 + 3 + 2 = 10
}
```

#### **Modern Error Handling**

```cpp
#include <expected>  // C++23

// Using std::expected for error handling (C++23)
enum class ErrorCode {
    FileNotFound,
    PermissionDenied,
    InvalidData
};

std::expected<std::string, ErrorCode> read_config_file(const std::string& filename) {
    // Simulate file reading
    if (filename.empty()) {
        return std::unexpected(ErrorCode::FileNotFound);
    }

    if (filename == "restricted.conf") {
        return std::unexpected(ErrorCode::PermissionDenied);
    }

    return std::string{"config data"};  // Success case
}

void modern_error_handling() {
    auto result = read_config_file("app.conf");

    if (result) {
        std::cout << "Config: " << *result << "\n";
    } else {
        switch (result.error()) {
            case ErrorCode::FileNotFound:
                std::cout << "File not found\n";
                break;
            case ErrorCode::PermissionDenied:
                std::cout << "Permission denied\n";
                break;
            case ErrorCode::InvalidData:
                std::cout << "Invalid data\n";
                break;
        }
    }

    // Monadic operations (C++23)
    auto processed = read_config_file("app.conf")
        .and_then([](const std::string& config) -> std::expected<int, ErrorCode> {
            if (config.empty()) {
                return std::unexpected(ErrorCode::InvalidData);
            }
            return static_cast<int>(config.size());
        })
        .or_else([](ErrorCode error) -> std::expected<int, ErrorCode> {
            std::cout << "Using default config size\n";
            return 100;  // Default value
        });
}

// RAII exception safety
class ExceptionSafeOperation {
private:
    std::vector<std::function<void()>> cleanup_stack_;
    bool committed_ = false;

public:
    template<typename Operation, typename Cleanup>
    void add_operation(Operation&& op, Cleanup&& cleanup) {
        cleanup_stack_.push_back(cleanup);
        try {
            op();
        } catch (...) {
            // Cleanup in reverse order
            for (auto it = cleanup_stack_.rbegin(); it != cleanup_stack_.rend(); ++it) {
                try { (*it)(); } catch (...) { /* Log but don't throw */ }
            }
            throw;
        }
    }

    void commit() {
        committed_ = true;
        cleanup_stack_.clear();
    }

    ~ExceptionSafeOperation() {
        if (!committed_) {
            for (auto it = cleanup_stack_.rbegin(); it != cleanup_stack_.rend(); ++it) {
                try { (*it)(); } catch (...) { /* Log but don't throw */ }
            }
        }
    }
};
```

### 📊 Performance Optimizations

#### **Move Semantics Best Practices**

```cpp
class OptimizedContainer {
private:
    std::vector<std::unique_ptr<Planet>> planets_;

public:
    // Return by value for optimal move semantics
    std::vector<std::unique_ptr<Planet>> extract_planets() && {
        return std::move(planets_);  // Move from rvalue object
    }

    // Prevent extraction from lvalue (optional)
    std::vector<std::unique_ptr<Planet>> extract_planets() & = delete;

    // Efficient insertion
    void add_planet(std::unique_ptr<Planet> planet) {
        planets_.push_back(std::move(planet));
    }

    // Universal reference for perfect forwarding
    template<typename... Args>
    void emplace_planet(Args&&... args) {
        planets_.emplace_back(std::make_unique<Planet>(std::forward<Args>(args)...));
    }

    // Ref-qualified member functions
    const std::vector<std::unique_ptr<Planet>>& get_planets() const& {
        return planets_;  // Return reference for lvalue
    }

    std::vector<std::unique_ptr<Planet>>&& get_planets() && {
        return std::move(planets_);  // Return moved value for rvalue
    }
};

// Usage
void move_semantics_usage() {
    OptimizedContainer container;
    container.emplace_planet("Mars", 500.0);

    // Extract from temporary (rvalue)
    auto planets = std::move(container).extract_planets();

    // This would be a compile error:
    // OptimizedContainer container2;
    // auto planets2 = container2.extract_planets();  // Deleted for lvalue
}
```

#### **Compile-Time Programming**

```cpp
// Compile-time computations
template<int N>
constexpr int fibonacci_ct() {
    if constexpr (N <= 1) {
        return N;
    } else {
        return fibonacci_ct<N-1>() + fibonacci_ct<N-2>();
    }
}

// Compile-time string processing
constexpr bool is_valid_planet_name(std::string_view name) {
    return !name.empty() &&
           std::isupper(name[0]) &&
           name.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz ") == std::string_view::npos;
}

// Compile-time configuration
template<std::string_view ConfigName>
constexpr auto get_config_value() {
    if constexpr (ConfigName == "max_planets") {
        return 100;
    } else if constexpr (ConfigName == "default_resources") {
        return 1000.0;
    } else {
        return 0;
    }
}

// Usage at compile time
constexpr int fib_10 = fibonacci_ct<10>();
constexpr bool valid_name = is_valid_planet_name("Earth");
constexpr int max_planets = get_config_value<"max_planets">();

// NTTP (Non-Type Template Parameters) with auto
template<auto Value>
constexpr auto constant_v = Value;

constexpr auto pi = constant_v<3.14159>;
constexpr auto greeting = constant_v<"Hello World">;
```

### 📋 Modern C++ Quick Reference

| Feature                  | C++ Version | Example                                | Use Case                 |
| ------------------------ | ----------- | -------------------------------------- | ------------------------ |
| `auto`                   | C++11       | `auto x = 42;`                         | Type deduction           |
| Range-for                | C++11       | `for (const auto& item : container)`   | Iteration                |
| Lambdas                  | C++11       | `[](int x){ return x*2; }`             | Function objects         |
| Smart Pointers           | C++11       | `std::unique_ptr<T>`                   | Memory management        |
| Move Semantics           | C++11       | `std::move(obj)`                       | Performance              |
| Generic Lambdas          | C++14       | `[](auto x){ return x; }`              | Generic code             |
| `make_unique`            | C++14       | `std::make_unique<T>()`                | Safe construction        |
| Structured Bindings      | C++17       | `auto [a, b] = pair;`                  | Unpacking                |
| `if constexpr`           | C++17       | `if constexpr (condition)`             | Template metaprogramming |
| `std::optional`          | C++17       | `std::optional<T>`                     | Maybe types              |
| `std::variant`           | C++17       | `std::variant<int, string>`            | Tagged unions            |
| Parallel Algorithms      | C++17       | `std::sort(std::execution::par, ...)`  | Parallelism              |
| Concepts                 | C++20       | `template<Concept T>`                  | Type constraints         |
| Ranges                   | C++20       | `vec \| filter(...) \| transform(...)` | Functional programming   |
| Coroutines               | C++20       | `co_yield value;`                      | Generators/async         |
| Modules                  | C++20       | `import module;`                       | Better compilation       |
| `operator[]` multi-param | C++23       | `matrix[x, y, z]`                      | Multi-dimensional access |

### 🎯 Key Modern C++ Principles

1. **Prefer `auto`** - Let the compiler deduce types
2. **Use range-based for loops** - Cleaner iteration
3. **Embrace lambdas** - Inline function objects
4. **Smart pointers everywhere** - Automatic memory management
5. **Move semantics** - Avoid unnecessary copies
6. **`constexpr` all the things** - Compile-time computation
7. **Structured bindings** - Clean unpacking
8. **Concepts over SFINAE** - Better error messages
9. **Ranges over raw loops** - Functional composition
10. **RAII for all resources** - Automatic cleanup

### 🚀 Adoption Strategy

#### **Incremental Adoption**

```cpp
// Start with these immediately:
auto variable = expression;                    // Type deduction
for (const auto& item : container) {}         // Range-based loops
std::make_unique<Type>(args)                  // Smart pointers
std::move(expensive_object)                   // Move semantics

// Add when comfortable:
[capture](params) { /* body */ }              // Lambdas
if constexpr (compile_time_condition) {}      // Conditional compilation
std::optional<Type> maybe_value;              // Optional values

// Advanced features:
template<Concept T> void func(T param) {}     // Concepts (C++20)
auto result = range | filter | transform;     // Ranges (C++20)
Generator<int> fibonacci_sequence() {}        // Coroutines (C++20)
```

---

_This cheat sheet covers the evolution of C++ from C++11 to C++23. For practical examples of these features in action, see various modules throughout the `src/` directory, especially `src/modern/`._
