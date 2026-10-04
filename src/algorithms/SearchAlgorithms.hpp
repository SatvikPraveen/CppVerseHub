/**
 * @file SearchAlgorithms.hpp
 * @brief Searching in sequences, strings and multidimensional space.
 *
 * Demonstrates:
 *  - Generic sequence searches (linear, binary family, exponential, jump, interpolation, ternary)
 *    written as concept-constrained function objects that accept iterator/sentinel pairs or ranges,
 *    comparators and projections, mirroring `std::ranges`. Their results are defined to coincide
 *    with the corresponding standard algorithms, which the tests verify property-style.
 *  - Exact string matching: naive, Knuth-Morris-Pratt (generic over any random-access range of
 *    equality-comparable values), Boyer-Moore-Horspool, Rabin-Karp and the Z-algorithm, plus
 *    Aho-Corasick for many patterns at once and a suffix array with Kasai LCP.
 *  - Approximate matching with Levenshtein edit distance.
 *  - Nearest-neighbour search with a k-d tree, checked against brute force.
 *
 * Complexities use n = text/sequence length, m = pattern length, z = number of matches.
 */

#ifndef CPPVERSEHUB_ALGORITHMS_SEARCHALGORITHMS_HPP
#define CPPVERSEHUB_ALGORITHMS_SEARCHALGORITHMS_HPP

#include <algorithm>
#include <array>
#include <cmath>
#include <compare>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <iterator>
#include <limits>
#include <optional>
#include <queue>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace CppVerseHub::Algorithms {

// ============================================================================================
// Sequence searches
// ============================================================================================

/**
 * @brief Linear search: first element whose projection equals `value`.
 *
 * Works on any input range. Time: O(n). Space: O(1).
 */
struct LinearSearchFn {
    /**
     * @brief Searches [first, last).
     * @param first Iterator to the first element.
     * @param last  Sentinel.
     * @param value Value to find.
     * @param proj  Projection applied to each element.
     * @return Iterator to the first match, or an iterator equal to `last`.
     */
    template <std::input_iterator I, std::sentinel_for<I> S, class T, class Proj = std::identity>
        requires std::indirect_binary_predicate<std::ranges::equal_to, std::projected<I, Proj>, const T*>
    constexpr I operator()(I first, S last, const T& value, Proj proj = {}) const {
        for (; first != last; ++first) {
            if (std::invoke(proj, *first) == value) {
                return first;
            }
        }
        return first;
    }

    /**
     * @brief Searches a range.
     * @param r     Range to search.
     * @param value Value to find.
     * @param proj  Projection applied to each element.
     * @return Iterator to the first match or `end(r)`.
     */
    template <std::ranges::input_range R, class T, class Proj = std::identity>
        requires std::indirect_binary_predicate<std::ranges::equal_to,
                                                std::projected<std::ranges::iterator_t<R>, Proj>, const T*>
    constexpr std::ranges::borrowed_iterator_t<R> operator()(R&& r, const T& value, Proj proj = {}) const {
        return (*this)(std::ranges::begin(r), std::ranges::end(r), value, std::move(proj));
    }
};

namespace detail {

/// Shared range overload for the binary-search family (value + comparator + projection).
template <class Derived>
struct BoundRangeAdaptor {
    /**
     * @brief Applies the derived search to a whole range.
     * @param r     Sorted (w.r.t. comp/proj) random-access range.
     * @param value Value to locate.
     * @param comp  Strict weak ordering.
     * @param proj  Projection.
     * @return Whatever the iterator overload returns.
     */
    template <std::ranges::random_access_range R, class T, class Comp = std::ranges::less,
              class Proj = std::identity>
        requires std::indirect_strict_weak_order<Comp, const T*,
                                                 std::projected<std::ranges::iterator_t<R>, Proj>>
    constexpr auto operator()(R&& r, const T& value, Comp comp = {}, Proj proj = {}) const {
        return static_cast<const Derived&>(*this)(std::ranges::begin(r), std::ranges::end(r), value,
                                                  std::move(comp), std::move(proj));
    }
};

/// First position p in [first, first + n) with !(proj(*p) < value).
template <class I, class T, class Comp, class Proj>
constexpr I lower_bound_n(I first, std::iter_difference_t<I> n, const T& value, Comp& comp, Proj& proj) {
    while (n > 0) {
        const auto half = n / 2;
        I mid = first + half;
        if (std::invoke(comp, std::invoke(proj, *mid), value)) {
            first = mid + 1;
            n -= half + 1;
        } else {
            n = half;
        }
    }
    return first;
}

/// First position p in [first, first + n) with value < proj(*p).
template <class I, class T, class Comp, class Proj>
constexpr I upper_bound_n(I first, std::iter_difference_t<I> n, const T& value, Comp& comp, Proj& proj) {
    while (n > 0) {
        const auto half = n / 2;
        I mid = first + half;
        if (!std::invoke(comp, value, std::invoke(proj, *mid))) {
            first = mid + 1;
            n -= half + 1;
        } else {
            n = half;
        }
    }
    return first;
}

} // namespace detail

