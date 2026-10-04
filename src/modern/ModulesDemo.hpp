/**
 * @file ModulesDemo.hpp
 * @brief C++20 modules — a *documented emulation* using headers and namespaces.
 *
 * Real C++20 named modules (`export module X;` / `import X;`) still cannot be used portably in this
 * project: they need CMake's `FILE_SET CXX_MODULES` support, a scanning-capable generator, and
 * compiler-specific BMI formats (Clang `.pcm`, GCC `.gcm`, MSVC `.ifc`) that differ in maturity across
 * AppleClang, GCC 13 and MSVC. This file therefore **emulates** a module hierarchy with ordinary headers:
 *
 * | Module concept                      | Emulation used here                                          |
 * |-------------------------------------|--------------------------------------------------------------|
 * | `export module CppVerseHub.SpaceGame.Core;` | namespace `SpaceGame::Core` declared in this header      |
 * | `export` declarations               | declarations in this header (the "module interface unit")    |
 * | non-exported / module-linkage names | anonymous namespace inside ModulesDemo.cpp (internal linkage)|
 * | module implementation unit          | ModulesDemo.cpp                                              |
 * | `import A;` dependency edges        | the order of the namespaces + `moduleGraph()` metadata       |
 * | versioned interface                 | `inline namespace v1` inside `Core`                          |
 *
 * `moduleGraph()` records the import graph that real module units would declare,
 * `topologicalBuildOrder()` derives a valid build order from it (modules must be compiled in
 * dependency order, unlike headers), and `moduleInterfaceSketch()` returns the source text the real
 * interface units would contain. The simulated game itself (entities, missions, fleets, universe) is
 * real, tested code organised along those "module" boundaries.
 */
#pragma once

#include <cstddef>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace CppVerseHub::Modern::Modules {

// ======================================================================================
// "module CppVerseHub.SpaceGame.Core" — no imports
// ======================================================================================

namespace SpaceGame::Core::inline v1 {

/// @brief A 3D vector.
struct Vec3 {
    double x = 0.0; ///< X.
    double y = 0.0; ///< Y.
    double z = 0.0; ///< Z.

    /// @brief Memberwise equality.
    friend bool operator==(const Vec3&, const Vec3&) = default;
};

/// @brief Euclidean distance. @param a First point. @param b Second point. @return |a - b|.
[[nodiscard]] double calculateDistance(const Vec3& a, const Vec3& b) noexcept;

/// @brief Splits a comma-separated list, trimming whitespace and dropping empty items.
/// @param input Text such as "ore, water ,,gas". @return Items.
[[nodiscard]] std::vector<std::string> parseCommaSeparatedList(std::string_view input);

/// @brief Produces sequential ids "prefix-1", "prefix-2", ... (per-generator state, no globals).
class IdGenerator {
public:
    /// @brief Creates a generator. @param prefix Id prefix.
    explicit IdGenerator(std::string prefix = "entity");
    /// @brief Next id. @return A new unique id.
    [[nodiscard]] std::string next();
    /// @brief Ids issued so far. @return Count.
    [[nodiscard]] std::size_t issued() const noexcept { return counter_; }

private:
    std::string prefix_;
    std::size_t counter_ = 0;
};

/// @brief Polymorphic base of every simulated object.
class IEntity {
public:
    virtual ~IEntity() = default;
    /// @brief Identifier. @return Id.
    [[nodiscard]] virtual int getId() const noexcept = 0;
    /// @brief Name. @return Name.
    [[nodiscard]] virtual const std::string& getName() const noexcept = 0;
    /// @brief Advances the simulation. @param deltaTime Time step (>= 0).
    virtual void update(double deltaTime) = 0;
    /// @brief Dynamic type name. @return Type name.
    [[nodiscard]] virtual std::string_view getType() const noexcept = 0;

protected:
    IEntity() = default;
    IEntity(const IEntity&) = default;
    IEntity(IEntity&&) = default;
    IEntity& operator=(const IEntity&) = default;
    IEntity& operator=(IEntity&&) = default;
};

} // namespace SpaceGame::Core::inline v1

