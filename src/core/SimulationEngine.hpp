/**
 * @file SimulationEngine.hpp
 * @brief Deterministic fixed-timestep simulation driver.
 *
 * The engine advances a Galaxy in fixed steps of `timeStep` seconds ("fix your timestep"): a
 * real-time `advance(seconds)` call accumulates time and runs as many whole steps as fit, carrying the
 * remainder, so the simulated trajectory never depends on how the caller slices wall-clock time.
 * Every source of randomness flows from one seeded `std::mt19937_64` (wrapped by DeterministicRng)
 * and every collection is iterated in id order, so two engines built from the same scenario and
 * seed produce bit-identical states (`stateDigest()` makes that checkable). The engine is
 * single-threaded by design: determinism is a stronger guarantee than parallel speed-up here.
 *
 * Step order: (1) start pending missions whose fleets are free, (2) update active missions,
 * (3) update entities, (4) sync entity resource flows into the ledger and tick the economy,
 * (5) remove destroyed entities, (6) publish StepCompleted.
 *
 * Event handlers run synchronously inside step(); they may read the galaxy and add entities or
 * missions, but must not remove them while a step is in progress.
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

#include <nlohmann/json_fwd.hpp>

#include "core/EventSystem.hpp"
#include "core/Galaxy.hpp"
#include "core/Random.hpp"

namespace CppVerseHub::Core {

/// @brief Engine configuration.
struct SimulationConfig {
    double timeStep{0.1};                          ///< Fixed step length in seconds (> 0).
    std::uint64_t seed{DeterministicRng::kDefaultSeed}; ///< RNG seed.
    std::size_t maxStepsPerAdvance{1000};          ///< Cap per advance() call (prevents a "spiral of death").
    bool pruneFinishedMissions{false};             ///< Delete finished missions at the end of each step.
};

/// @brief Counters accumulated by the engine.
struct SimulationStats {
    std::uint64_t ticks{0};               ///< Steps executed.
    std::uint64_t missionsStarted{0};     ///< Missions activated.
    std::uint64_t missionsCompleted{0};   ///< Missions completed successfully.
    std::uint64_t missionsFailed{0};      ///< Missions failed.
    std::uint64_t entitiesDestroyed{0};   ///< Entities removed after destruction.
    std::uint64_t resourceShortages{0};   ///< Shortfall events.

    /// @brief Equality. @return true if every counter matches.
    friend bool operator==(const SimulationStats&, const SimulationStats&) = default;
};

/// @brief Fixed-timestep, seeded, deterministic simulation of one Galaxy.
class SimulationEngine {
public:
    /**
     * @brief Take ownership of a galaxy.
     * @param galaxy Non-null galaxy.
     * @param config Configuration (timeStep must be positive and finite).
     */
    explicit SimulationEngine(std::unique_ptr<Galaxy> galaxy, SimulationConfig config = {});

    SimulationEngine(const SimulationEngine&) = delete;            ///< Not copyable.
    SimulationEngine& operator=(const SimulationEngine&) = delete; ///< Not copy-assignable.
    SimulationEngine(SimulationEngine&&) = delete;                 ///< Not movable (subscriptions refer to the bus).
    SimulationEngine& operator=(SimulationEngine&&) = delete;      ///< Not move-assignable.
    ~SimulationEngine();                                           ///< Destructor.

    /// @brief Execute exactly one fixed step.
    void step();

    /// @brief Execute n steps. @param n Step count.
    void runSteps(std::uint64_t n);

    /**
     * @brief Accumulate wall-clock time and run every whole step that fits (capped by maxStepsPerAdvance).
     * @param seconds Non-negative, finite elapsed time.
     * @return Number of steps executed.
     */
    std::size_t advance(double seconds);

    /**
     * @brief Run until a predicate holds (checked before each step) or a step limit is reached.
     * @param done Predicate evaluated on the engine.
     * @param maxSteps Upper bound on steps.
     * @return Steps executed.
     */
    std::uint64_t runUntil(const std::function<bool(const SimulationEngine&)>& done, std::uint64_t maxSteps);

    /// @brief Run until no mission is pending or active. @param maxSteps Step limit. @return Steps executed.
    std::uint64_t runUntilMissionsFinished(std::uint64_t maxSteps);

    /// @brief The world. @return Galaxy.
    [[nodiscard]] Galaxy& galaxy() noexcept { return *galaxy_; }
    /// @brief The world. @return Galaxy.
    [[nodiscard]] const Galaxy& galaxy() const noexcept { return *galaxy_; }
    /// @brief The event bus; subscribe here to observe the simulation. @return Bus.
    [[nodiscard]] EventBus& events() noexcept { return events_; }
    /// @brief The random source. @return RNG.
    [[nodiscard]] DeterministicRng& rng() noexcept { return rng_; }
    /// @brief Configuration. @return Config.
    [[nodiscard]] const SimulationConfig& config() const noexcept { return config_; }
    /// @brief Counters. @return Stats.
    [[nodiscard]] const SimulationStats& stats() const noexcept { return stats_; }
    /// @brief Steps executed. @return Tick count.
    [[nodiscard]] std::uint64_t tick() const noexcept { return stats_.ticks; }
    /// @brief Simulated time, computed as tick * timeStep (no accumulated rounding drift). @return Seconds.
    [[nodiscard]] double time() const noexcept { return static_cast<double>(stats_.ticks) * config_.timeStep; }
    /// @brief Unconsumed time in the advance() accumulator. @return Seconds in [0, timeStep).
    [[nodiscard]] double accumulator() const noexcept { return accumulator_; }

    /// @brief Serialise the complete engine state (config, counters, RNG state, galaxy). @param out Destination.
    void toJson(nlohmann::json& out) const;

    /**
     * @brief Rebuild an engine from toJson() output; continuing it reproduces the original exactly.
     * @param in Document.
     * @return Engine; throws SerializationException on malformed input.
     */
    [[nodiscard]] static std::unique_ptr<SimulationEngine> fromJson(const nlohmann::json& in);

    /**
     * @brief 64-bit FNV-1a digest of the serialised state.
     *
     * Doubles are serialised with round-trip precision, so equal digests mean bit-identical states
     * (up to the usual hash-collision caveat).
     * @return Digest.
     */
    [[nodiscard]] std::uint64_t stateDigest() const;

private:
    void startPendingMissions(MissionContext& ctx);
    void tickEconomy();
    void removeDestroyed();

    std::unique_ptr<Galaxy> galaxy_;
    SimulationConfig config_;
    DeterministicRng rng_;
    EventBus events_;
    SimulationStats stats_;
    double accumulator_{0.0};
    Subscription startedCounter_;
    Subscription completedCounter_;
    Subscription failedCounter_;
};

} // namespace CppVerseHub::Core
