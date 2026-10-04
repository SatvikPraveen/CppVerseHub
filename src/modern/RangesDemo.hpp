/**
 * @file RangesDemo.hpp
 * @brief C++20 ranges: lazy view pipelines, range algorithms with projections, and a custom view.
 *
 * The ranges library replaces iterator pairs with composable, lazily evaluated *views*. This module
 * demonstrates:
 *  - pipelines built with `|` from standard adaptors (`filter`, `transform`, `take`, `drop`,
 *    `take_while`, `drop_while`, `reverse`, `iota`, `keys`, `values`, `elements`, `split`, `join`);
 *  - range algorithms with projections (`std::ranges::sort(v, {}, &Planet::population)`);
 *  - laziness: elements are computed only when a view is iterated (`countEvaluationsForFirst`);
 *  - writing a custom view (`EveryNthView`) with its own iterator/sentinel and a pipeable adaptor
 *    (`everyNth(n)`), i.e. the machinery behind C++23's `std::views::stride`.
 *
 * Only C++20 adaptors are used (no `zip`, `chunk`, `stride` or `std::ranges::to`, which are C++23), so
 * the code builds with libc++ and libstdc++ alike. `toVector` stands in for `std::ranges::to`.
 */
#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <map>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace CppVerseHub::Modern::Ranges {

// ===== DATA =====

/// @brief A planet in the sample galaxy.
struct Planet {
    int id = 0;                 ///< Identifier.
    std::string name;           ///< Display name.
    std::string system;         ///< Star system.
    double distanceAu = 0.0;    ///< Distance from its star in AU.
    long long population = 0;   ///< Inhabitants.
    double resourceValue = 0.0; ///< Economic value of resources.
    bool habitable = false;     ///< Supports life.
};

/// @brief A fleet in the sample galaxy.
struct Fleet {
    int id = 0;            ///< Identifier.
    std::string commander; ///< Commanding officer.
    int ships = 0;         ///< Ship count.
    double fuel = 0.0;     ///< Fuel percentage.
    bool active = true;    ///< Operational flag.
};

/// @brief A mission in the sample galaxy.
struct Mission {
    int id = 0;            ///< Identifier.
    std::string type;      ///< Mission type.
    int priority = 0;      ///< 1 (low) .. 5 (critical).
    double progress = 0.0; ///< Completion percentage.
};

/// @brief Deterministic sample planets. @return Ten planets across three systems.
[[nodiscard]] std::vector<Planet> generatePlanets();
/// @brief Deterministic sample fleets. @return Six fleets.
[[nodiscard]] std::vector<Fleet> generateFleets();
/// @brief Deterministic sample missions. @return Eight missions.
[[nodiscard]] std::vector<Mission> generateMissions();

// ===== GENERIC HELPERS =====

/// @brief Materialises any input range into a `std::vector` (stand-in for C++23 `std::ranges::to`).
/// @param r Range to copy. @return Vector of the range's values.
template <std::ranges::input_range R>
[[nodiscard]] auto toVector(R&& r) {
    std::vector<std::ranges::range_value_t<R>> out;
    if constexpr (std::ranges::sized_range<R>) {
        out.reserve(static_cast<std::size_t>(std::ranges::size(r)));
    }
    for (auto&& e : r) {
        out.push_back(std::forward<decltype(e)>(e));
    }
    return out;
}

// ===== CUSTOM VIEW =====

/// @brief A view over every n-th element of an underlying forward range (elements 0, n, 2n, ...).
/// @tparam V Underlying view.
template <std::ranges::view V>
    requires std::ranges::forward_range<V>
class EveryNthView : public std::ranges::view_interface<EveryNthView<V>> {
public:
    using difference_type = std::ranges::range_difference_t<V>; ///< Step type.

