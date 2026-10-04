/**
 * @file mission_event_log.cpp
 * @brief Builds a small galaxy by hand and observes it through the typed event bus.
 *
 * Demonstrates the core API: creating planets and fleets, assigning missions through the
 * Galaxy, subscribing to strongly typed events with RAII subscriptions, and running the
 * fixed-timestep engine until every mission has finished.
 */

#include "core/Events.hpp"
#include "core/Galaxy.hpp"
#include "core/Mission.hpp"
#include "core/Scenario.hpp"
#include "core/SimulationEngine.hpp"

#include <iostream>
#include <map>
#include <string>

namespace core = CppVerseHub::Core;

int main() {
    core::SampleScenarioOptions options;
    options.seed = 7;
    options.planets = 6;
    options.fleets = 5;
    auto engine = core::makeSampleScenario(options);

    std::map<std::string, int> tally;
    const auto onStart = engine->events().subscribe<core::MissionStarted>([&](const core::MissionStarted& e) {
        std::cout << "[t=" << engine->time() << "] mission #" << e.id.value() << " ("
                  << core::toString(e.type) << ") started: fleet #" << e.fleet.value() << " -> planet #"
                  << e.target.value() << '\n';
        ++tally["started"];
    });
    const auto onDone = engine->events().subscribe<core::MissionCompleted>(
        [&](const core::MissionCompleted& e) {
            std::cout << "[t=" << engine->time() << "] mission #" << e.id.value() << " completed\n";
            ++tally["completed"];
        });
    const auto onFail = engine->events().subscribe<core::MissionFailed>([&](const core::MissionFailed& e) {
        std::cout << "[t=" << engine->time() << "] mission #" << e.id.value() << " failed: " << e.reason
                  << '\n';
        ++tally["failed"];
    });

    const auto steps = engine->runUntilMissionsFinished(20000);

    std::cout << "\nFinished after " << steps << " steps (" << engine->time() << " simulated seconds)\n";
    for (const auto& [what, count] : tally) {
        std::cout << "  " << what << ": " << count << '\n';
    }
    std::cout << "Missions tracked by galaxy: " << engine->galaxy().missions().size() << '\n';
    return 0;
}
