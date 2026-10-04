/**
 * @file Functors.hpp
 * @brief Callable-object showcase: hand-written function objects, lambdas (captures, mutable,
 *        generic, constexpr), the <functional> library (std::plus/std::less<>/std::not_fn/
 *        std::mem_fn/std::bind/std::bind_front/std::function/std::invoke) and higher-order helpers.
 *
 * A recurring theme is *where state lives*: algorithms take callables by value, so stateful
 * functors are copied. StarshipNameGenerator and SharedCallCounter show the two standard answers
 * (pass std::ref, or keep the state behind a shared handle).
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <numeric>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace CppVerseHub::STL {

/**
 * @brief A starship record used by the functor demonstrations.
 */
struct Starship {
    std::string name;            ///< Ship name.
    std::string ship_class;      ///< Ship class.
    double shield_strength{0.0}; ///< Shield percentage 0..100.
    double warp_speed{0.0};      ///< Maximum warp factor.
    int crew{0};                 ///< Crew complement.
    double firepower{0.0};       ///< Abstract firepower rating.

    /**
     * @brief Combat effectiveness combining firepower, shields and warp.
     * @return firepower * shield/100 * (1 + warp/10).
     */
    [[nodiscard]] constexpr double combatEffectiveness() const noexcept {
        return firepower * (shield_strength / 100.0) * (1.0 + warp_speed / 10.0);
    }

    /// @brief Member-wise equality.
    friend bool operator==(const Starship&, const Starship&) = default;
};

/**
 * @brief Deterministic sample starships (five ships, three combat ready by default thresholds).
 * @return Ships in a fixed order.
 */
[[nodiscard]] std::vector<Starship> sampleStarships();

/**
 * @brief Configurable predicate functor: is a ship ready for combat?
 */
class IsCombatReady {
public:
    /**
     * @brief Construct with thresholds.
     * @param min_shields Minimum shield strength.
     * @param min_warp Minimum warp speed.
     */
    explicit constexpr IsCombatReady(double min_shields = 50.0, double min_warp = 5.0) noexcept
        : min_shields_(min_shields), min_warp_(min_warp) {}

    /**
     * @brief Evaluate the predicate.
     * @param ship Ship to test.
     * @return true if both thresholds are met.
     */
    [[nodiscard]] constexpr bool operator()(const Starship& ship) const noexcept {
        return ship.shield_strength >= min_shields_ && ship.warp_speed >= min_warp_;
    }

    /// @brief Minimum shields. @return Threshold.
    [[nodiscard]] constexpr double minShields() const noexcept { return min_shields_; }
    /// @brief Minimum warp. @return Threshold.
    [[nodiscard]] constexpr double minWarp() const noexcept { return min_warp_; }

private:
    double min_shields_;
    double min_warp_;
};

/**
 * @brief Strict-weak-ordering comparator: higher combat effectiveness first, ties by name.
 */
struct ByCombatEffectivenessDesc {
    /**
     * @brief Compare two ships.
     * @param lhs First ship.
     * @param rhs Second ship.
     * @return true if lhs should be ordered before rhs.
     */
    [[nodiscard]] bool operator()(const Starship& lhs, const Starship& rhs) const noexcept {
        const double l = lhs.combatEffectiveness();
        const double r = rhs.combatEffectiveness();
        if (l != r) {
            return l > r;
        }
        return lhs.name < rhs.name;
    }
};

/**
 * @brief Aggregate fleet statistics produced by FleetStatsAccumulator.
 */
struct FleetStats {
    std::size_t ship_count{0};   ///< Number of ships folded in.
    long long total_crew{0};     ///< Sum of crew.
    double total_firepower{0.0}; ///< Sum of firepower.
    double max_warp{0.0};        ///< Highest warp speed.

    /**
     * @brief Mean crew per ship.
     * @return total_crew / ship_count, or 0 when empty.
     */
    [[nodiscard]] constexpr double averageCrew() const noexcept {
        return ship_count == 0 ? 0.0 : static_cast<double>(total_crew) / static_cast<double>(ship_count);
    }
};

/**
 * @brief Binary fold operation for std::accumulate over starships.
 */
struct FleetStatsAccumulator {
    /**
     * @brief Fold one ship into the running statistics.
     * @param stats Accumulated statistics so far.
     * @param ship Next ship.
     * @return Updated statistics.
     */
    [[nodiscard]] FleetStats operator()(FleetStats stats, const Starship& ship) const noexcept;
};

