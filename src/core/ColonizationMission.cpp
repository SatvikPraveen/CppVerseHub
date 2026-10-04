/**
 * @file ColonizationMission.cpp
 * @brief ColonizationMission implementation.
 */
#include "core/ColonizationMission.hpp"

#include <cmath>

#include <nlohmann/json.hpp>

#include "core/Exceptions.hpp"
#include "core/Galaxy.hpp"

namespace CppVerseHub::Core {

namespace {
double validatedColonists(double c) {
    if (!std::isfinite(c) || c <= 0.0) {
        throw InvalidArgumentException("colonists must be positive and finite");
    }
    return c;
}
} // namespace

ColonizationMission::ColonizationMission(MissionId id, EntityId fleet, EntityId target, double duration,
                                         double colonists)
    : Mission(id, fleet, target, duration), colonists_(validatedColonists(colonists)) {}

ColonizationMission::ColonizationMission(MissionId id, const nlohmann::json& params)
    : Mission(id, params), colonists_(validatedColonists(params.value("colonists", 1000.0))) {
    if (params.contains("delivered")) {
        for (const auto& [key, value] : params["delivered"].items()) {
            const auto t = parseResourceType(key);
            if (!t) {
                throw SerializationException("unknown resource type '" + key + "'");
            }
            delivered_[*t] = value.get<ResourceAmount>();
        }
    }
}

std::optional<std::string> ColonizationMission::checkStart(const Fleet& fleet, const Planet& target) const {
    if (auto base = Mission::checkStart(fleet, target)) {
        return base;
    }
    if (fleet.shipCount(ShipType::Colonizer) == 0) {
        return std::string("fleet has no colonizer ship");
    }
    if (target.isHostile()) {
        return std::string("target is hostile");
    }
    return std::nullopt;
}

Mission::StepOutcome ColonizationMission::execute(MissionContext& ctx, Fleet& fleet, Planet& target, double dt) {
    static_cast<void>(dt);
    if (target.isHostile()) {
        setFailureReason("target is hostile");
        return StepOutcome::Failure;
    }
    if (fleet.shipCount(ShipType::Colonizer) == 0) {
        setFailureReason("colonizer ship lost");
        return StepOutcome::Failure;
    }
    if (!durationElapsed()) {
        return StepOutcome::Continue;
    }
    delivered_ = ctx.galaxy.resources().transferAll(fleet.id(), target.id());
    target.setPopulation(target.population() + colonists_);
    static_cast<void>(fleet.removeShips(ShipType::Colonizer, 1));
    return StepOutcome::Success;
}

void ColonizationMission::toJson(nlohmann::json& out) const {
    Mission::toJson(out);
    out["colonists"] = colonists_;
    nlohmann::json d = nlohmann::json::object();
    for (ResourceType t : kAllResourceTypes) {
        if (delivered_[t] != 0) {
            d[std::string(toString(t))] = delivered_[t];
        }
    }
    out["delivered"] = std::move(d);
}

} // namespace CppVerseHub::Core
