// File: src/patterns/Builder.hpp
// CppVerseHub - Builder Pattern Implementation for Complex Entity Construction

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <chrono>

namespace CppVerseHub::Patterns {

/**
 * @brief Spacecraft component specifications
 */
struct ComponentSpec {
    std::string type;
    std::string name;
    double mass;
    double power_consumption;
    double reliability;
    std::unordered_map<std::string, double> attributes;
    
    ComponentSpec(const std::string& t, const std::string& n, double m = 0.0, 
                 double pc = 0.0, double r = 1.0)
        : type(t), name(n), mass(m), power_consumption(pc), reliability(r) {}
    
    void setAttribute(const std::string& key, double value) {
        attributes[key] = value;
    }
    
    double getAttribute(const std::string& key, double default_value = 0.0) const {
        auto it = attributes.find(key);
        return (it != attributes.end()) ? it->second : default_value;
    }
};

/**
 * @brief Complete Spacecraft specification
 */
struct SpacecraftSpec {
    std::string name;
    std::string class_type;
    std::string hull_type;
    
    // Core specifications
    double total_mass = 0.0;
    double power_capacity = 0.0;
    double fuel_capacity = 0.0;
    double cargo_capacity = 0.0;
    double crew_capacity = 0.0;
    
    // Performance characteristics
    double max_speed = 0.0;
    double acceleration = 0.0;
    double maneuverability = 0.0;
    double shield_strength = 0.0;
    double armor_rating = 0.0;
    
    // Components
    std::vector<ComponentSpec> engines;
    std::vector<ComponentSpec> weapons;
    std::vector<ComponentSpec> sensors;
    std::vector<ComponentSpec> life_support;
    std::vector<ComponentSpec> special_equipment;
    
    // Metadata
    std::chrono::system_clock::time_point construction_date;
    std::string constructor_id;
    double construction_cost = 0.0;
    
    SpacecraftSpec(const std::string& n = "", const std::string& ct = "")
        : name(n), class_type(ct), construction_date(std::chrono::system_clock::now()) {}
    
    /**
     * @brief Calculate total component mass
     */
    double calculateTotalMass() const {
        double component_mass = 0.0;
        
        for (const auto& engine : engines) component_mass += engine.mass;
        for (const auto& weapon : weapons) component_mass += weapon.mass;
        for (const auto& sensor : sensors) component_mass += sensor.mass;
        for (const auto& ls : life_support) component_mass += ls.mass;
        for (const auto& special : special_equipment) component_mass += special.mass;
        
        return total_mass + component_mass;
    }
    
    /**
     * @brief Calculate total power consumption
     */
    double calculatePowerConsumption() const {
        double power_needed = 0.0;
        
        for (const auto& engine : engines) power_needed += engine.power_consumption;
        for (const auto& weapon : weapons) power_needed += weapon.power_consumption;
        for (const auto& sensor : sensors) power_needed += sensor.power_consumption;
        for (const auto& ls : life_support) power_needed += ls.power_consumption;
        for (const auto& special : special_equipment) power_needed += special.power_consumption;
        
        return power_needed;
    }
    
    /**
     * @brief Validate spacecraft configuration
     */
    bool isValid() const {
        return !name.empty() && 
               !class_type.empty() &&
               calculateTotalMass() > 0.0 &&
               power_capacity >= calculatePowerConsumption() &&
               !engines.empty();
    }
    
    /**
     * @brief Get overall reliability score
     */
    double getReliabilityScore() const {
        if (engines.empty()) return 0.0;
        
        double total_reliability = 0.0;
        int component_count = 0;
        
        for (const auto& engine : engines) { total_reliability += engine.reliability; component_count++; }
        for (const auto& weapon : weapons) { total_reliability += weapon.reliability; component_count++; }
        for (const auto& sensor : sensors) { total_reliability += sensor.reliability; component_count++; }
        for (const auto& ls : life_support) { total_reliability += ls.reliability; component_count++; }
        for (const auto& special : special_equipment) { total_reliability += special.reliability; component_count++; }
        
        return component_count > 0 ? total_reliability / component_count : 0.0;
    }
};

/**
 * @brief Fleet specification for multiple spacecraft
 */
struct FleetSpec {
    std::string fleet_name;
    std::string fleet_designation;
    std::string commander_id;
    std::string home_base;
    
    std::vector<SpacecraftSpec> spacecraft;
    std::vector<std::string> mission_capabilities;
    
    // Fleet-level attributes
    double formation_cohesion = 1.0;
    double communication_range = 1000.0;
    double logistics_efficiency = 1.0;
    
    std::chrono::system_clock::time_point formation_date;
    
    FleetSpec(const std::string& name = "")
        : fleet_name(name), formation_date(std::chrono::system_clock::now()) {}
    
    /**
     * @brief Calculate total fleet mass
     */
    double getTotalMass() const {
        double total = 0.0;
        for (const auto& craft : spacecraft) {
            total += craft.calculateTotalMass();
        }
        return total;
    }
    
    /**
     * @brief Get total fleet firepower
     */
    double getTotalFirepower() const {
        double firepower = 0.0;
        for (const auto& craft : spacecraft) {
            for (const auto& weapon : craft.weapons) {
                firepower += weapon.getAttribute("damage", 0.0);
            }
        }
        return firepower;
    }
    
    /**
     * @brief Get fleet size
     */
    size_t getFleetSize() const {
        return spacecraft.size();
    }
};

/**
 * @brief Abstract Builder interface for spacecraft construction
 */
class ISpacecraftBuilder {
public:
    virtual ~ISpacecraftBuilder() = default;
    
