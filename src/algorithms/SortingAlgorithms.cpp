/**
 * @file SortingAlgorithms.cpp
 * @brief Runtime catalogue, deterministic data generators and the sorting showcase.
 */

#include "algorithms/SortingAlgorithms.hpp"

#include <numeric>
#include <iomanip>
#include <random>
#include <string>

namespace CppVerseHub::Algorithms {

namespace {

constexpr std::array<SortAlgorithmInfo, kAllSortAlgorithms.size()> kSortInfo{{
    {"bubble", true, true, true, "O(n^2)", "O(n^2)", "O(1)"},
    {"cocktail_shaker", true, true, true, "O(n^2)", "O(n^2)", "O(1)"},
    {"insertion", true, true, true, "O(n^2)", "O(n^2)", "O(1)"},
    {"binary_insertion", true, true, true, "O(n^2) moves, O(n log n) cmp", "O(n^2)", "O(1)"},
    {"selection", false, true, true, "O(n^2)", "O(n^2)", "O(1)"},
    {"shell (Ciura)", false, true, false, "~O(n^1.25)", "O(n^(4/3))+", "O(1)"},
    {"heap", false, true, false, "O(n log n)", "O(n log n)", "O(1)"},
    {"quick", false, true, false, "O(n log n)", "O(n^2)", "O(log n)"},
    {"quick_3way", false, true, false, "O(n log n)", "O(n^2)", "O(log n)"},
    {"intro", false, true, false, "O(n log n)", "O(n log n)", "O(log n)"},
    {"merge", true, true, false, "O(n log n)", "O(n log n)", "O(n)"},
    {"bottom_up_merge", true, true, false, "O(n log n)", "O(n log n)", "O(n)"},
    {"tim", true, true, false, "O(n log n)", "O(n log n)", "O(n)"},
    {"parallel_merge", true, true, false, "O(n log n) work", "O(n log n) work", "O(n)"},
    {"counting", true, false, false, "O(n + k)", "O(n + k)", "O(n + k)"},
    {"radix (LSD, base 256)", true, false, false, "O(w(n + 256))", "O(w(n + 256))", "O(n)"},
    {"bucket", true, false, false, "O(n) expected", "O(n^2)", "O(n)"},
}};

template <class Fn>
std::size_t run_counted(const Fn& fn, std::span<int> data) {
    std::atomic<std::size_t> counter{0};
    fn(data, CountingComparator<>{counter});
    return counter.load();
}

}  // namespace

const SortAlgorithmInfo& sort_algorithm_info(SortAlgorithm algorithm) noexcept {
    return kSortInfo[static_cast<std::size_t>(algorithm)];
}

std::size_t sort_ints(SortAlgorithm algorithm, std::span<int> data) {
    switch (algorithm) {
        case SortAlgorithm::Bubble:
            return run_counted(bubble_sort, data);
        case SortAlgorithm::CocktailShaker:
            return run_counted(cocktail_shaker_sort, data);
        case SortAlgorithm::Insertion:
            return run_counted(insertion_sort, data);
        case SortAlgorithm::BinaryInsertion:
            return run_counted(binary_insertion_sort, data);
        case SortAlgorithm::Selection:
            return run_counted(selection_sort, data);
        case SortAlgorithm::Shell:
            return run_counted(shell_sort, data);
        case SortAlgorithm::Heap:
            return run_counted(heap_sort, data);
        case SortAlgorithm::Quick:
            return run_counted(quick_sort, data);
        case SortAlgorithm::Quick3Way:
            return run_counted(quick_sort_3way, data);
        case SortAlgorithm::Intro:
            return run_counted(intro_sort, data);
        case SortAlgorithm::Merge:
            return run_counted(merge_sort, data);
        case SortAlgorithm::BottomUpMerge:
            return run_counted(bottom_up_merge_sort, data);
        case SortAlgorithm::Tim:
            return run_counted(tim_sort, data);
        case SortAlgorithm::ParallelMerge:
            return run_counted(parallel_merge_sort, data);
        case SortAlgorithm::Counting:
            counting_sort(data);
            return 0;
        case SortAlgorithm::Radix:
            radix_sort(data);
            return 0;
        case SortAlgorithm::Bucket:
            bucket_sort(data, [](int v) { return static_cast<double>(v); });
            return 0;
    }
    return 0;
}

std::string_view to_string(DataPattern pattern) noexcept {
    switch (pattern) {
        case DataPattern::Random:
            return "random";
        case DataPattern::Sorted:
            return "sorted";
        case DataPattern::ReverseSorted:
            return "reverse_sorted";
        case DataPattern::NearlySorted:
            return "nearly_sorted";
        case DataPattern::FewUnique:
            return "few_unique";
        case DataPattern::Sawtooth:
            return "sawtooth";
        case DataPattern::OrganPipe:
            return "organ_pipe";
        case DataPattern::AllEqual:
            return "all_equal";
    }
    return "unknown";
}

std::vector<int> generate_data(std::size_t size, DataPattern pattern, std::uint64_t seed) {
    std::mt19937_64 rng(seed);
    std::vector<int> v(size);
    const int n = static_cast<int>(std::min<std::size_t>(size, static_cast<std::size_t>(INT32_MAX / 2)));
    switch (pattern) {
        case DataPattern::Random: {
            std::uniform_int_distribution<int> dist(-1'000'000, 1'000'000);
            for (int& x : v) {
                x = dist(rng);
            }
            break;
        }
        case DataPattern::Sorted:
            std::iota(v.begin(), v.end(), 0);
            break;
        case DataPattern::ReverseSorted:
            for (std::size_t i = 0; i < size; ++i) {
                v[i] = n - static_cast<int>(i);
            }
            break;
        case DataPattern::NearlySorted: {
            std::iota(v.begin(), v.end(), 0);
            if (size > 1) {
                std::uniform_int_distribution<std::size_t> idx(0, size - 1);
                for (std::size_t s = 0; s < size / 20 + 1; ++s) {
                    std::swap(v[idx(rng)], v[idx(rng)]);
                }
            }
            break;
        }
        case DataPattern::FewUnique: {
            std::uniform_int_distribution<int> dist(0, 7);
            for (int& x : v) {
                x = dist(rng);
            }
            break;
        }
        case DataPattern::Sawtooth: {
            const std::size_t tooth = std::max<std::size_t>(1, size / 8);
            for (std::size_t i = 0; i < size; ++i) {
                v[i] = static_cast<int>(i % tooth);
            }
            break;
        }
        case DataPattern::OrganPipe:
            for (std::size_t i = 0; i < size; ++i) {
                v[i] = static_cast<int>(std::min(i, size - 1 - i));
            }
            break;
        case DataPattern::AllEqual:
            std::fill(v.begin(), v.end(), 7);
            break;
    }
    return v;
}

void demonstrate_sorting(std::ostream& out) {
    out << "=== Sorting algorithms (n = 512, comparisons counted) ===\n";
    constexpr std::size_t n = 512;
    const std::array<DataPattern, 4> patterns{DataPattern::Random, DataPattern::Sorted, DataPattern::ReverseSorted,
                                              DataPattern::FewUnique};
    out << std::left << std::setw(24) << "algorithm" << std::setw(8) << "stable";
    for (DataPattern p : patterns) {
        out << std::right << std::setw(16) << to_string(p);
    }
    out << "   avg / worst\n";
    for (SortAlgorithm a : kAllSortAlgorithms) {
        const auto& info = sort_algorithm_info(a);
        out << std::left << std::setw(24) << info.name << std::setw(8) << (info.stable ? "yes" : "no");
        bool all_ok = true;
        for (DataPattern p : patterns) {
            auto data = generate_data(n, p, 7);
            const std::size_t cmps = sort_ints(a, data);
            all_ok = all_ok && std::ranges::is_sorted(data);
            out << std::right << std::setw(16) << (info.comparison_based ? std::to_string(cmps) : std::string("-"));
        }
        out << "   " << info.average_time << " / " << info.worst_time << (all_ok ? "" : "  [FAILED]") << '\n';
    }

    // Stability illustrated: sort records by key only, observe tie order.
    struct Record {
        int key;
        char tag;
    };
    std::vector<Record> records{{3, 'a'}, {1, 'b'}, {3, 'c'}, {2, 'd'}, {1, 'e'}, {3, 'f'}};
    auto print_records = [&out](std::string_view label, const std::vector<Record>& rs) {
        out << label;
        for (const auto& r : rs) {
            out << ' ' << r.key << r.tag;
        }
        out << '\n';
    };
    print_records("records (key,tag):     ", records);
    auto stable_copy = records;
    merge_sort(stable_copy, {}, &Record::key);
    print_records("merge_sort by key:     ", stable_copy);
    auto radix_copy = records;
    radix_sort(radix_copy, &Record::key);
    print_records("radix_sort by key:     ", radix_copy);
    auto desc = records;
    tim_sort(desc, std::ranges::greater{}, &Record::key);
    print_records("tim_sort descending:   ", desc);
}

}  // namespace CppVerseHub::Algorithms
