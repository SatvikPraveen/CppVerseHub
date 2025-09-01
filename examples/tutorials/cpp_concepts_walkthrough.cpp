// File: examples/tutorials/cpp_concepts_walkthrough.cpp
// CppVerseHub C++ Concepts Learning Tutorial
// Learn C++ through practical examples in a space exploration context

#include <iostream>
#include <vector>
#include <memory>
#include <string>
#include <map>
#include <algorithm>
#include <functional>
#include <type_traits>
#include <concepts>
#include <ranges>
#include <optional>
#include <variant>

using namespace std;

/**
 * LESSON 1: OBJECT-ORIENTED PROGRAMMING FUNDAMENTALS
 * Learn classes, inheritance, and polymorphism through space entities
 */
namespace Lesson1_OOP {
    
    cout << "\n=== LESSON 1: Object-Oriented Programming ===" << endl;
    
    // Base class demonstrates encapsulation
    class SpaceObject {
    private:
        string name;          // Private data - encapsulation
        double mass;
        
    protected:
        int id;              // Protected - accessible to derived classes
        
    public:
        // Constructor with initialization list
        SpaceObject(const string& obj_name, double obj_mass) 
            : name(obj_name), mass(obj_mass), id(generateId()) {
            cout << "Created SpaceObject: " << name << endl;
        }
        
        // Virtual destructor for proper cleanup
        virtual ~SpaceObject() {
            cout << "Destroyed SpaceObject: " << name << endl;
        }
        
        // Pure virtual function - makes this an abstract class
        virtual void describe() const = 0;
        
        // Virtual function that can be overridden
        virtual double getGravitationalForce(const SpaceObject& other) const {
            const double G = 6.67430e-11;  // Gravitational constant
            return G * mass * other.mass;   // Simplified - missing distance
        }
        
        // Accessor methods (getters/setters)
        const string& getName() const { return name; }
        double getMass() const { return mass; }
        int getId() const { return id; }
        
        void setMass(double new_mass) { 
            if (new_mass > 0) mass = new_mass; 
        }
        
    private:
        static int nextId;
        static int generateId() { return ++nextId; }
    };
    
    int SpaceObject::nextId = 0;  // Static member initialization
    
    // Derived class - demonstrates inheritance
    class Planet : public SpaceObject {
    private:
        double radius;
        bool habitable;
        
    public:
        Planet(const string& name, double mass, double radius, bool is_habitable)
            : SpaceObject(name, mass), radius(radius), habitable(is_habitable) {
        }
        
        // Override pure virtual function
        void describe() const override {
            cout << "Planet " << getName() << ": Mass=" << getMass() 
                 << ", Radius=" << radius << ", Habitable=" << (habitable ? "Yes" : "No") << endl;
        }
        
        // Override virtual function with specialized behavior
        double getGravitationalForce(const SpaceObject& other) const override {
            // More sophisticated calculation for planets
            return SpaceObject::getGravitationalForce(other) * 1.1;  // 10% bonus
        }
        
        // Planet-specific methods
        bool isHabitable() const { return habitable; }
        double getRadius() const { return radius; }
        double getSurfaceGravity() const {
            const double G = 6.67430e-11;
            return G * getMass() / (radius * radius);
        }
    };
    
    class Spacecraft : public SpaceObject {
    private:
        string mission;
        double fuel;
        
    public:
        Spacecraft(const string& name, double mass, const string& mission_type, double fuel_amount)
            : SpaceObject(name, mass), mission(mission_type), fuel(fuel_amount) {
        }
        
        void describe() const override {
            cout << "Spacecraft " << getName() << ": Mission=" << mission 
                 << ", Fuel=" << fuel << " units" << endl;
        }
        
        void consumeFuel(double amount) {
            fuel = max(0.0, fuel - amount);
        }
        
        double getFuelLevel() const { return fuel; }
        const string& getMission() const { return mission; }
    };
    