    // Basic construction methods
    virtual ISpacecraftBuilder& setName(const std::string& name) = 0;
    virtual ISpacecraftBuilder& setClass(const std::string& class_type) = 0;
    virtual ISpacecraftBuilder& setHull(const std::string& hull_type, double mass, double armor) = 0;
    
    // Power and capacity methods
    virtual ISpacecraftBuilder& setPowerSystem(double capacity, double efficiency = 1.0) = 0;
    virtual ISpacecraftBuilder& setFuelCapacity(double capacity) = 0;
    virtual ISpacecraftBuilder& setCargoCapacity(double capacity) = 0;
    virtual ISpacecraftBuilder& setCrewCapacity(double capacity) = 0;
    
    // Component addition methods
    virtual ISpacecraftBuilder& addEngine(const ComponentSpec& engine) = 0;
    virtual ISpacecraftBuilder& addWeapon(const ComponentSpec& weapon) = 0;
    virtual ISpacecraftBuilder& addSensor(const ComponentSpec& sensor) = 0;
    virtual ISpacecraftBuilder& addLifeSupport(const ComponentSpec& life_support) = 0;
    virtual ISpacecraftBuilder& addSpecialEquipment(const ComponentSpec& equipment) = 0;
    
    // Shield and defense methods
    virtual ISpacecraftBuilder& setShields(double strength, double recharge_rate = 1.0) = 0;
    virtual ISpacecraftBuilder& setDefenses(double armor_rating, double evasion = 1.0) = 0;
    
    // Performance characteristics
    virtual ISpacecraftBuilder& setPerformance(double max_speed, double acceleration, double maneuverability) = 0;
    
    // Build method
    virtual std::unique_ptr<SpacecraftSpec> build() = 0;
    
    // Reset builder state
    virtual void reset() = 0;
    
    // Validation
    virtual bool canBuild() const = 0;
    virtual std::vector<std::string> getValidationErrors() const = 0;
};

/**
 * @brief Concrete Spacecraft Builder implementation
 */
class SpacecraftBuilder : public ISpacecraftBuilder {
public:
    SpacecraftBuilder() { reset(); }
    
    ISpacecraftBuilder& setName(const std::string& name) override {
        spacecraft_->name = name;
        return *this;
    }
    
    ISpacecraftBuilder& setClass(const std::string& class_type) override {
        spacecraft_->class_type = class_type;
        return *this;
    }
    
    ISpacecraftBuilder& setHull(const std::string& hull_type, double mass, double armor) override {
        spacecraft_->hull_type = hull_type;
        spacecraft_->total_mass = mass;
        spacecraft_->armor_rating = armor;
        return *this;
    }
    
    ISpacecraftBuilder& setPowerSystem(double capacity, double efficiency) override {
        spacecraft_->power_capacity = capacity;
        // Efficiency could be used to modify power consumption calculations
        return *this;
    }
    
    ISpacecraftBuilder& setFuelCapacity(double capacity) override {
        spacecraft_->fuel_capacity = capacity;
        return *this;
    }
    
    ISpacecraftBuilder& setCargoCapacity(double capacity) override {
        spacecraft_->cargo_capacity = capacity;
        return *this;
    }
    
    ISpacecraftBuilder& setCrewCapacity(double capacity) override {
        spacecraft_->crew_capacity = capacity;
        return *this;
    }
    
    ISpacecraftBuilder& addEngine(const ComponentSpec& engine) override {
        spacecraft_->engines.push_back(engine);
        // Update performance based on engine specifications
        spacecraft_->max_speed += engine.getAttribute("thrust", 0.0) / std::max(1.0, spacecraft_->calculateTotalMass());
        spacecraft_->acceleration += engine.getAttribute("acceleration_bonus", 0.0);
        return *this;
    }
    
    ISpacecraftBuilder& addWeapon(const ComponentSpec& weapon) override {
        spacecraft_->weapons.push_back(weapon);
        return *this;
    }
    
    ISpacecraftBuilder& addSensor(const ComponentSpec& sensor) override {
        spacecraft_->sensors.push_back(sensor);
        return *this;
    }
    
    ISpacecraftBuilder& addLifeSupport(const ComponentSpec& life_support) override {
        spacecraft_->life_support.push_back(life_support);
        return *this;
    }
    
    ISpacecraftBuilder& addSpecialEquipment(const ComponentSpec& equipment) override {
        spacecraft_->special_equipment.push_back(equipment);
        return *this;
    }
    
    ISpacecraftBuilder& setShields(double strength, double recharge_rate) override {
        spacecraft_->shield_strength = strength;
        // Recharge rate could be stored as a special attribute
        return *this;
    }
    
    ISpacecraftBuilder& setDefenses(double armor_rating, double evasion) override {
        spacecraft_->armor_rating = armor_rating;
        spacecraft_->maneuverability += evasion;
        return *this;
    }
    
    ISpacecraftBuilder& setPerformance(double max_speed, double acceleration, double maneuverability) override {
        spacecraft_->max_speed = max_speed;
        spacecraft_->acceleration = acceleration;
        spacecraft_->maneuverability = maneuverability;
        return *this;
    }
    
    std::unique_ptr<SpacecraftSpec> build() override {
        if (!canBuild()) {
            return nullptr;
        }
        
        // Finalize construction details
        spacecraft_->construction_date = std::chrono::system_clock::now();
        spacecraft_->construction_cost = calculateConstructionCost();
        
        auto result = std::move(spacecraft_);
        reset(); // Reset for next build
        return result;
    }
    
    void reset() override {
        spacecraft_ = std::make_unique<SpacecraftSpec>();
    }
    
    bool canBuild() const override {
        return spacecraft_ && 
               !spacecraft_->name.empty() && 
               !spacecraft_->class_type.empty() &&
               !spacecraft_->engines.empty() &&
               spacecraft_->power_capacity >= spacecraft_->calculatePowerConsumption();
    }
    
