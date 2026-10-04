/**
 * @file StructuredBindings.hpp
 * @brief Structured bindings (C++17, extended in C++20): decomposing tuples, pairs, arrays, aggregates
 *        and user-defined tuple-like types.
 *
 * `auto [a, b, c] = expr;` introduces names for the parts of an object. This header demonstrates the
 * three binding protocols defined by the standard:
 *  1. arrays (`std::array`, C arrays) — `centerOfMass`;
 *  2. tuple-like types (`std::tuple`, `std::pair`, and *our own* `ShipRecord`, which opts in through
 *     `std::tuple_size` / `std::tuple_element` / `get<I>`) — `orbitParameters`, `jumpDistance`;
 *  3. aggregates with public data members — `SpaceCoordinate`, `FleetStats`, `FuelRange`.
 * It also shows binding by reference to mutate elements in place, decomposing map entries and
 * `insert`/`try_emplace` results, and C++20 capture of structured bindings in lambdas.
 */
#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <map>
#include <numbers>
#include <optional>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace CppVerseHub::Modern::StructuredBindings {

// ===== AGGREGATES (binding protocol 3) =====

/// @brief A point in 3D space.
struct SpaceCoordinate {
    double x = 0.0; ///< X.
    double y = 0.0; ///< Y.
    double z = 0.0; ///< Z.
};

/// @brief A planet with a position and mass.
struct PlanetInfo {
    std::string name;         ///< Name.
    double mass = 0.0;        ///< Mass (arbitrary units).
    SpaceCoordinate position; ///< Location.
    bool habitable = false;   ///< Supports life.
};

/// @brief Fleet statistics.
struct FleetStats {
    std::string commander;       ///< Commanding officer.
    int shipCount = 0;           ///< Ship count.
    double fuelPercentage = 0.0; ///< Fuel 0..100.
    std::string missionType;     ///< Current mission.
};

/// @brief A mission report.
struct MissionReport {
    int missionId = 0;       ///< Identifier.
    std::string type;        ///< Mission type.
    double completion = 0.0; ///< Completion 0..100.
    int priority = 0;        ///< Priority.
};

/// @brief Result of a min/max scan: returning a named struct is often clearer than a tuple.
struct FuelRange {
    double minimum = 0.0; ///< Lowest fuel.
    double maximum = 0.0; ///< Highest fuel.
};

// ===== USER-DEFINED TUPLE-LIKE TYPE (binding protocol 2) =====

/// @brief A class with *private* data that still supports `auto [id, name, crew] = record;` by
///        implementing the tuple-like protocol (member `get<I>()` plus `std::tuple_size`/`tuple_element`).
class ShipRecord {
public:
    /// @brief Creates a record. @param id Id. @param name Name. @param crew Crew size.
    ShipRecord(int id, std::string name, int crew) : id_(id), name_(std::move(name)), crew_(crew) {}

    /// @brief Tuple-like accessor (const lvalue). @return Element I.
    template <std::size_t I>
    [[nodiscard]] const auto& get() const& noexcept {
        static_assert(I < 3, "ShipRecord has three elements");
        if constexpr (I == 0) {
            return id_;
        } else if constexpr (I == 1) {
            return name_;
        } else {
            return crew_;
        }
    }

    /// @brief Tuple-like accessor (mutable lvalue). @return Element I.
    template <std::size_t I>
    [[nodiscard]] auto& get() & noexcept {
        static_assert(I < 3, "ShipRecord has three elements");
        if constexpr (I == 0) {
            return id_;
        } else if constexpr (I == 1) {
            return name_;
        } else {
            return crew_;
        }
    }

    /// @brief Tuple-like accessor (rvalue): moves the element out. @return Element I.
    template <std::size_t I>
    [[nodiscard]] auto&& get() && noexcept {
        return std::move(get<I>());
    }

    /// @brief Crew size. @return Crew.
    [[nodiscard]] int crew() const noexcept { return crew_; }

private:
    int id_;
    std::string name_;
    int crew_;
};

} // namespace CppVerseHub::Modern::StructuredBindings

/// @cond
namespace std {
template <>
struct tuple_size<CppVerseHub::Modern::StructuredBindings::ShipRecord> : integral_constant<size_t, 3> {};
template <>
struct tuple_element<0, CppVerseHub::Modern::StructuredBindings::ShipRecord> {
    using type = int;
};
template <>
struct tuple_element<1, CppVerseHub::Modern::StructuredBindings::ShipRecord> {
    using type = string;
};
template <>
struct tuple_element<2, CppVerseHub::Modern::StructuredBindings::ShipRecord> {
    using type = int;
};
} // namespace std
/// @endcond

