/**
 * @file CombatMission.cpp
 * @brief CombatMission implementation.
 */
#include "core/CombatMission.hpp"

#include <array>

#include <nlohmann/json.hpp>

#include "core/EventSystem.hpp"
#include "core/Events.hpp"
#include "core/Exceptions.hpp"
#include "core/Galaxy.hpp"
#include "core/Random.hpp"

namespace CppVerseHub::Core {

namespace {
constexpr std::array<std::string_view, 3> kNames{"aggressive", "balanced", "defensive"};
} // namespace

std::string_view toString(CombatStrategy s) noexcept {
    const auto i = static_cast<std::size_t>(s);
    return i < kNames.size() ? kNames[i] : std::string_view{"unknown"};
}

std::optional<CombatStrategy> parseCombatStrategy(std::string_view name) noexcept {
    for (std::size_t i = 0; i < kNames.size(); ++i) {
        if (kNames[i] == name) {
            return static_cast<CombatStrategy>(i);
        }
    }
    return std::nullopt;
}

CombatMission::CombatMission(MissionId id, EntityId fleet, EntityId target, double duration, CombatStrategy strategy)
    : Mission(id, fleet, target, duration), strategy_(strategy) {}

CombatMission::CombatMission(MissionId id, const nlohmann::json& params)
    : Mission(id, params), strategy_(CombatStrategy::Balanced) {
    if (params.contains("strategy")) {
        const auto s = parseCombatStrategy(params["strategy"].get<std::string>());
        if (!s) {
            throw InvalidArgumentException("unknown combat strategy");
        }
        strategy_ = *s;
    }
    damageDealt_ = params.value("damageDealt", 0.0);
    damageTaken_ = params.value("damageTaken", 0.0);
}

std::optional<std::string> CombatMission::checkStart(const Fleet& fleet, const Planet& target) const {
    if (auto base = Mission::checkStart(fleet, target)) {
        return base;
    }
    if (fleet.attackPower() <= 0.0) {
        return std::string("fleet has no weapons");
    }
    return std::nullopt;
}

Mission::StepOutcome CombatMission::execute(MissionContext& ctx, Fleet& fleet, Planet& target, double dt) {
    if (!target.isHostile()) {
        return StepOutcome::Success;
    }
    const auto [dealtFactor, takenFactor] = modifiers(strategy_);
    // Fixed draw order (fleet volley, then planet volley) keeps the RNG stream reproducible.
    const double volley = fleet.attackPower() * dealtFactor * dt * ctx.rng.uniform(0.5, 1.5);
    const double response = target.defense() * kDefenseFirepower * takenFactor * dt * ctx.rng.uniform(0.5, 1.5);
    const double dealt = volley < target.defense() ? volley : target.defense();
    target.setDefense(target.defense() - dealt);
    const double hullBefore = fleet.totalHull();
    fleet.takeDamage(response);
    const double taken = hullBefore - fleet.totalHull();
    damageDealt_ += dealt;
    damageTaken_ += taken;
    ctx.events.publish(CombatExchange{id(), dealt, taken});

    if (!target.isHostile()) {
        return StepOutcome::Success;
    }
    if (!fleet.isAlive()) {
        setFailureReason("fleet destroyed");
        return StepOutcome::Failure;
    }
    if (durationElapsed()) {
        setFailureReason("retreated: defences still standing");
        return StepOutcome::Failure;
    }
    return StepOutcome::Continue;
}

void CombatMission::toJson(nlohmann::json& out) const {
    Mission::toJson(out);
    out["strategy"] = std::string(toString(strategy_));
    out["damageDealt"] = damageDealt_;
    out["damageTaken"] = damageTaken_;
}

} // namespace CppVerseHub::Core