    std::vector<std::string> getValidationErrors() const override {
        std::vector<std::string> errors;
        
        if (!spacecraft_) {
            errors.push_back("No spacecraft being built");
            return errors;
        }
        
        if (spacecraft_->name.empty()) {
            errors.push_back("Spacecraft name is required");
        }
        
        if (spacecraft_->class_type.empty()) {
            errors.push_back("Spacecraft class is required");
        }
        
        if (spacecraft_->engines.empty()) {
            errors.push_back("At least one engine is required");
        }
        
        if (spacecraft_->power_capacity < spacecraft_->calculatePowerConsumption()) {
            errors.push_back("Insufficient power capacity for installed components");
        }
        
        if (spacecraft_->total_mass <= 0.0) {
            errors.push_back("Hull mass must be greater than zero");
        }
        
        return errors;
    }
    
    /**
     * @brief Get current spacecraft being built (for inspection)
     */
    const SpacecraftSpec* getCurrentSpacecraft() const {
        return spacecraft_.get();
    }

private:
    std::unique_ptr<SpacecraftSpec> spacecraft_;
    
    double calculateConstructionCost() const {
        if (!spacecraft_) return 0.0;
        
        double base_cost = spacecraft_->total_mass * 100.0; // Base cost per unit mass
        double component_cost = 0.0;
        
        // Add component costs
        for (const auto& engine : spacecraft_->engines) {
            component_cost += engine.mass * 500.0; // Engines are expensive
        }
        for (const auto& weapon : spacecraft_->weapons) {
            component_cost += weapon.mass * 300.0;
        }
        for (const auto& sensor : spacecraft_->sensors) {
            component_cost += sensor.mass * 200.0;
        }
        for (const auto& ls : spacecraft_->life_support) {
            component_cost += ls.mass * 150.0;
        }
        for (const auto& special : spacecraft_->special_equipment) {
            component_cost += special.mass * 400.0;
        }
        
        return base_cost + component_cost;
    }
};

/**
 * @brief Fleet Builder interface
 */
class IFleetBuilder {
public:
    virtual ~IFleetBuilder() = default;
    
    virtual IFleetBuilder& setFleetName(const std::string& name) = 0;
    virtual IFleetBuilder& setDesignation(const std::string& designation) = 0;
    virtual IFleetBuilder& setCommander(const std::string& commander_id) = 0;
    virtual IFleetBuilder& setHomeBase(const std::string& base) = 0;
    
    virtual IFleetBuilder& addSpacecraft(std::unique_ptr<SpacecraftSpec> spacecraft) = 0;
    virtual IFleetBuilder& addMissionCapability(const std::string& capability) = 0;
    
    virtual IFleetBuilder& setFormationCohesion(double cohesion) = 0;
    virtual IFleetBuilder& setCommunicationRange(double range) = 0;
    virtual IFleetBuilder& setLogisticsEfficiency(double efficiency) = 0;
    
    virtual std::unique_ptr<FleetSpec> build() = 0;
    virtual void reset() = 0;
    virtual bool canBuild() const = 0;
    virtual std::vector<std::string> getValidationErrors() const = 0;
};

/**
 * @brief Concrete Fleet Builder implementation
 */
class FleetBuilder : public IFleetBuilder {
public:
    FleetBuilder() { reset(); }
    
    IFleetBuilder& setFleetName(const std::string& name) override {
        fleet_->fleet_name = name;
        return *this;
    }
    
    IFleetBuilder& setDesignation(const std::string& designation) override {
        fleet_->fleet_designation = designation;
        return *this;
    }
    
    IFleetBuilder& setCommander(const std::string& commander_id) override {
        fleet_->commander_id = commander_id;
        return *this;
    }
    
    IFleetBuilder& setHomeBase(const std::string& base) override {
        fleet_->home_base = base;
        return *this;
    }
    
    IFleetBuilder& addSpacecraft(std::unique_ptr<SpacecraftSpec> spacecraft) override {
        if (spacecraft && spacecraft->isValid()) {
            fleet_->spacecraft.push_back(*spacecraft);
        }
        return *this;
    }
    
    IFleetBuilder& addMissionCapability(const std::string& capability) override {
        fleet_->mission_capabilities.push_back(capability);
        return *this;
    }
    
    IFleetBuilder& setFormationCohesion(double cohesion) override {
        fleet_->formation_cohesion = std::max(0.0, std::min(2.0, cohesion)); // Clamp to reasonable range
        return *this;
    }
    
    IFleetBuilder& setCommunicationRange(double range) override {
        fleet_->communication_range = std::max(0.0, range);
        return *this;
    }
    
    IFleetBuilder& setLogisticsEfficiency(double efficiency) override {
        fleet_->logistics_efficiency = std::max(0.1, std::min(2.0, efficiency));
        return *this;
    }
    
    std::unique_ptr<FleetSpec> build() override {
        if (!canBuild()) {
            return nullptr;
        }
        
        fleet_->formation_date = std::chrono::system_clock::now();
        
        auto result = std::move(fleet_);
        reset();
        return result;
    }
    
    void reset() override {
        fleet_ = std::make_unique<FleetSpec>();
    }
    
    bool canBuild() const override {
        return fleet_ &&
               !fleet_->fleet_name.empty() &&
               !fleet_->spacecraft.empty();
    }
    