/**
 * @brief Iterative binary search for the lower bound (first element not less than `value`).
 *
 * Precondition: the range is partitioned by `comp(proj(e), value)`. Time: O(log n) comparisons.
 * Space: O(1). Equivalent to `std::ranges::lower_bound`.
 */
struct LowerBoundFn : detail::BoundRangeAdaptor<LowerBoundFn> {
    using detail::BoundRangeAdaptor<LowerBoundFn>::operator();

    /**
     * @brief Searches [first, last).
     * @param first Iterator to the first element.
     * @param last  Sentinel.
     * @param value Value to locate.
     * @param comp  Strict weak ordering.
     * @param proj  Projection.
     * @return Iterator to the lower bound.
     */
    template <std::random_access_iterator I, std::sentinel_for<I> S, class T, class Comp = std::ranges::less,
              class Proj = std::identity>
        requires std::indirect_strict_weak_order<Comp, const T*, std::projected<I, Proj>>
    constexpr I operator()(I first, S last, const T& value, Comp comp = {}, Proj proj = {}) const {
        const auto n = std::ranges::distance(first, last);
        return detail::lower_bound_n(first, n, value, comp, proj);
    }
};

/**
 * @brief Binary search for the upper bound (first element greater than `value`).
 *
 * Time: O(log n). Space: O(1). Equivalent to `std::ranges::upper_bound`.
 */
struct UpperBoundFn : detail::BoundRangeAdaptor<UpperBoundFn> {
    using detail::BoundRangeAdaptor<UpperBoundFn>::operator();

    /**
     * @brief Searches [first, last).
     * @param first Iterator to the first element.
     * @param last  Sentinel.
     * @param value Value to locate.
     * @param comp  Strict weak ordering.
     * @param proj  Projection.
     * @return Iterator to the upper bound.
     */
    template <std::random_access_iterator I, std::sentinel_for<I> S, class T, class Comp = std::ranges::less,
              class Proj = std::identity>
        requires std::indirect_strict_weak_order<Comp, const T*, std::projected<I, Proj>>
    constexpr I operator()(I first, S last, const T& value, Comp comp = {}, Proj proj = {}) const {
        const auto n = std::ranges::distance(first, last);
        return detail::upper_bound_n(first, n, value, comp, proj);
    }
};

/**
 * @brief Range of elements equivalent to `value`, as a subrange [lower_bound, upper_bound).
 *
 * Time: O(log n). Space: O(1). Equivalent to `std::ranges::equal_range`.
 */
struct EqualRangeFn : detail::BoundRangeAdaptor<EqualRangeFn> {
    using detail::BoundRangeAdaptor<EqualRangeFn>::operator();

    /**
     * @brief Searches [first, last).
     * @param first Iterator to the first element.
     * @param last  Sentinel.
     * @param value Value to locate.
     * @param comp  Strict weak ordering.
     * @param proj  Projection.
     * @return Subrange of equivalent elements (possibly empty, positioned at the insertion point).
     */
    template <std::random_access_iterator I, std::sentinel_for<I> S, class T, class Comp = std::ranges::less,
              class Proj = std::identity>
        requires std::indirect_strict_weak_order<Comp, const T*, std::projected<I, Proj>>
    constexpr std::ranges::subrange<I> operator()(I first, S last, const T& value, Comp comp = {},
                                                  Proj proj = {}) const {
        const auto n = std::ranges::distance(first, last);
        I lo = detail::lower_bound_n(first, n, value, comp, proj);
        I hi = detail::upper_bound_n(lo, n - (lo - first), value, comp, proj);
        return {lo, hi};
    }
};

/**
 * @brief Binary search membership test.
 *
 * Time: O(log n). Space: O(1). Equivalent to `std::ranges::binary_search`.
 */
struct BinarySearchFn : detail::BoundRangeAdaptor<BinarySearchFn> {
    using detail::BoundRangeAdaptor<BinarySearchFn>::operator();

