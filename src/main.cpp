/**
 * @file main.cpp
 * @brief Command-line front end for CppVerseHub.
 *
 * The CLI is non-interactive so every run is scriptable and reproducible:
 *
 * @code
 *   cppversehub list                         # list demonstration modules
 *   cppversehub demo all                     # run every module demonstration
 *   cppversehub demo concurrency             # run one module
 *   cppversehub simulate --seed 42 --steps 2000 --save run.json
 *   cppversehub replay run.json --steps 500  # resume a saved scenario
 * @endcode
 *
 * `simulate` prints a 64-bit state digest. Two runs with the same seed and parameters on the
 * same binary produce the same digest, which makes the simulation usable as a regression oracle.
 */

#include "algorithms/Demo.hpp"
#include "concurrency/Demo.hpp"
#include "core/Demo.hpp"
#include "core/Events.hpp"
#include "core/Galaxy.hpp"
#include "core/Mission.hpp"
#include "core/Scenario.hpp"
#include "core/SimulationEngine.hpp"
#include "memory/Demo.hpp"
#include "modern/Demo.hpp"
#include "patterns/Demo.hpp"
#include "stl_showcase/Demo.hpp"
#include "templates/Demo.hpp"
#include "utils/Demo.hpp"

#include <array>
#include <chrono>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <functional>
#include <iomanip>
#include <iostream>
#include <optional>
#include <ostream>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr std::string_view kVersion = "2.0.0";

/// @brief One registered module demonstration.
struct DemoEntry {
    std::string_view name;
    std::string_view description;
    void (*run)(std::ostream&);
};

constexpr std::array kDemos{
    DemoEntry{"core", "Deterministic space-fleet simulation domain", &CppVerseHub::Core::runDemo},
    DemoEntry{"templates", "Concepts, SFINAE, variadics, metaprogramming", &CppVerseHub::Templates::runDemo},
    DemoEntry{"patterns", "GoF design patterns in modern C++", &CppVerseHub::Patterns::runDemo},
    DemoEntry{"stl", "Containers, algorithms, iterators, functors", &CppVerseHub::STL::runDemo},
    DemoEntry{"memory", "Allocators, pools, RAII, smart pointers", &CppVerseHub::Memory::runDemo},
    DemoEntry{"concurrency", "Thread pools, lock-free queues, coroutines",
              &CppVerseHub::Concurrency::runDemo},
    DemoEntry{"modern", "C++20 language features", &CppVerseHub::Modern::runDemo},
    DemoEntry{"algorithms", "Sorting, searching, graphs, data structures", &CppVerseHub::Algorithms::runDemo},
    DemoEntry{"utils", "Logging, configuration, parsing, math", &CppVerseHub::Utils::runDemo},
};

void printUsage(std::ostream& out) {
    out << "CppVerseHub " << kVersion << " - Modern C++20 reference implementation\n\n"
        << "Usage:\n"
        << "  cppversehub list\n"
        << "  cppversehub demo <module|all>\n"
        << "  cppversehub simulate [--seed N] [--planets N] [--fleets N] [--steps N] [--dt S]\n"
        << "                       [--save FILE] [--quiet]\n"
        << "  cppversehub replay FILE [--steps N] [--save FILE] [--quiet]\n"
        << "  cppversehub --help | --version\n";
}

/// @brief Minimal "--key value" option parser.
class Options {
public:
    explicit Options(std::span<const std::string_view> args) : args_(args.begin(), args.end()) {}

