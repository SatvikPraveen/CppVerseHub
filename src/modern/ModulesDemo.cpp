// File: src/modern/ModulesDemo.cpp
// C++20 Modules System Implementation

#include "ModulesDemo.hpp"
#include <algorithm>
#include <random>
#include <sstream>
#include <cmath>
#include <iomanip>

namespace CppVerseHub::Modern::Modules {

// ===== CORE MODULE IMPLEMENTATION =====

namespace SpaceGame::Core {
    
    std::string generateUniqueId(const std::string& prefix) {
        static int counter = 1000;
        return prefix + "_" + std::to_string(++counter);
    }
    
    double calculateDistance(double x1, double y1, double x2, double y2) {
        double dx = x2 - x1;
        double dy = y2 - y1;
        return std::sqrt(dx * dx + dy * dy);
    }
    
    std::vector<std::string> parseCommaSeparatedList(const std::string& input) {
        std::vector<std::string> result;
        std::stringstream ss(input);
        std::string item;
        
        while (std::getline(ss, item, ',')) {
            // Trim whitespace
            item.erase(0, item.find_first_not_of(" \t"));
            item.erase(item.find_last_not_of(" \t") + 1);
            if (!item.empty()) {
                result.push_back(item);
            }
        }
        
        return result;
    }
    
} // namespace SpaceGame::Core

// ===== ENTITIES MODULE IMPLEMENTATION =====

namespace SpaceGame::Entities {
    
    // Planet Implementation
    Planet::Planet(int id, std::string name, double x, double y, double z, 
                   long long population, bool habitable)
        : id_(id), name_(std::move(name)), x_(x), y_(y), z_(z), 
          population_(population), habitable_(habitable) {
    }
    
    void Planet::update(double deltaTime) {
        // Simulate population growth
        if (habitable_ && population_ > 0) {
            double growth_rate = 0.001; // 0.1% per time unit
            population_ += static_cast<long long>(population_ * growth_rate * deltaTime);
        }
    }
    
    void Planet::setPosition(double x, double y, double z) {
        x_ = x;
        y_ = y;
        z_ = z;
    }
    
    std::tuple<double, double, double> Planet::getPosition() const {
        return std::make_tuple(x_, y_, z_);
    }
    
    void Planet::addResource(const std::string& resource) {
        if (std::find(resources_.begin(), resources_.end(), resource) == resources_.end()) {
            resources_.push_back(resource);
        }
    }
    
    double Planet::distanceTo(const Planet& other) const {
        double dx = other.x_ - x_;
        double dy = other.y_ - y_;
        double dz = other.z_ - z_;
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }
    
    // Starship Implementation
    Starship::Starship(int id, std::string name, std::string class_type, 
                       double x, double y, double z, int crew_size)
        : id_(id), name_(std::move(name)), class_type_(std::move(class_type)),
          x_(x), y_(y), z_(z), velocity_x_(0.0), velocity_y_(0.0), velocity_z_(0.0),
          fuel_(100.0), max_fuel_(100.0), crew_size_(crew_size) {
    }
    
    void Starship::update(double deltaTime) {
        // Update position based on velocity
        x_ += velocity_x_ * deltaTime;
        y_ += velocity_y_ * deltaTime;
        z_ += velocity_z_ * deltaTime;
        
        // Consume fuel based on movement
        double speed = std::sqrt(velocity_x_ * velocity_x_ + 
                                velocity_y_ * velocity_y_ + 
                                velocity_z_ * velocity_z_);
        if (speed > 0) {
            double fuel_consumption = speed * 0.1 * deltaTime; // Fuel consumption rate
            fuel_ = std::max(0.0, fuel_ - fuel_consumption);
        }
    }
    
    void Starship::setPosition(double x, double y, double z) {
        x_ = x;
        y_ = y;
        z_ = z;
    }
    
    std::tuple<double, double, double> Starship::getPosition() const {
        return std::make_tuple(x_, y_, z_);
    }
    
    void Starship::setVelocity(double vx, double vy, double vz) {
        velocity_x_ = vx;
        velocity_y_ = vy;
        velocity_z_ = vz;
    }
    
    std::tuple<double, double, double> Starship::getVelocity() const {
        return std::make_tuple(velocity_x_, velocity_y_, velocity_z_);
    }
    
    void Starship::refuel(double amount) {
        fuel_ = std::min(max_fuel_, fuel_ + amount);
    }
    