namespace CppVerseHub::Modern::StructuredBindings {

// ===== FUNCTIONS WHOSE RESULTS ARE MEANT TO BE DECOMPOSED =====

/// @brief Circular-orbit parameters around a body (G = 1 units).
/// @param mass Central mass (> 0). @param distance Orbit radius (> 0).
/// @return {orbital velocity, period, escape velocity}.
[[nodiscard]] inline std::tuple<double, double, double> orbitParameters(double mass, double distance) {
    const double velocity = std::sqrt(mass / distance);
    const double period = 2.0 * std::numbers::pi * distance / velocity;
    const double escape = std::numbers::sqrt2 * velocity;
    return {velocity, period, escape};
}

/// @brief Distance between two coordinates and a jump classification.
/// @param from Origin. @param to Destination.
/// @return {distance, "short"/"medium"/"long"}.
[[nodiscard]] inline std::pair<double, std::string> jumpDistance(const SpaceCoordinate& from,
                                                                 const SpaceCoordinate& to) {
    const auto& [x1, y1, z1] = from;
    const auto& [x2, y2, z2] = to;
    const double d = std::sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1) + (z2 - z1) * (z2 - z1));
    const char* category = d < 10.0 ? "short" : (d < 100.0 ? "medium" : "long");
    return {d, category};
}

/// @brief Fleet with the best ships x fuel score.
/// @param fleets Candidates. @return {commander, score}, or nullopt if `fleets` is empty.
[[nodiscard]] inline std::optional<std::pair<std::string, double>> findBestFleet(
    const std::vector<FleetStats>& fleets) {
    std::optional<std::pair<std::string, double>> best;
    for (const auto& [commander, ships, fuel, mission] : fleets) {
        const double score = static_cast<double>(ships) * fuel / 100.0;
        if (!best || score > best->second) {
            best = std::pair{commander, score};
        }
    }
    return best;
}

/// @brief Completed-mission count and mean completion.
/// @param missions Reports. @return {missions at 100%, mean completion (0 if empty)}.
[[nodiscard]] inline std::pair<int, double> missionStats(const std::vector<MissionReport>& missions) {
    int completed = 0;
    double total = 0.0;
    for (const auto& [id, type, completion, priority] : missions) {
        completed += completion >= 100.0 ? 1 : 0;
        total += completion;
    }
    return {completed, missions.empty() ? 0.0 : total / static_cast<double>(missions.size())};
}

/// @brief Mass-weighted centre of a set of planets.
/// @param planets Input. @return {x, y, z}; all zero if total mass is zero.
[[nodiscard]] inline std::array<double, 3> centerOfMass(const std::vector<PlanetInfo>& planets) {
    std::array<double, 3> c{0.0, 0.0, 0.0};
    double totalMass = 0.0;
    for (const auto& [name, mass, pos, habitable] : planets) {
        c[0] += pos.x * mass;
        c[1] += pos.y * mass;
        c[2] += pos.z * mass;
        totalMass += mass;
    }
    if (totalMass > 0.0) {
        for (double& v : c) {
            v /= totalMass;
        }
    }
    return c;
}

/// @brief Lowest and highest fuel across fleets.
/// @param fleets Input (non-empty for a meaningful result). @return {min, max}; {0, 0} if empty.
[[nodiscard]] inline FuelRange fuelRange(const std::vector<FleetStats>& fleets) {
    if (fleets.empty()) {
        return {};
    }
    FuelRange r{fleets.front().fuelPercentage, fleets.front().fuelPercentage};
    for (const auto& f : fleets) {
        r.minimum = std::min(r.minimum, f.fuelPercentage);
        r.maximum = std::max(r.maximum, f.fuelPercentage);
    }
    return r;
}

/// @brief Refuels every fleet below `threshold` to 100% by binding *references* to members.
/// @param fleets Fleets (modified). @param threshold Fuel threshold. @return Number refuelled.
inline int refuelBelow(std::vector<FleetStats>& fleets, double threshold) {
    int refuelled = 0;
    for (auto& [commander, ships, fuel, mission] : fleets) {
        if (fuel < threshold) {
            fuel = 100.0;
            ++refuelled;
        }
    }
    return refuelled;
}

/// @brief Total ships per mission type, built by decomposing `try_emplace`'s result.
/// @param fleets Input. @return mission type -> ship count.
[[nodiscard]] inline std::map<std::string, int> shipsByMission(const std::vector<FleetStats>& fleets) {
    std::map<std::string, int> totals;
    for (const auto& [commander, ships, fuel, mission] : fleets) {
        auto [it, inserted] = totals.try_emplace(mission, 0);
        it->second += ships;
    }
    return totals;
}

