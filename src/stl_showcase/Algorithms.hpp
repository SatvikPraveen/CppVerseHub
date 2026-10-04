/**
 * @file Algorithms.hpp
 * @brief STL algorithm showcase: non-modifying, modifying, sorting, numeric, set and permutation
 *        algorithms, in both classic iterator-pair and C++20 std::ranges (projection) forms.
 *
 * The functions here are small, reusable building blocks for mission planning. Each one is a
 * thin, correct composition of standard algorithms, chosen to illustrate a specific idea:
 * erase-remove, stable_partition, partial_sort_copy vs nth_element, heap maintenance,
 * transform_reduce/inclusive_scan, sorted-range set operations and next_permutation.
 */
#pragma once

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <functional>
#include <iostream>
#include <iterator>
#include <numeric>
#include <optional>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace CppVerseHub::STL {

/**
 * @brief A mission record used by the algorithm demonstrations.
 */
struct Mission {
    std::string id;                  ///< Unique identifier, e.g. "M001".
    std::string type;                ///< Category, e.g. "Combat".
    int priority{0};                 ///< 1 (low) .. 10 (critical).
    double duration_hours{0.0};      ///< Expected duration.
    double success_probability{1.0}; ///< In [0, 1].
    std::string assigned_fleet;      ///< Fleet in charge.

    /// @brief Member-wise equality.
    friend bool operator==(const Mission&, const Mission&) = default;
};

/**
 * @brief A stockpiled resource.
 */
struct Resource {
    std::string name;       ///< Resource name.
    int quantity{0};        ///< Units held.
    double unit_value{0.0}; ///< Credits per unit.
    std::string location;   ///< Storage location.

    /**
     * @brief Total value of the stockpile.
     * @return quantity * unit_value.
     */
    [[nodiscard]] constexpr double totalValue() const noexcept {
        return static_cast<double>(quantity) * unit_value;
    }
};

/**
 * @brief Stream a one-line mission summary.
 * @param os Destination stream.
 * @param mission Mission to print.
 * @return @p os.
 */
std::ostream& operator<<(std::ostream& os, const Mission& mission);

/**
 * @brief Deterministic sample missions (six entries, two of type "Combat").
 * @return Missions in a fixed order.
 */
[[nodiscard]] std::vector<Mission> sampleMissions();

/**
 * @brief Deterministic sample resources.
 * @return Four resources.
 */
[[nodiscard]] std::vector<Resource> sampleResources();

// ------------------------------------------------------------------------------ non-modifying

/**
 * @brief Find a mission by id (std::ranges::find with a projection).
 * @param missions Missions to search.
 * @param id Id to look for.
 * @return Copy of the mission, or std::nullopt.
 */
[[nodiscard]] std::optional<Mission> findMissionById(std::span<const Mission> missions, std::string_view id);

/**
 * @brief Count missions of a given type (std::ranges::count).
 * @param missions Missions to inspect.
 * @param type Type to count.
 * @return Number of matches.
 */
[[nodiscard]] std::size_t countMissionsOfType(std::span<const Mission> missions, std::string_view type);

/**
 * @brief Whether every mission meets a minimum success probability (std::ranges::all_of).
 * @param missions Missions to inspect (vacuously true when empty).
 * @param min_success Threshold in [0, 1].
 * @return true if all missions qualify.
 */
[[nodiscard]] bool allMissionsFeasible(std::span<const Mission> missions, double min_success);

/**
 * @brief Aggregate statistics over a mission list.
 */
struct MissionStats {
    std::size_t count{0};           ///< Number of missions.
    double total_hours{0.0};        ///< Sum of durations.
    double mean_hours{0.0};         ///< Mean duration (0 when empty).
    int min_priority{0};            ///< Smallest priority (0 when empty).
    int max_priority{0};            ///< Largest priority (0 when empty).
    double expected_successes{0.0}; ///< Sum of success probabilities.
};

/**
 * @brief Compute MissionStats with std::accumulate and std::ranges::minmax_element.
 * @param missions Missions to summarise.
 * @return Statistics; all zero for an empty input.
 */
[[nodiscard]] MissionStats computeMissionStats(std::span<const Mission> missions);

/**
 * @brief Index of the first position where two sequences differ (std::mismatch).
 * @param lhs First sequence.
 * @param rhs Second sequence.
 * @return Index of the first difference (or the shorter length if one is a prefix of the other),
 *         std::nullopt if the sequences are equal.
 */
[[nodiscard]] std::optional<std::size_t> firstMismatch(std::span<const int> lhs, std::span<const int> rhs);

/**
 * @brief Position of the first occurrence of @p needle inside @p haystack (std::search).
 * @param haystack Sequence to search.
 * @param needle Subsequence to find (an empty needle matches at 0).
 * @return Start index, or std::nullopt.
 */
