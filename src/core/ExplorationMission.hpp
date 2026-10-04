/**
 * @file ExplorationMission.hpp
 * @brief Survey a planet; on completion it is marked explored and random deposits are discovered.
 *
 * The discovery roll uses the simulation's DeterministicRng, so the same seed always yields the same
 * deposits. Discovered resources are minted into the planet's ResourceManager account.
 */
#pragma once

#include "core/Mission.hpp"
#include "core/Resources.hpp"

namespace CppVerseHub::Core {

/// @brief Exploration mission.
class ExplorationMission final : public Mission {
public:
    /// @brief Probability that a given resource type is found.
    static constexpr double kDiscoveryChance = 0.6;
    /// @brief Minimum deposit size before habitability scaling.
    static constexpr ResourceAmount kMinDeposit = 50;
    /// @brief Maximum deposit size before habitability scaling.
    static constexpr ResourceAmount kMaxDeposit = 500;

    /**
     * @brief Construct.
     * @param id Mission id.
     * @param fleet Fleet id.
     * @param target Planet id.
     * @param duration Survey time on station, seconds.
     * @param surveyResources Whether deposits are rolled on completion.
     */
    ExplorationMission(MissionId id, EntityId fleet, EntityId target, double duration,
                       bool surveyResources = true);

    /// @brief Construct from JSON (base keys plus "surveyResources", "discovered"). @param id Id. @param
    /// params Params.
    ExplorationMission(MissionId id, const nlohmann::json& params);

    /// @brief Type tag. @return MissionType::Exploration.
    [[nodiscard]] MissionType type() const noexcept override { return MissionType::Exploration; }
    /// @brief Whether deposits are rolled. @return Flag.
    [[nodiscard]] bool surveysResources() const noexcept { return survey_; }
    /// @brief Deposits found on completion. @return Amounts per type.
    [[nodiscard]] const ResourceAmounts& discovered() const noexcept { return discovered_; }

    /// @brief Serialise. @param out Destination.
    void toJson(nlohmann::json& out) const override;

protected:
    /// @brief Succeeds once the duration has elapsed. @return Outcome.
    StepOutcome execute(MissionContext& ctx, Fleet& fleet, Planet& target, double dt) override;

private:
    bool survey_;
    ResourceAmounts discovered_;
};

} // namespace CppVerseHub::Core