    double Starship::getFuelPercentage() const {
        return (fuel_ / max_fuel_) * 100.0;
    }
    
    bool Starship::hasEnoughFuelFor(double distance) const {
        double fuel_needed = distance * 0.1; // Fuel consumption per unit distance
        return fuel_ >= fuel_needed;
    }
    
} // namespace SpaceGame::Entities

// ===== MISSIONS MODULE IMPLEMENTATION =====

namespace SpaceGame::Missions {
    
    Mission::Mission(int id, std::string name, MissionType type, int priority)
        : id_(id), name_(std::move(name)), type_(type), status_(MissionStatus::Pending),
          progress_(0.0), priority_(priority), estimated_duration_(10.0), elapsed_time_(0.0) {
    }
    
    void Mission::start() {
        if (status_ == MissionStatus::Pending) {
            status_ = MissionStatus::InProgress;
            elapsed_time_ = 0.0;
            progress_ = 0.0;
        }
    }
    
    void Mission::complete() {
        status_ = MissionStatus::Completed;
        progress_ = 100.0;
    }
    
    void Mission::fail() {
        status_ = MissionStatus::Failed;
    }
    
    void Mission::cancel() {
        status_ = MissionStatus::Cancelled;
    }
    
    void Mission::update(double deltaTime) {
        if (status_ == MissionStatus::InProgress) {
            elapsed_time_ += deltaTime;
            progress_ = std::min(100.0, (elapsed_time_ / estimated_duration_) * 100.0);
            
            // Auto-complete when progress reaches 100%
            if (progress_ >= 100.0) {
                complete();
            }
        }
    }
    
    void Mission::assignShip(int ship_id) {
        if (std::find(assigned_ship_ids_.begin(), assigned_ship_ids_.end(), ship_id) 
            == assigned_ship_ids_.end()) {
            assigned_ship_ids_.push_back(ship_id);
        }
    }
    
    void Mission::unassignShip(int ship_id) {
        assigned_ship_ids_.erase(
            std::remove(assigned_ship_ids_.begin(), assigned_ship_ids_.end(), ship_id),
            assigned_ship_ids_.end());
    }
    
    std::string Mission::getStatusString() const {
        switch (status_) {
            case MissionStatus::Pending: return "Pending";
            case MissionStatus::InProgress: return "In Progress";
            case MissionStatus::Completed: return "Completed";
            case MissionStatus::Failed: return "Failed";
            case MissionStatus::Cancelled: return "Cancelled";
            default: return "Unknown";
        }
    }
    
    std::string Mission::getTypeString() const {
        switch (type_) {
            case MissionType::Exploration: return "Exploration";
            case MissionType::Combat: return "Combat";
            case MissionType::Colonization: return "Colonization";
            case MissionType::Trade: return "Trade";
            case MissionType::Rescue: return "Rescue";
            case MissionType::Research: return "Research";
            case MissionType::Patrol: return "Patrol";
            default: return "Unknown";
        }
    }
    
    // MissionFactory Implementation
    std::unique_ptr<Mission> MissionFactory::createExplorationMission(
        int id, const std::string& target_system) {
        auto mission = std::make_unique<Mission>(
            id, "Explore " + target_system, MissionType::Exploration, 2);
        mission->setEstimatedDuration(15.0);
        return mission;
    }
    
    std::unique_ptr<Mission> MissionFactory::createCombatMission(
        int id, const std::string& enemy_location) {
        auto mission = std::make_unique<Mission>(
            id, "Engage enemies at " + enemy_location, MissionType::Combat, 5);
        mission->setEstimatedDuration(8.0);
        return mission;
    }
    
    std::unique_ptr<Mission> MissionFactory::createColonizationMission(
        int id, const std::string& target_planet) {
        auto mission = std::make_unique<Mission>(
            id, "Colonize " + target_planet, MissionType::Colonization, 1);
        mission->setEstimatedDuration(25.0);
        return mission;
    }
    
    std::unique_ptr<Mission> MissionFactory::createTradeMission(
        int id, const std::string& trade_route) {
        auto mission = std::make_unique<Mission>(
            id, "Trade mission: " + trade_route, MissionType::Trade, 3);
        mission->setEstimatedDuration(12.0);
        return mission;
    }
    