// ======================================================================================
// "module CppVerseHub.SpaceGame.Entities" — import Core;
// ======================================================================================
namespace SpaceGame::Entities {

/// @brief A planet; habitable planets grow their population by 1% per unit time.
class Planet final : public Core::IEntity {
public:
    /// @brief Creates a planet. @param id Id. @param name Name. @param position Location.
    /// @param population Inhabitants. @param habitable Supports life.
    Planet(int id, std::string name, Core::Vec3 position, long long population = 0, bool habitable = false);

    [[nodiscard]] int getId() const noexcept override { return id_; }
    [[nodiscard]] const std::string& getName() const noexcept override { return name_; }
    void update(double deltaTime) override;
    [[nodiscard]] std::string_view getType() const noexcept override { return "Planet"; }

    /// @brief Location. @return Position.
    [[nodiscard]] const Core::Vec3& getPosition() const noexcept { return position_; }
    /// @brief Inhabitants. @return Population.
    [[nodiscard]] long long getPopulation() const noexcept { return population_; }
    /// @brief Sets inhabitants. @param population New population (negative values clamp to 0).
    void setPopulation(long long population) noexcept { population_ = population < 0 ? 0 : population; }
    /// @brief Habitability. @return True if habitable.
    [[nodiscard]] bool isHabitable() const noexcept { return habitable_; }
    /// @brief Adds a resource name (duplicates ignored). @param resource Resource.
    void addResource(const std::string& resource);
    /// @brief Resources. @return Resource names.
    [[nodiscard]] const std::vector<std::string>& getResources() const noexcept { return resources_; }
    /// @brief Distance to another planet. @param other Planet. @return Distance.
    [[nodiscard]] double distanceTo(const Planet& other) const noexcept;

private:
    int id_;
    std::string name_;
    Core::Vec3 position_;
    long long population_;
    bool habitable_;
    std::vector<std::string> resources_;
};

/// @brief A starship that moves with constant velocity and burns fuel proportional to distance.
class Starship final : public Core::IEntity {
public:
    static constexpr double kMaxFuel = 1000.0;  ///< Tank size.
    static constexpr double kFuelPerUnit = 1.0; ///< Fuel burned per distance unit.

    /// @brief Creates a ship with a full tank. @param id Id. @param name Name. @param classType Class.
    /// @param position Start position. @param crewSize Crew.
    Starship(int id, std::string name, std::string classType, Core::Vec3 position, int crewSize = 100);

    [[nodiscard]] int getId() const noexcept override { return id_; }
    [[nodiscard]] const std::string& getName() const noexcept override { return name_; }
    /// @brief Moves by velocity * dt if there is enough fuel, otherwise stops (velocity zeroed).
    /// @param deltaTime Time step.
    void update(double deltaTime) override;
    [[nodiscard]] std::string_view getType() const noexcept override { return "Starship"; }

    /// @brief Position. @return Position.
    [[nodiscard]] const Core::Vec3& getPosition() const noexcept { return position_; }
    /// @brief Sets the velocity. @param velocity New velocity.
    void setVelocity(const Core::Vec3& velocity) noexcept { velocity_ = velocity; }
    /// @brief Velocity. @return Velocity.
    [[nodiscard]] const Core::Vec3& getVelocity() const noexcept { return velocity_; }
    /// @brief Adds fuel (clamped to the tank). @param amount Fuel to add (negative ignored).
    void refuel(double amount) noexcept;
    /// @brief Fuel level. @return Percentage 0..100.
    [[nodiscard]] double getFuelPercentage() const noexcept { return fuel_ / kMaxFuel * 100.0; }
    /// @brief Whether a trip is affordable. @param distance Trip length. @return True if enough fuel.
    [[nodiscard]] bool hasEnoughFuelFor(double distance) const noexcept {
        return fuel_ >= distance * kFuelPerUnit;
    }
    /// @brief Crew. @return Crew size.
    [[nodiscard]] int getCrewSize() const noexcept { return crewSize_; }
    /// @brief Ship class. @return Class name.
    [[nodiscard]] const std::string& getClassType() const noexcept { return classType_; }

private:
    int id_;
    std::string name_;
    std::string classType_;
    Core::Vec3 position_;
    Core::Vec3 velocity_{};
    double fuel_ = kMaxFuel;
    int crewSize_;
};

} // namespace SpaceGame::Entities

