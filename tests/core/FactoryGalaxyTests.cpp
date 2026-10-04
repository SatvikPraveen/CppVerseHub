#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <memory>
#include <string>

#include <nlohmann/json.hpp>

#include "core/ExplorationMission.hpp"
#include "core/Factory.hpp"
#include "core/Galaxy.hpp"

using namespace CppVerseHub::Core;

namespace {

struct Shape {
    virtual ~Shape() = default;
    [[nodiscard]] virtual int sides() const = 0;
};
struct Triangle final : Shape {
    explicit Triangle(int scale) : scale_(scale) {}
    [[nodiscard]] int sides() const override { return 3 * scale_; }
    int scale_;
};
struct Square final : Shape {
    explicit Square(int) {}
    [[nodiscard]] int sides() const override { return 4; }
};

/// A custom entity kind registered at run time.
class Starbase final : public Entity {
public:
    Starbase(EntityId id, const nlohmann::json& params) : Entity(id, params) {}
    [[nodiscard]] EntityKind kind() const noexcept override { return EntityKind::Planet; }
    void update(double dt) override { uptime += dt; }
    double uptime{0.0};
};

} // namespace

TEST_CASE("Generic Factory registers, creates and lists products", "[core][factory]") {
    Factory<Shape, std::string, int> f;
    f.registerType<Triangle>("triangle");
    f.registerCreator("square", [](int s) { return std::make_unique<Square>(s); });
    REQUIRE(f.size() == 2);
    REQUIRE(f.contains("triangle"));
    REQUIRE(f.keys() == std::vector<std::string>{"square", "triangle"});
    REQUIRE(f.create("triangle", 2)->sides() == 6);
    REQUIRE(f.create("square", 1)->sides() == 4);
    REQUIRE(f.unregister("square"));
    REQUIRE_FALSE(f.unregister("square"));
    REQUIRE_FALSE(f.contains("square"));
}

TEST_CASE("Generic Factory reports misuse with FactoryException", "[core][factory]") {
    Factory<Shape, std::string, int> f;
    f.registerType<Triangle>("triangle");
    REQUIRE_THROWS_AS(f.registerType<Triangle>("triangle"), FactoryException);
    REQUIRE_THROWS_AS(f.registerCreator("empty", nullptr), FactoryException);
    REQUIRE_THROWS_AS(f.create("hexagon", 1), FactoryException);
    f.registerCreator("null", [](int) { return std::unique_ptr<Shape>{}; });
    REQUIRE_THROWS_AS(f.create("null", 1), FactoryException);
    try {
        static_cast<void>(f.create("hexagon", 1));
    } catch (const CoreException& e) { // catchable by the hierarchy root
        REQUIRE_THAT(e.what(), Catch::Matchers::ContainsSubstring("hexagon"));
    }
}

TEST_CASE("Default factories know the built-in kinds", "[core][factory]") {
    const auto entities = makeDefaultEntityFactory();
    REQUIRE(entities.keys() == std::vector<std::string>{"fleet", "planet"});
    const auto missions = makeDefaultMissionFactory();
    REQUIRE(missions.keys() == std::vector<std::string>{"colonization", "combat", "exploration"});
    auto planet = entities.create("planet", EntityId{3}, {{"name", "P"}, {"habitability", 0.9}});
    REQUIRE(planet->kind() == EntityKind::Planet);
    REQUIRE(dynamic_cast<Planet&>(*planet).habitability() == 0.9);
    auto mission = missions.create("combat", MissionId{1}, {{"fleet", 1}, {"target", 2}, {"duration", 5.0}});
    REQUIRE(mission->type() == MissionType::Combat);
}

TEST_CASE("Galaxy allocates unique ids and opens resource accounts", "[core][galaxy]") {
    Galaxy g("Milky Way");
    Planet& a = g.createPlanet("A", {0, 0, 0});
    Fleet& f = g.createFleet("F", {1, 1, 1});
    REQUIRE(a.id() != f.id());
    REQUIRE(g.entityCount() == 2);
    REQUIRE(g.resources().hasAccount(a.id()));
    REQUIRE(g.resources().hasAccount(f.id()));
    REQUIRE(g.find<Planet>(a.id()) == &a);
    REQUIRE(g.find<Fleet>(a.id()) == nullptr);
    REQUIRE_THROWS_AS(g.get<Fleet>(a.id()), EntityNotFoundException);
    REQUIRE_THROWS_AS(g.get<Planet>(EntityId{999}), EntityNotFoundException);
    REQUIRE(g.all<Planet>().size() == 1);
    REQUIRE(g.all<Entity>().size() == 2);
    REQUIRE(g.removeEntity(f.id()));
    REQUIRE_FALSE(g.removeEntity(EntityId{999}));
    REQUIRE(g.resources().accountCount() == 1);
}

