# Object-Oriented Programming (OOP) Cheat Sheet

**Location:** `CppVerseHub/docs/cheat_sheets/OOP_CheatSheet.md`

## 📚 Quick Reference Guide for C++ OOP Concepts

### 🏗️ Core OOP Principles

#### 1. **Encapsulation** 🔒

Bundling data and methods that operate on that data within a single unit.

```cpp
class Planet {
private:
    std::string name_;           // Private data
    double resources_;

protected:
    int security_level_;         // Accessible to derived classes

public:
    // Public interface
    void setName(const std::string& name) { name_ = name; }
    std::string getName() const { return name_; }

    // Getter/Setter pattern
    double getResources() const { return resources_; }
    void addResources(double amount) {
        if (amount > 0) resources_ += amount;
    }
};
```

#### 2. **Inheritance** 🌳

Creating new classes based on existing classes.

```cpp
// Base class
class Entity {
protected:
    std::string id_;
    Vector3D position_;

public:
    Entity(const std::string& id) : id_(id) {}
    virtual ~Entity() = default;                    // Virtual destructor

    virtual void update() = 0;                      // Pure virtual function
    virtual void render() const {}                  // Virtual function

    const std::string& getId() const { return id_; }
};

// Derived class
class Planet : public Entity {
private:
    double resources_;
    std::vector<std::string> inhabitants_;

public:
    Planet(const std::string& id, double resources)
        : Entity(id), resources_(resources) {}

    // Override base class methods
    void update() override {
        // Planet-specific update logic
        generateResources();
    }

    void render() const override {
        // Planet-specific rendering
        drawPlanet();
    }

private:
    void generateResources() { resources_ += 0.1; }
    void drawPlanet() const { /* rendering code */ }
};
```

#### 3. **Polymorphism** 🎭

Objects of different types can be treated as objects of a common base type.

```cpp
// Runtime polymorphism with virtual functions
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
        std::cout << "Engaging enemy forces...\n";
    }

    std::string getType() const override {
        return "Combat";
    }

    double getCost() const override {
        return 2500.0;
    }
};

// Polymorphic usage
void executeMission(std::unique_ptr<Mission> mission) {
    std::cout << "Mission type: " << mission->getType() << "\n";
    std::cout << "Cost: " << mission->getCost() << "\n";
    mission->execute();
}

// Usage example
auto exploration = std::make_unique<ExplorationMission>();
auto combat = std::make_unique<CombatMission>();

executeMission(std::move(exploration));   // Calls ExplorationMission methods
executeMission(std::move(combat));        // Calls CombatMission methods
```

#### 4. **Abstraction** 🎨

Hiding complex implementation details and showing only essential features.

```cpp
// Abstract interface
class IResourceManager {
public:
    virtual ~IResourceManager() = default;
    virtual void allocateResources(const std::string& entityId, double amount) = 0;
    virtual double getAvailableResources() const = 0;
    virtual bool hasEnoughResources(double amount) const = 0;
};

// Concrete implementation
class ResourceManager : public IResourceManager {
private:
    double total_resources_;
    std::unordered_map<std::string, double> allocations_;

public:
    void allocateResources(const std::string& entityId, double amount) override {
        if (hasEnoughResources(amount)) {
            total_resources_ -= amount;
            allocations_[entityId] += amount;
        }
    }

    double getAvailableResources() const override {
        return total_resources_;
    }

    bool hasEnoughResources(double amount) const override {
        return total_resources_ >= amount;
    }
};
```

### 🏛️ Class Design Patterns

#### **Constructor Types**

```cpp
class Fleet {
private:
    std::string name_;
    std::vector<std::unique_ptr<Ship>> ships_;
    double fuel_;

public:
    // Default constructor
    Fleet() : name_("Unnamed Fleet"), fuel_(0.0) {}

    // Parameterized constructor
    Fleet(const std::string& name, double fuel)
        : name_(name), fuel_(fuel) {}

    // Copy constructor
    Fleet(const Fleet& other)
        : name_(other.name_), fuel_(other.fuel_) {
        // Deep copy ships
        for (const auto& ship : other.ships_) {
            ships_.push_back(std::make_unique<Ship>(*ship));
        }
    }

    // Move constructor
    Fleet(Fleet&& other) noexcept
        : name_(std::move(other.name_))
        , ships_(std::move(other.ships_))
        , fuel_(other.fuel_) {
        other.fuel_ = 0.0;
    }

    // Copy assignment operator
    Fleet& operator=(const Fleet& other) {
        if (this != &other) {
            name_ = other.name_;
            fuel_ = other.fuel_;
            ships_.clear();
            for (const auto& ship : other.ships_) {
                ships_.push_back(std::make_unique<Ship>(*ship));
            }
        }
        return *this;
    }

    // Move assignment operator
    Fleet& operator=(Fleet&& other) noexcept {
        if (this != &other) {
            name_ = std::move(other.name_);
            ships_ = std::move(other.ships_);
            fuel_ = other.fuel_;
            other.fuel_ = 0.0;
        }
        return *this;
    }

    // Destructor
    ~Fleet() = default;  // Automatic cleanup with smart pointers
};
```