// ======================================================================================
// "module CppVerseHub.SpaceGame.Missions" — import Core;
// ======================================================================================
namespace SpaceGame::Missions {

/// @brief Lifecycle states. Pending -> InProgress -> {Completed, Failed}; Pending/InProgress -> Cancelled.
enum class MissionStatus { Pending, InProgress, Completed, Failed, Cancelled };

/// @brief Mission kinds.
enum class MissionType { Exploration, Combat, Colonization, Trade, Rescue };

/// @brief Name of a status. @param s Status. @return Name.
[[nodiscard]] std::string_view toString(MissionStatus s) noexcept;
/// @brief Name of a type. @param t Type. @return Name.
[[nodiscard]] std::string_view toString(MissionType t) noexcept;

/// @brief A mission with a small state machine; progress advances with simulated time.
class Mission {
public:
    /// @brief Creates a pending mission. @param id Id. @param name Name. @param type Kind.
    /// @param estimatedDuration Time to complete (> 0). @param priority Priority.
    Mission(int id, std::string name, MissionType type, double estimatedDuration, int priority = 1);

    /// @brief Pending -> InProgress. @return True if the transition happened.
    bool start() noexcept;
    /// @brief InProgress -> Failed. @return True if the transition happened.
    bool fail() noexcept;
    /// @brief Pending/InProgress -> Cancelled. @return True if the transition happened.
    bool cancel() noexcept;
    /// @brief Advances an in-progress mission; completes it when elapsed >= duration.
    /// @param deltaTime Time step.
    void update(double deltaTime) noexcept;

    /// @brief Assigns a ship (duplicates ignored). @param shipId Ship id. @return True if added.
    bool assignShip(int shipId);
    /// @brief Unassigns a ship. @param shipId Ship id. @return True if removed.
    bool unassignShip(int shipId);

    /// @brief Id. @return Id.
    [[nodiscard]] int getId() const noexcept { return id_; }
    /// @brief Name. @return Name.
    [[nodiscard]] const std::string& getName() const noexcept { return name_; }
    /// @brief Kind. @return Type.
    [[nodiscard]] MissionType getType() const noexcept { return type_; }
    /// @brief State. @return Status.
    [[nodiscard]] MissionStatus getStatus() const noexcept { return status_; }
    /// @brief Completion. @return Percentage 0..100.
    [[nodiscard]] double getProgress() const noexcept { return progress_; }
    /// @brief Priority. @return Priority.
    [[nodiscard]] int getPriority() const noexcept { return priority_; }
    /// @brief Assigned ship ids. @return Ids.
    [[nodiscard]] const std::vector<int>& getAssignedShips() const noexcept { return assignedShips_; }
    /// @brief Remaining time. @return max(0, duration - elapsed).
    [[nodiscard]] double getRemainingTime() const noexcept;

private:
    int id_;
    std::string name_;
    MissionType type_;
    MissionStatus status_ = MissionStatus::Pending;
    double progress_ = 0.0;
    int priority_;
    double estimatedDuration_;
    double elapsed_ = 0.0;
    std::vector<int> assignedShips_;
};

/// @brief Factory with sensible defaults per mission type.
class MissionFactory {
public:
    /// @brief Creates a mission of the given type with its default duration and priority.
    /// @param id Id. @param type Kind. @param target Target description (used in the name).
    /// @return Owning pointer to the mission.
    [[nodiscard]] static std::unique_ptr<Mission> create(int id, MissionType type, const std::string& target);
};

} // namespace SpaceGame::Missions

