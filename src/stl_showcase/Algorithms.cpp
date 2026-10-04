/**
 * @file Algorithms.cpp
 * @brief Implementation of the STL algorithm showcase.
 */
#include "stl_showcase/Algorithms.hpp"

#include <array>
#include <cctype>
#include <numeric>
#include <random>
#include <tuple>

namespace CppVerseHub::STL {

std::ostream& operator<<(std::ostream& os, const Mission& mission) {
    return os << mission.id << " (" << mission.type << ", p" << mission.priority << ", "
              << mission.duration_hours << "h, " << mission.success_probability * 100.0 << "%)";
}

std::vector<Mission> sampleMissions() {
    return {
        {"M001", "Exploration", 3, 24.0, 0.80, "Alpha Squadron"},
        {"M002", "Combat", 8, 6.0, 0.60, "Beta Fleet"},
        {"M003", "Transport", 2, 12.0, 0.95, "Cargo Wing"},
        {"M004", "Combat", 9, 8.0, 0.50, "Strike Force"},
        {"M005", "Diplomatic", 5, 48.0, 0.90, "Diplomatic Corps"},
        {"M006", "Rescue", 10, 4.0, 0.70, "Emergency Response"},
    };
}

std::vector<Resource> sampleResources() {
    return {
        {"Dilithium", 120, 50.0, "Starbase 1"},
        {"Deuterium", 800, 2.5, "Refinery"},
        {"Tritanium", 300, 12.0, "Shipyard"},
        {"Antimatter", 10, 400.0, "Containment Bay"},
    };
}

// ------------------------------------------------------------------------------ non-modifying

std::optional<Mission> findMissionById(std::span<const Mission> missions, std::string_view id) {
    const auto it = std::ranges::find(missions, id, &Mission::id);
    if (it == missions.end()) {
        return std::nullopt;
    }
    return *it;
}

std::size_t countMissionsOfType(std::span<const Mission> missions, std::string_view type) {
    return static_cast<std::size_t>(std::ranges::count(missions, type, &Mission::type));
}

bool allMissionsFeasible(std::span<const Mission> missions, double min_success) {
    return std::ranges::all_of(
        missions, [min_success](double p) { return p >= min_success; }, &Mission::success_probability);
}

MissionStats computeMissionStats(std::span<const Mission> missions) {
    MissionStats stats;
    if (missions.empty()) {
        return stats;
    }
    stats.count = missions.size();
    stats.total_hours = std::accumulate(missions.begin(), missions.end(), 0.0,
                                        [](double sum, const Mission& m) { return sum + m.duration_hours; });
    stats.mean_hours = stats.total_hours / static_cast<double>(stats.count);
    const auto [lowest, highest] = std::ranges::minmax_element(missions, {}, &Mission::priority);
    stats.min_priority = lowest->priority;
    stats.max_priority = highest->priority;
    stats.expected_successes = std::transform_reduce(missions.begin(), missions.end(), 0.0, std::plus<>{},
                                                     [](const Mission& m) { return m.success_probability; });
    return stats;
}

std::optional<std::size_t> firstMismatch(std::span<const int> lhs, std::span<const int> rhs) {
    const auto [l, r] = std::ranges::mismatch(lhs, rhs);
    if (l == lhs.end() && r == rhs.end()) {
        return std::nullopt;
    }
    return static_cast<std::size_t>(l - lhs.begin());
}

std::optional<std::size_t> findSubsequence(std::span<const int> haystack, std::span<const int> needle) {
    const auto it = std::search(haystack.begin(), haystack.end(), needle.begin(), needle.end());
    if (it == haystack.end() && !needle.empty()) {
        return std::nullopt;
    }
    return static_cast<std::size_t>(it - haystack.begin());
}

bool isPalindrome(std::string_view text) noexcept {
    return std::equal(text.begin(), text.begin() + static_cast<std::ptrdiff_t>(text.size() / 2),
                      text.rbegin());
}

// ---------------------------------------------------------------------------------- modifying

std::vector<std::string> extractIds(std::span<const Mission> missions) {
    std::vector<std::string> ids;
    ids.reserve(missions.size());
    std::ranges::transform(missions, std::back_inserter(ids), &Mission::id);
    return ids;
}

std::vector<int> rotatedLeft(std::vector<int> values, std::ptrdiff_t k) {
    if (values.empty()) {
        return values;
    }
    const auto n = static_cast<std::ptrdiff_t>(values.size());
    const std::ptrdiff_t shift = ((k % n) + n) % n;
    std::rotate(values.begin(), values.begin() + shift, values.end());
    return values;
}

std::size_t capValues(std::vector<int>& values, int cap) {
    const auto changed = std::ranges::count_if(values, [cap](int v) { return v > cap; });
    std::ranges::replace_if(values, [cap](int v) { return v > cap; }, cap);
    return static_cast<std::size_t>(changed);
}

std::size_t partitionByPriority(std::vector<Mission>& missions, int threshold) {
    const auto tail = std::ranges::stable_partition(
        missions, [threshold](int p) { return p >= threshold; }, &Mission::priority);
    return static_cast<std::size_t>(tail.begin() - missions.begin());
}

// ------------------------------------------------------------------------------------ sorting

namespace {

bool moreUrgent(const Mission& lhs, const Mission& rhs) noexcept {
    // Negate-free descending order on priority: swap lhs/rhs for that component only.
    return std::tie(rhs.priority, lhs.duration_hours, lhs.id) <
           std::tie(lhs.priority, rhs.duration_hours, rhs.id);
}

} // namespace

void sortMissionsByUrgency(std::vector<Mission>& missions) {
    std::ranges::sort(missions, moreUrgent);
}

void stableSortByType(std::vector<Mission>& missions) {
    std::ranges::stable_sort(missions, {}, &Mission::type);
}

std::vector<Mission> topNByPriority(std::span<const Mission> missions, std::size_t n) {
    std::vector<Mission> top(std::min(n, missions.size()));
    std::partial_sort_copy(missions.begin(), missions.end(), top.begin(), top.end(), moreUrgent);
    return top;
}

std::size_t insertionIndex(std::span<const int> sorted, int value) noexcept {
    return static_cast<std::size_t>(std::ranges::lower_bound(sorted, value) - sorted.begin());
}

std::size_t countInSorted(std::span<const int> sorted, int value) noexcept {
    return std::ranges::equal_range(sorted, value).size();
}

// ------------------------------------------------------------------------------------ numeric

double totalResourceValue(std::span<const Resource> resources) {
    return std::transform_reduce(resources.begin(), resources.end(), 0.0, std::plus<>{},
                                 [](const Resource& r) { return r.totalValue(); });
}

std::vector<long long> prefixSums(std::span<const int> values) {
    std::vector<long long> sums(values.size());
    std::inclusive_scan(values.begin(), values.end(), sums.begin(), std::plus<long long>{}, 0LL);
    return sums;
}

std::vector<int> adjacentDifferences(std::span<const int> values) {
    std::vector<int> diffs(values.size());
    std::adjacent_difference(values.begin(), values.end(), diffs.begin());
    return diffs;
}

long long dotProduct(std::span<const int> lhs, std::span<const int> rhs) {
    if (lhs.size() != rhs.size()) {
        throw std::invalid_argument("dotProduct: size mismatch");
    }
    return std::inner_product(lhs.begin(), lhs.end(), rhs.begin(), 0LL, std::plus<long long>{},
                              [](int a, int b) { return static_cast<long long>(a) * b; });
}

std::vector<int> iotaVector(int first, std::size_t count) {
    std::vector<int> values(count);
    std::iota(values.begin(), values.end(), first);
    return values;
}

// ---------------------------------------------------------------------------------------- set

bool includesAll(std::span<const int> superset, std::span<const int> subset) {
    return std::ranges::includes(superset, subset);
}

// ------------------------------------------------------------------------------- permutations

std::vector<std::string> allPermutations(std::string text) {
    std::vector<std::string> permutations;
    std::ranges::sort(text);
    do {
        permutations.push_back(text);
    } while (std::ranges::next_permutation(text).found);
    return permutations;
}

bool isAnagram(std::string_view lhs, std::string_view rhs) {
    return std::ranges::is_permutation(lhs, rhs);
}

bool caseInsensitiveLess(std::string_view lhs, std::string_view rhs) noexcept {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), [](char a, char b) {
        return std::tolower(static_cast<unsigned char>(a)) < std::tolower(static_cast<unsigned char>(b));
    });
}

