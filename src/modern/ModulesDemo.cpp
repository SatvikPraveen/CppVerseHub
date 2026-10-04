/**
 * @file ModulesDemo.cpp
 * @brief "Module implementation unit" of the emulated SpaceGame modules (see ModulesDemo.hpp).
 *
 * Helpers in the anonymous namespace play the role of non-exported, module-linkage entities: they are
 * invisible to every other translation unit, just as unexported names are invisible to importers.
 */
#include "modern/ModulesDemo.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iomanip>
#include <map>
#include <sstream>
#include <stdexcept>

namespace CppVerseHub::Modern::Modules {

namespace {

// ---- non-exported helpers ("module-private") ----

std::string_view trim(std::string_view s) noexcept {
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front())) != 0) {
        s.remove_prefix(1);
    }
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back())) != 0) {
        s.remove_suffix(1);
    }
    return s;
}

std::vector<std::string_view> splitFields(std::string_view line, char delimiter) {
    std::vector<std::string_view> fields;
    std::size_t start = 0;
    while (true) {
        const std::size_t pos = line.find(delimiter, start);
        fields.push_back(
            line.substr(start, pos == std::string_view::npos ? std::string_view::npos : pos - start));
        if (pos == std::string_view::npos) {
            break;
        }
        start = pos + 1;
    }
    return fields;
}

template <typename T>
T parseNumber(std::string_view text) {
    std::istringstream is{std::string(text)};
    T value{};
    is >> value;
    if (!is || !is.eof()) {
        throw std::invalid_argument("malformed number: '" + std::string(text) + "'");
    }
    return value;
}

} // namespace

// ===== Core =====

