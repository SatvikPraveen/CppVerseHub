/**
 * @file Builder.hpp
 * @brief Builder pattern: assembling validated spacecraft and fleets step by step.
 *
 * A spacecraft has many optional parts and cross-field invariants (power budget, mass limit,
 * minimum crew per hull class) that a constructor cannot express readably. This file shows:
 *  - a **fluent builder** (`SpacecraftBuilder`) that accumulates parts and validates every
 *    invariant in `build()`, producing an immutable `Spacecraft` or throwing `BuildError` with
 *    the full list of problems (`validate()` lets callers check first);
 *  - a **director** (`ShipyardDirector`) that encodes standard recipes (scout, frigate, carrier)
 *    so clients do not need to know the construction steps;
 *  - a **compile-time "type-state" builder** (`BlueprintBuilder<HasName, HasHull>`) where calling
 *    `build()` before the mandatory steps is a *compile* error rather than a run-time one;
 *  - a simple aggregate builder (`FleetBuilder`) composing products of the first builder.
 */

#pragma once

#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace CppVerseHub::Patterns {

/// @brief Hull classes, which bound mass and crew.
enum class HullClass { Scout, Frigate, Cruiser, Carrier };

/// @brief Name of a hull class. @param hull Hull. @return Name.
[[nodiscard]] std::string_view toString(HullClass hull) noexcept;

/// @brief Maximum total mass a hull can carry (tonnes). @param hull Hull. @return Mass limit.
[[nodiscard]] constexpr double maxMass(HullClass hull) noexcept {
    switch (hull) {
        case HullClass::Scout:
            return 200.0;
        case HullClass::Frigate:
            return 800.0;
        case HullClass::Cruiser:
            return 2500.0;
        case HullClass::Carrier:
            return 6000.0;
    }
    return 0.0;
}

/// @brief Minimum crew for a hull. @param hull Hull. @return Crew count.
[[nodiscard]] constexpr int minCrew(HullClass hull) noexcept {
    switch (hull) {
        case HullClass::Scout:
            return 1;
        case HullClass::Frigate:
            return 20;
        case HullClass::Cruiser:
            return 120;
        case HullClass::Carrier:
            return 400;
    }
    return 0;
}

/// @brief Base mass of a hull (tonnes). @param hull Hull. @return Mass.
[[nodiscard]] constexpr double hullMass(HullClass hull) noexcept {
    return maxMass(hull) * 0.4;
}

/// @brief Kinds of installable components.
enum class ComponentType { Engine, Reactor, Weapon, Shield, Sensor, Hangar };

/**
 * @brief An installed component.
 */
struct Component {
    ComponentType type{}; ///< Kind.
    std::string name;     ///< Model name.
    double mass = 0.0;    ///< Tonnes.
    double power = 0.0;   ///< Power produced (> 0, reactors) or consumed (< 0).
    double rating = 0.0;  ///< Thrust / damage / shield strength / range, depending on type.
};

/**
 * @brief Immutable product: a fully validated spacecraft.
 */
class Spacecraft {
public:
    /// @brief Ship name. @return Name.
    [[nodiscard]] const std::string& name() const noexcept { return name_; }
    /// @brief Hull class. @return Hull.
    [[nodiscard]] HullClass hull() const noexcept { return hull_; }
    /// @brief Crew. @return Crew count.
    [[nodiscard]] int crew() const noexcept { return crew_; }
    /// @brief Installed components. @return Components.
    [[nodiscard]] const std::vector<Component>& components() const noexcept { return components_; }
    /// @brief Total mass including hull. @return Tonnes.
    [[nodiscard]] double totalMass() const noexcept;
    /// @brief Net power (production - consumption). @return Power units.
    [[nodiscard]] double powerBalance() const noexcept;
    /// @brief Sum of ratings of components of @p type. @param type Component type. @return Rating.
    [[nodiscard]] double rating(ComponentType type) const noexcept;
    /// @brief Number of components of @p type. @param type Component type. @return Count.
    [[nodiscard]] std::size_t count(ComponentType type) const noexcept;

private:
    friend class SpacecraftBuilder;
    Spacecraft() = default;

    std::string name_;
    HullClass hull_ = HullClass::Scout;
    int crew_ = 0;
    std::vector<Component> components_;
};

/**
 * @brief Thrown by build() when invariants are violated; carries every problem found.
 */
class BuildError : public std::invalid_argument {
public:
    /// @brief Construct. @param problems Validation problems (non-empty).
    explicit BuildError(std::vector<std::string> problems);
    /// @brief Problems found. @return Messages.
    [[nodiscard]] const std::vector<std::string>& problems() const noexcept { return problems_; }

private:
    std::vector<std::string> problems_;
};

/**
 * @brief Fluent builder for Spacecraft.
 *
 * Methods return `*this` so calls chain. `build()` does not consume the builder, so one partially
 * configured builder can stamp out several ships (e.g. after `name()` is changed).
 */
class SpacecraftBuilder {
public:
    SpacecraftBuilder() = default;

