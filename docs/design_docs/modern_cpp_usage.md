# Modern C++ Usage - CppVerseHub

**Location:** `CppVerseHub/docs/design_docs/modern_cpp_usage.md`

## Modern C++ Philosophy

CppVerseHub extensively leverages C++17, C++20, and emerging C++23 features to demonstrate state-of-the-art C++ programming practices. This document explains the rationale behind using modern C++ features, their benefits in context, and how they contribute to code quality, performance, and maintainability.

## Feature Selection Criteria

### Adoption Strategy

1. **Standard Compliance**: Only use standardized features (no experimental extensions)
2. **Compiler Support**: Features must be supported by major compilers (GCC 10+, Clang 11+, MSVC 19.28+)
3. **Performance Benefits**: Features should improve runtime or compile-time performance
4. **Safety Improvements**: Prefer features that prevent common programming errors
5. **Educational Value**: Demonstrate practical usage of modern language capabilities

### Progressive Feature Usage

- **C++17**: Baseline requirement for all code
- **C++20**: Used where compiler support is available
- **C++23**: Early adoption of stable features with fallbacks

## C++17 Features Implementation

### Structured Bindings

Structured bindings improve code readability when dealing with pairs, tuples, and custom types:

```cpp
// Planet resource management with structured bindings
class Planet {
    std::unordered_map<ResourceType, ResourceInfo> resources_;

public:
    void updateResources() {
        for (const auto& [resource_type, resource_info] : resources_) {
            auto [current_level, regeneration_rate, maximum_capacity] = resource_info;

            // Calculate new resource level
            double new_level = std::min(
                current_level + regeneration_rate * getDeltaTime(),
                maximum_capacity
            );

            resources_[resource_type].current_level = new_level;
        }
    }

    std::optional<std::pair<ResourceType, double>> findLowestResource() const {
        if (resources_.empty()) {
            return std::nullopt;
        }

        auto lowest_iter = std::min_element(resources_.begin(), resources_.end(),
            [](const auto& a, const auto& b) {
                const auto& [type_a, info_a] = a;
                const auto& [type_b, info_b] = b;
                return info_a.current_level < info_b.current_level;
            });

        const auto& [resource_type, resource_info] = *lowest_iter;
        return std::make_pair(resource_type, resource_info.current_level);
    }
};
```

**Benefits:**

- **Readability**: Clear variable names instead of `.first`, `.second`
- **Maintainability**: Changes to underlying structure don't break access patterns
- **Performance**: Zero-cost abstraction with compiler optimization

### std::optional and Error Handling

Modern error handling using `std::optional` instead of error codes or exceptions:

```cpp
class NavigationSystem {
public:
    std::optional<Route> calculateRoute(const Coordinates& from, const Coordinates& to) const {
        if (!isValidCoordinate(from) || !isValidCoordinate(to)) {
            return std::nullopt;
        }

        if (auto direct_route = tryDirectPath(from, to)) {
            return direct_route;
        }

        if (auto safe_route = trySafePath(from, to)) {
            return safe_route;
        }

        return std::nullopt; // No valid route found
    }

    // Chaining optional operations
    std::optional<double> getRouteDistance(const Coordinates& from, const Coordinates& to) const {
        return calculateRoute(from, to)
            .transform([](const Route& route) { return route.getTotalDistance(); });
    }

    // Using optional in mission planning
    bool planMission(const MissionParameters& params) {
        auto route = calculateRoute(params.start, params.destination);
        auto fuel_estimate = route.transform([](const Route& r) { return r.getFuelRequirement(); });
        auto time_estimate = route.transform([](const Route& r) { return r.getTimeRequirement(); });

        if (route && fuel_estimate && time_estimate) {
            auto& fleet = getFleet(params.fleet_id);

            if (fleet.getFuelLevel() >= *fuel_estimate &&
                fleet.getTimeAvailable() >= *time_estimate) {

                executeMission(params, *route);
                return true;
            }
        }

        return false;
    }
};
```

**Advantages:**

