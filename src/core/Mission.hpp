/**
 * @file Mission.hpp
 * @brief Abstract mission with a Template Method lifecycle; concrete missions supply one step hook.
 *
 * The base class owns the state machine (Pending -> Active -> Completed | Failed | Cancelled), the
 * travel phase (the assigned fleet flies to the target planet), elapsed-time bookkeeping, failure
 * detection when the fleet or target disappears, and event publication. Derived classes implement
 * only `execute()`, which runs once per step while the fleet is on station and returns whether the
 * mission continues, succeeded or failed. Missions refer to entities by EntityId, never by raw
 * pointer, so a destroyed fleet cannot leave a dangling reference.
 */
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include <nlohmann/json_fwd.hpp>

#include "core/Identifiers.hpp"

namespace CppVerseHub::Core {

class Galaxy;
class EventBus;
class DeterministicRng;
class Fleet;
class Planet;

/// @brief Concrete mission category.
enum class MissionType : std::uint8_t { Exploration, Combat, Colonization };
/// @brief Mission lifecycle state.
enum class MissionStatus : std::uint8_t { Pending, Active, Completed, Failed, Cancelled };
/// @brief Sub-state of an active mission.
enum class MissionPhase : std::uint8_t { Travel, Execute };

/// @brief Lower-case name. @param type Type. @return Name such as "combat".
[[nodiscard]] std::string_view toString(MissionType type) noexcept;
/// @brief Parse a mission type. @param name Name. @return Type or std::nullopt.
[[nodiscard]] std::optional<MissionType> parseMissionType(std::string_view name) noexcept;
/// @brief Lower-case name. @param status Status. @return Name such as "active".
[[nodiscard]] std::string_view toString(MissionStatus status) noexcept;
/// @brief Parse a mission status. @param name Name. @return Status or std::nullopt.
[[nodiscard]] std::optional<MissionStatus> parseMissionStatus(std::string_view name) noexcept;

/// @brief Everything a mission may touch during a step.
struct MissionContext {
    Galaxy& galaxy;        ///< World state (entities and resource ledger).
    DeterministicRng& rng; ///< The simulation's only random source.
    EventBus& events;      ///< Where mission events are published.
};

/// @brief Abstract mission. See the file comment for the lifecycle.
class Mission {
public:
    /// @brief Tolerance used when comparing accumulated elapsed time with the duration.
    static constexpr double kTimeEpsilon = 1e-9;

    virtual ~Mission() = default;                ///< Virtual destructor.
    Mission(const Mission&) = delete;            ///< Not copyable (identity).
    Mission& operator=(const Mission&) = delete; ///< Not copy-assignable.
    Mission(Mission&&) = delete;                 ///< Not movable.
    Mission& operator=(Mission&&) = delete;      ///< Not move-assignable.

    /// @brief Id. @return Mission id.
    [[nodiscard]] MissionId id() const noexcept { return id_; }
    /// @brief Concrete type. @return Type.
    [[nodiscard]] virtual MissionType type() const noexcept = 0;
    /// @brief Assigned fleet. @return Fleet id.
    [[nodiscard]] EntityId fleetId() const noexcept { return fleet_; }
    /// @brief Target planet. @return Planet id.
    [[nodiscard]] EntityId targetId() const noexcept { return target_; }
    /// @brief Time on station required/allowed, in seconds. @return Duration.
    [[nodiscard]] double duration() const noexcept { return duration_; }
    /// @brief Time spent on station so far. @return Seconds.
    [[nodiscard]] double elapsed() const noexcept { return elapsed_; }
    /// @brief Fraction of the duration spent on station, 1 when completed. @return Progress in [0,1].
    [[nodiscard]] double progress() const noexcept;
    /// @brief Lifecycle state. @return Status.
    [[nodiscard]] MissionStatus status() const noexcept { return status_; }
    /// @brief Sub-state. @return Phase.
    [[nodiscard]] MissionPhase phase() const noexcept { return phase_; }
    /// @brief Why the mission failed (empty otherwise). @return Reason.
    [[nodiscard]] const std::string& failureReason() const noexcept { return failureReason_; }
    /// @brief Whether the mission reached a terminal state. @return true if completed, failed or cancelled.
    [[nodiscard]] bool isFinished() const noexcept;

    /**
     * @brief Activate a pending mission (publishes MissionStarted, or MissionFailed if preconditions fail).
     * @param ctx Simulation context.
     */
    void start(MissionContext& ctx);

    /// @brief Cancel a pending or active mission; no effect once finished.
    void cancel() noexcept;

    /**
     * @brief Advance an active mission by one step (no-op otherwise).
     * @param ctx Simulation context.
     * @param dt Step length in seconds.
     */
    void update(MissionContext& ctx, double dt);

    /// @brief One-line summary. @return Description.
    [[nodiscard]] virtual std::string describe() const;

    /// @brief Serialise (including "type"); accepted by the matching factory. @param out Destination object.
    virtual void toJson(nlohmann::json& out) const;

protected:
    /// @brief Result of one execute() call.
    enum class StepOutcome : std::uint8_t { Continue, Success, Failure };

    /**
     * @brief Construct a pending mission.
     * @param id Valid mission id.
     * @param fleet Valid fleet id.
     * @param target Valid planet id.
     * @param duration Positive, finite duration in seconds.
     */
    Mission(MissionId id, EntityId fleet, EntityId target, double duration);

    /**
     * @brief Construct from JSON: fleet, target, duration and optionally status/phase/elapsed/failureReason.
     * @param id Valid mission id.
     * @param params Parameters.
     */
    Mission(MissionId id, const nlohmann::json& params);

    /**
     * @brief Check type-specific start preconditions.
     * @param fleet Assigned fleet.
     * @param target Target planet.
     * @return Failure reason, or std::nullopt if the mission can start.
     */
    [[nodiscard]] virtual std::optional<std::string> checkStart(const Fleet& fleet, const Planet& target) const;

    /**
     * @brief One on-station step; elapsed() already includes dt.
     * @param ctx Simulation context.
     * @param fleet Assigned fleet (alive).
     * @param target Target planet (alive).
     * @param dt Step length.
     * @return Whether to continue, or the final outcome.
     */
    virtual StepOutcome execute(MissionContext& ctx, Fleet& fleet, Planet& target, double dt) = 0;

    /// @brief Whether elapsed() has reached duration(). @return true if time is up.
    [[nodiscard]] bool durationElapsed() const noexcept { return elapsed_ + kTimeEpsilon >= duration_; }

    /// @brief Record the reason the next Failure outcome should report. @param reason Reason text.
    void setFailureReason(std::string reason) { failureReason_ = std::move(reason); }

private:
    void fail(MissionContext& ctx, std::string reason);
    void complete(MissionContext& ctx);

    MissionId id_;
    EntityId fleet_;
    EntityId target_;
    double duration_;
    double elapsed_{0.0};
    MissionStatus status_{MissionStatus::Pending};
    MissionPhase phase_{MissionPhase::Travel};
    std::string failureReason_;
};

} // namespace CppVerseHub::Core