    /// @brief Set the name. @param name Name. @return *this.
    SpacecraftBuilder& name(std::string name);
    /// @brief Set the hull. @param hull Hull class. @return *this.
    SpacecraftBuilder& hull(HullClass hull) noexcept;
    /// @brief Set the crew. @param crew Crew count. @return *this.
    SpacecraftBuilder& crew(int crew) noexcept;
    /// @brief Add an arbitrary component. @param component Component. @return *this.
    SpacecraftBuilder& add(Component component);
    /// @brief Add an engine. @param thrust Thrust rating. @return *this.
    SpacecraftBuilder& engine(double thrust);
    /// @brief Add a reactor. @param output Power output. @return *this.
    SpacecraftBuilder& reactor(double output);
    /// @brief Add a weapon. @param damage Damage rating. @return *this.
    SpacecraftBuilder& weapon(double damage);
    /// @brief Add a shield. @param strength Shield strength. @return *this.
    SpacecraftBuilder& shield(double strength);
    /// @brief Add a sensor suite. @param range Sensor range. @return *this.
    SpacecraftBuilder& sensor(double range);
    /// @brief Add a hangar bay. @param craft Small-craft capacity. @return *this.
    SpacecraftBuilder& hangar(double craft);
    /// @brief Remove all components and reset fields. @return *this.
    SpacecraftBuilder& reset();

    /// @brief Check invariants without building. @return Problems (empty if valid).
    [[nodiscard]] std::vector<std::string> validate() const;
    /// @brief Build the product. @return Validated spacecraft. @throws BuildError if invalid.
    [[nodiscard]] Spacecraft build() const;

private:
    std::string name_;
    HullClass hull_ = HullClass::Scout;
    bool hullSet_ = false;
    int crew_ = 0;
    std::vector<Component> components_;
};

/**
 * @brief Director: standard construction recipes.
 */
class ShipyardDirector {
public:
    /// @brief Fast, lightly armed scout. @param builder Builder (reset first). @param name Ship name. @return
    /// Ship.
    [[nodiscard]] static Spacecraft buildScout(SpacecraftBuilder& builder, std::string name);
    /// @brief Balanced frigate. @param builder Builder (reset first). @param name Ship name. @return Ship.
    [[nodiscard]] static Spacecraft buildFrigate(SpacecraftBuilder& builder, std::string name);
    /// @brief Carrier with hangars. @param builder Builder (reset first). @param name Ship name. @return
    /// Ship.
    [[nodiscard]] static Spacecraft buildCarrier(SpacecraftBuilder& builder, std::string name);
};

/**
 * @brief Product of FleetBuilder.
 */
struct Fleet {
    std::string name;              ///< Fleet name.
    std::string commander;         ///< Commanding officer.
    std::vector<Spacecraft> ships; ///< Ships.

    /// @brief Total crew. @return Crew.
    [[nodiscard]] int totalCrew() const noexcept;
    /// @brief Total firepower. @return Sum of weapon ratings.
    [[nodiscard]] double firepower() const noexcept;
};

/**
 * @brief Fluent builder for fleets.
 */
class FleetBuilder {
public:
    /// @brief Set the fleet name. @param name Name. @return *this.
    FleetBuilder& name(std::string name);
    /// @brief Set the commander. @param commander Commander. @return *this.
    FleetBuilder& commander(std::string commander);
    /// @brief Add a ship. @param ship Ship. @return *this.
    FleetBuilder& add(Spacecraft ship);
    /// @brief Add @p n copies of a ship. @param ship Prototype. @param n Copies. @return *this.
    FleetBuilder& addCopies(const Spacecraft& ship, std::size_t n);
    /// @brief Build the fleet. @return Fleet. @throws BuildError if unnamed or empty.
    [[nodiscard]] Fleet build() const;

private:
    Fleet fleet_;
};

// ----------------------------------------------------------------------------
// Compile-time type-state builder
// ----------------------------------------------------------------------------

/**
 * @brief Minimal blueprint produced by BlueprintBuilder.
 */
struct Blueprint {
    std::string name;           ///< Design name.
    HullClass hull{};           ///< Hull.
    std::size_t hardpoints = 0; ///< Weapon mounts.
};

/**
 * @brief Builder whose template parameters record which mandatory steps were performed.
 *
 * `withName()` and `withHull()` return a builder of a *different type*; `build()` only exists
 * (via a `requires` clause) once both flags are true, so forgetting a step fails to compile.
 *
 * @tparam HasName Whether the name has been set.
 * @tparam HasHull Whether the hull has been set.
 */
template <bool HasName = false, bool HasHull = false>
class BlueprintBuilder {
public:
    BlueprintBuilder() = default;

    /// @brief Set the name. @param name Design name. @return Builder with HasName = true.
    [[nodiscard]] BlueprintBuilder<true, HasHull> withName(std::string name) && {
        bp_.name = std::move(name);
        return BlueprintBuilder<true, HasHull>(std::move(bp_));
    }
    /// @brief Set the hull. @param hull Hull. @return Builder with HasHull = true.
    [[nodiscard]] BlueprintBuilder<HasName, true> withHull(HullClass hull) && {
        bp_.hull = hull;
        return BlueprintBuilder<HasName, true>(std::move(bp_));
    }
    /// @brief Optional step: weapon mounts. @param n Mount count. @return Same builder type.
    [[nodiscard]] BlueprintBuilder withHardpoints(std::size_t n) && {
        bp_.hardpoints = n;
        return std::move(*this);
    }
    /// @brief Finish; only available once name and hull are set. @return Blueprint.
    [[nodiscard]] Blueprint build() && requires(HasName&& HasHull) { return std::move(bp_); }

        private
        : template <bool, bool>
          friend class BlueprintBuilder;
    explicit BlueprintBuilder(Blueprint bp) : bp_(std::move(bp)) {}

    Blueprint bp_;
};

/// @brief Whether `B` can be built (used to demonstrate the compile-time guarantee in tests).
template <typename B>
concept BuildableBlueprint = requires(B b) { std::move(b).build(); };

/**
 * @brief Showcase the builders and director.
 * @param out Stream receiving the narration.
 */
void demonstrateBuilder(std::ostream& out = std::cout);

} // namespace CppVerseHub::Patterns
