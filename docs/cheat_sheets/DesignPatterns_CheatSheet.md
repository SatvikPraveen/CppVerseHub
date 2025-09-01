# Design Patterns Cheat Sheet

**Location:** `CppVerseHub/docs/cheat_sheets/DesignPatterns_CheatSheet.md`

## 📚 Gang of Four Design Patterns in Modern C++

### 🏗️ Creational Patterns

#### **Singleton Pattern**

Ensures a class has only one instance and provides global access to it.

```cpp
// Thread-safe Modern C++ Singleton
class ResourceManager {
private:
    ResourceManager() = default;
    double total_resources_ = 10000.0;

public:
    // Meyer's Singleton (thread-safe in C++11+)
    static ResourceManager& getInstance() {
        static ResourceManager instance;
        return instance;
    }

    // Delete copy and move operations
    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;
    ResourceManager(ResourceManager&&) = delete;
    ResourceManager& operator=(ResourceManager&&) = delete;

    void allocateResources(double amount) {
        if (total_resources_ >= amount) {
            total_resources_ -= amount;
            std::cout << "Allocated " << amount << " resources\n";
        }
    }

    double getAvailableResources() const {
        return total_resources_;
    }
};

// Alternative: call_once implementation
class SafeSingleton {
private:
    SafeSingleton() = default;
    static std::unique_ptr<SafeSingleton> instance_;
    static std::once_flag initialized_;

public:
    static SafeSingleton& getInstance() {
        std::call_once(initialized_, []() {
            instance_ = std::make_unique<SafeSingleton>();
        });
        return *instance_;
    }
};

std::unique_ptr<SafeSingleton> SafeSingleton::instance_ = nullptr;
std::once_flag SafeSingleton::initialized_;

// Usage
void singleton_usage() {
    auto& rm = ResourceManager::getInstance();
    rm.allocateResources(500.0);

    // Same instance everywhere
    auto& rm2 = ResourceManager::getInstance();
    std::cout << "Available: " << rm2.getAvailableResources() << "\n";
}
```

#### **Factory Pattern**

Creates objects without specifying their exact classes.

```cpp
// Product hierarchy
class Mission {
public:
    virtual ~Mission() = default;
    virtual void execute() = 0;
    virtual std::string getType() const = 0;
    virtual double getCost() const = 0;
};

class ExplorationMission : public Mission {
public:
    void execute() override {
        std::cout << "Scanning unknown sectors...\n";
    }

    std::string getType() const override {
        return "Exploration";
    }

    double getCost() const override {
        return 1000.0;
    }
};

class CombatMission : public Mission {
public:
    void execute() override {
        std::cout << "Engaging hostile forces...\n";
    }

    std::string getType() const override {
        return "Combat";
    }

    double getCost() const override {
        return 2500.0;
    }
};

class ColonizationMission : public Mission {
public:
    void execute() override {
        std::cout << "Establishing colony...\n";
    }

    std::string getType() const override {
        return "Colonization";
    }

    double getCost() const override {
        return 5000.0;
    }
};

// Factory
enum class MissionType {
    Exploration,
    Combat,
    Colonization
};

class MissionFactory {
public:
    static std::unique_ptr<Mission> createMission(MissionType type) {
        switch (type) {
            case MissionType::Exploration:
                return std::make_unique<ExplorationMission>();
            case MissionType::Combat:
                return std::make_unique<CombatMission>();
            case MissionType::Colonization:
                return std::make_unique<ColonizationMission>();
            default:
                throw std::invalid_argument("Unknown mission type");
        }
    }

    // Factory with parameters
    static std::unique_ptr<Mission> createMission(const std::string& type_name) {
        if (type_name == "exploration") {
            return createMission(MissionType::Exploration);
        } else if (type_name == "combat") {
            return createMission(MissionType::Combat);
        } else if (type_name == "colonization") {
            return createMission(MissionType::Colonization);
        }
        throw std::invalid_argument("Unknown mission type: " + type_name);
    }
};

// Modern C++: Factory with registry
class AdvancedMissionFactory {
private:
    using Creator = std::function<std::unique_ptr<Mission>()>;
    std::unordered_map<std::string, Creator> creators_;

public:
    template<typename T>
    void registerMission(const std::string& type) {
        creators_[type] = []() { return std::make_unique<T>(); };
    }

    std::unique_ptr<Mission> create(const std::string& type) {
        auto it = creators_.find(type);
        if (it != creators_.end()) {
            return it->second();
        }
        throw std::invalid_argument("Unknown mission type: " + type);
    }

    std::vector<std::string> getAvailableTypes() const {
        std::vector<std::string> types;
        for (const auto& [type, creator] : creators_) {
            types.push_back(type);
        }
        return types;
    }
};

// Usage
void factory_usage() {
    // Simple factory
    auto exploration = MissionFactory::createMission(MissionType::Exploration);
    auto combat = MissionFactory::createMission("combat");

    exploration->execute();
    combat->execute();

    // Advanced factory with registry
    AdvancedMissionFactory factory;
    factory.registerMission<ExplorationMission>("exploration");
    factory.registerMission<CombatMission>("combat");
    factory.registerMission<ColonizationMission>("colonization");

    auto mission = factory.create("exploration");
    mission->execute();
}
```

#### **Builder Pattern**

Constructs complex objects step by step.