    /**
     * @brief Searches [first, last).
     * @param first Iterator to the first element.
     * @param last  Sentinel.
     * @param value Value to locate.
     * @param comp  Strict weak ordering.
     * @param proj  Projection.
     * @return true if an element equivalent to `value` exists.
     */
    template <std::random_access_iterator I, std::sentinel_for<I> S, class T, class Comp = std::ranges::less,
              class Proj = std::identity>
        requires std::indirect_strict_weak_order<Comp, const T*, std::projected<I, Proj>>
    constexpr bool operator()(I first, S last, const T& value, Comp comp = {}, Proj proj = {}) const {
        const auto n = std::ranges::distance(first, last);
        I it = detail::lower_bound_n(first, n, value, comp, proj);
        return it != first + n && !std::invoke(comp, value, std::invoke(proj, *it));
    }
};

/**
 * @brief Exponential (galloping) search for the lower bound.
 *
 * Probes positions 1, 2, 4, ... until passing `value`, then binary searches the last bracket.
 * Time: O(log p) where p is the position of the result, which beats binary search when the target
 * is near the front (and is how Timsort's galloping mode works). Space: O(1).
 */
struct ExponentialSearchFn : detail::BoundRangeAdaptor<ExponentialSearchFn> {
    using detail::BoundRangeAdaptor<ExponentialSearchFn>::operator();

    /**
     * @brief Searches [first, last).
     * @param first Iterator to the first element.
     * @param last  Sentinel.
     * @param value Value to locate.
     * @param comp  Strict weak ordering.
     * @param proj  Projection.
     * @return Iterator to the lower bound (same as LowerBoundFn).
     */
    template <std::random_access_iterator I, std::sentinel_for<I> S, class T, class Comp = std::ranges::less,
              class Proj = std::identity>
        requires std::indirect_strict_weak_order<Comp, const T*, std::projected<I, Proj>>
    constexpr I operator()(I first, S last, const T& value, Comp comp = {}, Proj proj = {}) const {
        using D = std::iter_difference_t<I>;
        const D n = std::ranges::distance(first, last);
        D prev = 0;
        D bound = 1;
        while (bound <= n && std::invoke(comp, std::invoke(proj, first[bound - 1]), value)) {
            prev = bound;
            bound = (bound > n / 2) ? n + 1 : bound * 2;
        }
        const D hi = std::min(bound, n);
        return detail::lower_bound_n(first + prev, hi - prev, value, comp, proj);
    }
};

/**
 * @brief Jump search: skip ahead in blocks of sqrt(n), then scan the block linearly.
 *
 * Time: O(sqrt n). Space: O(1). Useful when stepping backwards is expensive.
 */
struct JumpSearchFn : detail::BoundRangeAdaptor<JumpSearchFn> {
    using detail::BoundRangeAdaptor<JumpSearchFn>::operator();

    /**
     * @brief Searches [first, last).
     * @param first Iterator to the first element.
     * @param last  Sentinel.
     * @param value Value to locate.
     * @param comp  Strict weak ordering.
     * @param proj  Projection.
     * @return Iterator to the lower bound (same as LowerBoundFn).
     */
    template <std::random_access_iterator I, std::sentinel_for<I> S, class T, class Comp = std::ranges::less,
              class Proj = std::identity>
        requires std::indirect_strict_weak_order<Comp, const T*, std::projected<I, Proj>>
    constexpr I operator()(I first, S last, const T& value, Comp comp = {}, Proj proj = {}) const {
        using D = std::iter_difference_t<I>;
        const D n = std::ranges::distance(first, last);
        D step = 1;
        while (step * step < n) {
            ++step;
        }
        D block = 0;
        while (block + step <= n && std::invoke(comp, std::invoke(proj, first[block + step - 1]), value)) {
            block += step;
        }
        D i = block;
        const D stop = std::min(block + step, n);
        while (i < stop && std::invoke(comp, std::invoke(proj, first[i]), value)) {
            ++i;
        }
        return first + i;
    }
};

/**
 * @brief Interpolation search on a range sorted ascending by an arithmetic projection.
 *
 * Estimates the probe position by linear interpolation between the end keys. Time: expected
 * O(log log n) for uniformly distributed keys, worst O(n) (e.g. exponentially growing keys).
 * Space: O(1).
 */
