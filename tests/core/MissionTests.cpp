#include "core/ColonizationMission.hpp"
#include "core/CombatMission.hpp"
#include "core/Events.hpp"
#include "core/ExplorationMission.hpp"
#include "core/SimulationEngine.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <nlohmann/json.hpp>

#include <memory>
#include <vector>

using namespace CppVerseHub::Core;
using Catch::Approx;

namespace {

/// A small world: home at the origin, a target 30 units away, fleets parked at home.
struct World {
    std::unique_ptr<SimulationEngine> engine;
    EntityId home;
    EntityId target;

    explicit World(std::uint64_t seed = 7) {
        auto galaxy = std::make_unique<Galaxy>("Test");
        home = galaxy->createPlanet("Home", {0, 0, 0}).id();
        target = galaxy->createPlanet("Target", {30, 0, 0}, PlanetType::Ocean, 0.6).id();
        SimulationConfig cfg;
        cfg.seed = seed;
        cfg.timeStep = 0.1;
        engine = std::make_unique<SimulationEngine>(std::move(galaxy), cfg);
    }
    Galaxy& galaxy() { return engine->galaxy(); }
    Fleet& fleet(const char* name, ShipType type, std::size_t count, ResourceAmount energy = 10'000) {
        Fleet& f = galaxy().createFleet(name, {0, 0, 0});
        f.addShips(type, count);
        galaxy().resources().deposit(f.id(), ResourceType::Energy, energy);
        return f;
    }
};

} // namespace

TEST_CASE("Mission constructor validates parameters", "[core][mission]") {
    REQUIRE_THROWS_AS(ExplorationMission(MissionId{1}, EntityId{}, EntityId{2}, 5.0),
                      InvalidArgumentException);
    REQUIRE_THROWS_AS(ExplorationMission(MissionId{1}, EntityId{1}, EntityId{1}, 5.0),
                      InvalidArgumentException);
    REQUIRE_THROWS_AS(ExplorationMission(MissionId{1}, EntityId{1}, EntityId{2}, 0.0),
                      InvalidArgumentException);
    REQUIRE_THROWS_AS(ExplorationMission(MissionId{}, EntityId{1}, EntityId{2}, 1.0),
                      InvalidArgumentException);
    REQUIRE_THROWS_AS(ColonizationMission(MissionId{1}, EntityId{1}, EntityId{2}, 1.0, -5.0),
                      InvalidArgumentException);
    const CombatMission m(MissionId{3}, EntityId{1}, EntityId{2}, 12.0, CombatStrategy::Defensive);
    REQUIRE(m.type() == MissionType::Combat);
    REQUIRE(m.status() == MissionStatus::Pending);
    REQUIRE(m.phase() == MissionPhase::Travel);
    REQUIRE(m.progress() == 0.0);
    REQUIRE(m.duration() == 12.0);
    REQUIRE(m.strategy() == CombatStrategy::Defensive);
}

TEST_CASE("Mission lifecycle: travel, execute, complete", "[core][mission]") {
    World w;
    Fleet& scouts = w.fleet("Scouts", ShipType::Scout, 2); // speed 40 -> 30 units in < 1 s
    auto& m = w.galaxy().addMission<ExplorationMission>(scouts.id(), w.target, 2.0);
    std::vector<MissionStatus> seen;
    auto sub = w.engine->events().subscribe<MissionStarted>([&](const MissionStarted& e) {
        REQUIRE(e.fleet == scouts.id());
        seen.push_back(MissionStatus::Active);
    });
    w.engine->step();
    REQUIRE(m.status() == MissionStatus::Active);
    REQUIRE(m.phase() == MissionPhase::Travel);
    REQUIRE(seen.size() == 1);
    w.engine->runSteps(10);
    REQUIRE(m.phase() == MissionPhase::Execute);
    REQUIRE(m.progress() > 0.0);
    w.engine->runUntilMissionsFinished(1000);
    REQUIRE(m.status() == MissionStatus::Completed);
    REQUIRE(m.progress() == 1.0);
    REQUIRE(m.elapsed() == Approx(2.0).margin(0.11));
    REQUIRE(w.galaxy().get<Planet>(w.target).isExplored());
    MissionContext ctx{w.galaxy(), w.engine->rng(), w.engine->events()};
    REQUIRE_THROWS_AS(m.start(ctx), InvalidStateException);
}