// ======================================================================================
// "module CppVerseHub.SpaceGame.Fleet" — import Entities; import Missions;
// ======================================================================================
namespace SpaceGame::Fleet {

/// @brief Commanding officer of a formation.
struct FleetCommander {
    std::string name;   ///< Name.
    std::string rank;   ///< Rank.
    int experience = 1; ///< Experience level.
};

/// @brief A formation that owns its ships and missions.
class FleetFormation {
public:
    /// @brief Creates an empty formation. @param id Id. @param name Name. @param commander Commander.
    FleetFormation(int id, std::string name, FleetCommander commander);

    /// @brief Adds a ship (null ignored). @param ship Ship to own.
    void addShip(std::unique_ptr<Entities::Starship> ship);
    /// @brief Removes a ship and hands ownership back. @param shipId Id. @return The ship, or null.
    [[nodiscard]] std::unique_ptr<Entities::Starship> removeShip(int shipId);
    /// @brief Non-owning lookup. @param shipId Id. @return Pointer or null.
    [[nodiscard]] Entities::Starship* findShip(int shipId) const noexcept;

    /// @brief Takes a mission, starts it and assigns all current ships. @param mission Mission (null
    /// ignored).
    void assignMission(std::unique_ptr<Missions::Mission> mission);
    /// @brief Advances ships and missions; completed missions are retired.
    /// @param deltaTime Time step.
    void update(double deltaTime);
    /// @brief Refuels every ship to full.
    void refuelAll() noexcept;

    /// @brief Id. @return Id.
    [[nodiscard]] int getId() const noexcept { return id_; }
    /// @brief Name. @return Name.
    [[nodiscard]] const std::string& getName() const noexcept { return name_; }
    /// @brief Commander. @return Commander.
    [[nodiscard]] const FleetCommander& getCommander() const noexcept { return commander_; }
    /// @brief Ships. @return Count.
    [[nodiscard]] std::size_t getShipCount() const noexcept { return ships_.size(); }
    /// @brief Active missions. @return Count.
    [[nodiscard]] std::size_t getActiveMissionCount() const noexcept { return missions_.size(); }
    /// @brief Missions retired as completed. @return Count.
    [[nodiscard]] std::size_t getCompletedMissionCount() const noexcept { return completedMissions_; }
    /// @brief Mean fuel. @return Percentage (0 if no ships).
    [[nodiscard]] double getAverageFuelLevel() const noexcept;
    /// @brief Total crew. @return Crew.
    [[nodiscard]] int getTotalCrewSize() const noexcept;

private:
    int id_;
    std::string name_;
    FleetCommander commander_;
    std::vector<std::unique_ptr<Entities::Starship>> ships_;
    std::vector<std::unique_ptr<Missions::Mission>> missions_;
    std::size_t completedMissions_ = 0;
};

} // namespace SpaceGame::Fleet

// ======================================================================================
// "module CppVerseHub.SpaceGame.System" — import Core; import Entities; import Missions; import Fleet;
// ======================================================================================
namespace SpaceGame::System {

/// @brief The whole simulation: owns planets, fleets and global missions.
class GameUniverse {
public:
    GameUniverse() = default;

    /// @brief Adds a planet (null ignored). @param planet Planet.
    void addPlanet(std::unique_ptr<Entities::Planet> planet);
    /// @brief Adds a fleet (null ignored). @param fleet Fleet.
    void addFleet(std::unique_ptr<Fleet::FleetFormation> fleet);
    /// @brief Adds a global mission (null ignored). @param mission Mission.
    void addMission(std::unique_ptr<Missions::Mission> mission);

