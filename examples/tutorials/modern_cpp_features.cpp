// File: examples/tutorials/modern_cpp_features.cpp
// CppVerseHub Modern C++ Features Tutorial
// Learn C++17/20/23 features through advanced space exploration scenarios

#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <optional>
#include <variant>
#include <any>
#include <string_view>
#include <filesystem>
#include <ranges>
#include <concepts>
#include <coroutine>
#include <span>
#include <format>
#include <chrono>
#include <thread>
#include <future>
#include <algorithm>
#include <numeric>
#include <execution>

using namespace std;
namespace fs = std::filesystem;
namespace rng = std::ranges;

/**
 * FEATURE 1: C++17 STRUCTURED BINDINGS
 * Decompose objects into individual variables
 */
namespace Feature1_StructuredBindings {
    
    struct PlanetData {
        string name;
        double mass;
        double radius;
        bool habitable;
    };
    
    tuple<string, int, double> getSpacecraftInfo() {
        return {"Voyager 1", 722, 61000.5};  // name, mass_kg, distance_au
    }
    
    map<string, int> getResourceInventory() {
        return {{"minerals", 1500}, {"energy", 800}, {"food", 1200}};
    }
    
    void demonstrateStructuredBindings() {
        cout << "\n=== FEATURE 1: C++17 Structured Bindings ===" << endl;
        cout << "Decomposing complex data structures elegantly" << endl;
        
        // Decompose tuple return value
        auto [craftName, mass, distance] = getSpacecraftInfo();
        cout << "Spacecraft: " << craftName << ", Mass: " << mass 
             << "kg, Distance: " << distance << " AU" << endl;
        
        // Decompose struct
        PlanetData earth{"Earth", 5.972e24, 6.371e6, true};
        auto [planetName, planetMass, planetRadius, isHabitable] = earth;
        cout << "Planet: " << planetName << ", Habitable: " 
             << (isHabitable ? "Yes" : "No") << endl;
        
        // Iterate over map with structured bindings
        cout << "Resource inventory:" << endl;
        for (const auto& [resource, amount] : getResourceInventory()) {
            cout << "  " << resource << ": " << amount << " units" << endl;
        }
        
        // Array decomposition
        int coordinates[3] = {100, 200, 300};
        auto [x, y, z] = coordinates;
        cout << "Position: (" << x << ", " << y << ", " << z << ")" << endl;
        
        cout << "--- Structured Bindings Complete ---" << endl;
    }
}

/**
 * FEATURE 2: C++17 OPTIONAL AND VARIANT
 * Handle optional values and type-safe unions
 */
namespace Feature2_OptionalVariant {
    
    optional<string> findPlanetType(const string& planetName) {
        static const map<string, string> planetDatabase = {
            {"Mercury", "Terrestrial"},
            {"Venus", "Terrestrial"},
            {"Earth", "Terrestrial"},
            {"Mars", "Terrestrial"},
            {"Jupiter", "Gas Giant"},
            {"Saturn", "Gas Giant"},
            {"Uranus", "Ice Giant"},
            {"Neptune", "Ice Giant"}
        };
        
        if (auto it = planetDatabase.find(planetName); it != planetDatabase.end()) {
            return it->second;
        }
        return nullopt;
    }
    
    // Variant for different measurement units
    using Measurement = variant<double, int, string>;
    
    struct SensorReading {
        string sensorName;
        Measurement value;
        string unit;
    };
    
    void processSensorReading(const SensorReading& reading) {
        cout << "Sensor: " << reading.sensorName << " = ";
        
        // Use visitor pattern with variant
        visit([&reading](const auto& value) {
            using T = decay_t<decltype(value)>;
            if constexpr (is_same_v<T, double>) {
                cout << fixed << setprecision(2) << value;
            } else if constexpr (is_same_v<T, int>) {
                cout << value;
            } else if constexpr (is_same_v<T, string>) {
                cout << "\"" << value << "\"";
            }
        }, reading.value);
        
        cout << " " << reading.unit << endl;
    }
    
