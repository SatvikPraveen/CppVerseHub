// Property-based and behavioural tests for algorithms/SortingAlgorithms.hpp.

#include "algorithms/SortingAlgorithms.hpp"

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <array>
#include <atomic>
#include <cmath>
#include <deque>
#include <list>
#include <memory>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace CppVerseHub::Algorithms;

namespace {

constexpr std::array<std::size_t, 14> kSizes{0, 1, 2, 3, 4, 5, 8, 15, 16, 17, 33, 64, 129, 600};

// Small-key records for stability checks: many duplicates by construction.
struct Rec {
    int key;
    std::size_t original;
    friend bool operator==(const Rec&, const Rec&) = default;
};

std::vector<Rec> make_records(std::size_t n, int key_range, std::uint64_t seed) {
    std::mt19937_64 rng(seed);
    std::uniform_int_distribution<int> dist(-key_range, key_range);
    std::vector<Rec> v(n);
    for (std::size_t i = 0; i < n; ++i) {
        v[i] = Rec{dist(rng), i};
    }
    return v;
}

template <class Fn>
bool is_quadratic() {
    return std::is_same_v<Fn, BubbleSortFn> || std::is_same_v<Fn, CocktailShakerSortFn> ||
           std::is_same_v<Fn, InsertionSortFn> || std::is_same_v<Fn, BinaryInsertionSortFn> ||
           std::is_same_v<Fn, SelectionSortFn>;
}

}  // namespace

#define ALL_COMPARISON_SORTS                                                                                  \
    BubbleSortFn, CocktailShakerSortFn, InsertionSortFn, BinaryInsertionSortFn, SelectionSortFn, ShellSortFn, \
        HeapSortFn, QuickSortFn, QuickSort3WayFn, IntroSortFn, MergeSortFn, BottomUpMergeSortFn, TimSortFn,  \
        ParallelMergeSortFn

#define STABLE_COMPARISON_SORTS                                                                       \
    BubbleSortFn, CocktailShakerSortFn, InsertionSortFn, BinaryInsertionSortFn, MergeSortFn,         \
        BottomUpMergeSortFn, TimSortFn, ParallelMergeSortFn

// ---------------------------------------------------------------------------------------------
// Property: every comparison sort agrees with std::sort
// ---------------------------------------------------------------------------------------------

TEMPLATE_TEST_CASE("comparison sorts agree with std::sort on every pattern, size and seed", "[sorting][property]",
                   ALL_COMPARISON_SORTS) {
    const TestType sorter{};
    for (DataPattern pattern : kAllDataPatterns) {
        for (std::size_t n : kSizes) {
            if (n > 200 && is_quadratic<TestType>()) {
                continue;
            }
            for (std::uint64_t seed = 0; seed < 6; ++seed) {
                auto data = generate_data(n, pattern, seed);
                auto expected = data;
                std::sort(expected.begin(), expected.end());
                auto it = sorter(data.begin(), data.end());
                REQUIRE(it == data.end());
                REQUIRE(data == expected);
            }
        }
    }
}

TEMPLATE_TEST_CASE("comparison sorts honour custom comparators and projections", "[sorting]",
                   ALL_COMPARISON_SORTS) {
    const TestType sorter{};
    SECTION("descending with std::ranges::greater") {
        auto data = generate_data(150, DataPattern::Random, 11);
        auto expected = data;
        std::sort(expected.begin(), expected.end(), std::greater<>{});
        sorter(data, std::ranges::greater{});
        REQUIRE(data == expected);
    }
    SECTION("projection onto a member orders by that member") {
        auto recs = make_records(120, 50, 3);
        sorter(recs, {}, &Rec::key);
        REQUIRE(std::ranges::is_sorted(recs, {}, &Rec::key));
        REQUIRE(std::ranges::is_permutation(recs, make_records(120, 50, 3)));
    }
    SECTION("strings sort lexicographically") {
        std::vector<std::string> words{"pear", "apple", "fig", "banana", "apple", "", "cherry", "date"};
        auto expected = words;
        std::sort(expected.begin(), expected.end());
        sorter(words);
        REQUIRE(words == expected);
    }
    SECTION("works on a non-contiguous random-access container (std::deque)") {
        const auto src = generate_data(100, DataPattern::Random, 5);
        std::deque<int> dq(src.begin(), src.end());
        sorter(dq);
        REQUIRE(std::ranges::is_sorted(dq));
    }
}