struct InterpolationSearchFn {
    /**
     * @brief Searches [first, last).
     * @param first Iterator to the first element.
     * @param last  Sentinel.
     * @param value Key to find.
     * @param proj  Projection yielding an arithmetic key.
     * @return Iterator to *the first* element whose key equals `value`, or an iterator equal to `last`.
     */
    template <std::random_access_iterator I, std::sentinel_for<I> S, class T, class Proj = std::identity>
        requires std::is_arithmetic_v<std::remove_cvref_t<std::indirect_result_t<Proj&, I>>> &&
                 std::is_arithmetic_v<T>
    constexpr I operator()(I first, S last, const T& value, Proj proj = {}) const {
        using D = std::iter_difference_t<I>;
        const D n = std::ranges::distance(first, last);
        I end = first + n;
        if (n == 0) {
            return end;
        }
        auto key = [&](D i) { return std::invoke(proj, first[i]); };
        D lo = 0;
        D hi = n - 1;
        while (lo <= hi && !(value < key(lo)) && !(key(hi) < value)) {
            D pos = lo;
            const auto klo = static_cast<double>(key(lo));
            const auto khi = static_cast<double>(key(hi));
            if (khi > klo) {
                const double frac = (static_cast<double>(value) - klo) / (khi - klo);
                pos = lo + static_cast<D>(frac * static_cast<double>(hi - lo));
                pos = std::clamp(pos, lo, hi);
            }
            if (key(pos) < value) {
                lo = pos + 1;
            } else if (value < key(pos)) {
                hi = pos - 1;
            } else {
                // Found an equal key; walk back to the first one with a binary search.
                auto less = std::ranges::less{};
                return detail::lower_bound_n(first + lo, pos - lo + 1, value, less, proj);
            }
        }
        return end;
    }

    /**
     * @brief Searches a whole range.
     * @param r     Range sorted ascending by `proj`.
     * @param value Key to find.
     * @param proj  Projection yielding an arithmetic key.
     * @return Iterator to the first matching element or `end(r)`.
     */
    template <std::ranges::random_access_range R, class T, class Proj = std::identity>
        requires std::is_arithmetic_v<
                     std::remove_cvref_t<std::indirect_result_t<Proj&, std::ranges::iterator_t<R>>>> &&
                 std::is_arithmetic_v<T>
    constexpr std::ranges::borrowed_iterator_t<R> operator()(R&& r, const T& value, Proj proj = {}) const {
        return (*this)(std::ranges::begin(r), std::ranges::end(r), value, std::move(proj));
    }
};

/**
 * @brief Ternary search for the peak of a strictly unimodal sequence (strictly increasing, then
 *        strictly decreasing, either part possibly empty).
 *
 * Time: O(log n) comparisons (implemented as binary search on the slope, which needs fewer probes
 * than the classical thirds split). Space: O(1).
 */
struct TernarySearchPeakFn {
    /**
     * @brief Finds the maximum of a unimodal range.
     * @param r    Strictly unimodal random-access range.
     * @param comp Strict weak ordering on projected values.
     * @param proj Projection.
     * @return Iterator to the peak, or `end(r)` if the range is empty.
     */
    template <std::ranges::random_access_range R, class Comp = std::ranges::less, class Proj = std::identity>
        requires std::indirect_strict_weak_order<Comp, std::projected<std::ranges::iterator_t<R>, Proj>>
    constexpr std::ranges::borrowed_iterator_t<R> operator()(R&& r, Comp comp = {}, Proj proj = {}) const {
        auto first = std::ranges::begin(r);
        auto lo = std::ranges::range_difference_t<R>{0};
        auto hi = std::ranges::distance(r);
        if (hi == 0) {
            return first;
        }
        --hi;
        while (lo < hi) {
            const auto mid = lo + (hi - lo) / 2;
            if (std::invoke(comp, std::invoke(proj, first[mid]), std::invoke(proj, first[mid + 1]))) {
                lo = mid + 1; // still ascending
            } else {
                hi = mid;
            }
        }
        return first + lo;
    }
};

/**
 * @brief Ternary search for the argmax of a unimodal real function on [lo, hi].
 *
 * Each iteration discards a third of the interval. Time: O(log((hi - lo) / eps)) evaluations.
 * Space: O(1).
 *
 * @param f   Unimodal function (strictly increasing then strictly decreasing).
 * @param lo  Left end of the interval.
 * @param hi  Right end of the interval (hi >= lo).
 * @param eps Absolute tolerance of the returned abscissa.
 * @return x in [lo, hi] within `eps` of the maximiser.
 */
template <std::invocable<double> F>
    requires std::totally_ordered<std::invoke_result_t<F&, double>>
[[nodiscard]] double ternary_search_max(F&& f, double lo, double hi, double eps = 1e-9) {
    while (hi - lo > eps) {
        const double m1 = lo + (hi - lo) / 3.0;
        const double m2 = hi - (hi - lo) / 3.0;
        if (std::invoke(f, m1) < std::invoke(f, m2)) {
            lo = m1;
        } else {
            hi = m2;
        }
    }
    return lo + (hi - lo) / 2.0;
}

