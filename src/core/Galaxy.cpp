/**
 * @file Galaxy.cpp
 * @brief Galaxy implementation: ownership, id allocation, lookup and (de)serialisation.
 */
#include "core/Galaxy.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

#include <nlohmann/json.hpp>

namespace CppVerseHub::Core {

namespace {

constexpr double kInf = std::numeric_limits<double>::infinity();

std::string requireName(std::string name) {
    if (name.empty()) {
        throw InvalidArgumentException("galaxy name must not be empty");
    }
    return name;
}

} // namespace

Galaxy::Galaxy(std::string name, const Vector3D& minCorner, const Vector3D& maxCorner)
    : name_(requireName(std::move(name))), min_(minCorner), max_(maxCorner),
      entityFactory_(makeDefaultEntityFactory()), missionFactory_(makeDefaultMissionFactory()) {
    if (!(min_.x <= max_.x && min_.y <= max_.y && min_.z <= max_.z)) {
        throw InvalidArgumentException("galaxy bounds: minCorner must not exceed maxCorner");
    }
}

Galaxy::Galaxy(std::string name)
    : Galaxy(std::move(name), Vector3D{-kDefaultHalfExtent, -kDefaultHalfExtent, -kDefaultHalfExtent},
             Vector3D{kDefaultHalfExtent, kDefaultHalfExtent, kDefaultHalfExtent}) {}

Galaxy::~Galaxy() = default;

bool Galaxy::contains(const Vector3D& p) const noexcept {
    return p.x >= min_.x && p.x <= max_.x && p.y >= min_.y && p.y <= max_.y && p.z >= min_.z && p.z <= max_.z;
}

Planet& Galaxy::createPlanet(std::string name, const Vector3D& position, PlanetType type, double habitability) {
    auto planet = std::make_unique<Planet>(allocateEntityId(), std::move(name), position, type, habitability);
    return static_cast<Planet&>(addEntity(std::move(planet)));
}

Fleet& Galaxy::createFleet(std::string name, const Vector3D& position) {
    auto fleet = std::make_unique<Fleet>(allocateEntityId(), std::move(name), position);
    return static_cast<Fleet&>(addEntity(std::move(fleet)));
}

Entity& Galaxy::spawn(const std::string& kind, const nlohmann::json& params) {
    std::unique_ptr<Entity> entity;
    try {
        entity = entityFactory_.create(kind, allocateEntityId(), params);
    } catch (const nlohmann::json::exception& e) {
        throw SerializationException("spawn '" + kind + "': " + e.what());
    }
    return addEntity(std::move(entity));
}

Entity& Galaxy::addEntity(std::unique_ptr<Entity> entity) {
    if (!entity) {
        throw InvalidArgumentException("cannot add a null entity");
    }
    const EntityId id = entity->id();
    if (entities_.contains(id)) {
        throw InvalidArgumentException("duplicate entity id " + std::to_string(id.value()));
    }
    if (!contains(entity->position())) {
        throw InvalidArgumentException("entity '" + entity->name() + "' lies outside the galaxy bounds");
    }
    resources_.openAccount(id);
    Entity& ref = *entity;
    try {
        entities_.emplace(id, std::move(entity));
    } catch (...) {
        static_cast<void>(resources_.closeAccount(id));
        throw;
    }
    nextEntityId_ = std::max(nextEntityId_, id.value() + 1);
    return ref;
}

bool Galaxy::removeEntity(EntityId id) {
    const auto it = entities_.find(id);
    if (it == entities_.end()) {
        return false;
    }
    static_cast<void>(resources_.closeAccount(id));
    entities_.erase(it);
    return true;
}

Entity* Galaxy::find(EntityId id) noexcept {
    const auto it = entities_.find(id);
    return it == entities_.end() ? nullptr : it->second.get();
}

const Entity* Galaxy::find(EntityId id) const noexcept {
    const auto it = entities_.find(id);
    return it == entities_.end() ? nullptr : it->second.get();
}

Planet* Galaxy::nearestPlanet(const Vector3D& point) noexcept {
    Planet* best = nullptr;
    double bestDistance = kInf;
    for (auto& entry : entities_) {
        if (auto* planet = dynamic_cast<Planet*>(entry.second.get())) {
            const double d = planet->position().distanceSquaredTo(point);
            if (d < bestDistance) { // strict: earlier (lower) id wins ties
                bestDistance = d;
                best = planet;
            }
        }
    }
    return best;
}

Mission& Galaxy::addMission(std::unique_ptr<Mission> mission) {
    if (!mission) {
        throw InvalidArgumentException("cannot add a null mission");
    }
    const MissionId id = mission->id();
    if (!id.isValid() || missions_.contains(id)) {
        throw InvalidArgumentException("mission id " + std::to_string(id.value()) + " is invalid or in use");
    }
    if (!mission->isFinished()) {
        if (find<Fleet>(mission->fleetId()) == nullptr) {
            throw EntityNotFoundException(mission->fleetId(), "mission fleet must be an existing fleet");
        }
        if (find<Planet>(mission->targetId()) == nullptr) {
            throw EntityNotFoundException(mission->targetId(), "mission target must be an existing planet");
        }
    }
    Mission& ref = *mission;
    missions_.emplace(id, std::move(mission));
    nextMissionId_ = std::max(nextMissionId_, id.value() + 1);
    return ref;
}

Mission& Galaxy::spawnMission(const std::string& type, const nlohmann::json& params) {
    std::unique_ptr<Mission> mission;
    try {
        mission = missionFactory_.create(type, allocateMissionId(), params);
    } catch (const nlohmann::json::exception& e) {
        throw SerializationException("spawnMission '" + type + "': " + e.what());
    }
    return addMission(std::move(mission));
}

Mission* Galaxy::findMission(MissionId id) noexcept {
    const auto it = missions_.find(id);
    return it == missions_.end() ? nullptr : it->second.get();
}

const Mission* Galaxy::findMission(MissionId id) const noexcept {
    const auto it = missions_.find(id);
    return it == missions_.end() ? nullptr : it->second.get();
}

std::size_t Galaxy::unfinishedMissionCount() const noexcept {
    return static_cast<std::size_t>(std::count_if(missions_.begin(), missions_.end(),
                                                  [](const auto& e) { return !e.second->isFinished(); }));
}

bool Galaxy::isFleetAssigned(EntityId fleet) const noexcept {
    return std::any_of(missions_.begin(), missions_.end(), [fleet](const auto& e) {
        return e.second->status() == MissionStatus::Active && e.second->fleetId() == fleet;
    });
}

std::size_t Galaxy::pruneFinishedMissions() {
    return static_cast<std::size_t>(std::erase_if(missions_, [](const auto& e) { return e.second->isFinished(); }));
}

void Galaxy::toJson(nlohmann::json& out) const {
    nlohmann::json minJ;
    nlohmann::json maxJ;
    vectorToJson(min_, minJ);
    vectorToJson(max_, maxJ);
    nlohmann::json entities = nlohmann::json::array();
    for (const auto& entry : entities_) {
        nlohmann::json e;
        entry.second->toJson(e);
        entities.push_back(std::move(e));
    }
    nlohmann::json missions = nlohmann::json::array();
    for (const auto& entry : missions_) {
        nlohmann::json m;
        entry.second->toJson(m);
        missions.push_back(std::move(m));
    }
    nlohmann::json ledger;
    resources_.toJson(ledger);
    out = {{"name", name_},
           {"minCorner", std::move(minJ)},
           {"maxCorner", std::move(maxJ)},
           {"nextEntityId", nextEntityId_},
           {"nextMissionId", nextMissionId_},
           {"entities", std::move(entities)},
           {"missions", std::move(missions)},
           {"resources", std::move(ledger)}};
}

std::unique_ptr<Galaxy> Galaxy::fromJson(const nlohmann::json& in, EntityFactory entityFactory,
                                         MissionFactory missionFactory) {
    try {
        auto galaxy = std::make_unique<Galaxy>(in.at("name").get<std::string>(), vectorFromJson(in.at("minCorner")),
                                               vectorFromJson(in.at("maxCorner")));
        galaxy->entityFactory_ = std::move(entityFactory);
        galaxy->missionFactory_ = std::move(missionFactory);
        for (const auto& e : in.at("entities")) {
            const EntityId id{e.at("id").get<std::uint64_t>()};
            galaxy->addEntity(galaxy->entityFactory_.create(e.at("kind").get<std::string>(), id, e));
        }
        for (const auto& m : in.at("missions")) {
            const MissionId id{m.at("id").get<std::uint64_t>()};
            galaxy->addMission(galaxy->missionFactory_.create(m.at("type").get<std::string>(), id, m));
        }
        // The ledger is restored last and replaces the empty accounts opened by addEntity.
        galaxy->resources_.fromJson(in.at("resources"));
        for (const auto& entry : galaxy->entities_) {
            if (!galaxy->resources_.hasAccount(entry.first)) {
                throw SerializationException("entity #" + std::to_string(entry.first.value()) +
                                             " has no resource account");
            }
        }
        galaxy->nextEntityId_ = std::max(galaxy->nextEntityId_, in.at("nextEntityId").get<std::uint64_t>());
        galaxy->nextMissionId_ = std::max(galaxy->nextMissionId_, in.at("nextMissionId").get<std::uint64_t>());
        return galaxy;
    } catch (const nlohmann::json::exception& e) {
        throw SerializationException(std::string("galaxy: ") + e.what());
    } catch (const SerializationException&) {
        throw;
    } catch (const CoreException& e) {
        throw SerializationException(std::string("galaxy: ") + e.what());
    }
}

} // namespace CppVerseHub::Core