    std::vector<std::string> getValidationErrors() const override {
        std::vector<std::string> errors;
        
        if (!fleet_) {
            errors.push_back("No fleet being built");
            return errors;
        }
        
        if (fleet_->fleet_name.empty()) {
            errors.push_back("Fleet name is required");
        }
        
        if (fleet_->spacecraft.empty()) {
            errors.push_back("Fleet must contain at least one spacecraft");
        }
        
        return errors;
    }

private:
    std::unique_ptr<FleetSpec> fleet_;
};

/**
 * @brief Construction Director - Orchestrates complex builds
 * 
 * Provides high-level construction methods using builders to create
 * common spacecraft and fleet configurations.
 */
class ConstructionDirector {
public:
    /**
     * @brief Build a standard fighter spacecraft
     */
    static std::unique_ptr<SpacecraftSpec> buildFighter(ISpacecraftBuilder& builder, const std::string& name) {
        builder.reset();
        
        // Basic fighter configuration
        builder.setName(name)
               .setClass("Fighter")
               .setHull("Light Fighter Hull", 50.0, 25.0)
               .setPowerSystem(100.0, 1.0)
               .setFuelCapacity(200.0)
               .setCargoCapacity(5.0)
               .setCrewCapacity(1.0);
        
        // Fighter engine
        ComponentSpec engine("Engine", "Interceptor Drive", 15.0, 40.0, 0.9);
        engine.setAttribute("thrust", 120.0);
        engine.setAttribute("acceleration_bonus", 2.0);
        builder.addEngine(engine);
        
        // Fighter weapons
        ComponentSpec laser("Weapon", "Pulse Laser", 8.0, 25.0, 0.95);
        laser.setAttribute("damage", 15.0);
        laser.setAttribute("range", 500.0);
        builder.addWeapon(laser);
        
        ComponentSpec missile("Weapon", "Light Missiles", 12.0, 5.0, 0.85);
        missile.setAttribute("damage", 30.0);
        missile.setAttribute("range", 1000.0);
        builder.addWeapon(missile);
        
        // Basic sensors and life support
        ComponentSpec sensor("Sensor", "Combat Radar", 5.0, 15.0, 0.9);
        sensor.setAttribute("range", 2000.0);
        builder.addSensor(sensor);
        
        ComponentSpec life_support("Life Support", "Basic Life Support", 10.0, 10.0, 0.95);
        builder.addLifeSupport(life_support);
        
        builder.setShields(50.0, 2.0)
               .setDefenses(25.0, 1.5)
               .setPerformance(800.0, 5.0, 8.0);
        
        return builder.build();
    }
    
    /**
     * @brief Build a standard cruiser spacecraft
     */
    static std::unique_ptr<SpacecraftSpec> buildCruiser(ISpacecraftBuilder& builder, const std::string& name) {
        builder.reset();
        
        builder.setName(name)
               .setClass("Cruiser")
               .setHull("Medium Cruiser Hull", 500.0, 150.0)
               .setPowerSystem(800.0, 1.2)
               .setFuelCapacity(1500.0)
               .setCargoCapacity(200.0)
               .setCrewCapacity(50.0);
        
        // Twin main engines
        ComponentSpec main_engine("Engine", "Cruiser Drive Alpha", 80.0, 200.0, 0.92);
        main_engine.setAttribute("thrust", 400.0);
        main_engine.setAttribute("acceleration_bonus", 1.0);
        builder.addEngine(main_engine);
        
        ComponentSpec aux_engine("Engine", "Cruiser Drive Beta", 80.0, 200.0, 0.92);
        aux_engine.setAttribute("thrust", 400.0);
        aux_engine.setAttribute("acceleration_bonus", 1.0);
        builder.addEngine(aux_engine);
        
        // Cruiser weapons array
        ComponentSpec heavy_laser("Weapon", "Heavy Laser Cannon", 35.0, 100.0, 0.9);
        heavy_laser.setAttribute("damage", 50.0);
        heavy_laser.setAttribute("range", 1500.0);
        builder.addWeapon(heavy_laser);
        
        ComponentSpec torpedo("Weapon", "Plasma Torpedoes", 40.0, 80.0, 0.85);
        torpedo.setAttribute("damage", 80.0);
        torpedo.setAttribute("range", 2000.0);
        builder.addWeapon(torpedo);
        
        ComponentSpec point_defense("Weapon", "Point Defense Array", 20.0, 40.0, 0.95);
        point_defense.setAttribute("damage", 10.0);
        point_defense.setAttribute("range", 300.0);
        builder.addWeapon(point_defense);
        
        // Advanced sensors
        ComponentSpec long_range_sensor("Sensor", "Deep Space Scanner", 25.0, 60.0, 0.9);
        long_range_sensor.setAttribute("range", 10000.0);
        builder.addSensor(long_range_sensor);
        
        ComponentSpec tactical_sensor("Sensor", "Tactical Array", 15.0, 40.0, 0.92);
        tactical_sensor.setAttribute("range", 3000.0);
        builder.addSensor(tactical_sensor);
        
        // Life support for crew
        ComponentSpec advanced_life_support("Life Support", "Advanced Life Support", 50.0, 80.0, 0.98);
        builder.addLifeSupport(advanced_life_support);
        
        builder.setShields(200.0, 1.5)
               .setDefenses(150.0, 0.8)
               .setPerformance(400.0, 2.0, 4.0);
        
        return builder.build();
    }
    
