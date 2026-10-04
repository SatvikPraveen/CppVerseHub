/**
 * @file Events.hpp
 * @brief Plain event structs published by the simulation on its EventBus.
 *
 * Events are small aggregates with no common base class; the EventBus dispatches on their static
 * type. Each carries ids rather than pointers so handlers can safely outlive the step that raised it.
 */
#pragma once

#include <cstdint>
#include <string>

#include "core/Entity.hpp"
#include "core/Identifiers.hpp"
#include "core/Mission.hpp"
#include "core/Resources.hpp"

namespace CppVerseHub::Core {

/// @brief An entity was destroyed and removed from the galaxy.
struct EntityDestroyed {
    EntityId id;      ///< Removed entity.
    EntityKind kind;  ///< Its kind.
    std::string name; ///< Its name.
};

/// @brief A mission became active.
struct MissionStarted {
    MissionId id;     ///< Mission.
    MissionType type; ///< Its type.
    EntityId fleet;   ///< Assigned fleet.
    EntityId target;  ///< Target planet.
};

/// @brief A mission finished successfully.
struct MissionCompleted {
    MissionId id;     ///< Mission.
    MissionType type; ///< Its type.
};

/// @brief A mission failed.
struct MissionFailed {
    MissionId id;       ///< Mission.
    MissionType type;   ///< Its type.
    std::string reason; ///< Why.
};

/// @brief One exchange of fire during a combat mission.
struct CombatExchange {
    MissionId mission;     ///< Combat mission.
    double damageToPlanet; ///< Defense removed from the planet.
    double damageToFleet;  ///< Hull removed from the fleet.
};

/// @brief An exploration mission discovered a deposit.
struct ResourceDiscovered {
    EntityId planet;       ///< Planet surveyed.
    ResourceType type;     ///< Resource found.
    ResourceAmount amount; ///< Units deposited into the planet's account.
};

/// @brief An entity could not consume what it needed this step.
struct ResourceShortage {
    EntityId entity;          ///< Consumer.
    ResourceType type;        ///< Resource lacking.
    ResourceAmount shortfall; ///< Units lacking.
};

/// @brief A simulation step finished.
struct StepCompleted {
    std::uint64_t tick; ///< Number of steps completed so far.
    double time;        ///< Simulated time in seconds (tick * timeStep).
};

} // namespace CppVerseHub::Core
