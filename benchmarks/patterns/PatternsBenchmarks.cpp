/**
 * @file PatternsBenchmarks.cpp
 * @brief Micro-benchmarks quantifying the cost of the pattern implementations.
 *
 * Highlights: classic weak_ptr Subject vs Signal dispatch; virtual (OO) vs std::variant state
 * dispatch; runtime vs compile-time strategy; bounded command history throughput; decorator chain
 * depth; Meyers-singleton access cost.
 */

#include "patterns/Command.hpp"
#include "patterns/Decorator.hpp"
#include "patterns/Observer.hpp"
#include "patterns/Singleton.hpp"
#include "patterns/State.hpp"
#include "patterns/Strategy.hpp"

#include <benchmark/benchmark.h>

#include <memory>
#include <vector>

using namespace CppVerseHub::Patterns;

namespace {

struct CountingObserver final : IObserver<int> {
    long total = 0;
    void onNotify(const int& v) override { total += v; }
};

void BM_SubjectNotify(benchmark::State& state) {
    Subject<int> subject;
    std::vector<std::shared_ptr<CountingObserver>> observers;
    for (int i = 0; i < state.range(0); ++i) {
        observers.push_back(std::make_shared<CountingObserver>());
        subject.attach(observers.back());
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(subject.notify(1));
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_SubjectNotify)->Arg(1)->Arg(16)->Arg(128);

void BM_SignalEmit(benchmark::State& state) {
    Signal<int> signal;
    long total = 0;
    std::vector<ScopedConnection> connections;
    for (int i = 0; i < state.range(0); ++i) {
        connections.push_back(signal.connectScoped([&total](int v) { total += v; }));
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(signal.emit(1));
    }
    benchmark::DoNotOptimize(total);
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_SignalEmit)->Arg(1)->Arg(16)->Arg(128);

void BM_CommandHistoryExecuteUndo(benchmark::State& state) {
    FleetReceiver fleet(FleetStatus{"Bench", "Sol", 1e12, 100.0, 1});
    CommandHistory history(static_cast<std::size_t>(state.range(0)));
    for (auto _ : state) {
        history.execute(std::make_unique<ReinforceCommand>(fleet, 1));
        history.execute(std::make_unique<MoveFleetCommand>(fleet, "Vega", 1.0));
        (void)history.undo();
        (void)history.redo();
    }
    int ships = fleet.status().ships;
    benchmark::DoNotOptimize(ships);
}
BENCHMARK(BM_CommandHistoryExecuteUndo)->Arg(16)->Arg(1024);

void BM_StateOO(benchmark::State& state) {
    for (auto _ : state) {
        MissionContext m("bench");
        m.plan();
        m.launch();
        for (int i = 0; i < 9; ++i) {
            m.advance(10.0);
            m.pause();
            m.resume();
        }
        m.advance(10.0);
        benchmark::DoNotOptimize(m.phase());
    }
}
BENCHMARK(BM_StateOO);

void BM_StateVariant(benchmark::State& state) {
    const std::vector<Warp::Event> cycle{Warp::Charge{50.0}, Warp::Charge{50.0}, Warp::Engage{"Vega"},
                                         Warp::Tick{},       Warp::Tick{},       Warp::Tick{},
                                         Warp::Tick{}};
    WarpDrive drive;
    drive.handle(Warp::PowerOn{});
    for (auto _ : state) {
        for (const auto& e : cycle) {
            benchmark::DoNotOptimize(drive.handle(e));
        }
    }
    state.SetItemsProcessed(state.iterations() * static_cast<long>(cycle.size()));
}
BENCHMARK(BM_StateVariant);

NavigationContext hazards() {
    NavigationContext ctx;
    for (int i = 0; i < 8; ++i) {
        ctx.hazards.push_back({{10.0 + i * 10.0, (i % 2 == 0 ? 1.0 : -1.0), 0.0}, 3.0, 1.0});
    }
    return ctx;
}

void BM_RoutingRuntime(benchmark::State& state) {
    const auto type = static_cast<RoutingStrategyType>(state.range(0));
    FleetRouter router(makeRoutingStrategy(type), hazards());
    for (auto _ : state) {
        benchmark::DoNotOptimize(router.plan({0, 0, 0}, {100, 0, 0}));
    }
    state.SetLabel(std::string(router.strategyName()));
}
BENCHMARK(BM_RoutingRuntime)->DenseRange(0, 3);

void BM_RoutingStaticPolicy(benchmark::State& state) {
    const StaticRouter<DirectLineStrategy> router(DirectLineStrategy{}, hazards());
    for (auto _ : state) {
        benchmark::DoNotOptimize(router.plan({0, 0, 0}, {100, 0, 0}));
    }
}
BENCHMARK(BM_RoutingStaticPolicy);

void BM_DecoratorChain(benchmark::State& state) {
    MissionPtr m = std::make_unique<BasicMission>(MissionKind::Combat, "Bench");
    for (int i = 0; i < state.range(0); ++i) {
        m = decorate<StealthEnhancement>(std::move(m));
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(m->successProbability());
        benchmark::DoNotOptimize(m->cost());
    }
}
BENCHMARK(BM_DecoratorChain)->Arg(1)->Arg(8)->Arg(64);

void BM_MemoizeHit(benchmark::State& state) {
    auto square = memoize<long, int>([](int x) { return static_cast<long>(x) * x; });
    (void)square(7);
    for (auto _ : state) {
        benchmark::DoNotOptimize(square(7));
    }
}
BENCHMARK(BM_MemoizeHit);

void BM_SingletonAccess(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(IdGenerator::instance().next());
    }
}
BENCHMARK(BM_SingletonAccess)->Threads(1)->Threads(4);

} // namespace
