/**
 * @file ModernBenchmarks.cpp
 * @brief Micro-benchmarks for the `modern` module: copy vs move, container growth strategies,
 *        ranges pipelines vs raw loops, std::function vs template callables, and constexpr tables.
 */
#include <benchmark/benchmark.h>

#include <cmath>
#include <functional>
#include <string>
#include <vector>

#include "modern/ConstexprProgramming.hpp"
#include "modern/LambdaExpressions.hpp"
#include "modern/MoveSemantics.hpp"
#include "modern/RangesDemo.hpp"

namespace {

using namespace CppVerseHub::Modern;

std::vector<MoveSemantics::TrackedResource> makeResources(std::size_t n, std::size_t payload) {
    std::vector<MoveSemantics::TrackedResource> v;
    v.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        v.emplace_back("res-" + std::to_string(i), payload);
    }
    return v;
}

void BM_CopyTrackedResources(benchmark::State& state) {
    const auto source = makeResources(static_cast<std::size_t>(state.range(0)), 256);
    for (auto _ : state) {
        auto copy = source;
        benchmark::DoNotOptimize(copy.data());
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_CopyTrackedResources)->Arg(64)->Arg(512);

void BM_MoveTrackedResources(benchmark::State& state) {
    auto source = makeResources(static_cast<std::size_t>(state.range(0)), 256);
    std::vector<MoveSemantics::TrackedResource> target;
    target.reserve(source.size());
    for (auto _ : state) {
        target.clear();
        for (auto& r : source) {
            target.push_back(std::move(r));
        }
        benchmark::DoNotOptimize(target.data());
        source.swap(target);  // move everything back for the next iteration
        target.clear();
        for (auto& r : source) {
            target.push_back(std::move(r));
        }
        source.swap(target);
    }
    state.SetItemsProcessed(state.iterations() * state.range(0) * 2);
}
BENCHMARK(BM_MoveTrackedResources)->Arg(64)->Arg(512);

void BM_MoveAwareVectorPushBack(benchmark::State& state) {
    for (auto _ : state) {
        MoveSemantics::MoveAwareVector<std::string> v;
        for (int i = 0; i < state.range(0); ++i) {
            v.emplace_back(32, 'x');
        }
        benchmark::DoNotOptimize(v.begin());
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_MoveAwareVectorPushBack)->Arg(1024);

void BM_StdVectorPushBack(benchmark::State& state) {
    for (auto _ : state) {
        std::vector<std::string> v;
        for (int i = 0; i < state.range(0); ++i) {
            v.emplace_back(32, 'x');
        }
        benchmark::DoNotOptimize(v.data());
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_StdVectorPushBack)->Arg(1024);

void BM_RangesPipeline(benchmark::State& state) {
    const auto data = Ranges::makeRandomValues(static_cast<std::size_t>(state.range(0)), 42);
    for (auto _ : state) {
        benchmark::DoNotOptimize(Ranges::sumSquaresOfEvensRanges(data));
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_RangesPipeline)->Arg(1 << 14);

void BM_RawLoop(benchmark::State& state) {
    const auto data = Ranges::makeRandomValues(static_cast<std::size_t>(state.range(0)), 42);
    for (auto _ : state) {
        benchmark::DoNotOptimize(Ranges::sumSquaresOfEvensLoop(data));
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_RawLoop)->Arg(1 << 14);

void BM_EveryNthView(benchmark::State& state) {
    const auto data = Ranges::makeRandomValues(1 << 14, 7);
    for (auto _ : state) {
        long long sum = 0;
        for (int v : data | Ranges::everyNth(state.range(0))) {
            sum += v;
        }
        benchmark::DoNotOptimize(sum);
    }
}
BENCHMARK(BM_EveryNthView)->Arg(2)->Arg(16);

void BM_StdFunctionCall(benchmark::State& state) {
    const std::function<int(int)> f = [](int x) { return x * 3 + 1; };
    int acc = 0;
    for (auto _ : state) {
        for (int i = 0; i < 1000; ++i) {
            acc = f(acc + i);
        }
        benchmark::DoNotOptimize(acc);
    }
}
BENCHMARK(BM_StdFunctionCall);

void BM_TemplateLambdaCall(benchmark::State& state) {
    auto f = [](int x) { return x * 3 + 1; };
    int acc = 0;
    for (auto _ : state) {
        for (int i = 0; i < 1000; ++i) {
            acc = f(acc + i);
            benchmark::ClobberMemory();
        }
        benchmark::DoNotOptimize(acc);
    }
}
BENCHMARK(BM_TemplateLambdaCall);

void BM_MemoizedVsDirect(benchmark::State& state) {
    LambdaExpressions::Memoized<int, double> memo([](const int& n) {
        double s = 0.0;
        for (int i = 1; i <= 2000; ++i) {
            s += std::sqrt(static_cast<double>(i * n));
        }
        return s;
    });
    for (auto _ : state) {
        double total = 0.0;
        for (int n = 0; n < 64; ++n) {
            total += memo(n);
        }
        benchmark::DoNotOptimize(total);
    }
}
BENCHMARK(BM_MemoizedVsDirect);

void BM_ParallelSum(benchmark::State& state) {
    std::vector<int> data(1 << 18, 1);
    for (auto _ : state) {
        benchmark::DoNotOptimize(LambdaExpressions::parallelSum(data, static_cast<std::size_t>(state.range(0))));
    }
}
BENCHMARK(BM_ParallelSum)->Arg(1)->Arg(4);

void BM_ConstexprTableLookup(benchmark::State& state) {
    std::size_t i = 0;
    for (auto _ : state) {
        double value = ConstexprProgramming::SINE_TABLE[i % ConstexprProgramming::SINE_TABLE.size()];
        benchmark::DoNotOptimize(value);
        ++i;
    }
}
BENCHMARK(BM_ConstexprTableLookup);

void BM_RuntimeSinTaylor(benchmark::State& state) {
    double x = 0.1;
    for (auto _ : state) {
        benchmark::DoNotOptimize(ConstexprProgramming::sinTaylor(x));
        x += 0.001;
    }
}
BENCHMARK(BM_RuntimeSinTaylor);

}  // namespace
