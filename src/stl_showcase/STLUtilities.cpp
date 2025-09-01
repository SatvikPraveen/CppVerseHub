/**
 * @file STLUtilities.cpp
 * @brief Implementation of STL Utilities demonstrations
 * @details File location: src/stl_showcase/STLUtilities.cpp
 */

#include "STLUtilities.hpp"
#include <cmath>
#include <random>
#include <sstream>

namespace CppVerseHub::STL {

// Implementation note: Most functionality is implemented in the header file
// using static methods and lambdas for educational clarity. This file serves
// as a placeholder for any additional utility functions and provides the
// main demonstration entry points.

/**
 * @brief Advanced utility functions for STL demonstrations
 */
namespace AdvancedUtilities {

    /**
     * @brief Create a formatted string representation of a pair
     */
    template<typename T1, typename T2>
    std::string format_pair(const std::pair<T1, T2>& p) {
        std::stringstream ss;
        ss << "(" << p.first << ", " << p.second << ")";
        return ss.str();
    }

    /**
     * @brief Create a formatted string representation of a tuple (recursive)
     */
    template<std::size_t I = 0, typename... Tp>
    std::string format_tuple_impl(const std::tuple<Tp...>& t) {
        std::stringstream ss;
        if constexpr (I == sizeof...(Tp)) {
            return "";
        } else {
            if constexpr (I != 0) {
                ss << ", ";
            }
            ss << std::get<I>(t);
            if constexpr (I + 1 < sizeof...(Tp)) {
                ss << format_tuple_impl<I + 1>(t);
            }
        }
        return ss.str();
    }

    template<typename... Tp>
    std::string format_tuple(const std::tuple<Tp...>& t) {
        std::stringstream ss;
        ss << "(" << format_tuple_impl(t) << ")";
        return ss.str();
    }

    /**
     * @brief Utility to check if an optional chain would succeed
     */
    template<typename T>
    bool would_optional_chain_succeed(const std::optional<T>& opt) {
        return opt.has_value();
    }

    /**
     * @brief Safe variant getter that returns optional
     */
    template<typename T, typename... Types>
    std::optional<T> safe_variant_get(const std::variant<Types...>& var) {
        if (std::holds_alternative<T>(var)) {
            return std::get<T>(var);
        }
        return std::nullopt;
    }

    /**
     * @brief Generic variant visitor for string conversion
     */
    struct ToStringVisitor {
        template<typename T>
        std::string operator()(const T& value) const {
            if constexpr (std::is_same_v<T, std::string>) {
                return "\"" + value + "\"";
            }
            else if constexpr (std::is_same_v<T, NavigationCoordinate>) {
                std::stringstream ss;
                ss << value;
                return ss.str();
            }
            else if constexpr (std::is_same_v<T, VesselStatus>) {
                switch (value) {
                    case VesselStatus::DOCKED: return "DOCKED";
                    case VesselStatus::IN_TRANSIT: return "IN_TRANSIT";
                    case VesselStatus::EXPLORING: return "EXPLORING";
                    case VesselStatus::COMBAT: return "COMBAT";
                    case VesselStatus::MAINTENANCE: return "MAINTENANCE";
                    case VesselStatus::EMERGENCY: return "EMERGENCY";
                    default: return "UNKNOWN";
                }
            }
            else if constexpr (std::is_arithmetic_v<T>) {
                return std::to_string(value);
            }
            else {
                return "[complex type]";
            }
        }
    };

