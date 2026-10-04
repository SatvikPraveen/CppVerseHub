/**
 * @file Scenario.hpp
 * @brief Scenario persistence (JSON via nlohmann::json) and a procedural sample-scenario generator.
 *
 * A scenario document captures the *entire* engine state - configuration, counters, the exact
 * mt19937_64 state, every entity, mission and ledger account - so that
 * `load(save(engine))` continues bit-identically to `engine` itself. The generator builds
 * reproducible worlds from a seed for demos, tests and benchmarks.
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>

#include <nlohmann/json.hpp>

#include "core/Galaxy.hpp"
#include "core/SimulationEngine.hpp"

namespace CppVerseHub::Core {

/// @brief Serialise an engine to a scenario document. @param engine Engine. @return Document.
[[nodiscard]] nlohmann::json saveScenario(const SimulationEngine& engine);

/// @brief Rebuild an engine from a scenario document. @param document Document. @return Engine.
[[nodiscard]] std::unique_ptr<SimulationEngine> loadScenario(const nlohmann::json& document);

/**
 * @brief Write a scenario to a file (pretty-printed JSON).
 * @param engine Engine to save.
 * @param path Destination path; throws SerializationException if it cannot be written.
 */
void saveScenarioFile(const SimulationEngine& engine, const std::filesystem::path& path);

/**
 * @brief Read a scenario file.
 * @param path Source path; throws SerializationException if unreadable or malformed.
 * @return Engine.
 */
[[nodiscard]] std::unique_ptr<SimulationEngine> loadScenarioFile(const std::filesystem::path& path);

/// @brief Parameters of the procedural sample scenario.
struct SampleScenarioOptions {
    std::uint64_t seed{DeterministicRng::kDefaultSeed}; ///< Seed for both world generation and the engine.
    std::size_t planets{8};                            ///< Number of planets (>= 1).
    std::size_t fleets{4};                             ///< Number of fleets; each receives one mission.
    double timeStep{0.1};                              ///< Engine time step.
    double radius{500.0};                              ///< Planets are placed within [-radius, radius]^3.
};

/**
 * @brief Build a reproducible galaxy: planets with production/defences, fleets with fuel and cargo,
 *        and a mix of exploration, combat and colonisation missions.
 * @param options Generation parameters.
 * @return Galaxy.
 */
[[nodiscard]] std::unique_ptr<Galaxy> makeSampleGalaxy(const SampleScenarioOptions& options = {});

/// @brief makeSampleGalaxy wrapped in an engine seeded with options.seed. @param options Parameters. @return Engine.
[[nodiscard]] std::unique_ptr<SimulationEngine> makeSampleScenario(const SampleScenarioOptions& options = {});

} // namespace CppVerseHub::Core
