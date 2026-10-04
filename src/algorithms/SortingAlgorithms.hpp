/**
 * @file SortingAlgorithms.hpp
 * @brief Generic, concept-constrained sorting algorithms (comparison and distribution sorts).
 *
 * This header demonstrates how classic sorting algorithms are written in modern, generic C++20:
 *
 *  - Every algorithm is a *function object* ("niebloid"), exactly like the algorithms in
 *    `std::ranges`. Function objects are not found by argument-dependent lookup, so calling
 *    `merge_sort(v.begin(), v.end())` can never silently pick up an unrelated `std::` overload.
 *  - Every comparison sort is constrained by `std::sortable<I, Comp, Proj>` and accepts either an
 *    iterator/sentinel pair or a range, a comparator (default `std::ranges::less`) and a projection
 *    (default `std::identity`). They therefore work with any random-access container, with custom
 *    orderings, and with move-only element types.
 *  - Distribution sorts (counting, radix, bucket) are constrained on the *projected key type*
 *    (integral or floating point) rather than on a comparator.
 *
 * Stability (preserving the relative order of equivalent elements) is documented per algorithm and
 * verified by the property-based tests in `tests/algorithms/`.
 *
 * Complexities use n = number of elements, k = key range, w = key width in bytes.
 */

#ifndef CPPVERSEHUB_ALGORITHMS_SORTINGALGORITHMS_HPP
#define CPPVERSEHUB_ALGORITHMS_SORTINGALGORITHMS_HPP

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <future>
#include <iostream>
#include <iterator>
#include <limits>
#include <numeric>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace CppVerseHub::Algorithms {

// ============================================================================================
// Concepts
// ============================================================================================

/**
 * @brief An integral type usable as a distribution-sort key (every integral type except `bool`).
 */
template <class T>
concept RadixKey = std::integral<T> && !std::same_as<std::remove_cv_t<T>, bool>;

/**
 * @brief A random-access range whose elements can be reordered by a distribution sort keyed by
 *        `Proj` producing a value satisfying `KeyConcept`.
 */
template <class R, class Proj>
concept PermutableRandomAccessRange =
    std::ranges::random_access_range<R> && std::permutable<std::ranges::iterator_t<R>> &&
    std::indirectly_regular_unary_invocable<Proj, std::ranges::iterator_t<R>>;