```cpp
// Complex object to build
class Spaceship {
private:
    std::string name_;
    std::string hull_type_;
    std::vector<std::string> weapons_;
    std::vector<std::string> modules_;
    double shield_strength_ = 0.0;
    double engine_power_ = 0.0;
    int crew_capacity_ = 0;

public:
    // Setters for builder
    void setName(const std::string& name) { name_ = name; }
    void setHullType(const std::string& hull_type) { hull_type_ = hull_type; }
    void addWeapon(const std::string& weapon) { weapons_.push_back(weapon); }
    void addModule(const std::string& module) { modules_.push_back(module); }
    void setShieldStrength(double strength) { shield_strength_ = strength; }
    void setEnginePower(double power) { engine_power_ = power; }
    void setCrewCapacity(int capacity) { crew_capacity_ = capacity; }

    void display() const {
        std::cout << "Spaceship: " << name_ << "\n";
        std::cout << "Hull: " << hull_type_ << "\n";
        std::cout << "Shield: " << shield_strength_ << "\n";
        std::cout << "Engine: " << engine_power_ << "\n";
        std::cout << "Crew: " << crew_capacity_ << "\n";

        std::cout << "Weapons: ";
        for (const auto& weapon : weapons_) {
            std::cout << weapon << " ";
        }
        std::cout << "\n";

        std::cout << "Modules: ";
        for (const auto& module : modules_) {
            std::cout << module << " ";
        }
        std::cout << "\n\n";
    }
};

// Builder interface
class SpaceshipBuilder {
protected:
    std::unique_ptr<Spaceship> spaceship_;

public:
    SpaceshipBuilder() : spaceship_(std::make_unique<Spaceship>()) {}
    virtual ~SpaceshipBuilder() = default;

    virtual SpaceshipBuilder& setName(const std::string& name) {
        spaceship_->setName(name);
        return *this;
    }

    virtual SpaceshipBuilder& buildHull() = 0;
    virtual SpaceshipBuilder& buildWeapons() = 0;
    virtual SpaceshipBuilder& buildModules() = 0;
    virtual SpaceshipBuilder& buildShields() = 0;
    virtual SpaceshipBuilder& buildEngine() = 0;

    std::unique_ptr<Spaceship> getResult() {
        return std::move(spaceship_);
    }
};

// Concrete builders
class FighterBuilder : public SpaceshipBuilder {
public:
    SpaceshipBuilder& buildHull() override {
        spaceship_->setHullType("Light Fighter Hull");
        spaceship_->setCrewCapacity(1);
        return *this;
    }

    SpaceshipBuilder& buildWeapons() override {
        spaceship_->addWeapon("Laser Cannon");
        spaceship_->addWeapon("Missile Launcher");
        return *this;
    }

    SpaceshipBuilder& buildModules() override {
        spaceship_->addModule("Navigation Computer");
        spaceship_->addModule("Life Support");
        return *this;
    }

    SpaceshipBuilder& buildShields() override {
        spaceship_->setShieldStrength(500.0);
        return *this;
    }

    SpaceshipBuilder& buildEngine() override {
        spaceship_->setEnginePower(800.0);
        return *this;
    }
};

class CruiserBuilder : public SpaceshipBuilder {
public:
    SpaceshipBuilder& buildHull() override {
        spaceship_->setHullType("Heavy Cruiser Hull");
        spaceship_->setCrewCapacity(50);
        return *this;
    }

    SpaceshipBuilder& buildWeapons() override {
        spaceship_->addWeapon("Plasma Cannon");
        spaceship_->addWeapon("Torpedo Launcher");
        spaceship_->addWeapon("Point Defense System");
        return *this;
    }

    SpaceshipBuilder& buildModules() override {
        spaceship_->addModule("Command Center");
        spaceship_->addModule("Medical Bay");
        spaceship_->addModule("Cargo Hold");
        spaceship_->addModule("Engineering");
        return *this;
    }

    SpaceshipBuilder& buildShields() override {
        spaceship_->setShieldStrength(2000.0);
        return *this;
    }

    SpaceshipBuilder& buildEngine() override {
        spaceship_->setEnginePower(1500.0);
        return *this;
    }
};

// Director
class SpaceshipDirector {
public:
    std::unique_ptr<Spaceship> buildStandardFighter(const std::string& name) {
        FighterBuilder builder;
        return builder
            .setName(name)
            .buildHull()
            .buildWeapons()
            .buildModules()
            .buildShields()
            .buildEngine()
            .getResult();
    }

    std::unique_ptr<Spaceship> buildStandardCruiser(const std::string& name) {
        CruiserBuilder builder;
        return builder
            .setName(name)
            .buildHull()
            .buildWeapons()
            .buildModules()
            .buildShields()
            .buildEngine()
            .getResult();
    }
};

// Modern C++: Fluent Builder with Method Chaining
class FluentSpaceshipBuilder {
private:
    Spaceship spaceship_;

public:
    FluentSpaceshipBuilder& name(const std::string& n) {
        spaceship_.setName(n);
        return *this;
    }

    FluentSpaceshipBuilder& hull(const std::string& type) {
        spaceship_.setHullType(type);
        return *this;
    }

    FluentSpaceshipBuilder& weapon(const std::string& w) {
        spaceship_.addWeapon(w);
        return *this;
    }

    FluentSpaceshipBuilder& module(const std::string& m) {
        spaceship_.addModule(m);
        return *this;
    }

    FluentSpaceshipBuilder& shields(double strength) {
        spaceship_.setShieldStrength(strength);
        return *this;
    }

    FluentSpaceshipBuilder& engine(double power) {
        spaceship_.setEnginePower(power);
        return *this;
    }

    FluentSpaceshipBuilder& crew(int capacity) {
        spaceship_.setCrewCapacity(capacity);
        return *this;
    }

    Spaceship build() {
        return std::move(spaceship_);
    }
};

// Usage
void builder_usage() {
    // Traditional builder with director
    SpaceshipDirector director;
    auto fighter = director.buildStandardFighter("Eagle");
    auto cruiser = director.buildStandardCruiser("Titan");

    fighter->display();
    cruiser->display();

    // Fluent builder
    auto custom_ship = FluentSpaceshipBuilder()
        .name("Custom Destroyer")
        .hull("Medium Hull")
        .weapon("Railgun")
        .weapon("Flak Cannon")
        .module("Sensor Array")
        .module("Communications")
        .shields(1200.0)
        .engine(1000.0)
        .crew(20)
        .build();

    custom_ship.display();
}
```

### 🏛️ Structural Patterns

#### **Adapter Pattern**

Allows incompatible interfaces to work together.

```cpp
// Legacy system interface
class LegacyNavigationSystem {
public:
    void set_coordinates(float x, float y) {
        std::cout << "Legacy: Setting coordinates to (" << x << ", " << y << ")\n";
    }

    void engage_autopilot() {
        std::cout << "Legacy: Autopilot engaged\n";
    }

    std::string get_status() {
        return "Legacy system operational";
    }
};

// Modern interface expected by new code
class ModernNavigationInterface {
public:
    virtual ~ModernNavigationInterface() = default;
    virtual void setDestination(const Point3D& destination) = 0;
    virtual void enableAutomaticNavigation() = 0;
    virtual NavigationStatus getSystemStatus() const = 0;
};

// Point3D and NavigationStatus for modern interface
struct Point3D {
    double x, y, z;
    Point3D(double x, double y, double z) : x(x), y(y), z(z) {}
};

enum class NavigationStatus {
    Online,
    Offline,
    Error
};

// Adapter class
class NavigationAdapter : public ModernNavigationInterface {
private:
    std::unique_ptr<LegacyNavigationSystem> legacy_system_;

public:
    NavigationAdapter(std::unique_ptr<LegacyNavigationSystem> legacy)
        : legacy_system_(std::move(legacy)) {}

    void setDestination(const Point3D& destination) override {
        // Adapt 3D coordinates to 2D
        legacy_system_->set_coordinates(
            static_cast<float>(destination.x),
            static_cast<float>(destination.y)
        );
    }

    void enableAutomaticNavigation() override {
        legacy_system_->engage_autopilot();
    }

    NavigationStatus getSystemStatus() const override {
        std::string status = legacy_system_->get_status();
        if (status.find("operational") != std::string::npos) {
            return NavigationStatus::Online;
        }
        return NavigationStatus::Error;
    }
};

// Modern client code
class Spaceship {
private:
    std::unique_ptr<ModernNavigationInterface> navigation_;

public:
    Spaceship(std::unique_ptr<ModernNavigationInterface> nav)
        : navigation_(std::move(nav)) {}

    void flyTo(const Point3D& destination) {
        navigation_->setDestination(destination);
        navigation_->enableAutomaticNavigation();

        auto status = navigation_->getSystemStatus();
        if (status == NavigationStatus::Online) {
            std::cout << "Flying to destination...\n";
        } else {
            std::cout << "Navigation system error!\n";
        }
    }
};

// Usage
void adapter_usage() {
    // Create legacy system
    auto legacy_nav = std::make_unique<LegacyNavigationSystem>();

    // Adapt it to modern interface
    auto adapted_nav = std::make_unique<NavigationAdapter>(std::move(legacy_nav));

    // Use with modern code
    Spaceship ship(std::move(adapted_nav));
    ship.flyTo(Point3D(100.0, 200.0, 300.0));
}
```