    std::unique_ptr<Mission> MissionFactory::createRescueMission(
        int id, const std::string& distress_location) {
        auto mission = std::make_unique<Mission>(
            id, "Rescue operation at " + distress_location, MissionType::Rescue, 5);
        mission->setEstimatedDuration(5.0);
        return mission;
    }
    
} // namespace SpaceGame::Missions

// ===== FLEET MODULE IMPLEMENTATION =====

namespace SpaceGame::Fleet {
    
    // FleetCommander Implementation
    FleetCommander::FleetCommander(int id, std::string name, std::string rank, int experience)
        : id_(id), name_(std::move(name)), rank_(std::move(rank)), experience_level_(experience) {
    }
    
    void FleetCommander::addSpecialization(const std::string& specialization) {
        if (std::find(specializations_.begin(), specializations_.end(), specialization) 
            == specializations_.end()) {
            specializations_.push_back(specialization);
        }
    }
    
    bool FleetCommander::hasSpecialization(const std::string& specialization) const {
        return std::find(specializations_.begin(), specializations_.end(), specialization) 
               != specializations_.end();
    }
    
    void FleetCommander::promoteRank(const std::string& new_rank) {
        rank_ = new_rank;
        experience_level_ += 10; // Gain experience from promotion
    }
    
    // FleetFormation Implementation
    FleetFormation::FleetFormation(int id, std::string name, std::unique_ptr<FleetCommander> commander)
        : formation_id_(id), name_(std::move(name)), commander_(std::move(commander)) {
    }
    
    void FleetFormation::addShip(std::unique_ptr<Starship> ship) {
        ships_.push_back(std::move(ship));
    }
    
    void FleetFormation::removeShip(int ship_id) {
        ships_.erase(
            std::remove_if(ships_.begin(), ships_.end(),
                [ship_id](const auto& ship) { return ship->getId() == ship_id; }),
            ships_.end());
    }
    
    Starship* FleetFormation::findShip(int ship_id) const {
        auto it = std::find_if(ships_.begin(), ships_.end(),
            [ship_id](const auto& ship) { return ship->getId() == ship_id; });
        return (it != ships_.end()) ? it->get() : nullptr;
    }
    
    void FleetFormation::assignMission(std::unique_ptr<Mission> mission) {
        // Assign all ships in formation to the mission
        for (const auto& ship : ships_) {
            mission->assignShip(ship->getId());
        }
        mission->start();
        active_missions_.push_back(std::move(mission));
    }
    
    void FleetFormation::completeMission(int mission_id) {
        auto it = std::find_if(active_missions_.begin(), active_missions_.end(),
            [mission_id](const auto& mission) { return mission->getId() == mission_id; });
        
        if (it != active_missions_.end()) {
            (*it)->complete();
            commander_->gainExperience(5); // Commander gains experience
        }
    }
    
    void FleetFormation::abortMission(int mission_id) {
        auto it = std::find_if(active_missions_.begin(), active_missions_.end(),
            [mission_id](const auto& mission) { return mission->getId() == mission_id; });
        
        if (it != active_missions_.end()) {
            (*it)->cancel();
        }
    }
    
    double FleetFormation::getAverageFuelLevel() const {
        if (ships_.empty()) return 0.0;
        
        double total_fuel = 0.0;
        for (const auto& ship : ships_) {
            total_fuel += ship->getFuelPercentage();
        }
        
        return total_fuel / ships_.size();
    }
    
    int FleetFormation::getTotalCrewSize() const {
        int total_crew = 0;
        for (const auto& ship : ships_) {
            total_crew += ship->getCrewSize();
        }
        return total_crew;
    }
    
    void FleetFormation::update(double deltaTime) {
        // Update all ships
        for (auto& ship : ships_) {
            ship->update(deltaTime);
        }
        
        // Update all missions
        for (auto& mission : active_missions_) {
            mission->update(deltaTime);
        }
        
        // Remove completed missions
        active_missions_.erase(
            std::remove_if(active_missions_.begin(), active_missions_.end(),
                [](const auto& mission) { 
                    return mission->getStatus() == MissionStatus::Completed ||
                           mission->getStatus() == MissionStatus::Failed ||
                           mission->getStatus() == MissionStatus::Cancelled;
                }),
            active_missions_.end());
    }
    
    void FleetFormation::refuelAllShips() {
        for (auto& ship : ships_) {
            ship->refuel(100.0); // Full refuel
        }
    }
    