namespace SpaceGame::Core::inline v1 {

double calculateDistance(const Vec3& a, const Vec3& b) noexcept {
    const double dx = b.x - a.x;
    const double dy = b.y - a.y;
    const double dz = b.z - a.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

std::vector<std::string> parseCommaSeparatedList(std::string_view input) {
    std::vector<std::string> items;
    for (std::string_view field : splitFields(input, ',')) {
        field = trim(field);
        if (!field.empty()) {
            items.emplace_back(field);
        }
    }
    return items;
}

IdGenerator::IdGenerator(std::string prefix) : prefix_(std::move(prefix)) {}

std::string IdGenerator::next() {
    return prefix_ + '-' + std::to_string(++counter_);
}

} // namespace SpaceGame::Core::inline v1

// ===== Entities =====

namespace SpaceGame::Entities {

Planet::Planet(int id, std::string name, Core::Vec3 position, long long population, bool habitable)
    : id_(id)
    , name_(std::move(name))
    , position_(position)
    , population_(population < 0 ? 0 : population)
    , habitable_(habitable) {}

void Planet::update(double deltaTime) {
    if (habitable_ && deltaTime > 0.0 && population_ > 0) {
        const double grown = static_cast<double>(population_) * (1.0 + 0.01 * deltaTime);
        population_ = std::llround(grown);
    }
}

void Planet::addResource(const std::string& resource) {
    if (std::find(resources_.begin(), resources_.end(), resource) == resources_.end()) {
        resources_.push_back(resource);
    }
}

double Planet::distanceTo(const Planet& other) const noexcept {
    return Core::calculateDistance(position_, other.position_);
}

Starship::Starship(int id, std::string name, std::string classType, Core::Vec3 position, int crewSize)
    : id_(id)
    , name_(std::move(name))
    , classType_(std::move(classType))
    , position_(position)
    , crewSize_(crewSize) {}

void Starship::update(double deltaTime) {
    if (deltaTime <= 0.0) {
        return;
    }
    const Core::Vec3 step{velocity_.x * deltaTime, velocity_.y * deltaTime, velocity_.z * deltaTime};
    const double distance = Core::calculateDistance({}, step);
    if (!hasEnoughFuelFor(distance)) {
        velocity_ = {}; // out of fuel: drift to a halt
        return;
    }
    position_ = {position_.x + step.x, position_.y + step.y, position_.z + step.z};
    fuel_ -= distance * kFuelPerUnit;
}

void Starship::refuel(double amount) noexcept {
    if (amount > 0.0) {
        fuel_ = std::min(kMaxFuel, fuel_ + amount);
    }
}

} // namespace SpaceGame::Entities

// ===== Missions =====

namespace SpaceGame::Missions {

std::string_view toString(MissionStatus s) noexcept {
    switch (s) {
        case MissionStatus::Pending:
            return "Pending";
        case MissionStatus::InProgress:
            return "InProgress";
        case MissionStatus::Completed:
            return "Completed";
        case MissionStatus::Failed:
            return "Failed";
        case MissionStatus::Cancelled:
            return "Cancelled";
    }
    return "Unknown";
}

std::string_view toString(MissionType t) noexcept {
    switch (t) {
        case MissionType::Exploration:
            return "Exploration";
        case MissionType::Combat:
            return "Combat";
        case MissionType::Colonization:
            return "Colonization";
        case MissionType::Trade:
            return "Trade";
        case MissionType::Rescue:
            return "Rescue";
    }
    return "Unknown";
}

Mission::Mission(int id, std::string name, MissionType type, double estimatedDuration, int priority)
    : id_(id)
    , name_(std::move(name))
    , type_(type)
    , priority_(priority)
    , estimatedDuration_(estimatedDuration > 0.0 ? estimatedDuration : 1.0) {}

bool Mission::start() noexcept {
    if (status_ != MissionStatus::Pending) {
        return false;
    }
    status_ = MissionStatus::InProgress;
    return true;
}

bool Mission::fail() noexcept {
    if (status_ != MissionStatus::InProgress) {
        return false;
    }
    status_ = MissionStatus::Failed;
    return true;
}

bool Mission::cancel() noexcept {
    if (status_ != MissionStatus::Pending && status_ != MissionStatus::InProgress) {
        return false;
    }
    status_ = MissionStatus::Cancelled;
    return true;
}

void Mission::update(double deltaTime) noexcept {
    if (status_ != MissionStatus::InProgress || deltaTime <= 0.0) {
        return;
    }
    elapsed_ = std::min(estimatedDuration_, elapsed_ + deltaTime);
    progress_ = elapsed_ / estimatedDuration_ * 100.0;
    if (elapsed_ >= estimatedDuration_) {
        progress_ = 100.0;
        status_ = MissionStatus::Completed;
    }
}

bool Mission::assignShip(int shipId) {
    if (std::find(assignedShips_.begin(), assignedShips_.end(), shipId) != assignedShips_.end()) {
        return false;
    }
    assignedShips_.push_back(shipId);
    return true;
}

bool Mission::unassignShip(int shipId) {
    auto it = std::find(assignedShips_.begin(), assignedShips_.end(), shipId);
    if (it == assignedShips_.end()) {
        return false;
    }
    assignedShips_.erase(it);
    return true;
}

double Mission::getRemainingTime() const noexcept {
    return std::max(0.0, estimatedDuration_ - elapsed_);
}

std::unique_ptr<Mission> MissionFactory::create(int id, MissionType type, const std::string& target) {
    double duration = 10.0;
    int priority = 1;
    switch (type) {
        case MissionType::Exploration:
            duration = 20.0;
            priority = 2;
            break;
        case MissionType::Combat:
            duration = 8.0;
            priority = 5;
            break;
        case MissionType::Colonization:
            duration = 40.0;
            priority = 3;
            break;
        case MissionType::Trade:
            duration = 12.0;
            priority = 1;
            break;
        case MissionType::Rescue:
            duration = 5.0;
            priority = 4;
            break;
    }
    return std::make_unique<Mission>(id, std::string(toString(type)) + ": " + target, type, duration,
                                     priority);
}

} // namespace SpaceGame::Missions

// ===== Fleet =====

namespace SpaceGame::Fleet {

FleetFormation::FleetFormation(int id, std::string name, FleetCommander commander)
    : id_(id), name_(std::move(name)), commander_(std::move(commander)) {}

void FleetFormation::addShip(std::unique_ptr<Entities::Starship> ship) {
    if (ship) {
        ships_.push_back(std::move(ship));
    }
}

std::unique_ptr<Entities::Starship> FleetFormation::removeShip(int shipId) {
    auto it = std::find_if(ships_.begin(), ships_.end(),
                           [shipId](const auto& s) { return s->getId() == shipId; });
    if (it == ships_.end()) {
        return nullptr;
    }
    std::unique_ptr<Entities::Starship> ship = std::move(*it);
    ships_.erase(it);
    for (auto& m : missions_) {
        (void)m->unassignShip(shipId);
    }
    return ship;
}

Entities::Starship* FleetFormation::findShip(int shipId) const noexcept {
    for (const auto& s : ships_) {
        if (s->getId() == shipId) {
            return s.get();
        }
    }
    return nullptr;
}

void FleetFormation::assignMission(std::unique_ptr<Missions::Mission> mission) {
    if (!mission) {
        return;
    }
    (void)mission->start();
    for (const auto& s : ships_) {
        (void)mission->assignShip(s->getId());
    }
    missions_.push_back(std::move(mission));
}

void FleetFormation::update(double deltaTime) {
    for (auto& s : ships_) {
        s->update(deltaTime);
    }
    for (auto& m : missions_) {
        m->update(deltaTime);
    }
    const auto before = missions_.size();
    std::erase_if(missions_,
                  [](const auto& m) { return m->getStatus() == Missions::MissionStatus::Completed; });
    completedMissions_ += before - missions_.size();
}

void FleetFormation::refuelAll() noexcept {
    for (auto& s : ships_) {
        s->refuel(Entities::Starship::kMaxFuel);
    }
}

double FleetFormation::getAverageFuelLevel() const noexcept {
    if (ships_.empty()) {
        return 0.0;
    }
    double total = 0.0;
    for (const auto& s : ships_) {
        total += s->getFuelPercentage();
    }
    return total / static_cast<double>(ships_.size());
}

int FleetFormation::getTotalCrewSize() const noexcept {
    int total = 0;
    for (const auto& s : ships_) {
        total += s->getCrewSize();
    }
    return total;
}

} // namespace SpaceGame::Fleet

// ===== System =====

namespace SpaceGame::System {

void GameUniverse::addPlanet(std::unique_ptr<Entities::Planet> planet) {
    if (planet) {
        planets_.push_back(std::move(planet));
    }
}

void GameUniverse::addFleet(std::unique_ptr<Fleet::FleetFormation> fleet) {
    if (fleet) {
        fleets_.push_back(std::move(fleet));
    }
}

void GameUniverse::addMission(std::unique_ptr<Missions::Mission> mission) {
    if (mission) {
        missions_.push_back(std::move(mission));
    }
}

void GameUniverse::update(double deltaTime) {
    if (deltaTime <= 0.0) {
        return;
    }
    gameTime_ += deltaTime;
    for (auto& p : planets_) {
        p->update(deltaTime);
    }
    for (auto& f : fleets_) {
        f->update(deltaTime);
    }
    for (auto& m : missions_) {
        m->update(deltaTime);
    }
}

void GameUniverse::runSimulation(int steps, double timeStep) {
    for (int i = 0; i < steps; ++i) {
        update(timeStep);
    }
}

const Entities::Planet* GameUniverse::findPlanetByName(std::string_view name) const noexcept {
    for (const auto& p : planets_) {
        if (p->getName() == name) {
            return p.get();
        }
    }
    return nullptr;
}

std::vector<const Entities::Planet*> GameUniverse::findHabitablePlanets() const {
    std::vector<const Entities::Planet*> result;
    for (const auto& p : planets_) {
        if (p->isHabitable()) {
            result.push_back(p.get());
        }
    }
    return result;
}

std::vector<const Missions::Mission*> GameUniverse::findMissionsByStatus(
    Missions::MissionStatus status) const {
    std::vector<const Missions::Mission*> result;
    for (const auto& m : missions_) {
        if (m->getStatus() == status) {
            result.push_back(m.get());
        }
    }
    return result;
}

long long GameUniverse::getTotalPopulation() const noexcept {
    long long total = 0;
    for (const auto& p : planets_) {
        total += p->getPopulation();
    }
    return total;
}

std::vector<std::string> GameUniverse::getUniverseReport() const {
    std::vector<std::string> lines;
    std::ostringstream header;
    header << "t=" << gameTime_ << " planets=" << planets_.size() << " fleets=" << fleets_.size()
           << " missions=" << missions_.size() << " population=" << getTotalPopulation();
    lines.push_back(header.str());
    for (const auto& f : fleets_) {
        std::ostringstream os;
        os << std::fixed << std::setprecision(1) << "fleet " << f->getName() << " (" << f->getCommander().rank
           << ' ' << f->getCommander().name << "): ships=" << f->getShipCount()
           << " crew=" << f->getTotalCrewSize() << " fuel=" << f->getAverageFuelLevel()
           << "% active=" << f->getActiveMissionCount() << " completed=" << f->getCompletedMissionCount();
        lines.push_back(os.str());
    }
    for (const auto& m : missions_) {
        std::ostringstream os;
        os << std::fixed << std::setprecision(1) << "mission " << m->getName() << ": "
           << toString(m->getStatus()) << ' ' << m->getProgress() << '%';
        lines.push_back(os.str());
    }
    return lines;
}

std::string GameUniverse::serializePlanets() const {
    std::ostringstream os;
    os << std::setprecision(17);
    for (const auto& p : planets_) {
        const std::string& name = p->getName();
        if (name.find_first_of(";\n") != std::string::npos) {
            throw std::invalid_argument("planet name contains a reserved character: " + name);
        }
        const auto& pos = p->getPosition();
        os << name << ';' << pos.x << ';' << pos.y << ';' << pos.z << ';' << p->getPopulation() << ';'
           << (p->isHabitable() ? 1 : 0) << '\n';
    }
    return os.str();
}

std::vector<std::unique_ptr<Entities::Planet>> GameUniverse::deserializePlanets(std::string_view data) {
    std::vector<std::unique_ptr<Entities::Planet>> planets;
    int nextId = 1;
    for (std::string_view line : splitFields(data, '\n')) {
        if (trim(line).empty()) {
            continue;
        }
        const auto f = splitFields(line, ';');
        if (f.size() != 6 || f[0].empty() || (f[5] != "0" && f[5] != "1")) {
            throw std::invalid_argument("malformed planet record: '" + std::string(line) + "'");
        }
        planets.push_back(std::make_unique<Entities::Planet>(
            nextId++, std::string(f[0]),
            Core::Vec3{parseNumber<double>(f[1]), parseNumber<double>(f[2]), parseNumber<double>(f[3])},
            parseNumber<long long>(f[4]), f[5] == "1"));
    }
    return planets;
}

GameUniverse GameUniverse::createSample() {
    using Entities::Planet;
    using Entities::Starship;
    using Missions::MissionFactory;
    using Missions::MissionType;

    GameUniverse u;
    u.addPlanet(std::make_unique<Planet>(1, "Earth", Core::Vec3{0, 0, 0}, 8'000'000'000LL, true));
    u.addPlanet(std::make_unique<Planet>(2, "Mars", Core::Vec3{50, 0, 0}, 0, false));
    u.addPlanet(std::make_unique<Planet>(3, "Kepler-442b", Core::Vec3{300, 400, 0}, 1'000'000LL, true));
    u.addPlanet(std::make_unique<Planet>(4, "Proxima-b", Core::Vec3{-120, 80, 30}, 0, true));

    auto alpha = std::make_unique<Fleet::FleetFormation>(1, "Alpha",
                                                         Fleet::FleetCommander{"Zhang", "Admiral", 9});
    auto explorer = std::make_unique<Starship>(101, "Explorer", "Science", Core::Vec3{}, 150);
    explorer->setVelocity({10, 0, 0});
    alpha->addShip(std::move(explorer));
    alpha->addShip(std::make_unique<Starship>(102, "Guardian", "Battleship", Core::Vec3{}, 300));
    alpha->assignMission(MissionFactory::create(1, MissionType::Rescue, "Mars orbit"));

    auto beta = std::make_unique<Fleet::FleetFormation>(2, "Beta",
                                                        Fleet::FleetCommander{"Okafor", "Captain", 4});
    beta->addShip(std::make_unique<Starship>(201, "Trader", "Freighter", Core::Vec3{50, 0, 0}, 40));
    beta->assignMission(MissionFactory::create(2, MissionType::Trade, "Earth-Mars route"));

    u.addFleet(std::move(alpha));
    u.addFleet(std::move(beta));

    auto colonize = MissionFactory::create(3, MissionType::Colonization, "Kepler-442b");
    (void)colonize->start();
    u.addMission(std::move(colonize));
    u.addMission(MissionFactory::create(4, MissionType::Exploration, "Proxima system"));
    return u;
}

} // namespace SpaceGame::System

// ===== Module metadata =====

std::vector<ModuleUnit> moduleGraph() {
    return {
        {"CppVerseHub.SpaceGame.Core",
         {},
         {"Vec3", "IEntity", "IdGenerator", "calculateDistance", "parseCommaSeparatedList"}},
        {"CppVerseHub.SpaceGame.Entities", {"CppVerseHub.SpaceGame.Core"}, {"Planet", "Starship"}},
        {"CppVerseHub.SpaceGame.Missions",
         {"CppVerseHub.SpaceGame.Core"},
         {"MissionStatus", "MissionType", "Mission", "MissionFactory"}},
        {"CppVerseHub.SpaceGame.Fleet",
         {"CppVerseHub.SpaceGame.Entities", "CppVerseHub.SpaceGame.Missions"},
         {"FleetCommander", "FleetFormation"}},
        {"CppVerseHub.SpaceGame.System",
         {"CppVerseHub.SpaceGame.Core", "CppVerseHub.SpaceGame.Entities", "CppVerseHub.SpaceGame.Missions",
          "CppVerseHub.SpaceGame.Fleet"},
         {"GameUniverse"}},
    };
}

std::optional<std::vector<std::string>> topologicalBuildOrder(const std::vector<ModuleUnit>& graph) {
    std::map<std::string, std::size_t> indegree;
    std::map<std::string, std::vector<std::string>> dependents;
    for (const auto& unit : graph) {
        indegree.try_emplace(unit.name, 0);
    }
    for (const auto& unit : graph) {
        for (const auto& imp : unit.imports) {
            if (indegree.find(imp) == indegree.end()) {
                return std::nullopt; // import of an unknown module
            }
            ++indegree[unit.name];
            dependents[imp].push_back(unit.name);
        }
    }
    // Process in the original declaration order among ready modules for a deterministic result.
    std::vector<std::string> order;
    std::vector<std::string> ready;
    for (const auto& unit : graph) {
        if (indegree[unit.name] == 0) {
            ready.push_back(unit.name);
        }
    }
    while (!ready.empty()) {
        const std::string current = ready.front();
        ready.erase(ready.begin());
        order.push_back(current);
        for (const auto& dep : dependents[current]) {
            if (--indegree[dep] == 0) {
                ready.push_back(dep);
            }
        }
    }
    if (order.size() != indegree.size()) {
        return std::nullopt; // cycle: modules may not have circular imports
    }
    return order;
}

std::string moduleInterfaceSketch() {
    std::ostringstream os;
    for (const auto& unit : moduleGraph()) {
        os << "// " << unit.name << ".cppm\n";
        os << "export module " << unit.name << ";\n";
        for (const auto& imp : unit.imports) {
            os << "import " << imp << ";\n";
        }
        const std::string ns = unit.name.substr(unit.name.rfind('.') + 1);
        os << "export namespace SpaceGame::" << ns << " {\n";
        for (const auto& e : unit.exports) {
            os << "    /* " << e << " */\n";
        }
        os << "}\n\n";
    }
    return os.str();
}

void demonstrateModules(std::ostream& out) {
    using namespace SpaceGame; // NOLINT(google-build-using-namespace) -- function scope, emulates `import`
    out << "\n=== C++20 Modules (emulated with namespaces) ===\n";
    out << "Module graph and a valid build order:\n";
    if (const auto order = topologicalBuildOrder(moduleGraph())) {
        int step = 1;
        for (const auto& name : *order) {
            out << "  " << step++ << ". " << name << '\n';
        }
    }
    out << "Interface unit sketch (first unit):\n";
    const std::string sketch = moduleInterfaceSketch();
    out << sketch.substr(0, sketch.find("\n\n") + 1);

    Core::IdGenerator ids("ship");
    out << "Core::IdGenerator: " << ids.next() << ", " << ids.next() << '\n';
    out << "Core::parseCommaSeparatedList(\" ore, water ,,gas\"):";
    for (const auto& item : Core::parseCommaSeparatedList(" ore, water ,,gas")) {
        out << " [" << item << ']';
    }
    out << '\n';

    auto universe = System::GameUniverse::createSample();
    universe.runSimulation(10, 1.0);
    out << "After 10 simulation steps:\n";
    for (const auto& line : universe.getUniverseReport()) {
        out << "  " << line << '\n';
    }
    const auto* earth = universe.findPlanetByName("Earth");
    const auto* kepler = universe.findPlanetByName("Kepler-442b");
    if (earth != nullptr && kepler != nullptr) {
        out << "Earth -> Kepler-442b distance: " << earth->distanceTo(*kepler) << '\n';
    }
    const auto restored = System::GameUniverse::deserializePlanets(universe.serializePlanets());
    out << "Serialized and restored " << restored.size() << " planets\n";
}

} // namespace CppVerseHub::Modern::Modules
