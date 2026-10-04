// Tests for stl_showcase/Algorithms.hpp
#include "stl_showcase/Algorithms.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <algorithm>
#include <array>
#include <forward_list>
#include <list>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace CppVerseHub::STL;
using Catch::Approx;

TEST_CASE("Mission streaming and sample data", "[algorithms][data]") {
    const auto missions = sampleMissions();
    REQUIRE(missions.size() == 6);
    std::ostringstream os;
    os << missions[0];
    REQUIRE(os.str() == "M001 (Exploration, p3, 24h, 80%)");
    REQUIRE(Resource{"x", 4, 2.5, ""}.totalValue() == Approx(10.0));
}

TEST_CASE("findMissionById, countMissionsOfType and allMissionsFeasible", "[algorithms][non-modifying]") {
    const auto missions = sampleMissions();
    const auto found = findMissionById(missions, "M004");
    REQUIRE(found.has_value());
    REQUIRE(found->assigned_fleet == "Strike Force");
    REQUIRE_FALSE(findMissionById(missions, "M999").has_value());
    REQUIRE(countMissionsOfType(missions, "Combat") == 2);
    REQUIRE(countMissionsOfType(missions, "Mining") == 0);
    REQUIRE(allMissionsFeasible(missions, 0.5));
    REQUIRE_FALSE(allMissionsFeasible(missions, 0.6));
    REQUIRE(allMissionsFeasible({}, 1.0));
}

TEST_CASE("computeMissionStats aggregates correctly", "[algorithms][non-modifying]") {
    const auto stats = computeMissionStats(sampleMissions());
    REQUIRE(stats.count == 6);
    REQUIRE(stats.total_hours == Approx(102.0));
    REQUIRE(stats.mean_hours == Approx(17.0));
    REQUIRE(stats.min_priority == 2);
    REQUIRE(stats.max_priority == 10);
    REQUIRE(stats.expected_successes == Approx(4.45));

    const auto empty = computeMissionStats({});
    REQUIRE(empty.count == 0);
    REQUIRE(empty.mean_hours == 0.0);
}

TEST_CASE("firstMismatch, findSubsequence and isPalindrome", "[algorithms][non-modifying]") {
    const std::vector<int> a{1, 2, 3, 4};
    REQUIRE_FALSE(firstMismatch(a, a).has_value());
    REQUIRE(firstMismatch(a, std::vector<int>{1, 2, 9, 4}) == 2U);
    REQUIRE(firstMismatch(a, std::vector<int>{1, 2}) == 2U);
    REQUIRE(firstMismatch(std::vector<int>{}, a) == 0U);

    REQUIRE(findSubsequence(a, std::vector<int>{3, 4}) == 2U);
    REQUIRE(findSubsequence(a, std::vector<int>{}) == 0U);
    REQUIRE_FALSE(findSubsequence(a, std::vector<int>{4, 5}).has_value());

    REQUIRE(isPalindrome(""));
    REQUIRE(isPalindrome("a"));
    REQUIRE(isPalindrome("abba"));
    REQUIRE(isPalindrome("racecar"));
    REQUIRE_FALSE(isPalindrome("rocket"));
}

TEST_CASE("removeDuplicates sorts and deduplicates", "[algorithms][modifying]") {
    std::vector<int> v{5, 1, 5, 3, 1, 1};
    removeDuplicates(v);
    REQUIRE(v == std::vector<int>{1, 3, 5});
    std::vector<std::string> s{"b", "a", "b"};
    removeDuplicates(s);
    REQUIRE(s == std::vector<std::string>{"a", "b"});
    std::vector<int> empty;
    removeDuplicates(empty);
    REQUIRE(empty.empty());
}

TEST_CASE("extractIds, rotatedLeft and capValues", "[algorithms][modifying]") {
    REQUIRE(extractIds(sampleMissions()).front() == "M001");
    REQUIRE(extractIds(sampleMissions()).size() == 6);

    const std::vector<int> base{1, 2, 3, 4, 5};
    REQUIRE(rotatedLeft(base, 2) == std::vector<int>{3, 4, 5, 1, 2});
    REQUIRE(rotatedLeft(base, 7) == std::vector<int>{3, 4, 5, 1, 2});
    REQUIRE(rotatedLeft(base, -1) == std::vector<int>{5, 1, 2, 3, 4});
    REQUIRE(rotatedLeft(base, 0) == base);
    REQUIRE(rotatedLeft({}, 3).empty());

    std::vector<int> values{10, 80, 30, 95};
    REQUIRE(capValues(values, 50) == 2);
    REQUIRE(values == std::vector<int>{10, 50, 30, 50});
}

TEST_CASE("partitionByPriority is stable", "[algorithms][modifying]") {
    auto missions = sampleMissions();
    const auto front = partitionByPriority(missions, 8);
    REQUIRE(front == 3);
    REQUIRE(extractIds(missions) == std::vector<std::string>{"M002", "M004", "M006", "M001", "M003", "M005"});
}

TEST_CASE("sortMissionsByUrgency uses priority, then duration, then id", "[algorithms][sorting]") {
    auto missions = sampleMissions();
    missions.push_back({"M007", "Combat", 9, 8.0, 0.5, ""});  // ties with M004 on priority+duration
    missions.push_back({"M000", "Combat", 9, 20.0, 0.5, ""}); // same priority, longer
    sortMissionsByUrgency(missions);
    REQUIRE(extractIds(missions) ==
            std::vector<std::string>{"M006", "M004", "M007", "M000", "M002", "M005", "M001", "M003"});
}

TEST_CASE("stableSortByType keeps input order within a type", "[algorithms][sorting]") {
    auto missions = sampleMissions();
    stableSortByType(missions);
    REQUIRE(extractIds(missions) == std::vector<std::string>{"M002", "M004", "M005", "M001", "M006", "M003"});
}

