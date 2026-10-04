// Micro-benchmarks for the templates module: hand-written containers vs. the standard library,
// expression templates vs. eager temporaries, and compile-time vs. runtime computation.
#include "templates/GenericContainers.hpp"
#include "templates/MetaProgramming.hpp"
#include "templates/VariadicTemplates.hpp"

#include <benchmark/benchmark.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace T = CppVerseHub::Templates;

namespace {

// ----- DynamicArray vs std::vector -----

void BM_DynamicArrayPushBack(benchmark::State& state) {
    const auto n = static_cast<int>(state.range(0));
    for (auto _ : state) {
        T::DynamicArray<int> values;
        for (int i = 0; i < n; ++i) {
            values.push_back(i);
        }
        benchmark::DoNotOptimize(values.data());
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_DynamicArrayPushBack)->Arg(1 << 10)->Arg(1 << 16);

void BM_StdVectorPushBack(benchmark::State& state) {
    const auto n = static_cast<int>(state.range(0));
    for (auto _ : state) {
        std::vector<int> values;
        for (int i = 0; i < n; ++i) {
            values.push_back(i);
        }
        benchmark::DoNotOptimize(values.data());
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_StdVectorPushBack)->Arg(1 << 10)->Arg(1 << 16);

void BM_DynamicArrayInsertFront(benchmark::State& state) {
    const auto n = static_cast<int>(state.range(0));
    for (auto _ : state) {
        T::DynamicArray<int> values;
        values.reserve(static_cast<std::size_t>(n));
        for (int i = 0; i < n; ++i) {
            values.insert(values.begin(), i);
        }
        benchmark::DoNotOptimize(values.data());
    }
}
BENCHMARK(BM_DynamicArrayInsertFront)->Arg(256)->Arg(2048);

// ----- smart pointers -----

void BM_SharedPtrCopy(benchmark::State& state) {
    const auto shared = T::make_shared_ptr<int>(42);
    for (auto _ : state) {
        T::SharedPtr<int> copy = shared;
        benchmark::DoNotOptimize(copy.get());
    }
}
BENCHMARK(BM_SharedPtrCopy);

void BM_StdSharedPtrCopy(benchmark::State& state) {
    const auto shared = std::make_shared<int>(42);
    for (auto _ : state) {
        std::shared_ptr<int> copy = shared;
        benchmark::DoNotOptimize(copy.get());
    }
}
BENCHMARK(BM_StdSharedPtrCopy);

void BM_MakeSharedPtr(benchmark::State& state) {
    for (auto _ : state) {
        auto p = T::make_shared_ptr<std::string>("payload");
        benchmark::DoNotOptimize(p.get());
    }
}
BENCHMARK(BM_MakeSharedPtr);

void BM_WeakPtrLock(benchmark::State& state) {
    const auto shared = T::make_shared_ptr<int>(1);
    const T::WeakPtr<int> weak = shared;
    for (auto _ : state) {
        auto locked = weak.lock();
        benchmark::DoNotOptimize(locked.get());
    }
}
BENCHMARK(BM_WeakPtrLock);

// ----- Optional -----

void BM_OptionalTransform(benchmark::State& state) {
    T::Optional<int> value = 21;
    for (auto _ : state) {
        benchmark::DoNotOptimize(value);
        auto doubled = value.transform([](int v) { return v * 2; });
        benchmark::DoNotOptimize(doubled);
    }
}
BENCHMARK(BM_OptionalTransform);

// ----- expression templates vs eager evaluation -----

std::vector<double> make_data(std::size_t n, double seed) {
    std::vector<double> data(n);
    for (std::size_t i = 0; i < n; ++i) {
        data[i] = seed + static_cast<double>(i % 97) * 0.5;
    }
    return data;
}

void BM_ExpressionTemplates(benchmark::State& state) {
    const auto n = static_cast<std::size_t>(state.range(0));
    T::Meta::ExprVector<double> a(n);
    T::Meta::ExprVector<double> b(n);
    T::Meta::ExprVector<double> c(n);
    const auto da = make_data(n, 1.0);
    const auto db = make_data(n, 2.0);
    const auto dc = make_data(n, 3.0);
    for (std::size_t i = 0; i < n; ++i) {
        a[i] = da[i];
        b[i] = db[i];
        c[i] = dc[i];
    }
    T::Meta::ExprVector<double> result(n);
    for (auto _ : state) {
        result = a + b * c - 2.0 * a; // one fused loop, no temporaries
        benchmark::DoNotOptimize(result[0]);
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_ExpressionTemplates)->Arg(1 << 12)->Arg(1 << 16);

std::vector<double> eager_add(const std::vector<double>& x, const std::vector<double>& y) {
    std::vector<double> r(x.size());
    for (std::size_t i = 0; i < x.size(); ++i) {
        r[i] = x[i] + y[i];
    }
    return r;
}
std::vector<double> eager_sub(const std::vector<double>& x, const std::vector<double>& y) {
    std::vector<double> r(x.size());
    for (std::size_t i = 0; i < x.size(); ++i) {
        r[i] = x[i] - y[i];
    }
    return r;
}
std::vector<double> eager_mul(const std::vector<double>& x, const std::vector<double>& y) {
    std::vector<double> r(x.size());
    for (std::size_t i = 0; i < x.size(); ++i) {
        r[i] = x[i] * y[i];
    }
    return r;
}
std::vector<double> eager_scale(double s, const std::vector<double>& x) {
    std::vector<double> r(x.size());
    for (std::size_t i = 0; i < x.size(); ++i) {
        r[i] = s * x[i];
    }
    return r;
}

void BM_EagerTemporaries(benchmark::State& state) {
    const auto n = static_cast<std::size_t>(state.range(0));
    const auto a = make_data(n, 1.0);
    const auto b = make_data(n, 2.0);
    const auto c = make_data(n, 3.0);
    for (auto _ : state) {
        auto result = eager_sub(eager_add(a, eager_mul(b, c)), eager_scale(2.0, a)); // four temporaries
        benchmark::DoNotOptimize(result.data());
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_EagerTemporaries)->Arg(1 << 12)->Arg(1 << 16);

// ----- compile-time vs runtime -----

void BM_RuntimeStringHash(benchmark::State& state) {
    const std::string key = "the quick brown fox jumps over the lazy dog";
    for (auto _ : state) {
        benchmark::DoNotOptimize(key.data());
        benchmark::DoNotOptimize(T::Meta::hash_string(key));
    }
}
BENCHMARK(BM_RuntimeStringHash);

void BM_CompileTimeStringHash(benchmark::State& state) {
    for (auto _ : state) {
        constexpr std::uint64_t folded = T::Meta::hash_string("the quick brown fox jumps over the lazy dog");
        std::uint64_t hash = folded; // the hash was computed by the compiler; only the copy remains
        benchmark::DoNotOptimize(hash);
    }
}
BENCHMARK(BM_CompileTimeStringHash);

void BM_ConstexprMapLookup(benchmark::State& state) {
    static constexpr auto map = T::Meta::make_constexpr_map(
        std::pair{std::string_view("alpha"), 1}, std::pair{std::string_view("beta"), 2},
        std::pair{std::string_view("gamma"), 3}, std::pair{std::string_view("delta"), 4});
    std::string_view key = "delta";
    for (auto _ : state) {
        benchmark::DoNotOptimize(key);
        benchmark::DoNotOptimize(map.find(key));
    }
}
BENCHMARK(BM_ConstexprMapLookup);

// ----- variadic utilities -----

void BM_MemoizedCall(benchmark::State& state) {
    auto square = T::Variadic::memoize<long(int)>([](int v) { return static_cast<long>(v) * v; });
    int i = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(square(i++ & 63)); // 64 distinct keys: mostly cache hits
    }
}
BENCHMARK(BM_MemoizedCall);

void BM_FormatString(benchmark::State& state) {
    for (auto _ : state) {
        auto text = T::Variadic::format_string("{} + {} = {}", 12, 30, 42);
        benchmark::DoNotOptimize(text.data());
    }
}
BENCHMARK(BM_FormatString);

} // namespace
