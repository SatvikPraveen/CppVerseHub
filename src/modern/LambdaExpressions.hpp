/**
 * @file LambdaExpressions.hpp
 * @brief Lambda expressions from C++11 to C++20: captures, generic/template lambdas, closures as
 *        state, higher-order utilities and asynchronous work.
 *
 * Lambdas are anonymous function objects synthesised by the compiler. This module demonstrates:
 *  - capture modes (`[=]`, `[&]`, mixed, init-captures that *move* resources in, `[*this]`);
 *  - `mutable` closures that carry private state (counters, running averages);
 *  - generic (`auto`) and template-parameter (`[]<typename T>`) lambdas, `constexpr` lambdas;
 *  - higher-order building blocks: `compose`, `pipeline`, `curry`, `Overloaded` (visitor sets),
 *    `Fix` (a Y-combinator for recursive lambdas without `std::function`) and `Memoized`;
 *  - type erasure with `std::function` for heterogeneous callbacks (`EventBus`);
 *  - lambdas as tasks for `std::async`.
 *
 * Everything except the `demonstrate*` functions is silent and returns values, so it can be tested.
 */
#pragma once

#include <concepts>
#include <cstddef>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <numeric>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace CppVerseHub::Modern::LambdaExpressions {

// ===== DATA =====

/// @brief A ship used by the lambda examples.
struct SpaceShip {
    int id = 0;             ///< Identifier.
    std::string name;       ///< Display name.
    std::string classType;  ///< Ship class.
    double fuelLevel = 0.0; ///< Fuel percentage 0..100.
    int crewSize = 0;       ///< Crew complement.
    bool isActive = true;   ///< Operational flag.
};

/// @brief A planet used by the lambda examples.
struct Planet {
    int id = 0;               ///< Identifier.
    std::string name;         ///< Display name.
    double distanceAu = 0.0;  ///< Distance from its star in AU.
    long long population = 0; ///< Inhabitants.
    bool habitable = false;   ///< Supports life.
};

/// @brief Deterministic sample fleet of five ships. @return The fleet.
[[nodiscard]] std::vector<SpaceShip> sampleFleet();

/// @brief Deterministic sample of seven planets. @return The planets.
[[nodiscard]] std::vector<Planet> samplePlanets();

// ===== GENERIC HIGHER-ORDER UTILITIES =====

/// @brief Mathematical composition: `compose(f, g, h)(x) == f(g(h(x)))`.
/// @param f Outermost function. @param rest Remaining functions (applied right to left).
/// @return A callable object.
template <typename F, typename... Rest>
[[nodiscard]] constexpr auto compose(F f, Rest... rest) {
    if constexpr (sizeof...(Rest) == 0) {
        return f;
    } else {
        return [f = std::move(f), inner = compose(std::move(rest)...)](auto&&... args) {
            return f(inner(std::forward<decltype(args)>(args)...));
        };
    }
}

/// @brief Left-to-right pipeline: `pipeline(f, g, h)(x) == h(g(f(x)))`.
/// @param f First stage. @param rest Remaining stages.
/// @return A callable object.
template <typename F, typename... Rest>
[[nodiscard]] constexpr auto pipeline(F f, Rest... rest) {
    if constexpr (sizeof...(Rest) == 0) {
        return f;
    } else {
        return [f = std::move(f), next = pipeline(std::move(rest)...)](auto&&... args) {
            return next(f(std::forward<decltype(args)>(args)...));
        };
    }
}

/// @brief Automatic currying: arguments may be supplied in any grouping until `f` becomes invocable,
///        e.g. `curry(add3)(1)(2)(3) == curry(add3)(1, 2)(3) == add3(1, 2, 3)`.
/// @param f Callable to curry. @param bound Arguments already bound (stored by value).
/// @return A callable that either invokes `f` or returns a further-curried callable.
template <typename F, typename... Bound>
[[nodiscard]] constexpr auto curry(F f, Bound... bound) {
    return [f = std::move(f), ... bound = std::move(bound)](auto&&... args) {
        if constexpr (std::invocable<const F&, const Bound&..., decltype(args)...>) {
            return std::invoke(f, bound..., std::forward<decltype(args)>(args)...);
        } else {
            return curry(f, bound..., std::decay_t<decltype(args)>(std::forward<decltype(args)>(args))...);
        }
    };
}

/// @brief Merges several lambdas into one overload set (the classic `std::visit` helper).
template <typename... Ts>
struct Overloaded : Ts... {
    using Ts::operator()...;
};

/// @brief Deduction guide; C++20 aggregate CTAD makes it redundant, but older Apple Clang still needs it.
template <typename... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

/// @brief Fixed-point combinator: lets a lambda call itself through its first parameter without the
///        overhead (and dangling-reference hazards) of a self-capturing `std::function`.
/// @tparam F Lambda of the form `[](const auto& self, Args...) -> R`.
template <typename F>
class Fix {
public:
    /// @brief Wraps the open-recursive function. @param f The function.
    explicit constexpr Fix(F f) noexcept(std::is_nothrow_move_constructible_v<F>) : f_(std::move(f)) {}

