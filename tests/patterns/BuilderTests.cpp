/**
 * @file BuilderTests.cpp
 * @brief Tests for the fluent, director and type-state builders.
 */

#include "patterns/Builder.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>

#include <algorithm>

using namespace CppVerseHub::Patterns;
using Catch::Approx;

TEST_CASE("Fluent builder produces a ship with the requested parts", "[builder]") {
    const Spacecraft ship = SpacecraftBuilder{}
                                .name("Kestrel")
                                .hull(HullClass::Frigate)
                                .crew(25)
                                .engine(50)
                                .reactor(100)
                                .weapon(30)
                                .build();
    CHECK(ship.name() == "Kestrel");
    CHECK(ship.hull() == HullClass::Frigate);
    CHECK(ship.crew() == 25);
    CHECK(ship.components().size() == 3);
    CHECK(ship.count(ComponentType::Engine) == 1);
    CHECK(ship.count(ComponentType::Shield) == 0);
    CHECK(ship.rating(ComponentType::Weapon) == Approx(30.0));
    CHECK(ship.powerBalance() == Approx(100.0 - 10.0 - 15.0));
    CHECK(ship.totalMass() == Approx(hullMass(HullClass::Frigate) + 35.0 + 35.0 + 16.0));
}

TEST_CASE("validate reports every violated invariant at once", "[builder][validation]") {
    SpacecraftBuilder b;
    const auto problems = b.validate();
    CHECK(problems.size() == 4); // name, hull, engine, crew (0 < 1 for the default Scout hull)
    CHECK_THROWS_AS(b.build(), BuildError);
}

TEST_CASE("Mass, power and crew limits are enforced", "[builder][validation]") {
    SpacecraftBuilder b;
    b.name("X").hull(HullClass::Scout).crew(1).engine(10).reactor(10);
    CHECK(b.validate().empty());
    SECTION("power deficit") {
        b.weapon(100);
        try {
            (void)b.build();
            FAIL("expected BuildError");
        } catch (const BuildError& e) {
            REQUIRE(e.problems().size() == 1);
            CHECK_THAT(e.problems()[0], Catch::Matchers::ContainsSubstring("power deficit"));
            CHECK_THAT(std::string(e.what()), Catch::Matchers::ContainsSubstring("invalid design"));
        }
    }
    SECTION("mass limit") {
        b.reactor(500); // heavy reactor
        const auto problems = b.validate();
        REQUIRE(problems.size() == 1);
        CHECK_THAT(problems[0], Catch::Matchers::ContainsSubstring("exceeds Scout limit"));
    }
    SECTION("crew minimum") {
        b.hull(HullClass::Cruiser);
        const auto problems = b.validate();
        CHECK(std::any_of(problems.begin(), problems.end(),
                          [](const std::string& p) { return p.find("crew") != std::string::npos; }));
    }
    SECTION("negative component mass") {
        b.add({ComponentType::Sensor, "Bogus", -5.0, 0.0, 1.0});
        CHECK_THAT(b.validate(),
                   Catch::Matchers::VectorContains(std::string("component 'Bogus' has negative mass")));
    }
}

TEST_CASE("Builder can be reused and reset", "[builder]") {
    SpacecraftBuilder b;
    b.name("One").hull(HullClass::Scout).crew(1).engine(10).reactor(10);
    const Spacecraft first = b.build();
    const Spacecraft second = b.name("Two").build();
    CHECK(first.name() == "One");
    CHECK(second.name() == "Two");
    CHECK(second.components().size() == first.components().size());
    b.reset();
    CHECK_FALSE(b.validate().empty());
}

TEST_CASE("Director recipes produce valid ships of the right class", "[builder][director]") {
    SpacecraftBuilder b;
    const Spacecraft scout = ShipyardDirector::buildScout(b, "S");
    const Spacecraft frigate = ShipyardDirector::buildFrigate(b, "F");
    const Spacecraft carrier = ShipyardDirector::buildCarrier(b, "C");
    CHECK(scout.hull() == HullClass::Scout);
    CHECK(frigate.hull() == HullClass::Frigate);
    CHECK(carrier.hull() == HullClass::Carrier);
    for (const Spacecraft* s : {&scout, &frigate, &carrier}) {
        CHECK(s->powerBalance() >= 0.0);
        CHECK(s->totalMass() <= maxMass(s->hull()));
        CHECK(s->crew() >= minCrew(s->hull()));
    }
    CHECK(carrier.count(ComponentType::Hangar) == 2);
    CHECK(frigate.rating(ComponentType::Weapon) > scout.rating(ComponentType::Weapon));
}

TEST_CASE("Hull tables are consistent and constexpr", "[builder]") {
    STATIC_REQUIRE(maxMass(HullClass::Scout) < maxMass(HullClass::Frigate));
    STATIC_REQUIRE(minCrew(HullClass::Carrier) == 400);
    STATIC_REQUIRE(hullMass(HullClass::Cruiser) == maxMass(HullClass::Cruiser) * 0.4);
    CHECK(toString(HullClass::Scout) == "Scout");
    CHECK(toString(HullClass::Frigate) == "Frigate");
    CHECK(toString(HullClass::Cruiser) == "Cruiser");
    CHECK(toString(HullClass::Carrier) == "Carrier");
}

TEST_CASE("FleetBuilder aggregates ships and validates", "[builder][fleet]") {
    SpacecraftBuilder b;
    const Spacecraft frigate = ShipyardDirector::buildFrigate(b, "F");
    const Spacecraft scout = ShipyardDirector::buildScout(b, "S");
    const Fleet fleet =
        FleetBuilder{}.name("Home").commander("Reyes").addCopies(frigate, 3).add(scout).build();
    CHECK(fleet.ships.size() == 4);
    CHECK(fleet.commander == "Reyes");
    CHECK(fleet.totalCrew() == 3 * frigate.crew() + scout.crew());
    CHECK(fleet.firepower() == Approx(3 * frigate.rating(ComponentType::Weapon)));
    CHECK_THROWS_AS(FleetBuilder{}.name("Empty").build(), BuildError);
    CHECK_THROWS_AS(FleetBuilder{}.add(scout).build(), BuildError);
}

TEST_CASE("Type-state builder only allows build() after mandatory steps", "[builder][typestate]") {
    STATIC_REQUIRE_FALSE(BuildableBlueprint<BlueprintBuilder<false, false>>);
    STATIC_REQUIRE_FALSE(BuildableBlueprint<BlueprintBuilder<true, false>>);
    STATIC_REQUIRE_FALSE(BuildableBlueprint<BlueprintBuilder<false, true>>);
    STATIC_REQUIRE(BuildableBlueprint<BlueprintBuilder<true, true>>);
    const Blueprint bp =
        BlueprintBuilder<>{}.withName("Paladin").withHardpoints(6).withHull(HullClass::Cruiser).build();
    CHECK(bp.name == "Paladin");
    CHECK(bp.hull == HullClass::Cruiser);
    CHECK(bp.hardpoints == 6);
}