    /// @brief Advances everything. @param deltaTime Time step (negative values are ignored).
    void update(double deltaTime);
    /// @brief Runs `steps` updates of `timeStep`. @param steps Number of steps. @param timeStep Step size.
    void runSimulation(int steps, double timeStep);

    /// @brief Lookup by name. @param name Name. @return Pointer or null.
    [[nodiscard]] const Entities::Planet* findPlanetByName(std::string_view name) const noexcept;
    /// @brief Habitable planets. @return Non-owning pointers.
    [[nodiscard]] std::vector<const Entities::Planet*> findHabitablePlanets() const;
    /// @brief Missions with a status. @param status Status. @return Non-owning pointers.
    [[nodiscard]] std::vector<const Missions::Mission*> findMissionsByStatus(
        Missions::MissionStatus status) const;

    /// @brief Planets. @return Count.
    [[nodiscard]] std::size_t getPlanetCount() const noexcept { return planets_.size(); }
    /// @brief Fleets. @return Count.
    [[nodiscard]] std::size_t getFleetCount() const noexcept { return fleets_.size(); }
    /// @brief Global missions. @return Count.
    [[nodiscard]] std::size_t getMissionCount() const noexcept { return missions_.size(); }
    /// @brief Total population. @return Sum over planets.
    [[nodiscard]] long long getTotalPopulation() const noexcept;
    /// @brief Simulated time. @return Time.
    [[nodiscard]] double getGameTime() const noexcept { return gameTime_; }

    /// @brief Human-readable status lines. @return Lines.
    [[nodiscard]] std::vector<std::string> getUniverseReport() const;
    /// @brief Serialises planets, one per line: `name;x;y;z;population;habitable`.
    /// @return Text. @throws std::invalid_argument if a name contains ';' or a newline.
    [[nodiscard]] std::string serializePlanets() const;
    /// @brief Parses the output of `serializePlanets` (ids are assigned 1..n).
    /// @param data Text. @return Planets. @throws std::invalid_argument on malformed input.
    [[nodiscard]] static std::vector<std::unique_ptr<Entities::Planet>> deserializePlanets(
        std::string_view data);

    /// @brief Deterministic sample universe: 4 planets, 2 fleets, 2 global missions.
    /// @return The universe.
    [[nodiscard]] static GameUniverse createSample();

private:
    std::vector<std::unique_ptr<Entities::Planet>> planets_;
    std::vector<std::unique_ptr<Fleet::FleetFormation>> fleets_;
    std::vector<std::unique_ptr<Missions::Mission>> missions_;
    double gameTime_ = 0.0;
};

} // namespace SpaceGame::System

// ======================================================================================
// Module metadata (what the real module units would declare)
// ======================================================================================

/// @brief One module unit of the emulated hierarchy.
struct ModuleUnit {
    std::string name;                 ///< Module name, e.g. "CppVerseHub.SpaceGame.Core".
    std::vector<std::string> imports; ///< Modules it imports.
    std::vector<std::string> exports; ///< Entities it exports.
};

/// @brief The emulated module dependency graph. @return All module units.
[[nodiscard]] std::vector<ModuleUnit> moduleGraph();

/// @brief A build order in which every module is compiled after its imports (Kahn's algorithm).
/// @param graph Module units. @return Module names, or nullopt if the graph has a cycle or an unknown import.
[[nodiscard]] std::optional<std::vector<std::string>> topologicalBuildOrder(
    const std::vector<ModuleUnit>& graph);

/// @brief Source text of the module interface units this file emulates. @return C++ module code.
[[nodiscard]] std::string moduleInterfaceSketch();

/// @brief Showcase of the emulated module system and the game it organises. @param out Destination stream.
void demonstrateModules(std::ostream& out = std::cout);

} // namespace CppVerseHub::Modern::Modules