// ------------------------------------------------------------------------------ demonstrations

namespace {

template <typename Range>
void printRange(std::ostream& out, std::string_view label, const Range& range) {
    out << label << ":";
    for (const auto& value : range) {
        out << ' ' << value;
    }
    out << '\n';
}

} // namespace

void demonstrateNonModifyingAlgorithms(std::ostream& out) {
    out << "\n=== Non-modifying Algorithms ===\n";
    const auto missions = sampleMissions();
    if (const auto found = findMissionById(missions, "M003")) {
        out << "ranges::find with projection: " << *found << '\n';
    }
    out << "combat missions (ranges::count): " << countMissionsOfType(missions, "Combat") << '\n';
    out << std::boolalpha << "all missions >= 50% (all_of): " << allMissionsFeasible(missions, 0.5) << '\n';
    const auto stats = computeMissionStats(missions);
    out << "stats: total " << stats.total_hours << "h, mean " << stats.mean_hours << "h, priority range ["
        << stats.min_priority << ", " << stats.max_priority << "], expected successes "
        << stats.expected_successes << '\n';
    const std::array<int, 6> telemetry{4, 8, 15, 16, 23, 42};
    const std::array<int, 6> replay{4, 8, 15, 61, 23, 42};
    const std::array<int, 2> pattern{16, 23};
    out << "first mismatch at index " << firstMismatch(telemetry, replay).value_or(0) << '\n';
    out << "search {16,23} found at " << findSubsequence(telemetry, pattern).value_or(telemetry.size())
        << '\n';
    out << "\"radar\" palindrome? " << isPalindrome("radar") << '\n';
}