#### **Access Specifiers**

```cpp
class SpaceStation {
private:
    // Only accessible within this class
    std::string security_code_;
    void validateSecurity() const {}

protected:
    // Accessible to this class and derived classes
    int defense_level_;
    void upgradeDefenses() {}

public:
    // Accessible to everyone
    std::string getName() const { return name_; }
    void dockShip(const Ship& ship) {}

    // Friend function can access private members
    friend std::ostream& operator<<(std::ostream& os, const SpaceStation& station);

private:
    std::string name_;
};

// Friend function implementation
std::ostream& operator<<(std::ostream& os, const SpaceStation& station) {
    os << "Station: " << station.name_
       << " (Security: " << station.security_code_ << ")";
    return os;
}
```

### 🔄 Inheritance Patterns

#### **Single Inheritance**

```cpp
class Vehicle {
protected:
    double speed_;
    double fuel_;
public:
    Vehicle(double speed) : speed_(speed), fuel_(100.0) {}
    virtual void move() = 0;
};

class Spaceship : public Vehicle {
public:
    Spaceship() : Vehicle(50.0) {}
    void move() override {
        std::cout << "Spaceship moving at " << speed_ << " units/sec\n";
    }
};
```

#### **Multiple Inheritance** (Use with caution)

```cpp
class Flyable {
public:
    virtual void fly() = 0;
    virtual ~Flyable() = default;
};

class Weaponized {
public:
    virtual void attack() = 0;
    virtual ~Weaponized() = default;
};

class Fighter : public Spaceship, public Flyable, public Weaponized {
public:
    void fly() override {
        std::cout << "Fighter flying in formation\n";
    }

    void attack() override {
        std::cout << "Fighter attacking with lasers\n";
    }
};
```

#### **Diamond Problem Solution**

```cpp
class Entity {
public:
    virtual void update() = 0;
};

class Physical : virtual public Entity {  // Virtual inheritance
public:
    void applyPhysics() {}
};

class Renderable : virtual public Entity {  // Virtual inheritance
public:
    void render() {}
};

class GameObject : public Physical, public Renderable {
public:
    void update() override {  // Only one implementation needed
        applyPhysics();
        render();
    }
};
```

### 🎯 Virtual Functions & Polymorphism

#### **Virtual Function Types**

```cpp
class Mission {
public:
    // Pure virtual function (abstract)
    virtual void execute() = 0;

    // Virtual function with default implementation
    virtual void prepare() {
        std::cout << "General mission preparation\n";
    }

    // Virtual destructor (essential for inheritance)
    virtual ~Mission() = default;

    // Non-virtual function (static binding)
    void logMission() const {
        std::cout << "Mission logged\n";
    }
};
```

#### **Override and Final Keywords**

```cpp
class SpecialMission : public Mission {
public:
    // Override ensures we're actually overriding
    void execute() override {
        std::cout << "Executing special mission\n";
    }

    void prepare() override {
        std::cout << "Special preparation required\n";
    }

    // Final prevents further overriding
    virtual void finalize() final {
        std::cout << "Mission finalized\n";
    }
};

// This would cause compilation error:
// class UltraSpecialMission : public SpecialMission {
//     void finalize() override {}  // ERROR: cannot override final
// };
```

### 🏭 Advanced OOP Concepts

#### **Composition vs Inheritance**

```cpp
// Composition: "Has-a" relationship
class Engine {
public:
    void start() { std::cout << "Engine started\n"; }
    void stop() { std::cout << "Engine stopped\n"; }
};

class Ship {
private:
    Engine engine_;  // Composition: Ship HAS an Engine
    std::string name_;

public:
    Ship(const std::string& name) : name_(name) {}

    void launch() {
        engine_.start();
        std::cout << name_ << " launching\n";
    }

    void land() {
        std::cout << name_ << " landing\n";
        engine_.stop();
    }
};

// Inheritance: "Is-a" relationship
class Cruiser : public Ship {  // Cruiser IS a Ship
public:
    Cruiser(const std::string& name) : Ship(name) {}

    void patrol() {
        std::cout << "Cruiser patrolling sector\n";
    }
};
```