- **Explicit Failure**: Optional makes potential failure visible in type system
- **No Exceptions**: Avoids exception overhead for expected failure cases
- **Composable**: Monadic operations with `transform`, `and_then`
- **Memory Efficient**: No heap allocation for simple failure cases

### std::variant for Type-Safe Unions

Using `std::variant` for type-safe polymorphism alternatives:

```cpp
// Different types of mission results
using MissionResult = std::variant<
    SuccessResult,
    PartialFailureResult,
    CompleteFailureResult,
    AbortedResult
>;

class Mission {
public:
    virtual MissionResult execute() = 0;

protected:
    // Type-safe result processing
    template<typename ResultHandler>
    void processResult(const MissionResult& result, ResultHandler&& handler) {
        std::visit(std::forward<ResultHandler>(handler), result);
    }
};

class ExplorationMission : public Mission {
public:
    MissionResult execute() override {
        try {
            auto exploration_data = conductExploration();

            if (exploration_data.planets_discovered > 0) {
                return SuccessResult{
                    .data_collected = exploration_data,
                    .mission_duration = getMissionDuration(),
                    .resources_consumed = getResourcesUsed()
                };
            } else {
                return PartialFailureResult{
                    .reason = "No new planets discovered",
                    .partial_data = exploration_data.sensor_readings
                };
            }

        } catch (const CriticalSystemFailure& e) {
            return CompleteFailureResult{
                .error_message = e.what(),
                .recovery_suggestions = generateRecoverySuggestions()
            };
        }
    }

private:
    void reportResults(const MissionResult& result) {
        processResult(result, [this](const auto& specific_result) {
            using ResultType = std::decay_t<decltype(specific_result)>;

            if constexpr (std::is_same_v<ResultType, SuccessResult>) {
                logSuccess(specific_result);
                updatePlanetDatabase(specific_result.data_collected);
            } else if constexpr (std::is_same_v<ResultType, PartialFailureResult>) {
                logWarning(specific_result.reason);
                if (!specific_result.partial_data.empty()) {
                    updatePlanetDatabase(specific_result.partial_data);
                }
            } else if constexpr (std::is_same_v<ResultType, CompleteFailureResult>) {
                logError(specific_result.error_message);
                initiateEmergencyProtocol();
            }
        });
    }
};
```

**Key Benefits:**

- **Type Safety**: Compile-time guarantee of handling all cases
- **Performance**: No virtual function call overhead
- **Memory Efficiency**: Single allocation for all variant types
- **Pattern Matching**: Visitor pattern with generic lambda support

### constexpr Programming

Compile-time computation for performance-critical calculations:

```cpp
// Compile-time mathematical constants and functions
namespace MathConstants {
    constexpr double PI = 3.14159265358979323846;
    constexpr double LIGHT_SPEED = 299792458.0; // m/s
    constexpr double AU = 149597870700.0; // meters in astronomical unit

    constexpr double toRadians(double degrees) {
        return degrees * PI / 180.0;
    }

    constexpr double distanceInAU(double meters) {
        return meters / AU;
    }
}

// Compile-time coordinate system calculations
class Coordinates {
private:
    double x_, y_, z_;

public:
    constexpr Coordinates(double x, double y, double z) : x_(x), y_(y), z_(z) {}

    constexpr double distance(const Coordinates& other) const {
        double dx = x_ - other.x_;
        double dy = y_ - other.y_;
        double dz = z_ - other.z_;
        return std::sqrt(dx*dx + dy*dy + dz*dz);
    }

    constexpr Coordinates operator+(const Coordinates& other) const {
        return Coordinates(x_ + other.x_, y_ + other.y_, z_ + other.z_);
    }

    constexpr Coordinates normalize() const {
        double magnitude = std::sqrt(x_*x_ + y_*y_ + z_*z_);
        return magnitude > 0 ? Coordinates(x_/magnitude, y_/magnitude, z_/magnitude)
                             : Coordinates(0, 0, 0);
    }
};

// Compile-time lookup tables for performance
template<size_t Size>
struct CompileTimeLookupTable {
    std::array<double, Size> values{};

    constexpr CompileTimeLookupTable() {
        for (size_t i = 0; i < Size; ++i) {
            values[i] = std::sin(2 * MathConstants::PI * i / Size);
        }
    }

    constexpr double operator[](size_t index) const {
        return values[index % Size];
    }
};

// Pre-computed sine table available at compile time
constexpr auto SINE_TABLE = CompileTimeLookupTable<360>{};

// Usage in runtime code
double fastSine(double angle_degrees) {
    // Convert to table index
    int index = static_cast<int>(angle_degrees) % 360;
    if (index < 0) index += 360;

    return SINE_TABLE[index]; // No runtime computation!
}
```