    void demonstrateOOP() {
        cout << "\n--- OOP Concepts Demonstration ---" << endl;
        
        // Polymorphism - storing different types in same container
        vector<unique_ptr<SpaceObject>> spaceObjects;
        
        spaceObjects.push_back(make_unique<Planet>("Earth", 5.972e24, 6371000, true));
        spaceObjects.push_back(make_unique<Planet>("Mars", 6.39e23, 3389500, false));
        spaceObjects.push_back(make_unique<Spacecraft>("Voyager 1", 722.0, "Deep Space", 100.0));
        spaceObjects.push_back(make_unique<Spacecraft>("ISS", 420000.0, "Research", 0.0));
        
        cout << "\nAll space objects:" << endl;
        for (const auto& obj : spaceObjects) {
            obj->describe();  // Polymorphic call - different behavior based on actual type
        }
        
        // Demonstrate dynamic casting
        cout << "\nHabitable planets:" << endl;
        for (const auto& obj : spaceObjects) {
            // Try to cast to Planet - returns nullptr if not a Planet
            if (auto* planet = dynamic_cast<Planet*>(obj.get())) {
                if (planet->isHabitable()) {
                    cout << "  " << planet->getName() << " (Surface gravity: " 
                         << planet->getSurfaceGravity() << " m/s²)" << endl;
                }
            }
        }
        
        cout << "--- OOP Lesson Complete ---" << endl;
    }
}

/**
 * LESSON 2: STL AND GENERIC PROGRAMMING
 * Learn containers, algorithms, and iterators through fleet management
 */
namespace Lesson2_STL {
    
    struct Ship {
        string name;
        string type;
        int strength;
        bool operational;
        
        Ship(const string& n, const string& t, int s) 
            : name(n), type(t), strength(s), operational(true) {}
    };
    
    void demonstrateContainers() {
        cout << "\n=== LESSON 2: STL Containers and Algorithms ===" << endl;
        cout << "\n--- STL Containers ---" << endl;
        
        // Vector - dynamic array
        vector<Ship> fleet = {
            {"Enterprise", "Explorer", 500},
            {"Defiant", "Warship", 800},
            {"Voyager", "Explorer", 400},
            {"Prometheus", "Warship", 750},
            {"Discovery", "Science", 300}
        };
        
        cout << "Fleet roster (vector):" << endl;
        for (const auto& ship : fleet) {
            cout << "  " << ship.name << " (" << ship.type << ") - Strength: " << ship.strength << endl;
        }
        
        // Map - key-value pairs for ship registry
        map<string, Ship> shipRegistry;
        for (const auto& ship : fleet) {
            shipRegistry[ship.name] = ship;
        }
        
        cout << "\nShip lookup (map):" << endl;
        string searchShip = "Enterprise";
        if (auto it = shipRegistry.find(searchShip); it != shipRegistry.end()) {
            cout << "Found " << searchShip << ": " << it->second.type << " class" << endl;
        }
        
        // Set - unique ship types
        set<string> shipTypes;
        for (const auto& ship : fleet) {
            shipTypes.insert(ship.type);
        }
        
        cout << "Unique ship types (set): ";
        for (const auto& type : shipTypes) {
            cout << type << " ";
        }
        cout << endl;
    }
    