    [[nodiscard]] bool flag(std::string_view key) const {
        for (const auto& a : args_) {
            if (a == key) {
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] std::optional<std::string_view> value(std::string_view key) const {
        for (std::size_t i = 0; i + 1 < args_.size(); ++i) {
            if (args_[i] == key) {
                return args_[i + 1];
            }
        }
        return std::nullopt;
    }

    [[nodiscard]] std::uint64_t integer(std::string_view key, std::uint64_t fallback) const {
        const auto v = value(key);
        if (!v) {
            return fallback;
        }
        try {
            std::size_t used = 0;
            const std::string s(*v);
            const auto parsed = std::stoull(s, &used);
            if (used != s.size()) {
                throw std::invalid_argument("trailing characters");
            }
            return parsed;
        } catch (const std::exception&) {
            throw std::invalid_argument("option " + std::string(key) + " expects a non-negative integer");
        }
    }

    [[nodiscard]] double real(std::string_view key, double fallback) const {
        const auto v = value(key);
        if (!v) {
            return fallback;
        }
        try {
            return std::stod(std::string(*v));
        } catch (const std::exception&) {
            throw std::invalid_argument("option " + std::string(key) + " expects a number");
        }
    }

private:
    std::vector<std::string_view> args_;
};

int cmdList(std::ostream& out) {
    out << "Available demonstrations:\n";
    for (const auto& d : kDemos) {
        out << "  " << std::left << std::setw(13) << d.name << d.description << '\n';
    }
    return 0;
}

int cmdDemo(std::string_view which, std::ostream& out) {
    bool found = false;
    for (const auto& d : kDemos) {
        if (which == "all" || which == d.name) {
            found = true;
            out << "\n==================== " << d.name << " ====================\n";
            const auto start = std::chrono::steady_clock::now();
            d.run(out);
            const std::chrono::duration<double, std::milli> elapsed = std::chrono::steady_clock::now() -
                                                                      start;
            out << "[" << d.name << " finished in " << std::fixed << std::setprecision(1) << elapsed.count()
                << " ms]\n";
            out.unsetf(std::ios::fixed);
        }
    }
    if (!found) {
        std::cerr << "Unknown module '" << which << "'. Run 'cppversehub list'.\n";
        return 2;
    }
    return 0;
}

void reportEngine(const CppVerseHub::Core::SimulationEngine& engine, std::ostream& out) {
    const auto& s = engine.stats();
    out << "Simulated time ...... " << engine.time() << " s (" << s.ticks << " ticks)\n"
        << "Missions started .... " << s.missionsStarted << '\n'
        << "Missions completed .. " << s.missionsCompleted << '\n'
        << "Missions failed ..... " << s.missionsFailed << '\n'
        << "Entities destroyed .. " << s.entitiesDestroyed << '\n'
        << "Resource shortages .. " << s.resourceShortages << '\n'
        << "State digest ........ 0x" << std::hex << std::setw(16) << std::setfill('0')
        << engine.stateDigest() << std::dec << std::setfill(' ') << '\n';
}

int runEngine(CppVerseHub::Core::SimulationEngine& engine, const Options& opts, std::uint64_t steps,
              std::ostream& out) {
    using namespace CppVerseHub::Core;
    const bool quiet = opts.flag("--quiet");

    std::vector<Subscription> subs;
    if (!quiet) {
        subs.push_back(engine.events().subscribe<MissionCompleted>([&](const MissionCompleted& e) {
            out << "  t=" << engine.time() << "s  mission #" << e.id.value() << " (" << toString(e.type)
                << ") completed\n";
        }));
        subs.push_back(engine.events().subscribe<MissionFailed>([&](const MissionFailed& e) {
            out << "  t=" << engine.time() << "s  mission #" << e.id.value() << " (" << toString(e.type)
                << ") failed: " << e.reason << '\n';
        }));
    }

    const auto start = std::chrono::steady_clock::now();
    engine.runSteps(steps);
    const std::chrono::duration<double, std::milli> elapsed = std::chrono::steady_clock::now() - start;
    subs.clear();

    reportEngine(engine, out);
    out << "Wall-clock time ..... " << elapsed.count() << " ms\n";

    if (const auto path = opts.value("--save")) {
        saveScenarioFile(engine, std::filesystem::path(std::string(*path)));
        out << "Saved scenario to " << *path << '\n';
    }
    return 0;
}

int cmdSimulate(const Options& opts, std::ostream& out) {
    using namespace CppVerseHub::Core;
    SampleScenarioOptions scenario;
    scenario.seed = opts.integer("--seed", scenario.seed);
    scenario.planets = static_cast<std::size_t>(opts.integer("--planets", scenario.planets));
    scenario.fleets = static_cast<std::size_t>(opts.integer("--fleets", scenario.fleets));
    scenario.timeStep = opts.real("--dt", scenario.timeStep);
    const auto steps = opts.integer("--steps", 1000);

    auto engine = makeSampleScenario(scenario);
    out << "Simulating seed=" << scenario.seed << " planets=" << scenario.planets
        << " fleets=" << scenario.fleets << " dt=" << scenario.timeStep << " steps=" << steps << '\n';
    return runEngine(*engine, opts, steps, out);
}

int cmdReplay(std::string_view file, const Options& opts, std::ostream& out) {
    using namespace CppVerseHub::Core;
    auto engine = loadScenarioFile(std::filesystem::path(std::string(file)));
    out << "Loaded " << file << " at t=" << engine->time() << "s\n";
    return runEngine(*engine, opts, opts.integer("--steps", 1000), out);
}

} // namespace

int main(int argc, char* argv[]) {
    const std::vector<std::string_view> args(argv + 1, argv + argc);
    std::ostream& out = std::cout;

    try {
        if (args.empty() || args[0] == "--help" || args[0] == "-h" || args[0] == "help") {
            printUsage(out);
            return args.empty() ? 1 : 0;
        }
        if (args[0] == "--version") {
            out << "CppVerseHub " << kVersion << '\n';
            return 0;
        }

        const Options opts(std::span<const std::string_view>(args).subspan(1));
        if (args[0] == "list") {
            return cmdList(out);
        }
        if (args[0] == "demo") {
            return cmdDemo(args.size() > 1 ? args[1] : "all", out);
        }
        if (args[0] == "simulate") {
            return cmdSimulate(opts, out);
        }
        if (args[0] == "replay") {
            if (args.size() < 2) {
                std::cerr << "replay requires a scenario file\n";
                return 2;
            }
            return cmdReplay(args[1], opts, out);
        }

        std::cerr << "Unknown command '" << args[0] << "'\n\n";
        printUsage(std::cerr);
        return 2;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << '\n';
        return 1;
    }
}