    /**
     * @brief Build a standard destroyer squadron fleet
     */
    static std::unique_ptr<FleetSpec> buildDestroyerSquadron(IFleetBuilder& fleet_builder, 
                                                           ISpacecraftBuilder& ship_builder,
                                                           const std::string& fleet_name) {
        fleet_builder.reset();
        
        fleet_builder.setFleetName(fleet_name)
                     .setDesignation("DD-Squadron")
                     .setCommander("Commander-" + fleet_name)
                     .setHomeBase("Fleet Command")
                     .setFormationCohesion(1.2)
                     .setCommunicationRange(5000.0)
                     .setLogisticsEfficiency(1.1);
        
        // Add mission capabilities
        fleet_builder.addMissionCapability("Patrol")
                     .addMissionCapability("Escort")
                     .addMissionCapability("Anti-Fighter")
                     .addMissionCapability("Reconnaissance");
        
        // Build destroyer specifications (simplified destroyer = enhanced fighter)
        for (int i = 1; i <= 4; ++i) {
            ship_builder.reset();
            
            std::string ship_name = fleet_name + "-DD-" + std::to_string(i);
            
            ship_builder.setName(ship_name)
                       .setClass("Destroyer")
                       .setHull("Destroyer Hull", 200.0, 80.0)
                       .setPowerSystem(400.0, 1.1)
                       .setFuelCapacity(800.0)
                       .setCargoCapacity(50.0)
                       .setCrewCapacity(15.0);
            
            // Destroyer propulsion
            ComponentSpec destroyer_engine("Engine", "Destroyer Drive", 40.0, 120.0, 0.91);
            destroyer_engine.setAttribute("thrust", 250.0);
            destroyer_engine.setAttribute("acceleration_bonus", 1.5);
            ship_builder.addEngine(destroyer_engine);
            
            // Destroyer armament
            ComponentSpec main_gun("Weapon", "Destroyer Cannon", 25.0, 80.0, 0.9);
            main_gun.setAttribute("damage", 40.0);
            main_gun.setAttribute("range", 1200.0);
            ship_builder.addWeapon(main_gun);
            
            ComponentSpec missiles("Weapon", "Multi-Role Missiles", 18.0, 15.0, 0.88);
            missiles.setAttribute("damage", 25.0);
            missiles.setAttribute("range", 1500.0);
            ship_builder.addWeapon(missiles);
            
            // Sensors and life support
            ComponentSpec destroyer_sensors("Sensor", "Naval Radar", 12.0, 30.0, 0.9);
            destroyer_sensors.setAttribute("range", 4000.0);
            ship_builder.addSensor(destroyer_sensors);
            
            ComponentSpec destroyer_life_support("Life Support", "Naval Life Support", 20.0, 25.0, 0.96);
            ship_builder.addLifeSupport(destroyer_life_support);
            
            ship_builder.setShields(100.0, 1.8)
                       .setDefenses(80.0, 1.2)
                       .setPerformance(600.0, 3.5, 6.0);
            
            auto destroyer = ship_builder.build();
            if (destroyer) {
                fleet_builder.addSpacecraft(std::move(destroyer));
            }
        }
        
        return fleet_builder.build();
    }
    
    /**
     * @brief Build a balanced task force fleet
     */
    static std::unique_ptr<FleetSpec> buildTaskForce(IFleetBuilder& fleet_builder,
                                                   ISpacecraftBuilder& ship_builder,
                                                   const std::string& fleet_name) {
        fleet_builder.reset();
        
        fleet_builder.setFleetName(fleet_name)
                     .setDesignation("Task Force")
                     .setCommander("Admiral-" + fleet_name)
                     .setHomeBase("Naval Station")
                     .setFormationCohesion(1.0)
                     .setCommunicationRange(8000.0)
                     .setLogisticsEfficiency(1.0);
        
        // Add comprehensive mission capabilities
        fleet_builder.addMissionCapability("Deep Strike")
                     .addMissionCapability("System Defense")
                     .addMissionCapability("Fleet Engagement")
                     .addMissionCapability("Planetary Assault")
                     .addMissionCapability("Long Range Patrol");
        
        // Add 1 cruiser (flagship)
        auto cruiser = buildCruiser(ship_builder, fleet_name + "-CL-01");
        if (cruiser) {
            fleet_builder.addSpacecraft(std::move(cruiser));
        }
        
        // Add 2 destroyers
        for (int i = 1; i <= 2; ++i) {
            // Build a destroyer (similar to destroyer squadron but individual ships)
            ship_builder.reset();
            
            std::string ship_name = fleet_name + "-DD-" + std::to_string(i);
            
            ship_builder.setName(ship_name)
                       .setClass("Destroyer")
                       .setHull("Destroyer Hull", 200.0, 80.0)
                       .setPowerSystem(400.0, 1.1)
                       .setFuelCapacity(800.0)
                       .setCargoCapacity(50.0)
                       .setCrewCapacity(15.0);
            
            ComponentSpec destroyer_engine("Engine", "Destroyer Drive", 40.0, 120.0, 0.91);
            destroyer_engine.setAttribute("thrust", 250.0);
            destroyer_engine.setAttribute("acceleration_bonus", 1.5);
            ship_builder.addEngine(destroyer_engine);
            
            ComponentSpec main_gun("Weapon", "Destroyer Cannon", 25.0, 80.0, 0.9);
            main_gun.setAttribute("damage", 40.0);
            main_gun.setAttribute("range", 1200.0);
            ship_builder.addWeapon(main_gun);
            
            ComponentSpec destroyer_sensors("Sensor", "Naval Radar", 12.0, 30.0, 0.9);
            destroyer_sensors.setAttribute("range", 4000.0);
            ship_builder.addSensor(destroyer_sensors);
            
            ComponentSpec destroyer_life_support("Life Support", "Naval Life Support", 20.0, 25.0, 0.96);
            ship_builder.addLifeSupport(destroyer_life_support);
            
            ship_builder.setShields(100.0, 1.8)
                       .setDefenses(80.0, 1.2)
                       .setPerformance(600.0, 3.5, 6.0);
            
            auto destroyer = ship_builder.build();
            if (destroyer) {
                fleet_builder.addSpacecraft(std::move(destroyer));
            }
        }
        
        // Add 4 fighters
        for (int i = 1; i <= 4; ++i) {
            auto fighter = buildFighter(ship_builder, fleet_name + "-FTR-" + std::to_string(i));
            if (fighter) {
                fleet_builder.addSpacecraft(std::move(fighter));
            }
        }
        
        return fleet_builder.build();
    }
};

/**
 * @brief Specialized builders for different spacecraft classes
 */
class FighterBuilder : public SpacecraftBuilder {
public:
    FighterBuilder() {
        reset();
        // Pre-configure for fighter role
        setClass("Fighter")
            .setHull("Fighter Hull", 50.0, 25.0)
            .setPowerSystem(100.0)
            .setFuelCapacity(200.0)
            .setCargoCapacity(5.0)
            .setCrewCapacity(1.0);
    }
    