namespace detail {

/// Projected key type of an iterator under a projection, with cv-ref removed.
template <class I, class Proj>
using projected_key_t = std::remove_cvref_t<std::indirect_result_t<Proj&, I>>;

/**
 * @brief CRTP mix-in that gives an iterator-based sorting function object a range overload.
 *
 * The derived class implements `operator()(I first, S last, Comp, Proj)`; this base forwards
 * `operator()(R&& range, Comp, Proj)` to it and returns a dangling-safe iterator.
 */
template <class Derived>
struct SortRangeAdaptor {
    /**
     * @brief Sorts a whole random-access range.
     * @param r    Range to sort in place.
     * @param comp Strict weak ordering on projected values.
     * @param proj Projection applied to each element before comparison.
     * @return Iterator equal to `std::ranges::end(r)` (or `std::ranges::dangling`).
     */
    template <std::ranges::random_access_range R, class Comp = std::ranges::less, class Proj = std::identity,
              class D = Derived>
        requires std::sortable<std::ranges::iterator_t<R>, Comp, Proj> &&
                 std::invocable<const D&, std::ranges::iterator_t<R>, std::ranges::sentinel_t<R>, Comp, Proj>
    constexpr std::ranges::borrowed_iterator_t<R> operator()(R&& r, Comp comp = {}, Proj proj = {}) const {
        return static_cast<const Derived&>(*this)(std::ranges::begin(r), std::ranges::end(r), std::move(comp),
                                                  std::move(proj));
    }
};

/// Builds a binary predicate `less(a, b)` = `comp(proj(a), proj(b))`.
template <class Comp, class Proj>
constexpr auto make_less(Comp& comp, Proj& proj) {
    return [&comp, &proj](auto&& a, auto&& b) -> bool {
        return std::invoke(comp, std::invoke(proj, std::forward<decltype(a)>(a)),
                           std::invoke(proj, std::forward<decltype(b)>(b)));
    };
}

/// Stable insertion sort on [first, last) using a prepared `less` predicate.
template <std::random_access_iterator I, class Less>
constexpr void insertion_sort_impl(I first, I last, Less& less) {
    if (first == last) {
        return;
    }
    for (I i = std::next(first); i != last; ++i) {
        if (!less(*i, *std::prev(i))) {
            continue; // already in place; keeps sorted input O(n)
        }
        std::iter_value_t<I> tmp = std::ranges::iter_move(i);
        I j = i;
        do {
            *j = std::ranges::iter_move(std::prev(j));
            --j;
        } while (j != first && less(tmp, *std::prev(j)));
        *j = std::move(tmp);
    }
}

/// Restores the max-heap property below `root` within the heap [first, first + size).
template <std::random_access_iterator I, class Less>
constexpr void sift_down(I first, std::iter_difference_t<I> root, std::iter_difference_t<I> size,
                         Less& less) {
    while (true) {
        auto child = 2 * root + 1;
        if (child >= size) {
            return;
        }
        if (child + 1 < size && less(first[child], first[child + 1])) {
            ++child;
        }
        if (!less(first[root], first[child])) {
            return;
        }
        std::ranges::iter_swap(first + root, first + child);
        root = child;
    }
}

/// In-place heap sort on [first, last).
template <std::random_access_iterator I, class Less>
constexpr void heap_sort_impl(I first, I last, Less& less) {
    const auto n = last - first;
    for (auto i = n / 2; i-- > 0;) {
        sift_down(first, i, n, less);
    }
    for (auto end = n - 1; end > 0; --end) {
        std::ranges::iter_swap(first, first + end);
        sift_down(first, decltype(end){0}, end, less);
    }
}

/// Moves the median of `a`, `b`, `c` into `a` (the other two keep arbitrary order).
template <std::random_access_iterator I, class Less>
constexpr void median_of_three_to_front(I a, I b, I c, Less& less) {
    // Three-element sorting network: afterwards *a <= *b <= *c.
    if (less(*b, *a)) {
        std::ranges::iter_swap(a, b);
    }
    if (less(*c, *b)) {
        std::ranges::iter_swap(b, c);
        if (less(*b, *a)) {
            std::ranges::iter_swap(a, b);
        }
    }
    std::ranges::iter_swap(a, b); // median to the front
}

/**
 * @brief Sedgewick-style Hoare partition around the pivot stored at `*first`.
 * @return Final pivot position p: [first, p) <= pivot <= (p, last).
 *
 * Both scans stop on keys equal to the pivot, which keeps splits balanced for inputs with many
 * duplicates (a Lomuto partition degrades to O(n^2) there).
 */
template <std::random_access_iterator I, class Less>
constexpr I hoare_partition(I first, I last, Less& less) {
    I i = first;
    I j = last;
    while (true) {
        do {
            ++i;
        } while (i != last && less(*i, *first));
        do {
            --j;
        } while (less(*first, *j)); // terminates at `first` because !(p < p)
        if (!(i < j)) {
            break;
        }
        std::ranges::iter_swap(i, j);
    }
    std::ranges::iter_swap(first, j);
    return j;
}

inline constexpr std::ptrdiff_t kInsertionThreshold = 16;

/// Quicksort / introsort driver. `depth_limit < 0` disables the heap-sort fallback.
template <std::random_access_iterator I, class Less>
constexpr void quick_sort_loop(I first, I last, Less& less, int depth_limit) {
    while (last - first > kInsertionThreshold) {
        if (depth_limit == 0) {
            heap_sort_impl(first, last, less);
            return;
        }
        if (depth_limit > 0) {
            --depth_limit;
        }
        const auto n = last - first;
        median_of_three_to_front(first, first + n / 2, last - 1, less);
        I p = hoare_partition(first, last, less);
        // Recurse into the smaller side, iterate on the larger: O(log n) stack depth.
        if (p - first < last - p) {
            quick_sort_loop(first, p, less, depth_limit);
            first = std::next(p);
        } else {
            quick_sort_loop(std::next(p), last, less, depth_limit);
            last = p;
        }
    }
    insertion_sort_impl(first, last, less);
}

/**
 * @brief Stable merge of the sorted runs [first, mid) and [mid, last) using `buf` as scratch.
 *
 * Only the left run is moved into the buffer, so the scratch space is O(mid - first). Equivalent
 * elements are taken from the left run first, which is what makes every merge-based sort stable.
 */
template <std::random_access_iterator I, class Less>
void merge_adjacent(I first, I mid, I last, std::vector<std::iter_value_t<I>>& buf, Less& less) {
    if (first == mid || mid == last || !less(*mid, *std::prev(mid))) {
        return; // one side empty or already ordered
    }
    buf.clear();
    for (I it = first; it != mid; ++it) {
        buf.push_back(std::ranges::iter_move(it));
    }
    auto b = buf.begin();
    const auto be = buf.end();
    I r = mid;
    I out = first;
    while (b != be && r != last) {
        if (less(*r, *b)) {
            *out = std::ranges::iter_move(r);
            ++r;
        } else {
            *out = std::move(*b);
            ++b;
        }
        ++out;
    }
    std::move(b, be, out);
}

/// Top-down recursive merge sort with an insertion-sort cutoff.
template <std::random_access_iterator I, class Less>
void merge_sort_impl(I first, I last, std::vector<std::iter_value_t<I>>& buf, Less& less) {
    if (last - first <= kInsertionThreshold) {
        insertion_sort_impl(first, last, less);
        return;
    }
    I mid = first + (last - first) / 2;
    merge_sort_impl(first, mid, buf, less);
    merge_sort_impl(mid, last, buf, less);
    merge_adjacent(first, mid, last, buf, less);
}

/// Applies a permutation: afterwards element i of the range is the element formerly at order[i].
template <std::random_access_iterator I>
void apply_order(I first, const std::vector<std::size_t>& order) {
    std::vector<std::iter_value_t<I>> tmp;
    tmp.reserve(order.size());
    for (std::size_t idx : order) {
        tmp.push_back(std::ranges::iter_move(first + static_cast<std::iter_difference_t<I>>(idx)));
    }
    std::ranges::move(tmp, first);
}

/// Maps an integral key to an unsigned key with the same ordering (sign bit flipped for signed).
template <RadixKey K>
constexpr std::make_unsigned_t<K> to_ordered_unsigned(K key) noexcept {
    using U = std::make_unsigned_t<K>;
    auto u = static_cast<U>(key);
    if constexpr (std::is_signed_v<K>) {
        u = static_cast<U>(u ^ (U{1} << (std::numeric_limits<U>::digits - 1)));
    }
    return u;
}

} // namespace detail

// ============================================================================================
// Elementary O(n^2) comparison sorts
// ============================================================================================

/**
 * @brief Bubble sort with early termination and a shrinking boundary at the last swap.
 *
 * Stable. Time: best O(n) (already sorted), average/worst O(n^2). Space: O(1).
 */
struct BubbleSortFn : detail::SortRangeAdaptor<BubbleSortFn> {
    using detail::SortRangeAdaptor<BubbleSortFn>::operator();

    /**
     * @brief Sorts [first, last).
     * @param first Iterator to the first element.
     * @param last  Sentinel for the range.
     * @param comp  Strict weak ordering on projected values.
     * @param proj  Projection applied before comparison.
     * @return Iterator equal to `last`.
     */
    template <std::random_access_iterator I, std::sentinel_for<I> S, class Comp = std::ranges::less,
              class Proj = std::identity>
        requires std::sortable<I, Comp, Proj>
    constexpr I operator()(I first, S last, Comp comp = {}, Proj proj = {}) const {
        I end = std::ranges::next(first, last);
        auto less = detail::make_less(comp, proj);
        I bound = end;
        while (bound - first > 1) {
            I last_swap = first;
            for (I i = std::next(first); i != bound; ++i) {
                if (less(*i, *std::prev(i))) {
                    std::ranges::iter_swap(i, std::prev(i));
                    last_swap = i;
                }
            }
            bound = last_swap; // everything at or beyond the last swap is in final position
        }
        return end;
    }
};

/**
 * @brief Cocktail-shaker sort: bidirectional bubble sort that moves small "turtles" quickly.
 *
 * Stable. Time: best O(n), average/worst O(n^2). Space: O(1).
 */
