// File: examples/tutorials/design_patterns_tutorial.cpp
// CppVerseHub Design Patterns Learning Tutorial
// Learn Gang of Four design patterns through space exploration scenarios

#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <map>
#include <functional>
#include <algorithm>
#include <queue>
#include <mutex>
#include <thread>

using namespace std;

/**
 * PATTERN 1: SINGLETON PATTERN
 * Ensures only one instance exists - perfect for mission control centers
 */
namespace Pattern1_Singleton {
    
    class MissionControl {
    private:
        static unique_ptr<MissionControl> instance;
        static mutex instanceMutex;
        
        string controllerName;
        int activeMissions = 0;
        
        // Private constructor prevents external instantiation
        MissionControl(const string& name) : controllerName(name) {
            cout << "Mission Control Center '" << name << "' established" << endl;
        }
        
    public:
        // Delete copy constructor and assignment operator
        MissionControl(const MissionControl&) = delete;
        MissionControl& operator=(const MissionControl&) = delete;
        
        // Thread-safe singleton access
        static MissionControl& getInstance(const string& name = "Houston") {
            lock_guard<mutex> lock(instanceMutex);
            if (!instance) {
                instance = unique_ptr<MissionControl>(new MissionControl(name));
            }
            return *instance;
        }
        
        void startMission(const string& missionName) {
            activeMissions++;
            cout << controllerName << ": Starting mission '" << missionName 
                 << "' (Total active: " << activeMissions << ")" << endl;
        }
        
        void completeMission(const string& missionName) {
            if (activeMissions > 0) {
                activeMissions--;
                cout << controllerName << ": Mission '" << missionName 
                     << "' completed (Remaining: " << activeMissions << ")" << endl;
            }
        }
        
        int getActiveMissionCount() const { return activeMissions; }
        const string& getName() const { return controllerName; }
    };
    
    // Static member definitions
    unique_ptr<MissionControl> MissionControl::instance = nullptr;
    mutex MissionControl::instanceMutex;
    
    void demonstrateSingleton() {
        cout << "\n=== PATTERN 1: Singleton Pattern ===" << endl;
        cout << "Ensuring only one Mission Control Center exists" << endl;
        
        // Multiple calls return the same instance
        auto& control1 = MissionControl::getInstance("Houston");
        auto& control2 = MissionControl::getInstance("Cape Kennedy");  // Name ignored - same instance
        
        cout << "Control1 name: " << control1.getName() << endl;
        cout << "Control2 name: " << control2.getName() << endl;
        cout << "Same instance? " << (&control1 == &control2 ? "Yes" : "No") << endl;
        
        control1.startMission("Apollo 11");
        control1.startMission("Voyager 1");
        
        cout << "Active missions via control2: " << control2.getActiveMissionCount() << endl;
        
        control2.completeMission("Apollo 11");
        
        cout << "--- Singleton Pattern Complete ---" << endl;
    }
}

/**
 * PATTERN 2: FACTORY PATTERN
 * Creates objects without specifying exact classes - spacecraft manufacturing
 */
namespace Pattern2_Factory {
    
    // Abstract product
    class Spacecraft {
    public:
        virtual ~Spacecraft() = default;
        virtual void describe() const = 0;
        virtual double getMaxSpeed() const = 0;
        virtual string getType() const = 0;
    };
    
    // Concrete products
    class Scout : public Spacecraft {
    public:
        void describe() const override {
            cout << "Fast scout vessel for reconnaissance missions" << endl;
        }
        double getMaxSpeed() const override { return 0.8; }  // 80% light speed
        string getType() const override { return "Scout"; }
    };
    
    class Freighter : public Spacecraft {
    public:
        void describe() const override {
            cout << "Heavy cargo vessel for transport missions" << endl;
        }
        double getMaxSpeed() const override { return 0.3; }  // 30% light speed
        string getType() const override { return "Freighter"; }
    };
    
    class Warship : public Spacecraft {
    public:
        void describe() const override {
            cout << "Armed combat vessel for military operations" << endl;
        }
        double getMaxSpeed() const override { return 0.6; }  // 60% light speed
        string getType() const override { return "Warship"; }
    };
    
