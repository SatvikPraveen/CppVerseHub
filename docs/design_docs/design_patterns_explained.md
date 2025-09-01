# Design Patterns Explained - CppVerseHub

**Location:** `CppVerseHub/docs/design_docs/design_patterns_explained.md`

## Introduction to Design Patterns in CppVerseHub

This document explains the rationale behind each design pattern implementation in CppVerseHub, detailing why each pattern was chosen, how it solves specific problems, and how it contributes to the overall system architecture.

## Pattern Selection Criteria

Our pattern selection follows these principles:

1. **Educational Value**: Demonstrates pattern concepts clearly
2. **Real-World Applicability**: Solves actual architectural problems
3. **C++ Specifics**: Leverages C++ language features effectively
4. **Maintainability**: Improves code organization and flexibility
5. **Performance**: Maintains or improves system efficiency

## Creational Patterns

### Singleton Pattern - ResourceManager

**Problem Solved:**
The simulation requires a single, globally accessible resource management system that coordinates resource allocation across all entities while maintaining thread safety.

**Why Singleton Here:**

- **Global State Necessity**: Resources are inherently shared across the entire simulation
- **Controlled Access**: Prevents multiple resource managers from creating conflicts
- **Thread Safety**: Single instance simplifies concurrent access patterns
- **Initialization Control**: Ensures proper setup before any resource operations

**C++ Implementation Details:**

```cpp
class ResourceManager {
private:
    static std::unique_ptr<ResourceManager> instance_;
    static std::once_flag init_flag_;

    ResourceManager() = default;  // Private constructor

public:
    static ResourceManager& getInstance() {
        std::call_once(init_flag_, []() {
            instance_ = std::unique_ptr<ResourceManager>(new ResourceManager());
        });
        return *instance_;
    }

    // Delete copy constructor and assignment
    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;
};
```

**Benefits in Context:**

- Thread-safe initialization using `std::call_once`
- Memory management via `std::unique_ptr`
- Prevents accidental copying
- Lazy initialization reduces startup overhead

**Alternative Considered:**
Dependency injection was considered but rejected because resource management is truly global in scope and the singleton pattern better represents the domain reality.

### Factory Pattern - EntityFactory

**Problem Solved:**
The system needs to create different types of entities (planets, fleets) based on runtime configuration while decoupling creation logic from client code.

**Why Factory Here:**

- **Runtime Type Determination**: Entity types determined by configuration files
- **Decoupling Creation**: Clients don't need to know concrete classes
- **Extensibility**: New entity types can be added without modifying existing code
- **Parameter Complexity**: Entity creation requires complex initialization

**C++ Implementation Approach:**

```cpp
class EntityFactory {
public:
    template<typename T, typename... Args>
    static std::unique_ptr<Entity> create(Args&&... args) {
        static_assert(std::is_base_of_v<Entity, T>, "T must derive from Entity");
        return std::make_unique<T>(std::forward<Args>(args)...);
    }

    static std::unique_ptr<Entity> createFromConfig(const EntityConfig& config);
    static std::unique_ptr<Entity> createPlanet(const PlanetParams& params);
    static std::unique_ptr<Entity> createFleet(const FleetParams& params);
};
```

**Advanced Features:**

- Template-based creation with perfect forwarding
- Compile-time type checking with `static_assert`
- Configuration-driven creation
- Smart pointer returns for automatic memory management

**Benefits in Context:**

- Type safety at compile time
- Zero-overhead abstraction through templates
- Exception-safe object construction
- Simplified client code

### Builder Pattern - FleetBuilder

**Problem Solved:**
Fleet creation involves complex configuration with many optional components (ships, weapons, shields, navigation systems) that can be combined in various ways.

**Why Builder Here:**

- **Complex Construction**: Fleets have many optional components
- **Step-by-Step Assembly**: Logical construction sequence (ships → weapons → shields)
- **Multiple Representations**: Different fleet types (military, exploration, cargo)
- **Validation**: Each step can validate the current configuration

**C++ Implementation Strategy:**

```cpp
class FleetBuilder {
private:
    std::unique_ptr<Fleet> fleet_;

public:
    FleetBuilder& addShips(const std::vector<ShipType>& ships) {
        // Validation and addition logic
        return *this;
    }

    FleetBuilder& addWeapons(const WeaponConfiguration& weapons) {
        if (!fleet_->hasShips()) {
            throw std::logic_error("Cannot add weapons without ships");
        }
        // Addition logic
        return *this;
    }

    template<typename T>
    FleetBuilder& addComponent(T&& component) {
        static_assert(is_fleet_component_v<T>, "T must be a fleet component");
        fleet_->addComponent(std::forward<T>(component));
        return *this;
    }

    std::unique_ptr<Fleet> build() && {
        validateFleet();
        return std::move(fleet_);
    }
};
```