[[nodiscard]] std::optional<std::size_t> findSubsequence(std::span<const int> haystack,
                                                         std::span<const int> needle);

/**
 * @brief Whether a string reads the same backwards (std::equal with reverse iterators).
 * @param text Text to test.
 * @return true for palindromes (including the empty string).
 */
[[nodiscard]] bool isPalindrome(std::string_view text) noexcept;

// ---------------------------------------------------------------------------------- modifying

/**
 * @brief Sort and remove duplicates in place (sort + unique + erase idiom).
 * @tparam T A totally ordered type.
 * @param values Vector to deduplicate.
 */
template <std::totally_ordered T>
void removeDuplicates(std::vector<T>& values) {
    std::ranges::sort(values);
    const auto tail = std::ranges::unique(values);
    values.erase(tail.begin(), tail.end());
}

/**
 * @brief Extract mission ids (std::ranges::transform with a member projection).
 * @param missions Source missions.
 * @return Ids in input order.
 */
[[nodiscard]] std::vector<std::string> extractIds(std::span<const Mission> missions);

/**
 * @brief Rotate left by @p k positions (std::rotate); k may exceed the size or be negative.
 * @param values Sequence to rotate (taken by value).
 * @param k Rotation amount; negative rotates right.
 * @return The rotated sequence.
 */
[[nodiscard]] std::vector<int> rotatedLeft(std::vector<int> values, std::ptrdiff_t k);

/**
 * @brief Replace every value above @p cap with @p cap (std::ranges::replace_if).
 * @param values Values to cap in place.
 * @param cap Upper bound.
 * @return Number of values changed.
 */
std::size_t capValues(std::vector<int>& values, int cap);

/**
 * @brief Move missions with priority >= threshold to the front, preserving relative order
 *        within both groups (std::stable_partition).
 * @param missions Missions to reorder.
 * @param threshold Minimum priority of the front group.
 * @return Size of the front group.
 */
std::size_t partitionByPriority(std::vector<Mission>& missions, int threshold);

// ------------------------------------------------------------------------------------ sorting

/**
 * @brief Sort by priority (desc), then duration (asc), then id (asc) using std::tie.
 * @param missions Missions to sort in place.
 */
void sortMissionsByUrgency(std::vector<Mission>& missions);

/**
 * @brief Stable sort by type using a projection; equal types keep input order.
 * @param missions Missions to sort in place.
 */
void stableSortByType(std::vector<Mission>& missions);

/**
 * @brief The @p n highest-priority missions in urgency order (std::partial_sort_copy).
 * @param missions Source missions (not modified).
 * @param n Number wanted.
 * @return min(n, size) missions.
 */
[[nodiscard]] std::vector<Mission> topNByPriority(std::span<const Mission> missions, std::size_t n);

/**
 * @brief The n-th smallest element (0-based) via std::nth_element, average O(n).
 * @tparam T A totally ordered, copyable type.
 * @param values Values (taken by value; reordered internally).
 * @param n Rank requested.
 * @return The element that would be at index n after sorting.
 * @throws std::out_of_range if n >= values.size().
 */
template <std::totally_ordered T>
[[nodiscard]] T nthSmallest(std::vector<T> values, std::size_t n) {
    if (n >= values.size()) {
        throw std::out_of_range("nthSmallest: rank out of range");
    }
    const auto nth = values.begin() + static_cast<std::ptrdiff_t>(n);
    std::nth_element(values.begin(), nth, values.end());
    return *nth;
}

/**
 * @brief The k smallest values in ascending order, maintained with a bounded max-heap
 *        (std::make_heap / push_heap / pop_heap / sort_heap). O(n log k).
 * @tparam R Input range of a totally ordered, copyable type.
 * @param values Input values.
 * @param k Number wanted.
 * @return min(k, size) smallest values, ascending.
 */
template <std::ranges::input_range R>
    requires std::totally_ordered<std::ranges::range_value_t<R>>
[[nodiscard]] std::vector<std::ranges::range_value_t<R>> kSmallest(R&& values, std::size_t k) {
    std::vector<std::ranges::range_value_t<R>> heap;
    if (k == 0) {
        return heap;
    }
    if constexpr (std::ranges::sized_range<R>) {
        heap.reserve(std::min(k, static_cast<std::size_t>(std::ranges::size(values))));
    }
    for (const auto& value : values) {
        if (heap.size() < k) {
            heap.push_back(value);
            std::push_heap(heap.begin(), heap.end());
        } else if (value < heap.front()) {
            std::pop_heap(heap.begin(), heap.end());
            heap.back() = value;
            std::push_heap(heap.begin(), heap.end());
        }
    }
    std::sort_heap(heap.begin(), heap.end());
    return heap;
}

