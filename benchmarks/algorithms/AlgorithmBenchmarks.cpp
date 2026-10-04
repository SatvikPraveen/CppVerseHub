// Google Benchmark suite for the algorithms module.
//
// Every benchmark is parameterised over input size (Range / RangeMultiplier) and reports
// SetComplexityN so Google Benchmark fits the empirical complexity (BigO / RMS rows), which can be
// compared with the bounds documented in the headers. Sorting benchmarks copy the input inside the
// timed loop (an O(n) cost that does not change the fitted O(n log n) / O(n^2) class) to avoid the
// large overhead of PauseTiming/ResumeTiming. The many cheap sort/search instances use a shorter
// per-benchmark MinTime so the whole suite stays well under 20 s even in Debug builds.

#include "algorithms/DataStructures.hpp"
#include "algorithms/GraphAlgorithms.hpp"
#include "algorithms/SearchAlgorithms.hpp"
#include "algorithms/SortingAlgorithms.hpp"

#include <benchmark/benchmark.h>

#include <algorithm>
#include <cmath>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

namespace algo = CppVerseHub::Algorithms;

namespace {

// ---------------------------------------------------------------------------------------------
// Sorting
// ---------------------------------------------------------------------------------------------

template <class Sorter>
void BM_Sort(benchmark::State& state) {
    const auto n = static_cast<std::size_t>(state.range(0));
    const auto input = algo::generate_data(n, algo::DataPattern::Random, 42);
    const Sorter sorter{};
    for (auto _ : state) {
        auto data = input;
        sorter(data);
        benchmark::DoNotOptimize(data.data());
        benchmark::ClobberMemory();
    }
    state.SetComplexityN(state.range(0));
    state.SetItemsProcessed(state.iterations() * state.range(0));
}

struct StdSortFn {
    void operator()(std::vector<int>& v) const { std::sort(v.begin(), v.end()); }
};
struct StdStableSortFn {
    void operator()(std::vector<int>& v) const { std::stable_sort(v.begin(), v.end()); }
};
struct BucketIntFn {
    void operator()(std::vector<int>& v) const {
        algo::bucket_sort(v, [](int x) { return static_cast<double>(x); });
    }
};

#define NLOGN_SORT(Fn) \
    BENCHMARK_TEMPLATE(BM_Sort, Fn)->RangeMultiplier(8)->Range(1 << 9, 1 << 15)->MinTime(0.02)->Complexity(benchmark::oNLogN)
#define QUADRATIC_SORT(Fn) \
    BENCHMARK_TEMPLATE(BM_Sort, Fn)->RangeMultiplier(4)->Range(1 << 6, 1 << 10)->MinTime(0.02)->Complexity(benchmark::oNSquared)
#define LINEAR_SORT(Fn) \
    BENCHMARK_TEMPLATE(BM_Sort, Fn)->RangeMultiplier(8)->Range(1 << 9, 1 << 15)->MinTime(0.02)->Complexity(benchmark::oN)

NLOGN_SORT(StdSortFn);
NLOGN_SORT(StdStableSortFn);
NLOGN_SORT(algo::IntroSortFn);
NLOGN_SORT(algo::QuickSortFn);
NLOGN_SORT(algo::QuickSort3WayFn);
NLOGN_SORT(algo::HeapSortFn);
NLOGN_SORT(algo::MergeSortFn);
NLOGN_SORT(algo::BottomUpMergeSortFn);
NLOGN_SORT(algo::TimSortFn);
NLOGN_SORT(algo::ShellSortFn);
QUADRATIC_SORT(algo::InsertionSortFn);
QUADRATIC_SORT(algo::SelectionSortFn);
QUADRATIC_SORT(algo::BubbleSortFn);
LINEAR_SORT(algo::RadixSortFn);
LINEAR_SORT(BucketIntFn);

void BM_CountingSortFewUnique(benchmark::State& state) {
    const auto input = algo::generate_data(static_cast<std::size_t>(state.range(0)), algo::DataPattern::FewUnique);
    for (auto _ : state) {
        auto data = input;
        algo::counting_sort(data);
        benchmark::DoNotOptimize(data.data());
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_CountingSortFewUnique)->RangeMultiplier(8)->Range(1 << 9, 1 << 15)->Complexity(benchmark::oN);

// Adaptivity: Timsort / merge sort on already sorted input are linear.
template <class Sorter>
void BM_SortPresorted(benchmark::State& state) {
    const auto input = algo::generate_data(static_cast<std::size_t>(state.range(0)), algo::DataPattern::Sorted);
    const Sorter sorter{};
    for (auto _ : state) {
        auto data = input;
        sorter(data);
        benchmark::DoNotOptimize(data.data());
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK_TEMPLATE(BM_SortPresorted, algo::TimSortFn)->RangeMultiplier(8)->Range(1 << 9, 1 << 15)->Complexity(benchmark::oN);
BENCHMARK_TEMPLATE(BM_SortPresorted, algo::IntroSortFn)
    ->RangeMultiplier(8)
    ->Range(1 << 9, 1 << 15)
    ->Complexity(benchmark::oNLogN);

void BM_ParallelMergeSort(benchmark::State& state) {
    const auto input = algo::generate_data(static_cast<std::size_t>(state.range(0)), algo::DataPattern::Random);
    for (auto _ : state) {
        auto data = input;
        algo::parallel_merge_sort(data);
        benchmark::DoNotOptimize(data.data());
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_ParallelMergeSort)->RangeMultiplier(4)->Range(1 << 14, 1 << 16)->UseRealTime()->Complexity(benchmark::oNLogN);

// ---------------------------------------------------------------------------------------------
// Searching
// ---------------------------------------------------------------------------------------------

std::vector<int> sorted_input(std::size_t n) {
    std::vector<int> v(n);
    for (std::size_t i = 0; i < n; ++i) {
        v[i] = static_cast<int>(3 * i);
    }
    return v;
}

template <class Search>
void BM_SortedSearch(benchmark::State& state) {
    const auto n = static_cast<std::size_t>(state.range(0));
    const auto data = sorted_input(n);
    std::mt19937_64 rng(1);
    std::uniform_int_distribution<int> target(0, static_cast<int>(3 * n));
    const Search search{};
    for (auto _ : state) {
        benchmark::DoNotOptimize(search(data, target(rng)));
    }
    state.SetComplexityN(state.range(0));
}

#define SEARCH_BENCH(Fn, Big) \
    BENCHMARK_TEMPLATE(BM_SortedSearch, Fn)->RangeMultiplier(16)->Range(1 << 8, 1 << 20)->MinTime(0.02)->Complexity(Big)

SEARCH_BENCH(algo::LowerBoundFn, benchmark::oLogN);
SEARCH_BENCH(algo::ExponentialSearchFn, benchmark::oLogN);
SEARCH_BENCH(algo::InterpolationSearchFn, benchmark::oAuto);  // O(log log n) on uniform data
SEARCH_BENCH(algo::JumpSearchFn, [](benchmark::IterationCount n) { return std::sqrt(static_cast<double>(n)); });
BENCHMARK_TEMPLATE(BM_SortedSearch, algo::LinearSearchFn)
    ->RangeMultiplier(16)
    ->Range(1 << 8, 1 << 16)
    ->Complexity(benchmark::oN);

using StringMatcher = std::vector<std::size_t> (*)(std::string_view, std::string_view);

void BM_StringSearch(benchmark::State& state, StringMatcher matcher) {
    std::mt19937_64 rng(3);
    std::string text(static_cast<std::size_t>(state.range(0)), 'a');
    for (char& c : text) {
        c = static_cast<char>('a' + rng() % 4);
    }
    const std::string pattern = "abcdabcdab";
    for (auto _ : state) {
        benchmark::DoNotOptimize(matcher(text, pattern));
    }
    state.SetComplexityN(state.range(0));
    state.SetBytesProcessed(state.iterations() * state.range(0));
}
std::vector<std::size_t> kmp_adapter(std::string_view t, std::string_view p) { return algo::kmp_search(t, p); }

BENCHMARK_CAPTURE(BM_StringSearch, naive, &algo::naive_search)
    ->RangeMultiplier(8)->Range(1 << 12, 1 << 15)->Complexity(benchmark::oN);
BENCHMARK_CAPTURE(BM_StringSearch, kmp, &kmp_adapter)->RangeMultiplier(8)->Range(1 << 12, 1 << 15)->Complexity(benchmark::oN);
BENCHMARK_CAPTURE(BM_StringSearch, horspool, &algo::boyer_moore_horspool_search)
    ->RangeMultiplier(8)->Range(1 << 12, 1 << 15)->Complexity(benchmark::oN);
BENCHMARK_CAPTURE(BM_StringSearch, rabin_karp, &algo::rabin_karp_search)
    ->RangeMultiplier(8)->Range(1 << 12, 1 << 15)->Complexity(benchmark::oN);
BENCHMARK_CAPTURE(BM_StringSearch, z_algorithm, &algo::z_search)
    ->RangeMultiplier(8)->Range(1 << 12, 1 << 15)->Complexity(benchmark::oN);

void BM_AhoCorasick(benchmark::State& state) {
    std::mt19937_64 rng(4);
    std::string text(static_cast<std::size_t>(state.range(0)), 'a');
    for (char& c : text) {
        c = static_cast<char>('a' + rng() % 4);
    }
    const algo::AhoCorasick ac({"abc", "bcd", "dab", "cabd", "aaaa", "dcba", "bb"});
    for (auto _ : state) {
        benchmark::DoNotOptimize(ac.find_all(text));
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_AhoCorasick)->RangeMultiplier(8)->Range(1 << 12, 1 << 15)->Complexity(benchmark::oN);

void BM_KDTreeNearest(benchmark::State& state) {
    using Tree = algo::KDTree<double, 3>;
    std::mt19937_64 rng(5);
    std::uniform_real_distribution<double> c(0.0, 1.0);
    std::vector<Tree::Point> pts(static_cast<std::size_t>(state.range(0)));
    for (auto& p : pts) {
        p = {c(rng), c(rng), c(rng)};
    }
    const Tree tree(pts);
    for (auto _ : state) {
        benchmark::DoNotOptimize(tree.nearest({c(rng), c(rng), c(rng)}, 4));
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_KDTreeNearest)->RangeMultiplier(8)->Range(1 << 9, 1 << 15)->Complexity(benchmark::oLogN);

// ---------------------------------------------------------------------------------------------
// Graphs (sparse random graphs with E = 4V)
// ---------------------------------------------------------------------------------------------

algo::WeightedGraph<long long> sparse_graph(std::size_t n, bool directed) {
    std::mt19937_64 rng(6);
    algo::WeightedGraph<long long> g(n, directed);
    for (std::size_t i = 0; i < 4 * n; ++i) {
        g.add_edge(rng() % n, rng() % n, static_cast<long long>(1 + rng() % 100));
    }
    for (std::size_t v = 1; v < n; ++v) {
        g.add_edge(v - 1, v, 1000);  // keep it connected
    }
    return g;
}

void BM_BFS(benchmark::State& state) {
    const auto g = sparse_graph(static_cast<std::size_t>(state.range(0)), false);
    for (auto _ : state) {
        benchmark::DoNotOptimize(algo::bfs(g, 0));
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_BFS)->RangeMultiplier(8)->Range(1 << 9, 1 << 15)->Complexity(benchmark::oN);

void BM_Dijkstra(benchmark::State& state) {
    const auto g = sparse_graph(static_cast<std::size_t>(state.range(0)), true);
    for (auto _ : state) {
        benchmark::DoNotOptimize(algo::dijkstra(g, 0));
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_Dijkstra)->RangeMultiplier(8)->Range(1 << 9, 1 << 15)->Complexity(benchmark::oNLogN);

void BM_BellmanFord(benchmark::State& state) {
    const auto g = sparse_graph(static_cast<std::size_t>(state.range(0)), true);
    for (auto _ : state) {
        benchmark::DoNotOptimize(algo::bellman_ford(g, 0));
    }
    state.SetComplexityN(state.range(0));
}
// Early termination makes random graphs far faster than the O(VE) worst case; let the fit decide.
BENCHMARK(BM_BellmanFord)->RangeMultiplier(4)->Range(1 << 7, 1 << 11)->Complexity(benchmark::oAuto);

void BM_FloydWarshall(benchmark::State& state) {
    const auto g = sparse_graph(static_cast<std::size_t>(state.range(0)), true);
    for (auto _ : state) {
        benchmark::DoNotOptimize(algo::floyd_warshall(g));
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_FloydWarshall)->RangeMultiplier(2)->Range(1 << 5, 1 << 7)->Complexity(benchmark::oNCubed);

void BM_AStarGrid(benchmark::State& state) {
    const auto side = static_cast<std::size_t>(state.range(0));
    algo::WeightedGraph<long long> g(side * side);
    for (std::size_t y = 0; y < side; ++y) {
        for (std::size_t x = 0; x < side; ++x) {
            if (x + 1 < side) {
                g.add_edge(y * side + x, y * side + x + 1, 1);
            }
            if (y + 1 < side) {
                g.add_edge(y * side + x, (y + 1) * side + x, 1);
            }
        }
    }
    const std::size_t goal = side * side - 1;
    auto h = [side](std::size_t v) {
        return static_cast<long long>((side - 1 - v % side) + (side - 1 - v / side));
    };
    for (auto _ : state) {
        benchmark::DoNotOptimize(algo::a_star(g, 0, goal, h));
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_AStarGrid)->RangeMultiplier(2)->Range(16, 64)->Complexity(benchmark::oAuto);

template <bool UseKruskal>
void BM_MST(benchmark::State& state) {
    const auto g = sparse_graph(static_cast<std::size_t>(state.range(0)), false);
    for (auto _ : state) {
        if constexpr (UseKruskal) {
            benchmark::DoNotOptimize(algo::kruskal_mst(g));
        } else {
            benchmark::DoNotOptimize(algo::prim_mst(g));
        }
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK_TEMPLATE(BM_MST, true)->Name("BM_Kruskal")->RangeMultiplier(8)->Range(1 << 9, 1 << 15)->Complexity(benchmark::oNLogN);
BENCHMARK_TEMPLATE(BM_MST, false)->Name("BM_Prim")->RangeMultiplier(8)->Range(1 << 9, 1 << 15)->Complexity(benchmark::oNLogN);

void BM_TarjanSCC(benchmark::State& state) {
    const auto g = sparse_graph(static_cast<std::size_t>(state.range(0)), true);
    for (auto _ : state) {
        benchmark::DoNotOptimize(algo::strongly_connected_components(g));
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_TarjanSCC)->RangeMultiplier(8)->Range(1 << 9, 1 << 15)->Complexity(benchmark::oN);

void BM_EdmondsKarp(benchmark::State& state) {
    const auto n = static_cast<std::size_t>(state.range(0));
    std::mt19937_64 rng(7);
    algo::FlowNetwork<long long> f(n);
    for (std::size_t i = 0; i < 4 * n; ++i) {
        f.add_edge(rng() % n, rng() % n, static_cast<long long>(1 + rng() % 50));
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(f.max_flow(0, n - 1));
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_EdmondsKarp)->RangeMultiplier(4)->Range(1 << 6, 1 << 9)->Complexity(benchmark::oAuto);

void BM_HeldKarp(benchmark::State& state) {
    const auto n = static_cast<std::size_t>(state.range(0));
    std::mt19937_64 rng(8);
    std::vector<std::vector<int>> d(n, std::vector<int>(n));
    for (auto& row : d) {
        for (int& x : row) {
            x = static_cast<int>(1 + rng() % 100);
        }
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(algo::held_karp_tsp(d));
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_HeldKarp)->DenseRange(6, 12, 3)->Complexity([](benchmark::IterationCount n) {
    return std::ldexp(static_cast<double>(n * n), static_cast<int>(n));  // n^2 2^n
});

// ---------------------------------------------------------------------------------------------
// Data structures
// ---------------------------------------------------------------------------------------------

std::vector<int> random_keys(std::size_t n) {
    std::mt19937_64 rng(9);
    std::vector<int> v(n);
    for (int& x : v) {
        x = static_cast<int>(rng() % (4 * n + 1));
    }
    return v;
}

void BM_HashTableInsertFind(benchmark::State& state) {
    const auto keys = random_keys(static_cast<std::size_t>(state.range(0)));
    for (auto _ : state) {
        algo::HashTable<int, int> h;
        for (int k : keys) {
            h.insert_or_assign(k, k);
        }
        std::size_t hits = 0;
        for (int k : keys) {
            hits += h.contains(k + 1) ? 1U : 0U;
        }
        benchmark::DoNotOptimize(hits);
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_HashTableInsertFind)->RangeMultiplier(8)->Range(1 << 9, 1 << 15)->Complexity(benchmark::oN);

void BM_StdUnorderedMapInsertFind(benchmark::State& state) {
    const auto keys = random_keys(static_cast<std::size_t>(state.range(0)));
    for (auto _ : state) {
        std::unordered_map<int, int> h;
        for (int k : keys) {
            h.insert_or_assign(k, k);
        }
        std::size_t hits = 0;
        for (int k : keys) {
            hits += h.contains(k + 1) ? 1U : 0U;
        }
        benchmark::DoNotOptimize(hits);
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_StdUnorderedMapInsertFind)->RangeMultiplier(8)->Range(1 << 9, 1 << 15)->Complexity(benchmark::oN);

template <class Set>
void BM_OrderedSetInsert(benchmark::State& state) {
    const auto keys = random_keys(static_cast<std::size_t>(state.range(0)));
    for (auto _ : state) {
        Set s;
        for (int k : keys) {
            s.insert(k);
        }
        benchmark::DoNotOptimize(s.size());
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK_TEMPLATE(BM_OrderedSetInsert, algo::BinarySearchTree<int>)
    ->RangeMultiplier(8)->Range(1 << 9, 1 << 15)->Complexity(benchmark::oNLogN);
BENCHMARK_TEMPLATE(BM_OrderedSetInsert, algo::SkipList<int>)
    ->RangeMultiplier(8)->Range(1 << 9, 1 << 15)->Complexity(benchmark::oNLogN);

void BM_MinHeapPushPop(benchmark::State& state) {
    const auto keys = random_keys(static_cast<std::size_t>(state.range(0)));
    for (auto _ : state) {
        algo::MinHeap<int> h;
        for (int k : keys) {
            h.push(k);
        }
        long long sum = 0;
        while (!h.empty()) {
            sum += h.pop();
        }
        benchmark::DoNotOptimize(sum);
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_MinHeapPushPop)->RangeMultiplier(8)->Range(1 << 9, 1 << 15)->Complexity(benchmark::oNLogN);

void BM_DynamicArrayPushBack(benchmark::State& state) {
    for (auto _ : state) {
        algo::DynamicArray<int> a;
        for (int i = 0; i < state.range(0); ++i) {
            a.push_back(i);
        }
        benchmark::DoNotOptimize(a.data());
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_DynamicArrayPushBack)->RangeMultiplier(8)->Range(1 << 9, 1 << 15)->Complexity(benchmark::oN);

void BM_UnionFind(benchmark::State& state) {
    const auto n = static_cast<std::size_t>(state.range(0));
    std::mt19937_64 rng(10);
    std::vector<std::pair<std::size_t, std::size_t>> ops(2 * n);
    for (auto& [a, b] : ops) {
        a = rng() % n;
        b = rng() % n;
    }
    for (auto _ : state) {
        algo::DisjointSet dsu(n);
        for (const auto& [a, b] : ops) {
            dsu.unite(a, b);
        }
        benchmark::DoNotOptimize(dsu.set_count());
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_UnionFind)->RangeMultiplier(8)->Range(1 << 9, 1 << 15)->Complexity(benchmark::oN);

void BM_BloomFilterQuery(benchmark::State& state) {
    const auto n = static_cast<std::size_t>(state.range(0));
    algo::BloomFilter f(n, 0.01);
    for (std::size_t i = 0; i < n; ++i) {
        f.insert("key" + std::to_string(i));
    }
    const std::string probe = "key" + std::to_string(n / 2);
    for (auto _ : state) {
        benchmark::DoNotOptimize(f.might_contain(probe));
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_BloomFilterQuery)->RangeMultiplier(8)->Range(1 << 9, 1 << 15)->Complexity(benchmark::o1);

void BM_TrieInsert(benchmark::State& state) {
    std::mt19937_64 rng(11);
    std::vector<std::string> words(static_cast<std::size_t>(state.range(0)));
    for (auto& w : words) {
        w.resize(4 + rng() % 8);
        for (char& c : w) {
            c = static_cast<char>('a' + rng() % 26);
        }
    }
    for (auto _ : state) {
        algo::Trie t;
        for (const auto& w : words) {
            t.insert(w);
        }
        benchmark::DoNotOptimize(t.size());
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_TrieInsert)->RangeMultiplier(8)->Range(1 << 9, 1 << 13)->Complexity(benchmark::oN);

}  // namespace