    void demonstrateOptionalVariant() {
        cout << "\n=== FEATURE 2: C++17 Optional and Variant ===" << endl;
        cout << "Safe handling of optional values and type unions" << endl;
        
        // Optional usage
        cout << "\n--- Optional Values ---" << endl;
        vector<string> queries = {"Earth", "Mars", "Pluto", "Jupiter"};
        
        for (const auto& planet : queries) {
            if (auto type = findPlanetType(planet); type.has_value()) {
                cout << planet << " is a " << *type << " planet" << endl;
            } else {
                cout << planet << " not found in our database" << endl;
            }
        }
        
        // Optional with value_or
        string unknownPlanet = "Kepler-442b";
        string planetType = findPlanetType(unknownPlanet).value_or("Unknown");
        cout << unknownPlanet << " type: " << planetType << endl;
        
        // Variant usage
        cout << "\n--- Variant Sensor Readings ---" << endl;
        vector<SensorReading> readings = {
            {"Temperature", 273.15, "K"},
            {"Pressure", 101325, "Pa"},
            {"Atmosphere", string("78% N2, 21% O2"), "composition"},
            {"Gravity", 9.81, "m/s²"},
            {"Population", 7800000000, "inhabitants"}
        };
        
        for (const auto& reading : readings) {
            processSensorReading(reading);
        }
        
        // Variant type checking
        auto& tempReading = readings[0];
        if (holds_alternative<double>(tempReading.value)) {
            double temp = get<double>(tempReading.value);
            cout << "Temperature in Celsius: " << (temp - 273.15) << "°C" << endl;
        }
        
        cout << "--- Optional and Variant Complete ---" << endl;
    }
}

/**
 * FEATURE 3: C++20 RANGES
 * Functional programming with composable operations
 */
namespace Feature3_Ranges {
    
    struct Spacecraft {
        string name;
        string type;
        int year;
        double speed;  // km/s
        bool active;
        
        void print() const {
            cout << name << " (" << type << ", " << year << ") - " 
                 << speed << " km/s " << (active ? "[Active]" : "[Inactive]") << endl;
        }
    };
    
    void demonstrateRanges() {
        cout << "\n=== FEATURE 3: C++20 Ranges ===" << endl;
        cout << "Composable functional programming operations" << endl;
        
        vector<Spacecraft> fleet = {
            {"Voyager 1", "Probe", 1977, 17.0, true},
            {"Voyager 2", "Probe", 1977, 15.4, true},
            {"Cassini", "Orbiter", 1997, 6.7, false},
            {"New Horizons", "Probe", 2006, 16.26, true},
            {"Parker Solar Probe", "Probe", 2018, 95.0, true},
            {"James Webb", "Telescope", 2021, 0.1, true},
            {"Hubble", "Telescope", 1990, 7.6, true},
            {"Perseverance", "Rover", 2020, 0.042, true}
        };
        
        cout << "\nOriginal fleet:" << endl;
        for (const auto& craft : fleet) {
            craft.print();
        }
        
        // Filter active spacecraft launched after 2000
        cout << "\n--- Modern Active Spacecraft (after 2000) ---" << endl;
        auto modernActive = fleet 
            | rng::views::filter([](const Spacecraft& s) { return s.active && s.year > 2000; })
            | rng::views::take(5);  // Take first 5
        
        for (const auto& craft : modernActive) {
            craft.print();
        }
        
        // Transform to get speeds and calculate average
        cout << "\n--- Speed Analysis ---" << endl;
        auto speeds = fleet 
            | rng::views::filter([](const Spacecraft& s) { return s.active; })
            | rng::views::transform([](const Spacecraft& s) { return s.speed; });
        
        vector<double> speedVector(speeds.begin(), speeds.end());
        double avgSpeed = reduce(speedVector.begin(), speedVector.end()) / speedVector.size();
        cout << "Average speed of active spacecraft: " << fixed << setprecision(2) 
             << avgSpeed << " km/s" << endl;
        
        // Find fastest spacecraft
        auto fastest = rng::max_element(fleet, {}, [](const Spacecraft& s) { return s.speed; });
        cout << "Fastest spacecraft: ";
        fastest->print();
        
        // Group by type using ranges
        cout << "\n--- Spacecraft by Type ---" << endl;
        auto sortedFleet = fleet;
        rng::sort(sortedFleet, {}, &Spacecraft::type);
        
        for (auto typeGroup = sortedFleet.begin(); typeGroup != sortedFleet.end(); ) {
            string currentType = typeGroup->type;
            cout << currentType << " spacecraft:" << endl;
            
            while (typeGroup != sortedFleet.end() && typeGroup->type == currentType) {
                cout << "  ";
                typeGroup->print();
                ++typeGroup;
            }
        }
        
        // Chain multiple operations
        cout << "\n--- Fast Active Probes (Chained Operations) ---" << endl;
        auto fastActiveProbes = fleet
            | rng::views::filter([](const Spacecraft& s) { return s.active; })
            | rng::views::filter([](const Spacecraft& s) { return s.type == "Probe"; })
            | rng::views::filter([](const Spacecraft& s) { return s.speed > 10.0; })
            | rng::views::transform([](const Spacecraft& s) { return s.name; });
        
        for (const auto& name : fastActiveProbes) {
            cout << "  " << name << endl;
        }
        
        cout << "--- Ranges Complete ---" << endl;
    }
}