struct CocktailShakerSortFn : detail::SortRangeAdaptor<CocktailShakerSortFn> {
    using detail::SortRangeAdaptor<CocktailShakerSortFn>::operator();

    /**
     * @brief Sorts [first, last).
     * @param first Iterator to the first element.
     * @param last  Sentinel for the range.
     * @param comp  Strict weak ordering on projected values.
     * @param proj  Projection applied before comparison.
     * @return Iterator equal to `last`.
     */
    template <std::random_access_iterator I, std::sentinel_for<I> S, class Comp = std::ranges::less,
              class Proj = std::identity>
        requires std::sortable<I, Comp, Proj>
    constexpr I operator()(I first, S last, Comp comp = {}, Proj proj = {}) const {
        I end = std::ranges::next(first, last);
        auto less = detail::make_less(comp, proj);
        I lo = first;
        I hi = end;
        while (hi - lo > 1) {
            I new_hi = lo;
            for (I i = std::next(lo); i != hi; ++i) {
                if (less(*i, *std::prev(i))) {
                    std::ranges::iter_swap(i, std::prev(i));
                    new_hi = i;
                }
            }
            hi = new_hi;
            if (hi - lo <= 1) {
                break;
            }
            I new_lo = hi;
            for (I i = std::prev(hi); i != lo; --i) {
                if (less(*i, *std::prev(i))) {
                    std::ranges::iter_swap(i, std::prev(i));
                    new_lo = i;
                }
            }
            lo = new_lo;
        }
        return end;
    }
};

/**
 * @brief Straight insertion sort.
 *
 * Stable and adaptive: O(n + d) where d is the number of inversions. Time: best O(n),
 * average/worst O(n^2). Space: O(1). The method of choice for tiny or nearly sorted inputs.
 */
struct InsertionSortFn : detail::SortRangeAdaptor<InsertionSortFn> {
    using detail::SortRangeAdaptor<InsertionSortFn>::operator();

    /**
     * @brief Sorts [first, last).
     * @param first Iterator to the first element.
     * @param last  Sentinel for the range.
     * @param comp  Strict weak ordering on projected values.
     * @param proj  Projection applied before comparison.
     * @return Iterator equal to `last`.
     */
    template <std::random_access_iterator I, std::sentinel_for<I> S, class Comp = std::ranges::less,
              class Proj = std::identity>
        requires std::sortable<I, Comp, Proj>
    constexpr I operator()(I first, S last, Comp comp = {}, Proj proj = {}) const {
        I end = std::ranges::next(first, last);
        auto less = detail::make_less(comp, proj);
        detail::insertion_sort_impl(first, end, less);
        return end;
    }
};

/**
 * @brief Binary insertion sort: binary search for the insertion point, then rotate.
 *
 * Stable (inserts after equivalent keys via upper bound). Comparisons: O(n log n);
 * moves: O(n^2) worst/average. Space: O(1).
 */
struct BinaryInsertionSortFn : detail::SortRangeAdaptor<BinaryInsertionSortFn> {
    using detail::SortRangeAdaptor<BinaryInsertionSortFn>::operator();

    /**
     * @brief Sorts [first, last).
     * @param first Iterator to the first element.
     * @param last  Sentinel for the range.
     * @param comp  Strict weak ordering on projected values.
     * @param proj  Projection applied before comparison.
     * @return Iterator equal to `last`.
     */
    template <std::random_access_iterator I, std::sentinel_for<I> S, class Comp = std::ranges::less,
              class Proj = std::identity>
        requires std::sortable<I, Comp, Proj>
    constexpr I operator()(I first, S last, Comp comp = {}, Proj proj = {}) const {
        I end = std::ranges::next(first, last);
        auto less = detail::make_less(comp, proj);
        if (first == end) {
            return end;
        }
        for (I i = std::next(first); i != end; ++i) {
            // upper bound of *i in [first, i)
            I lo = first;
            I hi = i;
            while (lo < hi) {
                I mid = lo + (hi - lo) / 2;
                if (less(*i, *mid)) {
                    hi = mid;
                } else {
                    lo = std::next(mid);
                }
            }
            std::ranges::rotate(lo, i, std::next(i));
        }
        return end;
    }
};

/**
 * @brief Selection sort: repeatedly select the minimum of the unsorted suffix.
 *
 * NOT stable. Time: Theta(n^2) comparisons in every case, but at most n - 1 swaps, which makes it
 * useful when writes are expensive. Space: O(1).
 */
struct SelectionSortFn : detail::SortRangeAdaptor<SelectionSortFn> {
    using detail::SortRangeAdaptor<SelectionSortFn>::operator();

    /**
     * @brief Sorts [first, last).
     * @param first Iterator to the first element.
     * @param last  Sentinel for the range.
     * @param comp  Strict weak ordering on projected values.
     * @param proj  Projection applied before comparison.
     * @return Iterator equal to `last`.
     */
    template <std::random_access_iterator I, std::sentinel_for<I> S, class Comp = std::ranges::less,
              class Proj = std::identity>
        requires std::sortable<I, Comp, Proj>
    constexpr I operator()(I first, S last, Comp comp = {}, Proj proj = {}) const {
        I end = std::ranges::next(first, last);
        auto less = detail::make_less(comp, proj);
        for (I i = first; i != end; ++i) {
            I min_it = i;
            for (I j = std::next(i); j != end; ++j) {
                if (less(*j, *min_it)) {
                    min_it = j;
                }
            }
            if (min_it != i) {
                std::ranges::iter_swap(i, min_it);
            }
        }
        return end;
    }
};

/**
 * @brief Shell sort using Ciura's empirically optimal gap sequence (extended by x2.25).
 *
 * NOT stable. Time: no tight bound is known for Ciura gaps; empirically about O(n^1.25), worst case
 * at least O(n^(4/3)). Space: O(1).
 */
struct ShellSortFn : detail::SortRangeAdaptor<ShellSortFn> {
    using detail::SortRangeAdaptor<ShellSortFn>::operator();