    // Factory class
    class SpacecraftFactory {
    public:
        enum class SpacecraftType { SCOUT, FREIGHTER, WARSHIP };
        
        static unique_ptr<Spacecraft> createSpacecraft(SpacecraftType type) {
            switch (type) {
                case SpacecraftType::SCOUT:
                    cout << "Manufacturing scout vessel..." << endl;
                    return make_unique<Scout>();
                    
                case SpacecraftType::FREIGHTER:
                    cout << "Manufacturing freighter..." << endl;
                    return make_unique<Freighter>();
                    
                case SpacecraftType::WARSHIP:
                    cout << "Manufacturing warship..." << endl;
                    return make_unique<Warship>();
                    
                default:
                    cout << "Unknown spacecraft type requested" << endl;
                    return nullptr;
            }
        }
        
        // Factory method with string parameter
        static unique_ptr<Spacecraft> createSpacecraft(const string& typeName) {
            if (typeName == "scout") {
                return createSpacecraft(SpacecraftType::SCOUT);
            } else if (typeName == "freighter") {
                return createSpacecraft(SpacecraftType::FREIGHTER);
            } else if (typeName == "warship") {
                return createSpacecraft(SpacecraftType::WARSHIP);
            }
            return nullptr;
        }
    };
    
    void demonstrateFactory() {
        cout << "\n=== PATTERN 2: Factory Pattern ===" << endl;
        cout << "Creating different spacecraft types without knowing exact classes" << endl;
        
        // Create spacecraft using enum
        auto scout = SpacecraftFactory::createSpacecraft(SpacecraftFactory::SpacecraftType::SCOUT);
        auto freighter = SpacecraftFactory::createSpacecraft(SpacecraftFactory::SpacecraftType::FREIGHTER);
        
        // Create spacecraft using string
        auto warship = SpacecraftFactory::createSpacecraft("warship");
        
        cout << "\nFleet roster:" << endl;
        vector<unique_ptr<Spacecraft>> fleet;
        fleet.push_back(move(scout));
        fleet.push_back(move(freighter));
        fleet.push_back(move(warship));
        
        for (const auto& ship : fleet) {
            if (ship) {
                cout << "- " << ship->getType() << " (Max speed: " 
                     << ship->getMaxSpeed() << "c): ";
                ship->describe();
            }
        }
        
        cout << "--- Factory Pattern Complete ---" << endl;
    }
}

/**
 * PATTERN 3: OBSERVER PATTERN
 * Notifies multiple objects about state changes - mission status updates
 */
namespace Pattern3_Observer {
    
    // Forward declaration
    class MissionStatus;
    
    // Observer interface
    class StatusObserver {
    public:
        virtual ~StatusObserver() = default;
        virtual void onStatusUpdate(const MissionStatus& mission) = 0;
    };
    
    // Subject (Observable)
    class MissionStatus {
    private:
        string missionName;
        string status;
        int progress;  // 0-100%
        vector<StatusObserver*> observers;
        
    public:
        MissionStatus(const string& name) : missionName(name), status("Preparing"), progress(0) {}
        
        void addObserver(StatusObserver* observer) {
            observers.push_back(observer);
            cout << "Observer added to mission '" << missionName << "'" << endl;
        }
        
        void removeObserver(StatusObserver* observer) {
            observers.erase(remove(observers.begin(), observers.end(), observer), observers.end());
            cout << "Observer removed from mission '" << missionName << "'" << endl;
        }
        
        void setStatus(const string& newStatus, int newProgress) {
            status = newStatus;
            progress = newProgress;
            notifyObservers();
        }
        
        // Getters
        const string& getMissionName() const { return missionName; }
        const string& getStatus() const { return status; }
        int getProgress() const { return progress; }
        
    private:
        void notifyObservers() {
            cout << "Notifying " << observers.size() << " observers of status change" << endl;
            for (auto* observer : observers) {
                observer->onStatusUpdate(*this);
            }
        }
    };
    
    // Concrete observers
    class GroundControl : public StatusObserver {
    private:
        string stationName;
        
    public:
        GroundControl(const string& name) : stationName(name) {}
        
        void onStatusUpdate(const MissionStatus& mission) override {
            cout << "[" << stationName << "] Mission '" << mission.getMissionName() 
                 << "' status: " << mission.getStatus() 
                 << " (" << mission.getProgress() << "% complete)" << endl;
        }
    };
    