/// @brief Linear search niebloid.
inline constexpr LinearSearchFn linear_search{};
/// @brief Lower bound niebloid.
inline constexpr LowerBoundFn lower_bound{};
/// @brief Upper bound niebloid.
inline constexpr UpperBoundFn upper_bound{};
/// @brief Equal range niebloid.
inline constexpr EqualRangeFn equal_range{};
/// @brief Binary search niebloid.
inline constexpr BinarySearchFn binary_search{};
/// @brief Exponential search niebloid.
inline constexpr ExponentialSearchFn exponential_search{};
/// @brief Jump search niebloid.
inline constexpr JumpSearchFn jump_search{};
/// @brief Interpolation search niebloid.
inline constexpr InterpolationSearchFn interpolation_search{};
/// @brief Unimodal peak search niebloid.
inline constexpr TernarySearchPeakFn ternary_search_peak{};

// ============================================================================================
// Exact string matching
// ============================================================================================

/**
 * @brief KMP prefix function: pi[i] = length of the longest proper border of pattern[0..i].
 * @param pattern Any random-access range of equality-comparable values.
 * @return The prefix-function table (same length as the pattern).
 *
 * Time: O(m) amortised. Space: O(m).
 */
template <std::ranges::random_access_range R>
    requires std::equality_comparable<std::ranges::range_value_t<R>>
[[nodiscard]] std::vector<std::size_t> prefix_function(const R& pattern) {
    const auto m = static_cast<std::size_t>(std::ranges::size(pattern));
    auto p = std::ranges::begin(pattern);
    std::vector<std::size_t> pi(m, 0);
    for (std::size_t i = 1; i < m; ++i) {
        std::size_t k = pi[i - 1];
        while (k > 0 && !(p[static_cast<std::ptrdiff_t>(i)] == p[static_cast<std::ptrdiff_t>(k)])) {
            k = pi[k - 1];
        }
        if (p[static_cast<std::ptrdiff_t>(i)] == p[static_cast<std::ptrdiff_t>(k)]) {
            ++k;
        }
        pi[i] = k;
    }
    return pi;
}

/**
 * @brief Knuth-Morris-Pratt search for all (possibly overlapping) occurrences.
 * @param text    Sequence to search.
 * @param pattern Sequence to find; an empty pattern matches at every position 0..n.
 * @return Sorted start positions of every occurrence.
 *
 * Generic over random-access ranges of equality-comparable values (strings, vectors of tokens,
 * DNA bases...). Time: O(n + m) worst case. Space: O(m).
 */
template <std::ranges::random_access_range T, std::ranges::random_access_range P>
    requires std::equality_comparable_with<std::ranges::range_reference_t<const T>,
                                           std::ranges::range_reference_t<const P>> &&
             std::equality_comparable<std::ranges::range_value_t<P>>
[[nodiscard]] std::vector<std::size_t> kmp_search(const T& text, const P& pattern) {
    const auto n = static_cast<std::size_t>(std::ranges::size(text));
    const auto m = static_cast<std::size_t>(std::ranges::size(pattern));
    std::vector<std::size_t> matches;
    if (m == 0) {
        for (std::size_t i = 0; i <= n; ++i) {
            matches.push_back(i);
        }
        return matches;
    }
    const auto pi = prefix_function(pattern);
    auto t = std::ranges::begin(text);
    auto p = std::ranges::begin(pattern);
    std::size_t k = 0;
    for (std::size_t i = 0; i < n; ++i) {
        while (k > 0 && !(t[static_cast<std::ptrdiff_t>(i)] == p[static_cast<std::ptrdiff_t>(k)])) {
            k = pi[k - 1];
        }
        if (t[static_cast<std::ptrdiff_t>(i)] == p[static_cast<std::ptrdiff_t>(k)]) {
            ++k;
        }
        if (k == m) {
            matches.push_back(i + 1 - m);
            k = pi[k - 1];
        }
    }
    return matches;
}

/**
 * @brief Naive (brute-force) string search; the reference oracle for the faster algorithms.
 * @param text    Text to search.
 * @param pattern Pattern; empty matches at every position 0..n.
 * @return Sorted start positions of every occurrence.
 *
 * Time: O(n * m) worst case. Space: O(1) besides the output.
 */
[[nodiscard]] std::vector<std::size_t> naive_search(std::string_view text, std::string_view pattern);

/**
 * @brief Boyer-Moore-Horspool search (bad-character shift table on the last window byte).
 * @param text    Text to search.
 * @param pattern Pattern; empty matches at every position 0..n.
 * @return Sorted start positions of every occurrence.
 *
 * Time: O(n / m) best (sublinear), O(n * m) worst. Space: O(256).
 */
