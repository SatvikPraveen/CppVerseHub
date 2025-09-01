// File: src/modern/LambdaExpressions.cpp
// Advanced Lambda Expressions Implementation

#include "LambdaExpressions.hpp"
#include <thread>
#include <chrono>
#include <random>
#include <iomanip>

namespace CppVerseHub::Modern::LambdaExpressions {

// ===== LAMBDA PERFORMANCE BENCHMARKING =====

void benchmark_lambda_vs_function() {
    std::cout << "\n=== Lambda vs Function Performance ===" << std::endl;
    
    const size_t iterations = 1000000;
    std::vector<int> test_data;
    test_data.reserve(iterations);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 1000);
    
    for (size_t i = 0; i < iterations; ++i) {
        test_data.push_back(dis(gen));
    }
    
    // Lambda version
    auto lambda_processor = [](int x) { return x * x + 2 * x + 1; };
    
    auto start = std::chrono::high_resolution_clock::now();
    long long lambda_sum = 0;
    for (const auto& val : test_data) {
        lambda_sum += lambda_processor(val);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto lambda_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Function pointer version
    auto function_processor = [](int x) { return x * x + 2 * x + 1; };
    std::function<int(int)> func_ptr = function_processor;
    
    start = std::chrono::high_resolution_clock::now();
    long long function_sum = 0;
    for (const auto& val : test_data) {
        function_sum += func_ptr(val);
    }
    end = std::chrono::high_resolution_clock::now();
    auto function_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Processing " << iterations << " elements:" << std::endl;
    std::cout << "  Lambda time: " << lambda_duration.count() << " μs" << std::endl;
    std::cout << "  std::function time: " << function_duration.count() << " μs" << std::endl;
    std::cout << "  Results match: " << (lambda_sum == function_sum ? "Yes" : "No") << std::endl;
    
    double speedup = static_cast<double>(function_duration.count()) / lambda_duration.count();
    std::cout << "  Lambda speedup: " << std::fixed << std::setprecision(2) << speedup << "x" << std::endl;
}

// ===== COMPLEX LAMBDA PATTERNS =====

void demonstrate_lambda_patterns() {
    std::cout << "\n=== Advanced Lambda Patterns ===" << std::endl;
    
    // Builder pattern with lambdas
    auto fleet_builder = []() {
        struct FleetBuilder {
            std::vector<SpaceShip> ships;
            
            FleetBuilder& add_ship(int id, const std::string& name, const std::string& type, double fuel, int crew) {
                ships.emplace_back(id, name, type, fuel, crew);
                return *this;
            }
            
            FleetBuilder& set_all_active(bool active) {
                std::for_each(ships.begin(), ships.end(), 
                    [active](SpaceShip& ship) { ship.is_active = active; });
                return *this;
            }
            
            FleetBuilder& refuel_all(double fuel_level) {
                std::for_each(ships.begin(), ships.end(), 
                    [fuel_level](SpaceShip& ship) { ship.fuel_level = fuel_level; });
                return *this;
            }
            
            std::vector<SpaceShip> build() { return std::move(ships); }
        };
        return FleetBuilder{};
    };
    
    auto battle_fleet = fleet_builder()
        .add_ship(101, "USS Destroyer", "Combat", 95.0, 250)
        .add_ship(102, "USS Cruiser", "Combat", 87.5, 300)
        .add_ship(103, "USS Frigate", "Combat", 92.3, 180)
        .set_all_active(true)
        .build();
    
    std::cout << "Built battle fleet:" << std::endl;
    for (const auto& ship : battle_fleet) {
        std::cout << "  " << ship << std::endl;
    }
    
    // Strategy pattern with lambdas
    using NavigationStrategy = std::function<double(double, double, double, double)>;
    
    std::map<std::string, NavigationStrategy> navigation_strategies = {
        {"Direct", [](double x1, double y1, double x2, double y2) {
            return std::sqrt((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1));
        }},
        {"Manhattan", [](double x1, double y1, double x2, double y2) {
            return std::abs(x2-x1) + std::abs(y2-y1);
        }},
        {"Hyperspace", [](double x1, double y1, double x2, double y2) {
            double direct = std::sqrt((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1));
            return direct * 0.1; // Hyperspace is 10x faster
        }}
    };
    
    std::cout << "\nNavigation strategies from (0,0) to (10,10):" << std::endl;
    for (const auto& [strategy_name, strategy_func] : navigation_strategies) {
        double distance = strategy_func(0, 0, 10, 10);
        std::cout << "  " << strategy_name << ": " << distance << " units" << std::endl;
    }
}

// ===== LAMBDA COMPOSITION =====

void demonstrate_lambda_composition() {
    std::cout << "\n=== Lambda Composition ===" << std::endl;
    
    // Function composition utility
    template<typename F, typename G>
    auto compose(F&& f, G&& g) {
        return [f = std::forward<F>(f), g = std::forward<G>(g)](auto&& x) {
            return f(g(std::forward<decltype(x)>(x)));
        };
    }
    
    // Basic mathematical functions
    auto square = [](double x) { return x * x; };
    auto add_ten = [](double x) { return x + 10; };
    auto halve = [](double x) { return x / 2; };
    
    // Compose functions
    auto complex_operation = compose(square, compose(add_ten, halve));
    
    std::cout << "Function composition example:" << std::endl;
    double input = 20.0;
    std::cout << "  Input: " << input << std::endl;
    std::cout << "  halve(20) = " << halve(input) << std::endl;
    std::cout << "  add_ten(halve(20)) = " << add_ten(halve(input)) << std::endl;
    std::cout << "  square(add_ten(halve(20))) = " << complex_operation(input) << std::endl;
    
    // Pipeline of data transformations
    auto create_pipeline = [](auto... transforms) {
        return [transforms...](auto input) {
            return (transforms(input), ...); // C++17 fold expression - applies each transform
        };
    };
    
    std::vector<SpaceShip> fleet = {
        {1, "USS Alpha", "Explorer", 45.2, 100, false},
        {2, "USS Beta", "Combat", 78.5, 200, true},
        {3, "USS Gamma", "Research", 91.0, 150, true}
    };
    
    // Create processing pipeline
    auto activate_ships = [](std::vector<SpaceShip>& ships) {
        std::for_each(ships.begin(), ships.end(), [](SpaceShip& ship) { ship.is_active = true; });
    };
    
    auto refuel_ships = [](std::vector<SpaceShip>& ships) {
        std::for_each(ships.begin(), ships.end(), [](SpaceShip& ship) { ship.fuel_level = 100.0; });
    };
    
    auto add_equipment = [](std::vector<SpaceShip>& ships) {
        std::for_each(ships.begin(), ships.end(), [](SpaceShip& ship) { 
            ship.equipment.push_back("Standard Equipment"); 
        });
    };
    
    std::cout << "\nFleet before processing:" << std::endl;
    for (const auto& ship : fleet) {
        std::cout << "  " << ship << " (Active: " << ship.is_active << ")" << std::endl;
    }
    
    // Apply pipeline
    create_pipeline(activate_ships, refuel_ships, add_equipment)(fleet);
    
    std::cout << "Fleet after processing:" << std::endl;
    for (const auto& ship : fleet) {
        std::cout << "  " << ship << " (Active: " << ship.is_active 
                  << ", Equipment: " << ship.equipment.size() << " items)" << std::endl;
    }
}

// ===== LAMBDA-BASED EVENT SYSTEM =====

class EventSystem {
private:
    std::map<std::string, std::vector<std::function<void(const std::string&)>>> event_handlers_;
    
public:
    template<typename Handler>
    void subscribe(const std::string& event_type, Handler&& handler) {
        event_handlers_[event_type].emplace_back(std::forward<Handler>(handler));
    }
    