    class CrewFamilies : public StatusObserver {
    public:
        void onStatusUpdate(const MissionStatus& mission) override {
            cout << "[Crew Families] Received update: " << mission.getMissionName() 
                 << " is " << mission.getStatus();
            
            if (mission.getProgress() >= 100) {
                cout << " - Welcome home! 🏠";
            } else if (mission.getStatus() == "In Progress") {
                cout << " - Stay safe! 🚀";
            }
            cout << endl;
        }
    };
    
    class MediaCenter : public StatusObserver {
    public:
        void onStatusUpdate(const MissionStatus& mission) override {
            if (mission.getProgress() % 25 == 0 || mission.getProgress() >= 100) {
                cout << "[Media Center] BREAKING: " << mission.getMissionName() 
                     << " mission " << mission.getStatus() 
                     << " (" << mission.getProgress() << "% complete)" << endl;
            }
        }
    };
    
    void demonstrateObserver() {
        cout << "\n=== PATTERN 3: Observer Pattern ===" << endl;
        cout << "Multiple parties observing mission status changes" << endl;
        
        // Create mission
        MissionStatus apolloMission("Apollo 11");
        
        // Create observers
        GroundControl houston("Houston Control");
        CrewFamilies families;
        MediaCenter media;
        
        // Register observers
        apolloMission.addObserver(&houston);
        apolloMission.addObserver(&families);
        apolloMission.addObserver(&media);
        
        cout << "\n--- Mission Progress Updates ---" << endl;
        
        // Simulate mission progress
        apolloMission.setStatus("Launch", 0);
        this_thread::sleep_for(chrono::milliseconds(500));
        
        apolloMission.setStatus("In Transit", 25);
        this_thread::sleep_for(chrono::milliseconds(500));
        
        apolloMission.setStatus("Lunar Orbit", 50);
        this_thread::sleep_for(chrono::milliseconds(500));
        
        apolloMission.setStatus("Moon Landing", 75);
        this_thread::sleep_for(chrono::milliseconds(500));
        
        apolloMission.setStatus("Mission Complete", 100);
        
        cout << "--- Observer Pattern Complete ---" << endl;
    }
}

/**
 * PATTERN 4: STRATEGY PATTERN
 * Encapsulates algorithms and makes them interchangeable - navigation systems
 */
namespace Pattern4_Strategy {
    
    struct Coordinate {
        double x, y, z;
        Coordinate(double x = 0, double y = 0, double z = 0) : x(x), y(y), z(z) {}
        
        double distanceTo(const Coordinate& other) const {
            double dx = x - other.x;
            double dy = y - other.y;
            double dz = z - other.z;
            return sqrt(dx*dx + dy*dy + dz*dz);
        }
        
        void print() const {
            cout << "(" << x << ", " << y << ", " << z << ")";
        }
    };
    
    // Strategy interface
    class NavigationStrategy {
    public:
        virtual ~NavigationStrategy() = default;
        virtual vector<Coordinate> calculateRoute(const Coordinate& start, 
                                                const Coordinate& destination) = 0;
        virtual string getName() const = 0;
    };
    
    // Concrete strategies
    class DirectRoute : public NavigationStrategy {
    public:
        vector<Coordinate> calculateRoute(const Coordinate& start, 
                                        const Coordinate& destination) override {
            cout << "Calculating direct route..." << endl;
            return {start, destination};  // Straight line
        }
        
        string getName() const override { return "Direct Route"; }
    };
    
    class EconomicalRoute : public NavigationStrategy {
    public:
        vector<Coordinate> calculateRoute(const Coordinate& start, 
                                        const Coordinate& destination) override {
            cout << "Calculating fuel-efficient route..." << endl;
            
            // Add intermediate point for fuel efficiency
            Coordinate midpoint(
                (start.x + destination.x) / 2,
                (start.y + destination.y) / 2 - 5,  // Dip down to use gravity
                (start.z + destination.z) / 2
            );
            
            return {start, midpoint, destination};
        }
        
        string getName() const override { return "Economical Route"; }
    };
    
