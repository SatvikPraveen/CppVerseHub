/**
 * @file Galaxy.hpp
 * @brief The world container: owns every entity, mission and the resource ledger.
 *
 * Ownership is explicit and single: entities and missions are held in `std::map`s of
 * `std::unique_ptr`, keyed by strong ids, so iteration order is the id order and therefore
 * deterministic. The galaxy is the only place ids are allocated, and it keeps the ResourceManager's
 * accounts in sync with entity lifetimes (an account is opened when an entity is added and closed
 * when it is removed). Typed lookup (`find<Fleet>(id)`, `get<Planet>(id)`) hides `dynamic_cast`
 * behind a safe interface.
 */
#pragma once

#include <concepts>
#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <nlohmann/json_fwd.hpp>

#include "core/Entity.hpp"
#include "core/Exceptions.hpp"
#include "core/Factory.hpp"
#include "core/Fleet.hpp"
#include "core/Mission.hpp"
#include "core/Planet.hpp"
#include "core/ResourceManager.hpp"

namespace CppVerseHub::Core {

/// @brief Container and id authority for one simulated world.
class Galaxy {
public:
    /// @brief Default half-extent of the galaxy cube.
    static constexpr double kDefaultHalfExtent = 1.0e6;

    /**
     * @brief Construct an empty galaxy spanning the axis-aligned box [minCorner, maxCorner].
     * @param name Non-empty name.
     * @param minCorner Lower corner.
     * @param maxCorner Upper corner (each component >= minCorner's).
     */
    Galaxy(std::string name, const Vector3D& minCorner, const Vector3D& maxCorner);

    /// @brief Construct an empty galaxy spanning [-kDefaultHalfExtent, kDefaultHalfExtent]^3. @param name Name.
    explicit Galaxy(std::string name = "Galaxy");

    Galaxy(const Galaxy&) = delete;            ///< Not copyable.
    Galaxy& operator=(const Galaxy&) = delete; ///< Not copy-assignable.
    Galaxy(Galaxy&&) = delete;                 ///< Not movable (owns a ResourceManager).
    Galaxy& operator=(Galaxy&&) = delete;      ///< Not move-assignable.
    ~Galaxy();                                 ///< Destructor.

    /// @brief Name. @return Name.
    [[nodiscard]] const std::string& name() const noexcept { return name_; }
    /// @brief Lower bound corner. @return Corner.
    [[nodiscard]] const Vector3D& minCorner() const noexcept { return min_; }
    /// @brief Upper bound corner. @return Corner.
    [[nodiscard]] const Vector3D& maxCorner() const noexcept { return max_; }
    /// @brief Whether a point lies within the bounds. @param p Point. @return true if inside (inclusive).
    [[nodiscard]] bool contains(const Vector3D& p) const noexcept;

    // ---- entities ----------------------------------------------------------------------------

    /// @brief Reserve a fresh entity id. @return Id never used before in this galaxy.
    [[nodiscard]] EntityId allocateEntityId() noexcept { return EntityId{nextEntityId_++}; }

    /**
     * @brief Create and add a planet.
     * @param name Non-empty name.
     * @param position Position inside the bounds.
     * @param type Classification.
     * @param habitability Habitability in [0, 1].
     * @return The new planet.
     */
    Planet& createPlanet(std::string name, const Vector3D& position, PlanetType type = PlanetType::Terrestrial,
                         double habitability = 0.5);

    /**
     * @brief Create and add an empty fleet.
     * @param name Non-empty name.
     * @param position Position inside the bounds.
     * @return The new fleet.
     */
    Fleet& createFleet(std::string name, const Vector3D& position);

    /**
     * @brief Create an entity through the entity factory.
     * @param kind Factory key ("planet", "fleet", or a custom registration).
     * @param params Constructor parameters (see Planet/Fleet JSON constructors).
     * @return The new entity.
     */
    Entity& spawn(const std::string& kind, const nlohmann::json& params);