#### **Decorator Pattern**

Adds new functionality to objects dynamically.

```cpp
// Base component
class Mission {
public:
    virtual ~Mission() = default;
    virtual void execute() = 0;
    virtual double getCost() const = 0;
    virtual std::string getDescription() const = 0;
};

// Concrete component
class BasicMission : public Mission {
private:
    std::string type_;
    double base_cost_;

public:
    BasicMission(const std::string& type, double cost)
        : type_(type), base_cost_(cost) {}

    void execute() override {
        std::cout << "Executing " << type_ << " mission\n";
    }

    double getCost() const override {
        return base_cost_;
    }

    std::string getDescription() const override {
        return type_ + " mission";
    }
};

// Base decorator
class MissionDecorator : public Mission {
protected:
    std::unique_ptr<Mission> wrapped_mission_;

public:
    MissionDecorator(std::unique_ptr<Mission> mission)
        : wrapped_mission_(std::move(mission)) {}

    void execute() override {
        wrapped_mission_->execute();
    }

    double getCost() const override {
        return wrapped_mission_->getCost();
    }

    std::string getDescription() const override {
        return wrapped_mission_->getDescription();
    }
};

// Concrete decorators
class EncryptedCommunication : public MissionDecorator {
public:
    EncryptedCommunication(std::unique_ptr<Mission> mission)
        : MissionDecorator(std::move(mission)) {}

    void execute() override {
        std::cout << "Enabling encrypted communication...\n";
        MissionDecorator::execute();
        std::cout << "Communication secured\n";
    }

    double getCost() const override {
        return MissionDecorator::getCost() + 200.0;  // Additional cost
    }

    std::string getDescription() const override {
        return MissionDecorator::getDescription() + " with encrypted communication";
    }
};

class StealthMode : public MissionDecorator {
public:
    StealthMode(std::unique_ptr<Mission> mission)
        : MissionDecorator(std::move(mission)) {}

    void execute() override {
        std::cout << "Activating stealth mode...\n";
        MissionDecorator::execute();
        std::cout << "Stealth mode disabled\n";
    }

    double getCost() const override {
        return MissionDecorator::getCost() + 500.0;
    }

    std::string getDescription() const override {
        return MissionDecorator::getDescription() + " with stealth capabilities";
    }
};

class BackupSupport : public MissionDecorator {
public:
    BackupSupport(std::unique_ptr<Mission> mission)
        : MissionDecorator(std::move(mission)) {}

    void execute() override {
        std::cout << "Backup fleet on standby...\n";
        MissionDecorator::execute();
        std::cout << "Mission completed with backup support\n";
    }

    double getCost() const override {
        return MissionDecorator::getCost() + 800.0;
    }

    std::string getDescription() const override {
        return MissionDecorator::getDescription() + " with backup support";
    }
};

// Usage
void decorator_usage() {
    // Basic mission
    auto mission = std::make_unique<BasicMission>("Reconnaissance", 1000.0);

    std::cout << "Basic mission:\n";
    std::cout << "Description: " << mission->getDescription() << "\n";
    std::cout << "Cost: " << mission->getCost() << "\n";
    mission->execute();
    std::cout << "\n";

    // Decorated mission
    auto enhanced_mission = std::make_unique<BackupSupport>(
        std::make_unique<StealthMode>(
            std::make_unique<EncryptedCommunication>(
                std::make_unique<BasicMission>("Reconnaissance", 1000.0)
            )
        )
    );

    std::cout << "Enhanced mission:\n";
    std::cout << "Description: " << enhanced_mission->getDescription() << "\n";
    std::cout << "Cost: " << enhanced_mission->getCost() << "\n";
    enhanced_mission->execute();
}
```

### 🎯 Behavioral Patterns

#### **Observer Pattern**

Defines a one-to-many dependency between objects.

```cpp
#include <algorithm>

// Forward declaration
class Planet;

// Observer interface
class PlanetObserver {
public:
    virtual ~PlanetObserver() = default;
    virtual void onResourcesChanged(Planet* planet, double old_value, double new_value) = 0;
    virtual void onStatusChanged(Planet* planet, const std::string& old_status, const std::string& new_status) = 0;
};

// Subject (Observable)
class Planet {
private:
    std::string name_;
    double resources_;
    std::string status_;
    std::vector<PlanetObserver*> observers_;

public:
    Planet(const std::string& name, double resources)
        : name_(name), resources_(resources), status_("Unexplored") {}

    void addObserver(PlanetObserver* observer) {
        observers_.push_back(observer);
    }

    void removeObserver(PlanetObserver* observer) {
        observers_.erase(
            std::remove(observers_.begin(), observers_.end(), observer),
            observers_.end()
        );
    }

    void setResources(double new_resources) {
        double old_resources = resources_;
        resources_ = new_resources;
        notifyResourcesChanged(old_resources, new_resources);
    }

    void setStatus(const std::string& new_status) {
        std::string old_status = status_;
        status_ = new_status;
        notifyStatusChanged(old_status, new_status);
    }

    // Getters
    const std::string& getName() const { return name_; }
    double getResources() const { return resources_; }
    const std::string& getStatus() const { return status_; }

private:
    void notifyResourcesChanged(double old_value, double new_value) {
        for (auto* observer : observers_) {
            observer->onResourcesChanged(this, old_value, new_value);
        }
    }

    void notifyStatusChanged(const std::string& old_status, const std::string& new_status) {
        for (auto* observer : observers_) {
            observer->onStatusChanged(this, old_status, new_status);
        }
    }
};

// Concrete observers
class MissionControl : public PlanetObserver {
public:
    void onResourcesChanged(Planet* planet, double old_value, double new_value) override {
        std::cout << "[Mission Control] Planet " << planet->getName()
                  << " resources changed from " << old_value
                  << " to " << new_value << "\n";

        if (new_value > 5000.0) {
            std::cout << "[Mission Control] High-value planet detected! Sending colonization fleet.\n";
        }
    }

    void onStatusChanged(Planet* planet, const std::string& old_status, const std::string& new_status) override {
        std::cout << "[Mission Control] Planet " << planet->getName()
                  << " status changed from " << old_status
                  << " to " << new_status << "\n";
    }
};

class EconomicAnalyzer : public PlanetObserver {
private:
    double total_discovered_resources_ = 0.0;

public:
    void onResourcesChanged(Planet* planet, double old_value, double new_value) override {
        total_discovered_resources_ += (new_value - old_value);
        std::cout << "[Economic Analyzer] Total discovered resources: "
                  << total_discovered_resources_ << "\n";
    }

    void onStatusChanged(Planet* planet, const std::string& old_status, const std::string& new_status) override {
        if (new_status == "Colonized") {
            std::cout << "[Economic Analyzer] New colony established on "
                      << planet->getName() << ". Economic impact: +"
                      << planet->getResources() * 0.1 << " per cycle\n";
        }
    }
};

class ThreatAssessment : public PlanetObserver {
public:
    void onResourcesChanged(Planet* planet, double old_value, double new_value) override {
        // Only care about significant changes
        if (std::abs(new_value - old_value) > 1000.0) {
            std::cout << "[Threat Assessment] Significant resource change detected on "
                      << planet->getName() << ". Investigating...\n";
        }
    }

    void onStatusChanged(Planet* planet, const std::string& old_status, const std::string& new_status) override {
        if (new_status == "Hostile") {
            std::cout << "[Threat Assessment] ALERT: Hostile activity detected on "
                      << planet->getName() << "! Recommend defensive measures.\n";
        }
    }
};

// Modern C++: Type-safe observer with std::function
template<typename EventType>
class TypedObserver {
public:
    using HandlerType = std::function<void(const EventType&)>;

private:
    std::vector<HandlerType> handlers_;

public:
    void subscribe(HandlerType handler) {
        handlers_.push_back(handler);
    }

    void notify(const EventType& event) {
        for (const auto& handler : handlers_) {
            handler(event);
        }
    }
};

struct ResourceChangeEvent {
    std::string planet_name;
    double old_value;
    double new_value;
};

// Usage
void observer_usage() {
    // Create planet and observers
    Planet earth("Earth", 1000.0);

    MissionControl mission_control;
    EconomicAnalyzer economic_analyzer;
    ThreatAssessment threat_assessment;

    // Subscribe observers
    earth.addObserver(&mission_control);
    earth.addObserver(&economic_analyzer);
    earth.addObserver(&threat_assessment);

    // Trigger events
    std::cout << "=== Discovering resources ===\n";
    earth.setResources(6000.0);

    std::cout << "\n=== Changing status ===\n";
    earth.setStatus("Colonized");

    std::cout << "\n=== Another planet becomes hostile ===\n";
    Planet mars("Mars", 3000.0);
    mars.addObserver(&threat_assessment);
    mars.setStatus("Hostile");

    // Modern observer example
    std::cout << "\n=== Modern typed observer ===\n";
    TypedObserver<ResourceChangeEvent> resource_observer;

    resource_observer.subscribe([](const ResourceChangeEvent& event) {
        std::cout << "[Lambda Observer] " << event.planet_name
                  << " resources: " << event.old_value
                  << " -> " << event.new_value << "\n";
    });

    resource_observer.notify({"Venus", 500.0, 1500.0});
}
```