[[nodiscard]] std::vector<std::size_t> boyer_moore_horspool_search(std::string_view text,
                                                                   std::string_view pattern);

/**
 * @brief Rabin-Karp search with a 64-bit polynomial rolling hash modulo the Mersenne prime 2^61-1.
 * @param text    Text to search.
 * @param pattern Pattern; empty matches at every position 0..n.
 * @return Sorted start positions of every occurrence (hash hits are verified, so no false matches).
 *
 * Time: O(n + m) expected, O(n * m) worst (adversarial collisions). Space: O(1).
 */
[[nodiscard]] std::vector<std::size_t> rabin_karp_search(std::string_view text, std::string_view pattern);

/**
 * @brief Z-function: z[i] = length of the longest common prefix of s and s[i..]; z[0] = |s|.
 * @param s Input string.
 * @return The Z array.
 *
 * Time: O(n). Space: O(n).
 */
[[nodiscard]] std::vector<std::size_t> z_function(std::string_view s);

/**
 * @brief Z-algorithm search (Z-function over pattern + separator-free concatenation).
 * @param text    Text to search.
 * @param pattern Pattern; empty matches at every position 0..n.
 * @return Sorted start positions of every occurrence.
 *
 * Time: O(n + m). Space: O(n + m).
 */
[[nodiscard]] std::vector<std::size_t> z_search(std::string_view text, std::string_view pattern);

/**
 * @class AhoCorasick
 * @brief Aho-Corasick automaton: finds all occurrences of many patterns in one pass.
 *
 * The trie is stored as a flat vector of nodes with a full 256-way transition table (goto plus
 * failure links folded into a DFA), so matching performs exactly one table lookup per text byte.
 * Construction: O(L * 256) time/space where L = total pattern length. Matching: O(n + z).
 */
class AhoCorasick {
public:
    /// @brief One match: pattern `pattern_index` occurs at `position` in the text.
    struct Match {
        std::size_t position;      ///< Start offset in the text.
        std::size_t pattern_index; ///< Index into the pattern list given at construction.
        /// @brief Lexicographic ordering by (position, pattern_index).
        friend constexpr auto operator<=>(const Match&, const Match&) = default;
    };

    /**
     * @brief Builds the automaton.
     * @param patterns Patterns to search for; empty patterns are ignored.
     */
    explicit AhoCorasick(std::vector<std::string> patterns);

    /**
     * @brief Finds every occurrence of every pattern.
     * @param text Text to scan.
     * @return Matches sorted by (position, pattern_index).
     */
    [[nodiscard]] std::vector<Match> find_all(std::string_view text) const;

    /// @brief Number of automaton states (trie nodes).
    [[nodiscard]] std::size_t state_count() const noexcept { return nodes_.size(); }

    /// @brief The patterns this automaton was built from.
    [[nodiscard]] const std::vector<std::string>& patterns() const noexcept { return patterns_; }

private:
    struct Node {
        std::array<std::int32_t, 256> next{};
        std::int32_t fail = 0;
        std::int32_t dict_link = -1;     ///< nearest proper suffix state that ends a pattern
        std::vector<std::size_t> output; ///< patterns ending exactly at this state
    };
    std::vector<std::string> patterns_;
    std::vector<Node> nodes_;
};

/**
 * @class SuffixArray
 * @brief Suffix array with LCP array for substring queries.
 *
 * Built by prefix doubling with `std::sort` (O(n log^2 n) time, O(n) space); the LCP array is
 * computed with Kasai's algorithm in O(n). Pattern lookup is a binary search over suffixes in
 * O(m log n).
 */
class SuffixArray {
public:
    /**
     * @brief Builds the suffix and LCP arrays of `text`.
     * @param text Text to index (copied).
     */
    explicit SuffixArray(std::string text);

    /// @brief The indexed text.
    [[nodiscard]] const std::string& text() const noexcept { return text_; }
    /// @brief Starting offsets of the suffixes in lexicographic order.
    [[nodiscard]] const std::vector<std::size_t>& suffixes() const noexcept { return sa_; }
    /// @brief lcp()[i] = LCP of suffixes()[i-1] and suffixes()[i]; lcp()[0] = 0.
    [[nodiscard]] const std::vector<std::size_t>& lcp() const noexcept { return lcp_; }

    /**
     * @brief All occurrences of `pattern`.
     * @param pattern Pattern to locate (empty matches at every position 0..n).
     * @return Sorted start positions. Time: O(m log n + z log z).
     */
    [[nodiscard]] std::vector<std::size_t> find_all(std::string_view pattern) const;

