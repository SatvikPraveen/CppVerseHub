/**
 * @file ColonizationMission.hpp
 * @brief Settle a non-hostile planet with colonists and the fleet's cargo.
 *
 * On completion one Colonizer ship is consumed, the colonists join the planet's population and the
 * fleet's entire resource account is *transferred* (not minted) to the planet, so the economy's
 * conservation invariant is preserved.
 */
#pragma once

#include "core/Mission.hpp"
#include "core/Resources.hpp"

namespace CppVerseHub::Core {

/// @brief Colonisation mission.
class ColonizationMission final : public Mission {
public:
    /**
     * @brief Construct.
     * @param id Mission id.
     * @param fleet Fleet id (must contain a Colonizer when the mission starts).
     * @param target Planet id.
     * @param duration Settlement time on station, seconds.
     * @param colonists Positive number of colonists delivered.
     */
    ColonizationMission(MissionId id, EntityId fleet, EntityId target, double duration, double colonists = 1000.0);

    /// @brief Construct from JSON (base keys plus "colonists", "delivered"). @param id Id. @param params Params.
    ColonizationMission(MissionId id, const nlohmann::json& params);

    /// @brief Type tag. @return MissionType::Colonization.
    [[nodiscard]] MissionType type() const noexcept override { return MissionType::Colonization; }
    /// @brief Colonists carried. @return Count.
    [[nodiscard]] double colonists() const noexcept { return colonists_; }
    /// @brief Cargo transferred on completion. @return Amounts.
    [[nodiscard]] const ResourceAmounts& delivered() const noexcept { return delivered_; }

    /// @brief Serialise. @param out Destination.
    void toJson(nlohmann::json& out) const override;

protected:
    /// @brief Require a Colonizer and a non-hostile target. @param fleet Fleet. @param target Planet. @return Reason.
    [[nodiscard]] std::optional<std::string> checkStart(const Fleet& fleet, const Planet& target) const override;
    /// @brief Settle once the duration has elapsed. @return Outcome.
    StepOutcome execute(MissionContext& ctx, Fleet& fleet, Planet& target, double dt) override;

private:
    double colonists_;
    ResourceAmounts delivered_;
};

} // namespace CppVerseHub::Core