    void emit(const std::string& event_type, const std::string& data = "") {
        if (event_handlers_.find(event_type) != event_handlers_.end()) {
            for (const auto& handler : event_handlers_[event_type]) {
                handler(data);
            }
        }
    }
    
    void clear_handlers(const std::string& event_type) {
        event_handlers_[event_type].clear();
    }
    
    size_t handler_count(const std::string& event_type) const {
        auto it = event_handlers_.find(event_type);
        return (it != event_handlers_.end()) ? it->second.size() : 0;
    }
};

void demonstrate_lambda_event_system() {
    std::cout << "\n=== Lambda-based Event System ===" << std::endl;
    
    EventSystem game_events;
    
    // Subscribe to ship events with lambdas
    game_events.subscribe("ship_destroyed", [](const std::string& data) {
        std::cout << "🚨 Alert: Ship " << data << " has been destroyed!" << std::endl;
    });
    
    game_events.subscribe("ship_destroyed", [](const std::string& data) {
        std::cout << "📊 Updating fleet statistics after loss of " << data << std::endl;
    });
    
    game_events.subscribe("mission_completed", [](const std::string& data) {
        std::cout << "✅ Mission " << data << " completed successfully!" << std::endl;
    });
    
    game_events.subscribe("mission_completed", [](const std::string& data) {
        std::cout << "🎖️ Awarding experience points for mission " << data << std::endl;
    });
    
    game_events.subscribe("planet_discovered", [](const std::string& data) {
        std::cout << "🌍 New planet discovered: " << data << std::endl;
    });
    
    // Stateful lambda for resource tracking
    int total_resources = 1000;
    game_events.subscribe("resource_found", [&total_resources](const std::string& data) {
        int amount = std::stoi(data);
        total_resources += amount;
        std::cout << "💎 Found " << amount << " resources. Total: " << total_resources << std::endl;
    });
    
    // Emit various events
    std::cout << "Event handlers registered:" << std::endl;
    std::cout << "  ship_destroyed: " << game_events.handler_count("ship_destroyed") << " handlers" << std::endl;
    std::cout << "  mission_completed: " << game_events.handler_count("mission_completed") << " handlers" << std::endl;
    
    std::cout << "\nEmitting events:" << std::endl;
    game_events.emit("ship_destroyed", "USS Explorer");
    game_events.emit("mission_completed", "Alpha-7");
    game_events.emit("planet_discovered", "Kepler-442c");
    game_events.emit("resource_found", "250");
    game_events.emit("resource_found", "150");
}

// ===== LAMBDA METAPROGRAMMING =====

void demonstrate_lambda_metaprogramming() {
    std::cout << "\n=== Lambda Metaprogramming ===" << std::endl;
    
    // Type-based lambda dispatch
    auto process_by_type = []<typename T>(const T& value) {
        if constexpr (std::is_integral_v<T>) {
            std::cout << "Processing integer: " << value << " (squared = " << value * value << ")" << std::endl;
        } else if constexpr (std::is_floating_point_v<T>) {
            std::cout << "Processing float: " << value << " (sqrt = " << std::sqrt(value) << ")" << std::endl;
        } else if constexpr (std::is_same_v<T, std::string>) {
            std::cout << "Processing string: \"" << value << "\" (length = " << value.length() << ")" << std::endl;
        } else {
            std::cout << "Processing unknown type" << std::endl;
        }
    };
    
    process_by_type(42);
    process_by_type(3.14159);
    process_by_type(std::string("CppVerseHub"));
    
    // Variadic lambda for processing multiple arguments
    auto process_all = []<typename... Args>(Args&&... args) {
        std::cout << "Processing " << sizeof...(args) << " arguments:" << std::endl;
        ((std::cout << "  " << args << std::endl), ...);
        
        if constexpr (sizeof...(args) > 0) {
            auto sum = (args + ...);
            std::cout << "  Sum: " << sum << std::endl;
        }
    };
    
    process_all(1, 2, 3, 4, 5);
    process_all(1.5, 2.7, 3.14);
    
    // Compile-time lambda evaluation
    auto constexpr_fibonacci = []<int N>() constexpr {
        if constexpr (N <= 1) {
            return N;
        } else {
            // Note: This would require recursive template instantiation
            // For demonstration purposes, we'll use a simple iterative approach
            int a = 0, b = 1;
            for (int i = 2; i <= N; ++i) {
                int temp = a + b;
                a = b;
                b = temp;
            }
            return b;
        }
    };
    
    std::cout << "\nCompile-time Fibonacci calculations:" << std::endl;
    std::cout << "  F(10) = " << constexpr_fibonacci.template operator()<10>() << std::endl;
    std::cout << "  F(15) = " << constexpr_fibonacci.template operator()<15>() << std::endl;
}

// ===== LAMBDA UTILITIES AND HELPERS =====

namespace LambdaUtils {
    
