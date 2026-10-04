#include "core/Exceptions.hpp"
#include "core/Fleet.hpp"
#include "core/Planet.hpp"
#include "core/Ship.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <nlohmann/json.hpp>

#include <cmath>
#include <limits>
#include <memory>

using namespace CppVerseHub::Core;
using Catch::Approx;

TEST_CASE("Entity construction validates its invariants", "[core][entity]") {
    REQUIRE_THROWS_AS(Planet(EntityId{1}, "", {}), InvalidArgumentException);
    REQUIRE_THROWS_AS(Planet(EntityId{}, "Nameless", {}), InvalidArgumentException);
    REQUIRE_THROWS_AS(Fleet(EntityId{1}, "F", {std::numeric_limits<double>::quiet_NaN(), 0, 0}),
                      InvalidArgumentException);
    const Planet p(EntityId{7}, "Mars", {1.0, 2.0, 3.0});
    REQUIRE(p.id() == EntityId{7});
    REQUIRE(p.name() == "Mars");
    REQUIRE(p.position() == Vector3D{1.0, 2.0, 3.0});
    REQUIRE(p.health() == Entity::kMaxHealth);
    REQUIRE(p.status() == EntityStatus::Active);
    REQUIRE(p.kind() == EntityKind::Planet);
}

TEST_CASE("Entity health, damage, healing and status transitions", "[core][entity]") {
    Planet p(EntityId{1}, "Target", {});
    p.takeDamage(25.0);
    REQUIRE(p.health() == Approx(75.0));
    p.heal(10.0);
    REQUIRE(p.health() == Approx(85.0));
    p.heal(1000.0);
    REQUIRE(p.health() == Entity::kMaxHealth);
    REQUIRE_THROWS_AS(p.takeDamage(-1.0), InvalidArgumentException);
    REQUIRE_THROWS_AS(p.heal(-1.0), InvalidArgumentException);
    p.setStatus(EntityStatus::Inactive);
    REQUIRE(p.isAlive());
    p.takeDamage(500.0);
    REQUIRE(p.health() == 0.0);
    REQUIRE(p.status() == EntityStatus::Destroyed);
    p.heal(50.0);
    REQUIRE(p.health() == 0.0);
    REQUIRE_THROWS_AS(p.setStatus(EntityStatus::Active), InvalidStateException);
}

TEST_CASE("Entity naming, position and distance", "[core][entity]") {
    Fleet f(EntityId{1}, "A", {0, 0, 0});
    const Planet p(EntityId{2}, "B", {3, 4, 0});
    REQUIRE(f.distanceTo(p) == 5.0);
    f.setPosition({3, 4, 12});
    REQUIRE(f.distanceTo(p) == 12.0);
    f.setName("Renamed");
    REQUIRE(f.name() == "Renamed");
    REQUIRE_THROWS_AS(f.setName(""), InvalidArgumentException);
    REQUIRE_THAT(p.describe(), Catch::Matchers::ContainsSubstring("planet #2 'B'"));
}

TEST_CASE("Enum string conversions round-trip", "[core][entity]") {
    for (ResourceType t : kAllResourceTypes) {
        REQUIRE(parseResourceType(toString(t)) == t);
    }
    for (std::size_t i = 0; i < kShipTypeCount; ++i) {
        const auto t = static_cast<ShipType>(i);
        REQUIRE(parseShipType(toString(t)) == t);
    }
    REQUIRE(parsePlanetType(toString(PlanetType::GasGiant)) == PlanetType::GasGiant);
    REQUIRE(parseEntityKind("fleet") == EntityKind::Fleet);
    REQUIRE(parseEntityStatus("destroyed") == EntityStatus::Destroyed);
    REQUIRE_FALSE(parseResourceType("unobtainium").has_value());
    REQUIRE_FALSE(parseShipType("dreadnought").has_value());
}

