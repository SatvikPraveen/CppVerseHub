/**
 * @file deterministic_replay.cpp
 * @brief Shows that the simulation is a pure function of (scenario, seed, step count).
 *
 * 1. Two engines built from the same seed are stepped independently; their digests must match.
 * 2. One engine is checkpointed to JSON half-way, restored, and continued; the restored run must
 *    reach the same digest as the uninterrupted one.
 *
 * The program exits with a non-zero status if either property is violated, so it doubles as a
 * smoke test under CTest.
 */

#include "core/Scenario.hpp"
#include "core/SimulationEngine.hpp"

#include <cstdint>
#include <iomanip>
#include <iostream>

namespace core = CppVerseHub::Core;

namespace {

void printDigest(const char* label, std::uint64_t digest) {
    std::cout << std::left << std::setw(28) << label << "0x" << std::hex << std::setw(16) << std::setfill('0')
              << std::right << digest << std::dec << std::setfill(' ') << '\n';
}

} // namespace

int main() {
    constexpr std::uint64_t kSeed = 20261004;
    constexpr std::uint64_t kSteps = 1200;

    core::SampleScenarioOptions options;
    options.seed = kSeed;
    options.planets = 12;
    options.fleets = 6;

    // 1. Independent runs with the same seed.
    auto a = core::makeSampleScenario(options);
    auto b = core::makeSampleScenario(options);
    a->runSteps(kSteps);
    b->runSteps(kSteps);
    printDigest("run A digest:", a->stateDigest());
    printDigest("run B digest:", b->stateDigest());

    // 2. Checkpoint half-way, restore, continue.
    auto c = core::makeSampleScenario(options);
    c->runSteps(kSteps / 2);
    const auto checkpoint = core::saveScenario(*c);
    auto restored = core::loadScenario(checkpoint);
    restored->runSteps(kSteps - kSteps / 2);
    printDigest("checkpoint/restore digest:", restored->stateDigest());

    // A different seed must (with overwhelming probability) diverge.
    options.seed = kSeed + 1;
    auto d = core::makeSampleScenario(options);
    d->runSteps(kSteps);
    printDigest("different seed digest:", d->stateDigest());

    const bool reproducible = a->stateDigest() == b->stateDigest();
    const bool resumable = a->stateDigest() == restored->stateDigest();
    std::cout << "\nreproducible: " << std::boolalpha << reproducible << "\nresumable:    " << resumable
              << '\n';
    return reproducible && resumable ? 0 : 1;
}
