/**
 * @file Demo.cpp
 * @brief Core module showcase.
 */
#include "core/Demo.hpp"

#include <exception>
#include <iomanip>
#include <sstream>

#include "core/ColonizationMission.hpp"
#include "core/CombatMission.hpp"
#include "core/EventSystem.hpp"
#include "core/Events.hpp"
#include "core/ExplorationMission.hpp"
#include "core/Scenario.hpp"

namespace CppVerseHub::Core {

namespace {

void showVectors(std::ostream& out) {
    out << "-- Vector3D (constexpr value type) --\n";
    constexpr Vector3D a{1.0, 2.0, 3.0};
    constexpr Vector3D b{4.0, 5.0, 6.0};
    constexpr double dot = a.dot(b);
    static_assert(dot == 32.0);
    constexpr Vector3D c = a.cross(b);
    out << "a = " << a << ", b = " << b << "\n"
        << "a + b = " << (a + b) << ", a . b = " << dot << ", a x b = " << c << "\n"
        << "|b - a| = " << a.distanceTo(b) << ", lerp(a, b, 0.5) = " << Vector3D::lerp(a, b, 0.5) << "\n";
}

void showEntitiesAndFactory(std::ostream& out) {
    out << "-- Strong ids, entities and the factory registry --\n";
    Galaxy galaxy("Factory Demo");
    Planet& terra = galaxy.createPlanet("Terra", {0.0, 0.0, 0.0}, PlanetType::Terrestrial, 0.9);
    Entity& vega = galaxy.spawn("planet", {{"name", "Vega"},
                                           {"position", {120.0, 0.0, 0.0}},
                                           {"planetType", "ocean"},
                                           {"habitability", 0.7}});
    Entity& patrol =
        galaxy.spawn("fleet", {{"name", "Patrol"},
                               {"position", {0.0, 0.0, 0.0}},
                               {"ships", {{{"type", "fighter"}, {"count", 6}}, {{"type", "cruiser"}, {"count", 2}}}}});
    out << "registered kinds:";
    for (const auto& key : galaxy.entityFactory().keys()) {
        out << ' ' << key;
    }
    out << "\n" << terra.describe() << "\n" << vega.describe() << "\n" << patrol.describe() << "\n";
    try {
        static_cast<void>(galaxy.spawn("starbase", nlohmann::json::object()));
    } catch (const FactoryException& e) {
        out << "expected error: " << e.what() << "\n";
    }
    try {
        static_cast<void>(galaxy.get<Fleet>(terra.id()));
    } catch (const EntityNotFoundException& e) {
        out << "expected error: " << e.what() << "\n";
    }
}

void showEconomy(std::ostream& out) {
    out << "-- ResourceManager conservation --\n";
    ResourceManager ledger;
    const EntityId mine{1};
    const EntityId depot{2};
    ledger.openAccount(mine);
    ledger.openAccount(depot);
    ledger.deposit(mine, ResourceType::Minerals, 1000);
    ledger.setProductionRate(mine, ResourceType::Minerals, 12.5);
    ledger.setConsumptionRate(depot, ResourceType::Minerals, 40.0);
    for (int i = 0; i < 10; ++i) {
        static_cast<void>(ledger.tick(0.1));
        ledger.transfer(mine, depot, ResourceType::Minerals, 30);
    }
    out << "mine " << ledger.balance(mine, ResourceType::Minerals) << ", depot "
        << ledger.balance(depot, ResourceType::Minerals) << ", minted " << ledger.totalMinted(ResourceType::Minerals)
        << ", burned " << ledger.totalBurned(ResourceType::Minerals) << ", conserved "
        << std::boolalpha << ledger.checkConservation() << "\n";
    try {
        ledger.transfer(depot, mine, ResourceType::Minerals, 1'000'000);
    } catch (const InsufficientResourcesException& e) {
        out << "expected error: " << e.what() << "\n";
    }
}

void showSimulation(std::ostream& out) {
    out << "-- Deterministic simulation with events --\n";
    SampleScenarioOptions options;
    options.seed = 2026;
    options.planets = 6;
    options.fleets = 3;
    auto engine = makeSampleScenario(options);
    std::ostringstream log;
    std::size_t exchanges = 0;
    Subscription s1 = engine->events().subscribe<MissionStarted>([&](const MissionStarted& e) {
        log << "  t=" << engine->time() << " started " << toString(e.type) << " mission " << e.id << "\n";
    });
    Subscription s2 = engine->events().subscribe<MissionCompleted>([&](const MissionCompleted& e) {
        log << "  t=" << engine->time() << " completed " << toString(e.type) << " mission " << e.id << "\n";
    });
    Subscription s3 = engine->events().subscribe<MissionFailed>([&](const MissionFailed& e) {
        log << "  t=" << engine->time() << " failed " << toString(e.type) << " mission " << e.id << ": " << e.reason
            << "\n";
    });
    Subscription s4 = engine->events().subscribe<ResourceDiscovered>([&](const ResourceDiscovered& e) {
        log << "  discovered " << e.amount << ' ' << toString(e.type) << " on planet " << e.planet << "\n";
    });
    Subscription s5 = engine->events().subscribe<CombatExchange>([&](const CombatExchange&) { ++exchanges; });

    const std::uint64_t steps = engine->runUntilMissionsFinished(5000);
    out << log.str();
    const SimulationStats& st = engine->stats();
    out << "ran " << steps << " steps (" << engine->time() << " s simulated), missions started "
        << st.missionsStarted << ", completed " << st.missionsCompleted << ", failed " << st.missionsFailed
        << ", combat exchanges " << exchanges << ", conservation "
        << std::boolalpha << engine->galaxy().resources().checkConservation() << "\n";
    for (const auto& entry : engine->galaxy().missions()) {
        out << "  " << entry.second->describe() << "\n";
    }

    out << "-- Reproducibility and save/load round trip --\n";
    auto replay = makeSampleScenario(options);
    static_cast<void>(replay->runUntilMissionsFinished(5000));
    out << "same seed, same digest: " << std::boolalpha << (replay->stateDigest() == engine->stateDigest()) << "\n";

    const nlohmann::json saved = saveScenario(*engine);
    auto restored = loadScenario(saved);
    engine->runSteps(50);
    restored->runSteps(50);
    out << "scenario size " << saved.dump().size() << " bytes; restored run matches original: "
        << (restored->stateDigest() == engine->stateDigest()) << "\n";
    out << "digest 0x" << std::hex << engine->stateDigest() << std::dec << "\n";
}

} // namespace

void runDemo(std::ostream& out) {
    try {
        out << "=== CppVerseHub Core: space-fleet simulation domain ===\n";
        showVectors(out);
        showEntitiesAndFactory(out);
        showEconomy(out);
        showSimulation(out);
        out << "=== core demo complete ===\n";
    } catch (const std::exception& e) {
        out << "core demo aborted: " << e.what() << "\n";
    } catch (...) {
        out << "core demo aborted: unknown error\n";
    }
}

} // namespace CppVerseHub::Core