TEST_CASE("Ship specification table is consistent", "[core][ship]") {
    for (const ShipSpec& spec : kShipSpecs) {
        REQUIRE(spec.maxHull > 0.0);
        REQUIRE(spec.speed > 0.0);
        REQUIRE(spec.attack >= 0.0);
    }
    Ship s = Ship::make(ShipType::Cruiser);
    REQUIRE(s.integrity() == 1.0);
    s.hull /= 2.0;
    REQUIRE(s.effectiveAttack() == Approx(specOf(ShipType::Cruiser).attack / 2.0));
}

TEST_CASE("Planet attributes are validated", "[core][planet]") {
    Planet p(EntityId{1}, "Kepler", {}, PlanetType::Ocean, 0.8);
    REQUIRE(p.planetType() == PlanetType::Ocean);
    REQUIRE(p.populationCapacity() == Approx(0.8 * Planet::kCapacityPerHabitability));
    REQUIRE_THROWS_AS(p.setHabitability(1.5), InvalidArgumentException);
    REQUIRE_THROWS_AS(p.setHabitability(-0.1), InvalidArgumentException);
    REQUIRE_THROWS_AS(p.setPopulation(-1.0), InvalidArgumentException);
    REQUIRE_THROWS_AS(p.setDefense(std::numeric_limits<double>::infinity()), InvalidArgumentException);
    REQUIRE_THROWS_AS(p.setProductionRate(ResourceType::Energy, -2.0), InvalidArgumentException);
    REQUIRE_FALSE(p.isHostile());
    p.setDefense(10.0);
    REQUIRE(p.isHostile());
}

TEST_CASE("Planet population follows logistic growth towards capacity", "[core][planet]") {
    Planet p(EntityId{1}, "Growth", {}, PlanetType::Terrestrial, 0.001); // capacity 10'000
    p.setPopulation(1000.0);
    double previous = p.population();
    for (int i = 0; i < 20000; ++i) {
        p.update(0.1);
        REQUIRE(p.population() >= previous); // monotone below capacity
        previous = p.population();
    }
    REQUIRE(p.population() == Approx(p.populationCapacity()).epsilon(1e-3));
    REQUIRE(p.population() <= p.populationCapacity() + 1e-6);
    Planet empty(EntityId{2}, "Empty", {});
    empty.update(1.0);
    REQUIRE(empty.population() == 0.0);
}

TEST_CASE("Planet declares production and food demand, and starves on shortage", "[core][planet]") {
    Planet p(EntityId{1}, "Farm", {});
    p.setProductionRate(ResourceType::Minerals, 4.0);
    p.setPopulation(20000.0);
    const ResourceFlows flows = p.resourceFlows();
    REQUIRE(flows.production[ResourceType::Minerals] == 4.0);
    REQUIRE(flows.consumption[ResourceType::Food] == Approx(20000.0 * Planet::kFoodPerCapita));
    ResourceAmounts shortfall;
    shortfall[ResourceType::Food] = 100;
    p.onResourceTick(shortfall);
    REQUIRE(p.population() == Approx(20000.0 - 100.0 * Planet::kStarvationPerUnit));
    shortfall[ResourceType::Food] = 1'000'000;
    p.onResourceTick(shortfall);
    REQUIRE(p.population() == 0.0);
}

TEST_CASE("Fleet composition drives speed, firepower, health and upkeep", "[core][fleet]") {
    Fleet f(EntityId{1}, "Armada", {});
    REQUIRE(f.speed() == 0.0);
    f.addShips(ShipType::Fighter, 4);
    f.addShips(ShipType::Battleship, 1);
    REQUIRE(f.shipCount() == 5);
    REQUIRE(f.shipCount(ShipType::Fighter) == 4);
    REQUIRE(f.speed() == specOf(ShipType::Battleship).speed);
    REQUIRE(f.attackPower() == Approx(4 * 5.0 + 40.0));
    REQUIRE(f.maxHull() == Approx(4 * 20.0 + 250.0));
    REQUIRE(f.resourceFlows().consumption[ResourceType::Energy] == Approx(4 * 0.5 + 4.0));
    REQUIRE(f.removeShips(ShipType::Battleship, 3) == 1);
    REQUIRE(f.speed() == specOf(ShipType::Fighter).speed);
    REQUIRE_THROWS_AS(f.addShip(Ship{ShipType::Scout, 0.0}), InvalidArgumentException);
}