    /**
     * @brief Sorts [first, last).
     * @param first Iterator to the first element.
     * @param last  Sentinel for the range.
     * @param comp  Strict weak ordering on projected values.
     * @param proj  Projection applied before comparison.
     * @return Iterator equal to `last`.
     */
    template <std::random_access_iterator I, std::sentinel_for<I> S, class Comp = std::ranges::less,
              class Proj = std::identity>
        requires std::sortable<I, Comp, Proj>
    constexpr I operator()(I first, S last, Comp comp = {}, Proj proj = {}) const {
        I end = std::ranges::next(first, last);
        auto less = detail::make_less(comp, proj);
        using D = std::iter_difference_t<I>;
        const D n = end - first;
        std::vector<D> gaps{1, 4, 10, 23, 57, 132, 301, 701, 1750};
        while (gaps.back() < n) {
            gaps.push_back(gaps.back() * 9 / 4);
        }
        for (auto g = gaps.rbegin(); g != gaps.rend(); ++g) {
            const D gap = *g;
            if (gap >= n) {
                continue;
            }
            for (D i = gap; i < n; ++i) {
                if (!less(first[i], first[i - gap])) {
                    continue;
                }
                std::iter_value_t<I> tmp = std::ranges::iter_move(first + i);
                D j = i;
                do {
                    first[j] = std::ranges::iter_move(first + (j - gap));
                    j -= gap;
                } while (j >= gap && less(tmp, first[j - gap]));
                first[j] = std::move(tmp);
            }
        }
        return end;
    }
};

// ============================================================================================
// O(n log n) comparison sorts
// ============================================================================================

/**
 * @brief Heap sort: build a binary max-heap in place (Floyd, O(n)), then repeatedly pop the max.
 *
 * NOT stable. Time: O(n log n) in every case. Space: O(1). Poor cache locality compared with
 * quicksort, but it is the worst-case guarantee behind introsort.
 */
struct HeapSortFn : detail::SortRangeAdaptor<HeapSortFn> {
    using detail::SortRangeAdaptor<HeapSortFn>::operator();

    /**
     * @brief Sorts [first, last).
     * @param first Iterator to the first element.
     * @param last  Sentinel for the range.
     * @param comp  Strict weak ordering on projected values.
     * @param proj  Projection applied before comparison.
     * @return Iterator equal to `last`.
     */
    template <std::random_access_iterator I, std::sentinel_for<I> S, class Comp = std::ranges::less,
              class Proj = std::identity>
        requires std::sortable<I, Comp, Proj>
    constexpr I operator()(I first, S last, Comp comp = {}, Proj proj = {}) const {
        I end = std::ranges::next(first, last);
        auto less = detail::make_less(comp, proj);
        detail::heap_sort_impl(first, end, less);
        return end;
    }
};

/**
 * @brief Quicksort with median-of-three pivot, Hoare partitioning and insertion-sort cutoff.
 *
 * NOT stable. Time: average O(n log n); worst O(n^2) on adversarial ("median-of-3 killer") inputs.
 * Space: O(log n) stack, guaranteed by recursing into the smaller partition only.
 * Works with move-only element types (the pivot is never copied).
 */
struct QuickSortFn : detail::SortRangeAdaptor<QuickSortFn> {
    using detail::SortRangeAdaptor<QuickSortFn>::operator();

    /**
     * @brief Sorts [first, last).
     * @param first Iterator to the first element.
     * @param last  Sentinel for the range.
     * @param comp  Strict weak ordering on projected values.
     * @param proj  Projection applied before comparison.
     * @return Iterator equal to `last`.
     */
    template <std::random_access_iterator I, std::sentinel_for<I> S, class Comp = std::ranges::less,
              class Proj = std::identity>
        requires std::sortable<I, Comp, Proj>
    constexpr I operator()(I first, S last, Comp comp = {}, Proj proj = {}) const {
        I end = std::ranges::next(first, last);
        auto less = detail::make_less(comp, proj);
        detail::quick_sort_loop(first, end, less, -1);
        return end;
    }
};

/**
 * @brief Three-way ("Dutch national flag", Dijkstra) quicksort.
 *
 * Partitions into < pivot, == pivot, > pivot, so inputs with few distinct keys sort in
 * O(n log u) for u distinct keys (O(n) when all keys are equal). NOT stable.
 * Time: average O(n log n), worst O(n^2). Space: O(log n) stack.
 * Requires a copyable value type because the pivot value is held aside while elements move.
 */
struct QuickSort3WayFn : detail::SortRangeAdaptor<QuickSort3WayFn> {
    using detail::SortRangeAdaptor<QuickSort3WayFn>::operator();

    /**
     * @brief Sorts [first, last).
     * @param first Iterator to the first element.
     * @param last  Sentinel for the range.
     * @param comp  Strict weak ordering on projected values.
     * @param proj  Projection applied before comparison.
     * @return Iterator equal to `last`.
     */
    template <std::random_access_iterator I, std::sentinel_for<I> S, class Comp = std::ranges::less,
              class Proj = std::identity>
        requires std::sortable<I, Comp, Proj> && std::copyable<std::iter_value_t<I>>
    constexpr I operator()(I first, S last, Comp comp = {}, Proj proj = {}) const {
        I end = std::ranges::next(first, last);
        auto less = detail::make_less(comp, proj);
        sort_impl(first, end, less);
        return end;
    }

private:
    template <class I, class Less>
    static constexpr void sort_impl(I first, I last, Less& less) {
        while (last - first > detail::kInsertionThreshold) {
            const auto n = last - first;
            detail::median_of_three_to_front(first, first + n / 2, last - 1, less);
            const std::iter_value_t<I> pivot = *first;
            I lt = first; // [first, lt) < pivot
            I i = first;  // [lt, i) == pivot
            I gt = last;  // [gt, last) > pivot
            while (i < gt) {
                if (less(*i, pivot)) {
                    std::ranges::iter_swap(lt, i);
                    ++lt;
                    ++i;
                } else if (less(pivot, *i)) {
                    --gt;
                    std::ranges::iter_swap(i, gt);
                } else {
                    ++i;
                }
            }
            if (lt - first < last - gt) {
                sort_impl(first, lt, less);
                first = gt;
            } else {
                sort_impl(gt, last, less);
                last = lt;
            }
        }
        detail::insertion_sort_impl(first, last, less);
    }
};

/**
 * @brief Introsort (Musser 1997): quicksort that falls back to heap sort past depth 2*log2(n),
 *        finishing small partitions with insertion sort. This is how `std::sort` is implemented.
 *
 * NOT stable. Time: O(n log n) worst case. Space: O(log n).
 */
