#include "core/Events.hpp"
#include "core/ExplorationMission.hpp"
#include "core/Scenario.hpp"
#include "core/SimulationEngine.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <vector>

using namespace CppVerseHub::Core;
using Catch::Approx;

namespace {

SampleScenarioOptions options(std::uint64_t seed) {
    SampleScenarioOptions o;
    o.seed = seed;
    o.planets = 10;
    o.fleets = 6;
    return o;
}

/// Record every mission outcome so trajectories can be compared event by event.
struct Recorder {
    std::vector<std::string> log;
    Subscription a, b, c, d;
    explicit Recorder(SimulationEngine& e) {
        a = e.events().subscribe<MissionCompleted>([this, &e](const MissionCompleted& m) {
            log.push_back("C" + std::to_string(m.id.value()) + "@" + std::to_string(e.tick()));
        });
        b = e.events().subscribe<MissionFailed>([this, &e](const MissionFailed& m) {
            log.push_back("F" + std::to_string(m.id.value()) + "@" + std::to_string(e.tick()));
        });
        c = e.events().subscribe<CombatExchange>([this](const CombatExchange& x) {
            log.push_back("X" + std::to_string(x.damageToFleet) + "/" + std::to_string(x.damageToPlanet));
        });
        d = e.events().subscribe<ResourceDiscovered>(
            [this](const ResourceDiscovered& r) { log.push_back("D" + std::to_string(r.amount)); });
    }
};

} // namespace

TEST_CASE("Engine rejects invalid configuration", "[core][engine]") {
    REQUIRE_THROWS_AS(SimulationEngine(nullptr), InvalidArgumentException);
    SimulationConfig bad;
    bad.timeStep = 0.0;
    REQUIRE_THROWS_AS(SimulationEngine(std::make_unique<Galaxy>(), bad), InvalidArgumentException);
    bad.timeStep = 0.1;
    bad.maxStepsPerAdvance = 0;
    REQUIRE_THROWS_AS(SimulationEngine(std::make_unique<Galaxy>(), bad), InvalidArgumentException);
}

TEST_CASE("Engine time is tick * timeStep and StepCompleted is published each step", "[core][engine]") {
    SimulationEngine engine(std::make_unique<Galaxy>(), SimulationConfig{0.25, 1, 100, false});
    std::vector<std::uint64_t> ticks;
    auto sub = engine.events().subscribe<StepCompleted>([&](const StepCompleted& s) {
        ticks.push_back(s.tick);
        REQUIRE(s.time == static_cast<double>(s.tick) * 0.25);
    });
    engine.runSteps(8);
    REQUIRE(engine.tick() == 8);
    REQUIRE(engine.time() == 2.0);
    REQUIRE(ticks == std::vector<std::uint64_t>{1, 2, 3, 4, 5, 6, 7, 8});
}

TEST_CASE("Engine advance() uses a fixed step with an accumulator", "[core][engine]") {
    SimulationEngine engine(std::make_unique<Galaxy>(), SimulationConfig{0.1, 1, 5, false});
    REQUIRE(engine.advance(0.05) == 0);
    REQUIRE(engine.accumulator() == Approx(0.05));
    REQUIRE(engine.advance(0.06) == 1);
    REQUIRE(engine.accumulator() == Approx(0.01));
    REQUIRE(engine.advance(10.0) == 5); // capped; backlog dropped
    REQUIRE(engine.accumulator() == 0.0);
    REQUIRE(engine.tick() == 6);
    REQUIRE_THROWS_AS(engine.advance(-1.0), InvalidArgumentException);
}

TEST_CASE("Slicing wall-clock time differently yields the same trajectory", "[core][engine][determinism]") {
    auto coarse = makeSampleScenario(options(11));
    auto fine = makeSampleScenario(options(11));
    for (int i = 0; i < 20; ++i) {
        static_cast<void>(
            coarse->advance(1.0)); // 10 steps per call (power-of-two-free slices still sum to 200)
    }
    for (int i = 0; i < 200; ++i) {
        static_cast<void>(fine->advance(0.1));
    }
    const auto steps = static_cast<std::int64_t>(coarse->tick()) - static_cast<std::int64_t>(fine->tick());
    REQUIRE(steps >= -2);
    REQUIRE(steps <= 2); // floating accumulation may shift at most a step at the boundary
    // Bring both to the same tick and compare exactly.
    while (coarse->tick() < fine->tick()) {
        coarse->step();
    }
    while (fine->tick() < coarse->tick()) {
        fine->step();
    }
    REQUIRE(coarse->galaxy().resources().checkConservation());
    nlohmann::json a = saveScenario(*coarse);
    nlohmann::json b = saveScenario(*fine);
    a.erase("accumulator");
    b.erase("accumulator");
    REQUIRE(a == b);
}