/**
 * @brief Index where @p value would be inserted to keep @p sorted ordered (std::lower_bound).
 * @param sorted Ascending sequence.
 * @param value Value to locate.
 * @return First index i with sorted[i] >= value.
 */
[[nodiscard]] std::size_t insertionIndex(std::span<const int> sorted, int value) noexcept;

/**
 * @brief Number of occurrences of @p value in a sorted sequence (std::equal_range), O(log n).
 * @param sorted Ascending sequence.
 * @param value Value to count.
 * @return Occurrence count.
 */
[[nodiscard]] std::size_t countInSorted(std::span<const int> sorted, int value) noexcept;

// ------------------------------------------------------------------------------------ numeric

/**
 * @brief Total value of all resources (std::transform_reduce).
 * @param resources Resources to value.
 * @return Sum of Resource::totalValue().
 */
[[nodiscard]] double totalResourceValue(std::span<const Resource> resources);

/**
 * @brief Running totals (std::inclusive_scan) widened to long long to avoid overflow.
 * @param values Input values.
 * @return out[i] = values[0] + ... + values[i].
 */
[[nodiscard]] std::vector<long long> prefixSums(std::span<const int> values);

/**
 * @brief Differences between neighbours (std::adjacent_difference); out[0] = values[0].
 * @param values Input values.
 * @return Same-sized difference sequence.
 */
[[nodiscard]] std::vector<int> adjacentDifferences(std::span<const int> values);

/**
 * @brief Dot product (std::inner_product) in long long.
 * @param lhs First vector.
 * @param rhs Second vector.
 * @return Sum of products.
 * @throws std::invalid_argument when the sizes differ.
 */
[[nodiscard]] long long dotProduct(std::span<const int> lhs, std::span<const int> rhs);

/**
 * @brief Consecutive integers starting at @p first (std::iota).
 * @param first First value.
 * @param count Number of values.
 * @return {first, first + 1, ...}.
 */
[[nodiscard]] std::vector<int> iotaVector(int first, std::size_t count);

/**
 * @brief Least common multiple of a sequence (std::lcm folded with std::accumulate).
 * @param values Positive integers.
 * @return lcm of all values; 1 for an empty input.
 */
[[nodiscard]] constexpr unsigned long long lcmOf(std::span<const unsigned long long> values) noexcept {
    unsigned long long result = 1;
    for (const auto value : values) {
        result = std::lcm(result, value);
    }
    return result;
}

// ---------------------------------------------------------------------------------------- set

/**
 * @brief Union of two sorted ranges (std::set_union).
 * @tparam R1 First sorted input range.
 * @tparam R2 Second sorted input range (same value type).
 * @param lhs Sorted input.
 * @param rhs Sorted input.
 * @return Sorted union (multiset semantics).
 */
template <std::ranges::input_range R1, std::ranges::input_range R2>
    requires std::mergeable<std::ranges::iterator_t<R1>, std::ranges::iterator_t<R2>,
                            std::back_insert_iterator<std::vector<std::ranges::range_value_t<R1>>>>
[[nodiscard]] std::vector<std::ranges::range_value_t<R1>> sortedUnion(R1&& lhs, R2&& rhs) {
    std::vector<std::ranges::range_value_t<R1>> result;
    std::ranges::set_union(lhs, rhs, std::back_inserter(result));
    return result;
}

/**
 * @brief Intersection of two sorted ranges (std::set_intersection).
 * @tparam R1 First sorted input range.
 * @tparam R2 Second sorted input range (same value type).
 * @param lhs Sorted input.
 * @param rhs Sorted input.
 * @return Sorted intersection.
 */
template <std::ranges::input_range R1, std::ranges::input_range R2>
    requires std::mergeable<std::ranges::iterator_t<R1>, std::ranges::iterator_t<R2>,
                            std::back_insert_iterator<std::vector<std::ranges::range_value_t<R1>>>>
[[nodiscard]] std::vector<std::ranges::range_value_t<R1>> sortedIntersection(R1&& lhs, R2&& rhs) {
    std::vector<std::ranges::range_value_t<R1>> result;
    std::ranges::set_intersection(lhs, rhs, std::back_inserter(result));
    return result;
}

/**
 * @brief Elements of @p lhs not in @p rhs (std::set_difference).
 * @tparam R1 First sorted input range.
 * @tparam R2 Second sorted input range (same value type).
 * @param lhs Sorted input.
 * @param rhs Sorted input.
 * @return Sorted difference.
 */
template <std::ranges::input_range R1, std::ranges::input_range R2>
    requires std::mergeable<std::ranges::iterator_t<R1>, std::ranges::iterator_t<R2>,
                            std::back_insert_iterator<std::vector<std::ranges::range_value_t<R1>>>>