struct IntroSortFn : detail::SortRangeAdaptor<IntroSortFn> {
    using detail::SortRangeAdaptor<IntroSortFn>::operator();

    /**
     * @brief Sorts [first, last).
     * @param first Iterator to the first element.
     * @param last  Sentinel for the range.
     * @param comp  Strict weak ordering on projected values.
     * @param proj  Projection applied before comparison.
     * @return Iterator equal to `last`.
     */
    template <std::random_access_iterator I, std::sentinel_for<I> S, class Comp = std::ranges::less,
              class Proj = std::identity>
        requires std::sortable<I, Comp, Proj>
    constexpr I operator()(I first, S last, Comp comp = {}, Proj proj = {}) const {
        I end = std::ranges::next(first, last);
        auto less = detail::make_less(comp, proj);
        const auto n = static_cast<std::make_unsigned_t<std::iter_difference_t<I>>>(end - first);
        const int depth = 2 * static_cast<int>(std::bit_width(n));
        detail::quick_sort_loop(first, end, less, depth);
        return end;
    }
};

/**
 * @brief Top-down merge sort with an insertion-sort cutoff and a "skip already ordered" check.
 *
 * Stable. Time: O(n log n) worst case, O(n) on already sorted input. Space: O(n) auxiliary buffer
 * (only the left half of each merge is buffered).
 */
struct MergeSortFn : detail::SortRangeAdaptor<MergeSortFn> {
    using detail::SortRangeAdaptor<MergeSortFn>::operator();

    /**
     * @brief Sorts [first, last).
     * @param first Iterator to the first element.
     * @param last  Sentinel for the range.
     * @param comp  Strict weak ordering on projected values.
     * @param proj  Projection applied before comparison.
     * @return Iterator equal to `last`.
     */
    template <std::random_access_iterator I, std::sentinel_for<I> S, class Comp = std::ranges::less,
              class Proj = std::identity>
        requires std::sortable<I, Comp, Proj>
    I operator()(I first, S last, Comp comp = {}, Proj proj = {}) const {
        I end = std::ranges::next(first, last);
        auto less = detail::make_less(comp, proj);
        std::vector<std::iter_value_t<I>> buf;
        buf.reserve(static_cast<std::size_t>((end - first) / 2 + 1));
        detail::merge_sort_impl(first, end, buf, less);
        return end;
    }
};

/**
 * @brief Bottom-up (iterative) merge sort: merges runs of width 1, 2, 4, ... with no recursion.
 *
 * Stable. Time: Theta(n log n). Space: O(n) auxiliary buffer, O(1) stack.
 */
struct BottomUpMergeSortFn : detail::SortRangeAdaptor<BottomUpMergeSortFn> {
    using detail::SortRangeAdaptor<BottomUpMergeSortFn>::operator();

    /**
     * @brief Sorts [first, last).
     * @param first Iterator to the first element.
     * @param last  Sentinel for the range.
     * @param comp  Strict weak ordering on projected values.
     * @param proj  Projection applied before comparison.
     * @return Iterator equal to `last`.
     */
    template <std::random_access_iterator I, std::sentinel_for<I> S, class Comp = std::ranges::less,
              class Proj = std::identity>
        requires std::sortable<I, Comp, Proj>
    I operator()(I first, S last, Comp comp = {}, Proj proj = {}) const {
        I end = std::ranges::next(first, last);
        auto less = detail::make_less(comp, proj);
        using D = std::iter_difference_t<I>;
        const D n = end - first;
        std::vector<std::iter_value_t<I>> buf;
        buf.reserve(static_cast<std::size_t>(n / 2 + 1));
        for (D width = 1; width < n; width *= 2) {
            for (D lo = 0; lo < n - width; lo += 2 * width) {
                const D mid = lo + width;
                const D hi = std::min(lo + 2 * width, n);
                detail::merge_adjacent(first + lo, first + mid, first + hi, buf, less);
            }
        }
        return end;
    }
};

/**
 * @brief Simplified Timsort: natural-run detection, binary-insertion run extension to `minrun`,
 *        and run-stack merging under the (corrected, de Gouw et al. 2015) Timsort invariants.
 *
 * Galloping mode is omitted for clarity. Stable. Time: O(n log n) worst, O(n) on inputs made of
 * a few long ascending/descending runs. Space: O(n) buffer + O(log n) run stack.
 */
struct TimSortFn : detail::SortRangeAdaptor<TimSortFn> {
    using detail::SortRangeAdaptor<TimSortFn>::operator();

    /**
     * @brief Computes Timsort's minimum run length for n elements (in [32, 64] for n >= 64).
     * @param n Number of elements.
     * @return The minimum run length.
     */
    [[nodiscard]] static constexpr std::ptrdiff_t min_run_length(std::ptrdiff_t n) noexcept {
        std::ptrdiff_t r = 0;
        while (n >= 64) {
            r |= n & 1;
            n >>= 1;
        }
        return n + r;
    }