TEMPLATE_TEST_CASE("stable sorts preserve the order of equal keys", "[sorting][stability][property]",
                   STABLE_COMPARISON_SORTS) {
    const TestType sorter{};
    for (std::size_t n : kSizes) {
        if (n > 200 && is_quadratic<TestType>()) {
            continue;
        }
        for (int key_range : {0, 1, 3, 20}) {
            for (std::uint64_t seed = 0; seed < 4; ++seed) {
                auto recs = make_records(n, key_range, seed);
                auto expected = recs;
                std::stable_sort(expected.begin(), expected.end(),
                                 [](const Rec& a, const Rec& b) { return a.key < b.key; });
                sorter(recs, {}, &Rec::key);
                REQUIRE(recs == expected);
            }
        }
    }
}

TEMPLATE_TEST_CASE("comparison sorts handle move-only element types", "[sorting][move-only]", BubbleSortFn,
                   CocktailShakerSortFn, InsertionSortFn, BinaryInsertionSortFn, SelectionSortFn, ShellSortFn,
                   HeapSortFn, QuickSortFn, IntroSortFn, MergeSortFn, BottomUpMergeSortFn, TimSortFn,
                   ParallelMergeSortFn) {
    const TestType sorter{};
    auto values = generate_data(90, DataPattern::FewUnique, 2);
    std::vector<std::unique_ptr<int>> ptrs;
    for (int v : values) {
        ptrs.push_back(std::make_unique<int>(v));
    }
    sorter(ptrs, {}, [](const std::unique_ptr<int>& p) { return *p; });
    std::sort(values.begin(), values.end());
    REQUIRE(ptrs.size() == values.size());
    for (std::size_t i = 0; i < values.size(); ++i) {
        REQUIRE(ptrs[i] != nullptr);
        REQUIRE(*ptrs[i] == values[i]);
    }
}

TEST_CASE("sort function objects are constrained by concepts", "[sorting][concepts]") {
    STATIC_REQUIRE(std::invocable<const MergeSortFn&, std::vector<int>&>);
    STATIC_REQUIRE(std::invocable<const MergeSortFn&, std::vector<int>&, std::ranges::greater>);
    STATIC_REQUIRE_FALSE(std::invocable<const MergeSortFn&, std::list<int>&>);  // not random access
    STATIC_REQUIRE_FALSE(std::invocable<const QuickSortFn&, const std::vector<int>&>);  // not permutable
    STATIC_REQUIRE_FALSE(std::invocable<const QuickSort3WayFn&, std::vector<std::unique_ptr<int>>&>);
    STATIC_REQUIRE(std::invocable<const RadixSortFn&, std::vector<long long>&>);
    STATIC_REQUIRE_FALSE(std::invocable<const RadixSortFn&, std::vector<double>&>);
    STATIC_REQUIRE_FALSE(std::invocable<const RadixSortFn&, std::vector<bool>&>);
    STATIC_REQUIRE_FALSE(std::invocable<const BucketSortFn&, std::vector<int>&>);
    STATIC_REQUIRE(std::invocable<const BucketSortFn&, std::vector<float>&>);
}

TEST_CASE("sorting is usable in constant expressions", "[sorting][constexpr]") {
    constexpr auto sorted = [] {
        std::array<int, 7> a{5, -1, 3, 3, 9, 0, 2};
        intro_sort(a);
        return a;
    }();
    STATIC_REQUIRE(sorted == std::array<int, 7>{-1, 0, 2, 3, 3, 5, 9});
    constexpr auto shell = [] {
        std::array<int, 5> a{4, 3, 2, 1, 0};
        insertion_sort(a, std::ranges::greater{});
        return a;
    }();
    STATIC_REQUIRE(shell == std::array<int, 5>{4, 3, 2, 1, 0});
}

// ---------------------------------------------------------------------------------------------
// Distribution sorts
// ---------------------------------------------------------------------------------------------

TEST_CASE("counting sort matches std::sort and is stable", "[sorting][counting][property]") {
    for (std::size_t n : kSizes) {
        for (std::uint64_t seed = 0; seed < 5; ++seed) {
            auto data = generate_data(n, DataPattern::FewUnique, seed);
            for (int& x : data) {
                x -= 4;  // include negatives
            }
            auto expected = data;
            std::sort(expected.begin(), expected.end());
            counting_sort(data);
            REQUIRE(data == expected);

            auto recs = make_records(n, 6, seed);
            auto stable = recs;
            std::stable_sort(stable.begin(), stable.end(), [](const Rec& a, const Rec& b) { return a.key < b.key; });
            counting_sort(recs, &Rec::key);
            REQUIRE(recs == stable);
        }
    }
}