    /**
     * @brief Configure as interceptor variant
     */
    FighterBuilder& configureAsInterceptor() {
        ComponentSpec interceptor_engine("Engine", "High-Speed Drive", 12.0, 35.0, 0.9);
        interceptor_engine.setAttribute("thrust", 150.0);
        interceptor_engine.setAttribute("acceleration_bonus", 3.0);
        addEngine(interceptor_engine);
        
        ComponentSpec rapid_laser("Weapon", "Rapid Pulse Laser", 6.0, 20.0, 0.95);
        rapid_laser.setAttribute("damage", 12.0);
        rapid_laser.setAttribute("range", 600.0);
        addWeapon(rapid_laser);
        
        setPerformance(900.0, 6.0, 9.0);
        return *this;
    }
    
    /**
     * @brief Configure as bomber variant
     */
    FighterBuilder& configureAsBomber() {
        ComponentSpec bomber_engine("Engine", "Heavy Fighter Drive", 18.0, 45.0, 0.88);
        bomber_engine.setAttribute("thrust", 100.0);
        bomber_engine.setAttribute("acceleration_bonus", 1.0);
        addEngine(bomber_engine);
        
        ComponentSpec heavy_missiles("Weapon", "Heavy Missiles", 20.0, 8.0, 0.82);
        heavy_missiles.setAttribute("damage", 60.0);
        heavy_missiles.setAttribute("range", 1200.0);
        addWeapon(heavy_missiles);
        
        ComponentSpec bomb_bay("Special", "Bomb Bay", 15.0, 0.0, 0.9);
        bomb_bay.setAttribute("capacity", 500.0);
        addSpecialEquipment(bomb_bay);
        
        setPerformance(600.0, 3.0, 5.0);
        return *this;
    }
};

class CruiserBuilder : public SpacecraftBuilder {
public:
    CruiserBuilder() {
        reset();
        // Pre-configure for cruiser role
        setClass("Cruiser")
            .setHull("Cruiser Hull", 500.0, 150.0)
            .setPowerSystem(800.0)
            .setFuelCapacity(1500.0)
            .setCargoCapacity(200.0)
            .setCrewCapacity(50.0);
    }
    
    /**
     * @brief Configure as heavy cruiser
     */
    CruiserBuilder& configureAsHeavyCruiser() {
        setHull("Heavy Cruiser Hull", 750.0, 200.0)
            .setPowerSystem(1200.0);
        
        ComponentSpec heavy_engine("Engine", "Heavy Cruiser Drive", 120.0, 300.0, 0.9);
        heavy_engine.setAttribute("thrust", 500.0);
        heavy_engine.setAttribute("acceleration_bonus", 0.8);
        addEngine(heavy_engine);
        
        ComponentSpec heavy_cannon("Weapon", "Heavy Plasma Cannon", 60.0, 150.0, 0.88);
        heavy_cannon.setAttribute("damage", 100.0);
        heavy_cannon.setAttribute("range", 2000.0);
        addWeapon(heavy_cannon);
        
        setShields(300.0, 1.2)
            .setDefenses(200.0, 0.6)
            .setPerformance(350.0, 1.5, 3.0);
        
        return *this;
    }
    
    /**
     * @brief Configure as light cruiser
     */
    CruiserBuilder& configureAsLightCruiser() {
        setHull("Light Cruiser Hull", 350.0, 100.0)
            .setPowerSystem(600.0);
        
        ComponentSpec light_engine("Engine", "Light Cruiser Drive", 60.0, 150.0, 0.92);
        light_engine.setAttribute("thrust", 300.0);
        light_engine.setAttribute("acceleration_bonus", 1.5);
        addEngine(light_engine);
        
        ComponentSpec medium_laser("Weapon", "Medium Laser Array", 25.0, 70.0, 0.92);
        medium_laser.setAttribute("damage", 35.0);
        medium_laser.setAttribute("range", 1200.0);
        addWeapon(medium_laser);
        
        setShields(150.0, 2.0)
            .setDefenses(100.0, 1.0)
            .setPerformance(500.0, 2.5, 5.0);
        
        return *this;
    }
};

/**
 * @brief Component Factory - Creates standardized components
 */
class ComponentFactory {
public:
    // Engine components
    static ComponentSpec createFighterEngine(const std::string& name = "Fighter Engine") {
        ComponentSpec engine("Engine", name, 15.0, 40.0, 0.9);
        engine.setAttribute("thrust", 120.0);
        engine.setAttribute("acceleration_bonus", 2.0);
        return engine;
    }
    
    static ComponentSpec createCruiserEngine(const std::string& name = "Cruiser Engine") {
        ComponentSpec engine("Engine", name, 80.0, 200.0, 0.92);
        engine.setAttribute("thrust", 400.0);
        engine.setAttribute("acceleration_bonus", 1.0);
        return engine;
    }
    
    static ComponentSpec createCapitalEngine(const std::string& name = "Capital Engine") {
        ComponentSpec engine("Engine", name, 200.0, 500.0, 0.88);
        engine.setAttribute("thrust", 800.0);
        engine.setAttribute("acceleration_bonus", 0.5);
        return engine;
    }
    