/**
 * @brief Stateful generator functor producing "<prefix>-001", "<prefix>-002", ...
 *
 * std::generate_n takes its generator by value, so pass std::ref(generator) to keep the count.
 */
class StarshipNameGenerator {
public:
    /**
     * @brief Construct a generator.
     * @param prefix Name prefix.
     * @param start First serial number.
     */
    explicit StarshipNameGenerator(std::string prefix, int start = 1)
        : prefix_(std::move(prefix)), next_(start) {}

    /**
     * @brief Produce the next name.
     * @return Name with a zero-padded three digit serial.
     */
    [[nodiscard]] std::string operator()();

    /// @brief How many names this object produced. @return Count.
    [[nodiscard]] int generated() const noexcept { return generated_; }

private:
    std::string prefix_;
    int next_;
    int generated_{0};
};

/**
 * @brief Predicate wrapper that counts invocations in a counter shared between all copies.
 * @tparam Pred Wrapped predicate.
 */
template <typename Pred>
class SharedCallCounter {
public:
    /**
     * @brief Wrap a predicate.
     * @param pred Predicate to wrap.
     */
    explicit SharedCallCounter(Pred pred)
        : pred_(std::move(pred)), calls_(std::make_shared<std::size_t>(0)) {}

    /**
     * @brief Invoke the predicate and bump the shared counter.
     * @tparam Args Argument types.
     * @param args Arguments forwarded to the predicate.
     * @return The predicate's result.
     */
    template <typename... Args>
    decltype(auto) operator()(Args&&... args) const {
        ++*calls_;
        return std::invoke(pred_, std::forward<Args>(args)...);
    }

    /// @brief Total calls across every copy. @return Call count.
    [[nodiscard]] std::size_t calls() const noexcept { return *calls_; }

private:
    Pred pred_;
    std::shared_ptr<std::size_t> calls_;
};

// ------------------------------------------------------------------------------------ lambdas

/**
 * @brief Closure factory: capture by value.
 * @param factor Multiplier captured into the closure.
 * @return constexpr callable x -> x * factor.
 */
[[nodiscard]] constexpr auto makeMultiplier(int factor) noexcept {
    return [factor](int x) constexpr noexcept { return x * factor; };
}

/**
 * @brief Closure factory: init-capture + mutable state.
 * @param start First value returned.
 * @return Callable returning start, start + 1, ... on successive calls.
 */
[[nodiscard]] constexpr auto makeCounter(int start = 0) noexcept {
    return [n = start]() mutable noexcept { return n++; };
}

/**
 * @brief Right-to-left function composition: compose(f, g, h)(x) == f(g(h(x))).
 * @tparam F Outermost callable.
 * @tparam Fs Remaining callables.
 * @param f Outermost callable.
 * @param fs Remaining callables.
 * @return The composed callable.
 */
template <typename F, typename... Fs>
[[nodiscard]] constexpr auto compose(F f, Fs... fs) {
    if constexpr (sizeof...(Fs) == 0) {
        return f;
    } else {
        return [f = std::move(f), inner = compose(std::move(fs)...)](auto&&... args) -> decltype(auto) {
            return std::invoke(f, std::invoke(inner, std::forward<decltype(args)>(args)...));
        };
    }
}

/**
 * @brief Count elements satisfying a predicate using a generic lambda over any range.
 * @tparam Range Iterable range.
 * @tparam Pred Unary predicate.
 * @param range Elements to inspect.
 * @param pred Predicate.
 * @return Number of matching elements.
 */
template <typename Range, typename Pred>
[[nodiscard]] std::size_t countWhere(const Range& range, Pred pred) {
    std::size_t count = 0;
    for (const auto& element : range) {
        if (std::invoke(pred, element)) {
            ++count;
        }
    }
    return count;
}

/**
 * @brief Cache results of an expensive unary function in an unordered_map.
 * @tparam Result Result type.
 * @tparam Arg Hashable argument type.
 */
template <typename Result, typename Arg>
class Memoized {
public:
    /**
     * @brief Wrap a function.
     * @param fn Pure function to memoise.
     */
    explicit Memoized(std::function<Result(const Arg&)> fn) : fn_(std::move(fn)) {}

    /**
     * @brief Return the cached result, computing it on first use.
     * @param arg Argument.
     * @return Reference to the cached result (valid until the cache is cleared or destroyed).
     */
    const Result& operator()(const Arg& arg) {
        if (const auto it = cache_.find(arg); it != cache_.end()) {
            ++hits_;
            return it->second;
        }
        ++misses_;
        return cache_.emplace(arg, fn_(arg)).first->second;
    }