/// @brief Mission type with the most ships (ties: lexicographically first).
/// @param totals Output of `shipsByMission`. @return {type, ships}, or nullopt if empty.
[[nodiscard]] inline std::optional<std::pair<std::string, int>> busiestMission(
    const std::map<std::string, int>& totals) {
    std::optional<std::pair<std::string, int>> best;
    for (const auto& [type, ships] : totals) {
        if (!best || ships > best->second) {
            best = std::pair{type, ships};
        }
    }
    return best;
}

/// @brief Sum of crews of the records (each decomposed through the tuple-like protocol).
/// @param records Input. @return Total crew.
[[nodiscard]] inline int totalCrew(const std::vector<ShipRecord>& records) {
    int total = 0;
    for (const auto& [id, name, crew] : records) {
        total += crew;
    }
    return total;
}

/// @brief C++20: structured bindings may be captured by lambdas.
/// @param coordinate Point to scale. @param factor Multiplier.
/// @return A callable returning the point scaled by `factor` and offset by its argument.
[[nodiscard]] inline auto makeScaledOffset(const SpaceCoordinate& coordinate, double factor) {
    const auto [x, y, z] = coordinate;
    return [x, y, z, factor](double offset) {
        return SpaceCoordinate{x * factor + offset, y * factor + offset, z * factor + offset};
    };
}

/// @brief Sample fleets. @return Four fleets.
[[nodiscard]] inline std::vector<FleetStats> sampleFleets() {
    return {{"Zhang", 12, 85.0, "Exploration"},
            {"Okafor", 20, 45.0, "Combat"},
            {"Ivanova", 8, 92.5, "Exploration"},
            {"Reyes", 15, 30.0, "Trade"}};
}

/// @brief Showcase of every structured-binding form. @param out Destination stream.
inline void demonstrateStructuredBindings(std::ostream& out = std::cout) {
    out << "\n=== Structured Bindings ===\n";

    const auto [velocity, period, escape] = orbitParameters(1000.0, 10.0);
    out << "tuple:  velocity=" << velocity << " period=" << period << " escape=" << escape << '\n';

    const auto [distance, category] = jumpDistance({0, 0, 0}, {30, 40, 0});
    out << "pair:   distance=" << distance << " (" << category << " jump)\n";

    const std::vector<PlanetInfo> planets{{"Alpha", 2.0, {0, 0, 0}, true}, {"Beta", 1.0, {3, 6, 9}, false}};
    const auto [cx, cy, cz] = centerOfMass(planets);
    out << "array:  center of mass=(" << cx << ", " << cy << ", " << cz << ")\n";

    auto fleets = sampleFleets();
    const auto [lowest, highest] = fuelRange(fleets);
    out << "struct: fuel range " << lowest << "%.." << highest << "%\n";

    const int refuelled = refuelBelow(fleets, 50.0);
    out << "by reference: refuelled " << refuelled << " fleets; now";
    for (const auto& [commander, ships, fuel, mission] : fleets) {
        out << ' ' << commander << '=' << fuel;
    }
    out << '\n';

    const auto totals = shipsByMission(fleets);
    for (const auto& [type, ships] : totals) {
        out << "map:    " << type << " -> " << ships << " ships\n";
    }
    if (const auto busiest = busiestMission(totals)) {
        const auto& [type, ships] = *busiest;
        out << "busiest mission: " << type << " (" << ships << ")\n";
    }
    if (auto best = findBestFleet(fleets)) {
        const auto& [commander, score] = *best;
        out << "best fleet: " << commander << " score " << score << '\n';
    }

    std::vector<ShipRecord> records{{1, "Explorer", 150}, {2, "Guardian", 300}};
    auto& [firstId, firstName, firstCrew] = records.front();
    firstCrew += 10; // binds to the private member through get<2>() &
    out << "tuple-like class: " << firstName << " (#" << firstId << ") crew " << records.front().crew()
        << ", total crew " << totalCrew(records) << '\n';

    const auto scaled = makeScaledOffset({1.0, 2.0, 3.0}, 2.0)(0.5);
    out << "lambda-captured bindings: (" << scaled.x << ", " << scaled.y << ", " << scaled.z << ")\n";

    const auto [stats_completed, stats_mean] = missionStats(
        {{1, "Scan", 100.0, 1}, {2, "Mine", 50.0, 2}, {3, "Escort", 100.0, 3}});
    out << "missions: " << stats_completed << " complete, mean " << stats_mean << "%\n";
}

} // namespace CppVerseHub::Modern::StructuredBindings