/**
 * FEATURE 4: C++20 CONCEPTS
 * Constrain templates with readable requirements
 */
namespace Feature4_Concepts {
    
    // Basic concepts
    template<typename T>
    concept Numeric = is_arithmetic_v<T>;
    
    template<typename T>
    concept SpaceObject = requires(T obj) {
        { obj.getName() } -> convertible_to<string>;
        { obj.getPosition() } -> convertible_to<tuple<double, double, double>>;
        { obj.getMass() } -> convertible_to<double>;
    };
    
    // More complex concept
    template<typename T>
    concept Movable = SpaceObject<T> && requires(T obj, double x, double y, double z) {
        obj.moveTo(x, y, z);
        { obj.getVelocity() } -> convertible_to<tuple<double, double, double>>;
    };
    
    // Example classes that satisfy concepts
    class Planet {
    private:
        string name;
        tuple<double, double, double> position;
        double mass;
        
    public:
        Planet(const string& n, double x, double y, double z, double m) 
            : name(n), position{x, y, z}, mass(m) {}
        
        string getName() const { return name; }
        tuple<double, double, double> getPosition() const { return position; }
        double getMass() const { return mass; }
    };
    
    class Spacecraft {
    private:
        string name;
        tuple<double, double, double> position;
        tuple<double, double, double> velocity;
        double mass;
        
    public:
        Spacecraft(const string& n, double x, double y, double z, double m)
            : name(n), position{x, y, z}, velocity{0, 0, 0}, mass(m) {}
        
        string getName() const { return name; }
        tuple<double, double, double> getPosition() const { return position; }
        tuple<double, double, double> getVelocity() const { return velocity; }
        double getMass() const { return mass; }
        
        void moveTo(double x, double y, double z) {
            auto [oldX, oldY, oldZ] = position;
            position = {x, y, z};
            
            // Calculate velocity (simplified)
            velocity = {x - oldX, y - oldY, z - oldZ};
        }
    };
    
    // Functions using concepts
    template<SpaceObject T>
    void describeObject(const T& obj) {
        auto [x, y, z] = obj.getPosition();
        cout << obj.getName() << " at (" << x << ", " << y << ", " << z 
             << ") with mass " << obj.getMass() << " kg" << endl;
    }
    
    template<Movable T>
    void relocateObject(T& obj, double newX, double newY, double newZ) {
        cout << "Moving " << obj.getName() << " to new position..." << endl;
        obj.moveTo(newX, newY, newZ);
        
        auto [vx, vy, vz] = obj.getVelocity();
        cout << "Velocity: (" << vx << ", " << vy << ", " << vz << ")" << endl;
    }
    