#### **Strategy Pattern**

Defines a family of algorithms and makes them interchangeable.

```cpp
// Strategy interface
class PathfindingStrategy {
public:
    virtual ~PathfindingStrategy() = default;
    virtual std::vector<Point3D> findPath(const Point3D& start, const Point3D& goal) = 0;
    virtual std::string getName() const = 0;
};

// Concrete strategies
class DirectRouteStrategy : public PathfindingStrategy {
public:
    std::vector<Point3D> findPath(const Point3D& start, const Point3D& goal) override {
        std::cout << "[Direct Route] Calculating straight line path...\n";

        // Simple direct path
        std::vector<Point3D> path;
        path.push_back(start);
        path.push_back(goal);

        return path;
    }

    std::string getName() const override {
        return "Direct Route";
    }
};

class SafeRouteStrategy : public PathfindingStrategy {
public:
    std::vector<Point3D> findPath(const Point3D& start, const Point3D& goal) override {
        std::cout << "[Safe Route] Calculating path avoiding dangerous sectors...\n";

        // Longer but safer path with waypoints
        std::vector<Point3D> path;
        path.push_back(start);

        // Add safe waypoints
        Point3D waypoint1(start.x + (goal.x - start.x) * 0.3,
                         start.y + 100, // Detour above
                         start.z + (goal.z - start.z) * 0.3);

        Point3D waypoint2(start.x + (goal.x - start.x) * 0.7,
                         start.y + 100,
                         start.z + (goal.z - start.z) * 0.7);

        path.push_back(waypoint1);
        path.push_back(waypoint2);
        path.push_back(goal);

        return path;
    }

    std::string getName() const override {
        return "Safe Route";
    }
};

class EfficientRouteStrategy : public PathfindingStrategy {
public:
    std::vector<Point3D> findPath(const Point3D& start, const Point3D& goal) override {
        std::cout << "[Efficient Route] Optimizing for fuel consumption...\n";

        // Optimized path considering fuel efficiency
        std::vector<Point3D> path;
        path.push_back(start);

        // Calculate gravity-assist waypoints
        Point3D assist_point(start.x + (goal.x - start.x) * 0.4,
                            start.y - 50, // Use gravity well
                            start.z + (goal.z - start.z) * 0.6);

        path.push_back(assist_point);
        path.push_back(goal);

        return path;
    }

    std::string getName() const override {
        return "Fuel-Efficient Route";
    }
};

// Context class
class NavigationSystem {
private:
    std::unique_ptr<PathfindingStrategy> strategy_;

public:
    NavigationSystem(std::unique_ptr<PathfindingStrategy> strategy)
        : strategy_(std::move(strategy)) {}

    void setStrategy(std::unique_ptr<PathfindingStrategy> new_strategy) {
        strategy_ = std::move(new_strategy);
    }

    std::vector<Point3D> calculateRoute(const Point3D& start, const Point3D& goal) {
        std::cout << "Using strategy: " << strategy_->getName() << "\n";
        return strategy_->findPath(start, goal);
    }

    void displayRoute(const std::vector<Point3D>& route) {
        std::cout << "Route waypoints:\n";
        for (size_t i = 0; i < route.size(); ++i) {
            std::cout << "  " << i + 1 << ". ("
                      << route[i].x << ", "
                      << route[i].y << ", "
                      << route[i].z << ")\n";
        }
        std::cout << "\n";
    }
};

// Modern C++: Strategy with lambdas and std::function
class ModernNavigationSystem {
public:
    using Strategy = std::function<std::vector<Point3D>(const Point3D&, const Point3D&)>;

private:
    Strategy current_strategy_;
    std::string strategy_name_;

public:
    void setStrategy(const std::string& name, Strategy strategy) {
        strategy_name_ = name;
        current_strategy_ = strategy;
    }

    std::vector<Point3D> calculateRoute(const Point3D& start, const Point3D& goal) {
        std::cout << "Using strategy: " << strategy_name_ << "\n";
        return current_strategy_(start, goal);
    }
};

// Usage
void strategy_usage() {
    Point3D start(0, 0, 0);
    Point3D goal(1000, 1000, 1000);

    // Traditional strategy pattern
    NavigationSystem nav_system(std::make_unique<DirectRouteStrategy>());

    std::cout << "=== Direct Route ===\n";
    auto route = nav_system.calculateRoute(start, goal);
    nav_system.displayRoute(route);

    std::cout << "=== Safe Route ===\n";
    nav_system.setStrategy(std::make_unique<SafeRouteStrategy>());
    route = nav_system.calculateRoute(start, goal);
    nav_system.displayRoute(route);

    std::cout << "=== Efficient Route ===\n";
    nav_system.setStrategy(std::make_unique<EfficientRouteStrategy>());
    route = nav_system.calculateRoute(start, goal);
    nav_system.displayRoute(route);

    // Modern strategy with lambdas
    std::cout << "=== Modern Strategy with Lambdas ===\n";
    ModernNavigationSystem modern_nav;

    modern_nav.setStrategy("Lambda Direct", [](const Point3D& start, const Point3D& goal) {
        std::cout << "[Lambda] Direct path calculation\n";
        return std::vector<Point3D>{start, goal};
    });

    route = modern_nav.calculateRoute(start, goal);
    std::cout << "Lambda route calculated with " << route.size() << " waypoints\n\n";

    // Strategy selection based on conditions
    auto selectStrategy = [](double distance, bool dangerous_sector, bool low_fuel) -> std::unique_ptr<PathfindingStrategy> {
        if (low_fuel) {
            return std::make_unique<EfficientRouteStrategy>();
        } else if (dangerous_sector) {
            return std::make_unique<SafeRouteStrategy>();
        } else {
            return std::make_unique<DirectRouteStrategy>();
        }
    };

    std::cout << "=== Dynamic Strategy Selection ===\n";
    nav_system.setStrategy(selectStrategy(1500.0, true, false));  // Safe route
    route = nav_system.calculateRoute(start, goal);
    nav_system.displayRoute(route);
}
```