    /**
     * @brief Performance comparison between different utility types
     */
    class UtilityPerformanceTest {
    public:
        static void compareContainerAccess() {
            std::cout << "\n=== Utility Performance Comparison ===\n";
            
            constexpr size_t iterations = 100000;
            
            // Test pair vs tuple performance
            auto test_pair_access = [iterations]() {
                std::vector<std::pair<int, double>> pairs;
                pairs.reserve(iterations);
                
                for (size_t i = 0; i < iterations; ++i) {
                    pairs.emplace_back(static_cast<int>(i), static_cast<double>(i) * 1.5);
                }
                
                auto start = std::chrono::high_resolution_clock::now();
                
                double sum = 0.0;
                for (const auto& p : pairs) {
                    sum += p.first + p.second;
                }
                
                auto end = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
                
                std::cout << "Pair access time: " << duration.count() << " microseconds (sum: " << sum << ")\n";
            };
            
            auto test_tuple_access = [iterations]() {
                std::vector<std::tuple<int, double>> tuples;
                tuples.reserve(iterations);
                
                for (size_t i = 0; i < iterations; ++i) {
                    tuples.emplace_back(static_cast<int>(i), static_cast<double>(i) * 1.5);
                }
                
                auto start = std::chrono::high_resolution_clock::now();
                
                double sum = 0.0;
                for (const auto& t : tuples) {
                    sum += std::get<0>(t) + std::get<1>(t);
                }
                
                auto end = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
                
                std::cout << "Tuple access time: " << duration.count() << " microseconds (sum: " << sum << ")\n";
            };
            
            test_pair_access();
            test_tuple_access();
            
            // Test optional vs pointer performance
            auto test_optional_access = [iterations]() {
                std::vector<std::optional<int>> optionals;
                optionals.reserve(iterations);
                
                std::random_device rd;
                std::mt19937 gen(rd());
                std::bernoulli_distribution dist(0.8); // 80% have values
                
                for (size_t i = 0; i < iterations; ++i) {
                    if (dist(gen)) {
                        optionals.emplace_back(static_cast<int>(i));
                    } else {
                        optionals.emplace_back(std::nullopt);
                    }
                }
                
                auto start = std::chrono::high_resolution_clock::now();
                
                int sum = 0;
                for (const auto& opt : optionals) {
                    if (opt.has_value()) {
                        sum += *opt;
                    }
                }
                
                auto end = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
                
                std::cout << "Optional access time: " << duration.count() << " microseconds (sum: " << sum << ")\n";
            };
            
            auto test_pointer_access = [iterations]() {
                std::vector<std::unique_ptr<int>> pointers;
                pointers.reserve(iterations);
                
                std::random_device rd;
                std::mt19937 gen(rd());
                std::bernoulli_distribution dist(0.8); // 80% have values
                
                for (size_t i = 0; i < iterations; ++i) {
                    if (dist(gen)) {
                        pointers.emplace_back(std::make_unique<int>(static_cast<int>(i)));
                    } else {
                        pointers.emplace_back(nullptr);
                    }
                }
                
                auto start = std::chrono::high_resolution_clock::now();
                
                int sum = 0;
                for (const auto& ptr : pointers) {
                    if (ptr) {
                        sum += *ptr;
                    }
                }
                
                auto end = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
                
                std::cout << "Pointer access time: " << duration.count() << " microseconds (sum: " << sum << ")\n";
            };
            
            test_optional_access();
            test_pointer_access();
        }
        