    // Concept for calculations
    template<Numeric T>
    T calculateGravitationalForce(T mass1, T mass2, T distance) {
        const T G = 6.67430e-11;  // Gravitational constant
        return G * mass1 * mass2 / (distance * distance);
    }
    
    // Abbreviated function template (C++20)
    auto calculateOrbitalPeriod(Numeric auto mass, Numeric auto radius) {
        const double G = 6.67430e-11;
        return 2 * 3.14159 * sqrt((radius * radius * radius) / (G * mass));
    }
    
    void demonstrateConcepts() {
        cout << "\n=== FEATURE 4: C++20 Concepts ===" << endl;
        cout << "Constraining templates with readable requirements" << endl;
        
        // Create objects
        Planet earth("Earth", 0, 0, 0, 5.972e24);
        Spacecraft voyager("Voyager 1", 1000, 0, 0, 722);
        
        cout << "\n--- SpaceObject Concept ---" << endl;
        describeObject(earth);      // Works - Planet satisfies SpaceObject
        describeObject(voyager);    // Works - Spacecraft satisfies SpaceObject
        
        cout << "\n--- Movable Concept ---" << endl;
        // describeObject(earth);   // Would work - Planet satisfies SpaceObject
        relocateObject(voyager, 2000, 500, 100);  // Works - Spacecraft satisfies Movable
        // relocateObject(earth);   // Wouldn't compile - Planet doesn't satisfy Movable
        
        cout << "\n--- Numeric Concept ---" << endl;
        double earthMass = 5.972e24;
        double moonMass = 7.342e22;
        double distance = 384400000;  // meters
        
        auto force = calculateGravitationalForce(earthMass, moonMass, distance);
        cout << "Gravitational force between Earth and Moon: " << scientific << force << " N" << endl;
        
        // Abbreviated function template
        auto period = calculateOrbitalPeriod(earthMass, 6.371e6);
        cout << "Orbital period at Earth's surface: " << fixed << setprecision(0) 
             << period << " seconds" << endl;
        
        // Concepts provide better error messages
        cout << "\nConcepts ensure type safety and provide clear error messages!" << endl;
        
        cout << "--- Concepts Complete ---" << endl;
    }
}

/**
 * FEATURE 5: C++20 COROUTINES
 * Asynchronous programming made easy
 */
namespace Feature5_Coroutines {
    
    // Simple generator coroutine
    template<typename T>
    struct Generator {
        struct promise_type {
            T current_value;
            
            Generator get_return_object() {
                return Generator{coroutine_handle<promise_type>::from_promise(*this)};
            }
            
            suspend_always initial_suspend() { return {}; }
            suspend_always final_suspend() noexcept { return {}; }
            
            suspend_always yield_value(T value) {
                current_value = value;
                return {};
            }
            
            void return_void() {}
            void unhandled_exception() {}
        };
        
        coroutine_handle<promise_type> handle;
        
        Generator(coroutine_handle<promise_type> h) : handle(h) {}
        ~Generator() { if (handle) handle.destroy(); }
        
        // Move only
        Generator(const Generator&) = delete;
        Generator& operator=(const Generator&) = delete;
        Generator(Generator&& other) noexcept : handle(other.handle) { other.handle = {}; }
        Generator& operator=(Generator&& other) noexcept {
            if (this != &other) {
                if (handle) handle.destroy();
                handle = other.handle;
                other.handle = {};
            }
            return *this;
        }
        
        bool move_next() {
            handle.resume();
            return !handle.done();
        }
        
        T current_value() {
            return handle.promise().current_value;
        }
    };
    
    // Task coroutine for async operations
    struct Task {
        struct promise_type {
            Task get_return_object() { return {}; }
            suspend_never initial_suspend() { return {}; }
            suspend_never final_suspend() noexcept { return {}; }
            void return_void() {}
            void unhandled_exception() {}
        };
    };
    