[[nodiscard]] std::vector<std::ranges::range_value_t<R1>> sortedDifference(R1&& lhs, R2&& rhs) {
    std::vector<std::ranges::range_value_t<R1>> result;
    std::ranges::set_difference(lhs, rhs, std::back_inserter(result));
    return result;
}

/**
 * @brief Elements in exactly one of the inputs (std::set_symmetric_difference).
 * @tparam R1 First sorted input range.
 * @tparam R2 Second sorted input range (same value type).
 * @param lhs Sorted input.
 * @param rhs Sorted input.
 * @return Sorted symmetric difference.
 */
template <std::ranges::input_range R1, std::ranges::input_range R2>
    requires std::mergeable<std::ranges::iterator_t<R1>, std::ranges::iterator_t<R2>,
                            std::back_insert_iterator<std::vector<std::ranges::range_value_t<R1>>>>
[[nodiscard]] std::vector<std::ranges::range_value_t<R1>> sortedSymmetricDifference(R1&& lhs, R2&& rhs) {
    std::vector<std::ranges::range_value_t<R1>> result;
    std::ranges::set_symmetric_difference(lhs, rhs, std::back_inserter(result));
    return result;
}

/**
 * @brief Merge two sorted ranges keeping duplicates (std::merge, stable).
 * @tparam R1 First sorted input range.
 * @tparam R2 Second sorted input range (same value type).
 * @param lhs Sorted input.
 * @param rhs Sorted input.
 * @return Sorted merge of size lhs.size() + rhs.size().
 */
template <std::ranges::input_range R1, std::ranges::input_range R2>
    requires std::mergeable<std::ranges::iterator_t<R1>, std::ranges::iterator_t<R2>,
                            std::back_insert_iterator<std::vector<std::ranges::range_value_t<R1>>>>
[[nodiscard]] std::vector<std::ranges::range_value_t<R1>> mergeSorted(R1&& lhs, R2&& rhs) {
    std::vector<std::ranges::range_value_t<R1>> result;
    std::ranges::merge(lhs, rhs, std::back_inserter(result));
    return result;
}

/**
 * @brief Whether @p subset is contained in @p superset (std::includes, both sorted).
 * @param superset Sorted superset candidate.
 * @param subset Sorted subset candidate.
 * @return true when every element of subset appears in superset (with multiplicity).
 */
[[nodiscard]] bool includesAll(std::span<const int> superset, std::span<const int> subset);

// ------------------------------------------------------------------------------- permutations

/**
 * @brief All distinct permutations in lexicographic order (std::next_permutation).
 * @param text Characters to permute (duplicates are handled).
 * @return Distinct permutations, ascending.
 */
[[nodiscard]] std::vector<std::string> allPermutations(std::string text);

/**
 * @brief Whether two strings are anagrams (std::is_permutation).
 * @param lhs First string.
 * @param rhs Second string.
 * @return true if one is a permutation of the other.
 */
[[nodiscard]] bool isAnagram(std::string_view lhs, std::string_view rhs);

/**
 * @brief Case-insensitive lexicographic less-than (std::lexicographical_compare).
 * @param lhs First string.
 * @param rhs Second string.
 * @return true if lhs orders before rhs ignoring ASCII case.
 */
[[nodiscard]] bool caseInsensitiveLess(std::string_view lhs, std::string_view rhs) noexcept;

// ------------------------------------------------------------------------------ demonstrations

/// @brief Narrate find/count/all_of/mismatch/search/minmax. @param out Destination stream.
void demonstrateNonModifyingAlgorithms(std::ostream& out = std::cout);
/// @brief Narrate copy/transform/fill/generate/replace/remove/rotate/partition. @param out Destination
/// stream.
void demonstrateModifyingAlgorithms(std::ostream& out = std::cout);
/// @brief Narrate sort/stable_sort/partial_sort/nth_element/heaps/binary search. @param out Destination
/// stream.
void demonstrateSortingAlgorithms(std::ostream& out = std::cout);
/// @brief Narrate accumulate/reduce/scan/inner_product/iota/gcd/lcm. @param out Destination stream.
void demonstrateNumericAlgorithms(std::ostream& out = std::cout);
/// @brief Narrate set operations and merges on sorted ranges. @param out Destination stream.
void demonstrateSetAlgorithms(std::ostream& out = std::cout);
/// @brief Narrate permutations and lexicographic comparison. @param out Destination stream.
void demonstratePermutationAlgorithms(std::ostream& out = std::cout);
/// @brief Run every algorithm demonstration. @param out Destination stream.
void runAlgorithmsDemo(std::ostream& out = std::cout);

} // namespace CppVerseHub::STL