TEST_CASE("Exploration discoveries are minted into the planet account and published", "[core][mission]") {
    World w(GENERATE(1U, 2U, 3U, 4U));
    Fleet& scouts = w.fleet("Scouts", ShipType::Scout, 1);
    auto& m = w.galaxy().addMission<ExplorationMission>(scouts.id(), w.target, 1.0);
    ResourceAmounts published;
    auto sub = w.engine->events().subscribe<ResourceDiscovered>([&](const ResourceDiscovered& e) {
        REQUIRE(e.planet == w.target);
        published[e.type] += e.amount;
    });
    w.engine->runUntilMissionsFinished(1000);
    REQUIRE(m.status() == MissionStatus::Completed);
    REQUIRE(published == m.discovered());
    const ResourceAmounts held = w.galaxy().resources().balances(w.target);
    for (ResourceType t : kAllResourceTypes) {
        REQUIRE(held[t] >= m.discovered()[t]);
        if (m.discovered()[t] != 0) {
            REQUIRE(m.discovered()[t] >= static_cast<ResourceAmount>(ExplorationMission::kMinDeposit * 0.5));
        }
    }
    REQUIRE(w.galaxy().resources().checkConservation());
}

TEST_CASE("Combat mission defeats weak defences and records damage", "[core][mission]") {
    World w;
    w.galaxy().get<Planet>(w.target).setDefense(60.0);
    Fleet& armada = w.fleet("Armada", ShipType::Battleship, 2);
    auto& m = w.galaxy().addMission<CombatMission>(armada.id(), w.target, 60.0, CombatStrategy::Aggressive);
    int exchanges = 0;
    auto sub = w.engine->events().subscribe<CombatExchange>([&](const CombatExchange& e) {
        REQUIRE(e.mission == m.id());
        REQUIRE(e.damageToPlanet >= 0.0);
        ++exchanges;
    });
    w.engine->runUntilMissionsFinished(5000);
    REQUIRE(m.status() == MissionStatus::Completed);
    REQUIRE(w.galaxy().get<Planet>(w.target).defense() == 0.0);
    REQUIRE(m.damageDealt() == Approx(60.0));
    REQUIRE(m.damageTaken() > 0.0);
    REQUIRE(exchanges > 0);
}

TEST_CASE("Combat mission fails when the fleet is destroyed", "[core][mission]") {
    World w;
    w.galaxy().get<Planet>(w.target).setDefense(1.0e6);
    Fleet& doomed = w.fleet("Doomed", ShipType::Fighter, 1);
    const EntityId doomedId = doomed.id();
    auto& m = w.galaxy().addMission<CombatMission>(doomedId, w.target, 1000.0);
    std::vector<EntityId> destroyed;
    auto sub = w.engine->events().subscribe<EntityDestroyed>(
        [&](const EntityDestroyed& e) { destroyed.push_back(e.id); });
    w.engine->runUntilMissionsFinished(5000);
    REQUIRE(m.status() == MissionStatus::Failed);
    REQUIRE(m.failureReason() == "fleet destroyed");
    REQUIRE(destroyed == std::vector<EntityId>{doomedId});
    REQUIRE(w.galaxy().find(doomedId) == nullptr);
    REQUIRE_FALSE(w.galaxy().resources().hasAccount(doomedId));
    REQUIRE(w.engine->stats().entitiesDestroyed == 1);
}

TEST_CASE("Combat mission retreats when time runs out", "[core][mission]") {
    World w;
    w.galaxy().get<Planet>(w.target).setDefense(5000.0);
    Fleet& tanks = w.fleet("Tanks", ShipType::Battleship, 6);
    auto& m = w.galaxy().addMission<CombatMission>(tanks.id(), w.target, 1.0, CombatStrategy::Defensive);
    w.engine->runUntilMissionsFinished(5000);
    REQUIRE(m.status() == MissionStatus::Failed);
    REQUIRE(m.failureReason().find("retreated") == 0);
    REQUIRE(w.galaxy().get<Planet>(w.target).defense() < 5000.0);
}

TEST_CASE("Combat strategy modifiers are ordered sensibly", "[core][mission]") {
    constexpr auto aggressive = CombatMission::modifiers(CombatStrategy::Aggressive);
    constexpr auto balanced = CombatMission::modifiers(CombatStrategy::Balanced);
    constexpr auto defensive = CombatMission::modifiers(CombatStrategy::Defensive);
    static_assert(aggressive.first > balanced.first && balanced.first > defensive.first);
    static_assert(aggressive.second > balanced.second && balanced.second > defensive.second);
    REQUIRE(parseCombatStrategy("defensive") == CombatStrategy::Defensive);
    REQUIRE_FALSE(parseCombatStrategy("reckless").has_value());
}

