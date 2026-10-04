/**
 * @file Ship.hpp
 * @brief Ship classes and their compile-time specification table.
 *
 * Demonstrates data-driven design with a `constexpr` lookup table: per-class combat and movement
 * statistics live in one place, are usable in constant expressions (see the `static_assert`s), and
 * individual ships are plain value types that a Fleet stores contiguously in a `std::vector`.
 */
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace CppVerseHub::Core {

/// @brief Class of a ship.
enum class ShipType : std::uint8_t { Scout, Fighter, Cruiser, Battleship, Transport, Colonizer };

/// @brief Number of ShipType enumerators.
inline constexpr std::size_t kShipTypeCount = 6;

/// @brief Static characteristics of a ship class.
struct ShipSpec {
    double attack;       ///< Damage per second at full hull.
    double maxHull;      ///< Hit points of a new ship.
    double speed;        ///< Units per second.
    double energyUpkeep; ///< Energy consumed per second.
};

/// @brief Specification table indexed by ShipType.
inline constexpr std::array<ShipSpec, kShipTypeCount> kShipSpecs{{
    {1.0, 15.0, 40.0, 0.2},   // Scout
    {5.0, 20.0, 30.0, 0.5},   // Fighter
    {15.0, 80.0, 20.0, 1.5},  // Cruiser
    {40.0, 250.0, 12.0, 4.0}, // Battleship
    {0.5, 60.0, 15.0, 0.8},   // Transport
    {0.0, 50.0, 10.0, 1.0},   // Colonizer
}};

/// @brief Specification of a ship class. @param type Class. @return Its spec.
[[nodiscard]] constexpr const ShipSpec& specOf(ShipType type) noexcept {
    return kShipSpecs[static_cast<std::size_t>(type)];
}

static_assert(specOf(ShipType::Battleship).attack > specOf(ShipType::Cruiser).attack);
static_assert(specOf(ShipType::Scout).speed > specOf(ShipType::Colonizer).speed);

/// @brief Lower-case class name. @param type Class. @return Name such as "cruiser".
[[nodiscard]] std::string_view toString(ShipType type) noexcept;
/// @brief Parse a class name. @param name Name. @return Class or std::nullopt.
[[nodiscard]] std::optional<ShipType> parseShipType(std::string_view name) noexcept;

/// @brief One ship: its class and remaining hull.
struct Ship {
    ShipType type{ShipType::Fighter};               ///< Class.
    double hull{specOf(ShipType::Fighter).maxHull}; ///< Remaining hit points.

    /// @brief A brand-new ship of a class. @param type Class. @return Ship at full hull.
    [[nodiscard]] static constexpr Ship make(ShipType type) noexcept {
        return Ship{type, specOf(type).maxHull};
    }

    /// @brief Hull fraction in [0,1]. @return hull / maxHull.
    [[nodiscard]] constexpr double integrity() const noexcept { return hull / specOf(type).maxHull; }

    /// @brief Effective damage per second (attack scaled by integrity). @return Attack.
    [[nodiscard]] constexpr double effectiveAttack() const noexcept {
        return specOf(type).attack * integrity();
    }

    /// @brief Equality. @return true if same class and hull.
    friend constexpr bool operator==(const Ship&, const Ship&) noexcept = default;
};

static_assert(Ship::make(ShipType::Cruiser).integrity() == 1.0);

} // namespace CppVerseHub::Core