    /**
     * @brief Number of occurrences of `pattern`.
     * @param pattern Pattern to count.
     * @return Occurrence count. Time: O(m log n).
     */
    [[nodiscard]] std::size_t count(std::string_view pattern) const;

    /**
     * @brief Longest substring occurring at least twice (leftmost-in-suffix-order on ties).
     * @return The substring (empty if none). Time: O(n).
     */
    [[nodiscard]] std::string longest_repeated_substring() const;

private:
    std::pair<std::size_t, std::size_t> range_of(std::string_view pattern) const;

    std::string text_;
    std::vector<std::size_t> sa_;
    std::vector<std::size_t> lcp_;
};

// ============================================================================================
// Approximate matching
// ============================================================================================

/**
 * @brief Levenshtein edit distance (unit-cost insert, delete, substitute).
 * @param a First string.
 * @param b Second string.
 * @return Minimum number of edits turning a into b.
 *
 * Wagner-Fischer dynamic programming with a single rolling row. Time: O(|a| * |b|).
 * Space: O(min(|a|, |b|)).
 */
[[nodiscard]] std::size_t levenshtein_distance(std::string_view a, std::string_view b);

/// @brief A dictionary word together with its edit distance to a query.
struct FuzzyMatch {
    std::string word;     ///< Dictionary entry.
    std::size_t distance; ///< Levenshtein distance to the query.
    /// @brief Equality on both fields.
    friend bool operator==(const FuzzyMatch&, const FuzzyMatch&) = default;
};

/**
 * @brief Dictionary words within `max_distance` edits of `query`.
 * @param dictionary   Candidate words.
 * @param query        Word to match.
 * @param max_distance Maximum edit distance (inclusive).
 * @return Matches sorted by (distance, word). Time: O(sum |w| * |query|).
 */
[[nodiscard]] std::vector<FuzzyMatch> fuzzy_search(const std::vector<std::string>& dictionary,
                                                   std::string_view query, std::size_t max_distance);

// ============================================================================================
// Nearest-neighbour search
// ============================================================================================

/**
 * @class KDTree
 * @brief Static k-d tree over points in R^Dim for k-nearest-neighbour and radius queries.
 *
 * Built by recursive median splits (`std::nth_element`) cycling through the axes, stored as an
 * implicit balanced tree over a permuted index array (no per-node allocation). Build: O(n log n).
 * k-NN query: O(log n + k log k) expected for well-distributed data, O(n) worst. Radius query:
 * O(n^(1-1/Dim) + z) worst for balanced trees. Space: O(n).
 *
 * @tparam T   Floating-point coordinate type.
 * @tparam Dim Number of dimensions (> 0).
 */
template <std::floating_point T, std::size_t Dim>
    requires(Dim > 0)
class KDTree {
public:
    /// @brief A point in R^Dim.
    using Point = std::array<T, Dim>;

    /**
     * @brief Builds a tree over `points` (copied).
     * @param points Points to index; results refer to positions in this vector.
     */
    explicit KDTree(std::vector<Point> points) : points_(std::move(points)), index_(points_.size()) {
        for (std::size_t i = 0; i < index_.size(); ++i) {
            index_[i] = i;
        }
        build(0, index_.size(), 0);
    }

    /// @brief Number of indexed points.
    [[nodiscard]] std::size_t size() const noexcept { return points_.size(); }

    /// @brief The indexed points.
    [[nodiscard]] const std::vector<Point>& points() const noexcept { return points_; }

    /**
     * @brief Squared Euclidean distance.
     * @param a First point.
     * @param b Second point.
     * @return |a - b|^2.
     */
    [[nodiscard]] static constexpr T squared_distance(const Point& a, const Point& b) noexcept {
        T s{};
        for (std::size_t d = 0; d < Dim; ++d) {
            const T diff = a[d] - b[d];
            s += diff * diff;
        }
        return s;
    }

    /**
     * @brief The k points nearest to `query`.
     * @param query Query point.
     * @param k     Number of neighbours.
     * @return Indices into points(), ordered by (distance, index); fewer than k if size() < k.
     */
    [[nodiscard]] std::vector<std::size_t> nearest(const Point& query, std::size_t k) const {
        std::priority_queue<std::pair<T, std::size_t>> best; // max-heap of (dist2, idx)
        if (k > 0) {
            knn(0, index_.size(), 0, query, k, best);
        }
        std::vector<std::pair<T, std::size_t>> out;
        while (!best.empty()) {
            out.push_back(best.top());
            best.pop();
        }
        std::ranges::sort(out);
        std::vector<std::size_t> idx;
        idx.reserve(out.size());
        for (const auto& entry : out) {
            idx.push_back(entry.second);
        }
        return idx;
    }