TEST_CASE("Colonization transfers cargo, adds colonists and consumes the colonizer", "[core][mission]") {
    World w;
    Fleet& colony = w.fleet("Ark", ShipType::Colonizer, 1);
    colony.addShips(ShipType::Transport, 1);
    ResourceManager& rm = w.galaxy().resources();
    rm.deposit(colony.id(), ResourceType::Minerals, 300);
    rm.deposit(colony.id(), ResourceType::Food, 400);
    auto& m = w.galaxy().addMission<ColonizationMission>(colony.id(), w.target, 1.5, 2500.0);
    const ResourceAmount mineralsBefore = rm.total(ResourceType::Minerals);
    w.engine->runUntilMissionsFinished(5000);
    REQUIRE(m.status() == MissionStatus::Completed);
    const Planet& target = w.galaxy().get<Planet>(w.target);
    REQUIRE(target.population() >= 2500.0 * 0.9); // colonists arrive (minus any later starvation)
    REQUIRE(m.delivered()[ResourceType::Minerals] == 300);
    REQUIRE(rm.balance(w.target, ResourceType::Minerals) >= 300);
    REQUIRE(rm.balance(colony.id(), ResourceType::Minerals) == 0);
    REQUIRE(rm.total(ResourceType::Minerals) >= mineralsBefore); // moved, never lost
    REQUIRE(colony.shipCount(ShipType::Colonizer) == 0);
    REQUIRE(rm.checkConservation());
}

TEST_CASE("Colonization preconditions: colonizer required and target not hostile", "[core][mission]") {
    World w;
    Fleet& noColonizer = w.fleet("Transports", ShipType::Transport, 2);
    auto& m1 = w.galaxy().addMission<ColonizationMission>(noColonizer.id(), w.target, 1.0);
    Fleet& ark = w.fleet("Ark", ShipType::Colonizer, 1);
    w.galaxy().get<Planet>(w.home).setDefense(10.0);
    auto& m2 = w.galaxy().addMission<ColonizationMission>(ark.id(), w.home, 1.0);
    std::vector<std::string> reasons;
    auto sub = w.engine->events().subscribe<MissionFailed>(
        [&](const MissionFailed& e) { reasons.push_back(e.reason); });
    w.engine->step();
    REQUIRE(m1.status() == MissionStatus::Failed);
    REQUIRE(m1.failureReason() == "fleet has no colonizer ship");
    REQUIRE(m2.status() == MissionStatus::Failed);
    REQUIRE(m2.failureReason() == "target is hostile");
    REQUIRE(reasons.size() == 2);
    REQUIRE(w.engine->stats().missionsFailed == 2);
}

TEST_CASE("A fleet runs one mission at a time; queued missions wait", "[core][mission]") {
    World w;
    Fleet& scouts = w.fleet("Scouts", ShipType::Scout, 1);
    auto& first = w.galaxy().addMission<ExplorationMission>(scouts.id(), w.target, 1.0);
    auto& second = w.galaxy().addMission<ExplorationMission>(scouts.id(), w.home, 1.0);
    w.engine->step();
    REQUIRE(first.status() == MissionStatus::Active);
    REQUIRE(second.status() == MissionStatus::Pending);
    REQUIRE(w.galaxy().isFleetAssigned(scouts.id()));
    w.engine->runUntilMissionsFinished(5000);
    REQUIRE(first.status() == MissionStatus::Completed);
    REQUIRE(second.status() == MissionStatus::Completed);
    REQUIRE(scouts.position() == w.galaxy().get<Planet>(w.home).position());
}

TEST_CASE("Missions fail when their fleet disappears and can be cancelled", "[core][mission]") {
    World w;
    Fleet& scouts = w.fleet("Scouts", ShipType::Scout, 1);
    auto& m = w.galaxy().addMission<ExplorationMission>(scouts.id(), w.target, 50.0);
    auto& other = w.galaxy().addMission<ExplorationMission>(w.fleet("B", ShipType::Scout, 1).id(), w.target,
                                                            50.0);
    w.engine->step();
    REQUIRE(w.galaxy().removeEntity(scouts.id()));
    w.engine->step();
    REQUIRE(m.status() == MissionStatus::Failed);
    REQUIRE(m.failureReason() == "fleet lost");
    other.cancel();
    REQUIRE(other.status() == MissionStatus::Cancelled);
    other.cancel();
    REQUIRE(other.isFinished());
    REQUIRE(w.galaxy().pruneFinishedMissions() == 2);
    REQUIRE(w.galaxy().missionCount() == 0);
}

TEST_CASE("Mission JSON round-trip preserves type-specific state", "[core][mission][json]") {
    CombatMission c(MissionId{4}, EntityId{1}, EntityId{2}, 9.0, CombatStrategy::Aggressive);
    nlohmann::json j;
    c.toJson(j);
    REQUIRE(j["type"] == "combat");
    const CombatMission c2(MissionId{4}, j);
    nlohmann::json j2;
    c2.toJson(j2);
    REQUIRE(j == j2);

    ColonizationMission k(MissionId{5}, EntityId{1}, EntityId{2}, 3.0, 750.0);
    k.toJson(j);
    const ColonizationMission k2(MissionId{5}, j);
    REQUIRE(k2.colonists() == 750.0);
    REQUIRE(k2.describe().find("colonization mission #5") == 0);
}
