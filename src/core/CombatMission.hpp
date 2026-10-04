/**
 * @file CombatMission.hpp
 * @brief Assault a defended planet until its defences fall, the fleet dies, or time runs out.
 *
 * Each step both sides exchange fire scaled by a strategy multiplier and a random factor drawn from
 * the simulation's DeterministicRng. Success = planet defense reduced to zero.
 */
#pragma once

#include "core/Mission.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace CppVerseHub::Core {

/// @brief Combat posture.
enum class CombatStrategy : std::uint8_t { Aggressive, Balanced, Defensive };

/// @brief Lower-case name. @param s Strategy. @return Name.
[[nodiscard]] std::string_view toString(CombatStrategy s) noexcept;
/// @brief Parse a strategy name. @param name Name. @return Strategy or std::nullopt.
[[nodiscard]] std::optional<CombatStrategy> parseCombatStrategy(std::string_view name) noexcept;

/// @brief Combat mission.
class CombatMission final : public Mission {
public:
    /// @brief Fleet hull destroyed per unit of planetary defense per second.
    static constexpr double kDefenseFirepower = 0.05;

    /**
     * @brief Construct.
     * @param id Mission id.
     * @param fleet Fleet id.
     * @param target Planet id.
     * @param duration Maximum time on station before retreating, seconds.
     * @param strategy Combat posture.
     */
    CombatMission(MissionId id, EntityId fleet, EntityId target, double duration,
                  CombatStrategy strategy = CombatStrategy::Balanced);

    /// @brief Construct from JSON (base keys plus "strategy", "damageDealt", "damageTaken"). @param id Id.
    /// @param params Params.
    CombatMission(MissionId id, const nlohmann::json& params);

    /// @brief Type tag. @return MissionType::Combat.
    [[nodiscard]] MissionType type() const noexcept override { return MissionType::Combat; }
    /// @brief Posture. @return Strategy.
    [[nodiscard]] CombatStrategy strategy() const noexcept { return strategy_; }
    /// @brief Total defense removed from the planet. @return Damage.
    [[nodiscard]] double damageDealt() const noexcept { return damageDealt_; }
    /// @brief Total hull lost by the fleet. @return Damage.
    [[nodiscard]] double damageTaken() const noexcept { return damageTaken_; }

    /// @brief (Damage dealt multiplier, damage taken multiplier) of a strategy. @param s Strategy. @return
    /// Pair.
    [[nodiscard]] static constexpr std::pair<double, double> modifiers(CombatStrategy s) noexcept {
        switch (s) {
            case CombatStrategy::Aggressive:
                return {1.5, 1.3};
            case CombatStrategy::Defensive:
                return {0.7, 0.6};
            case CombatStrategy::Balanced:
                break;
        }
        return {1.0, 1.0};
    }

    /// @brief Serialise. @param out Destination.
    void toJson(nlohmann::json& out) const override;

protected:
    /// @brief Fail when the fleet carries no weapons. @param fleet Fleet. @param target Planet. @return
    /// Reason.
    [[nodiscard]] std::optional<std::string> checkStart(const Fleet& fleet,
                                                        const Planet& target) const override;
    /// @brief One exchange of fire. @return Outcome.
    StepOutcome execute(MissionContext& ctx, Fleet& fleet, Planet& target, double dt) override;

private:
    CombatStrategy strategy_;
    double damageDealt_{0.0};
    double damageTaken_{0.0};
};

} // namespace CppVerseHub::Core