    /// @brief Forward iterator that advances `n` steps at a time, never past the end.
    class Iterator {
    public:
        using iterator_concept = std::forward_iterator_tag;         ///< Ranges iterator concept.
        using iterator_category = std::forward_iterator_tag;        ///< Legacy iterator category.
        using value_type = std::ranges::range_value_t<V>;           ///< Element value type.
        using difference_type = std::ranges::range_difference_t<V>; ///< Distance type.

        Iterator() = default;
        /// @brief Positions the iterator. @param cur Current. @param end End of base. @param step Stride.
        Iterator(std::ranges::iterator_t<V> cur, std::ranges::sentinel_t<V> end, difference_type step)
            : cur_(std::move(cur)), end_(std::move(end)), step_(step) {}

        /// @brief Dereference. @return The current element.
        decltype(auto) operator*() const { return *cur_; }
        /// @brief Advances by `step` (clamped at the end). @return *this.
        Iterator& operator++() {
            std::ranges::advance(cur_, step_, end_);
            return *this;
        }
        /// @brief Post-increment. @return Previous position.
        Iterator operator++(int) {
            Iterator tmp = *this;
            ++*this;
            return tmp;
        }
        /// @brief Iterator equality. @return True if at the same position.
        friend bool operator==(const Iterator& a, const Iterator& b) { return a.cur_ == b.cur_; }
        /// @brief Sentinel comparison. @return True if at the end.
        friend bool operator==(const Iterator& it, std::default_sentinel_t /*unused*/) {
            return it.cur_ == it.end_;
        }

    private:
        std::ranges::iterator_t<V> cur_{};
        std::ranges::sentinel_t<V> end_{};
        difference_type step_ = 1;
    };

    EveryNthView()
        requires std::default_initializable<V>
    = default;

    /// @brief Creates the view. @param base Underlying view. @param step Stride (values < 1 become 1).
    EveryNthView(V base, difference_type step) : base_(std::move(base)), step_(step < 1 ? 1 : step) {}

    /// @brief Start of the view. @return Iterator to the first element.
    [[nodiscard]] Iterator begin() {
        return Iterator(std::ranges::begin(base_), std::ranges::end(base_), step_);
    }
    /// @brief End of the view. @return `std::default_sentinel`.
    [[nodiscard]] std::default_sentinel_t end() const noexcept { return std::default_sentinel; }
    /// @brief The stride. @return n.
    [[nodiscard]] difference_type step() const noexcept { return step_; }

private:
    V base_{};
    difference_type step_ = 1;
};

/// @brief Deduction guide wrapping ranges in `views::all`.
template <typename R>
EveryNthView(R&&, std::ranges::range_difference_t<R>) -> EveryNthView<std::views::all_t<R>>;

/// @brief Pipeable adaptor object produced by `everyNth(n)`.
struct EveryNthAdaptor {
    std::ptrdiff_t step; ///< Stride.

    /// @brief `range | everyNth(n)`. @param r Viewable range. @param a Adaptor. @return The view.
    template <std::ranges::viewable_range R>
        requires std::ranges::forward_range<std::views::all_t<R>>
    friend auto operator|(R&& r, EveryNthAdaptor a) {
        using View = std::views::all_t<R>;
        return EveryNthView<View>(std::views::all(std::forward<R>(r)),
                                  static_cast<std::ranges::range_difference_t<View>>(a.step));
    }
};

/// @brief Creates a pipeable every-n-th adaptor. @param n Stride. @return Adaptor.
[[nodiscard]] constexpr EveryNthAdaptor everyNth(std::ptrdiff_t n) noexcept {
    return EveryNthAdaptor{n};
}

static_assert(std::ranges::forward_range<EveryNthView<std::views::all_t<std::vector<int>&>>>);
static_assert(std::ranges::view<EveryNthView<std::views::all_t<std::vector<int>&>>>);

// ===== QUERIES (all implemented with views / range algorithms) =====

/// @brief Names of habitable planets, in input order. @param planets Input. @return Names.
[[nodiscard]] std::vector<std::string> habitablePlanetNames(const std::vector<Planet>& planets);