TEST_CASE("Galaxy enforces bounds and id uniqueness", "[core][galaxy]") {
    Galaxy g("Small", {0, 0, 0}, {10, 10, 10});
    REQUIRE(g.contains({10, 0, 5}));
    REQUIRE_FALSE(g.contains({10.5, 0, 5}));
    REQUIRE_THROWS_AS(g.createPlanet("Far", {11, 0, 0}), InvalidArgumentException);
    REQUIRE_THROWS_AS(Galaxy("Bad", {1, 0, 0}, {0, 0, 0}), InvalidArgumentException);
    REQUIRE_THROWS_AS(Galaxy(""), InvalidArgumentException);
    g.addEntity(std::make_unique<Planet>(EntityId{50}, "Fifty", Vector3D{1, 1, 1}));
    REQUIRE_THROWS_AS(g.addEntity(std::make_unique<Planet>(EntityId{50}, "Dup", Vector3D{1, 1, 1})),
                      InvalidArgumentException);
    REQUIRE_THROWS_AS(g.addEntity(nullptr), InvalidArgumentException);
    REQUIRE(g.createFleet("Next", {2, 2, 2}).id() == EntityId{51}); // allocation skips adopted ids
}

TEST_CASE("Galaxy spawns entities and missions through its factories", "[core][galaxy][factory]") {
    Galaxy g;
    g.entityFactory().registerType<Starbase>("starbase");
    Entity& base = g.spawn("starbase", {{"name", "Deep Space 9"}, {"position", {5, 5, 5}}});
    base.update(2.0);
    REQUIRE(dynamic_cast<Starbase&>(base).uptime == 2.0);
    Entity& fleet = g.spawn("fleet", {{"name", "F"}, {"ships", {{{"type", "scout"}, {"count", 2}}}}});
    REQUIRE(dynamic_cast<Fleet&>(fleet).shipCount() == 2);
    Entity& planet = g.spawn("planet", {{"name", "P"}, {"position", {1, 2, 3}}});
    Mission& m = g.spawnMission("exploration",
                                {{"fleet", fleet.id().value()}, {"target", planet.id().value()}, {"duration", 3.0}});
    REQUIRE(m.type() == MissionType::Exploration);
    REQUIRE(g.findMission(m.id()) == &m);
    REQUIRE_THROWS_AS(g.spawn("planet", {{"position", {0, 0, 0}}}), InvalidArgumentException); // no name
    REQUIRE_THROWS_AS(g.spawn("fleet", {{"name", "X"}, {"ships", {{{"type", 5}}}}}), SerializationException);
    REQUIRE_THROWS_AS(g.spawnMission("exploration", {{"fleet", 1}}), SerializationException);
}

TEST_CASE("Galaxy validates mission references", "[core][galaxy]") {
    Galaxy g;
    Planet& p = g.createPlanet("P", {});
    Fleet& f = g.createFleet("F", {});
    REQUIRE_THROWS_AS(g.addMission<ExplorationMission>(p.id(), f.id(), 1.0), EntityNotFoundException);
    REQUIRE_THROWS_AS(g.addMission(std::unique_ptr<Mission>{}), InvalidArgumentException);
    auto& m = g.addMission<ExplorationMission>(f.id(), p.id(), 1.0);
    REQUIRE_THROWS_AS(g.addMission(std::make_unique<ExplorationMission>(m.id(), f.id(), p.id(), 1.0)),
                      InvalidArgumentException);
    REQUIRE(g.unfinishedMissionCount() == 1);
    REQUIRE_FALSE(g.isFleetAssigned(f.id())); // pending, not active
}

TEST_CASE("Galaxy nearestPlanet breaks ties by id", "[core][galaxy]") {
    Galaxy g;
    REQUIRE(g.nearestPlanet({}) == nullptr);
    Planet& a = g.createPlanet("A", {10, 0, 0});
    g.createPlanet("B", {-10, 0, 0});
    Planet& c = g.createPlanet("C", {0, 3, 0});
    REQUIRE(g.nearestPlanet({0, 0, 0}) == &c);
    REQUIRE(g.nearestPlanet({0, -100, 0}) == &a); // A and B are equidistant: lower id wins
}