        static void compareVariantVsAny() {
            std::cout << "\n=== Variant vs Any Performance ===\n";
            
            constexpr size_t iterations = 50000;
            
            // Test variant performance
            auto test_variant_performance = [iterations]() {
                using TestVariant = std::variant<int, double, std::string>;
                std::vector<TestVariant> variants;
                variants.reserve(iterations);
                
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<> type_dist(0, 2);
                
                for (size_t i = 0; i < iterations; ++i) {
                    switch (type_dist(gen)) {
                        case 0: variants.emplace_back(static_cast<int>(i)); break;
                        case 1: variants.emplace_back(static_cast<double>(i) * 1.5); break;
                        case 2: variants.emplace_back("Value-" + std::to_string(i)); break;
                    }
                }
                
                auto start = std::chrono::high_resolution_clock::now();
                
                size_t processed = 0;
                for (const auto& var : variants) {
                    std::visit([&processed](const auto& value) {
                        processed += 1;
                        // Simulate some work
                        [[maybe_unused]] volatile auto temp = value;
                    }, var);
                }
                
                auto end = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
                
                std::cout << "Variant processing time: " << duration.count() 
                          << " microseconds (processed: " << processed << ")\n";
            };
            
            // Test any performance
            auto test_any_performance = [iterations]() {
                std::vector<std::any> anys;
                anys.reserve(iterations);
                
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<> type_dist(0, 2);
                
                for (size_t i = 0; i < iterations; ++i) {
                    switch (type_dist(gen)) {
                        case 0: anys.emplace_back(static_cast<int>(i)); break;
                        case 1: anys.emplace_back(static_cast<double>(i) * 1.5); break;
                        case 2: anys.emplace_back("Value-" + std::to_string(i)); break;
                    }
                }
                
                auto start = std::chrono::high_resolution_clock::now();
                
                size_t processed = 0;
                for (const auto& any_val : anys) {
                    if (any_val.type() == typeid(int)) {
                        [[maybe_unused]] volatile auto temp = std::any_cast<int>(any_val);
                        processed++;
                    } else if (any_val.type() == typeid(double)) {
                        [[maybe_unused]] volatile auto temp = std::any_cast<double>(any_val);
                        processed++;
                    } else if (any_val.type() == typeid(std::string)) {
                        [[maybe_unused]] volatile auto temp = std::any_cast<std::string>(any_val);
                        processed++;
                    }
                }
                
                auto end = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
                
                std::cout << "Any processing time: " << duration.count() 
                          << " microseconds (processed: " << processed << ")\n";
            };
            
            test_variant_performance();
            test_any_performance();
        }
    };

    /**
     * @brief Real-world scenario combining all utilities
     */
    class SpaceStationDataSystem {
    public:
        // Complex data structure representing a space station's systems
        struct SystemStatus {
            std::string system_name;
            std::optional<double> efficiency_rating;
            std::variant<std::string, int, bool> current_status;
            std::vector<std::pair<std::string, std::any>> diagnostics;
            std::tuple<double, double, double> resource_usage; // power, fuel, coolant
        };
        