#### **Abstract Base Classes**

```cpp
// Abstract factory pattern
class EntityFactory {
public:
    virtual ~EntityFactory() = default;
    virtual std::unique_ptr<Entity> createPlanet(const std::string& name) = 0;
    virtual std::unique_ptr<Entity> createFleet(const std::string& name) = 0;
};

class StandardEntityFactory : public EntityFactory {
public:
    std::unique_ptr<Entity> createPlanet(const std::string& name) override {
        return std::make_unique<Planet>(name, 1000.0);
    }

    std::unique_ptr<Entity> createFleet(const std::string& name) override {
        return std::make_unique<Fleet>(name, 500.0);
    }
};
```

### 🛠️ Best Practices

#### **RAII (Resource Acquisition Is Initialization)**

```cpp
class DatabaseConnection {
private:
    bool connected_;

public:
    DatabaseConnection() : connected_(false) {
        // Acquire resource in constructor
        connect();
    }

    ~DatabaseConnection() {
        // Release resource in destructor
        if (connected_) {
            disconnect();
        }
    }

    // Delete copy operations for unique resources
    DatabaseConnection(const DatabaseConnection&) = delete;
    DatabaseConnection& operator=(const DatabaseConnection&) = delete;

    // Move operations are OK
    DatabaseConnection(DatabaseConnection&& other) noexcept
        : connected_(other.connected_) {
        other.connected_ = false;
    }

private:
    void connect() { connected_ = true; }
    void disconnect() { connected_ = false; }
};
```

#### **Const Correctness**

```cpp
class Point {
private:
    double x_, y_;

public:
    Point(double x, double y) : x_(x), y_(y) {}

    // Const member functions
    double getX() const { return x_; }
    double getY() const { return y_; }
    double distance(const Point& other) const {
        double dx = x_ - other.x_;
        double dy = y_ - other.y_;
        return std::sqrt(dx*dx + dy*dy);
    }

    // Non-const member functions
    void setX(double x) { x_ = x; }
    void setY(double y) { y_ = y; }

    // Const and non-const overloads
    const Point& getClosestPoint(const std::vector<Point>& points) const;
    Point& getClosestPoint(std::vector<Point>& points);
};
```

### 🔍 Common Pitfalls & Solutions

#### **Slicing Problem**

```cpp
// Problem: Object slicing
void processEntity(Entity entity) {  // Pass by value - BAD!
    entity.update();  // Loses derived class information
}

// Solution: Use references or pointers
void processEntity(const Entity& entity) {  // Pass by reference - GOOD!
    entity.update();  // Maintains polymorphism
}

void processEntity(std::unique_ptr<Entity> entity) {  // Smart pointer - BEST!
    entity->update();  // Safe memory management + polymorphism
}
```

#### **Memory Management**

```cpp
class BadFleet {
    Ship** ships_;  // Raw pointers - BAD!
    int ship_count_;
};

class GoodFleet {
    std::vector<std::unique_ptr<Ship>> ships_;  // Smart pointers - GOOD!
};
```

### 📋 Quick Reference Summary

| Concept        | Keyword                       | Example                        |
| -------------- | ----------------------------- | ------------------------------ |
| Inheritance    | `class Derived : public Base` | `class Planet : public Entity` |
| Pure Virtual   | `virtual func() = 0`          | `virtual void update() = 0`    |
| Override       | `override`                    | `void update() override`       |
| Final          | `final`                       | `virtual void process() final` |
| Access         | `private/protected/public`    | `public: void method()`        |
| Composition    | Member variable               | `Engine engine_;`              |
| Polymorphism   | Virtual functions             | `virtual void execute()`       |
| Abstract Class | Pure virtual function         | Contains `= 0` methods         |

### 🎯 Key Takeaways

1. **Favor composition over inheritance** when possible
2. **Always use virtual destructors** in base classes
3. **Make interfaces abstract** with pure virtual functions
4. **Use const correctness** throughout your design
5. **Prefer smart pointers** over raw pointers
6. **Follow RAII principles** for resource management
7. **Use override keyword** to catch errors early
8. **Design for polymorphism** with virtual functions

---

_This cheat sheet covers the essential OOP concepts used throughout CppVerseHub. For practical examples, see the `src/core/` directory._