**Performance Benefits:**

- **Zero Runtime Cost**: Calculations moved to compile time
- **Optimized Code**: Compiler can inline and optimize aggressively
- **Reduced Binary Size**: Pre-computed constants stored efficiently
- **Type Safety**: constexpr functions validated at compile time

## C++20 Features Implementation

### Concepts for Template Constraints

Type-safe template programming with clear error messages:

```cpp
#include <concepts>

// Define concepts for our domain
template<typename T>
concept Entity = requires(T t) {
    { t.getId() } -> std::convertible_to<EntityId>;
    { t.getPosition() } -> std::convertible_to<Coordinates>;
    { t.update(1.0f) } -> std::same_as<void>;
    typename T::entity_type;
};

template<typename T>
concept Moveable = Entity<T> && requires(T t, const Coordinates& coords) {
    { t.moveTo(coords) } -> std::same_as<void>;
    { t.getVelocity() } -> std::convertible_to<Velocity>;
};

template<typename T>
concept ResourceConsumer = requires(T t, ResourceType type, double amount) {
    { t.consumeResource(type, amount) } -> std::same_as<bool>;
    { t.getResourceLevel(type) } -> std::convertible_to<double>;
};

// Template functions with concept constraints
template<Moveable EntityType>
void planRoute(EntityType& entity, const Coordinates& destination) {
    auto current_pos = entity.getPosition();
    auto route = calculateOptimalPath(current_pos, destination);

    if (route) {
        entity.moveTo(destination);
    }
}

template<Entity EntityType>
requires ResourceConsumer<EntityType>
bool executeMission(EntityType& entity, const Mission& mission) {
    auto required_resources = mission.getResourceRequirements();

    // Check if entity has sufficient resources
    for (const auto& [resource_type, amount] : required_resources) {
        if (entity.getResourceLevel(resource_type) < amount) {
            return false;
        }
    }

    // Consume resources and execute mission
    for (const auto& [resource_type, amount] : required_resources) {
        entity.consumeResource(resource_type, amount);
    }

    return mission.execute(entity);
}

// Generic container operations with concepts
template<std::ranges::range Container>
requires std::sortable<std::ranges::iterator_t<Container>>
void sortEntitiesByDistance(Container& entities, const Coordinates& reference_point) {
    std::ranges::sort(entities, [&reference_point](const auto& a, const auto& b) {
        return a.getPosition().distance(reference_point) <
               b.getPosition().distance(reference_point);
    });
}
```

**Concept Advantages:**

- **Better Error Messages**: Clear compile-time errors when constraints not met
- **Self-Documenting**: Constraints make template requirements explicit
- **Partial Specialization**: Enable different implementations based on capabilities
- **Composition**: Complex concepts built from simple ones

### Ranges and Views

Modern sequence processing with lazy evaluation:

