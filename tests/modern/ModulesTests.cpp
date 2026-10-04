#include "modern/ModulesDemo.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <string>

using namespace CppVerseHub::Modern::Modules;
using namespace CppVerseHub::Modern::Modules::SpaceGame;
using Catch::Approx;

TEST_CASE("Core utilities", "[modern][modules]") {
    CHECK(Core::calculateDistance({0, 0, 0}, {2, 3, 6}) == Approx(7.0));
    CHECK(Core::parseCommaSeparatedList(" ore, water ,,gas ,") ==
          std::vector<std::string>{"ore", "water", "gas"});
    CHECK(Core::parseCommaSeparatedList("").empty());
    Core::IdGenerator gen("fleet");
    CHECK(gen.next() == "fleet-1");
    CHECK(gen.next() == "fleet-2");
    CHECK(gen.issued() == 2);
    Core::IdGenerator other;
    CHECK(other.next() == "entity-1"); // independent state, no globals
}

TEST_CASE("Core is versioned through an inline namespace", "[modern][modules]") {
    STATIC_CHECK(std::is_same_v<Core::Vec3, Core::v1::Vec3>);
}

TEST_CASE("Planet behaviour", "[modern][modules]") {
    Entities::Planet p(1, "Terra", {1, 2, 3}, 1000, true);
    CHECK(p.getType() == "Planet");
    p.update(1.0);
    CHECK(p.getPopulation() == 1010);
    p.update(-1.0);
    CHECK(p.getPopulation() == 1010);
    p.addResource("ore");
    p.addResource("ore");
    p.addResource("gas");
    CHECK(p.getResources().size() == 2);
    p.setPopulation(-5);
    CHECK(p.getPopulation() == 0);
    Entities::Planet barren(2, "Rock", {1, 2, 13}, 1000, false);
    barren.update(5.0);
    CHECK(barren.getPopulation() == 1000);
    CHECK(p.distanceTo(barren) == Approx(10.0));
}

TEST_CASE("Starship movement consumes fuel", "[modern][modules]") {
    Entities::Starship s(1, "Arrow", "Scout", {0, 0, 0}, 12);
    CHECK(s.getFuelPercentage() == Approx(100.0));
    s.setVelocity({3, 4, 0});
    s.update(2.0); // travels 10 units
    CHECK(s.getPosition() == Core::Vec3{6, 8, 0});
    CHECK(s.getFuelPercentage() == Approx(99.0));
    CHECK(s.hasEnoughFuelFor(990.0));
    CHECK_FALSE(s.hasEnoughFuelFor(991.0));
    s.setVelocity({1000, 0, 0});
    s.update(1.0); // not enough fuel: stops in place
    CHECK(s.getPosition() == Core::Vec3{6, 8, 0});
    CHECK(s.getVelocity() == Core::Vec3{});
    s.refuel(5000.0);
    CHECK(s.getFuelPercentage() == Approx(100.0));
    CHECK(s.getType() == "Starship");
}

TEST_CASE("Mission state machine", "[modern][modules]") {
    using Missions::Mission;
    using Missions::MissionStatus;
    Mission m(1, "Survey", Missions::MissionType::Exploration, 4.0, 2);
    CHECK(m.getStatus() == MissionStatus::Pending);
    m.update(1.0);
    CHECK(m.getProgress() == 0.0); // pending missions don't progress
    CHECK_FALSE(m.fail());
    CHECK(m.start());
    CHECK_FALSE(m.start());
    m.update(1.0);
    CHECK(m.getProgress() == Approx(25.0));
    CHECK(m.getRemainingTime() == Approx(3.0));
    m.update(10.0);
    CHECK(m.getStatus() == MissionStatus::Completed);
    CHECK(m.getProgress() == 100.0);
    CHECK(m.getRemainingTime() == 0.0);
    CHECK_FALSE(m.cancel());

    Mission n(2, "Strike", Missions::MissionType::Combat, 1.0);
    CHECK(n.cancel());
    CHECK(n.getStatus() == MissionStatus::Cancelled);
    Mission f(3, "Doomed", Missions::MissionType::Combat, 1.0);
    CHECK(f.start());
    CHECK(f.fail());
    CHECK(toString(f.getStatus()) == "Failed");
}

TEST_CASE("Mission ship assignment", "[modern][modules]") {
    Missions::Mission m(1, "Escort", Missions::MissionType::Trade, 1.0);
    CHECK(m.assignShip(5));
    CHECK_FALSE(m.assignShip(5));
    CHECK(m.assignShip(6));
    CHECK(m.unassignShip(5));
    CHECK_FALSE(m.unassignShip(5));
    CHECK(m.getAssignedShips() == std::vector<int>{6});
}

TEST_CASE("MissionFactory applies per-type defaults", "[modern][modules]") {
    auto m = Missions::MissionFactory::create(9, Missions::MissionType::Rescue, "Titan");
    CHECK(m->getId() == 9);
    CHECK(m->getName() == "Rescue: Titan");
    CHECK(m->getPriority() == 4);
    CHECK(m->getRemainingTime() == Approx(5.0));
    CHECK(toString(Missions::MissionType::Colonization) == "Colonization");
}