    // Curry utility for partial application
    template<typename Func>
    auto curry(Func&& func) {
        return [func = std::forward<Func>(func)](auto&&... args1) {
            if constexpr (std::is_invocable_v<Func, decltype(args1)...>) {
                return func(std::forward<decltype(args1)>(args1)...);
            } else {
                return [func, args1...](auto&&... args2) {
                    return func(args1..., std::forward<decltype(args2)>(args2)...);
                };
            }
        };
    }
    
    // Memoization utility
    template<typename Func>
    auto memoize(Func&& func) {
        return [func = std::forward<Func>(func), cache = std::map<std::string, decltype(func(std::string{}))>{}]
               (const auto& arg) mutable {
            auto key = std::to_string(arg);
            if (auto it = cache.find(key); it != cache.end()) {
                return it->second;
            }
            auto result = func(arg);
            cache[key] = result;
            return result;
        };
    }
    
} // namespace LambdaUtils

void demonstrate_lambda_utilities() {
    std::cout << "\n=== Lambda Utilities ===" << std::endl;
    
    // Currying demonstration
    auto add_three = [](int a, int b, int c) { return a + b + c; };
    auto curried_add = LambdaUtils::curry(add_three);
    
    auto add_5_and = curried_add(5);
    auto add_5_10_and = add_5_and(10);
    int result = add_5_10_and(15);
    
    std::cout << "Curried addition: 5 + 10 + 15 = " << result << std::endl;
    
    // Memoization demonstration
    auto expensive_calculation = [](int n) {
        std::cout << "    Computing factorial of " << n << "..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Simulate work
        
        long long result = 1;
        for (int i = 2; i <= n; ++i) {
            result *= i;
        }
        return result;
    };
    
    auto memoized_factorial = LambdaUtils::memoize(expensive_calculation);
    
    std::cout << "\nMemoized factorial calculations:" << std::endl;
    std::cout << "First call to factorial(8): " << memoized_factorial(8) << std::endl;
    std::cout << "Second call to factorial(8): " << memoized_factorial(8) << std::endl; // Should be cached
    std::cout << "First call to factorial(10): " << memoized_factorial(10) << std::endl;
}

} // namespace CppVerseHub::Modern::LambdaExpressions