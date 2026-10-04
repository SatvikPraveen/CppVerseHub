/**
 * @file Fleet.hpp
 * @brief A fleet: a mobile Entity composed of individual ships.
 *
 * Demonstrates composition inside a polymorphic hierarchy: the Fleet's health, speed, firepower and
 * energy upkeep are all *derived* from its `std::vector<Ship>`, so they can never fall out of sync.
 * Movement is integrated with the fixed simulation step and never overshoots its destination;
 * damage is distributed deterministically (front ship first) and destroyed ships are removed.
 */
#pragma once

#include "core/Entity.hpp"
#include "core/Ship.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace CppVerseHub::Core {

/// @brief A group of ships moving and fighting together.
class Fleet final : public Entity {
public:
    /**
     * @brief Construct an empty fleet.
     * @param id Valid id.
     * @param name Non-empty name.
     * @param position Starting position.
     */
    Fleet(EntityId id, std::string name, const Vector3D& position);

    /**
     * @brief Construct from JSON parameters (the format produced by toJson()).
     *
     * Recognised keys: name, position, health, status, destination [x,y,z], lowPower, and
     * ships: array of {"type": "...", "hull": h} or {"type": "...", "count": n} entries.
     * @param id Valid id.
     * @param params Parameters.
     */
    Fleet(EntityId id, const nlohmann::json& params);

    /// @brief Kind tag. @return EntityKind::Fleet.
    [[nodiscard]] EntityKind kind() const noexcept override { return EntityKind::Fleet; }

    /// @brief Move toward the destination, if any. @param dt Step length in seconds.
    void update(double dt) override;

    /// @brief Distribute damage over ships front to back. @param amount Non-negative damage.
    void takeDamage(double amount) override;

    /// @brief Energy upkeep of all ships. @return Flows.
    [[nodiscard]] ResourceFlows resourceFlows() const override;

    /// @brief Enter low-power mode (half speed) while energy upkeep is unmet. @param shortfall Units lacking.
    void onResourceTick(const ResourceAmounts& shortfall) override;

    /// @brief Summary line. @return Description.
    [[nodiscard]] std::string describe() const override;

    /// @brief Serialise. @param out Destination object.
    void toJson(nlohmann::json& out) const override;

    /**
     * @brief Add new ships at full hull.
     * @param type Ship class.
     * @param count Number of ships.
     */
    void addShips(ShipType type, std::size_t count = 1);

    /// @brief Add one ship as-is. @param ship Ship with hull in (0, maxHull].
    void addShip(const Ship& ship);

    /**
     * @brief Remove up to `count` ships of a class (rearmost first).
     * @param type Ship class.
     * @param count Maximum number to remove.
     * @return Number actually removed.
     */
    std::size_t removeShips(ShipType type, std::size_t count = 1);

    /// @brief All ships in order. @return Ships.
    [[nodiscard]] const std::vector<Ship>& ships() const noexcept { return ships_; }
    /// @brief Number of ships. @return Count.
    [[nodiscard]] std::size_t shipCount() const noexcept { return ships_.size(); }
    /// @brief Number of ships of a class. @param type Class. @return Count.
    [[nodiscard]] std::size_t shipCount(ShipType type) const noexcept;

    /// @brief Current speed: slowest ship, halved in low-power mode, 0 when empty. @return Units per second.
    [[nodiscard]] double speed() const noexcept;
    /// @brief Combined effective attack per second. @return Attack.
    [[nodiscard]] double attackPower() const noexcept;
    /// @brief Sum of remaining hull. @return Hull.
    [[nodiscard]] double totalHull() const noexcept;
    /// @brief Sum of maximum hull. @return Hull.
    [[nodiscard]] double maxHull() const noexcept;
    /// @brief Whether the fleet is in low-power mode. @return true if energy upkeep was last unmet.
    [[nodiscard]] bool isLowPower() const noexcept { return lowPower_; }

    /// @brief Order the fleet to move. @param destination Finite target position.
    void setDestination(const Vector3D& destination);
    /// @brief Stop moving.
    void clearDestination() noexcept { destination_.reset(); }
    /// @brief Current destination. @return Destination or std::nullopt.
    [[nodiscard]] const std::optional<Vector3D>& destination() const noexcept { return destination_; }
    /// @brief Whether the fleet has no pending destination. @return true if idle.
    [[nodiscard]] bool hasArrived() const noexcept { return !destination_.has_value(); }

    /**
     * @brief Time needed to reach a point at the current speed.
     * @param target Target position.
     * @return Seconds, or +infinity when the fleet cannot move.
     */
    [[nodiscard]] double etaTo(const Vector3D& target) const noexcept;

private:
    void refreshHealth() noexcept;

    std::vector<Ship> ships_;
    std::optional<Vector3D> destination_;
    bool lowPower_{false};
};

} // namespace CppVerseHub::Core
