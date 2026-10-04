/**
 * @file ExplorationMission.cpp
 * @brief ExplorationMission implementation.
 */
#include "core/ExplorationMission.hpp"

#include <nlohmann/json.hpp>

#include "core/EventSystem.hpp"
#include "core/Events.hpp"
#include "core/Exceptions.hpp"
#include "core/Galaxy.hpp"
#include "core/Random.hpp"

namespace CppVerseHub::Core {

ExplorationMission::ExplorationMission(MissionId id, EntityId fleet, EntityId target, double duration,
                                       bool surveyResources)
    : Mission(id, fleet, target, duration), survey_(surveyResources) {}

ExplorationMission::ExplorationMission(MissionId id, const nlohmann::json& params)
    : Mission(id, params), survey_(params.value("surveyResources", true)) {
    if (params.contains("discovered")) {
        for (const auto& [key, value] : params["discovered"].items()) {
            const auto t = parseResourceType(key);
            if (!t) {
                throw SerializationException("unknown resource type '" + key + "'");
            }
            discovered_[*t] = value.get<ResourceAmount>();
        }
    }
}

Mission::StepOutcome ExplorationMission::execute(MissionContext& ctx, Fleet& fleet, Planet& target, double dt) {
    static_cast<void>(fleet);
    static_cast<void>(dt);
    if (!durationElapsed()) {
        return StepOutcome::Continue;
    }
    if (!target.isExplored() && survey_) {
        const double scale = 0.5 + target.habitability();
        for (ResourceType t : kAllResourceTypes) {
            if (ctx.rng.chance(kDiscoveryChance)) {
                const auto base = ctx.rng.uniformInt(kMinDeposit, kMaxDeposit);
                const auto amount = static_cast<ResourceAmount>(static_cast<double>(base) * scale);
                ctx.galaxy.resources().deposit(target.id(), t, amount);
                discovered_[t] += amount;
                ctx.events.publish(ResourceDiscovered{target.id(), t, amount});
            }
        }
    }
    target.setExplored(true);
    return StepOutcome::Success;
}

void ExplorationMission::toJson(nlohmann::json& out) const {
    Mission::toJson(out);
    out["surveyResources"] = survey_;
    nlohmann::json d = nlohmann::json::object();
    for (ResourceType t : kAllResourceTypes) {
        if (discovered_[t] != 0) {
            d[std::string(toString(t))] = discovered_[t];
        }
    }
    out["discovered"] = std::move(d);
}

} // namespace CppVerseHub::Core