```cpp
#include <ranges>

class FleetManager {
    std::vector<std::unique_ptr<Fleet>> fleets_;

public:
    // Find combat-ready fleets near a position
    auto getCombatReadyFleets(const Coordinates& position, double max_distance) {
        return fleets_
            | std::views::filter([](const auto& fleet) {
                return fleet->isCombatReady();
              })
            | std::views::filter([position, max_distance](const auto& fleet) {
                return fleet->getPosition().distance(position) <= max_distance;
              })
            | std::views::transform([](const auto& fleet) -> Fleet& {
                return *fleet;
              });
    }

    // Get fleet statistics using ranges
    struct FleetStats {
        size_t total_fleets;
        size_t combat_ready;
        size_t on_mission;
        double average_fuel_level;
    };

    FleetStats getFleetStatistics() const {
        auto combat_ready_count = std::ranges::count_if(fleets_,
            [](const auto& fleet) { return fleet->isCombatReady(); });

        auto on_mission_count = std::ranges::count_if(fleets_,
            [](const auto& fleet) { return fleet->isOnMission(); });

        auto fuel_levels = fleets_
            | std::views::transform([](const auto& fleet) {
                return fleet->getFuelLevel();
              });

        double total_fuel = std::accumulate(fuel_levels.begin(), fuel_levels.end(), 0.0);
        double avg_fuel = fleets_.empty() ? 0.0 : total_fuel / fleets_.size();

        return FleetStats{
            .total_fleets = fleets_.size(),
            .combat_ready = static_cast<size_t>(combat_ready_count),
            .on_mission = static_cast<size_t>(on_mission_count),
            .average_fuel_level = avg_fuel
        };
    }

    // Complex range processing for resource allocation
    void redistributeResources() {
        // Find fleets with excess resources
        auto resource_excess = fleets_
            | std::views::filter([](const auto& fleet) {
                return fleet->getResourceLevel(ResourceType::FUEL) > 0.8 * fleet->getMaxFuel();
              })
            | std::views::transform([](const auto& fleet) {
                return std::make_pair(fleet.get(), fleet->getExcessResources());
              });

        // Find fleets needing resources
        auto resource_deficit = fleets_
            | std::views::filter([](const auto& fleet) {
                return fleet->getResourceLevel(ResourceType::FUEL) < 0.3 * fleet->getMaxFuel();
              })
            | std::views::transform([](const auto& fleet) {
                return std::make_pair(fleet.get(), fleet->getResourceDeficit());
              });

        // Redistribute resources (simplified)
        for (const auto& [excess_fleet, excess_amount] : resource_excess) {
            for (const auto& [deficit_fleet, deficit_amount] : resource_deficit) {
                double transfer_amount = std::min(excess_amount, deficit_amount);
                if (transfer_amount > 0) {
                    transferResources(*excess_fleet, *deficit_fleet, transfer_amount);
                    break;
                }
            }
        }
    }
};
```

**Ranges Benefits:**

- **Lazy Evaluation**: Processing happens only when needed
- **Composability**: Chain operations naturally
- **Performance**: Optimized by compiler, often faster than manual loops
- **Readability**: Express intent clearly without implementation details

### Coroutines for Async Operations

Modern asynchronous programming with coroutines:

```cpp
#include <coroutine>
#include <future>

// Coroutine types for async operations
template<typename T>
struct Task {
    struct promise_type {
        std::optional<T> result;
        std::exception_ptr exception;

        Task get_return_object() {
            return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::suspend_never initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }

        void return_value(T value) {
            result = std::move(value);
        }

        void unhandled_exception() {
            exception = std::current_exception();
        }
    };

    std::coroutine_handle<promise_type> handle;

    explicit Task(std::coroutine_handle<promise_type> h) : handle(h) {}

    ~Task() {
        if (handle) {
            handle.destroy();
        }
    }

    // Move-only semantics
    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;

    Task(Task&& other) noexcept : handle(std::exchange(other.handle, {})) {}
    Task& operator=(Task&& other) noexcept {
        if (this != &other) {
            if (handle) {
                handle.destroy();
            }
            handle = std::exchange(other.handle, {});
        }
        return *this;
    }

    T get() {
        if (!handle.done()) {
            throw std::runtime_error("Task not completed");
        }

        if (handle.promise().exception) {
            std::rethrow_exception(handle.promise().exception);
        }

        return *handle.promise().result;
    }
};

// Awaitable for delayed operations
struct DelayAwaiter {
    std::chrono::milliseconds delay;

    bool await_ready() { return delay.count() == 0; }

    void await_suspend(std::coroutine_handle<> handle) {
        std::thread([handle, delay = this->delay]() {
            std::this_thread::sleep_for(delay);
            handle.resume();
        }).detach();
    }

    void await_resume() {}
};

// Coroutine-based mission execution
class AsyncMissionExecutor {
public:
    Task<MissionResult> executeMissionAsync(std::unique_ptr<Mission> mission) {
        // Pre-mission checks
        if (!mission->isReady()) {
            co_await DelayAwaiter{std::chrono::seconds(1)};
        }

        // Execute mission phases
        auto preparation_result = co_await prepareMissionAsync(*mission);
        if (!preparation_result.success) {
            co_return MissionResult::failure(preparation_result.error);
        }

        auto execution_result = co_await executeMainPhaseAsync(*mission);
        if (!execution_result.success) {
            co_await cleanupMissionAsync(*mission);
            co_return MissionResult::failure(execution_result.error);
        }

        auto cleanup_result = co_await cleanupMissionAsync(*mission);
        co_return MissionResult::success(execution_result.data);
    }

private:
    Task<PhaseResult> prepareMissionAsync(const Mission& mission) {
        // Simulate async preparation
        co_await DelayAwaiter{std::chrono::milliseconds(100)};

        if (checkResourceAvailability(mission)) {
            co_return PhaseResult::success();
        } else {
            co_return PhaseResult::failure("Insufficient resources");
        }
    }

    Task<PhaseResult> executeMainPhaseAsync(const Mission& mission) {
        co_await DelayAwaiter{std::chrono::seconds(2)};

        try {
            auto result = mission.executeMainPhase();
            co_return PhaseResult::success(result);
        } catch (const std::exception& e) {
            co_return PhaseResult::failure(e.what());
        }
    }

    Task<PhaseResult> cleanupMissionAsync(const Mission& mission) {
        co_await DelayAwaiter{std::chrono::milliseconds(50)};
        mission.cleanup();
        co_return PhaseResult::success();
    }
};
```

**Coroutine Benefits:**

- **Natural Async Code**: Looks like synchronous code but executes asynchronously
- **Exception Handling**: Standard try/catch works across suspension points
- **Composability**: Coroutines can call other coroutines naturally
- **Performance**: No callback overhead, efficient state machine generation

### Modules (Where Supported)

Modern modular programming for better compilation:

```cpp
// math_utilities.cppm (module interface)
export module math_utilities;

import <cmath>;
import <numbers>;

export namespace MathUtils {
    constexpr double PI = std::numbers::pi;
    constexpr double E = std::numbers::e;

    template<typename T>
    constexpr T square(T value) {
        return value * value;
    }

    template<typename T>
    T distance3D(T x1, T y1, T z1, T x2, T y2, T z2) {
        return std::sqrt(square(x2-x1) + square(y2-y1) + square(z2-z1));
    }

    class Coordinates {
        // Implementation details not exported
    private:
        double x_, y_, z_;

    public:
        constexpr Coordinates(double x, double y, double z)
            : x_(x), y_(y), z_(z) {}

        constexpr double distance(const Coordinates& other) const {
            return distance3D(x_, y_, z_, other.x_, other.y_, other.z_);
        }
    };
}

// Usage in other files
import math_utilities;

void someFunction() {
    using namespace MathUtils;
    auto pos1 = Coordinates{0, 0, 0};
    auto pos2 = Coordinates{1, 1, 1};

    auto dist = pos1.distance(pos2); // Uses exported interface
}
```

**Module Advantages:**

- **Faster Compilation**: No header parsing overhead
- **Better Encapsulation**: Implementation details truly private
- **Dependency Management**: Clear import/export relationships
- **Reduced Binary Size**: Better dead code elimination

## C++23 Preview Features

### Deducing this (Early Adoption)

CRTP alternative for performance:

```cpp
// Traditional CRTP approach
template<typename Derived>
class EntityCRTP {
public:
    void update(float delta_time) {
        static_cast<Derived*>(this)->updateImpl(delta_time);
    }

protected:
    ~EntityCRTP() = default;
};

// C++23 deducing this approach (where supported)
class ModernEntity {
public:
    template<typename Self>
    void update(this Self&& self, float delta_time) {
        self.updateImpl(delta_time);
    }

    virtual ~ModernEntity() = default;
};

class Planet : public ModernEntity {
public:
    void updateImpl(float delta_time) {
        // Planet-specific update logic
        regenerateResources(delta_time);
        processPopulation(delta_time);
    }
};
```