        static void demonstrateComplexScenario() {
            std::cout << "\n=== Complex Space Station Data System ===\n";
            
            std::vector<SystemStatus> station_systems{
                {
                    "Life Support",
                    98.5,
                    std::string("Nominal"),
                    {
                        {"oxygen_level", 21.2},
                        {"temperature", 22.5},
                        {"humidity", 45.0},
                        {"air_recycling_rate", std::string("Optimal")},
                        {"backup_systems", true}
                    },
                    {15.2, 5.1, 8.7}
                },
                {
                    "Propulsion",
                    std::nullopt, // Under maintenance, efficiency unknown
                    false, // Offline
                    {
                        {"thrust_capacity", 0.0},
                        {"fuel_injection_rate", 0.0},
                        {"engine_temperature", 15.0}, // Cold
                        {"maintenance_crew", std::string("Engineering Team Alpha")},
                        {"estimated_repair_time", 24.5}
                    },
                    {0.0, 0.0, 2.1} // Minimal power for monitoring
                },
                {
                    "Communications",
                    87.3,
                    42, // Signal strength percentage
                    {
                        {"transmission_power", 75.0},
                        {"receiver_sensitivity", -95.5},
                        {"active_channels", 12},
                        {"encryption_status", std::string("Active")},
                        {"last_contact_earth", std::string("2024-03-15 14:30:00")}
                    },
                    {8.9, 0.0, 3.2}
                }
            };
            
            // Comprehensive system analysis
            std::cout << "SPACE STATION SYSTEM STATUS REPORT\n";
            std::cout << std::string(50, '=') << "\n";
            
            double total_power = 0.0, total_fuel = 0.0, total_coolant = 0.0;
            int operational_systems = 0;
            std::vector<std::string> critical_issues;
            
            for (const auto& system : station_systems) {
                std::cout << "\nSYSTEM: " << system.system_name << "\n";
                std::cout << std::string(system.system_name.length() + 8, '-') << "\n";
                
                // Efficiency analysis
                if (system.efficiency_rating) {
                    std::cout << "Efficiency: " << *system.efficiency_rating << "%";
                    if (*system.efficiency_rating > 95.0) {
                        std::cout << " (Excellent)";
                    } else if (*system.efficiency_rating > 80.0) {
                        std::cout << " (Good)";
                    } else {
                        std::cout << " (Needs attention)";
                        critical_issues.push_back(system.system_name + " low efficiency");
                    }
                    std::cout << "\n";
                    operational_systems++;
                } else {
                    std::cout << "Efficiency: Unknown (System offline/maintenance)\n";
                    critical_issues.push_back(system.system_name + " offline");
                }
                
                // Status analysis using variant visitor
                std::cout << "Status: ";
                std::visit([&system, &critical_issues](const auto& status) {
                    using T = std::decay_t<decltype(status)>;
                    if constexpr (std::is_same_v<T, std::string>) {
                        std::cout << status;
                        if (status != "Nominal" && status != "Optimal") {
                            critical_issues.push_back(system.system_name + " status: " + status);
                        }
                    } else if constexpr (std::is_same_v<T, int>) {
                        std::cout << status << "% signal strength";
                        if (status < 50) {
                            critical_issues.push_back(system.system_name + " signal weak");
                        }
                    } else if constexpr (std::is_same_v<T, bool>) {
                        std::cout << (status ? "Online" : "Offline");
                        if (!status) {
                            critical_issues.push_back(system.system_name + " offline");
                        }
                    }
                }, system.current_status);
                std::cout << "\n";
                
                // Resource usage analysis
                const auto& [power, fuel, coolant] = system.resource_usage;
                std::cout << "Resource Usage:\n";
                std::cout << "  Power: " << power << " kW\n";
                std::cout << "  Fuel: " << fuel << " units/hour\n";
                std::cout << "  Coolant: " << coolant << " liters/minute\n";
                
                total_power += power;
                total_fuel += fuel;
                total_coolant += coolant;
                
                // Diagnostic data analysis
                std::cout << "Diagnostics:\n";
                for (const auto& [param_name, value] : system.diagnostics) {
                    std::cout << "  " << param_name << ": ";
                    
                    if (value.type() == typeid(double)) {
                        std::cout << std::any_cast<double>(value);
                    } else if (value.type() == typeid(int)) {
                        std::cout << std::any_cast<int>(value);
                    } else if (value.type() == typeid(std::string)) {
                        std::cout << "\"" << std::any_cast<std::string>(value) << "\"";
                    } else if (value.type() == typeid(bool)) {
                        std::cout << (std::any_cast<bool>(value) ? "Enabled" : "Disabled");
                    } else {
                        std::cout << "[Unknown type]";
                    }
                    std::cout << "\n";
                }
            }
            
            // Summary report
            std::cout << "\n" << std::string(50, '=') << "\n";
            std::cout << "STATION SUMMARY\n";
            std::cout << std::string(50, '=') << "\n";
            std::cout << "Operational systems: " << operational_systems << "/" << station_systems.size() << "\n";
            std::cout << "Total power consumption: " << total_power << " kW\n";
            std::cout << "Total fuel consumption: " << total_fuel << " units/hour\n";
            std::cout << "Total coolant consumption: " << total_coolant << " liters/minute\n";
            
            if (!critical_issues.empty()) {
                std::cout << "\nCRITICAL ISSUES (" << critical_issues.size() << "):\n";
                for (size_t i = 0; i < critical_issues.size(); ++i) {
                    std::cout << i + 1 << ". " << critical_issues[i] << "\n";
                }
            } else {
                std::cout << "\nNo critical issues detected. Station operating normally.\n";
            }
            
            // Calculate overall station health
            double health_score = 0.0;
            int scored_systems = 0;
            
            for (const auto& system : station_systems) {
                if (system.efficiency_rating) {
                    health_score += *system.efficiency_rating;
                    scored_systems++;
                }
            }
            
            if (scored_systems > 0) {
                health_score /= scored_systems;
                std::cout << "\nOverall station health: " << std::fixed << std::setprecision(1) 
                          << health_score << "%";
                
                if (health_score > 90.0) {
                    std::cout << " (Excellent condition)";
                } else if (health_score > 75.0) {
                    std::cout << " (Good condition)";
                } else if (health_score > 50.0) {
                    std::cout << " (Fair condition - maintenance recommended)";
                } else {
                    std::cout << " (Poor condition - immediate attention required)";
                }
                std::cout << "\n";
            }
        }
    };
}

// Explicit instantiation of template functions for common types
template std::string AdvancedUtilities::format_pair<int, std::string>(const std::pair<int, std::string>&);
template std::string AdvancedUtilities::format_pair<std::string, double>(const std::pair<std::string, double>&);

// Main demonstration functions
void runSTLUtilitiesDemo() {
    STLUtilitiesDemo::runAllDemonstrations();
}

void runAdvancedUtilitiesDemo() {
    std::cout << "\n========== ADVANCED STL UTILITIES DEMONSTRATION ==========\n";
    
    AdvancedUtilities::UtilityPerformanceTest::compareContainerAccess();
    AdvancedUtilities::UtilityPerformanceTest::compareVariantVsAny();
    AdvancedUtilities::SpaceStationDataSystem::demonstrateComplexScenario();
    
    std::cout << "\n========== ADVANCED DEMONSTRATION COMPLETE ==========\n";
}

void demonstrateUtilityBestPractices() {
    std::cout << "\n========== STL UTILITIES BEST PRACTICES ==========\n";
    
    std::cout << "\n=== Best Practices Summary ===\n";
    
    std::cout << "\n1. std::pair:\n";
    std::cout << "   - Use for simple two-element associations\n";
    std::cout << "   - Prefer structured bindings (C++17) for access\n";
    std::cout << "   - Consider std::tuple for more than two elements\n";
    std::cout << "   - Use make_pair for type deduction when needed\n";
    
    std::cout << "\n2. std::tuple:\n";
    std::cout << "   - Use for multiple return values from functions\n";
    std::cout << "   - Structured bindings make access more readable\n";
    std::cout << "   - Consider named structs for better readability in complex cases\n";
    std::cout << "   - Use tuple_cat for combining tuples\n";
    
    std::cout << "\n3. std::optional:\n";
    std::cout << "   - Use instead of pointers for nullable values\n";
    std::cout << "   - Always check has_value() or use implicit bool conversion\n";
    std::cout << "   - Use value_or() for default values\n";
    std::cout << "   - Prefer optional over exceptions for expected failures\n";
    
    std::cout << "\n4. std::variant:\n";
    std::cout << "   - Use for type-safe unions\n";
    std::cout << "   - Prefer std::visit with generic lambdas\n";
    std::cout << "   - Consider std::holds_alternative for type checking\n";
    std::cout << "   - Use get_if for safe access without exceptions\n";
    
    std::cout << "\n5. std::any:\n";
    std::cout << "   - Use sparingly, prefer variant when types are known\n";
    std::cout << "   - Always check type() before any_cast\n";
    std::cout << "   - Use any_cast with pointers for safe casting\n";
    std::cout << "   - Consider performance implications of type erasure\n";
    
    std::cout << "\n=== Performance Considerations ===\n";
    std::cout << "- pair: Zero overhead, optimal performance\n";
    std::cout << "- tuple: Near-zero overhead with good compilers\n";
    std::cout << "- optional: Minimal overhead, better than pointers\n";
    std::cout << "- variant: Union-like storage, visitor pattern efficient\n";
    std::cout << "- any: Type erasure overhead, use judiciously\n";
    
    std::cout << "\n=== When to Use Each Utility ===\n";
    std::cout << "- pair: Key-value pairs, coordinate pairs, simple associations\n";
    std::cout << "- tuple: Multiple return values, heterogeneous data groups\n";
    std::cout << "- optional: Nullable values, optional function parameters\n";
    std::cout << "- variant: Sum types, state machines, error handling\n";
    std::cout << "- any: Configuration systems, plugin architectures\n";
    
    std::cout << "\n========== BEST PRACTICES SUMMARY COMPLETE ==========\n";
}

} // namespace CppVerseHub::STL