    /**
     * @brief Sorts [first, last).
     * @param first Iterator to the first element.
     * @param last  Sentinel for the range.
     * @param comp  Strict weak ordering on projected values.
     * @param proj  Projection applied before comparison.
     * @return Iterator equal to `last`.
     */
    template <std::random_access_iterator I, std::sentinel_for<I> S, class Comp = std::ranges::less,
              class Proj = std::identity>
        requires std::sortable<I, Comp, Proj>
    I operator()(I first, S last, Comp comp = {}, Proj proj = {}) const {
        I end = std::ranges::next(first, last);
        auto less = detail::make_less(comp, proj);
        using D = std::iter_difference_t<I>;
        const D n = end - first;
        if (n < 2) {
            return end;
        }
        const D minrun = static_cast<D>(min_run_length(static_cast<std::ptrdiff_t>(n)));
        std::vector<std::iter_value_t<I>> buf;
        struct Run {
            D start;
            D len;
        };
        std::vector<Run> stack;

        auto merge_at = [&](std::size_t i) {
            Run& a = stack[i];
            const Run& b = stack[i + 1];
            detail::merge_adjacent(first + a.start, first + b.start, first + (b.start + b.len), buf, less);
            a.len += b.len;
            stack.erase(stack.begin() + static_cast<std::ptrdiff_t>(i) + 1);
        };
        auto merge_collapse = [&] {
            while (stack.size() > 1) {
                std::size_t k = stack.size() - 2;
                const bool violates_c = (k > 0 && stack[k - 1].len <= stack[k].len + stack[k + 1].len) ||
                                        (k > 1 && stack[k - 2].len <= stack[k - 1].len + stack[k].len);
                if (violates_c) {
                    if (stack[k - 1].len < stack[k + 1].len) {
                        --k;
                    }
                } else if (stack[k].len > stack[k + 1].len) {
                    break;
                }
                merge_at(k);
            }
        };

        D lo = 0;
        while (lo < n) {
            // 1. Find a natural run starting at lo.
            D hi = lo + 1;
            if (hi < n) {
                if (less(first[hi], first[lo])) {
                    // strictly descending: reversing keeps stability since no two are equal
                    while (hi + 1 < n && less(first[hi + 1], first[hi])) {
                        ++hi;
                    }
                    ++hi;
                    std::ranges::reverse(first + lo, first + hi);
                } else {
                    while (hi + 1 < n && !less(first[hi + 1], first[hi])) {
                        ++hi;
                    }
                    ++hi;
                }
            }
            // 2. Extend short runs to minrun with insertion sort.
            const D forced = std::min(n, lo + minrun);
            if (hi < forced) {
                detail::insertion_sort_impl(first + lo, first + forced, less);
                hi = forced;
            }
            stack.push_back(Run{lo, hi - lo});
            merge_collapse();
            lo = hi;
        }
        while (stack.size() > 1) {
            std::size_t k = stack.size() - 2;
            if (k > 0 && stack[k - 1].len < stack[k + 1].len) {
                --k;
            }
            merge_at(k);
        }
        return end;
    }
};

/**
 * @brief Fork-join parallel merge sort built on `std::async`.
 *
 * The range is split recursively; the left half is sorted on a new task while the current thread
 * sorts the right half, down to a depth of about log2(hardware threads) and a grain size of
 * `kGrain` elements, below which the sequential merge sort is used. Each task owns its scratch
 * buffer and writes only to its own sub-range, so there are no data races; the comparator and
 * projection are copied into each task and must be safe to invoke concurrently.
 *
 * Stable (same merge as MergeSortFn). Work: O(n log n). Span: O(n) (the final merge is
 * sequential). Space: O(n). Exceptions thrown by `comp`/`proj` propagate to the caller.
 */
struct ParallelMergeSortFn : detail::SortRangeAdaptor<ParallelMergeSortFn> {
    using detail::SortRangeAdaptor<ParallelMergeSortFn>::operator();

    /// Sub-ranges smaller than this are sorted sequentially.
    static constexpr std::ptrdiff_t kGrain = 1 << 13;

    /**
     * @brief Sorts [first, last) using multiple threads.
     * @param first Iterator to the first element.
     * @param last  Sentinel for the range.
     * @param comp  Strict weak ordering on projected values (must be thread-safe to call).
     * @param proj  Projection applied before comparison (must be thread-safe to call).
     * @return Iterator equal to `last`.
     */
    template <std::random_access_iterator I, std::sentinel_for<I> S, class Comp = std::ranges::less,
              class Proj = std::identity>
        requires std::sortable<I, Comp, Proj>
    I operator()(I first, S last, Comp comp = {}, Proj proj = {}) const {
        I end = std::ranges::next(first, last);
        const unsigned hw = std::max(1U, std::thread::hardware_concurrency());
        const int depth = static_cast<int>(std::bit_width(hw));
        sort_impl(first, end, comp, proj, depth);
        return end;
    }

private:
    template <class I, class Comp, class Proj>
    static void sort_impl(I first, I last, const Comp& comp, const Proj& proj, int depth) {
        if (depth <= 0 || last - first < kGrain) {
            MergeSortFn{}(first, last, comp, proj);
            return;
        }
        I mid = first + (last - first) / 2;
        auto left = std::async(std::launch::async, [=] { sort_impl(first, mid, comp, proj, depth - 1); });
        sort_impl(mid, last, comp, proj, depth - 1);
        left.get();
        Comp c = comp;
        Proj p = proj;
        auto less = detail::make_less(c, p);
        std::vector<std::iter_value_t<I>> buf;
        buf.reserve(static_cast<std::size_t>(mid - first));
        detail::merge_adjacent(first, mid, last, buf, less);
    }
};

// ============================================================================================
// Distribution (non-comparison) sorts
// ============================================================================================

/**
 * @brief Stable counting sort keyed by an integral projection.
 *
 * Time: O(n + k) where k = max_key - min_key + 1. Space: O(n + k).
 * Throws std::length_error if k exceeds `max_key_range` (to avoid accidental huge allocations).
 */
struct CountingSortFn {
    /// Default upper bound on the key range k.
    static constexpr std::size_t kDefaultMaxKeyRange = std::size_t{1} << 24;

    /**
     * @brief Sorts a random-access range by integral keys.
     * @param r             Range to sort in place.
     * @param proj          Projection yielding an integral (non-bool) key.
     * @param max_key_range Maximum permitted k; larger ranges throw std::length_error.
     */
    template <std::ranges::random_access_range R, class Proj = std::identity>
        requires PermutableRandomAccessRange<R, Proj> &&
                 RadixKey<detail::projected_key_t<std::ranges::iterator_t<R>, Proj>>
    void operator()(R&& r, Proj proj = {}, std::size_t max_key_range = kDefaultMaxKeyRange) const {
        using K = detail::projected_key_t<std::ranges::iterator_t<R>, Proj>;
        using U = std::make_unsigned_t<K>;
        const auto n = static_cast<std::size_t>(std::ranges::distance(r));
        if (n < 2) {
            return;
        }
        auto first = std::ranges::begin(r);
        std::vector<U> keys(n);
        for (std::size_t i = 0; i < n; ++i) {
            keys[i] = detail::to_ordered_unsigned(
                static_cast<K>(std::invoke(proj, first[static_cast<std::ranges::range_difference_t<R>>(i)])));
        }
        const auto [mn, mx] = std::ranges::minmax(keys);
        const auto key_span = static_cast<std::uintmax_t>(mx - mn);
        if (key_span >= max_key_range) {
            throw std::length_error("counting_sort: key range exceeds max_key_range");
        }
        std::vector<std::size_t> count(static_cast<std::size_t>(key_span) + 2, 0);
        for (U k : keys) {
            ++count[static_cast<std::size_t>(k - mn) + 1];
        }
        std::partial_sum(count.begin(), count.end(), count.begin());
        std::vector<std::size_t> order(n);
        for (std::size_t i = 0; i < n; ++i) {
            order[count[static_cast<std::size_t>(keys[i] - mn)]++] = i;
        }
        detail::apply_order(first, order);
    }
};