    /// @brief Invokes the function, passing itself as the first argument.
    /// @param args Arguments forwarded after `self`. @return The function's result.
    template <typename... Args>
    constexpr decltype(auto) operator()(Args&&... args) const {
        return f_(*this, std::forward<Args>(args)...);
    }

private:
    F f_;
};

/// @brief Caches results of a pure unary function (keys must be totally ordered).
/// @tparam Arg Argument type. @tparam Result Result type.
template <typename Arg, typename Result>
    requires std::totally_ordered<Arg>
class Memoized {
public:
    /// @brief Wraps a function. @param fn The function to memoise.
    template <typename F>
        requires std::invocable<F&, const Arg&> && (!std::same_as<std::remove_cvref_t<F>, Memoized>)
    explicit Memoized(F&& fn) : fn_(std::forward<F>(fn)) {}

    /// @brief Returns the cached value or computes and caches it.
    /// @param arg Argument. @return `fn(arg)`.
    const Result& operator()(const Arg& arg) {
        if (auto it = cache_.find(arg); it != cache_.end()) {
            ++hits_;
            return it->second;
        }
        ++misses_;
        return cache_.emplace(arg, fn_(arg)).first->second;
    }

    /// @brief Number of cache hits. @return Hits.
    [[nodiscard]] std::size_t hits() const noexcept { return hits_; }
    /// @brief Number of computations performed. @return Misses.
    [[nodiscard]] std::size_t misses() const noexcept { return misses_; }
    /// @brief Number of cached entries. @return Cache size.
    [[nodiscard]] std::size_t size() const noexcept { return cache_.size(); }

private:
    std::function<Result(const Arg&)> fn_;
    std::map<Arg, Result> cache_;
    std::size_t hits_ = 0;
    std::size_t misses_ = 0;
};

// ===== CLOSURE FACTORIES =====

/// @brief A counter closure: each call returns the next integer (state lives in a `mutable` capture).
/// @param start First value returned. @return A stateful callable `() -> int`.
[[nodiscard]] constexpr auto makeCounter(int start = 0) noexcept {
    return [next = start]() mutable noexcept { return next++; };
}

/// @brief A closure returning the running mean of all values passed so far.
/// @return A stateful callable `(double) -> double`.
[[nodiscard]] constexpr auto makeRunningAverage() noexcept {
    return [sum = 0.0, count = 0LL](double value) mutable noexcept {
        sum += value;
        ++count;
        return sum / static_cast<double>(count);
    };
}

/// @brief A closure testing membership in the closed interval [lo, hi].
/// @param lo Lower bound. @param hi Upper bound. @return Predicate `(T) -> bool`.
template <typename T>
[[nodiscard]] constexpr auto makeRangeValidator(T lo, T hi) noexcept {
    return [lo, hi](const T& value) noexcept { return !(value < lo) && !(hi < value); };
}

/// @brief Moves a resource into a closure with an init-capture (C++14) — the closure is then move-only.
/// @param payload Owned resource. @return Callable returning the payload's length (0 if moved-from).
[[nodiscard]] inline auto makeOwningReporter(std::unique_ptr<std::string> payload) {
    return [owned = std::move(payload)]() noexcept { return owned ? owned->size() : std::size_t{0}; };
}

/// @brief Copies elements satisfying `pred`. @param items Input. @param pred Predicate. @return Matches.
template <typename T, std::predicate<const T&> Pred>
[[nodiscard]] std::vector<T> filterBy(const std::vector<T>& items, Pred pred) {
    std::vector<T> out;
    for (const auto& item : items) {
        if (pred(item)) {
            out.push_back(item);
        }
    }
    return out;
}

/// @brief Maps every element through `f`. @param items Input. @param f Mapping. @return Mapped values.
template <typename T, std::invocable<const T&> F>
[[nodiscard]] auto mapTo(const std::vector<T>& items, F f) {
    std::vector<std::decay_t<std::invoke_result_t<F&, const T&>>> out;
    out.reserve(items.size());
    for (const auto& item : items) {
        out.push_back(f(item));
    }
    return out;
}

/// @brief Size in bits of any object; a C++20 template lambda with an explicit type parameter.
inline constexpr auto sizeInBits = []<typename T>(const T& /*unused*/) constexpr noexcept {
    return sizeof(T) * 8U;
};

/// @brief Factorial via `Fix`, evaluable at compile time.
inline constexpr Fix factorialFix{[](const auto& self, unsigned long long n) -> unsigned long long {
    return n <= 1 ? 1ULL : n * self(n - 1);
}};

static_assert(compose([](int x) { return x + 1; }, [](int x) { return x * 2; })(5) == 11);
static_assert(pipeline([](int x) { return x + 1; }, [](int x) { return x * 2; })(5) == 12);
static_assert(curry([](int a, int b, int c) { return a * 100 + b * 10 + c; })(1)(2)(3) == 123);
#if !defined(_MSC_VER) || defined(__clang__)
// MSVC cannot constant-evaluate a lambda that recurses through a deduced `self` parameter (C3615);
// the runtime tests still cover factorialFix there.
static_assert(factorialFix(10) == 3628800ULL);
#endif
static_assert(sizeInBits(0) == sizeof(int) * 8U);
static_assert([] {
    auto counter = makeCounter(5);
    (void)counter();
    return counter();
}() == 6);