TEST_CASE("topNByPriority returns the most urgent missions", "[algorithms][sorting]") {
    const auto missions = sampleMissions();
    REQUIRE(extractIds(topNByPriority(missions, 2)) == std::vector<std::string>{"M006", "M004"});
    REQUIRE(topNByPriority(missions, 100).size() == missions.size());
    REQUIRE(topNByPriority(missions, 0).empty());
}

TEST_CASE("nthSmallest agrees with full sort on random data", "[algorithms][sorting]") {
    std::mt19937 rng(1234U);
    std::vector<int> data(101);
    for (auto& x : data) {
        x = static_cast<int>(rng() % 1000U);
    }
    auto sorted = data;
    std::ranges::sort(sorted);
    const std::size_t rank = GENERATE(0U, 1U, 50U, 100U);
    REQUIRE(nthSmallest(data, rank) == sorted[rank]);
    REQUIRE_THROWS_AS(nthSmallest(data, 101), std::out_of_range);
}

TEST_CASE("kSmallest bounded heap", "[algorithms][heap]") {
    const std::vector<int> data{9, 4, 7, 1, 8, 2, 2, 6};
    REQUIRE(kSmallest(data, 3) == std::vector<int>{1, 2, 2});
    REQUIRE(kSmallest(data, 0).empty());
    REQUIRE(kSmallest(data, 50).size() == data.size());
    const std::forward_list<double> unsized{3.5, -1.0, 2.0}; // not a sized_range
    REQUIRE(kSmallest(unsized, 2) == std::vector<double>{-1.0, 2.0});
}

TEST_CASE("binary search helpers", "[algorithms][sorting]") {
    const std::vector<int> sorted{1, 3, 3, 3, 7, 9};
    REQUIRE(insertionIndex(sorted, 0) == 0);
    REQUIRE(insertionIndex(sorted, 3) == 1);
    REQUIRE(insertionIndex(sorted, 4) == 4);
    REQUIRE(insertionIndex(sorted, 10) == 6);
    REQUIRE(countInSorted(sorted, 3) == 3);
    REQUIRE(countInSorted(sorted, 5) == 0);
}

TEST_CASE("numeric algorithms", "[algorithms][numeric]") {
    REQUIRE(totalResourceValue(sampleResources()) == Approx(6000.0 + 2000.0 + 3600.0 + 4000.0));
    REQUIRE(totalResourceValue({}) == 0.0);
    REQUIRE(prefixSums(std::vector<int>{3, -1, 4}) == std::vector<long long>{3, 2, 6});
    const std::vector<int> big{2'000'000'000, 2'000'000'000};
    REQUIRE(prefixSums(big).back() == 4'000'000'000LL);
    REQUIRE(adjacentDifferences(std::vector<int>{1, 4, 9, 16}) == std::vector<int>{1, 3, 5, 7});
    REQUIRE(dotProduct(std::vector<int>{1, 2, 3}, std::vector<int>{4, 5, 6}) == 32);
    REQUIRE_THROWS_AS(dotProduct(std::vector<int>{1}, std::vector<int>{}), std::invalid_argument);
    REQUIRE(iotaVector(-2, 4) == std::vector<int>{-2, -1, 0, 1});
}

TEST_CASE("lcmOf is constexpr", "[algorithms][numeric]") {
    static constexpr std::array<unsigned long long, 3> periods{4, 6, 10};
    STATIC_REQUIRE(lcmOf(periods) == 60);
    REQUIRE(lcmOf({}) == 1);
}

TEST_CASE("set operations on sorted ranges", "[algorithms][set]") {
    const std::vector<int> a{1, 2, 4, 6, 8};
    const std::vector<int> b{2, 3, 4, 8, 10};
    REQUIRE(sortedUnion(a, b) == std::vector<int>{1, 2, 3, 4, 6, 8, 10});
    REQUIRE(sortedIntersection(a, b) == std::vector<int>{2, 4, 8});
    REQUIRE(sortedDifference(a, b) == std::vector<int>{1, 6});
    REQUIRE(sortedSymmetricDifference(a, b) == std::vector<int>{1, 3, 6, 10});
    REQUIRE(mergeSorted(a, b) == std::vector<int>{1, 2, 2, 3, 4, 4, 6, 8, 8, 10});
    REQUIRE(includesAll(a, std::vector<int>{2, 8}));
    REQUIRE_FALSE(includesAll(a, std::vector<int>{2, 3}));
    REQUIRE(includesAll(a, std::vector<int>{}));
    const std::list<std::string> names{"ant", "cat"};
    const std::vector<std::string> more{"bee", "cat"};
    REQUIRE(sortedUnion(names, more) == std::vector<std::string>{"ant", "bee", "cat"});
}

TEST_CASE("permutations and lexicographic comparison", "[algorithms][permutations]") {
    REQUIRE(allPermutations("cba") == std::vector<std::string>{"abc", "acb", "bac", "bca", "cab", "cba"});
    REQUIRE(allPermutations("aab") == std::vector<std::string>{"aab", "aba", "baa"});
    REQUIRE(allPermutations("") == std::vector<std::string>{""});
    REQUIRE(allPermutations("abcd").size() == 24);
    REQUIRE(isAnagram("listen", "silent"));
    REQUIRE_FALSE(isAnagram("listen", "listens"));
    REQUIRE(caseInsensitiveLess("alpha", "Beta"));
    REQUIRE_FALSE(caseInsensitiveLess("Beta", "alpha"));
    REQUIRE_FALSE(caseInsensitiveLess("ABC", "abc"));
    REQUIRE(caseInsensitiveLess("ab", "abc"));
}