    /**
     * @brief All points within `radius` (inclusive) of `center`.
     * @param center Query centre.
     * @param radius Search radius (>= 0).
     * @return Sorted indices into points().
     */
    [[nodiscard]] std::vector<std::size_t> within_radius(const Point& center, T radius) const {
        std::vector<std::size_t> out;
        radius_query(0, index_.size(), 0, center, radius * radius, out);
        std::ranges::sort(out);
        return out;
    }

    /**
     * @brief Brute-force k-NN reference implementation (O(n log n)).
     * @param points Point set.
     * @param query  Query point.
     * @param k      Number of neighbours.
     * @return Indices ordered by (distance, index).
     */
    [[nodiscard]] static std::vector<std::size_t> brute_force_nearest(const std::vector<Point>& points,
                                                                      const Point& query, std::size_t k) {
        std::vector<std::pair<T, std::size_t>> all;
        all.reserve(points.size());
        for (std::size_t i = 0; i < points.size(); ++i) {
            all.emplace_back(squared_distance(points[i], query), i);
        }
        std::ranges::sort(all);
        std::vector<std::size_t> idx;
        for (std::size_t i = 0; i < std::min(k, all.size()); ++i) {
            idx.push_back(all[i].second);
        }
        return idx;
    }

private:
    // Subtree over index_[lo, hi): the median position mid = lo + (hi - lo) / 2 is the node.
    void build(std::size_t lo, std::size_t hi, std::size_t axis) {
        if (hi - lo <= 1) {
            return;
        }
        const std::size_t mid = lo + (hi - lo) / 2;
        auto first = index_.begin();
        std::nth_element(first + static_cast<std::ptrdiff_t>(lo), first + static_cast<std::ptrdiff_t>(mid),
                         first + static_cast<std::ptrdiff_t>(hi), [this, axis](std::size_t a, std::size_t b) {
                             return points_[a][axis] < points_[b][axis];
                         });
        const std::size_t next = (axis + 1) % Dim;
        build(lo, mid, next);
        build(mid + 1, hi, next);
    }

    void knn(std::size_t lo, std::size_t hi, std::size_t axis, const Point& q, std::size_t k,
             std::priority_queue<std::pair<T, std::size_t>>& best) const {
        if (lo >= hi) {
            return;
        }
        const std::size_t mid = lo + (hi - lo) / 2;
        const std::size_t id = index_[mid];
        const T d2 = squared_distance(points_[id], q);
        const std::pair<T, std::size_t> cand{d2, id};
        if (best.size() < k) {
            best.push(cand);
        } else if (cand < best.top()) {
            best.pop();
            best.push(cand);
        }
        const T delta = q[axis] - points_[id][axis];
        const std::size_t next = (axis + 1) % Dim;
        const bool left_first = delta < T{0};
        if (left_first) {
            knn(lo, mid, next, q, k, best);
        } else {
            knn(mid + 1, hi, next, q, k, best);
        }
        // Visit the far side only if the splitting plane is closer than the current k-th best.
        if (best.size() < k || delta * delta <= best.top().first) {
            if (left_first) {
                knn(mid + 1, hi, next, q, k, best);
            } else {
                knn(lo, mid, next, q, k, best);
            }
        }
    }

    void radius_query(std::size_t lo, std::size_t hi, std::size_t axis, const Point& c, T r2,
                      std::vector<std::size_t>& out) const {
        if (lo >= hi) {
            return;
        }
        const std::size_t mid = lo + (hi - lo) / 2;
        const std::size_t id = index_[mid];
        if (squared_distance(points_[id], c) <= r2) {
            out.push_back(id);
        }
        const T delta = c[axis] - points_[id][axis];
        const std::size_t next = (axis + 1) % Dim;
        if (delta <= T{0} || delta * delta <= r2) {
            radius_query(lo, mid, next, c, r2, out);
        }
        if (delta >= T{0} || delta * delta <= r2) {
            radius_query(mid + 1, hi, next, c, r2, out);
        }
    }

    std::vector<Point> points_;
    std::vector<std::size_t> index_;
};

/**
 * @brief Showcase of every search technique in this header.
 * @param out Stream to write to.
 */
void demonstrate_searching(std::ostream& out = std::cout);

} // namespace CppVerseHub::Algorithms

#endif // CPPVERSEHUB_ALGORITHMS_SEARCHALGORITHMS_HPP