**Advanced C++ Features:**

- Fluent interface with method chaining
- Template methods for type-safe component addition
- Move-only build method (ref-qualified)
- RAII for resource management

**Benefits in Context:**

- Compile-time component type checking
- Impossible to create invalid fleets
- Self-documenting construction process
- Memory-efficient with move semantics

## Structural Patterns

### Adapter Pattern - NavigationAdapter

**Problem Solved:**
The system needs to integrate with legacy navigation systems that have incompatible interfaces while providing a modern, consistent API.

**Why Adapter Here:**

- **Legacy Integration**: Existing navigation code can't be modified
- **Interface Mismatch**: Legacy system uses different method names and parameters
- **Gradual Migration**: Allows incremental replacement of legacy components
- **API Consistency**: Provides uniform interface across all navigation systems

**C++ Implementation Design:**

```cpp
class ModernNavigationInterface {
public:
    virtual ~ModernNavigationInterface() = default;
    virtual Route calculateRoute(const Coordinates& from, const Coordinates& to) = 0;
    virtual void setNavigationMode(NavigationMode mode) = 0;
};

class LegacyNavigationAdapter : public ModernNavigationInterface {
private:
    std::unique_ptr<LegacyNavSystem> legacy_system_;

public:
    explicit LegacyNavigationAdapter(std::unique_ptr<LegacyNavSystem> legacy)
        : legacy_system_(std::move(legacy)) {}

    Route calculateRoute(const Coordinates& from, const Coordinates& to) override {
        // Convert modern parameters to legacy format
        LegacyCoords legacy_from = convertCoordinates(from);
        LegacyCoords legacy_to = convertCoordinates(to);

        // Call legacy method
        LegacyPath path = legacy_system_->computePath(legacy_from, legacy_to);

        // Convert legacy result to modern format
        return convertPath(path);
    }
};
```

**Key Design Decisions:**

- **Composition over Inheritance**: Adapter owns legacy system
- **Smart Pointer Management**: Automatic cleanup of legacy resources
- **Exception Translation**: Converts legacy exceptions to modern ones
- **Performance Optimization**: Minimal conversion overhead

**Benefits in Context:**

- Seamless legacy integration
- Type-safe interface conversion
- Resource management handled automatically
- Future legacy system replacement simplified

### Decorator Pattern - MissionDecorator

**Problem Solved:**
Missions need additional capabilities (stealth, armor, priority) that can be combined in various ways without creating a class explosion.

**Why Decorator Here:**

- **Runtime Enhancement**: Mission capabilities determined at runtime
- **Combinatorial Explosion**: Many possible combinations of enhancements
- **Single Responsibility**: Each decorator adds one specific capability
- **Dynamic Composition**: Decorators can be added/removed during execution

**C++ Implementation Approach:**

```cpp
class Mission {
public:
    virtual ~Mission() = default;
    virtual MissionResult execute() = 0;
    virtual double getCost() const = 0;
    virtual Duration getEstimatedTime() const = 0;
};

class MissionDecorator : public Mission {
protected:
    std::unique_ptr<Mission> mission_;

public:
    explicit MissionDecorator(std::unique_ptr<Mission> mission)
        : mission_(std::move(mission)) {}

    MissionResult execute() override {
        return mission_->execute();
    }

    double getCost() const override {
        return mission_->getCost();
    }
};

class StealthMissionDecorator : public MissionDecorator {
public:
    using MissionDecorator::MissionDecorator;

    MissionResult execute() override {
        enableStealth();
        auto result = MissionDecorator::execute();
        disableStealth();
        return result;
    }

    double getCost() const override {
        return MissionDecorator::getCost() * 1.3; // 30% stealth cost increase
    }
};
```

**Advanced Features:**

- **RAII for Decorations**: Automatic cleanup in destructors
- **Exception Safety**: Strong exception guarantee through RAII
- **Perfect Forwarding**: Constructor delegation for efficiency
- **Type Erasure**: All decorators work with base Mission interface

**Benefits in Context:**