TEST_CASE("FleetFormation owns ships and retires completed missions", "[modern][modules]") {
    Fleet::FleetFormation f(1, "Alpha", {"Zhang", "Admiral", 5});
    CHECK(f.getAverageFuelLevel() == 0.0);
    f.addShip(std::make_unique<Entities::Starship>(1, "A", "Scout", Core::Vec3{}, 10));
    f.addShip(std::make_unique<Entities::Starship>(2, "B", "Scout", Core::Vec3{}, 20));
    f.addShip(nullptr);
    CHECK(f.getShipCount() == 2);
    CHECK(f.getTotalCrewSize() == 30);
    REQUIRE(f.findShip(2) != nullptr);
    CHECK(f.findShip(3) == nullptr);

    f.assignMission(Missions::MissionFactory::create(1, Missions::MissionType::Rescue, "X")); // 5 time units
    CHECK(f.getActiveMissionCount() == 1);
    f.findShip(1)->setVelocity({100, 0, 0});
    f.update(2.0);
    CHECK(f.getAverageFuelLevel() == Approx(90.0));
    f.update(3.0);
    CHECK(f.getActiveMissionCount() == 0);
    CHECK(f.getCompletedMissionCount() == 1);
    f.refuelAll();
    CHECK(f.getAverageFuelLevel() == Approx(100.0));

    auto removed = f.removeShip(1);
    REQUIRE(removed);
    CHECK(removed->getName() == "A");
    CHECK(f.getShipCount() == 1);
    CHECK(f.removeShip(1) == nullptr);
    CHECK(f.getCommander().rank == "Admiral");
}

TEST_CASE("GameUniverse sample simulation", "[modern][modules]") {
    auto u = System::GameUniverse::createSample();
    CHECK(u.getPlanetCount() == 4);
    CHECK(u.getFleetCount() == 2);
    CHECK(u.getMissionCount() == 2);
    CHECK(u.findHabitablePlanets().size() == 3);
    CHECK(u.findPlanetByName("Nowhere") == nullptr);
    const auto startPop = u.getTotalPopulation();
    u.runSimulation(5, 1.0);
    CHECK(u.getGameTime() == Approx(5.0));
    CHECK(u.getTotalPopulation() > startPop);
    CHECK(u.findMissionsByStatus(Missions::MissionStatus::InProgress).size() == 1);
    CHECK(u.findMissionsByStatus(Missions::MissionStatus::Pending).size() == 1);
    u.update(-3.0);
    CHECK(u.getGameTime() == Approx(5.0));
    const auto report = u.getUniverseReport();
    REQUIRE(report.size() == 1 + 2 + 2);
    CHECK(report[0].rfind("t=5", 0) == 0);
    CHECK(report[1].find("completed=1") != std::string::npos); // Alpha's 5-unit rescue finished
}

TEST_CASE("Planet serialisation round-trips", "[modern][modules]") {
    auto u = System::GameUniverse::createSample();
    const auto text = u.serializePlanets();
    const auto restored = System::GameUniverse::deserializePlanets(text);
    REQUIRE(restored.size() == 4);
    const auto* original = u.findPlanetByName("Proxima-b");
    REQUIRE(original != nullptr);
    CHECK(restored[3]->getName() == "Proxima-b");
    CHECK(restored[3]->getPosition() == original->getPosition());
    CHECK(restored[3]->isHabitable());
    CHECK(restored[0]->getPopulation() == 8'000'000'000LL);
}

TEST_CASE("Planet deserialisation rejects malformed input", "[modern][modules]") {
    using System::GameUniverse;
    CHECK(GameUniverse::deserializePlanets("").empty());
    CHECK_THROWS_AS(GameUniverse::deserializePlanets("Earth;1;2;3;4\n"), std::invalid_argument);
    CHECK_THROWS_AS(GameUniverse::deserializePlanets("Earth;1;x;3;4;1\n"), std::invalid_argument);
    CHECK_THROWS_AS(GameUniverse::deserializePlanets("Earth;1;2;3;4;yes\n"), std::invalid_argument);
    CHECK_THROWS_AS(GameUniverse::deserializePlanets(";1;2;3;4;1\n"), std::invalid_argument);
    GameUniverse bad;
    bad.addPlanet(std::make_unique<Entities::Planet>(1, "a;b", Core::Vec3{}));
    CHECK_THROWS_AS(bad.serializePlanets(), std::invalid_argument);
}

TEST_CASE("Module graph yields a dependency-respecting build order", "[modern][modules]") {
    const auto graph = moduleGraph();
    REQUIRE(graph.size() == 5);
    const auto order = topologicalBuildOrder(graph);
    REQUIRE(order.has_value());
    auto pos = [&](const std::string& n) {
        return std::find(order->begin(), order->end(), n) - order->begin();
    };
    for (const auto& unit : graph) {
        for (const auto& imp : unit.imports) {
            CHECK(pos(imp) < pos(unit.name));
        }
    }
    CHECK(order->front() == "CppVerseHub.SpaceGame.Core");
    CHECK(order->back() == "CppVerseHub.SpaceGame.System");
}

TEST_CASE("topologicalBuildOrder detects cycles and unknown imports", "[modern][modules]") {
    CHECK_FALSE(topologicalBuildOrder({{"A", {"B"}, {}}, {"B", {"A"}, {}}}).has_value());
    CHECK_FALSE(topologicalBuildOrder({{"A", {"Missing"}, {}}}).has_value());
    CHECK(topologicalBuildOrder({}).value().empty());
}

TEST_CASE("moduleInterfaceSketch renders real module syntax", "[modern][modules]") {
    const auto sketch = moduleInterfaceSketch();
    CHECK(sketch.find("export module CppVerseHub.SpaceGame.Core;") != std::string::npos);
    CHECK(sketch.find("import CppVerseHub.SpaceGame.Missions;") != std::string::npos);
    CHECK(sketch.find("export namespace SpaceGame::Fleet {") != std::string::npos);
}

TEST_CASE("modules showcase writes to the stream", "[modern][modules]") {
    std::ostringstream os;
    demonstrateModules(os);
    CHECK(os.str().find("Serialized and restored 4 planets") != std::string::npos);
    CHECK(os.str().find("1. CppVerseHub.SpaceGame.Core") != std::string::npos);
}