/**
 * @brief Stable LSD radix sort (base 256) keyed by an integral projection.
 *
 * Signed keys are handled by flipping the sign bit. Passes over a byte that is identical for all
 * keys are skipped. Time: O(w * (n + 256)) with w = sizeof(key). Space: O(n).
 */
struct RadixSortFn {
    /**
     * @brief Sorts a random-access range by integral keys.
     * @param r    Range to sort in place.
     * @param proj Projection yielding an integral (non-bool) key.
     */
    template <std::ranges::random_access_range R, class Proj = std::identity>
        requires PermutableRandomAccessRange<R, Proj> &&
                 RadixKey<detail::projected_key_t<std::ranges::iterator_t<R>, Proj>>
    void operator()(R&& r, Proj proj = {}) const {
        using K = detail::projected_key_t<std::ranges::iterator_t<R>, Proj>;
        using U = std::make_unsigned_t<K>;
        const auto n = static_cast<std::size_t>(std::ranges::distance(r));
        if (n < 2) {
            return;
        }
        auto first = std::ranges::begin(r);
        std::vector<U> keys(n);
        for (std::size_t i = 0; i < n; ++i) {
            keys[i] = detail::to_ordered_unsigned(
                static_cast<K>(std::invoke(proj, first[static_cast<std::ranges::range_difference_t<R>>(i)])));
        }
        std::vector<std::size_t> order(n);
        std::vector<std::size_t> next(n);
        for (std::size_t i = 0; i < n; ++i) {
            order[i] = i;
        }
        for (std::size_t pass = 0; pass < sizeof(U); ++pass) {
            const unsigned shift = static_cast<unsigned>(pass * 8);
            std::array<std::size_t, 257> count{};
            for (U k : keys) {
                ++count[((k >> shift) & 0xFFU) + 1];
            }
            if (std::ranges::any_of(count, [n](std::size_t c) { return c == n; })) {
                continue; // every key has the same digit: this pass is the identity
            }
            std::partial_sum(count.begin(), count.end(), count.begin());
            for (std::size_t idx : order) {
                next[count[(keys[idx] >> shift) & 0xFFU]++] = idx;
            }
            order.swap(next);
        }
        detail::apply_order(first, order);
    }
};

/**
 * @brief Stable bucket sort keyed by a floating-point projection.
 *
 * Keys are scattered into n equal-width buckets over [min, max], each bucket is insertion sorted
 * and the buckets are concatenated. Time: expected O(n) for (near-)uniform keys, worst O(n^2) when
 * keys cluster into few buckets. Space: O(n). Precondition: no NaN keys.
 */
struct BucketSortFn {
    /**
     * @brief Sorts a random-access range by floating-point keys.
     * @param r    Range to sort in place.
     * @param proj Projection yielding a floating-point key (integers can be projected to double).
     */
    template <std::ranges::random_access_range R, class Proj = std::identity>
        requires PermutableRandomAccessRange<R, Proj> &&
                 std::floating_point<detail::projected_key_t<std::ranges::iterator_t<R>, Proj>>
    void operator()(R&& r, Proj proj = {}) const {
        using K = detail::projected_key_t<std::ranges::iterator_t<R>, Proj>;
        const auto n = static_cast<std::size_t>(std::ranges::distance(r));
        if (n < 2) {
            return;
        }
        auto first = std::ranges::begin(r);
        std::vector<K> keys(n);
        for (std::size_t i = 0; i < n; ++i) {
            keys[i] = std::invoke(proj, first[static_cast<std::ranges::range_difference_t<R>>(i)]);
        }
        const auto [mn, mx] = std::ranges::minmax(keys);
        if (!(mn < mx)) {
            return; // all keys equal
        }
        const K width = mx - mn;
        std::vector<std::vector<std::size_t>> buckets(n);
        for (std::size_t i = 0; i < n; ++i) {
            const K t = (keys[i] - mn) / width * static_cast<K>(n);
            const auto b = std::min(n - 1, static_cast<std::size_t>(t));
            buckets[b].push_back(i);
        }
        std::vector<std::size_t> order;
        order.reserve(n);
        auto key_less = [&keys](std::size_t a, std::size_t b) { return keys[a] < keys[b]; };
        for (auto& bucket : buckets) {
            detail::insertion_sort_impl(bucket.begin(), bucket.end(), key_less);
            order.insert(order.end(), bucket.begin(), bucket.end());
        }
        detail::apply_order(first, order);
    }
};

/// @brief Bubble sort niebloid. See BubbleSortFn.
inline constexpr BubbleSortFn bubble_sort{};
/// @brief Cocktail-shaker sort niebloid. See CocktailShakerSortFn.
inline constexpr CocktailShakerSortFn cocktail_shaker_sort{};
/// @brief Insertion sort niebloid. See InsertionSortFn.
inline constexpr InsertionSortFn insertion_sort{};
/// @brief Binary insertion sort niebloid. See BinaryInsertionSortFn.
inline constexpr BinaryInsertionSortFn binary_insertion_sort{};
/// @brief Selection sort niebloid. See SelectionSortFn.
inline constexpr SelectionSortFn selection_sort{};
/// @brief Shell sort niebloid. See ShellSortFn.
inline constexpr ShellSortFn shell_sort{};
/// @brief Heap sort niebloid. See HeapSortFn.
inline constexpr HeapSortFn heap_sort{};
/// @brief Quicksort niebloid. See QuickSortFn.
inline constexpr QuickSortFn quick_sort{};
/// @brief Three-way quicksort niebloid. See QuickSort3WayFn.
inline constexpr QuickSort3WayFn quick_sort_3way{};
/// @brief Introsort niebloid. See IntroSortFn.
inline constexpr IntroSortFn intro_sort{};
/// @brief Top-down merge sort niebloid. See MergeSortFn.
inline constexpr MergeSortFn merge_sort{};
/// @brief Bottom-up merge sort niebloid. See BottomUpMergeSortFn.
inline constexpr BottomUpMergeSortFn bottom_up_merge_sort{};
/// @brief Timsort niebloid. See TimSortFn.
inline constexpr TimSortFn tim_sort{};
/// @brief Parallel merge sort niebloid. See ParallelMergeSortFn.
inline constexpr ParallelMergeSortFn parallel_merge_sort{};
/// @brief Counting sort niebloid. See CountingSortFn.
inline constexpr CountingSortFn counting_sort{};
/// @brief Radix sort niebloid. See RadixSortFn.
inline constexpr RadixSortFn radix_sort{};
/// @brief Bucket sort niebloid. See BucketSortFn.
inline constexpr BucketSortFn bucket_sort{};