    /**
     * @brief Take ownership of an entity. Its id must be unused; later allocations skip past it.
     * @param entity Non-null entity positioned inside the bounds.
     * @return Reference to the stored entity.
     */
    Entity& addEntity(std::unique_ptr<Entity> entity);

    /// @brief Remove an entity and close its resource account. @param id Entity. @return true if removed.
    bool removeEntity(EntityId id);

    /// @brief Look up any entity. @param id Id. @return Pointer or nullptr.
    [[nodiscard]] Entity* find(EntityId id) noexcept;
    /// @brief Look up any entity. @param id Id. @return Pointer or nullptr.
    [[nodiscard]] const Entity* find(EntityId id) const noexcept;

    /// @brief Typed lookup. @tparam T Entity subtype. @param id Id. @return Pointer, or nullptr if absent/wrong type.
    template <std::derived_from<Entity> T>
    [[nodiscard]] T* find(EntityId id) noexcept {
        return dynamic_cast<T*>(find(id));
    }
    /// @brief Typed lookup. @tparam T Entity subtype. @param id Id. @return Pointer, or nullptr if absent/wrong type.
    template <std::derived_from<Entity> T>
    [[nodiscard]] const T* find(EntityId id) const noexcept {
        return dynamic_cast<const T*>(find(id));
    }

    /// @brief Typed lookup that throws EntityNotFoundException. @tparam T Subtype. @param id Id. @return Entity.
    template <std::derived_from<Entity> T>
    [[nodiscard]] T& get(EntityId id) {
        if (T* p = find<T>(id)) {
            return *p;
        }
        throw EntityNotFoundException(id, find(id) ? "entity has a different kind" : "");
    }
    /// @brief Typed lookup that throws EntityNotFoundException. @tparam T Subtype. @param id Id. @return Entity.
    template <std::derived_from<Entity> T>
    [[nodiscard]] const T& get(EntityId id) const {
        if (const T* p = find<T>(id)) {
            return *p;
        }
        throw EntityNotFoundException(id, find(id) ? "entity has a different kind" : "");
    }

    /// @brief All entities of a subtype in id order. @tparam T Subtype. @return Non-owning pointers.
    template <std::derived_from<Entity> T>
    [[nodiscard]] std::vector<T*> all() {
        std::vector<T*> out;
        for (auto& entry : entities_) {
            if (auto* p = dynamic_cast<T*>(entry.second.get())) {
                out.push_back(p);
            }
        }
        return out;
    }
    /// @brief All entities of a subtype in id order. @tparam T Subtype. @return Non-owning pointers.
    template <std::derived_from<Entity> T>
    [[nodiscard]] std::vector<const T*> all() const {
        std::vector<const T*> out;
        for (const auto& entry : entities_) {
            if (const auto* p = dynamic_cast<const T*>(entry.second.get())) {
                out.push_back(p);
            }
        }
        return out;
    }

    /// @brief Every entity, ordered by id. @return Owning map (read-only).
    [[nodiscard]] const std::map<EntityId, std::unique_ptr<Entity>>& entities() const noexcept { return entities_; }
    /// @brief Number of entities. @return Count.
    [[nodiscard]] std::size_t entityCount() const noexcept { return entities_.size(); }

    /**
     * @brief Nearest planet to a point (ties broken by lower id).
     * @param point Query point.
     * @return Planet or nullptr if there are none.
     */
    [[nodiscard]] Planet* nearestPlanet(const Vector3D& point) noexcept;

    // ---- missions ----------------------------------------------------------------------------

    /// @brief Reserve a fresh mission id. @return Id.
    [[nodiscard]] MissionId allocateMissionId() noexcept { return MissionId{nextMissionId_++}; }

