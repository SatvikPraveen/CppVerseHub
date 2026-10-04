/**
 * @file Demo.cpp
 * @brief Implementation of `CppVerseHub::Utils::runDemo`.
 */
#include "utils/Demo.hpp"

#include "utils/ConfigManager.hpp"
#include "utils/FileParser.hpp"
#include "utils/Logger.hpp"
#include "utils/MathUtils.hpp"
#include "utils/StringUtils.hpp"
#include "utils/TimeUtils.hpp"

#include <exception>
#include <functional>
#include <utility>

namespace CppVerseHub::Utils {

void runDemo(std::ostream& out) try {
    using Showcase = std::pair<const char*, std::function<void(std::ostream&)>>;
    const Showcase showcases[] = {
        {"logging", [](std::ostream& o) { demonstrateLogging(o); }},
        {"config", [](std::ostream& o) { demonstrateConfig(o); }},
        {"parsing", [](std::ostream& o) { demonstrateParsing(o); }},
        {"math", [](std::ostream& o) { Math::demonstrateMath(o); }},
        {"strings", [](std::ostream& o) { String::demonstrateStrings(o); }},
        {"time", [](std::ostream& o) { Time::demonstrateTime(o); }},
    };
    for (const auto& [name, run] : showcases) {
        try {
            run(out);
        } catch (const std::exception& e) {
            out << "[utils demo '" << name << "' failed: " << e.what() << "]\n";
        } catch (...) {
            out << "[utils demo '" << name << "' failed with an unknown exception]\n";
        }
        out << '\n';
    }
} catch (...) { // NOLINT(bugprone-empty-catch): runDemo must never throw (e.g. on a failing stream)
}

} // namespace CppVerseHub::Utils
