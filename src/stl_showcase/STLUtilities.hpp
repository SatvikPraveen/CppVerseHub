/**
 * @file STLUtilities.hpp
 * @brief Vocabulary-type showcase: std::pair, std::tuple, std::optional, std::variant, std::any,
 *        std::string_view and std::span, applied to spacecraft navigation and command handling.
 *
 * Each vocabulary type answers a distinct modelling question:
 *  - pair/tuple:   "return several values" (with structured bindings and std::tie ordering),
 *  - optional:     "a value may be absent" (with monadic helpers that C++23 later standardised),
 *  - variant:      "exactly one of a closed set of types" (visited with the overloaded idiom),
 *  - any:          "a value of an open, run-time chosen type" (type-safe heterogeneous storage),
 *  - string_view/span: "non-owning views over contiguous data".
 */
#pragma once

#include <any>
#include <cstddef>
#include <functional>
#include <iostream>
#include <map>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <typeinfo>
#include <utility>
#include <variant>
#include <vector>

namespace CppVerseHub::STL {

/**
 * @brief A point in 3-D space.
 */
struct NavigationCoordinate {
    double x{0.0}; ///< X component.
    double y{0.0}; ///< Y component.
    double z{0.0}; ///< Z component.

    /**
     * @brief Euclidean distance to another coordinate.
     * @param other Other coordinate.
     * @return Distance.
     */
    [[nodiscard]] double distanceTo(const NavigationCoordinate& other) const noexcept;

    /// @brief Member-wise equality.
    friend bool operator==(const NavigationCoordinate&, const NavigationCoordinate&) = default;
};

/**
 * @brief Operational status of a vessel.
 */
enum class VesselStatus { Docked, InTransit, Exploring, Combat, Maintenance };

/**
 * @brief Human-readable status name.
 * @param status Status value.
 * @return Name such as "InTransit".
 */
[[nodiscard]] constexpr std::string_view toString(VesselStatus status) noexcept {
    switch (status) {
        case VesselStatus::Docked:
            return "Docked";
        case VesselStatus::InTransit:
            return "InTransit";
        case VesselStatus::Exploring:
            return "Exploring";
        case VesselStatus::Combat:
            return "Combat";
        case VesselStatus::Maintenance:
            return "Maintenance";
    }
    return "Unknown";
}

// --------------------------------------------------------------------------------- pair/tuple

/**
 * @brief Indices of the two closest coordinates (O(n^2) scan).
 * @param points Coordinates.
 * @return (i, j) with i < j, or std::nullopt for fewer than two points.
 */
[[nodiscard]] std::optional<std::pair<std::size_t, std::size_t>> closestPairIndices(
    std::span<const NavigationCoordinate> points);

/**
 * @brief Nearest and farthest distance from the origin (std::minmax over a projection).
 * @param points Coordinates.
 * @return (min, max), or std::nullopt when empty.
 */
[[nodiscard]] std::optional<std::pair<double, double>> distanceRangeFromOrigin(
    std::span<const NavigationCoordinate> points);

/// @brief (centroid, maximum distance from the centroid, number of points).
using CoordinateStats = std::tuple<NavigationCoordinate, double, std::size_t>;

/**
 * @brief Centroid, spread and count of a point cloud, returned as a tuple.
 * @param points Coordinates.
 * @return Statistics, or std::nullopt when empty.
 */
[[nodiscard]] std::optional<CoordinateStats> coordinateStatistics(std::span<const NavigationCoordinate> points);

/**
 * @brief Format any streamable tuple as "(a, b, c)" using std::apply and a fold expression.
 * @tparam Ts Element types (each must be streamable).
 * @param tuple Tuple to format.
 * @return Formatted text.
 */
template <typename... Ts>
[[nodiscard]] std::string formatTuple(const std::tuple<Ts...>& tuple) {
    std::ostringstream os;
    os << '(';
    std::apply(
        [&os](const auto&... elements) {
            [[maybe_unused]] std::size_t index = 0;
            ((os << (index++ == 0 ? "" : ", ") << elements), ...);
        },
        tuple);
    os << ')';
    return os.str();
}

/**
 * @brief Apply a callable to each element of a tuple in order.
 * @tparam Tuple Tuple-like type.
 * @tparam F Callable accepting every element type.
 * @param tuple Tuple to visit.
 * @param f Callable.
 */
template <typename Tuple, typename F>
constexpr void forEachElement(Tuple&& tuple, F&& f) {
    std::apply([&f](auto&&... elements) { (std::invoke(f, std::forward<decltype(elements)>(elements)), ...); },
               std::forward<Tuple>(tuple));
}

/**
 * @brief Build a new tuple by applying @p f to every element.
 * @tparam Ts Element types.
 * @tparam F Callable accepting every element type.
 * @param tuple Source tuple.
 * @param f Transformation.
 * @return std::tuple of the results.
 */
template <typename... Ts, typename F>
[[nodiscard]] constexpr auto transformTuple(const std::tuple<Ts...>& tuple, F f) {
    return std::apply([&f](const auto&... elements) { return std::make_tuple(std::invoke(f, elements)...); },
                      tuple);
}

/**
 * @brief A vessel record used for multi-key sorting.
 */
struct VesselRecord {
    std::string name;                          ///< Vessel name.
    VesselStatus status{VesselStatus::Docked}; ///< Current status.
    int priority{0};                           ///< Higher is more important.