    class SafeRoute : public NavigationStrategy {
    public:
        vector<Coordinate> calculateRoute(const Coordinate& start, 
                                        const Coordinate& destination) override {
            cout << "Calculating safe route avoiding hazards..." << endl;
            
            // Route around potential hazards
            vector<Coordinate> route;
            route.push_back(start);
            
            // Add safety waypoints
            Coordinate waypoint1(start.x + 2, start.y, start.z + 2);
            Coordinate waypoint2(destination.x - 2, destination.y, destination.z + 2);
            
            route.push_back(waypoint1);
            route.push_back(waypoint2);
            route.push_back(destination);
            
            return route;
        }
        
        string getName() const override { return "Safe Route"; }
    };
    
    // Context class
    class SpaceNavigator {
    private:
        unique_ptr<NavigationStrategy> strategy;
        
    public:
        SpaceNavigator(unique_ptr<NavigationStrategy> nav_strategy) 
            : strategy(move(nav_strategy)) {}
        
        void setStrategy(unique_ptr<NavigationStrategy> new_strategy) {
            strategy = move(new_strategy);
            cout << "Navigation strategy changed to: " << strategy->getName() << endl;
        }
        
        void navigate(const Coordinate& from, const Coordinate& to) {
            cout << "\n--- Navigation Request ---" << endl;
            cout << "From: "; from.print(); cout << " To: "; to.print(); cout << endl;
            cout << "Using strategy: " << strategy->getName() << endl;
            
            auto route = strategy->calculateRoute(from, to);
            
            cout << "Calculated route (" << route.size() << " waypoints):" << endl;
            for (size_t i = 0; i < route.size(); ++i) {
                cout << "  " << (i + 1) << ". ";
                route[i].print();
                cout << endl;
            }
            
            // Calculate total distance
            double totalDistance = 0;
            for (size_t i = 1; i < route.size(); ++i) {
                totalDistance += route[i-1].distanceTo(route[i]);
            }
            cout << "Total distance: " << totalDistance << " units" << endl;
        }
    };
    
    void demonstrateStrategy() {
        cout << "\n=== PATTERN 4: Strategy Pattern ===" << endl;
        cout << "Different navigation algorithms for space travel" << endl;
        
        Coordinate earth(0, 0, 0);
        Coordinate mars(50, 30, 10);
        
        // Create navigator with direct route strategy
        SpaceNavigator navigator(make_unique<DirectRoute>());
        navigator.navigate(earth, mars);
        
        // Change to economical route
        navigator.setStrategy(make_unique<EconomicalRoute>());
        navigator.navigate(earth, mars);
        
        // Change to safe route
        navigator.setStrategy(make_unique<SafeRoute>());
        navigator.navigate(earth, mars);
        
        cout << "--- Strategy Pattern Complete ---" << endl;
    }
}

/**
 * PATTERN 5: COMMAND PATTERN
 * Encapsulates requests as objects - spacecraft control systems
 */
namespace Pattern5_Command {
    
    // Receiver - the object that performs the actual work
    class Spacecraft {
    private:
        string name;
        double x, y, z;  // Position
        double speed;
        bool enginesOn;
        
    public:
        Spacecraft(const string& spacecraft_name) 
            : name(spacecraft_name), x(0), y(0), z(0), speed(0), enginesOn(false) {}
        
        void startEngines() {
            enginesOn = true;
            cout << name << ": Engines started" << endl;
        }
        
        void stopEngines() {
            enginesOn = false;
            speed = 0;
            cout << name << ": Engines stopped" << endl;
        }
        
        void accelerate(double deltaV) {
            if (enginesOn) {
                speed += deltaV;
                cout << name << ": Accelerated by " << deltaV 
                     << " (Current speed: " << speed << ")" << endl;
            } else {
                cout << name << ": Cannot accelerate - engines off" << endl;
            }
        }
        
        void moveToPosition(double newX, double newY, double newZ) {
            x = newX; y = newY; z = newZ;
            cout << name << ": Moved to position (" << x << ", " << y << ", " << z << ")" << endl;
        }
        
        void getStatus() const {
            cout << name << " Status: Position(" << x << ", " << y << ", " << z 
                 << "), Speed(" << speed << "), Engines(" 
                 << (enginesOn ? "ON" : "OFF") << ")" << endl;
        }
        