    // Generator that produces Fibonacci sequence
    Generator<int> fibonacci(int count) {
        int a = 0, b = 1;
        for (int i = 0; i < count; ++i) {
            co_yield a;
            int temp = a;
            a = b;
            b = temp + b;
        }
    }
    
    // Generator for spacecraft positions during orbit
    Generator<tuple<double, double>> orbitPositions(double radius, int steps) {
        const double pi = 3.14159;
        for (int i = 0; i < steps; ++i) {
            double angle = 2 * pi * i / steps;
            double x = radius * cos(angle);
            double y = radius * sin(angle);
            co_yield make_tuple(x, y);
        }
    }
    
    // Simple async task
    Task simulateMission(const string& missionName, int duration_ms) {
        cout << "Starting mission: " << missionName << endl;
        
        for (int i = 0; i <= 100; i += 20) {
            cout << missionName << " progress: " << i << "%" << endl;
            this_thread::sleep_for(chrono::milliseconds(duration_ms / 5));
        }
        
        cout << "Mission " << missionName << " completed!" << endl;
        co_return;
    }
    
    void demonstrateCoroutines() {
        cout << "\n=== FEATURE 5: C++20 Coroutines ===" << endl;
        cout << "Asynchronous programming with generators and tasks" << endl;
        
        // Fibonacci generator
        cout << "\n--- Fibonacci Generator ---" << endl;
        auto fib = fibonacci(10);
        cout << "First 10 Fibonacci numbers: ";
        while (fib.move_next()) {
            cout << fib.current_value() << " ";
        }
        cout << endl;
        
        // Orbit positions generator
        cout << "\n--- Orbital Positions Generator ---" << endl;
        auto orbit = orbitPositions(100.0, 8);  // 8 positions around circle of radius 100
        cout << "Spacecraft orbital positions:" << endl;
        int step = 0;
        while (orbit.move_next()) {
            auto [x, y] = orbit.current_value();
            cout << "  Step " << step++ << ": (" << fixed << setprecision(1) 
                 << x << ", " << y << ")" << endl;
        }
        
        // Async mission simulation
        cout << "\n--- Async Mission Simulation ---" << endl;
        cout << "Starting parallel missions..." << endl;
        
        // Note: This is a simplified example
        // In practice, you'd use proper coroutine frameworks
        auto mission1 = simulateMission("Mars Rover", 1000);
        auto mission2 = simulateMission("Jupiter Probe", 800);
        
        cout << "Missions launched asynchronously!" << endl;
        
        cout << "--- Coroutines Complete ---" << endl;
    }
}

/**
 * FEATURE 6: C++23 AND BEYOND
 * Preview of upcoming features
 */
namespace Feature6_Cpp23 {
    