    /// @brief Member-wise equality.
    friend bool operator==(const VesselRecord&, const VesselRecord&) = default;
};

/**
 * @brief Sort by status (enum order), then priority descending, then name, via std::tie.
 * @param records Records to sort in place.
 */
void sortVesselRecords(std::vector<VesselRecord>& records);

// ----------------------------------------------------------------------------------- optional

/**
 * @brief Parse a base-10 int with std::from_chars (no locale, no exceptions).
 * @param text Text; the whole string must be consumed.
 * @return The value, or std::nullopt on malformed or out-of-range input.
 */
[[nodiscard]] std::optional<int> parseInt(std::string_view text) noexcept;

/**
 * @brief Division that refuses a zero divisor.
 * @param numerator Numerator.
 * @param denominator Denominator.
 * @return Quotient, or std::nullopt when denominator == 0.
 */
[[nodiscard]] constexpr std::optional<double> safeDivide(double numerator, double denominator) noexcept {
    if (denominator == 0.0) {
        return std::nullopt;
    }
    return numerator / denominator;
}

/**
 * @brief Monadic bind for std::optional (C++23 has optional::and_then; this works in C++20).
 * @tparam T Contained type.
 * @tparam F Callable T -> std::optional<U>.
 * @param opt Input optional.
 * @param f Continuation.
 * @return f(*opt) if engaged, otherwise an empty optional of f's result type.
 */
template <typename T, typename F>
[[nodiscard]] constexpr auto andThen(const std::optional<T>& opt, F&& f)
    -> std::remove_cvref_t<std::invoke_result_t<F, const T&>> {
    if (opt) {
        return std::invoke(std::forward<F>(f), *opt);
    }
    return std::nullopt;
}

/**
 * @brief Functor map for std::optional (C++23 optional::transform equivalent).
 * @tparam T Contained type.
 * @tparam F Callable T -> U.
 * @param opt Input optional.
 * @param f Transformation.
 * @return std::optional<U> holding f(*opt), or empty.
 */
template <typename T, typename F>
[[nodiscard]] constexpr auto transformOptional(const std::optional<T>& opt, F&& f)
    -> std::optional<std::remove_cvref_t<std::invoke_result_t<F, const T&>>> {
    if (opt) {
        return std::invoke(std::forward<F>(f), *opt);
    }
    return std::nullopt;
}

/**
 * @brief Look up a vessel's status by name.
 * @param registry name -> status registry.
 * @param name Vessel name.
 * @return Status, or std::nullopt if unknown.
 */
[[nodiscard]] std::optional<VesselStatus> findVesselStatus(const std::map<std::string, VesselStatus, std::less<>>& registry,
                                                           std::string_view name);

// ------------------------------------------------------------------------------------ variant

/// @brief Fly to a destination.
struct MoveCommand {
    NavigationCoordinate destination; ///< Target position.
    /// @brief Equality.
    friend bool operator==(const MoveCommand&, const MoveCommand&) = default;
};
/// @brief Engage a target.
struct AttackCommand {
    std::string target; ///< Target name.
    int intensity{1};   ///< 1..10.
    /// @brief Equality.
    friend bool operator==(const AttackCommand&, const AttackCommand&) = default;
};
/// @brief Sensor sweep.
struct ScanCommand {
    double radius{0.0}; ///< Scan radius.
    /// @brief Equality.
    friend bool operator==(const ScanCommand&, const ScanCommand&) = default;
};
/// @brief Dock at a station.
struct DockCommand {
    std::string station; ///< Station name.
    /// @brief Equality.
    friend bool operator==(const DockCommand&, const DockCommand&) = default;
};

/// @brief Closed set of commands a vessel accepts.
using Command = std::variant<MoveCommand, AttackCommand, ScanCommand, DockCommand>;

/**
 * @brief The "overloaded" idiom: inherit call operators from several lambdas for std::visit.
 * @tparam Fs Lambda types.
 */
template <typename... Fs>
struct Overloaded : Fs... {
    using Fs::operator()...;
};

/// @brief Deduction guide for Overloaded (needed for aggregates before C++20 CTAD for aggregates).
template <typename... Fs>
Overloaded(Fs...) -> Overloaded<Fs...>;

/**
 * @brief Describe a command (std::visit with Overloaded).
 * @param command Command to describe.
 * @return Human-readable description.
 */
[[nodiscard]] std::string describeCommand(const Command& command);

/**
 * @brief Status a vessel enters after executing a command.
 * @param command Command.
 * @return Resulting status.
 */
[[nodiscard]] VesselStatus statusAfter(const Command& command) noexcept;

/**
 * @brief Error produced by parseCommand.
 */
struct ParseError {
    std::string message;   ///< What went wrong.
    std::size_t token{0};  ///< 0-based index of the offending token.
    /// @brief Equality.
    friend bool operator==(const ParseError&, const ParseError&) = default;
};

/// @brief Either a parsed Command or a ParseError (a C++20 stand-in for std::expected).
using CommandParseResult = std::variant<Command, ParseError>;

/**
 * @brief Parse "move x y z", "attack <target> <intensity>", "scan <radius>" or "dock <station>".
 * @param text Command text (whitespace separated).
 * @return The command, or a ParseError describing the first problem.
 */
[[nodiscard]] CommandParseResult parseCommand(std::string_view text);

// ---------------------------------------------------------------------------------------- any

/**
 * @brief Heterogeneous key/value store backed by std::any, with type-checked retrieval.
 */
class PropertyBag {
public:
    /**
     * @brief Store (or replace) a value.
     * @tparam T Value type (decayed; must be copy constructible, as std::any requires).
     * @param key Key.
     * @param value Value.
     */
    template <typename T>
    void set(std::string key, T&& value) {
        properties_.insert_or_assign(std::move(key), std::any(std::forward<T>(value)));
    }