TEST_CASE("Fleet movement never overshoots and halves speed in low power", "[core][fleet]") {
    Fleet f(EntityId{1}, "Mover", {0, 0, 0});
    f.addShips(ShipType::Fighter, 1); // speed 30
    f.setDestination({100, 0, 0});
    REQUIRE(f.etaTo({100, 0, 0}) == Approx(100.0 / 30.0));
    f.update(1.0);
    REQUIRE(f.position().approxEquals({30, 0, 0}));
    REQUIRE_FALSE(f.hasArrived());
    ResourceAmounts shortfall;
    shortfall[ResourceType::Energy] = 1;
    f.onResourceTick(shortfall);
    REQUIRE(f.isLowPower());
    REQUIRE(f.speed() == 15.0);
    f.onResourceTick(ResourceAmounts{});
    REQUIRE_FALSE(f.isLowPower());
    for (int i = 0; i < 10; ++i) {
        f.update(1.0);
    }
    REQUIRE(f.position() == Vector3D{100, 0, 0});
    REQUIRE(f.hasArrived());
    Fleet empty(EntityId{2}, "Empty", {});
    REQUIRE(std::isinf(empty.etaTo({1, 0, 0})));
}

TEST_CASE("Fleet damage is applied front-to-back and destroys empty fleets", "[core][fleet]") {
    Fleet f(EntityId{1}, "Victim", {});
    f.addShips(ShipType::Fighter, 3); // 3 x 20 hull
    f.takeDamage(30.0);               // kills one, wounds the next
    REQUIRE(f.shipCount() == 2);
    REQUIRE(f.ships().front().hull == Approx(10.0));
    REQUIRE(f.totalHull() == Approx(30.0));
    REQUIRE(f.health() == Approx(100.0 * 30.0 / 40.0));
    f.takeDamage(1000.0);
    REQUIRE(f.shipCount() == 0);
    REQUIRE(f.status() == EntityStatus::Destroyed);
    REQUIRE(f.speed() == 0.0);
    REQUIRE_THROWS_AS(f.addShips(ShipType::Scout), InvalidStateException);
}

TEST_CASE("Planet and Fleet JSON round-trip losslessly", "[core][entity][json]") {
    Planet p(EntityId{3}, "Vega", {1.25, -2.5, 1e-7}, PlanetType::Volcanic, 0.33);
    p.setPopulation(1234.5678);
    p.setDefense(42.0);
    p.setExplored(true);
    p.setProductionRate(ResourceType::Technology, 0.125);
    p.takeDamage(12.5);
    nlohmann::json pj;
    p.toJson(pj);
    const Planet p2(EntityId{3}, pj);
    nlohmann::json pj2;
    p2.toJson(pj2);
    REQUIRE(pj == pj2);
    REQUIRE(p2.health() == Approx(87.5));

    Fleet f(EntityId{4}, "Hammer", {5, 5, 5});
    f.addShips(ShipType::Cruiser, 2);
    f.addShips(ShipType::Scout, 1);
    f.takeDamage(7.0);
    f.setDestination({9, 9, 9});
    nlohmann::json fj;
    f.toJson(fj);
    const Fleet f2(EntityId{4}, fj);
    nlohmann::json fj2;
    f2.toJson(fj2);
    REQUIRE(fj == fj2);
    REQUIRE(f2.ships() == f.ships());
    REQUIRE(f2.destination() == f.destination());
}