    void demonstrateAlgorithms() {
        cout << "\n--- STL Algorithms ---" << endl;
        
        vector<Ship> fleet = {
            {"Enterprise", "Explorer", 500},
            {"Defiant", "Warship", 800},
            {"Voyager", "Explorer", 400},
            {"Prometheus", "Warship", 750},
            {"Discovery", "Science", 300},
            {"Intrepid", "Science", 250}
        };
        
        // Sort by strength (descending)
        sort(fleet.begin(), fleet.end(), [](const Ship& a, const Ship& b) {
            return a.strength > b.strength;
        });
        
        cout << "Fleet sorted by strength:" << endl;
        for (const auto& ship : fleet) {
            cout << "  " << ship.name << ": " << ship.strength << endl;
        }
        
        // Find strongest warship
        auto warshipIt = find_if(fleet.begin(), fleet.end(), [](const Ship& ship) {
            return ship.type == "Warship";
        });
        
        if (warshipIt != fleet.end()) {
            cout << "Strongest warship: " << warshipIt->name << " (" << warshipIt->strength << ")" << endl;
        }
        
        // Count ships by type
        int explorerCount = count_if(fleet.begin(), fleet.end(), [](const Ship& ship) {
            return ship.type == "Explorer";
        });
        cout << "Number of Explorer ships: " << explorerCount << endl;
        
        // Transform - calculate total fleet strength
        vector<int> strengths;
        transform(fleet.begin(), fleet.end(), back_inserter(strengths), 
                 [](const Ship& ship) { return ship.strength; });
        
        int totalStrength = accumulate(strengths.begin(), strengths.end(), 0);
        cout << "Total fleet strength: " << totalStrength << endl;
        
        // Partition - separate operational from damaged ships
        // First, damage some ships for demonstration
        fleet[2].operational = false;
        fleet[4].operational = false;
        
        auto partitionPoint = partition(fleet.begin(), fleet.end(), [](const Ship& ship) {
            return ship.operational;
        });
        
        cout << "Operational ships:" << endl;
        for (auto it = fleet.begin(); it != partitionPoint; ++it) {
            cout << "  " << it->name << endl;
        }
        
        cout << "Ships needing repair:" << endl;
        for (auto it = partitionPoint; it != fleet.end(); ++it) {
            cout << "  " << it->name << endl;
        }
    }
    
    void demonstrateIterators() {
        cout << "\n--- STL Iterators ---" << endl;
        
        vector<int> shipIds = {101, 102, 103, 104, 105};
        
        // Forward iterator
        cout << "Ship IDs (forward): ";
        for (auto it = shipIds.begin(); it != shipIds.end(); ++it) {
            cout << *it << " ";
        }
        cout << endl;
        
        // Reverse iterator
        cout << "Ship IDs (reverse): ";
        for (auto it = shipIds.rbegin(); it != shipIds.rend(); ++it) {
            cout << *it << " ";
        }
        cout << endl;
        
        // Range-based for loop (C++11)
        cout << "Ship IDs (range-based): ";
        for (const auto& id : shipIds) {
            cout << id << " ";
        }
        cout << endl;
        
        // Iterator arithmetic
        auto middleIt = shipIds.begin() + shipIds.size() / 2;
        cout << "Middle ship ID: " << *middleIt << endl;
    }
    
    void demonstrateSTL() {
        demonstrateContainers();
        demonstrateAlgorithms();
        demonstrateIterators();
        cout << "--- STL Lesson Complete ---" << endl;
    }
}

/**
 * LESSON 3: MODERN C++ FEATURES
 * Learn C++11/14/17/20 features through advanced space simulation
 */
namespace Lesson3_ModernCpp {
    
    void demonstrateSmartPointers() {
        cout << "\n=== LESSON 3: Modern C++ Features ===" << endl;
        cout << "\n--- Smart Pointers ---" << endl;
        
        // unique_ptr - exclusive ownership
        auto planet = make_unique<Lesson1_OOP::Planet>("Kepler-438b", 4.6e24, 6400000, true);
        cout << "Created planet with unique_ptr: " << planet->getName() << endl;
        
        // shared_ptr - shared ownership
        auto spacecraft1 = make_shared<Lesson1_OOP::Spacecraft>("Apollo", 45000, "Lunar", 500);
        auto spacecraft2 = spacecraft1;  // Shared ownership
        
        cout << "Spacecraft shared by " << spacecraft1.use_count() << " owners" << endl;
        
        // weak_ptr - non-owning observer
        weak_ptr<Lesson1_OOP::Spacecraft> observer = spacecraft1;
        if (auto locked = observer.lock()) {
            cout << "Observed spacecraft: " << locked->getName() << endl;
        }
        
        cout << "Smart pointers automatically manage memory - no manual delete needed!" << endl;
    }
    