    std::vector<std::string> FleetFormation::getFleetReport() const {
        std::vector<std::string> report;
        
        report.push_back("Fleet: " + name_ + " (ID: " + std::to_string(formation_id_) + ")");
        report.push_back("Commander: " + commander_->getName() + " (" + commander_->getRank() + ")");
        report.push_back("Ships: " + std::to_string(ships_.size()));
        report.push_back("Total Crew: " + std::to_string(getTotalCrewSize()));
        report.push_back("Average Fuel: " + std::to_string(getAverageFuelLevel()) + "%");
        report.push_back("Active Missions: " + std::to_string(active_missions_.size()));
        
        return report;
    }
    
} // namespace SpaceGame::Fleet

// ===== SYSTEM MODULE IMPLEMENTATION =====

namespace SpaceGame::System {
    
    GameUniverse::GameUniverse() : game_time_(0.0), next_id_(1) {
    }
    
    void GameUniverse::update(double deltaTime) {
        game_time_ += deltaTime;
        
        // Update all planets
        for (auto& planet : planets_) {
            planet->update(deltaTime);
        }
        
        // Update all fleets
        for (auto& fleet : fleets_) {
            fleet->update(deltaTime);
        }
        
        // Update global missions
        for (auto& mission : global_missions_) {
            mission->update(deltaTime);
        }
        
        // Remove completed global missions
        global_missions_.erase(
            std::remove_if(global_missions_.begin(), global_missions_.end(),
                [](const auto& mission) { 
                    return mission->getStatus() == MissionStatus::Completed ||
                           mission->getStatus() == MissionStatus::Failed ||
                           mission->getStatus() == MissionStatus::Cancelled;
                }),
            global_missions_.end());
    }
    
    void GameUniverse::reset() {
        planets_.clear();
        fleets_.clear();
        global_missions_.clear();
        game_time_ = 0.0;
        next_id_ = 1;
    }
    
    void GameUniverse::addPlanet(std::unique_ptr<Planet> planet) {
        planets_.push_back(std::move(planet));
    }
    
    Planet* GameUniverse::findPlanet(int id) const {
        auto it = std::find_if(planets_.begin(), planets_.end(),
            [id](const auto& planet) { return planet->getId() == id; });
        return (it != planets_.end()) ? it->get() : nullptr;
    }
    
    Planet* GameUniverse::findPlanetByName(const std::string& name) const {
        auto it = std::find_if(planets_.begin(), planets_.end(),
            [&name](const auto& planet) { return planet->getName() == name; });
        return (it != planets_.end()) ? it->get() : nullptr;
    }
    
    std::vector<Planet*> GameUniverse::findHestablePlanets() const {
        std::vector<Planet*> habitable;
        for (const auto& planet : planets_) {
            if (planet->isHabitable()) {
                habitable.push_back(planet.get());
            }
        }
        return habitable;
    }
    
    void GameUniverse::addFleet(std::unique_ptr<FleetFormation> fleet) {
        fleets_.push_back(std::move(fleet));
    }
    
    FleetFormation* GameUniverse::findFleet(int id) const {
        auto it = std::find_if(fleets_.begin(), fleets_.end(),
            [id](const auto& fleet) { return fleet->getId() == id; });
        return (it != fleets_.end()) ? it->get() : nullptr;
    }
    
    FleetFormation* GameUniverse::findFleetByName(const std::string& name) const {
        auto it = std::find_if(fleets_.begin(), fleets_.end(),
            [&name](const auto& fleet) { return fleet->getName() == name; });
        return (it != fleets_.end()) ? it->get() : nullptr;
    }
    
    void GameUniverse::addGlobalMission(std::unique_ptr<Mission> mission) {
        global_missions_.push_back(std::move(mission));
    }
    
    Mission* GameUniverse::findMission(int id) const {
        auto it = std::find_if(global_missions_.begin(), global_missions_.end(),
            [id](const auto& mission) { return mission->getId() == id; });
        return (it != global_missions_.end()) ? it->get() : nullptr;
    }
    
    std::vector<Mission*> GameUniverse::findMissionsByType(MissionType type) const {
        std::vector<Mission*> missions;
        for (const auto& mission : global_missions_) {
            if (mission->getType() == type) {
                missions.push_back(mission.get());
            }
        }
        return missions;
    }
    