#### **Command Pattern**

Encapsulates requests as objects, allowing parameterization and queuing.

```cpp
// Command interface
class Command {
public:
    virtual ~Command() = default;
    virtual void execute() = 0;
    virtual void undo() = 0;
    virtual std::string getDescription() const = 0;
};

// Receiver classes
class Fleet {
private:
    std::string name_;
    Point3D position_;
    bool is_active_;

public:
    Fleet(const std::string& name, const Point3D& pos)
        : name_(name), position_(pos), is_active_(false) {}

    void moveTo(const Point3D& new_position) {
        std::cout << "Fleet " << name_ << " moving from ("
                  << position_.x << ", " << position_.y << ", " << position_.z
                  << ") to (" << new_position.x << ", " << new_position.y
                  << ", " << new_position.z << ")\n";
        position_ = new_position;
    }

    void activate() {
        is_active_ = true;
        std::cout << "Fleet " << name_ << " activated\n";
    }

    void deactivate() {
        is_active_ = false;
        std::cout << "Fleet " << name_ << " deactivated\n";
    }

    void attack(const std::string& target) {
        std::cout << "Fleet " << name_ << " attacking " << target << "\n";
    }

    // Getters
    const std::string& getName() const { return name_; }
    const Point3D& getPosition() const { return position_; }
    bool isActive() const { return is_active_; }
};

// Concrete commands
class MoveFleetCommand : public Command {
private:
    Fleet* fleet_;
    Point3D old_position_;
    Point3D new_position_;

public:
    MoveFleetCommand(Fleet* fleet, const Point3D& new_pos)
        : fleet_(fleet), old_position_(fleet->getPosition()), new_position_(new_pos) {}

    void execute() override {
        fleet_->moveTo(new_position_);
    }

    void undo() override {
        fleet_->moveTo(old_position_);
    }

    std::string getDescription() const override {
        return "Move " + fleet_->getName() + " to new position";
    }
};

class ActivateFleetCommand : public Command {
private:
    Fleet* fleet_;

public:
    ActivateFleetCommand(Fleet* fleet) : fleet_(fleet) {}

    void execute() override {
        fleet_->activate();
    }

    void undo() override {
        fleet_->deactivate();
    }

    std::string getDescription() const override {
        return "Activate " + fleet_->getName();
    }
};

class AttackCommand : public Command {
private:
    Fleet* fleet_;
    std::string target_;

public:
    AttackCommand(Fleet* fleet, const std::string& target)
        : fleet_(fleet), target_(target) {}

    void execute() override {
        fleet_->attack(target_);
    }

    void undo() override {
        std::cout << "Ceasing attack on " << target_ << " by " << fleet_->getName() << "\n";
    }

    std::string getDescription() const override {
        return fleet_->getName() + " attack " + target_;
    }
};

// Macro command (composite)
class MacroCommand : public Command {
private:
    std::vector<std::unique_ptr<Command>> commands_;
    std::string description_;

public:
    MacroCommand(const std::string& description) : description_(description) {}

    void addCommand(std::unique_ptr<Command> command) {
        commands_.push_back(std::move(command));
    }

    void execute() override {
        std::cout << "Executing macro: " << description_ << "\n";
        for (auto& command : commands_) {
            command->execute();
        }
    }

    void undo() override {
        std::cout << "Undoing macro: " << description_ << "\n";
        // Undo in reverse order
        for (auto it = commands_.rbegin(); it != commands_.rend(); ++it) {
            (*it)->undo();
        }
    }

    std::string getDescription() const override {
        return "Macro: " + description_;
    }
};

// Invoker
class CommandCenter {
private:
    std::stack<std::unique_ptr<Command>> command_history_;
    std::queue<std::unique_ptr<Command>> command_queue_;

public:
    // Execute command immediately and store in history
    void executeCommand(std::unique_ptr<Command> command) {
        std::cout << "[Command Center] Executing: " << command->getDescription() << "\n";
        command->execute();
        command_history_.push(std::move(command));
    }

    // Queue command for later execution
    void queueCommand(std::unique_ptr<Command> command) {
        std::cout << "[Command Center] Queuing: " << command->getDescription() << "\n";
        command_queue_.push(std::move(command));
    }

    // Process queued commands
    void processQueue() {
        std::cout << "[Command Center] Processing command queue...\n";
        while (!command_queue_.empty()) {
            auto command = std::move(command_queue_.front());
            command_queue_.pop();
            executeCommand(std::move(command));
        }
    }

    // Undo last command
    void undoLastCommand() {
        if (!command_history_.empty()) {
            auto last_command = std::move(command_history_.top());
            command_history_.pop();
            std::cout << "[Command Center] Undoing: " << last_command->getDescription() << "\n";
            last_command->undo();
        } else {
            std::cout << "[Command Center] No commands to undo\n";
        }
    }

    void showHistory() const {
        std::cout << "[Command Center] Command history size: " << command_history_.size() << "\n";
    }
};

// Usage
void command_usage() {
    // Create receivers
    Fleet alpha_fleet("Alpha Squadron", Point3D(0, 0, 0));
    Fleet beta_fleet("Beta Squadron", Point3D(100, 0, 0));

    // Create invoker
    CommandCenter command_center;

    // Execute individual commands
    std::cout << "=== Individual Commands ===\n";
    command_center.executeCommand(
        std::make_unique<ActivateFleetCommand>(&alpha_fleet)
    );

    command_center.executeCommand(
        std::make_unique<MoveFleetCommand>(&alpha_fleet, Point3D(50, 50, 0))
    );

    command_center.executeCommand(
        std::make_unique<AttackCommand>(&alpha_fleet, "Enemy Base")
    );

    // Undo commands
    std::cout << "\n=== Undo Operations ===\n";
    command_center.undoLastCommand();  // Undo attack
    command_center.undoLastCommand();  // Undo move

    // Macro command
    std::cout << "\n=== Macro Command ===\n";
    auto coordinated_attack = std::make_unique<MacroCommand>("Coordinated Attack");

    coordinated_attack->addCommand(
        std::make_unique<ActivateFleetCommand>(&beta_fleet)
    );
    coordinated_attack->addCommand(
        std::make_unique<MoveFleetCommand>(&beta_fleet, Point3D(200, 200, 0))
    );
    coordinated_attack->addCommand(
        std::make_unique<AttackCommand>(&beta_fleet, "Enemy Outpost")
    );

    command_center.executeCommand(std::move(coordinated_attack));

    // Command queuing
    std::cout << "\n=== Command Queuing ===\n";
    command_center.queueCommand(
        std::make_unique<MoveFleetCommand>(&alpha_fleet, Point3D(300, 100, 0))
    );
    command_center.queueCommand(
        std::make_unique<AttackCommand>(&alpha_fleet, "Strategic Target")
    );

    command_center.processQueue();

    command_center.showHistory();
}
```

