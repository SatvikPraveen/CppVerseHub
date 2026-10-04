/**
 * @file Planet.hpp
 * @brief A planet: a stationary Entity with population, defences and resource production.
 *
 * Shows a concrete Entity overriding the polymorphic hooks: `update` integrates logistic population
 * growth with a fixed step, `resourceFlows` declares production and the population's food demand to
 * the economy, and `onResourceTick` reacts to famine. Resource *stock* deliberately does not live in
 * the planet: it is held in the Galaxy's ResourceManager under the planet's id, so a single ledger
 * enforces conservation.
 */
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "core/Entity.hpp"

namespace CppVerseHub::Core {

/// @brief Planet classification.
enum class PlanetType : std::uint8_t { Terrestrial, Desert, Ocean, GasGiant, Ice, Volcanic };

/// @brief Lower-case name. @param type Planet type. @return Name such as "ocean".
[[nodiscard]] std::string_view toString(PlanetType type) noexcept;
/// @brief Parse a planet type name. @param name Name. @return Type or std::nullopt.
[[nodiscard]] std::optional<PlanetType> parsePlanetType(std::string_view name) noexcept;

/// @brief A planet entity.
class Planet final : public Entity {
public:
    /// @brief Population capacity per unit of habitability.
    static constexpr double kCapacityPerHabitability = 1.0e7;
    /// @brief Logistic growth rate per second.
    static constexpr double kGrowthRate = 0.01;
    /// @brief Food units consumed per inhabitant per second.
    static constexpr double kFoodPerCapita = 1.0e-4;
    /// @brief Inhabitants lost per unit of missing food.
    static constexpr double kStarvationPerUnit = 10.0;

    /**
     * @brief Construct a planet.
     * @param id Valid id.
     * @param name Non-empty name.
     * @param position Position.
     * @param type Planet classification.
     * @param habitability Habitability in [0, 1].
     */
    Planet(EntityId id, std::string name, const Vector3D& position, PlanetType type = PlanetType::Terrestrial,
           double habitability = 0.5);

    /**
     * @brief Construct from JSON parameters (the format produced by toJson()).
     *
     * Recognised keys: name, position, health, status, planetType, habitability, population, defense,
     * explored, production {resource: rate}.
     * @param id Valid id.
     * @param params Parameters.
     */
    Planet(EntityId id, const nlohmann::json& params);

    /// @brief Kind tag. @return EntityKind::Planet.
    [[nodiscard]] EntityKind kind() const noexcept override { return EntityKind::Planet; }

    /// @brief Integrate population growth over dt. @param dt Step length in seconds.
    void update(double dt) override;

    /// @brief Production rates plus the population's food demand. @return Flows.
    [[nodiscard]] ResourceFlows resourceFlows() const override;

    /// @brief Apply famine losses for unmet food demand. @param shortfall Units lacking per type.
    void onResourceTick(const ResourceAmounts& shortfall) override;

    /// @brief Summary line. @return Description.
    [[nodiscard]] std::string describe() const override;

    /// @brief Serialise. @param out Destination object.
    void toJson(nlohmann::json& out) const override;

    /// @brief Classification. @return Type.
    [[nodiscard]] PlanetType planetType() const noexcept { return type_; }

    /// @brief Habitability in [0,1]. @return Habitability.
    [[nodiscard]] double habitability() const noexcept { return habitability_; }
    /// @brief Set habitability. @param value Value in [0, 1].
    void setHabitability(double value);

    /// @brief Carrying capacity (habitability * kCapacityPerHabitability). @return Capacity.
    [[nodiscard]] double populationCapacity() const noexcept { return habitability_ * kCapacityPerHabitability; }

    /// @brief Population. @return Inhabitants.
    [[nodiscard]] double population() const noexcept { return population_; }
    /// @brief Set population. @param value Non-negative, finite value.
    void setPopulation(double value);

    /// @brief Defensive strength; a planet with defense > 0 is hostile to colonisation. @return Defense.
    [[nodiscard]] double defense() const noexcept { return defense_; }
    /// @brief Set defense. @param value Non-negative, finite value.
    void setDefense(double value);
    /// @brief Whether the planet resists colonisation. @return true if defense > 0.
    [[nodiscard]] bool isHostile() const noexcept { return defense_ > 0.0; }

    /// @brief Whether an exploration mission has surveyed the planet. @return true if explored.
    [[nodiscard]] bool isExplored() const noexcept { return explored_; }
    /// @brief Mark as explored or not. @param value New flag.
    void setExplored(bool value) noexcept { explored_ = value; }

    /// @brief Base production rate. @param type Resource. @return Units per second.
    [[nodiscard]] double productionRate(ResourceType type) const noexcept { return production_[type]; }
    /// @brief Set a base production rate. @param type Resource. @param unitsPerSecond Non-negative, finite rate.
    void setProductionRate(ResourceType type, double unitsPerSecond);

private:
    PlanetType type_;
    double habitability_{0.5};
    double population_{0.0};
    double defense_{0.0};
    bool explored_{false};
    ResourceRates production_;
};

} // namespace CppVerseHub::Core
