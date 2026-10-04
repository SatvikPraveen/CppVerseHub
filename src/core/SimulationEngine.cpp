/**
 * @file SimulationEngine.cpp
 * @brief SimulationEngine implementation.
 */
#include "core/SimulationEngine.hpp"

#include <cmath>
#include <vector>

#include <nlohmann/json.hpp>

#include "core/Events.hpp"

namespace CppVerseHub::Core {

namespace {

std::uint64_t fnv1a(const std::string& data) noexcept {
    std::uint64_t h = 14695981039346656037ULL;
    for (const char c : data) {
        h ^= static_cast<std::uint8_t>(c);
        h *= 1099511628211ULL;
    }
    return h;
}

} // namespace

SimulationEngine::SimulationEngine(std::unique_ptr<Galaxy> galaxy, SimulationConfig config)
    : galaxy_(std::move(galaxy)), config_(config), rng_(config.seed) {
    if (!galaxy_) {
        throw InvalidArgumentException("SimulationEngine requires a galaxy");
    }
    if (!std::isfinite(config_.timeStep) || config_.timeStep <= 0.0) {
        throw InvalidArgumentException("timeStep must be positive and finite");
    }
    if (config_.maxStepsPerAdvance == 0) {
        throw InvalidArgumentException("maxStepsPerAdvance must be positive");
    }
    startedCounter_ = events_.subscribe<MissionStarted>([this](const MissionStarted&) { ++stats_.missionsStarted; });
    completedCounter_ =
        events_.subscribe<MissionCompleted>([this](const MissionCompleted&) { ++stats_.missionsCompleted; });
    failedCounter_ = events_.subscribe<MissionFailed>([this](const MissionFailed&) { ++stats_.missionsFailed; });
}

SimulationEngine::~SimulationEngine() = default;

void SimulationEngine::startPendingMissions(MissionContext& ctx) {
    for (const auto& entry : galaxy_->missions()) {
        Mission& mission = *entry.second;
        if (mission.status() == MissionStatus::Pending && !galaxy_->isFleetAssigned(mission.fleetId())) {
            mission.start(ctx);
        }
    }
}

void SimulationEngine::tickEconomy() {
    ResourceManager& ledger = galaxy_->resources();
    for (const auto& [id, entity] : galaxy_->entities()) {
        const ResourceFlows flows = entity->resourceFlows();
        for (ResourceType t : kAllResourceTypes) {
            ledger.setProductionRate(id, t, flows.production[t]);
            ledger.setConsumptionRate(id, t, flows.consumption[t]);
        }
    }
    const ResourceTickReport report = ledger.tick(config_.timeStep);
    std::map<EntityId, ResourceAmounts> shortfalls;
    for (const ResourceShortfall& s : report.shortfalls) {
        shortfalls[s.account][s.type] += s.shortfall;
        ++stats_.resourceShortages;
        events_.publish(ResourceShortage{s.account, s.type, s.shortfall});
    }
    for (const auto& [id, entity] : galaxy_->entities()) {
        const auto it = shortfalls.find(id);
        entity->onResourceTick(it == shortfalls.end() ? ResourceAmounts{} : it->second);
    }
}

void SimulationEngine::removeDestroyed() {
    std::vector<EntityDestroyed> destroyed;
    for (const auto& [id, entity] : galaxy_->entities()) {
        if (!entity->isAlive()) {
            destroyed.push_back(EntityDestroyed{id, entity->kind(), entity->name()});
        }
    }
    for (const EntityDestroyed& e : destroyed) {
        static_cast<void>(galaxy_->removeEntity(e.id));
        ++stats_.entitiesDestroyed;
        events_.publish(e);
    }
}

void SimulationEngine::step() {
    const double dt = config_.timeStep;
    MissionContext ctx{*galaxy_, rng_, events_};
    startPendingMissions(ctx);
    for (const auto& entry : galaxy_->missions()) {
        entry.second->update(ctx, dt);
    }
    for (const auto& entry : galaxy_->entities()) {
        entry.second->update(dt);
    }
    tickEconomy();
    removeDestroyed();
    if (config_.pruneFinishedMissions) {
        static_cast<void>(galaxy_->pruneFinishedMissions());
    }
    ++stats_.ticks;
    events_.publish(StepCompleted{stats_.ticks, time()});
}

void SimulationEngine::runSteps(std::uint64_t n) {
    for (std::uint64_t i = 0; i < n; ++i) {
        step();
    }
}

std::size_t SimulationEngine::advance(double seconds) {
    if (!std::isfinite(seconds) || seconds < 0.0) {
        throw InvalidArgumentException("advance() requires a finite, non-negative duration");
    }
    accumulator_ += seconds;
    std::size_t steps = 0;
    while (accumulator_ >= config_.timeStep && steps < config_.maxStepsPerAdvance) {
        step();
        accumulator_ -= config_.timeStep;
        ++steps;
    }
    if (steps == config_.maxStepsPerAdvance && accumulator_ >= config_.timeStep) {
        accumulator_ = 0.0; // drop the backlog instead of falling ever further behind
    }
    return steps;
}

std::uint64_t SimulationEngine::runUntil(const std::function<bool(const SimulationEngine&)>& done,
                                         std::uint64_t maxSteps) {
    std::uint64_t steps = 0;
    while (steps < maxSteps && !(done && done(*this))) {
        step();
        ++steps;
    }
    return steps;
}

std::uint64_t SimulationEngine::runUntilMissionsFinished(std::uint64_t maxSteps) {
    return runUntil([](const SimulationEngine& e) { return e.galaxy().unfinishedMissionCount() == 0; }, maxSteps);
}

void SimulationEngine::toJson(nlohmann::json& out) const {
    nlohmann::json g;
    galaxy_->toJson(g);
    out = {{"format", "cppversehub-scenario"},
           {"version", 1},
           {"config",
            {{"timeStep", config_.timeStep},
             {"seed", config_.seed},
             {"maxStepsPerAdvance", config_.maxStepsPerAdvance},
             {"pruneFinishedMissions", config_.pruneFinishedMissions}}},
           {"stats",
            {{"ticks", stats_.ticks},
             {"missionsStarted", stats_.missionsStarted},
             {"missionsCompleted", stats_.missionsCompleted},
             {"missionsFailed", stats_.missionsFailed},
             {"entitiesDestroyed", stats_.entitiesDestroyed},
             {"resourceShortages", stats_.resourceShortages}}},
           {"accumulator", accumulator_},
           {"rngState", rng_.saveState()},
           {"galaxy", std::move(g)}};
}

std::unique_ptr<SimulationEngine> SimulationEngine::fromJson(const nlohmann::json& in) {
    try {
        if (in.value("format", std::string{}) != "cppversehub-scenario") {
            throw SerializationException("not a cppversehub scenario document");
        }
        if (in.at("version").get<int>() != 1) {
            throw SerializationException("unsupported scenario version");
        }
        const auto& c = in.at("config");
        SimulationConfig config;
        config.timeStep = c.at("timeStep").get<double>();
        config.seed = c.at("seed").get<std::uint64_t>();
        config.maxStepsPerAdvance = c.value("maxStepsPerAdvance", config.maxStepsPerAdvance);
        config.pruneFinishedMissions = c.value("pruneFinishedMissions", false);
        auto engine = std::make_unique<SimulationEngine>(Galaxy::fromJson(in.at("galaxy")), config);
        if (in.contains("stats")) {
            const auto& s = in["stats"];
            engine->stats_.ticks = s.value("ticks", std::uint64_t{0});
            engine->stats_.missionsStarted = s.value("missionsStarted", std::uint64_t{0});
            engine->stats_.missionsCompleted = s.value("missionsCompleted", std::uint64_t{0});
            engine->stats_.missionsFailed = s.value("missionsFailed", std::uint64_t{0});
            engine->stats_.entitiesDestroyed = s.value("entitiesDestroyed", std::uint64_t{0});
            engine->stats_.resourceShortages = s.value("resourceShortages", std::uint64_t{0});
        }
        engine->accumulator_ = in.value("accumulator", 0.0);
        if (in.contains("rngState")) {
            engine->rng_.loadState(in["rngState"].get<std::string>(), config.seed);
        }
        return engine;
    } catch (const nlohmann::json::exception& e) {
        throw SerializationException(std::string("scenario: ") + e.what());
    } catch (const SerializationException&) {
        throw;
    } catch (const CoreException& e) {
        throw SerializationException(std::string("scenario: ") + e.what());
    }
}

std::uint64_t SimulationEngine::stateDigest() const {
    nlohmann::json j;
    toJson(j);
    return fnv1a(j.dump());
}

} // namespace CppVerseHub::Core