- Unlimited combination possibilities
- Runtime configuration of mission capabilities
- Memory efficient with move semantics
- Exception-safe decoration management

## Behavioral Patterns

### Observer Pattern - PlanetMonitoring

**Problem Solved:**
Multiple systems need to react to planet state changes (resource depletion, population changes, threat detection) without tight coupling between components.

**Why Observer Here:**

- **Event-Driven Architecture**: Many systems react to planet changes
- **Loose Coupling**: Planets don't need to know about all dependent systems
- **Dynamic Subscription**: Systems can subscribe/unsubscribe at runtime
- **Broadcast Communication**: One event reaches multiple subscribers

**C++ Implementation Strategy:**

```cpp
template<typename EventType>
class Observable {
private:
    mutable std::shared_mutex observers_mutex_;
    std::vector<std::weak_ptr<Observer<EventType>>> observers_;

    void cleanupExpiredObservers() const {
        std::unique_lock lock(observers_mutex_);
        observers_.erase(
            std::remove_if(observers_.begin(), observers_.end(),
                [](const auto& weak_obs) { return weak_obs.expired(); }),
            observers_.end());
    }

public:
    void addObserver(std::shared_ptr<Observer<EventType>> observer) {
        std::unique_lock lock(observers_mutex_);
        observers_.emplace_back(observer);
    }

    void notifyObservers(const EventType& event) const {
        std::shared_lock lock(observers_mutex_);
        cleanupExpiredObservers();

        for (const auto& weak_obs : observers_) {
            if (auto obs = weak_obs.lock()) {
                obs->onNotify(event);
            }
        }
    }
};
```

**Advanced C++ Features:**

- **Weak Pointers**: Prevents circular dependencies and memory leaks
- **Template-Based**: Type-safe events with compile-time checking
- **Thread-Safe**: Reader-writer locks for concurrent access
- **Automatic Cleanup**: Expired observers removed automatically

**Benefits in Context:**

- Memory leak prevention through weak_ptr
- Thread-safe notifications
- Type safety for events
- Automatic observer lifecycle management

### Strategy Pattern - PathfindingStrategy

**Problem Solved:**
Navigation systems need different pathfinding algorithms (direct route, safe route, fuel-efficient route) that can be selected at runtime based on mission requirements and environmental conditions.

**Why Strategy Here:**

- **Algorithm Variation**: Multiple valid pathfinding approaches exist
- **Runtime Selection**: Algorithm choice depends on mission parameters
- **Easy Extension**: New algorithms can be added without modifying existing code
- **Performance Optimization**: Different algorithms for different scenarios

**C++ Implementation Design:**

```cpp
class PathfindingStrategy {
public:
    virtual ~PathfindingStrategy() = default;
    virtual Route findPath(const Coordinates& start, const Coordinates& end,
                          const NavigationContext& context) = 0;
    virtual double estimateCost(const Coordinates& start, const Coordinates& end) const = 0;
};

class NavigationSystem {
private:
    std::unique_ptr<PathfindingStrategy> strategy_;

public:
    template<typename StrategyType, typename... Args>
    void setStrategy(Args&&... args) {
        static_assert(std::is_base_of_v<PathfindingStrategy, StrategyType>,
                     "StrategyType must inherit from PathfindingStrategy");
        strategy_ = std::make_unique<StrategyType>(std::forward<Args>(args)...);
    }

    Route calculateRoute(const Coordinates& start, const Coordinates& end) {
        if (!strategy_) {
            throw std::logic_error("No pathfinding strategy set");
        }
        return strategy_->findPath(start, end, getCurrentContext());
    }
};

// Strategy implementations
class DirectPathStrategy : public PathfindingStrategy {
    Route findPath(const Coordinates& start, const Coordinates& end,
                  const NavigationContext& context) override {
        return Route::createDirectPath(start, end);
    }
};

class SafePathStrategy : public PathfindingStrategy {
    Route findPath(const Coordinates& start, const Coordinates& end,
                  const NavigationContext& context) override {
        auto threats = context.getKnownThreats();
        return findPathAvoidingThreats(start, end, threats);
    }
};
```

**Advanced Features:**

- **Template-based Strategy Setting**: Type-safe strategy creation
- **Perfect Forwarding**: Efficient parameter passing to strategy constructors
- **Context-Aware**: Strategies receive environmental information
- **Null Object Protection**: Validation prevents null strategy usage

**Benefits in Context:**