void demonstrateModifyingAlgorithms(std::ostream& out) {
    out << "\n=== Modifying Algorithms ===\n";
    auto missions = sampleMissions();
    printRange(out, "ranges::transform ids", extractIds(missions));

    std::vector<int> fuel(5);
    std::ranges::fill(fuel, 100);
    // std::mt19937's output sequence is fully specified, unlike the distributions, so reducing it
    // directly keeps the demo byte-identical across standard libraries.
    std::mt19937 rng(42U);
    std::ranges::generate(fuel, [&rng] { return 100 - static_cast<int>(rng() % 61U); });
    printRange(out, "generate (seeded)", fuel);
    out << "capped " << capValues(fuel, 70) << " values at 70\n";

    std::vector<int> ids{5, 3, 5, 1, 3, 9, 1};
    removeDuplicates(ids);
    printRange(out, "sort+unique+erase", ids);
    printRange(out, "rotate left by 2", rotatedLeft({1, 2, 3, 4, 5}, 2));

    const auto front = partitionByPriority(missions, 8);
    out << "stable_partition put " << front << " urgent missions first:";
    for (const auto& m : missions) {
        out << ' ' << m.id;
    }
    out << '\n';
}

void demonstrateSortingAlgorithms(std::ostream& out) {
    out << "\n=== Sorting Algorithms ===\n";
    auto missions = sampleMissions();
    sortMissionsByUrgency(missions);
    printRange(out, "sort by urgency", extractIds(missions));
    stableSortByType(missions);
    printRange(out, "stable_sort by type (projection)", extractIds(missions));
    printRange(out, "partial_sort_copy top 3", extractIds(topNByPriority(sampleMissions(), 3)));

    const std::vector<int> readings{42, 7, 19, 3, 88, 21, 5, 64};
    out << "nth_element median (rank 4): " << nthSmallest(readings, 4) << '\n';
    printRange(out, "bounded heap k-smallest (k=3)", kSmallest(readings, 3));

    const std::vector<int> sorted{1, 3, 3, 3, 7, 9};
    out << "lower_bound index of 4: " << insertionIndex(sorted, 4)
        << ", equal_range count of 3: " << countInSorted(sorted, 3) << '\n';
}

void demonstrateNumericAlgorithms(std::ostream& out) {
    out << "\n=== Numeric Algorithms ===\n";
    out << "transform_reduce total resource value: " << totalResourceValue(sampleResources()) << '\n';
    const std::vector<int> deltas{3, -1, 4, -1, 5};
    printRange(out, "inclusive_scan", prefixSums(deltas));
    printRange(out, "adjacent_difference", adjacentDifferences(std::vector<int>{1, 4, 9, 16, 25}));
    const std::array<int, 3> thrust{1, 2, 3};
    const std::array<int, 3> weight{4, 5, 6};
    out << "inner_product: " << dotProduct(thrust, weight) << '\n';
    printRange(out, "iota from 10", iotaVector(10, 5));
    constexpr std::array<unsigned long long, 3> orbits{4, 6, 10};
    constexpr auto alignment = lcmOf(orbits);
    static_assert(alignment == 60);
    out << "orbits re-align after lcm = " << alignment << " cycles, gcd(84, 36) = " << std::gcd(84, 36)
        << '\n';
}

void demonstrateSetAlgorithms(std::ostream& out) {
    out << "\n=== Set Algorithms ===\n";
    const std::vector<int> alpha{1, 2, 4, 6, 8};
    const std::vector<int> beta{2, 3, 4, 8, 10};
    printRange(out, "set_union", sortedUnion(alpha, beta));
    printRange(out, "set_intersection", sortedIntersection(alpha, beta));
    printRange(out, "set_difference", sortedDifference(alpha, beta));
    printRange(out, "set_symmetric_difference", sortedSymmetricDifference(alpha, beta));
    printRange(out, "merge", mergeSorted(alpha, beta));
    out << std::boolalpha << "includes {2,8}? " << includesAll(alpha, std::vector<int>{2, 8}) << '\n';
}

void demonstratePermutationAlgorithms(std::ostream& out) {
    out << "\n=== Permutation Algorithms ===\n";
    printRange(out, "next_permutation of \"aab\"", allPermutations("aab"));
    out << std::boolalpha << "is_permutation(\"listen\", \"silent\"): " << isAnagram("listen", "silent")
        << '\n';
    out << "lexicographical_compare(\"alpha\", \"Beta\") ignoring case: "
        << caseInsensitiveLess("alpha", "Beta") << '\n';
}

void runAlgorithmsDemo(std::ostream& out) {
    demonstrateNonModifyingAlgorithms(out);
    demonstrateModifyingAlgorithms(out);
    demonstrateSortingAlgorithms(out);
    demonstrateNumericAlgorithms(out);
    demonstrateSetAlgorithms(out);
    demonstratePermutationAlgorithms(out);
}

} // namespace CppVerseHub::STL