    void demonstrateAutoAndTypes() {
        cout << "\n--- Auto and Type Deduction ---" << endl;
        
        // Auto type deduction
        auto shipName = "Millennium Falcon";  // deduced as const char*
        auto shipStrength = 650;               // deduced as int
        auto isOperational = true;             // deduced as bool
        
        cout << "Auto deduced types work seamlessly" << endl;
        
        // Lambda expressions with auto
        auto calculateDistance = [](double x1, double y1, double x2, double y2) {
            return sqrt((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1));
        };
        
        double distance = calculateDistance(0, 0, 3, 4);
        cout << "Distance calculated by lambda: " << distance << endl;
        
        // decltype for complex types
        vector<pair<string, int>> fleetStats = {{"Explorer", 500}, {"Warship", 800}};
        decltype(fleetStats) backupStats = fleetStats;  // Same type as fleetStats
        
        cout << "decltype helps with complex type declarations" << endl;
    }
    
    void demonstrateRangeBasedFor() {
        cout << "\n--- Range-Based For Loops ---" << endl;
        
        vector<string> planets = {"Mercury", "Venus", "Earth", "Mars", "Jupiter"};
        
        // Traditional for loop
        cout << "Traditional loop: ";
        for (size_t i = 0; i < planets.size(); ++i) {
            cout << planets[i] << " ";
        }
        cout << endl;
        
        // Range-based for loop (much cleaner)
        cout << "Range-based loop: ";
        for (const auto& planet : planets) {
            cout << planet << " ";
        }
        cout << endl;
        
        // Modifying elements
        vector<int> coordinates = {1, 2, 3, 4, 5};
        for (auto& coord : coordinates) {
            coord *= 2;  // Double each coordinate
        }
        
        cout << "Modified coordinates: ";
        for (const auto& coord : coordinates) {
            cout << coord << " ";
        }
        cout << endl;
    }
    
    void demonstrateLambdas() {
        cout << "\n--- Lambda Expressions ---" << endl;
        
        vector<int> shipStrengths = {300, 500, 800, 450, 600};
        
        // Simple lambda
        auto printStrength = [](int strength) {
            cout << "Ship strength: " << strength << endl;
        };
        printStrength(500);
        
        // Lambda with capture
        int threshold = 500;
        auto countStrongShips = [threshold](const vector<int>& strengths) {
            return count_if(strengths.begin(), strengths.end(), 
                           [threshold](int strength) { return strength >= threshold; });
        };
        
        int strongShips = countStrongShips(shipStrengths);
        cout << "Ships with strength >= " << threshold << ": " << strongShips << endl;
        
        // Lambda as function parameter
        auto processFleet = [](const vector<int>& strengths, function<bool(int)> condition) {
            for (int strength : strengths) {
                if (condition(strength)) {
                    cout << "Processing ship with strength " << strength << endl;
                }
            }
        };
        
        // Use lambda with processFleet
        processFleet(shipStrengths, [](int s) { return s > 400 && s < 700; });
    }
    
    void demonstrateOptionalAndVariant() {
        cout << "\n--- Optional and Variant (C++17) ---" << endl;
        
        // Optional - may or may not have a value
        auto findPlanetByName = [](const string& name) -> optional<string> {
            map<string, string> planets = {
                {"Earth", "Terrestrial"},
                {"Mars", "Terrestrial"}, 
                {"Jupiter", "Gas Giant"}
            };
            
            if (auto it = planets.find(name); it != planets.end()) {
                return it->second;
            }
            return nullopt;  // No value
        };
        
        if (auto type = findPlanetByName("Earth"); type.has_value()) {
            cout << "Earth is a " << *type << " planet" << endl;
        }
        
        if (auto type = findPlanetByName("Pluto"); !type.has_value()) {
            cout << "Pluto not found in database" << endl;
        }
        
        // Variant - can hold one of several types
        using SpaceEntity = variant<string, int, double>;  // name, id, or mass
        
        vector<SpaceEntity> entities = {
            string("Enterprise"),  // name
            42,                    // id  
            5.972e24              // mass
        };
        
        for (const auto& entity : entities) {
            visit([](const auto& value) {
                cout << "Entity: " << value << endl;
            }, entity);
        }
    }
    