- Zero runtime overhead for strategy calls (virtual function optimization)
- Easy testing of individual algorithms
- Runtime algorithm selection based on conditions
- Clean separation between algorithm and usage

### Command Pattern - FleetCommands

**Problem Solved:**
Fleet operations need to be encapsulated as objects to support queuing, logging, undo operations, and macro commands composed of multiple actions.

**Why Command Here:**

- **Action Encapsulation**: Operations become first-class objects
- **Undo/Redo Support**: Commands can reverse their effects
- **Macro Commands**: Complex operations built from simple ones
- **Logging and Persistence**: Commands can be serialized and replayed

**C++ Implementation Strategy:**

```cpp
class Command {
public:
    virtual ~Command() = default;
    virtual void execute() = 0;
    virtual void undo() = 0;
    virtual bool canUndo() const { return true; }
    virtual std::string getDescription() const = 0;
};

class MoveFleetCommand : public Command {
private:
    std::weak_ptr<Fleet> fleet_;
    Coordinates from_position_;
    Coordinates to_position_;
    bool executed_{false};

public:
    MoveFleetCommand(std::shared_ptr<Fleet> fleet, const Coordinates& to)
        : fleet_(fleet), to_position_(to) {
        if (auto f = fleet_.lock()) {
            from_position_ = f->getPosition();
        }
    }

    void execute() override {
        auto fleet = fleet_.lock();
        if (!fleet) {
            throw std::runtime_error("Fleet no longer exists");
        }

        fleet->moveTo(to_position_);
        executed_ = true;
    }

    void undo() override {
        if (!executed_) return;

        auto fleet = fleet_.lock();
        if (!fleet) {
            throw std::runtime_error("Cannot undo: Fleet no longer exists");
        }

        fleet->moveTo(from_position_);
        executed_ = false;
    }

    std::string getDescription() const override {
        return "Move fleet to " + to_position_.toString();
    }
};

class CommandProcessor {
private:
    std::stack<std::unique_ptr<Command>> undo_stack_;
    std::stack<std::unique_ptr<Command>> redo_stack_;

public:
    void execute(std::unique_ptr<Command> command) {
        try {
            command->execute();
            undo_stack_.push(std::move(command));
            // Clear redo stack when new command is executed
            while (!redo_stack_.empty()) {
                redo_stack_.pop();
            }
        } catch (...) {
            // Command failed, don't add to undo stack
            throw;
        }
    }

    void undo() {
        if (undo_stack_.empty()) return;

        auto command = std::move(undo_stack_.top());
        undo_stack_.pop();

        command->undo();
        redo_stack_.push(std::move(command));
    }

    void redo() {
        if (redo_stack_.empty()) return;

        auto command = std::move(redo_stack_.top());
        redo_stack_.pop();

        command->execute();
        undo_stack_.push(std::move(command));
    }
};
```

**Advanced Features:**

- **RAII for State Management**: Exception-safe command execution
- **Weak Pointers**: Commands don't keep objects alive unnecessarily
- **Exception Safety**: Failed commands aren't added to undo stack
- **Move Semantics**: Efficient command transfer between stacks

**Benefits in Context:**

- Full undo/redo functionality
- Command queuing and batching
- Audit trail of all operations
- Macro command composition

## Pattern Interactions and Combinations

### Factory + Strategy Combination

The EntityFactory uses Strategy pattern internally to determine creation algorithms:

```cpp
class EntityFactory {
    static std::unique_ptr<CreationStrategy> getCreationStrategy(EntityType type) {
        switch (type) {
            case EntityType::Planet:
                return std::make_unique<PlanetCreationStrategy>();
            case EntityType::Fleet:
                return std::make_unique<FleetCreationStrategy>();
        }
    }
};
```

### Observer + Command Integration

Commands can trigger observer notifications:

```cpp
class NotifyingCommand : public Command {
    void execute() override {
        Command::execute();
        notifyObservers(CommandExecutedEvent{*this});
    }
};
```

### Decorator + Strategy Layering

Mission decorators can change the strategy used:

```cpp
class StrategyChangingDecorator : public MissionDecorator {
public:
    MissionResult execute() override {
        auto original_strategy = mission_->getNavigationStrategy();
        mission_->setNavigationStrategy(std::make_unique<StealthPathStrategy>());

        auto result = MissionDecorator::execute();

        mission_->setNavigationStrategy(std::move(original_strategy));
        return result;
    }
};
```

## Performance Considerations