### 📊 Pattern Relationships and Usage

#### **When to Use Each Pattern**

| Pattern       | Problem Solved                                 | When to Use                                          | Example Use Cases                   |
| ------------- | ---------------------------------------------- | ---------------------------------------------------- | ----------------------------------- |
| **Singleton** | Global access to unique instance               | Need exactly one instance                            | Resource managers, loggers, config  |
| **Factory**   | Object creation without specifying exact class | Many similar objects, runtime decisions              | Mission types, UI elements          |
| **Builder**   | Complex object construction                    | Many constructor parameters, step-by-step creation   | Spaceships, complex queries         |
| **Adapter**   | Interface incompatibility                      | Integrate legacy systems, third-party libraries      | Legacy APIs, external libraries     |
| **Decorator** | Add behavior dynamically                       | Enhance objects at runtime, avoid subclass explosion | Mission enhancements, UI components |
| **Observer**  | One-to-many notifications                      | Loose coupling, event systems                        | UI updates, monitoring systems      |
| **Strategy**  | Algorithm selection at runtime                 | Multiple ways to solve same problem                  | Pathfinding, sorting, pricing       |
| **Command**   | Encapsulate requests                           | Undo/redo, queuing, logging, macro operations        | GUI actions, network requests       |

#### **Modern C++ Pattern Enhancements**

```cpp
// Using modern C++ features with patterns
namespace modern_patterns {

    // Factory with perfect forwarding
    template<typename T, typename... Args>
    std::unique_ptr<T> make_mission(Args&&... args) {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }

    // Observer with weak_ptr to avoid dangling pointers
    class SafeObserver {
    public:
        using WeakObserver = std::weak_ptr<PlanetObserver>;

    private:
        std::vector<WeakObserver> observers_;

    public:
        void addObserver(std::shared_ptr<PlanetObserver> observer) {
            observers_.push_back(observer);
        }

        void notifyAll(auto&& notification) {
            observers_.erase(
                std::remove_if(observers_.begin(), observers_.end(),
                    [&](const WeakObserver& weak_obs) {
                        if (auto obs = weak_obs.lock()) {
                            notification(obs.get());
                            return false;  // Keep valid observers
                        }
                        return true;  // Remove expired observers
                    }),
                observers_.end()
            );
        }
    };

    // Strategy with concepts (C++20)
    template<typename T>
    concept PathfindingAlgorithm = requires(T t, Point3D start, Point3D goal) {
        { t.findPath(start, goal) } -> std::convertible_to<std::vector<Point3D>>;
        { t.getName() } -> std::convertible_to<std::string>;
    };

    template<PathfindingAlgorithm Strategy>
    class ConceptNavigationSystem {
    private:
        Strategy strategy_;

    public:
        ConceptNavigationSystem(Strategy strategy) : strategy_(std::move(strategy)) {}

        std::vector<Point3D> calculateRoute(const Point3D& start, const Point3D& goal) {
            std::cout << "Using algorithm: " << strategy_.getName() << "\n";
            return strategy_.findPath(start, goal);
        }
    };

    // RAII Command with automatic cleanup
    class RAIICommand : public Command {
    private:
        std::function<void()> execute_func_;
        std::function<void()> cleanup_func_;
        bool executed_ = false;

    public:
        template<typename ExecFunc, typename CleanupFunc>
        RAIICommand(const std::string& desc, ExecFunc&& exec, CleanupFunc&& cleanup)
            : execute_func_(std::forward<ExecFunc>(exec))
            , cleanup_func_(std::forward<CleanupFunc>(cleanup))
            , description_(desc) {}

        void execute() override {
            execute_func_();
            executed_ = true;
        }

        void undo() override {
            if (executed_) {
                cleanup_func_();
            }
        }

        std::string getDescription() const override {
            return description_;
        }

        ~RAIICommand() {
            if (executed_) {
                try {
                    cleanup_func_();
                } catch (...) {
                    // Log error but don't throw in destructor
                }
            }
        }

    private:
        std::string description_;
    };

    // Factory with registry and type erasure
    class UniversalFactory {
    public:
        template<typename T>
        class Creator {
        public:
            virtual ~Creator() = default;
            virtual std::unique_ptr<T> create() = 0;
            virtual std::string getTypeName() const = 0;
        };

        template<typename Base, typename Derived>
        class ConcreteCreator : public Creator<Base> {
        public:
            std::unique_ptr<Base> create() override {
                return std::make_unique<Derived>();
            }

            std::string getTypeName() const override {
                return typeid(Derived).name();
            }
        };

    private:
        template<typename T>
        using CreatorMap = std::unordered_map<std::string, std::unique_ptr<Creator<T>>>;

        template<typename T>
        static CreatorMap<T>& getRegistry() {
            static CreatorMap<T> registry;
            return registry;
        }

    public:
        template<typename Base, typename Derived>
        static void registerType(const std::string& name) {
            getRegistry<Base>()[name] = std::make_unique<ConcreteCreator<Base, Derived>>();
        }

        template<typename T>
        static std::unique_ptr<T> create(const std::string& name) {
            auto& registry = getRegistry<T>();
            auto it = registry.find(name);
            if (it != registry.end()) {
                return it->second->create();
            }
            throw std::invalid_argument("Type not registered: " + name);
        }

        template<typename T>
        static std::vector<std::string> getRegisteredTypes() {
            std::vector<std::string> types;
            for (const auto& [name, creator] : getRegistry<T>()) {
                types.push_back(name);
            }
            return types;
        }
    };
}

// Usage of modern patterns
void modern_patterns_usage() {
    using namespace modern_patterns;

    // Universal factory
    UniversalFactory::registerType<Mission, ExplorationMission>("exploration");
    UniversalFactory::registerType<Mission, CombatMission>("combat");

    auto mission = UniversalFactory::create<Mission>("exploration");
    mission->execute();

    // RAII Command
    Fleet test_fleet("Test", Point3D(0, 0, 0));
    auto raii_cmd = std::make_unique<RAIICommand>(
        "RAII Fleet Operation",
        [&test_fleet]() {
            test_fleet.activate();
            std::cout << "Fleet operation started\n";
        },
        [&test_fleet]() {
            test_fleet.deactivate();
            std::cout << "Fleet operation cleaned up\n";
        }
    );

    raii_cmd->execute();
    // Cleanup happens automatically when raii_cmd is destroyed
}
```