    // Weapon components
    static ComponentSpec createLightLaser(const std::string& name = "Light Laser") {
        ComponentSpec weapon("Weapon", name, 8.0, 25.0, 0.95);
        weapon.setAttribute("damage", 15.0);
        weapon.setAttribute("range", 500.0);
        weapon.setAttribute("fire_rate", 3.0);
        return weapon;
    }
    
    static ComponentSpec createHeavyLaser(const std::string& name = "Heavy Laser") {
        ComponentSpec weapon("Weapon", name, 35.0, 100.0, 0.9);
        weapon.setAttribute("damage", 50.0);
        weapon.setAttribute("range", 1500.0);
        weapon.setAttribute("fire_rate", 1.0);
        return weapon;
    }
    
    static ComponentSpec createMissileLauncher(const std::string& name = "Missile Launcher", 
                                              double damage = 30.0, double range = 1000.0) {
        ComponentSpec weapon("Weapon", name, 15.0, 10.0, 0.85);
        weapon.setAttribute("damage", damage);
        weapon.setAttribute("range", range);
        weapon.setAttribute("fire_rate", 0.5);
        weapon.setAttribute("ammo_limited", 1.0);
        return weapon;
    }
    
    // Sensor components
    static ComponentSpec createBasicSensors(const std::string& name = "Basic Sensors") {
        ComponentSpec sensor("Sensor", name, 5.0, 15.0, 0.9);
        sensor.setAttribute("range", 2000.0);
        sensor.setAttribute("resolution", 1.0);
        return sensor;
    }
    
    static ComponentSpec createAdvancedSensors(const std::string& name = "Advanced Sensors") {
        ComponentSpec sensor("Sensor", name, 25.0, 60.0, 0.95);
        sensor.setAttribute("range", 10000.0);
        sensor.setAttribute("resolution", 3.0);
        sensor.setAttribute("stealth_detection", 1.0);
        return sensor;
    }
    
    // Life support components
    static ComponentSpec createBasicLifeSupport(const std::string& name = "Basic Life Support") {
        ComponentSpec ls("Life Support", name, 10.0, 10.0, 0.95);
        ls.setAttribute("crew_capacity", 5.0);
        ls.setAttribute("duration", 168.0); // Hours
        return ls;
    }
    
    static ComponentSpec createAdvancedLifeSupport(const std::string& name = "Advanced Life Support") {
        ComponentSpec ls("Life Support", name, 50.0, 80.0, 0.98);
        ls.setAttribute("crew_capacity", 100.0);
        ls.setAttribute("duration", 720.0); // Hours
        ls.setAttribute("recycling_efficiency", 0.95);
        return ls;
    }
    
    // Special equipment
    static ComponentSpec createCloakingDevice(const std::string& name = "Cloaking Device") {
        ComponentSpec cloak("Special", name, 40.0, 200.0, 0.75);
        cloak.setAttribute("stealth_rating", 8.0);
        cloak.setAttribute("power_surge", 1.0); // Causes power spikes
        return cloak;
    }
    
    static ComponentSpec createShieldGenerator(const std::string& name = "Shield Generator", 
                                              double strength = 100.0) {
        ComponentSpec shield("Special", name, 30.0, 80.0, 0.9);
        shield.setAttribute("shield_strength", strength);
        shield.setAttribute("recharge_rate", strength * 0.1);
        return shield;
    }
    
    static ComponentSpec createJumpDrive(const std::string& name = "Jump Drive") {
        ComponentSpec jump("Special", name, 100.0, 300.0, 0.8);
        jump.setAttribute("jump_range", 50.0); // Light years
        jump.setAttribute("charge_time", 600.0); // Seconds
        jump.setAttribute("accuracy", 0.9);
        return jump;
    }
};

/**
 * @brief Build Configuration - Serializable build instructions
 */
struct BuildConfiguration {
    std::string template_name;
    std::unordered_map<std::string, std::string> parameters;
    std::vector<std::string> component_overrides;
    
    BuildConfiguration(const std::string& template_name = "") 
        : template_name(template_name) {}
    
    void setParameter(const std::string& key, const std::string& value) {
        parameters[key] = value;
    }
    
    std::string getParameter(const std::string& key, const std::string& default_value = "") const {
        auto it = parameters.find(key);
        return (it != parameters.end()) ? it->second : default_value;
    }
    
    void addComponentOverride(const std::string& component_config) {
        component_overrides.push_back(component_config);
    }
};

/**
 * @brief Template Builder - Builds from predefined templates
 */
class TemplateBuilder {
public:
    /**
     * @brief Register a build template
     */
    static void registerTemplate(const std::string& name, 
                                std::function<std::unique_ptr<SpacecraftSpec>(ISpacecraftBuilder&, const BuildConfiguration&)> builder_func) {
        templates_[name] = builder_func;
    }
    
    /**
     * @brief Build spacecraft from template
     */
    static std::unique_ptr<SpacecraftSpec> buildFromTemplate(const std::string& template_name,
                                                           ISpacecraftBuilder& builder,
                                                           const BuildConfiguration& config) {
        auto it = templates_.find(template_name);
        if (it != templates_.end()) {
            return it->second(builder, config);
        }
        return nullptr;
    }
    
    /**
     * @brief Get available template names
     */
    static std::vector<std::string> getAvailableTemplates() {
        std::vector<std::string> names;
        names.reserve(templates_.size());
        for (const auto& [name, func] : templates_) {
            names.push_back(name);
        }
        return names;
    }
    