// ============================================================================================
// Instrumentation
// ============================================================================================

/**
 * @brief Comparator adaptor that counts how many times it is invoked.
 *
 * The counter is a non-owning reference to a caller-owned atomic (relaxed increments), so copies of
 * the comparator made by the algorithms all report to the same counter, including from the worker
 * threads of parallel_merge_sort. The counter must outlive the comparator.
 *
 * @tparam Comp Underlying comparator.
 */
template <class Comp = std::ranges::less>
class CountingComparator {
public:
    /**
     * @brief Wraps `comp`, counting into `counter`.
     * @param counter Caller-owned counter, incremented once per comparison.
     * @param comp    Underlying comparator.
     */
    explicit CountingComparator(std::atomic<std::size_t>& counter,
                                Comp comp = {}) noexcept(std::is_nothrow_move_constructible_v<Comp>)
        : counter_(&counter), comp_(std::move(comp)) {}

    /**
     * @brief Counts one comparison and forwards to the wrapped comparator.
     * @param a Left operand.
     * @param b Right operand.
     * @return `comp(a, b)`.
     */
    template <class A, class B>
        requires std::invocable<const Comp&, A, B>
    bool operator()(A&& a, B&& b) const {
        counter_->fetch_add(1, std::memory_order_relaxed);
        return std::invoke(comp_, std::forward<A>(a), std::forward<B>(b));
    }

private:
    std::atomic<std::size_t>* counter_;
    Comp comp_;
};

// ============================================================================================
// Runtime catalogue (used by the demo, benchmarks and tests)
// ============================================================================================

/// @brief Every sorting algorithm in this header, for runtime selection.
enum class SortAlgorithm {
    Bubble,
    CocktailShaker,
    Insertion,
    BinaryInsertion,
    Selection,
    Shell,
    Heap,
    Quick,
    Quick3Way,
    Intro,
    Merge,
    BottomUpMerge,
    Tim,
    ParallelMerge,
    Counting,
    Radix,
    Bucket
};

/// @brief All values of SortAlgorithm, in declaration order.
inline constexpr std::array<SortAlgorithm, 17> kAllSortAlgorithms{
    SortAlgorithm::Bubble,    SortAlgorithm::CocktailShaker,
    SortAlgorithm::Insertion, SortAlgorithm::BinaryInsertion,
    SortAlgorithm::Selection, SortAlgorithm::Shell,
    SortAlgorithm::Heap,      SortAlgorithm::Quick,
    SortAlgorithm::Quick3Way, SortAlgorithm::Intro,
    SortAlgorithm::Merge,     SortAlgorithm::BottomUpMerge,
    SortAlgorithm::Tim,       SortAlgorithm::ParallelMerge,
    SortAlgorithm::Counting,  SortAlgorithm::Radix,
    SortAlgorithm::Bucket};

/// @brief Static properties of a sorting algorithm.
struct SortAlgorithmInfo {
    std::string_view name;         ///< Human readable name.
    bool stable;                   ///< Preserves the order of equivalent elements.
    bool comparison_based;         ///< Uses only the comparator (vs. key distribution).
    bool quadratic;                ///< Average case is O(n^2) (avoid on large inputs).
    std::string_view average_time; ///< Average-case time complexity.
    std::string_view worst_time;   ///< Worst-case time complexity.
    std::string_view extra_space;  ///< Auxiliary space complexity.
};

/**
 * @brief Looks up the static properties of an algorithm.
 * @param algorithm Algorithm to describe.
 * @return Reference to a static description.
 */
[[nodiscard]] const SortAlgorithmInfo& sort_algorithm_info(SortAlgorithm algorithm) noexcept;

/**
 * @brief Sorts integers ascending with the chosen algorithm (runtime dispatch).
 * @param algorithm Algorithm to use.
 * @param data      Values to sort in place.
 * @return Number of comparator invocations (0 for distribution sorts).
 */
std::size_t sort_ints(SortAlgorithm algorithm, std::span<int> data);

/// @brief Input distributions used to exercise sorting algorithms.
enum class DataPattern {
    Random,
    Sorted,
    ReverseSorted,
    NearlySorted,
    FewUnique,
    Sawtooth,
    OrganPipe,
    AllEqual
};

/// @brief All values of DataPattern.
inline constexpr std::array<DataPattern, 8> kAllDataPatterns{
    DataPattern::Random,    DataPattern::Sorted,   DataPattern::ReverseSorted, DataPattern::NearlySorted,
    DataPattern::FewUnique, DataPattern::Sawtooth, DataPattern::OrganPipe,     DataPattern::AllEqual};

/**
 * @brief Human-readable name of a data pattern.
 * @param pattern Pattern to name.
 * @return Static name string.
 */
[[nodiscard]] std::string_view to_string(DataPattern pattern) noexcept;

/**
 * @brief Generates a deterministic test input.
 * @param size    Number of elements.
 * @param pattern Distribution to generate.
 * @param seed    RNG seed (std::mt19937_64), so results are reproducible.
 * @return The generated values.
 */
[[nodiscard]] std::vector<int> generate_data(std::size_t size, DataPattern pattern, std::uint64_t seed = 42);

/**
 * @brief Showcase: runs every algorithm on several input patterns and prints comparison counts,
 *        stability and complexity.
 * @param out Stream to write to.
 */
void demonstrate_sorting(std::ostream& out = std::cout);

} // namespace CppVerseHub::Algorithms

#endif // CPPVERSEHUB_ALGORITHMS_SORTINGALGORITHMS_HPP