        const string& getName() const { return name; }
    };
    
    // Command interface
    class Command {
    public:
        virtual ~Command() = default;
        virtual void execute() = 0;
        virtual void undo() = 0;
        virtual string getDescription() const = 0;
    };
    
    // Concrete commands
    class StartEnginesCommand : public Command {
    private:
        Spacecraft* spacecraft;
        
    public:
        StartEnginesCommand(Spacecraft* ship) : spacecraft(ship) {}
        
        void execute() override {
            spacecraft->startEngines();
        }
        
        void undo() override {
            spacecraft->stopEngines();
        }
        
        string getDescription() const override {
            return "Start engines on " + spacecraft->getName();
        }
    };
    
    class AccelerateCommand : public Command {
    private:
        Spacecraft* spacecraft;
        double deltaV;
        
    public:
        AccelerateCommand(Spacecraft* ship, double acceleration) 
            : spacecraft(ship), deltaV(acceleration) {}
        
        void execute() override {
            spacecraft->accelerate(deltaV);
        }
        
        void undo() override {
            spacecraft->accelerate(-deltaV);  // Decelerate
        }
        
        string getDescription() const override {
            return "Accelerate " + spacecraft->getName() + " by " + to_string(deltaV);
        }
    };
    
    class MoveCommand : public Command {
    private:
        Spacecraft* spacecraft;
        double newX, newY, newZ;
        double oldX, oldY, oldZ;  // For undo
        
    public:
        MoveCommand(Spacecraft* ship, double x, double y, double z) 
            : spacecraft(ship), newX(x), newY(y), newZ(z) {}
        
        void execute() override {
            // Store current position for undo
            // In real implementation, we'd get current position from spacecraft
            oldX = oldY = oldZ = 0;  // Simplified
            spacecraft->moveToPosition(newX, newY, newZ);
        }
        
        void undo() override {
            spacecraft->moveToPosition(oldX, oldY, oldZ);
        }
        
        string getDescription() const override {
            return "Move " + spacecraft->getName() + " to (" + 
                   to_string(newX) + ", " + to_string(newY) + ", " + to_string(newZ) + ")";
        }
    };
    
    // Macro command - combines multiple commands
    class MacroCommand : public Command {
    private:
        vector<unique_ptr<Command>> commands;
        string description;
        
    public:
        MacroCommand(const string& desc) : description(desc) {}
        
        void addCommand(unique_ptr<Command> command) {
            commands.push_back(move(command));
        }
        
        void execute() override {
            cout << "Executing macro: " << description << endl;
            for (auto& command : commands) {
                command->execute();
            }
        }
        
        void undo() override {
            cout << "Undoing macro: " << description << endl;
            // Undo in reverse order
            for (auto it = commands.rbegin(); it != commands.rend(); ++it) {
                (*it)->undo();
            }
        }
        
        string getDescription() const override {
            return "Macro: " + description + " (" + to_string(commands.size()) + " commands)";
        }
    };
    
    // Invoker - executes commands and maintains history
    class MissionControl {
    private:
        queue<unique_ptr<Command>> commandQueue;
        vector<unique_ptr<Command>> commandHistory;
        
    public:
        void queueCommand(unique_ptr<Command> command) {
            cout << "Queued: " << command->getDescription() << endl;
            commandQueue.push(move(command));
        }
        
        void executeNextCommand() {
            if (!commandQueue.empty()) {
                auto command = move(commandQueue.front());
                commandQueue.pop();
                
                cout << "Executing: " << command->getDescription() << endl;
                command->execute();
                
                // Store in history for potential undo
                commandHistory.push_back(move(command));
            } else {
                cout << "No commands in queue" << endl;
            }
        }
        
        void executeAllCommands() {
            cout << "\n--- Executing All Queued Commands ---" << endl;
            while (!commandQueue.empty()) {
                executeNextCommand();
            }
        }
        
        void undoLastCommand() {
            if (!commandHistory.empty()) {
                auto& lastCommand = commandHistory.back();
                cout << "Undoing: " << lastCommand->getDescription() << endl;
                lastCommand->undo();
                commandHistory.pop_back();
            } else {
                cout << "No commands to undo" << endl;
            }
        }
        