    /**
     * @brief Initialize default templates
     */
    static void initializeDefaultTemplates() {
        // Standard Fighter Template
        registerTemplate("standard_fighter", [](ISpacecraftBuilder& builder, const BuildConfiguration& config) {
            std::string name = config.getParameter("name", "Fighter");
            return ConstructionDirector::buildFighter(builder, name);
        });
        
        // Standard Cruiser Template
        registerTemplate("standard_cruiser", [](ISpacecraftBuilder& builder, const BuildConfiguration& config) {
            std::string name = config.getParameter("name", "Cruiser");
            return ConstructionDirector::buildCruiser(builder, name);
        });
        
        // Custom Fighter Template with parameters
        registerTemplate("custom_fighter", [](ISpacecraftBuilder& builder, const BuildConfiguration& config) {
            builder.reset();
            
            std::string name = config.getParameter("name", "Custom Fighter");
            std::string variant = config.getParameter("variant", "standard");
            
            builder.setName(name)
                   .setClass("Fighter")
                   .setHull("Fighter Hull", 50.0, 25.0)
                   .setPowerSystem(100.0)
                   .setFuelCapacity(200.0)
                   .setCrewCapacity(1.0);
            
            if (variant == "interceptor") {
                builder.addEngine(ComponentFactory::createFighterEngine("High-Speed Engine"));
                builder.addWeapon(ComponentFactory::createLightLaser("Rapid Laser"));
                builder.setPerformance(900.0, 6.0, 9.0);
            } else if (variant == "bomber") {
                builder.addEngine(ComponentFactory::createFighterEngine("Bomber Engine"));
                builder.addWeapon(ComponentFactory::createMissileLauncher("Heavy Missiles", 60.0, 1200.0));
                builder.setPerformance(600.0, 3.0, 5.0);
            } else {
                // Standard configuration
                builder.addEngine(ComponentFactory::createFighterEngine());
                builder.addWeapon(ComponentFactory::createLightLaser());
                builder.setPerformance(800.0, 5.0, 8.0);
            }
            
            builder.addSensor(ComponentFactory::createBasicSensors())
                   .addLifeSupport(ComponentFactory::createBasicLifeSupport())
                   .setShields(50.0, 2.0)
                   .setDefenses(25.0, 1.5);
            
            return builder.build();
        });
    }

private:
    static std::unordered_map<std::string, std::function<std::unique_ptr<SpacecraftSpec>(ISpacecraftBuilder&, const BuildConfiguration&)>> templates_;
};

// Static member definition
std::unordered_map<std::string, std::function<std::unique_ptr<SpacecraftSpec>(ISpacecraftBuilder&, const BuildConfiguration&)>> 
    TemplateBuilder::templates_;

/**
 * @brief Build Validator - Validates build configurations
 */
class BuildValidator {
public:
    struct ValidationResult {
        bool is_valid = true;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
        
        void addError(const std::string& error) {
            errors.push_back(error);
            is_valid = false;
        }
        
        void addWarning(const std::string& warning) {
            warnings.push_back(warning);
        }
    };
    
    /**
     * @brief Validate spacecraft specification
     */
    static ValidationResult validateSpacecraft(const SpacecraftSpec& spec) {
        ValidationResult result;
        
        // Basic validation
        if (spec.name.empty()) {
            result.addError("Spacecraft name cannot be empty");
        }
        
        if (spec.class_type.empty()) {
            result.addError("Spacecraft class cannot be empty");
        }
        
        if (spec.engines.empty()) {
            result.addError("Spacecraft must have at least one engine");
        }
        
        // Power balance validation
        double power_needed = spec.calculatePowerConsumption();
        if (power_needed > spec.power_capacity) {
            result.addError("Insufficient power capacity: need " + 
                          std::to_string(power_needed) + ", have " + 
                          std::to_string(spec.power_capacity));
        }
        
        // Mass validation
        double total_mass = spec.calculateTotalMass();
        if (total_mass <= 0.0) {
            result.addError("Total spacecraft mass must be positive");
        }
        
        // Performance warnings
        if (spec.max_speed <= 0.0) {
            result.addWarning("Maximum speed not set - may indicate missing engine configuration");
        }
        
        if (spec.shield_strength <= 0.0 && spec.armor_rating <= 0.0) {
            result.addWarning("No defensive systems installed - spacecraft is vulnerable");
        }
        
        if (spec.weapons.empty()) {
            result.addWarning("No weapons installed - limited combat capability");
        }
        
        if (spec.sensors.empty()) {
            result.addWarning("No sensors installed - limited situational awareness");
        }
        
        // Reliability check
        double reliability = spec.getReliabilityScore();
        if (reliability < 0.8) {
            result.addWarning("Low overall reliability score: " + std::to_string(reliability));
        }
        
        return result;
    }
    
    /**
     * @brief Validate fleet specification
     */
    static ValidationResult validateFleet(const FleetSpec& spec) {
        ValidationResult result;
        
        if (spec.fleet_name.empty()) {
            result.addError("Fleet name cannot be empty");
        }
        
        if (spec.spacecraft.empty()) {
            result.addError("Fleet must contain at least one spacecraft");
        }
        
        // Validate each spacecraft in the fleet
        for (const auto& craft : spec.spacecraft) {
            ValidationResult craft_result = validateSpacecraft(craft);
            if (!craft_result.is_valid) {
                result.addError("Invalid spacecraft '" + craft.name + "' in fleet");
                for (const auto& error : craft_result.errors) {
                    result.addError("  " + craft.name + ": " + error);
                }
            }
        }
        
        // Fleet composition warnings
        if (spec.spacecraft.size() == 1) {
            result.addWarning("Single-ship fleet - limited operational flexibility");
        }
        
        if (spec.mission_capabilities.empty()) {
            result.addWarning("No mission capabilities defined");
        }
        
        return result;
    }
};

} // namespace CppVerseHub::Patterns