    /// @brief Number of cache hits. @return Hit count.
    [[nodiscard]] std::size_t hits() const noexcept { return hits_; }
    /// @brief Number of computations. @return Miss count.
    [[nodiscard]] std::size_t misses() const noexcept { return misses_; }
    /// @brief Drop every cached value.
    void clear() noexcept { cache_.clear(); }

private:
    std::function<Result(const Arg&)> fn_;
    std::unordered_map<Arg, Result> cache_;
    std::size_t hits_{0};
    std::size_t misses_{0};
};

/**
 * @brief n-th Fibonacci number via a recursive generic lambda with an explicit memo table.
 * @param n Index (fib(0) = 0, fib(1) = 1); must be <= 93 to fit in 64 bits.
 * @return fib(n).
 * @throws std::out_of_range if n > 93.
 */
[[nodiscard]] std::uint64_t memoizedFibonacci(unsigned n);

// ---------------------------------------------------------------------- <functional> utilities

/**
 * @brief Euclidean distance in the plane.
 * @param x1 First point x.
 * @param y1 First point y.
 * @param x2 Second point x.
 * @param y2 Second point y.
 * @return Distance.
 */
[[nodiscard]] double calculateDistance(double x1, double y1, double x2, double y2) noexcept;

/**
 * @brief Travel time for a ship over a distance (distance / warp^3).
 * @param ship Ship travelling.
 * @param distance Distance in light years.
 * @return Time in arbitrary units; +infinity if the ship has no warp drive.
 */
[[nodiscard]] double calculateWarpTime(const Starship& ship, double distance) noexcept;

/**
 * @brief Travel time of every ship to a fixed distance, built with std::bind and a placeholder.
 * @param ships Ships.
 * @param distance Distance.
 * @return Times in input order.
 */
[[nodiscard]] std::vector<double> warpTimesTo(std::span<const Starship> ships, double distance);

/**
 * @brief Names of ships that are NOT combat ready (std::not_fn + std::mem_fn).
 * @param ships Ships.
 * @param ready Readiness predicate.
 * @return Names in input order.
 */
[[nodiscard]] std::vector<std::string> namesNotReady(std::span<const Starship> ships,
                                                     const IsCombatReady& ready);

/**
 * @brief Publish/subscribe dispatcher built on std::function and a transparent std::map.
 */
class EventDispatcher {
public:
    /// @brief Type-erased handler.
    using Handler = std::function<void(const Starship&)>;
    /// @brief Subscription handle.
    using Token = std::uint64_t;

    /**
     * @brief Register a handler for an event.
     * @param event Event name.
     * @param handler Callable to invoke; must not be empty.
     * @return Token usable with unsubscribe.
     * @throws std::invalid_argument for an empty handler.
     */
    Token subscribe(std::string event, Handler handler);

    /**
     * @brief Remove a handler.
     * @param token Token returned by subscribe.
     * @return true if the handler existed.
     */
    bool unsubscribe(Token token);

    /**
     * @brief Invoke every handler of an event in subscription order.
     * @param event Event name.
     * @param ship Payload.
     * @return Number of handlers invoked.
     */
    std::size_t dispatch(std::string_view event, const Starship& ship) const;

    /**
     * @brief Number of handlers for an event.
     * @param event Event name.
     * @return Handler count.
     */
    [[nodiscard]] std::size_t handlerCount(std::string_view event) const;

private:
    std::map<std::string, std::vector<std::pair<Token, Handler>>, std::less<>> handlers_;
    Token next_token_{1};
};

// ------------------------------------------------------------------------------ demonstrations

/// @brief Narrate predicate/comparator/accumulator/generator functors. @param out Destination stream.
void demonstrateFunctionObjects(std::ostream& out = std::cout);
/// @brief Narrate captures, mutable, generic, constexpr and recursive lambdas. @param out Destination stream.
void demonstrateLambdas(std::ostream& out = std::cout);
/// @brief Narrate std::plus/std::greater/std::less<>/std::not_fn/std::mem_fn. @param out Destination stream.
void demonstrateStandardFunctionObjects(std::ostream& out = std::cout);
/// @brief Narrate std::bind, std::bind_front, std::function and std::invoke. @param out Destination stream.
void demonstrateFunctionBinding(std::ostream& out = std::cout);
/// @brief Run every functor demonstration. @param out Destination stream.
void runFunctorsDemo(std::ostream& out = std::cout);

} // namespace CppVerseHub::STL