    /**
     * @brief Retrieve a value of an exact type.
     * @tparam T Requested type.
     * @param key Key.
     * @return The value, or std::nullopt if missing or stored with a different type.
     */
    template <typename T>
    [[nodiscard]] std::optional<T> get(std::string_view key) const {
        const auto it = properties_.find(key);
        if (it == properties_.end()) {
            return std::nullopt;
        }
        if (const T* value = std::any_cast<T>(&it->second)) {
            return *value;
        }
        return std::nullopt;
    }

    /**
     * @brief Whether a key holds a value of type T.
     * @tparam T Type to test.
     * @param key Key.
     * @return true on an exact type match.
     */
    template <typename T>
    [[nodiscard]] bool holds(std::string_view key) const {
        const auto it = properties_.find(key);
        return it != properties_.end() && it->second.type() == typeid(T);
    }

    /// @brief Whether a key exists. @param key Key. @return true if present.
    [[nodiscard]] bool contains(std::string_view key) const { return properties_.find(key) != properties_.end(); }

    /// @brief Remove a key. @param key Key. @return true if removed.
    bool erase(std::string_view key);

    /// @brief Sorted keys. @return Keys in ascending order.
    [[nodiscard]] std::vector<std::string> keys() const;

    /// @brief Number of properties. @return Count.
    [[nodiscard]] std::size_t size() const noexcept { return properties_.size(); }

private:
    std::map<std::string, std::any, std::less<>> properties_;
};

// --------------------------------------------------------------------------- string_view/span

/**
 * @brief Split into non-owning fields (empty fields are kept: "a,,b" -> {"a", "", "b"}).
 * @param text Text to split; the returned views alias it.
 * @param delimiter Separator character.
 * @return Fields.
 */
[[nodiscard]] std::vector<std::string_view> splitView(std::string_view text, char delimiter);

/**
 * @brief Mean of a contiguous sequence viewed through std::span.
 * @param values Values.
 * @return Mean, or std::nullopt when empty.
 */
[[nodiscard]] std::optional<double> mean(std::span<const double> values) noexcept;

// ------------------------------------------------------------------------------ demonstrations

/// @brief Narrate std::pair. @param out Destination stream.
void demonstratePairs(std::ostream& out = std::cout);
/// @brief Narrate std::tuple, std::tie and std::apply. @param out Destination stream.
void demonstrateTuples(std::ostream& out = std::cout);
/// @brief Narrate std::optional and monadic helpers. @param out Destination stream.
void demonstrateOptional(std::ostream& out = std::cout);
/// @brief Narrate std::variant, std::visit and error-as-value parsing. @param out Destination stream.
void demonstrateVariant(std::ostream& out = std::cout);
/// @brief Narrate std::any and PropertyBag. @param out Destination stream.
void demonstrateAny(std::ostream& out = std::cout);
/// @brief Narrate std::string_view and std::span. @param out Destination stream.
void demonstrateViews(std::ostream& out = std::cout);
/// @brief Run every utility demonstration. @param out Destination stream.
void runSTLUtilitiesDemo(std::ostream& out = std::cout);

}  // namespace CppVerseHub::STL