        void printCommandHistory() const {
            cout << "\n--- Command History ---" << endl;
            for (size_t i = 0; i < commandHistory.size(); ++i) {
                cout << (i + 1) << ". " << commandHistory[i]->getDescription() << endl;
            }
        }
    };
    
    void demonstrateCommand() {
        cout << "\n=== PATTERN 5: Command Pattern ===" << endl;
        cout << "Encapsulating spacecraft operations as command objects" << endl;
        
        // Create spacecraft
        Spacecraft enterprise("USS Enterprise");
        enterprise.getStatus();
        
        // Create mission control
        MissionControl control;
        
        // Create and queue individual commands
        control.queueCommand(make_unique<StartEnginesCommand>(&enterprise));
        control.queueCommand(make_unique<AccelerateCommand>(&enterprise, 50.0));
        control.queueCommand(make_unique<AccelerateCommand>(&enterprise, 25.0));
        control.queueCommand(make_unique<MoveCommand>(&enterprise, 100, 50, 25));
        
        // Execute all commands
        control.executeAllCommands();
        
        enterprise.getStatus();
        control.printCommandHistory();
        
        // Demonstrate undo
        cout << "\n--- Undo Operations ---" << endl;
        control.undoLastCommand();
        control.undoLastCommand();
        
        enterprise.getStatus();
        
        // Demonstrate macro command
        cout << "\n--- Macro Command ---" << endl;
        auto launchSequence = make_unique<MacroCommand>("Launch Sequence");
        launchSequence->addCommand(make_unique<StartEnginesCommand>(&enterprise));
        launchSequence->addCommand(make_unique<AccelerateCommand>(&enterprise, 100.0));
        launchSequence->addCommand(make_unique<MoveCommand>(&enterprise, 0, 100, 0));
        
        control.queueCommand(move(launchSequence));
        control.executeNextCommand();
        
        enterprise.getStatus();
        
        cout << "--- Command Pattern Complete ---" << endl;
    }
}

/**
 * TUTORIAL ORCHESTRATOR
 */
void runDesignPatternsLearning() {
    cout << "🎯 Design Patterns Learning Tutorial 🎯" << endl;
    cout << "Learn Gang of Four patterns through space exploration!" << endl;
    cout << "====================================================" << endl;
    
    try {
        // Run all pattern demonstrations
        Pattern1_Singleton::demonstrateSingleton();
        Pattern2_Factory::demonstrateFactory();
        Pattern3_Observer::demonstrateObserver();
        Pattern4_Strategy::demonstrateStrategy();
        Pattern5_Command::demonstrateCommand();
        
        cout << "\n🎉 Design Patterns Tutorial Complete! 🎉" << endl;
        cout << "\nPatterns you've mastered:" << endl;
        cout << "✓ Singleton - Ensuring single instances (Mission Control)" << endl;
        cout << "✓ Factory - Creating objects without specifying classes (Spacecraft)" << endl;
        cout << "✓ Observer - Notifying multiple objects of changes (Mission Status)" << endl;
        cout << "✓ Strategy - Interchangeable algorithms (Navigation Routes)" << endl;
        cout << "✓ Command - Encapsulating requests as objects (Spacecraft Controls)" << endl;
        
        cout << "\nWhen to use each pattern:" << endl;
        cout << "• Singleton: Global access points, resource managers, logging" << endl;
        cout << "• Factory: Object creation without knowing exact types" << endl;
        cout << "• Observer: Event handling, model-view architectures" << endl;
        cout << "• Strategy: Multiple algorithms for same problem" << endl;
        cout << "• Command: Undo/redo, queuing operations, macro recording" << endl;
        
        cout << "\nNext steps:" << endl;
        cout << "• Study other behavioral patterns (State, Template Method)" << endl;
        cout << "• Explore structural patterns (Decorator, Adapter, Facade)" << endl;
        cout << "• Practice identifying patterns in existing codebases" << endl;
        cout << "• Try modern_cpp_features.cpp for advanced C++ techniques" << endl;
        
    } catch (const exception& e) {
        cout << "Tutorial error: " << e.what() << endl;
    }
}

/**
 * @brief Main function
 */
int main() {
    runDesignPatternsLearning();
    return 0;
}