### Compile-Time Optimizations

- Template-based patterns use zero-cost abstractions
- constexpr pattern implementations where possible
- Template specialization for performance-critical paths

### Runtime Optimizations

- Object pooling for frequently created command objects
- Strategy caching to avoid repeated algorithm setup
- Observer notification batching to reduce overhead

### Memory Management

- Smart pointers prevent memory leaks in all patterns
- Weak pointers break cycles in Observer pattern
- Move semantics minimize copying in Command pattern

## Testing Strategy for Patterns

### Unit Testing Patterns

Each pattern implementation includes comprehensive tests:

```cpp
TEST_CASE("Singleton thread safety", "[patterns][singleton]") {
    std::vector<std::thread> threads;
    std::vector<ResourceManager*> instances;
    std::mutex instances_mutex;

    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&]() {
            auto& instance = ResourceManager::getInstance();
            std::lock_guard lock(instances_mutex);
            instances.push_back(&instance);
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // All instances should be the same
    for (size_t i = 1; i < instances.size(); ++i) {
        REQUIRE(instances[i] == instances[0]);
    }
}
```

### Integration Testing

Tests verify pattern interactions work correctly:

```cpp
TEST_CASE("Factory creates objects observable by observer", "[integration]") {
    auto planet = EntityFactory::createPlanet(PlanetParams{});
    auto observer = std::make_shared<MockPlanetObserver>();

    planet->addObserver(observer);
    planet->changeResourceLevel(100);

    REQUIRE(observer->wasNotified());
}
```

## Pattern Documentation Standards

### Code Comments

Every pattern implementation includes:

- Purpose and problem solved
- Key design decisions
- Usage examples
- Performance characteristics
- Thread safety guarantees

### API Documentation

Doxygen documentation for all pattern interfaces:

```cpp
/**
 * @brief Abstract base for all pathfinding strategies
 *
 * This class defines the Strategy pattern interface for pathfinding
 * algorithms. Concrete strategies implement different approaches to
 * finding routes between coordinates.
 *
 * @note All implementations must be thread-safe for concurrent access
 * @see DirectPathStrategy, SafePathStrategy, EfficientPathStrategy
 */
class PathfindingStrategy {
    /**
     * @brief Find path between two coordinates
     * @param start Starting coordinates
     * @param end Destination coordinates
     * @param context Navigation context with environmental data
     * @return Route object representing the calculated path
     * @throws NavigationException if no valid path exists
     */
    virtual Route findPath(const Coordinates& start, const Coordinates& end,
                          const NavigationContext& context) = 0;
};
```

## Conclusion

The design patterns in CppVerseHub were chosen based on real architectural needs rather than just educational demonstration. Each pattern:

1. **Solves a Specific Problem**: Addresses actual design challenges in the simulation system
2. **Leverages C++ Features**: Uses modern C++ capabilities for efficient implementation
3. **Maintains Performance**: Patterns don't add unnecessary overhead
4. **Supports Evolution**: Architecture can grow and change over time
5. **Educational Value**: Demonstrates patterns in realistic contexts

The combination of these patterns creates a flexible, maintainable system that showcases advanced C++ programming techniques while solving real-world software architecture problems.

## Pattern Summary Table

| Pattern   | Primary Use Case            | Key C++ Features                   | Performance Impact       | Educational Value             |
| --------- | --------------------------- | ---------------------------------- | ------------------------ | ----------------------------- |
| Singleton | Global resource management  | `std::call_once`, smart pointers   | Minimal                  | Thread safety, initialization |
| Factory   | Runtime entity creation     | Templates, perfect forwarding      | Zero overhead            | Type safety, extensibility    |
| Builder   | Complex object construction | Fluent interface, move semantics   | Efficient                | Step validation, flexibility  |
| Adapter   | Legacy integration          | Composition, exception translation | Minimal conversion cost  | Interface design              |
| Decorator | Runtime enhancement         | RAII, inheritance                  | Dynamic composition      | Responsibility separation     |
| Observer  | Event notification          | Weak pointers, templates           | Lock-free possible       | Loose coupling                |
| Strategy  | Algorithm selection         | Virtual functions, templates       | Optimizable              | Algorithm encapsulation       |
| Command   | Action encapsulation        | Move semantics, RAII               | Command objects overhead | Undo/redo, queuing            |

This comprehensive approach ensures that every pattern serves both educational and practical purposes while maintaining the high performance standards expected from modern C++ applications.