    /**
     * @brief Construct a mission in place with a freshly allocated id.
     * @tparam M Mission subtype, constructible as M(MissionId, Args...).
     * @param args Remaining constructor arguments (typically fleet id, target id, duration, ...).
     * @return The new mission.
     */
    template <std::derived_from<Mission> M, typename... Args>
        requires std::constructible_from<M, MissionId, Args...>
    M& addMission(Args&&... args) {
        auto mission = std::make_unique<M>(allocateMissionId(), std::forward<Args>(args)...);
        M& ref = *mission;
        addMission(std::move(mission));
        return ref;
    }

    /**
     * @brief Take ownership of a mission. An unfinished mission must reference an existing fleet and planet.
     * @param mission Non-null mission with an unused id.
     * @return Reference to the stored mission.
     */
    Mission& addMission(std::unique_ptr<Mission> mission);

    /**
     * @brief Create a mission through the mission factory.
     * @param type Factory key ("exploration", "combat", "colonization").
     * @param params Parameters: fleet, target, duration and type-specific keys.
     * @return The new mission.
     */
    Mission& spawnMission(const std::string& type, const nlohmann::json& params);

    /// @brief Look up a mission. @param id Id. @return Pointer or nullptr.
    [[nodiscard]] Mission* findMission(MissionId id) noexcept;
    /// @brief Look up a mission. @param id Id. @return Pointer or nullptr.
    [[nodiscard]] const Mission* findMission(MissionId id) const noexcept;
    /// @brief Every mission, ordered by id. @return Owning map (read-only).
    [[nodiscard]] const std::map<MissionId, std::unique_ptr<Mission>>& missions() const noexcept { return missions_; }
    /// @brief Number of missions (any status). @return Count.
    [[nodiscard]] std::size_t missionCount() const noexcept { return missions_.size(); }
    /// @brief Number of pending or active missions. @return Count.
    [[nodiscard]] std::size_t unfinishedMissionCount() const noexcept;
    /// @brief Whether a fleet is assigned to an active mission. @param fleet Fleet id. @return true if busy.
    [[nodiscard]] bool isFleetAssigned(EntityId fleet) const noexcept;
    /// @brief Delete finished missions. @return Number removed.
    std::size_t pruneFinishedMissions();

    // ---- infrastructure ----------------------------------------------------------------------

    /// @brief The resource ledger. @return Manager.
    [[nodiscard]] ResourceManager& resources() noexcept { return resources_; }
    /// @brief The resource ledger. @return Manager.
    [[nodiscard]] const ResourceManager& resources() const noexcept { return resources_; }
    /// @brief Factory used by spawn() and fromJson(); register custom kinds here. @return Factory.
    [[nodiscard]] EntityFactory& entityFactory() noexcept { return entityFactory_; }
    /// @brief Factory used by spawnMission() and fromJson(). @return Factory.
    [[nodiscard]] MissionFactory& missionFactory() noexcept { return missionFactory_; }

    /// @brief Serialise the whole world. @param out Destination object.
    void toJson(nlohmann::json& out) const;

    /**
     * @brief Rebuild a galaxy from toJson() output.
     * @param in Document.
     * @param entityFactory Factory for entities (defaults cover planets and fleets).
     * @param missionFactory Factory for missions.
     * @return The galaxy; throws SerializationException on malformed input.
     */
    [[nodiscard]] static std::unique_ptr<Galaxy> fromJson(const nlohmann::json& in,
                                                          EntityFactory entityFactory = makeDefaultEntityFactory(),
                                                          MissionFactory missionFactory = makeDefaultMissionFactory());

private:
    std::string name_;
    Vector3D min_;
    Vector3D max_;
    std::uint64_t nextEntityId_{1};
    std::uint64_t nextMissionId_{1};
    std::map<EntityId, std::unique_ptr<Entity>> entities_;
    std::map<MissionId, std::unique_ptr<Mission>> missions_;
    ResourceManager resources_;
    EntityFactory entityFactory_;
    MissionFactory missionFactory_;
};

} // namespace CppVerseHub::Core
