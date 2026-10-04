// Micro-benchmarks for the core simulation domain.
#include <benchmark/benchmark.h>

#include <cstdint>
#include <vector>

#include "core/EventSystem.hpp"
#include "core/Random.hpp"
#include "core/ResourceManager.hpp"
#include "core/Scenario.hpp"
#include "core/Vector3D.hpp"

using namespace CppVerseHub::Core;

namespace {

struct Tick {
    std::uint64_t n;
};

void BM_Vector3DDistanceSum(benchmark::State& state) {
    DeterministicRng rng(1);
    std::vector<Vector3D> points(static_cast<std::size_t>(state.range(0)));
    for (auto& p : points) {
        p = {rng.uniform(-1, 1), rng.uniform(-1, 1), rng.uniform(-1, 1)};
    }
    for (auto _ : state) {
        double total = 0.0;
        for (std::size_t i = 1; i < points.size(); ++i) {
            total += points[i].distanceTo(points[i - 1]);
        }
        benchmark::DoNotOptimize(total);
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_Vector3DDistanceSum)->Arg(1024)->Arg(16384);

void BM_RngUniformInt(benchmark::State& state) {
    DeterministicRng rng(7);
    for (auto _ : state) {
        benchmark::DoNotOptimize(rng.uniformInt(0, 999));
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_RngUniformInt);

void BM_EventBusPublish(benchmark::State& state) {
    EventBus bus;
    std::uint64_t sink = 0;
    std::vector<Subscription> subs;
    for (int i = 0; i < state.range(0); ++i) {
        subs.push_back(bus.subscribe<Tick>([&sink](const Tick& t) { sink += t.n; }));
    }
    std::uint64_t n = 0;
    for (auto _ : state) {
        bus.publish(Tick{++n});
    }
    benchmark::DoNotOptimize(sink);
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_EventBusPublish)->Arg(1)->Arg(16);

void BM_ResourceTransfer(benchmark::State& state) {
    ResourceManager rm;
    constexpr std::uint64_t kAccounts = 64;
    for (std::uint64_t i = 1; i <= kAccounts; ++i) {
        rm.openAccount(EntityId{i});
        rm.deposit(EntityId{i}, ResourceType::Minerals, 1'000'000);
    }
    DeterministicRng rng(3);
    for (auto _ : state) {
        const EntityId from{1 + rng.below(kAccounts)};
        const EntityId to{1 + rng.below(kAccounts)};
        rm.transfer(from, to, ResourceType::Minerals, 1);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ResourceTransfer);

void BM_ResourceTick(benchmark::State& state) {
    ResourceManager rm;
    for (std::uint64_t i = 1; i <= static_cast<std::uint64_t>(state.range(0)); ++i) {
        rm.openAccount(EntityId{i});
        rm.setProductionRate(EntityId{i}, ResourceType::Energy, 3.3);
        rm.setConsumptionRate(EntityId{i}, ResourceType::Energy, 2.1);
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(rm.tick(0.1));
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_ResourceTick)->Arg(64)->Arg(1024);

void BM_SimulationStep(benchmark::State& state) {
    SampleScenarioOptions options;
    options.seed = 5;
    options.planets = static_cast<std::size_t>(state.range(0));
    options.fleets = static_cast<std::size_t>(state.range(0)) / 2;
    auto engine = makeSampleScenario(options);
    for (auto _ : state) {
        engine->step();
    }
    state.SetItemsProcessed(state.iterations());
    state.counters["entities"] = static_cast<double>(engine->galaxy().entityCount());
}
BENCHMARK(BM_SimulationStep)->Arg(16)->Arg(128);

void BM_ScenarioSaveLoad(benchmark::State& state) {
    SampleScenarioOptions options;
    options.planets = 64;
    options.fleets = 32;
    auto engine = makeSampleScenario(options);
    engine->runSteps(20);
    for (auto _ : state) {
        auto restored = loadScenario(saveScenario(*engine));
        benchmark::DoNotOptimize(restored->tick());
    }
}
BENCHMARK(BM_ScenarioSaveLoad)->Unit(benchmark::kMillisecond);

} // namespace
