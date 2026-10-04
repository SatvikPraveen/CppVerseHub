/**
 * @file Factory.cpp
 * @brief Default factory registrations.
 */
#include "core/Factory.hpp"

#include "core/ColonizationMission.hpp"
#include "core/CombatMission.hpp"
#include "core/ExplorationMission.hpp"
#include "core/Fleet.hpp"
#include "core/Planet.hpp"

#include <nlohmann/json.hpp>

namespace CppVerseHub::Core {

EntityFactory makeDefaultEntityFactory() {
    EntityFactory factory;
    factory.registerType<Planet>("planet");
    factory.registerType<Fleet>("fleet");
    return factory;
}

MissionFactory makeDefaultMissionFactory() {
    MissionFactory factory;
    factory.registerType<ExplorationMission>("exploration");
    factory.registerType<CombatMission>("combat");
    factory.registerType<ColonizationMission>("colonization");
    return factory;
}

} // namespace CppVerseHub::Core