    void demonstrateModernFeatures() {
        cout << "\n=== FEATURE 6: C++23 and Modern Features ===" << endl;
        cout << "Latest and upcoming C++ capabilities" << endl;
        
        // std::span (C++20) - non-owning view of contiguous data
        cout << "\n--- std::span ---" << endl;
        vector<int> shipIds = {101, 102, 103, 104, 105, 106, 107, 108};
        
        auto processShips = [](span<const int> ships) {
            cout << "Processing " << ships.size() << " ships: ";
            for (int id : ships) {
                cout << id << " ";
            }
            cout << endl;
        };
        
        processShips(shipIds);  // Entire vector
        processShips(span(shipIds.data() + 2, 3));  // Ships 103, 104, 105
        
        // std::format (C++20) - type-safe formatting
        cout << "\n--- std::format ---" << endl;
        string shipName = "Enterprise";
        double speed = 2.5e8;  // m/s
        int crew = 430;
        
        string status = format("Ship: {} | Speed: {:.2e} m/s | Crew: {:>3}", 
                              shipName, speed, crew);
        cout << status << endl;
        
        // Ranges with views (C++20)
        cout << "\n--- Advanced Ranges ---" << endl;
        vector<int> distances = {150, 300, 450, 200, 100, 350, 250};
        
        auto longDistances = distances 
            | rng::views::filter([](int d) { return d > 200; })
            | rng::views::transform([](int d) { return d * 1.5; })  // Apply modifier
            | rng::views::reverse;
        
        cout << "Long distances (modified, reversed): ";
        for (int d : longDistances) {
            cout << d << " ";
        }
        cout << endl;
        
        // Parallel algorithms (C++17/20)
        cout << "\n--- Parallel Algorithms ---" << endl;
        vector<double> calculations(1000000);
        iota(calculations.begin(), calculations.end(), 1.0);
        
        auto start = chrono::high_resolution_clock::now();
        
        // Parallel execution
        transform(execution::par_unseq, calculations.begin(), calculations.end(), 
                 calculations.begin(), [](double x) { return sqrt(x * x + 1); });
        
        auto end = chrono::high_resolution_clock::now();
        auto duration = chrono::duration_cast<chrono::microseconds>(end - start);
        
        cout << "Parallel calculation of 1M square roots: " << duration.count() << " microseconds" << endl;
        
        // string_view for efficient string handling
        cout << "\n--- string_view ---" << endl;
        auto analyzeShipName = [](string_view name) {
            cout << "Ship name '" << name << "' has " << name.length() 
                 << " characters and " << count(name.begin(), name.end(), ' ') 
                 << " spaces" << endl;
        };
        
        string fullName = "USS Enterprise NCC-1701";
        analyzeShipName(fullName);  // No copy made
        analyzeShipName("Voyager");  // No copy made
        
        cout << "--- Modern Features Complete ---" << endl;
    }
}

/**
 * TUTORIAL ORCHESTRATOR
 */
void runModernCppLearning() {
    cout << "🚀 Modern C++ Features Tutorial 🚀" << endl;
    cout << "Master C++17/20/23 features through space exploration!" << endl;
    cout << "=====================================================" << endl;
    
    try {
        // Run all feature demonstrations
        Feature1_StructuredBindings::demonstrateStructuredBindings();
        Feature2_OptionalVariant::demonstrateOptionalVariant();
        Feature3_Ranges::demonstrateRanges();
        Feature4_Concepts::demonstrateConcepts();
        Feature5_Coroutines::demonstrateCoroutines();
        Feature6_Cpp23::demonstrateModernFeatures();
        
        cout << "\n🎉 Modern C++ Tutorial Complete! 🎉" << endl;
        cout << "\nModern features you've learned:" << endl;
        cout << "✓ Structured Bindings - Clean data decomposition" << endl;
        cout << "✓ Optional & Variant - Safe value and type handling" << endl;
        cout << "✓ Ranges - Functional programming pipelines" << endl;
        cout << "✓ Concepts - Readable template constraints" << endl;
        cout << "✓ Coroutines - Asynchronous programming" << endl;
        cout << "✓ Modern Features - Latest C++ capabilities" << endl;
        
        cout << "\nWhy these features matter:" << endl;
        cout << "• Structured Bindings: More readable code" << endl;
        cout << "• Optional/Variant: Eliminate null pointer errors" << endl;
        cout << "• Ranges: Functional programming without performance cost" << endl;
        cout << "• Concepts: Better error messages and API design" << endl;
        cout << "• Coroutines: Simple async programming" << endl;
        cout << "• Modern Features: Cutting-edge performance and safety" << endl;
        
        cout << "\nNext steps:" << endl;
        cout << "• Practice using these features in real projects" << endl;
        cout << "• Explore performance implications" << endl;
        cout << "• Study upcoming C++26 features" << endl;
        cout << "• Try examples/advanced_usage/ for complex scenarios" << endl;
        
    } catch (const exception& e) {
        cout << "Tutorial error: " << e.what() << endl;
    }
}

/**
 * @brief Main function
 */
int main() {
    runModernCppLearning();
    return 0;
}