    void demonstrateModernCpp() {
        demonstrateSmartPointers();
        demonstrateAutoAndTypes();
        demonstrateRangeBasedFor();
        demonstrateLambdas();
        demonstrateOptionalAndVariant();
        cout << "--- Modern C++ Lesson Complete ---" << endl;
    }
}

/**
 * LESSON 4: TEMPLATES AND GENERIC PROGRAMMING
 * Learn template programming through generic space utilities
 */
namespace Lesson4_Templates {
    
    // Function template
    template<typename T>
    T calculateDistance3D(T x1, T y1, T z1, T x2, T y2, T z2) {
        T dx = x2 - x1;
        T dy = y2 - y1;
        T dz = z2 - z1;
        return sqrt(dx*dx + dy*dy + dz*dz);
    }
    
    // Class template
    template<typename T, size_t Dimensions>
    class Coordinate {
    private:
        array<T, Dimensions> coords;
        
    public:
        Coordinate() : coords{} {}
        
        Coordinate(initializer_list<T> values) : coords{} {
            size_t i = 0;
            for (const auto& value : values) {
                if (i < Dimensions) coords[i++] = value;
            }
        }
        
        T& operator[](size_t index) { return coords[index]; }
        const T& operator[](size_t index) const { return coords[index]; }
        
        T distanceTo(const Coordinate& other) const {
            T sum = T{};
            for (size_t i = 0; i < Dimensions; ++i) {
                T diff = coords[i] - other.coords[i];
                sum += diff * diff;
            }
            return sqrt(sum);
        }
        
        void print() const {
            cout << "(";
            for (size_t i = 0; i < Dimensions; ++i) {
                cout << coords[i];
                if (i < Dimensions - 1) cout << ", ";
            }
            cout << ")";
        }
    };
    
    // Specialized template for 2D coordinates
    template<typename T>
    class Coordinate<T, 2> {
    private:
        T x, y;
        
    public:
        Coordinate() : x{}, y{} {}
        Coordinate(T x_val, T y_val) : x(x_val), y(y_val) {}
        
        T getX() const { return x; }
        T getY() const { return y; }
        
        T distanceTo(const Coordinate& other) const {
            T dx = x - other.x;
            T dy = y - other.y;
            return sqrt(dx*dx + dy*dy);
        }
        
        void print() const {
            cout << "(" << x << ", " << y << ")";
        }
    };
    
    // Variadic template
    template<typename T, typename... Args>
    void printSpaceLog(T&& first, Args&&... args) {
        cout << "[SPACE LOG] " << forward<T>(first);
        ((cout << " " << forward<Args>(args)), ...);  // C++17 fold expression
        cout << endl;
    }
    
    // Template with concepts (C++20)
    template<typename T>
    concept Numeric = is_arithmetic_v<T>;
    
    template<Numeric T>
    T calculateOrbitalVelocity(T mass, T radius) {
        const T G = 6.67430e-11;
        return sqrt(G * mass / radius);
    }
    