TEST_CASE("Identical seeds give bit-identical simulations", "[core][engine][determinism]") {
    const std::uint64_t seed = GENERATE(1U, 42U, 2026U);
    auto first = makeSampleScenario(options(seed));
    auto second = makeSampleScenario(options(seed));
    Recorder r1(*first);
    Recorder r2(*second);
    REQUIRE(first->stateDigest() == second->stateDigest());
    for (int chunk = 0; chunk < 10; ++chunk) {
        first->runSteps(40);
        second->runSteps(40);
        REQUIRE(first->stateDigest() == second->stateDigest());
    }
    REQUIRE(r1.log == r2.log);
    REQUIRE_FALSE(r1.log.empty());
    REQUIRE(first->stats() == second->stats());
    REQUIRE(first->rng() == second->rng());
}

TEST_CASE("Different seeds give different simulations", "[core][engine][determinism]") {
    auto a = makeSampleScenario(options(1));
    auto b = makeSampleScenario(options(2));
    a->runSteps(100);
    b->runSteps(100);
    REQUIRE(a->stateDigest() != b->stateDigest());
}

TEST_CASE("Engine syncs entity resource flows into the ledger and conserves totals",
          "[core][engine][invariant]") {
    auto galaxy = std::make_unique<Galaxy>();
    Planet& farm = galaxy->createPlanet("Farm", {});
    farm.setProductionRate(ResourceType::Food, 10.0);
    farm.setPopulation(1000.0); // eats 0.1 food/s
    Fleet& patrol = galaxy->createFleet("Patrol", {});
    patrol.addShips(ShipType::Fighter, 4); // 2 energy/s, but no energy in stock
    const EntityId farmId = farm.id();
    const EntityId patrolId = patrol.id();
    SimulationEngine engine(std::move(galaxy));
    std::size_t shortages = 0;
    auto sub = engine.events().subscribe<ResourceShortage>([&](const ResourceShortage& s) {
        REQUIRE(s.entity == patrolId);
        REQUIRE(s.type == ResourceType::Energy);
        ++shortages;
    });
    engine.runSteps(100); // 10 s
    const ResourceManager& rm = engine.galaxy().resources();
    REQUIRE(rm.productionRate(farmId, ResourceType::Food) == 10.0);
    REQUIRE(rm.consumptionRate(patrolId, ResourceType::Energy) == Approx(2.0));
    REQUIRE(rm.balance(farmId, ResourceType::Food) > 90);
    REQUIRE(rm.balance(farmId, ResourceType::Food) < 100);
    REQUIRE(shortages > 0);
    REQUIRE(engine.stats().resourceShortages == shortages);
    REQUIRE(engine.galaxy().get<Fleet>(patrolId).isLowPower());
    REQUIRE(rm.checkConservation());
}

TEST_CASE("Sample scenario runs to completion with consistent statistics", "[core][engine]") {
    auto engine = makeSampleScenario(options(GENERATE(3U, 5U, 8U)));
    const std::size_t missions = engine->galaxy().missionCount();
    REQUIRE(missions > 0);
    const auto steps = engine->runUntilMissionsFinished(20000);
    REQUIRE(steps < 20000);
    REQUIRE(engine->galaxy().unfinishedMissionCount() == 0);
    const SimulationStats& s = engine->stats();
    REQUIRE(s.missionsCompleted + s.missionsFailed == missions);
    REQUIRE(s.missionsStarted <= missions);
    REQUIRE(engine->galaxy().resources().checkConservation());
}

TEST_CASE("runUntil stops when the predicate holds", "[core][engine]") {
    SimulationEngine engine(std::make_unique<Galaxy>());
    const auto steps = engine.runUntil([](const SimulationEngine& e) { return e.tick() >= 7; }, 100);
    REQUIRE(steps == 7);
    REQUIRE(engine.runUntil([](const SimulationEngine&) { return false; }, 3) == 3);
    REQUIRE(engine.runUntilMissionsFinished(10) == 0);
}

TEST_CASE("Engine can prune finished missions automatically", "[core][engine]") {
    auto galaxy = std::make_unique<Galaxy>();
    const EntityId p = galaxy->createPlanet("P", {}).id();
    Fleet& f = galaxy->createFleet("F", {});
    f.addShips(ShipType::Scout);
    galaxy->addMission<ExplorationMission>(f.id(), p, 0.3);
    SimulationConfig cfg;
    cfg.pruneFinishedMissions = true;
    SimulationEngine engine(std::move(galaxy), cfg);
    engine.runSteps(20);
    REQUIRE(engine.galaxy().missionCount() == 0);
    REQUIRE(engine.stats().missionsCompleted == 1);
}