### 🔄 Advanced Pattern Combinations

#### **Model-View-Controller (MVC) with Observer**

```cpp
// Model with Observer pattern
class GameModel {
public:
    struct GameState {
        int score = 0;
        int level = 1;
        std::string status = "Playing";
    };

    using StateObserver = std::function<void(const GameState&)>;

private:
    GameState state_;
    std::vector<StateObserver> observers_;

public:
    void addObserver(StateObserver observer) {
        observers_.push_back(std::move(observer));
    }

    void updateScore(int points) {
        state_.score += points;
        notifyObservers();
    }

    void levelUp() {
        state_.level++;
        notifyObservers();
    }

    void setStatus(const std::string& status) {
        state_.status = status;
        notifyObservers();
    }

    const GameState& getState() const { return state_; }

private:
    void notifyObservers() {
        for (const auto& observer : observers_) {
            observer(state_);
        }
    }
};

// View
class GameView {
public:
    void displayState(const GameModel::GameState& state) {
        std::cout << "\n=== Game Status ===\n";
        std::cout << "Score: " << state.score << "\n";
        std::cout << "Level: " << state.level << "\n";
        std::cout << "Status: " << state.status << "\n";
        std::cout << "==================\n";
    }

    void showMessage(const std::string& message) {
        std::cout << "[MESSAGE] " << message << "\n";
    }
};

// Controller with Command pattern
class GameController {
private:
    GameModel* model_;
    GameView* view_;
    std::stack<std::unique_ptr<Command>> command_history_;

public:
    GameController(GameModel* model, GameView* view)
        : model_(model), view_(view) {

        // Register as observer of model
        model_->addObserver([this](const GameModel::GameState& state) {
            view_->displayState(state);
        });
    }

    void executeCommand(std::unique_ptr<Command> command) {
        command->execute();
        command_history_.push(std::move(command));
    }

    void undo() {
        if (!command_history_.empty()) {
            auto command = std::move(command_history_.top());
            command_history_.pop();
            command->undo();
        }
    }
};

// Game commands
class ScoreCommand : public Command {
private:
    GameModel* model_;
    int points_;

public:
    ScoreCommand(GameModel* model, int points)
        : model_(model), points_(points) {}

    void execute() override {
        model_->updateScore(points_);
    }

    void undo() override {
        model_->updateScore(-points_);
    }

    std::string getDescription() const override {
        return "Add " + std::to_string(points_) + " points";
    }
};
```

#### **Abstract Factory with Builder**

```cpp
// Abstract product families
class Engine {
public:
    virtual ~Engine() = default;
    virtual void start() = 0;
    virtual std::string getType() const = 0;
};

class Weapon {
public:
    virtual ~Weapon() = default;
    virtual void fire() = 0;
    virtual std::string getType() const = 0;
};

class Shield {
public:
    virtual ~Shield() = default;
    virtual void activate() = 0;
    virtual std::string getType() const = 0;
};

// Concrete products for different families
// Military family
class MilitaryEngine : public Engine {
public:
    void start() override {
        std::cout << "Military fusion engine started\n";
    }

    std::string getType() const override {
        return "Military Fusion Engine";
    }
};

class MilitaryWeapon : public Weapon {
public:
    void fire() override {
        std::cout << "Plasma cannon fired\n";
    }

    std::string getType() const override {
        return "Plasma Cannon";
    }
};

class MilitaryShield : public Shield {
public:
    void activate() override {
        std::cout << "Military shield activated\n";
    }

    std::string getType() const override {
        return "Military Grade Shield";
    }
};

// Civilian family
class CivilianEngine : public Engine {
public:
    void start() override {
        std::cout << "Civilian ion engine started\n";
    }

    std::string getType() const override {
        return "Civilian Ion Engine";
    }
};

class CivilianWeapon : public Weapon {
public:
    void fire() override {
        std::cout << "Point defense system activated\n";
    }

    std::string getType() const override {
        return "Point Defense System";
    }
};

class CivilianShield : public Shield {
public:
    void activate() override {
        std::cout << "Civilian shield activated\n";
    }

    std::string getType() const override {
        return "Civilian Shield";
    }
};

// Abstract factory
class ComponentFactory {
public:
    virtual ~ComponentFactory() = default;
    virtual std::unique_ptr<Engine> createEngine() = 0;
    virtual std::unique_ptr<Weapon> createWeapon() = 0;
    virtual std::unique_ptr<Shield> createShield() = 0;
    virtual std::string getFactoryType() const = 0;
};

// Concrete factories
class MilitaryComponentFactory : public ComponentFactory {
public:
    std::unique_ptr<Engine> createEngine() override {
        return std::make_unique<MilitaryEngine>();
    }

    std::unique_ptr<Weapon> createWeapon() override {
        return std::make_unique<MilitaryWeapon>();
    }

    std::unique_ptr<Shield> createShield() override {
        return std::make_unique<MilitaryShield>();
    }

    std::string getFactoryType() const override {
        return "Military";
    }
};

class CivilianComponentFactory : public ComponentFactory {
public:
    std::unique_ptr<Engine> createEngine() override {
        return std::make_unique<CivilianEngine>();
    }

    std::unique_ptr<Weapon> createWeapon() override {
        return std::make_unique<CivilianWeapon>();
    }

    std::unique_ptr<Shield> createShield() override {
        return std::make_unique<CivilianShield>();
    }

    std::string getFactoryType() const override {
        return "Civilian";
    }
};

// Builder using Abstract Factory
class SpaceshipBuilderWithFactory {
private:
    std::unique_ptr<ComponentFactory> factory_;
    std::string name_;
    std::unique_ptr<Engine> engine_;
    std::vector<std::unique_ptr<Weapon>> weapons_;
    std::unique_ptr<Shield> shield_;

public:
    SpaceshipBuilderWithFactory(std::unique_ptr<ComponentFactory> factory)
        : factory_(std::move(factory)) {}

    SpaceshipBuilderWithFactory& setName(const std::string& name) {
        name_ = name;
        return *this;
    }

    SpaceshipBuilderWithFactory& addEngine() {
        engine_ = factory_->createEngine();
        return *this;
    }

    SpaceshipBuilderWithFactory& addWeapon() {
        weapons_.push_back(factory_->createWeapon());
        return *this;
    }

    SpaceshipBuilderWithFactory& addShield() {
        shield_ = factory_->createShield();
        return *this;
    }

    void displaySpaceship() {
        std::cout << "\n=== Spaceship: " << name_ << " ===\n";
        std::cout << "Type: " << factory_->getFactoryType() << "\n";

        if (engine_) {
            std::cout << "Engine: " << engine_->getType() << "\n";
        }

        if (shield_) {
            std::cout << "Shield: " << shield_->getType() << "\n";
        }

        std::cout << "Weapons:\n";
        for (const auto& weapon : weapons_) {
            std::cout << "  - " << weapon->getType() << "\n";
        }

        std::cout << "========================\n\n";
    }
};

// Usage
void combined_patterns_usage() {
    std::cout << "=== MVC Pattern Example ===\n";
    GameModel model;
    GameView view;
    GameController controller(&model, &view);

    // Execute commands through controller
    controller.executeCommand(std::make_unique<ScoreCommand>(&model, 100));
    controller.executeCommand(std::make_unique<ScoreCommand>(&model, 50));

    std::cout << "\n=== Abstract Factory + Builder ===\n";

    // Military spaceship
    auto military_factory = std::make_unique<MilitaryComponentFactory>();
    SpaceshipBuilderWithFactory military_builder(std::move(military_factory));

    military_builder
        .setName("USS Enterprise")
        .addEngine()
        .addWeapon()
        .addWeapon()
        .addShield();

    military_builder.displaySpaceship();

    // Civilian spaceship
    auto civilian_factory = std::make_unique<CivilianComponentFactory>();
    SpaceshipBuilderWithFactory civilian_builder(std::move(civilian_factory));

    civilian_builder
        .setName("Merchant Vessel Aurora")
        .addEngine()
        .addWeapon()
        .addShield();

    civilian_builder.displaySpaceship();
}
```