    std::vector<Mission*> GameUniverse::findMissionsByStatus(MissionStatus status) const {
        std::vector<Mission*> missions;
        for (const auto& mission : global_missions_) {
            if (mission->getStatus() == status) {
                missions.push_back(mission.get());
            }
        }
        return missions;
    }
    
    long long GameUniverse::getTotalPopulation() const {
        long long total = 0;
        for (const auto& planet : planets_) {
            total += planet->getPopulation();
        }
        return total;
    }
    
    std::vector<std::string> GameUniverse::getUniverseReport() const {
        std::vector<std::string> report;
        
        report.push_back("=== UNIVERSE STATUS REPORT ===");
        report.push_back("Game Time: " + std::to_string(game_time_) + " time units");
        report.push_back("Planets: " + std::to_string(planets_.size()));
        report.push_back("Fleets: " + std::to_string(fleets_.size()));
        report.push_back("Global Missions: " + std::to_string(global_missions_.size()));
        report.push_back("Total Population: " + std::to_string(getTotalPopulation()));
        
        auto habitable = findHestablePlanets();
        report.push_back("Habitable Planets: " + std::to_string(habitable.size()));
        
        return report;
    }
    
    // GameUtilities Implementation
    std::unique_ptr<GameUniverse> GameUtilities::createSampleUniverse() {
        auto universe = std::make_unique<GameUniverse>();
        populateWithSampleData(*universe);
        return universe;
    }
    
    void GameUtilities::populateWithSampleData(GameUniverse& universe) {
        // Add sample planets
        auto earth = std::make_unique<Planet>(universe.generateNextId(), "Earth", 0.0, 0.0, 0.0, 8000000000LL, true);
        earth->addResource("Water");
        earth->addResource("Oxygen");
        earth->addResource("Iron");
        universe.addPlanet(std::move(earth));
        
        auto mars = std::make_unique<Planet>(universe.generateNextId(), "Mars", 10.0, 5.0, 0.0, 0, false);
        mars->addResource("Iron");
        mars->addResource("Silicon");
        universe.addPlanet(std::move(mars));
        
        auto kepler = std::make_unique<Planet>(universe.generateNextId(), "Kepler-442b", 100.0, 50.0, 25.0, 50000000LL, true);
        kepler->addResource("Rare_Metals");
        kepler->addResource("Water");
        universe.addPlanet(std::move(kepler));
        
        // Add sample fleet
        auto commander = std::make_unique<FleetCommander>(universe.generateNextId(), "Admiral Zhang", "Admiral", 15);
        commander->addSpecialization("Exploration");
        commander->addSpecialization("Combat");
        
        auto fleet = std::make_unique<FleetFormation>(universe.generateNextId(), "Alpha Fleet", std::move(commander));
        
        auto ship1 = std::make_unique<Starship>(universe.generateNextId(), "USS Explorer", "Explorer-class", 0.0, 0.0, 0.0, 150);
        auto ship2 = std::make_unique<Starship>(universe.generateNextId(), "USS Guardian", "Battleship-class", 1.0, 0.0, 0.0, 300);
        
        fleet->addShip(std::move(ship1));
        fleet->addShip(std::move(ship2));
        
        universe.addFleet(std::move(fleet));
        
        // Add sample global mission
        auto exploration_mission = MissionFactory::createExplorationMission(
            universe.generateNextId(), "Kepler System");
        universe.addGlobalMission(std::move(exploration_mission));
    }
    
    void GameUtilities::runSimulation(GameUniverse& universe, double duration, double time_step) {
        double elapsed = 0.0;
        while (elapsed < duration) {
            universe.update(time_step);
            elapsed += time_step;
        }
    }
    
    std::string GameUtilities::serializeUniverse(const GameUniverse& universe) {
        std::stringstream ss;
        ss << "GameTime:" << universe.getGameTime() << "\n";
        ss << "Planets:" << universe.getPlanetCount() << "\n";
        ss << "Fleets:" << universe.getFleetCount() << "\n";
        ss << "Missions:" << universe.getMissionCount() << "\n";
        return ss.str();
    }
    
    std::unique_ptr<GameUniverse> GameUtilities::deserializeUniverse(const std::string& data) {
        // Simple deserialization implementation
        auto universe = std::make_unique<GameUniverse>();
        // In a real implementation, parse the data and recreate the universe
        populateWithSampleData(*universe);
        return universe;
    }
    
} // namespace SpaceGame::System

} // namespace CppVerseHub::Modern::Modules