### std::expected for Error Handling

Enhanced error handling (polyfill implementation):

```cpp
// Polyfill for std::expected (until C++23 widely available)
template<typename T, typename E>
class Expected {
private:
    std::variant<T, E> storage_;

public:
    Expected(const T& value) : storage_(value) {}
    Expected(T&& value) : storage_(std::move(value)) {}
    Expected(const E& error) : storage_(error) {}
    Expected(E&& error) : storage_(std::move(error)) {}

    bool has_value() const noexcept {
        return std::holds_alternative<T>(storage_);
    }

    const T& value() const& {
        if (!has_value()) {
            throw std::runtime_error("Expected contains error");
        }
        return std::get<T>(storage_);
    }

    T& value() & {
        if (!has_value()) {
            throw std::runtime_error("Expected contains error");
        }
        return std::get<T>(storage_);
    }

    T&& value() && {
        if (!has_value()) {
            throw std::runtime_error("Expected contains error");
        }
        return std::get<T>(std::move(storage_));
    }

    const E& error() const& {
        if (has_value()) {
            throw std::runtime_error("Expected contains value, not error");
        }
        return std::get<E>(storage_);
    }

    template<typename F>
    auto and_then(F&& func) -> Expected<std::invoke_result_t<F, T>, E> {
        if (has_value()) {
            return func(value());
        } else {
            return error();
        }
    }

    template<typename F>
    auto transform(F&& func) -> Expected<std::invoke_result_t<F, T>, E> {
        if (has_value()) {
            return func(value());
        } else {
            return error();
        }
    }
};

// Usage in navigation system
class NavigationSystem {
public:
    Expected<Route, NavigationError> calculateRoute(
        const Coordinates& from, const Coordinates& to) {

        if (!isValidCoordinate(from)) {
            return NavigationError{"Invalid source coordinates"};
        }

        if (!isValidCoordinate(to)) {
            return NavigationError{"Invalid destination coordinates"};
        }

        return findOptimalPath(from, to)
            .and_then([](const Route& route) -> Expected<Route, NavigationError> {
                if (route.isValid()) {
                    return route;
                } else {
                    return NavigationError{"Generated route is invalid"};
                }
            });
    }

    // Chained operations with expected
    Expected<double, NavigationError> calculateMissionCost(
        const Coordinates& from, const Coordinates& to) {

        return calculateRoute(from, to)
            .transform([](const Route& route) { return route.getFuelCost(); })
            .and_then([](double fuel_cost) -> Expected<double, NavigationError> {
                if (fuel_cost > 0) {
                    return fuel_cost;
                } else {
                    return NavigationError{"Invalid fuel cost calculation"};
                }
            });
    }
};
```

## Performance Optimization Techniques

### Template Metaprogramming for Zero-Cost Abstractions

Compile-time computation eliminates runtime overhead:

```cpp
// Compile-time type list processing
template<typename... Types>
struct TypeList {};

template<typename List>
struct TypeListSize;

template<typename... Types>
struct TypeListSize<TypeList<Types...>> {
    static constexpr size_t value = sizeof...(Types);
};

// Compile-time algorithm selection
template<size_t N>
struct SortAlgorithmSelector {
    template<typename Iterator>
    static void sort(Iterator begin, Iterator end) {
        if constexpr (N <= 10) {
            // Use insertion sort for small arrays
            std::sort(begin, end); // Compiler will likely inline as insertion sort
        } else if constexpr (N <= 1000) {
            // Use quicksort for medium arrays
            std::sort(begin, end);
        } else {
            // Use parallel sort for large arrays
            std::sort(std::execution::par_unseq, begin, end);
        }
    }
};

template<typename Container>
void optimizedSort(Container& container) {
    constexpr size_t size = std::tuple_size_v<Container>;
    SortAlgorithmSelector<size>::sort(container.begin(), container.end());
}
```

### Compile-Time Configuration

Template-based configuration system:

```cpp
// Compile-time feature flags
template<bool EnableDebugLogging, bool EnableProfiling, bool EnableConcurrency>
struct SystemConfiguration {
    static constexpr bool debug_logging = EnableDebugLogging;
    static constexpr bool profiling = EnableProfiling;
    static constexpr bool concurrency = EnableConcurrency;
};

using ProductionConfig = SystemConfiguration<false, false, true>;
using DebugConfig = SystemConfiguration<true, true, false>;
using TestConfig = SystemConfiguration<true, false, false>;

template<typename Config>
class GameEngine {
public:
    void log(const std::string& message) {
        if constexpr (Config::debug_logging) {
            std::cout << "[DEBUG] " << message << std::endl;
        }
        // No runtime overhead when logging disabled
    }

    void profileFunction(const char* name, auto&& func) {
        if constexpr (Config::profiling) {
            auto start = std::chrono::high_resolution_clock::now();
            func();
            auto end = std::chrono::high_resolution_clock::now();

            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            std::cout << "[PROFILE] " << name << ": " << duration.count() << "μs" << std::endl;
        } else {
            func();
        }
    }

    template<typename Task>
    void executeTask(Task&& task) {
        if constexpr (Config::concurrency) {
            thread_pool_.enqueue(std::forward<Task>(task));
        } else {
            task();
        }
    }
};
```

## Memory Management Innovations

### Custom Allocators with Modern C++

PMRA (Polymorphic Memory Resource Allocator) usage:

```cpp
#include <memory_resource>

class GameMemoryManager {
private:
    // Different memory pools for different allocation patterns
    std::pmr::monotonic_buffer_resource mission_buffer_{1024 * 1024}; // 1MB
    std::pmr::pool_options entity_pool_options_{
        .max_blocks_per_chunk = 32,
        .largest_required_pool_block = 1024
    };
    std::pmr::synchronized_pool_resource entity_pool_{entity_pool_options_};

public:
    // Get allocator for short-lived mission objects
    std::pmr::memory_resource* getMissionAllocator() {
        return &mission_buffer_;
    }

    // Get allocator for entity objects
    std::pmr::memory_resource* getEntityAllocator() {
        return &entity_pool_;
    }

    void resetMissionMemory() {
        mission_buffer_.release(); // Fast reset of all mission memory
    }
};

// PMR-aware containers
class MissionManager {
private:
    GameMemoryManager& memory_manager_;
    std::pmr::vector<std::pmr::unique_ptr<Mission>> active_missions_;

public:
    explicit MissionManager(GameMemoryManager& mm)
        : memory_manager_(mm)
        , active_missions_(mm.getMissionAllocator()) {}

    template<typename MissionType, typename... Args>
    void createMission(Args&&... args) {
        auto mission_memory = memory_manager_.getMissionAllocator();

        auto mission = std::pmr::make_unique<MissionType>(
            mission_memory,
            std::forward<Args>(args)...
        );

        active_missions_.push_back(std::move(mission));
    }

    void clearCompletedMissions() {
        // Remove completed missions
        auto new_end = std::remove_if(active_missions_.begin(), active_missions_.end(),
            [](const auto& mission) { return mission->isCompleted(); });
        active_missions_.erase(new_end, active_missions_.end());

        // Optionally reset mission buffer if all missions complete
        if (active_missions_.empty()) {
            memory_manager_.resetMissionMemory();
        }
    }
};
```

## Debugging and Development Tools

### Compile-Time Debugging

Template debugging utilities:

```cpp
// Compile-time type information printing
template<typename T>
void print_type() {
    // Force compilation error to see type in error message
    static_assert(std::is_void_v<T> && !std::is_void_v<T>, "Type is:");
}

// Better approach: constexpr type names
template<typename T>
constexpr std::string_view type_name() {
    std::string_view name = __PRETTY_FUNCTION__;

#ifdef __clang__
    name.remove_prefix(34); // Remove "std::string_view type_name() [T = "
    name.remove_suffix(1);  // Remove "]"
#elif defined(__GNUC__)
    name.remove_prefix(46); // Remove "constexpr std::string_view type_name() [with T = "
    name.remove_suffix(1);  // Remove "]"
#elif defined(_MSC_VER)
    name.remove_prefix(38); // Remove "class std::basic_string_view<char,struct "
    name.remove_suffix(7);  // Remove " > __cdecl type_name<"
#endif

    return name;
}

// Usage in templates for debugging
template<typename T>
void debug_template_function(T&& value) {
    if constexpr (std::is_same_v<std::decay_t<T>, int>) {
        std::cout << "Processing int: " << value << std::endl;
    } else {
        std::cout << "Processing " << type_name<std::decay_t<T>>()
                  << ": " << value << std::endl;
    }
}
```

### Runtime Performance Monitoring

RAII-based profiling:

```cpp
class ScopedProfiler {
private:
    const char* name_;
    std::chrono::high_resolution_clock::time_point start_time_;

public:
    explicit ScopedProfiler(const char* name)
        : name_(name)
        , start_time_(std::chrono::high_resolution_clock::now()) {}

    ~ScopedProfiler() {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            end_time - start_time_);

        std::cout << "[PROFILE] " << name_ << ": "
                  << duration.count() << "μs" << std::endl;
    }
};

#ifdef ENABLE_PROFILING
    #define PROFILE_SCOPE(name) ScopedProfiler _prof(name)
    #define PROFILE_FUNCTION() PROFILE_SCOPE(__FUNCTION__)
#else
    #define PROFILE_SCOPE(name) do {} while(0)
    #define PROFILE_FUNCTION() do {} while(0)
#endif

// Usage
void expensiveFunction() {
    PROFILE_FUNCTION();

    {
        PROFILE_SCOPE("expensive_calculation");
        // Some expensive calculation
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    {
        PROFILE_SCOPE("database_query");
        // Database operation
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}
```

## Modern C++ Best Practices Summary

### Code Organization

1. **Header-Only Libraries**: For template-heavy code
2. **Module System**: Where compiler support exists
3. **Namespace Organization**: Prevent name collisions
4. **Forward Declarations**: Minimize compilation dependencies

### Type Safety

1. **Strong Typing**: Custom types over primitives
2. **constexpr**: Compile-time computation where possible
3. **Concepts**: Clear template constraints
4. **std::optional/std::expected**: Explicit error handling

### Performance

1. **Zero-Cost Abstractions**: Templates over runtime polymorphism where appropriate
2. **Move Semantics**: Minimize unnecessary copies
3. **RAII**: Automatic resource management
4. **Custom Allocators**: Optimize memory allocation patterns

### Maintainability

1. **Clear Interfaces**: Well-defined contracts
2. **Documentation**: Comprehensive API documentation
3. **Testing**: Unit tests for all modern C++ features
4. **Static Analysis**: Automated code quality checks

## Conclusion

CppVerseHub's modern C++ implementation demonstrates practical usage of advanced language features while maintaining:

- **Performance**: Zero-overhead abstractions and compile-time optimization
- **Safety**: Strong typing and explicit error handling
- **Maintainability**: Clear code organization and comprehensive documentation
- **Educational Value**: Real-world examples of modern C++ best practices

The codebase serves as both a working application and a comprehensive reference for modern C++ development techniques, showing how advanced language features solve real engineering problems while maintaining code quality and performance standards.

## Feature Adoption Timeline

| Feature             | Standard | Support Status   | Usage in CppVerseHub              |
| ------------------- | -------- | ---------------- | --------------------------------- |
| Structured Bindings | C++17    | Full             | Resource management, iteration    |
| std::optional       | C++17    | Full             | Error handling, optional values   |
| constexpr           | C++17    | Full             | Math calculations, configurations |
| Concepts            | C++20    | Modern compilers | Template constraints              |
| Ranges              | C++20    | Modern compilers | Data processing pipelines         |
| Coroutines          | C++20    | Partial          | Async mission execution           |
| Modules             | C++20    | Limited          | Where supported                   |
| std::expected       | C++23    | Polyfill         | Enhanced error handling           |

This progressive adoption ensures the codebase remains both cutting-edge and practically usable across different compiler environments.