// ===== CAPTURING *this =====

/// @brief Shows the difference between capturing `this` (reference semantics) and `*this` (a copy).
class Beacon {
public:
    /// @brief Creates a beacon. @param label Initial label.
    explicit Beacon(std::string label) : label_(std::move(label)) {}

    /// @brief Changes the label. @param label New label.
    void relabel(std::string label) { label_ = std::move(label); }

    /// @brief Closure that observes the *live* object (must not outlive it).
    /// @return Callable returning the current label.
    [[nodiscard]] auto liveReporter() const {
        return [this] { return label_; };
    }

    /// @brief Closure holding a snapshot copy of the object (C++17 `[*this]`); safe to outlive it.
    /// @return Callable returning the label at the time of the call to `snapshotReporter`.
    [[nodiscard]] auto snapshotReporter() const {
        return [*this] { return label_; };
    }

private:
    std::string label_;
};

// ===== TYPE-ERASED CALLBACKS =====

/// @brief A synchronous publish/subscribe bus storing heterogeneous lambdas in `std::function`.
///        Not thread-safe; handlers must not subscribe/unsubscribe re-entrantly while emitting.
class EventBus {
public:
    using Handler = std::function<void(const std::string& payload)>; ///< Callback signature.
    using SubscriptionId = std::size_t;                              ///< Handle for unsubscribing.

    /// @brief Registers a handler for an event name.
    /// @param event Event name. @param handler Callback. @return Subscription id (never 0).
    SubscriptionId subscribe(const std::string& event, Handler handler);

    /// @brief Removes a subscription. @param id Id returned by `subscribe`. @return True if removed.
    bool unsubscribe(SubscriptionId id);

    /// @brief Invokes every handler registered for `event`, in subscription order.
    /// @param event Event name. @param payload Data passed to handlers. @return Handlers invoked.
    std::size_t emit(const std::string& event, const std::string& payload) const;

    /// @brief Number of handlers for an event. @param event Event name. @return Count.
    [[nodiscard]] std::size_t handlerCount(const std::string& event) const;

private:
    struct Entry {
        SubscriptionId id;
        Handler handler;
    };
    std::map<std::string, std::vector<Entry>, std::less<>> handlers_;
    SubscriptionId nextId_ = 1;
};

// ===== ASYNC =====

/// @brief Sums a vector by splitting it into `chunks` slices, each summed by a lambda run via
///        `std::async(std::launch::async, ...)`.
/// @param data Input values. @param chunks Number of tasks (clamped to [1, data.size()]).
/// @return Sum of all elements.
[[nodiscard]] long long parallelSum(const std::vector<int>& data, std::size_t chunks);

/// @brief Operations over a fleet expressed as lambdas over STL algorithms.
struct FleetSummary {
    std::vector<std::string> operationalNames; ///< Active ships with fuel > 50%, by fuel descending.
    int activeCrew = 0;                        ///< Crew on active ships.
    double meanFuel = 0.0;                     ///< Mean fuel across all ships.
};

/// @brief Computes a `FleetSummary` with `std::copy_if`, `std::sort`, `std::accumulate` and lambdas.
/// @param fleet Input ships. @return The summary.
[[nodiscard]] FleetSummary summarizeFleet(const std::vector<SpaceShip>& fleet);

// ===== SHOWCASES =====

/// @brief Syntax, parameters, return types and basic captures. @param out Destination stream.
void demonstrateBasicLambdas(std::ostream& out = std::cout);
/// @brief `[=]`, `[&]`, mixed, init- and `*this` captures. @param out Destination stream.
void demonstrateCaptureModes(std::ostream& out = std::cout);
/// @brief Lambdas as STL algorithm arguments. @param out Destination stream.
void demonstrateStlLambdas(std::ostream& out = std::cout);
/// @brief Generic, template and constexpr lambdas; lambdas in unevaluated contexts. @param out Stream.
void demonstrateGenericLambdas(std::ostream& out = std::cout);
/// @brief Mutable closures and closure factories. @param out Destination stream.
void demonstrateStatefulLambdas(std::ostream& out = std::cout);
/// @brief compose / pipeline / curry / Overloaded / Fix / Memoized. @param out Destination stream.
void demonstrateFunctionalUtilities(std::ostream& out = std::cout);
/// @brief Type-erased callbacks in an `EventBus`. @param out Destination stream.
void demonstrateEventSystem(std::ostream& out = std::cout);
/// @brief Lambdas as `std::async` tasks. @param out Destination stream.
void demonstrateAsyncLambdas(std::ostream& out = std::cout);
/// @brief Runs every lambda showcase. @param out Destination stream.
void demonstrateAllLambdas(std::ostream& out = std::cout);

} // namespace CppVerseHub::Modern::LambdaExpressions