    void demonstrateTemplates() {
        cout << "\n=== LESSON 4: Templates and Generic Programming ===" << endl;
        
        // Function template usage
        cout << "\n--- Function Templates ---" << endl;
        double earthToMoon = calculateDistance3D(0.0, 0.0, 0.0, 384400.0, 0.0, 0.0);
        cout << "Distance to Moon: " << earthToMoon << " km" << endl;
        
        // Class template usage  
        cout << "\n--- Class Templates ---" << endl;
        
        // 3D coordinate
        Coordinate<double, 3> earthPos{0, 0, 0};
        Coordinate<double, 3> marsPos{227.9e6, 0, 0};  // km from sun
        
        cout << "Earth position: ";
        earthPos.print();
        cout << endl;
        
        cout << "Mars position: ";
        marsPos.print();
        cout << endl;
        
        cout << "Distance Earth to Mars: " << earthPos.distanceTo(marsPos) << " km" << endl;
        
        // 2D coordinate (specialized template)
        Coordinate<int, 2> shipPos{100, 200};
        Coordinate<int, 2> stationPos{300, 400};
        
        cout << "Ship at: ";
        shipPos.print();
        cout << ", Station at: ";
        stationPos.print();
        cout << endl;
        
        cout << "Distance: " << shipPos.distanceTo(stationPos) << " units" << endl;
        
        // Variadic template usage
        cout << "\n--- Variadic Templates ---" << endl;
        printSpaceLog("Mission", "Apollo", "status:", "nominal");
        printSpaceLog("Fuel level:", 75, "percent");
        printSpaceLog("Coordinates:", 12.5, 67.8, -34.2);
        
        // Concepts (C++20)
        cout << "\n--- Concepts ---" << endl;
        double earthMass = 5.972e24;  // kg
        double earthRadius = 6.371e6; // m
        
        double orbitalVel = calculateOrbitalVelocity(earthMass, earthRadius);
        cout << "Earth orbital velocity: " << orbitalVel << " m/s" << endl;
        
        cout << "--- Templates Lesson Complete ---" << endl;
    }
}

/**
 * LESSON 5: MEMORY MANAGEMENT AND RAII
 * Learn modern memory management through resource handling
 */
namespace Lesson5_Memory {
    
    // RAII example - automatically manages file resources
    class SpaceLogFile {
    private:
        FILE* file;
        string filename;
        
    public:
        SpaceLogFile(const string& name) : filename(name) {
            file = fopen(filename.c_str(), "w");
            if (!file) {
                throw runtime_error("Could not open log file: " + filename);
            }
            cout << "Opened log file: " << filename << endl;
        }
        
        ~SpaceLogFile() {
            if (file) {
                fclose(file);
                cout << "Closed log file: " << filename << endl;
            }
        }
        
        // Delete copy constructor/assignment (non-copyable resource)
        SpaceLogFile(const SpaceLogFile&) = delete;
        SpaceLogFile& operator=(const SpaceLogFile&) = delete;
        
        // Enable move semantics
        SpaceLogFile(SpaceLogFile&& other) noexcept : file(other.file), filename(move(other.filename)) {
            other.file = nullptr;
        }
        
        SpaceLogFile& operator=(SpaceLogFile&& other) noexcept {
            if (this != &other) {
                if (file) fclose(file);
                file = other.file;
                filename = move(other.filename);
                other.file = nullptr;
            }
            return *this;
        }
        
        void write(const string& message) {
            if (file) {
                fprintf(file, "%s\n", message.c_str());
                fflush(file);
            }
        }
    };
    
    // Custom deleter example
    struct SpaceStationDeleter {
        void operator()(void* ptr) {
            cout << "Decommissioning space station at " << ptr << endl;
            free(ptr);
        }
    };
    
    void demonstrateRAII() {
        cout << "\n=== LESSON 5: Memory Management and RAII ===" << endl;
        cout << "\n--- RAII (Resource Acquisition Is Initialization) ---" << endl;
        
        try {
            // RAII ensures automatic cleanup even if exceptions occur
            SpaceLogFile missionLog("mission.log");
            missionLog.write("Mission started");
            missionLog.write("Engines nominal");
            
            // Simulate potential error
            if (true) {  // Always true for demo
                missionLog.write("Navigation systems online");
            }
            
            missionLog.write("Mission completed successfully");
            // File automatically closed when missionLog goes out of scope
            
        } catch (const exception& e) {
            cout << "Exception caught: " << e.what() << endl;
            // File would still be properly closed due to RAII
        }
    }
    