### 🎯 Pattern Selection Guidelines

#### **Decision Tree for Pattern Selection**

```cpp
namespace pattern_selection {

    // Helper for pattern selection
    class PatternSelector {
    public:
        enum class Concern {
            ObjectCreation,
            ObjectStructure,
            ObjectBehavior,
            SystemArchitecture
        };

        enum class Constraint {
            SingleInstance,
            ComplexConstruction,
            RuntimePolymorphism,
            LegacyIntegration,
            EventNotification,
            AlgorithmVariation,
            RequestEncapsulation
        };

        static std::string recommendPattern(Concern concern, Constraint constraint) {
            switch (concern) {
                case Concern::ObjectCreation:
                    switch (constraint) {
                        case Constraint::SingleInstance:
                            return "Singleton - Ensure only one instance exists";
                        case Constraint::ComplexConstruction:
                            return "Builder - Step-by-step object construction";
                        default:
                            return "Factory - Hide object creation complexity";
                    }

                case Concern::ObjectStructure:
                    switch (constraint) {
                        case Constraint::LegacyIntegration:
                            return "Adapter - Make incompatible interfaces work together";
                        case Constraint::RuntimePolymorphism:
                            return "Decorator - Add behavior dynamically";
                        default:
                            return "Composite - Treat objects and compositions uniformly";
                    }

                case Concern::ObjectBehavior:
                    switch (constraint) {
                        case Constraint::EventNotification:
                            return "Observer - Notify multiple objects of changes";
                        case Constraint::AlgorithmVariation:
                            return "Strategy - Choose algorithms at runtime";
                        case Constraint::RequestEncapsulation:
                            return "Command - Encapsulate requests as objects";
                        default:
                            return "Template Method - Define algorithm skeleton";
                    }

                default:
                    return "Consider architectural patterns (MVC, MVP, MVVM)";
            }
        }
    };

    void demonstrate_selection() {
        std::cout << "=== Pattern Selection Guide ===\n";

        std::cout << PatternSelector::recommendPattern(
            PatternSelector::Concern::ObjectCreation,
            PatternSelector::Constraint::SingleInstance
        ) << "\n";

        std::cout << PatternSelector::recommendPattern(
            PatternSelector::Concern::ObjectBehavior,
            PatternSelector::Constraint::EventNotification
        ) << "\n";

        std::cout << PatternSelector::recommendPattern(
            PatternSelector::Concern::ObjectStructure,
            PatternSelector::Constraint::LegacyIntegration
        ) << "\n";
    }
}
```

### 📊 Pattern Comparison Table

| Aspect            | Singleton       | Factory         | Builder              | Adapter                 | Decorator        | Observer           | Strategy            | Command               |
| ----------------- | --------------- | --------------- | -------------------- | ----------------------- | ---------------- | ------------------ | ------------------- | --------------------- |
| **Purpose**       | Single instance | Object creation | Complex construction | Interface compatibility | Dynamic behavior | Event notification | Algorithm selection | Request encapsulation |
| **Complexity**    | Low             | Medium          | Medium-High          | Low-Medium              | Medium           | Medium-High        | Medium              | Medium-High           |
| **Flexibility**   | Low             | High            | High                 | Medium                  | Very High        | High               | High                | Very High             |
| **Performance**   | Good            | Good            | Good                 | Good                    | Medium           | Medium             | Good                | Medium                |
| **Testability**   | Poor            | Good            | Good                 | Good                    | Good             | Medium             | Excellent           | Good                  |
| **Thread Safety** | Challenging     | Good            | Good                 | Good                    | Depends          | Challenging        | Good                | Good                  |

### 🏆 Best Practices Summary

#### **General Guidelines**

1. **Favor composition over inheritance** - Use patterns like Strategy and Decorator
2. **Program to interfaces, not implementations** - Abstract factories and interfaces
3. **Principle of least knowledge** - Objects should know as little as possible about others
4. **Open/Closed principle** - Open for extension, closed for modification
5. **Single responsibility** - Each class should have one reason to change

#### **Modern C++ Considerations**

```cpp
// Prefer smart pointers for memory management
std::unique_ptr<Strategy> strategy = std::make_unique<ConcreteStrategy>();

// Use perfect forwarding in factories
template<typename T, typename... Args>
std::unique_ptr<T> make_object(Args&&... args) {
    return std::make_unique<T>(std::forward<Args>(args)...);
}

// Leverage move semantics
Command createCommand() {
    return std::move(local_command);  // Move instead of copy
}

// Use concepts for better constraints (C++20)
template<typename T>
concept Drawable = requires(T obj) {
    obj.draw();
};
```

#### **Anti-Patterns to Avoid**

- **God Object**: Single class doing too much
- **Singleton Overuse**: Not everything needs to be a singleton
- **Anemic Domain Model**: Classes with no behavior, only data
- **Magic Numbers**: Use named constants instead
- **Copy-Paste Programming**: Extract common functionality

### 🎯 Key Takeaways

1. **Patterns are tools, not rules** - Use when appropriate, not everywhere
2. **Understand the problem first** - Pattern selection should solve real problems
3. **Keep it simple** - Don't over-engineer solutions
4. **Consider modern C++ features** - Smart pointers, lambdas, concepts
5. **Think about testability** - Patterns should make testing easier
6. **Performance matters** - Some patterns have runtime costs
7. **Maintainability is key** - Code should be understandable and modifiable
8. **Document your decisions** - Explain why you chose specific patterns

---

_This cheat sheet covers the most commonly used design patterns in modern C++. For complete implementations and real-world examples, see the `src/patterns/` directory in CppVerseHub._