/// @brief The `n` most populous planets, most populous first. @param planets Input. @param n Count.
/// @return Names.
[[nodiscard]] std::vector<std::string> topByPopulation(std::vector<Planet> planets, std::size_t n);

/// @brief Sum of all populations. @param planets Input. @return Total population.
[[nodiscard]] long long totalPopulation(const std::vector<Planet>& planets);

/// @brief Ids of active fleets with at least `minFuel` fuel and `minShips` ships.
/// @param fleets Input. @param minFuel Fuel threshold. @param minShips Ship threshold. @return Ids.
[[nodiscard]] std::vector<int> readyFleetIds(const std::vector<Fleet>& fleets, double minFuel, int minShips);

/// @brief Missions sorted by priority (desc), then progress (asc) — projections + stable sort.
/// @param missions Input (by value). @return Sorted ids.
[[nodiscard]] std::vector<int> missionIdsByUrgency(std::vector<Mission> missions);

/// @brief Groups planet names by star system. @param planets Input. @return system -> names.
[[nodiscard]] std::map<std::string, std::vector<std::string>> planetsBySystem(
    const std::vector<Planet>& planets);

/// @brief Splits text on a delimiter with `views::split`, dropping empty tokens.
/// @param text Input. @param delimiter Separator. @return Tokens.
[[nodiscard]] std::vector<std::string> splitWords(std::string_view text, char delimiter);

/// @brief Flattens nested vectors with `views::join`. @param nested Input. @return Flattened values.
[[nodiscard]] std::vector<int> flatten(const std::vector<std::vector<int>>& nested);

/// @brief First `count` squares of odd numbers, from an infinite `iota` view.
/// @param count Number of values. @return Squares 1, 9, 25, ...
[[nodiscard]] std::vector<long long> squaresOfOdds(std::size_t count);

/// @brief Demonstrates laziness: how many times a `transform` runs when only `first` results of a
///        filtered/transformed view over `data` are consumed.
/// @param data Input. @param first Elements consumed. @return Number of transform invocations.
[[nodiscard]] std::size_t countEvaluationsForFirst(const std::vector<int>& data, std::size_t first);

/// @brief Longest prefix of values below `limit` (`take_while`) and the remainder (`drop_while`).
/// @param data Input. @param limit Threshold. @return {prefix, rest}.
[[nodiscard]] std::pair<std::vector<int>, std::vector<int>> splitAtFirstNotBelow(const std::vector<int>& data,
                                                                                 int limit);

/// @brief Deterministic pseudo-random values (for benchmarks). @param n Count. @param seed Seed.
/// @return Values in [0, 1000).
[[nodiscard]] std::vector<int> makeRandomValues(std::size_t n, std::uint32_t seed);

/// @brief Sum of squares of even values, as a view pipeline. @param data Input. @return Sum.
[[nodiscard]] long long sumSquaresOfEvensRanges(const std::vector<int>& data);

/// @brief Same computation with a raw loop (baseline). @param data Input. @return Sum.
[[nodiscard]] long long sumSquaresOfEvensLoop(const std::vector<int>& data);

// ===== SHOWCASES =====

/// @brief Basic view pipelines over integers. @param out Destination stream.
void demonstrateBasicRanges(std::ostream& out = std::cout);
/// @brief Planet/fleet/mission analysis with projections. @param out Destination stream.
void demonstrateDomainQueries(std::ostream& out = std::cout);
/// @brief keys/values/elements/split/join/reverse/take_while. @param out Destination stream.
void demonstrateAdvancedAdaptors(std::ostream& out = std::cout);
/// @brief The custom `EveryNthView`. @param out Destination stream.
void demonstrateCustomView(std::ostream& out = std::cout);
/// @brief Lazy evaluation. @param out Destination stream.
void demonstrateLaziness(std::ostream& out = std::cout);
/// @brief Runs every ranges showcase. @param out Destination stream.
void demonstrateAllRanges(std::ostream& out = std::cout);

} // namespace CppVerseHub::Modern::Ranges