TEST_CASE("counting sort rejects key ranges above the limit", "[sorting][counting]") {
    std::vector<int> wide{0, 1'000'000};
    REQUIRE_THROWS_AS(counting_sort(wide, std::identity{}, 1000), std::length_error);
    std::vector<int> extremes{std::numeric_limits<int>::max(), std::numeric_limits<int>::min()};
    REQUIRE_THROWS_AS(counting_sort(extremes), std::length_error);
    std::vector<char> chars{'z', 'a', 'm', 'a'};
    counting_sort(chars);
    REQUIRE(chars == std::vector<char>{'a', 'a', 'm', 'z'});
}

TEST_CASE("radix sort handles signed extremes, all widths and is stable", "[sorting][radix][property]") {
    SECTION("int with full range including INT_MIN/INT_MAX") {
        for (std::uint64_t seed = 0; seed < 8; ++seed) {
            std::mt19937_64 rng(seed);
            std::uniform_int_distribution<int> dist(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
            std::vector<int> data(500);
            for (int& x : data) {
                x = dist(rng);
            }
            data.push_back(std::numeric_limits<int>::min());
            data.push_back(std::numeric_limits<int>::max());
            data.push_back(0);
            data.push_back(-1);
            auto expected = data;
            std::sort(expected.begin(), expected.end());
            radix_sort(data);
            REQUIRE(data == expected);
        }
    }
    SECTION("64-bit, unsigned and 8-bit keys") {
        std::vector<std::int64_t> big{INT64_MIN, 5, -5, INT64_MAX, 0, -1, 1LL << 40, -(1LL << 40)};
        auto big_expected = big;
        std::sort(big_expected.begin(), big_expected.end());
        radix_sort(big);
        REQUIRE(big == big_expected);

        std::vector<std::uint32_t> u{4000000000U, 1U, 0U, 77U, 4000000000U, 65536U};
        auto u_expected = u;
        std::sort(u_expected.begin(), u_expected.end());
        radix_sort(u);
        REQUIRE(u == u_expected);

        std::vector<std::int8_t> small{-128, 127, 0, -1, 1, -128};
        radix_sort(small);
        REQUIRE(std::ranges::is_sorted(small));
    }
    SECTION("stability through a projection") {
        for (std::size_t n : kSizes) {
            auto recs = make_records(n, 4, n);
            auto expected = recs;
            std::stable_sort(expected.begin(), expected.end(),
                             [](const Rec& a, const Rec& b) { return a.key < b.key; });
            radix_sort(recs, &Rec::key);
            REQUIRE(recs == expected);
        }
    }
}

TEST_CASE("bucket sort orders floating-point keys stably", "[sorting][bucket][property]") {
    for (std::uint64_t seed = 0; seed < 6; ++seed) {
        std::mt19937_64 rng(seed);
        std::uniform_real_distribution<double> uni(-50.0, 50.0);
        std::exponential_distribution<double> skew(0.5);
        std::vector<double> data(400);
        for (std::size_t i = 0; i < data.size(); ++i) {
            data[i] = (i % 2 == 0) ? uni(rng) : skew(rng);
        }
        auto expected = data;
        std::sort(expected.begin(), expected.end());
        bucket_sort(data);
        REQUIRE(data == expected);
    }
    std::vector<double> equal(10, 3.5);
    bucket_sort(equal);
    REQUIRE(equal == std::vector<double>(10, 3.5));
    std::vector<double> empty;
    bucket_sort(empty);
    REQUIRE(empty.empty());

    auto recs = make_records(300, 5, 9);
    auto expected = recs;
    std::stable_sort(expected.begin(), expected.end(), [](const Rec& a, const Rec& b) { return a.key < b.key; });
    bucket_sort(recs, [](const Rec& r) { return static_cast<double>(r.key); });
    REQUIRE(recs == expected);
}

// ---------------------------------------------------------------------------------------------
// Complexity witnesses (comparison counts)
// ---------------------------------------------------------------------------------------------

TEST_CASE("adaptive sorts use n-1 comparisons on sorted input", "[sorting][complexity]") {
    const std::size_t n = GENERATE(1u, 2u, 17u, 1000u, 1024u);
    auto data = generate_data(n, DataPattern::Sorted);
    for (SortAlgorithm a : {SortAlgorithm::Insertion, SortAlgorithm::Merge, SortAlgorithm::Tim,
                            SortAlgorithm::Bubble}) {
        auto copy = data;
        CAPTURE(sort_algorithm_info(a).name, n);
        REQUIRE(sort_ints(a, copy) == n - 1);
    }
}

TEST_CASE("comparison counts respect the documented asymptotic bounds", "[sorting][complexity]") {
    const std::size_t n = 2048;
    const double nlogn = static_cast<double>(n) * std::log2(static_cast<double>(n));
    auto random = generate_data(n, DataPattern::Random, 99);

    auto sel = random;
    REQUIRE(sort_ints(SortAlgorithm::Selection, sel) == n * (n - 1) / 2);

    for (SortAlgorithm a : {SortAlgorithm::Heap, SortAlgorithm::Merge, SortAlgorithm::BottomUpMerge,
                            SortAlgorithm::Tim, SortAlgorithm::Intro, SortAlgorithm::Quick,
                            SortAlgorithm::Quick3Way}) {
        auto copy = random;
        CAPTURE(sort_algorithm_info(a).name);
        const auto cmps = static_cast<double>(sort_ints(a, copy));
        REQUIRE(cmps <= 2.5 * nlogn);
        REQUIRE(std::ranges::is_sorted(copy));
    }
    // Three-way quicksort is linear on all-equal input; plain insertion of equal keys likewise.
    auto equal = generate_data(n, DataPattern::AllEqual);
    REQUIRE(sort_ints(SortAlgorithm::Quick3Way, equal) <= 3 * n);
    // Distribution sorts make no comparator calls.
    auto copy = random;
    REQUIRE(sort_ints(SortAlgorithm::Radix, copy) == 0);
    REQUIRE(std::ranges::is_sorted(copy));
}

TEST_CASE("introsort stays O(n log n) where plain quicksort would degrade", "[sorting][complexity]") {
    // Organ-pipe / sawtooth inputs are classic stressors for median-of-three pivots.
    const std::size_t n = 4096;
    const double nlogn = static_cast<double>(n) * std::log2(static_cast<double>(n));
    for (DataPattern p : {DataPattern::OrganPipe, DataPattern::Sawtooth, DataPattern::ReverseSorted}) {
        auto data = generate_data(n, p);
        CAPTURE(to_string(p));
        REQUIRE(static_cast<double>(sort_ints(SortAlgorithm::Intro, data)) <= 4.0 * nlogn);
        REQUIRE(std::ranges::is_sorted(data));
    }
}

// ---------------------------------------------------------------------------------------------
// Catalogue, generators, parallel sort, misc
// ---------------------------------------------------------------------------------------------

TEST_CASE("sort_ints dispatches every algorithm correctly", "[sorting][catalogue]") {
    for (SortAlgorithm a : kAllSortAlgorithms) {
        const auto& info = sort_algorithm_info(a);
        CAPTURE(info.name);
        for (DataPattern p : kAllDataPatterns) {
            auto data = generate_data(info.quadratic ? 200 : 3000, p, 4);
            auto expected = data;
            std::sort(expected.begin(), expected.end());
            const std::size_t cmps = sort_ints(a, data);
            REQUIRE(data == expected);
            if (!info.comparison_based) {
                REQUIRE(cmps == 0);
            }
        }
    }
}

TEST_CASE("sort catalogue metadata is consistent", "[sorting][catalogue]") {
    std::vector<std::string_view> names;
    for (SortAlgorithm a : kAllSortAlgorithms) {
        names.push_back(sort_algorithm_info(a).name);
    }
    auto sorted_names = names;
    std::sort(sorted_names.begin(), sorted_names.end());
    REQUIRE(std::adjacent_find(sorted_names.begin(), sorted_names.end()) == sorted_names.end());
    REQUIRE(sort_algorithm_info(SortAlgorithm::Merge).stable);
    REQUIRE_FALSE(sort_algorithm_info(SortAlgorithm::Quick).stable);
    REQUIRE_FALSE(sort_algorithm_info(SortAlgorithm::Radix).comparison_based);
    REQUIRE(sort_algorithm_info(SortAlgorithm::Heap).worst_time == "O(n log n)");
}

TEST_CASE("generate_data is deterministic and produces the requested shapes", "[sorting][data]") {
    REQUIRE(generate_data(100, DataPattern::Random, 1) == generate_data(100, DataPattern::Random, 1));
    REQUIRE(generate_data(100, DataPattern::Random, 1) != generate_data(100, DataPattern::Random, 2));
    REQUIRE(std::ranges::is_sorted(generate_data(50, DataPattern::Sorted)));
    REQUIRE(std::ranges::is_sorted(generate_data(50, DataPattern::ReverseSorted), std::greater<>{}));
    const auto eq = generate_data(20, DataPattern::AllEqual);
    REQUIRE(std::ranges::all_of(eq, [&](int v) { return v == eq.front(); }));
    const auto few = generate_data(500, DataPattern::FewUnique);
    REQUIRE(std::ranges::all_of(few, [](int v) { return v >= 0 && v <= 7; }));
    REQUIRE(generate_data(0, DataPattern::NearlySorted).empty());
    REQUIRE(to_string(DataPattern::OrganPipe) == "organ_pipe");
}

TEST_CASE("parallel merge sort matches std::stable_sort on large inputs", "[sorting][parallel]") {
    auto recs = make_records(200'000, 1000, 77);
    auto expected = recs;
    std::stable_sort(expected.begin(), expected.end(), [](const Rec& a, const Rec& b) { return a.key < b.key; });
    parallel_merge_sort(recs, {}, &Rec::key);
    REQUIRE(recs == expected);

    // Comparison counting is race-free across worker threads.
    auto data = generate_data(100'000, DataPattern::Random, 5);
    REQUIRE(sort_ints(SortAlgorithm::ParallelMerge, data) > 0);
    REQUIRE(std::ranges::is_sorted(data));
}

TEST_CASE("exceptions from the comparator propagate out of the sort", "[sorting][exceptions]") {
    auto data = generate_data(50'000, DataPattern::Random, 3);
    int calls = 0;
    auto throwing = [&calls](int a, int b) {
        if (++calls == 1000) {
            throw std::runtime_error("comparator failure");
        }
        return a < b;
    };
    REQUIRE_THROWS_AS(merge_sort(data, throwing), std::runtime_error);
    calls = 0;
    REQUIRE_THROWS_AS(quick_sort(data, throwing), std::runtime_error);
    std::atomic<int> pcalls{0};
    auto pthrowing = [&pcalls](int a, int b) {
        if (pcalls.fetch_add(1) == 5000) {
            throw std::runtime_error("parallel comparator failure");
        }
        return a < b;
    };
    REQUIRE_THROWS_AS(parallel_merge_sort(data, pthrowing), std::runtime_error);
}

TEST_CASE("Timsort minimum run length follows the reference definition", "[sorting][tim]") {
    REQUIRE(TimSortFn::min_run_length(0) == 0);
    REQUIRE(TimSortFn::min_run_length(63) == 63);
    REQUIRE(TimSortFn::min_run_length(64) == 32);
    REQUIRE(TimSortFn::min_run_length(65) == 33);
    REQUIRE(TimSortFn::min_run_length(1 << 20) == 32);
    for (std::ptrdiff_t n = 64; n < 5000; n += 37) {
        const auto r = TimSortFn::min_run_length(n);
        REQUIRE(r >= 32);
        REQUIRE(r <= 64);
    }
}

TEST_CASE("CountingComparator counts every call and forwards the result", "[sorting][instrumentation]") {
    std::atomic<std::size_t> counter{0};
    const CountingComparator<std::greater<>> cmp(counter);
    REQUIRE(cmp(3, 2));
    REQUIRE_FALSE(cmp(2, 3));
    REQUIRE(counter.load() == 2);
    std::vector<int> v{1, 2, 3};
    insertion_sort(v, cmp);
    REQUIRE(v == std::vector<int>{3, 2, 1});
    REQUIRE(counter.load() > 2);
}

TEST_CASE("sorting demo prints every algorithm without failures", "[sorting][demo]") {
    std::ostringstream oss;
    demonstrate_sorting(oss);
    const std::string s = oss.str();
    for (SortAlgorithm a : kAllSortAlgorithms) {
        REQUIRE(s.find(std::string(sort_algorithm_info(a).name)) != std::string::npos);
    }
    REQUIRE(s.find("FAILED") == std::string::npos);
    REQUIRE(s.find("1b 1e 2d 3a 3c 3f") != std::string::npos);  // stable order of ties
}
