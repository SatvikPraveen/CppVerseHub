// Micro-benchmarks for the stl_showcase module.
#include "stl_showcase/Algorithms.hpp"
#include "stl_showcase/Containers.hpp"
#include "stl_showcase/Functors.hpp"
#include "stl_showcase/Iterators.hpp"
#include "stl_showcase/STLUtilities.hpp"

#include <benchmark/benchmark.h>

#include <algorithm>
#include <cstdint>
#include <functional>
#include <list>
#include <map>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

namespace stl = CppVerseHub::STL;

namespace {

std::vector<int> randomInts(std::size_t n, std::uint32_t seed = 42U) {
    std::mt19937 rng(seed);
    std::vector<int> values(n);
    for (auto& v : values) {
        v = static_cast<int>(rng() % 1'000'000U);
    }
    return values;
}

// --- Containers: contiguous vs node-based traversal -------------------------------------------

void BM_SumVector(benchmark::State& state) {
    const auto data = randomInts(static_cast<std::size_t>(state.range(0)));
    for (auto _ : state) {
        long long sum = 0;
        for (const int v : data) {
            sum += v;
        }
        benchmark::DoNotOptimize(sum);
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_SumVector)->Arg(1 << 10)->Arg(1 << 16);

void BM_SumList(benchmark::State& state) {
    const auto source = randomInts(static_cast<std::size_t>(state.range(0)));
    const std::list<int> data(source.begin(), source.end());
    for (auto _ : state) {
        long long sum = 0;
        for (const int v : data) {
            sum += v;
        }
        benchmark::DoNotOptimize(sum);
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_SumList)->Arg(1 << 10)->Arg(1 << 16);

void BM_SumSimpleVector(benchmark::State& state) {
    const auto source = randomInts(static_cast<std::size_t>(state.range(0)));
    stl::SimpleVector<int> data;
    for (const int v : source) {
        data.push_back(v);
    }
    for (auto _ : state) {
        long long sum = 0;
        for (const int v : data) {
            sum += v;
        }
        benchmark::DoNotOptimize(sum);
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_SumSimpleVector)->Arg(1 << 10)->Arg(1 << 16);

// --- Associative lookup: ordered vs hashed vs sorted vector -----------------------------------

void BM_LookupMap(benchmark::State& state) {
    const auto keys = randomInts(static_cast<std::size_t>(state.range(0)));
    std::map<int, int> map;
    for (const int k : keys) {
        map.emplace(k, k);
    }
    std::size_t i = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(map.find(keys[i++ % keys.size()]));
    }
}
BENCHMARK(BM_LookupMap)->Arg(1 << 12);

void BM_LookupUnorderedMap(benchmark::State& state) {
    const auto keys = randomInts(static_cast<std::size_t>(state.range(0)));
    std::unordered_map<int, int> map;
    for (const int k : keys) {
        map.emplace(k, k);
    }
    std::size_t i = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(map.find(keys[i++ % keys.size()]));
    }
}
BENCHMARK(BM_LookupUnorderedMap)->Arg(1 << 12);

void BM_LookupSortedVector(benchmark::State& state) {
    const auto keys = randomInts(static_cast<std::size_t>(state.range(0)));
    auto sorted = keys;
    std::ranges::sort(sorted);
    std::size_t i = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(stl::insertionIndex(sorted, keys[i++ % keys.size()]));
    }
}
BENCHMARK(BM_LookupSortedVector)->Arg(1 << 12);

void BM_LruCacheMixed(benchmark::State& state) {
    const auto keys = randomInts(4096, 7U);
    stl::LruCache<int, int> cache(static_cast<std::size_t>(state.range(0)));
    std::size_t i = 0;
    for (auto _ : state) {
        const int key = keys[i++ % keys.size()] % 2048;
        if (auto hit = cache.get(key)) {
            benchmark::DoNotOptimize(*hit);
        } else {
            cache.put(key, key);
        }
    }
}
BENCHMARK(BM_LruCacheMixed)->Arg(256)->Arg(1024);

void BM_SlidingWindowMax(benchmark::State& state) {
    const auto data = randomInts(1 << 14);
    const auto window = static_cast<std::size_t>(state.range(0));
    for (auto _ : state) {
        benchmark::DoNotOptimize(stl::slidingWindowMaximum(data, window));
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(data.size()));
}
BENCHMARK(BM_SlidingWindowMax)->Arg(8)->Arg(512);

// --- Algorithms: full sort vs partial selection ------------------------------------------------

void BM_FullSort(benchmark::State& state) {
    const auto data = randomInts(static_cast<std::size_t>(state.range(0)));
    for (auto _ : state) {
        auto copy = data;
        std::ranges::sort(copy);
        benchmark::DoNotOptimize(copy.data());
    }
}
BENCHMARK(BM_FullSort)->Arg(1 << 14);

void BM_NthElement(benchmark::State& state) {
    const auto data = randomInts(static_cast<std::size_t>(state.range(0)));
    for (auto _ : state) {
        benchmark::DoNotOptimize(stl::nthSmallest(data, data.size() / 2));
    }
}
BENCHMARK(BM_NthElement)->Arg(1 << 14);

void BM_KSmallestHeap(benchmark::State& state) {
    const auto data = randomInts(1 << 14);
    const auto k = static_cast<std::size_t>(state.range(0));
    for (auto _ : state) {
        benchmark::DoNotOptimize(stl::kSmallest(data, k));
    }
}
BENCHMARK(BM_KSmallestHeap)->Arg(16)->Arg(1024);

void BM_SetIntersection(benchmark::State& state) {
    auto a = randomInts(static_cast<std::size_t>(state.range(0)), 1U);
    auto b = randomInts(static_cast<std::size_t>(state.range(0)), 2U);
    std::ranges::sort(a);
    std::ranges::sort(b);
    for (auto _ : state) {
        benchmark::DoNotOptimize(stl::sortedIntersection(a, b));
    }
}
BENCHMARK(BM_SetIntersection)->Arg(1 << 12);

// --- Iterators: FilterView vs eager copy_if -----------------------------------------------------

void BM_FilterViewSum(benchmark::State& state) {
    auto data = randomInts(static_cast<std::size_t>(state.range(0)));
    for (auto _ : state) {
        stl::FilterView evens(data, [](int x) { return x % 2 == 0; });
        long long sum = 0;
        for (const int v : evens) {
            sum += v;
        }
        benchmark::DoNotOptimize(sum);
    }
}
BENCHMARK(BM_FilterViewSum)->Arg(1 << 14);

void BM_CopyIfThenSum(benchmark::State& state) {
    const auto data = randomInts(static_cast<std::size_t>(state.range(0)));
    for (auto _ : state) {
        std::vector<int> evens;
        std::ranges::copy_if(data, std::back_inserter(evens), [](int x) { return x % 2 == 0; });
        long long sum = 0;
        for (const int v : evens) {
            sum += v;
        }
        benchmark::DoNotOptimize(sum);
    }
}
BENCHMARK(BM_CopyIfThenSum)->Arg(1 << 14);

// --- Functors: inlinable functor vs type-erased std::function ----------------------------------

void BM_CountIfFunctor(benchmark::State& state) {
    const auto data = randomInts(1 << 14);
    for (auto _ : state) {
        benchmark::DoNotOptimize(std::ranges::count_if(data, [](int x) { return x > 500'000; }));
    }
}
BENCHMARK(BM_CountIfFunctor);

void BM_CountIfStdFunction(benchmark::State& state) {
    const auto data = randomInts(1 << 14);
    const std::function<bool(int)> pred = [](int x) { return x > 500'000; };
    for (auto _ : state) {
        benchmark::DoNotOptimize(std::ranges::count_if(data, pred));
    }
}
BENCHMARK(BM_CountIfStdFunction);

// --- Utilities: variant visitation and command parsing -----------------------------------------

void BM_VisitCommands(benchmark::State& state) {
    const std::vector<stl::Command> commands{stl::MoveCommand{{1, 2, 3}}, stl::AttackCommand{"X", 3},
                                             stl::ScanCommand{5.0}, stl::DockCommand{"Y"}};
    std::size_t i = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(stl::statusAfter(commands[i++ % commands.size()]));
    }
}
BENCHMARK(BM_VisitCommands);

void BM_ParseCommand(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(stl::parseCommand("attack Pirate 7"));
    }
}
BENCHMARK(BM_ParseCommand);

} // namespace