    void demonstrateSmartPointerDetails() {
        cout << "\n--- Smart Pointers Deep Dive ---" << endl;
        
        // unique_ptr with custom deleter
        unique_ptr<void, SpaceStationDeleter> station(malloc(1024));
        cout << "Space station constructed at " << station.get() << endl;
        
        // shared_ptr reference counting
        {
            auto spacecraft = make_shared<string>("Enterprise");
            cout << "Spacecraft reference count: " << spacecraft.use_count() << endl;
            
            {
                auto mission = spacecraft;  // Share ownership
                cout << "Spacecraft reference count: " << spacecraft.use_count() << endl;
                
                weak_ptr<string> observer = spacecraft;  // Non-owning reference
                cout << "Observer valid: " << (observer.expired() ? "No" : "Yes") << endl;
                
            } // mission destroyed here
            
            cout << "After mission end, reference count: " << spacecraft.use_count() << endl;
        } // spacecraft destroyed here
        
        // Memory pool simulation
        cout << "\n--- Memory Pool Pattern ---" << endl;
        
        // Simple memory pool for frequent small allocations
        class SimplePool {
        private:
            static constexpr size_t BLOCK_SIZE = 64;
            static constexpr size_t POOL_SIZE = 10;
            
            struct Block {
                alignas(max_align_t) char data[BLOCK_SIZE];
                bool used = false;
            };
            
            array<Block, POOL_SIZE> pool;
            
        public:
            void* allocate() {
                for (auto& block : pool) {
                    if (!block.used) {
                        block.used = true;
                        cout << "Allocated block from pool" << endl;
                        return block.data;
                    }
                }
                cout << "Pool exhausted, using regular allocation" << endl;
                return malloc(BLOCK_SIZE);
            }
            
            void deallocate(void* ptr) {
                for (auto& block : pool) {
                    if (block.data == ptr) {
                        block.used = false;
                        cout << "Returned block to pool" << endl;
                        return;
                    }
                }
                cout << "Freeing regular allocation" << endl;
                free(ptr);
            }
        };
        
        SimplePool pool;
        vector<void*> allocations;
        
        // Allocate several blocks
        for (int i = 0; i < 12; ++i) {
            allocations.push_back(pool.allocate());
        }
        
        // Deallocate them
        for (void* ptr : allocations) {
            pool.deallocate(ptr);
        }
    }
    
    void demonstrateMemoryManagement() {
        demonstrateRAII();
        demonstrateSmartPointerDetails();
        cout << "--- Memory Management Lesson Complete ---" << endl;
    }
}

/**
 * MAIN TUTORIAL FUNCTION
 * Orchestrates all learning modules
 */
void runCompleteTutorial() {
    cout << "🚀 Welcome to CppVerseHub C++ Learning Tutorial! 🚀" << endl;
    cout << "Learn C++ programming through space exploration examples" << endl;
    cout << "========================================================" << endl;
    
    try {
        // Run all lessons
        Lesson1_OOP::demonstrateOOP();
        Lesson2_STL::demonstrateSTL();  
        Lesson3_ModernCpp::demonstrateModernCpp();
        Lesson4_Templates::demonstrateTemplates();
        Lesson5_Memory::demonstrateMemoryManagement();
        
        cout << "\n🎉 Congratulations! Tutorial Complete! 🎉" << endl;
        cout << "\nYou've learned:" << endl;
        cout << "✓ Object-Oriented Programming (Classes, Inheritance, Polymorphism)" << endl;
        cout << "✓ STL Containers and Algorithms" << endl;
        cout << "✓ Modern C++ Features (Smart Pointers, Auto, Lambdas)" << endl;
        cout << "✓ Template and Generic Programming" << endl;
        cout << "✓ Memory Management and RAII" << endl;
        
        cout << "\nNext steps:" << endl;
        cout << "• Explore examples/advanced_usage/ for complex scenarios" << endl;
        cout << "• Check out design_patterns_tutorial.cpp" << endl;
        cout << "• Try modern_cpp_features.cpp for cutting-edge features" << endl;
        
    } catch (const exception& e) {
        cout << "Tutorial error: " << e.what() << endl;
    }
}

/**
 * @brief Main function
 */
int main() {
    runCompleteTutorial();
    return 0;
}