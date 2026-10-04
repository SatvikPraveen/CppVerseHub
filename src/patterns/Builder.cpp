/**
 * @file Builder.cpp
 * @brief Spacecraft/fleet builders, director and the builder showcase.
 */

#include "patterns/Builder.hpp"

#include <algorithm>
#include <numeric>
#include <sstream>

namespace CppVerseHub::Patterns {

std::string_view toString(HullClass hull) noexcept {
    switch (hull) {
        case HullClass::Scout: return "Scout";
        case HullClass::Frigate: return "Frigate";
        case HullClass::Cruiser: return "Cruiser";
        case HullClass::Carrier: return "Carrier";
    }
    return "Unknown";
}

// ---------------------------------------------------------------------------
// Spacecraft
// ---------------------------------------------------------------------------

double Spacecraft::totalMass() const noexcept {
    return std::accumulate(components_.begin(), components_.end(), hullMass(hull_),
                           [](double acc, const Component& c) { return acc + c.mass; });
}

double Spacecraft::powerBalance() const noexcept {
    return std::accumulate(components_.begin(), components_.end(), 0.0,
                           [](double acc, const Component& c) { return acc + c.power; });
}

double Spacecraft::rating(ComponentType type) const noexcept {
    double total = 0.0;
    for (const auto& c : components_) {
        if (c.type == type) {
            total += c.rating;
        }
    }
    return total;
}

std::size_t Spacecraft::count(ComponentType type) const noexcept {
    return static_cast<std::size_t>(
        std::count_if(components_.begin(), components_.end(), [type](const Component& c) { return c.type == type; }));
}

// ---------------------------------------------------------------------------
// BuildError
// ---------------------------------------------------------------------------

namespace {
std::string joinProblems(const std::vector<std::string>& problems) {
    std::string msg = "invalid design:";
    for (const auto& p : problems) {
        msg += " " + p + ";";
    }
    return msg;
}
}  // namespace

BuildError::BuildError(std::vector<std::string> problems)
    : std::invalid_argument(joinProblems(problems)), problems_(std::move(problems)) {}

// ---------------------------------------------------------------------------
// SpacecraftBuilder
// ---------------------------------------------------------------------------

SpacecraftBuilder& SpacecraftBuilder::name(std::string name) {
    name_ = std::move(name);
    return *this;
}

SpacecraftBuilder& SpacecraftBuilder::hull(HullClass hull) noexcept {
    hull_ = hull;
    hullSet_ = true;
    return *this;
}

SpacecraftBuilder& SpacecraftBuilder::crew(int crew) noexcept {
    crew_ = crew;
    return *this;
}

SpacecraftBuilder& SpacecraftBuilder::add(Component component) {
    components_.push_back(std::move(component));
    return *this;
}

SpacecraftBuilder& SpacecraftBuilder::engine(double thrust) {
    return add({ComponentType::Engine, "Ion drive", 10.0 + thrust * 0.5, -thrust * 0.2, thrust});
}

SpacecraftBuilder& SpacecraftBuilder::reactor(double output) {
    return add({ComponentType::Reactor, "Fusion core", 5.0 + output * 0.3, output, output});
}

SpacecraftBuilder& SpacecraftBuilder::weapon(double damage) {
    return add({ComponentType::Weapon, "Railgun", 4.0 + damage * 0.4, -damage * 0.5, damage});
}

SpacecraftBuilder& SpacecraftBuilder::shield(double strength) {
    return add({ComponentType::Shield, "Deflector", 3.0 + strength * 0.2, -strength * 0.3, strength});
}

SpacecraftBuilder& SpacecraftBuilder::sensor(double range) {
    return add({ComponentType::Sensor, "Sensor array", 1.0 + range * 0.05, -range * 0.05, range});
}

SpacecraftBuilder& SpacecraftBuilder::hangar(double craft) {
    return add({ComponentType::Hangar, "Hangar bay", 50.0 + craft * 15.0, -craft * 0.5, craft});
}

SpacecraftBuilder& SpacecraftBuilder::reset() {
    *this = SpacecraftBuilder{};
    return *this;
}

std::vector<std::string> SpacecraftBuilder::validate() const {
    std::vector<std::string> problems;
    if (name_.empty()) {
        problems.emplace_back("name is required");
    }
    if (!hullSet_) {
        problems.emplace_back("hull class is required");
    }
    const bool hasEngine = std::any_of(components_.begin(), components_.end(),
                                       [](const Component& c) { return c.type == ComponentType::Engine; });
    if (!hasEngine) {
        problems.emplace_back("at least one engine is required");
    }
    if (crew_ < minCrew(hull_)) {
        problems.push_back(std::string(toString(hull_)) + " needs at least " + std::to_string(minCrew(hull_)) +
                           " crew");
    }
    double mass = hullMass(hull_);
    double power = 0.0;
    for (const auto& c : components_) {
        mass += c.mass;
        power += c.power;
        if (c.mass < 0.0) {
            problems.push_back("component '" + c.name + "' has negative mass");
        }
    }
    if (mass > maxMass(hull_)) {
        std::ostringstream os;
        os << "mass " << mass << "t exceeds " << toString(hull_) << " limit " << maxMass(hull_) << "t";
        problems.push_back(os.str());
    }
    if (power < 0.0) {
        std::ostringstream os;
        os << "power deficit of " << -power;
        problems.push_back(os.str());
    }
    return problems;
}

Spacecraft SpacecraftBuilder::build() const {
    if (auto problems = validate(); !problems.empty()) {
        throw BuildError(std::move(problems));
    }
    Spacecraft ship;
    ship.name_ = name_;
    ship.hull_ = hull_;
    ship.crew_ = crew_;
    ship.components_ = components_;
    return ship;
}

// ---------------------------------------------------------------------------
// Director
// ---------------------------------------------------------------------------

Spacecraft ShipyardDirector::buildScout(SpacecraftBuilder& builder, std::string name) {
    return builder.reset().name(std::move(name)).hull(HullClass::Scout).crew(2).engine(60.0).reactor(20.0).sensor(
        80.0).build();
}

Spacecraft ShipyardDirector::buildFrigate(SpacecraftBuilder& builder, std::string name) {
    return builder.reset()
        .name(std::move(name))
        .hull(HullClass::Frigate)
        .crew(45)
        .engine(100.0)
        .reactor(150.0)
        .weapon(60.0)
        .weapon(60.0)
        .shield(80.0)
        .sensor(40.0)
        .build();
}

Spacecraft ShipyardDirector::buildCarrier(SpacecraftBuilder& builder, std::string name) {
    return builder.reset()
        .name(std::move(name))
        .hull(HullClass::Carrier)
        .crew(900)
        .engine(300.0)
        .engine(300.0)
        .reactor(600.0)
        .hangar(24.0)
        .hangar(24.0)
        .shield(300.0)
        .weapon(80.0)
        .sensor(200.0)
        .build();
}

// ---------------------------------------------------------------------------
// Fleet
// ---------------------------------------------------------------------------

int Fleet::totalCrew() const noexcept {
    int crew = 0;
    for (const auto& s : ships) {
        crew += s.crew();
    }
    return crew;
}

double Fleet::firepower() const noexcept {
    double total = 0.0;
    for (const auto& s : ships) {
        total += s.rating(ComponentType::Weapon);
    }
    return total;
}

FleetBuilder& FleetBuilder::name(std::string name) {
    fleet_.name = std::move(name);
    return *this;
}

FleetBuilder& FleetBuilder::commander(std::string commander) {
    fleet_.commander = std::move(commander);
    return *this;
}

FleetBuilder& FleetBuilder::add(Spacecraft ship) {
    fleet_.ships.push_back(std::move(ship));
    return *this;
}

FleetBuilder& FleetBuilder::addCopies(const Spacecraft& ship, std::size_t n) {
    fleet_.ships.insert(fleet_.ships.end(), n, ship);
    return *this;
}

Fleet FleetBuilder::build() const {
    std::vector<std::string> problems;
    if (fleet_.name.empty()) {
        problems.emplace_back("fleet name is required");
    }
    if (fleet_.ships.empty()) {
        problems.emplace_back("a fleet needs at least one ship");
    }
    if (!problems.empty()) {
        throw BuildError(std::move(problems));
    }
    return fleet_;
}

// ---------------------------------------------------------------------------
// Showcase
// ---------------------------------------------------------------------------

void demonstrateBuilder(std::ostream& out) {
    out << "=== Builder pattern ===\n";
    SpacecraftBuilder builder;
    const Spacecraft scout = ShipyardDirector::buildScout(builder, "Pathfinder");
    const Spacecraft frigate = ShipyardDirector::buildFrigate(builder, "Resolute");
    const Spacecraft carrier = ShipyardDirector::buildCarrier(builder, "Leviathan");
    for (const Spacecraft* s : {&scout, &frigate, &carrier}) {
        out << "  " << s->name() << " [" << toString(s->hull()) << "] mass " << s->totalMass() << "t, power +"
            << s->powerBalance() << ", crew " << s->crew() << ", " << s->components().size() << " components\n";
    }

    try {
        (void)builder.reset().name("Overloaded").hull(HullClass::Scout).crew(1).engine(10.0).weapon(400.0).build();
    } catch (const BuildError& e) {
        out << "  rejected design with " << e.problems().size() << " problem(s):\n";
        for (const auto& p : e.problems()) {
            out << "    - " << p << '\n';
        }
    }

    const Fleet fleet =
        FleetBuilder{}.name("Home Guard").commander("Adm. Reyes").add(carrier).addCopies(frigate, 3).add(scout).build();
    out << "  fleet " << fleet.name << ": " << fleet.ships.size() << " ships, crew " << fleet.totalCrew()
        << ", firepower " << fleet.firepower() << '\n';

    const Blueprint bp = BlueprintBuilder<>{}.withHull(HullClass::Cruiser).withHardpoints(8).withName("Paladin").build();
    out << "  type-state blueprint: " << bp.name << " (" << toString(bp.hull) << ", " << bp.hardpoints
        << " hardpoints)\n";
}

}  // namespace CppVerseHub::Patterns
