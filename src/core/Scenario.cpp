/**
 * @file Scenario.cpp
 * @brief Scenario persistence and sample generation.
 */
#include "core/Scenario.hpp"

#include "core/ColonizationMission.hpp"
#include "core/CombatMission.hpp"
#include "core/ExplorationMission.hpp"

#include <array>
#include <fstream>
#include <string>

namespace CppVerseHub::Core {

nlohmann::json saveScenario(const SimulationEngine& engine) {
    nlohmann::json j;
    engine.toJson(j);
    return j;
}

std::unique_ptr<SimulationEngine> loadScenario(const nlohmann::json& document) {
    return SimulationEngine::fromJson(document);
}

void saveScenarioFile(const SimulationEngine& engine, const std::filesystem::path& path) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
        throw SerializationException("cannot open '" + path.string() + "' for writing");
    }
    out << saveScenario(engine).dump(2) << '\n';
    if (!out) {
        throw SerializationException("failed writing '" + path.string() + "'");
    }
}

std::unique_ptr<SimulationEngine> loadScenarioFile(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw SerializationException("cannot open '" + path.string() + "' for reading");
    }
    nlohmann::json document;
    try {
        in >> document;
    } catch (const nlohmann::json::exception& e) {
        throw SerializationException("'" + path.string() + "': " + e.what());
    }
    return loadScenario(document);
}

std::unique_ptr<Galaxy> makeSampleGalaxy(const SampleScenarioOptions& options) {
    if (options.planets == 0) {
        throw InvalidArgumentException("a sample scenario needs at least one planet");
    }
    if (!(options.radius > 0.0)) {
        throw InvalidArgumentException("sample radius must be positive");
    }
    static constexpr std::array<const char*, 8> kPlanetNames{"Terra", "Ares",   "Kepler", "Vega",
                                                             "Nova",  "Hadley", "Sol",    "Tycho"};
    static constexpr std::array<PlanetType, 6> kTypes{PlanetType::Terrestrial, PlanetType::Desert,
                                                      PlanetType::Ocean,       PlanetType::GasGiant,
                                                      PlanetType::Ice,         PlanetType::Volcanic};
    DeterministicRng rng(options.seed ^ 0x9E3779B97F4A7C15ULL);
    const double r = options.radius;
    auto galaxy = std::make_unique<Galaxy>("Sample Galaxy", Vector3D{-2.0 * r, -2.0 * r, -2.0 * r},
                                           Vector3D{2.0 * r, 2.0 * r, 2.0 * r});
    ResourceManager& ledger = galaxy->resources();

    std::vector<EntityId> planets;
    for (std::size_t i = 0; i < options.planets; ++i) {
        std::string name = kPlanetNames[i % kPlanetNames.size()];
        if (i >= kPlanetNames.size()) {
            name += "-" + std::to_string(i / kPlanetNames.size());
        }
        const Vector3D pos{rng.uniform(-r, r), rng.uniform(-r, r), rng.uniform(-r, r)};
        Planet& p = galaxy->createPlanet(std::move(name), pos, kTypes[rng.below(kTypes.size())],
                                         rng.uniform(0.1, 0.95));
        p.setProductionRate(ResourceType::Minerals, rng.uniform(1.0, 20.0));
        p.setProductionRate(ResourceType::Energy, rng.uniform(1.0, 15.0));
        p.setProductionRate(ResourceType::Food, rng.uniform(0.0, 10.0));
        if (i == 0) {
            p.setPopulation(50000.0);
            p.setExplored(true);
        } else if (rng.chance(0.35)) {
            p.setDefense(rng.uniform(50.0, 300.0));
            p.setPopulation(rng.uniform(1000.0, 20000.0));
        }
        ledger.deposit(p.id(), ResourceType::Food, 2000);
        planets.push_back(p.id());
    }

    for (std::size_t i = 0; i < options.fleets; ++i) {
        const Planet& home = galaxy->get<Planet>(planets.front());
        Fleet& fleet = galaxy->createFleet("Fleet-" + std::to_string(i + 1), home.position());
        ledger.deposit(fleet.id(), ResourceType::Energy, 5000);
        const EntityId target = planets[planets.size() > 1 ? 1 + rng.below(planets.size() - 1) : 0];
        if (target == planets.front()) {
            continue; // single-planet galaxy: nowhere to go
        }
        Planet& targetPlanet = galaxy->get<Planet>(target);
        switch (i % 3) {
            case 0:
                fleet.addShips(ShipType::Scout, 2);
                galaxy->addMission<ExplorationMission>(fleet.id(), target, rng.uniform(1.0, 4.0));
                break;
            case 1:
                fleet.addShips(ShipType::Cruiser, 3 + rng.below(3));
                fleet.addShips(ShipType::Battleship, 1 + rng.below(2));
                if (!targetPlanet.isHostile()) {
                    targetPlanet.setDefense(rng.uniform(40.0, 150.0));
                }
                galaxy->addMission<CombatMission>(fleet.id(), target, 30.0, CombatStrategy::Aggressive);
                break;
            default: {
                // Colonists need a peaceful world: prefer a non-hostile target, otherwise pacify this one.
                std::vector<EntityId> peaceful;
                for (std::size_t k = 1; k < planets.size(); ++k) {
                    if (!galaxy->get<Planet>(planets[k]).isHostile()) {
                        peaceful.push_back(planets[k]);
                    }
                }
                const EntityId colonyTarget = peaceful.empty() ? target
                                                               : peaceful[rng.below(peaceful.size())];
                galaxy->get<Planet>(colonyTarget).setDefense(0.0);
                fleet.addShips(ShipType::Colonizer, 1);
                fleet.addShips(ShipType::Transport, 2);
                ledger.deposit(fleet.id(), ResourceType::Minerals, 500);
                ledger.deposit(fleet.id(), ResourceType::Food, 800);
                galaxy->addMission<ColonizationMission>(fleet.id(), colonyTarget, rng.uniform(2.0, 5.0),
                                                        rng.uniform(500.0, 5000.0));
                break;
            }
        }
    }
    return galaxy;
}

std::unique_ptr<SimulationEngine> makeSampleScenario(const SampleScenarioOptions& options) {
    SimulationConfig config;
    config.seed = options.seed;
    config.timeStep = options.timeStep;
    return std::make_unique<SimulationEngine>(makeSampleGalaxy(options), config);
}

} // namespace CppVerseHub::Core
