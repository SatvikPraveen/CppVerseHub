/**
 * @file SmartPointers.hpp
 * @brief Comprehensive demonstration of smart pointer usage in C++
 * @details File location: src/memory/SmartPointers.hpp
 * 
 * This file demonstrates advanced usage of unique_ptr, shared_ptr, and weak_ptr
 * with real-world scenarios in a space simulation context.
 */

#ifndef SMARTPOINTERS_HPP
#define SMARTPOINTERS_HPP

#include <memory>
#include <vector>
#include <string>
#include <iostream>
#include <functional>
#include <unordered_map>
#include <chrono>

namespace CppVerseHub::Memory {

    /**
     * @class Resource
     * @brief Base class demonstrating proper resource management
     */
    class Resource {
    public:
        explicit Resource(const std::string& name) : name_(name), id_(++counter_) {
            std::cout << "Resource '" << name_ << "' (ID: " << id_ << ") created\n";
        }
        
        virtual ~Resource() {
            std::cout << "Resource '" << name_ << "' (ID: " << id_ << ") destroyed\n";
        }
        
        const std::string& getName() const { return name_; }
        int getId() const { return id_; }
        
        virtual void process() = 0;
        
    protected:
        std::string name_;
        int id_;
        static int counter_;
    };

    /**
     * @class SpaceStation
     * @brief Concrete resource representing a space station
     */
    class SpaceStation : public Resource {
    public:
        explicit SpaceStation(const std::string& name, int capacity = 1000)
            : Resource(name), capacity_(capacity), current_population_(0) {}
        
        void process() override {
            std::cout << "Processing space station '" << name_ 
                      << "' - Population: " << current_population_ 
                      << "/" << capacity_ << "\n";
        }
        
        void addInhabitants(int count) {
            current_population_ = std::min(current_population_ + count, capacity_);
        }
        
        int getCapacity() const { return capacity_; }
        int getPopulation() const { return current_population_; }
        
    private:
        int capacity_;
        int current_population_;
    };

    /**
     * @class Spacecraft
     * @brief Resource with observer pattern using weak_ptr
     */
    class Spacecraft : public Resource {
    public:
        explicit Spacecraft(const std::string& name, double fuel_capacity = 100.0)
            : Resource(name), fuel_capacity_(fuel_capacity), current_fuel_(fuel_capacity) {}
        
        void process() override {
            std::cout << "Processing spacecraft '" << name_ 
                      << "' - Fuel: " << current_fuel_ 
                      << "/" << fuel_capacity_ << "\n";
            
            // Notify observers
            notifyObservers();
        }
        
        void consumeFuel(double amount) {
            current_fuel_ = std::max(0.0, current_fuel_ - amount);
        }
        
        void refuel(double amount) {
            current_fuel_ = std::min(fuel_capacity_, current_fuel_ + amount);
        }
        
        double getFuelLevel() const { return current_fuel_; }
        
        // Observer pattern with weak_ptr
        void addObserver(std::weak_ptr<class FuelObserver> observer) {
            observers_.push_back(observer);
        }
        
    private:
        double fuel_capacity_;
        double current_fuel_;
        std::vector<std::weak_ptr<class FuelObserver>> observers_;
        
        void notifyObservers();
    };

    /**
     * @class FuelObserver
     * @brief Observer for spacecraft fuel levels
     */
    class FuelObserver {
    public:
        explicit FuelObserver(const std::string& name) : name_(name) {}
        virtual ~FuelObserver() = default;
        
        virtual void onFuelLevelChanged(const std::string& spacecraft_name, double fuel_level) = 0;
        const std::string& getName() const { return name_; }
        
    private:
        std::string name_;
    };

    /**
     * @class MissionControl
     * @brief Concrete observer for monitoring spacecraft
     */
    class MissionControl : public FuelObserver {
    public:
        explicit MissionControl(const std::string& name) : FuelObserver(name) {}
        
        void onFuelLevelChanged(const std::string& spacecraft_name, double fuel_level) override {
            std::cout << "Mission Control '" << getName() << "' notified: " 
                      << spacecraft_name << " fuel level: " << fuel_level << "\n";
            
            if (fuel_level < 20.0) {
                std::cout << "WARNING: Low fuel alert for " << spacecraft_name << "!\n";
            }
        }
    };

    /**
     * @class ResourceFactory
     * @brief Factory class demonstrating smart pointer factory patterns
     */
    class ResourceFactory {
    public:
        // Unique pointer factory
        template<typename T, typename... Args>
        static std::unique_ptr<T> createUnique(Args&&... args) {
            return std::make_unique<T>(std::forward<Args>(args)...);
        }
        
        // Shared pointer factory
        template<typename T, typename... Args>
        static std::shared_ptr<T> createShared(Args&&... args) {
            return std::make_shared<T>(std::forward<Args>(args)...);
        }
        
        // Custom deleter example
        static std::unique_ptr<Resource, std::function<void(Resource*)>> 
        createWithCustomDeleter(const std::string& type, const std::string& name);
    };

    /**
     * @class SmartPointerManager
     * @brief Comprehensive smart pointer usage examples
     */
    class SmartPointerManager {
    public:
        SmartPointerManager() = default;
        ~SmartPointerManager() = default;

        // Unique pointer demonstrations
        void demonstrateUniquePtr();
        void demonstrateUniquePtrArrays();
        void demonstrateUniquePtrPolymorphism();
        void demonstrateUniquePtrCustomDeleter();
        
        // Shared pointer demonstrations
        void demonstrateSharedPtr();
        void demonstrateSharedPtrCircularReference();
        void demonstrateSharedPtrCustomDeleter();
        void demonstrateSharedPtrAliasing();
        
        // Weak pointer demonstrations
        void demonstrateWeakPtr();
        void demonstrateWeakPtrObserver();
        void demonstrateWeakPtrCache();
        
        // Advanced patterns
        void demonstrateSmartPtrConversions();
        void demonstrateSmartPtrPerformance();
        void demonstratePimplIdiom();
        void demonstrateSmartPtrThreadSafety();
        
        // Utility functions
        void runAllDemonstrations();
        void printStatistics() const;
        
    private:
        // Cached resources using weak_ptr
        std::unordered_map<std::string, std::weak_ptr<Resource>> resource_cache_;
        
        // Statistics
        mutable size_t unique_ptr_count_ = 0;
        mutable size_t shared_ptr_count_ = 0;
        mutable size_t weak_ptr_count_ = 0;
        
        // Helper functions
        void cleanupExpiredReferences();
        std::shared_ptr<Resource> getCachedResource(const std::string& name);
        void addToCache(const std::string& name, std::shared_ptr<Resource> resource);
    };

    /**
     * @class PimplExample
     * @brief Demonstrates PIMPL (Pointer to Implementation) idiom
     */
    class PimplExample {
    public:
        PimplExample();
        ~PimplExample();
        
        // Non-copyable by default due to unique_ptr
        PimplExample(const PimplExample&) = delete;
        PimplExample& operator=(const PimplExample&) = delete;
        
        // Movable
        PimplExample(PimplExample&&) noexcept;
        PimplExample& operator=(PimplExample&&) noexcept;
        
        void doSomething();
        void setValue(int value);
        int getValue() const;
        
    private:
        // Forward declaration and unique_ptr for PIMPL
        class Implementation;
        std::unique_ptr<Implementation> pImpl_;
    };

    /**
     * @class SmartPtrBenchmark
     * @brief Performance benchmarking for smart pointers
     */
    class SmartPtrBenchmark {
    public:
        struct BenchmarkResult {
            std::chrono::microseconds creation_time;
            std::chrono::microseconds access_time;
            std::chrono::microseconds destruction_time;
            size_t memory_usage;
        };
        
        static BenchmarkResult benchmarkRawPointer(size_t iterations);
        static BenchmarkResult benchmarkUniquePtr(size_t iterations);
        static BenchmarkResult benchmarkSharedPtr(size_t iterations);
        
        static void comparePerformance(size_t iterations = 10000);
    };

    // Utility functions
    namespace SmartPtrUtils {
        
        /**
         * @brief Safe dynamic cast for smart pointers
         */
        template<typename Derived, typename Base>
        std::unique_ptr<Derived> dynamic_unique_cast(std::unique_ptr<Base> ptr) {
            if (auto* derived = dynamic_cast<Derived*>(ptr.get())) {
                ptr.release();
                return std::unique_ptr<Derived>(derived);
            }
            return nullptr;
        }
        
        /**
         * @brief Convert unique_ptr to shared_ptr
         */
        template<typename T>
        std::shared_ptr<T> to_shared(std::unique_ptr<T> ptr) {
            return std::shared_ptr<T>(ptr.release());
        }
        
        /**
         * @brief Check if weak_ptr is expired safely
         */
        template<typename T>
        bool is_expired(const std::weak_ptr<T>& weak) {
            return weak.expired();
        }
        
        /**
         * @brief Get use count safely
         */
        template<typename T>
        long get_use_count(const std::shared_ptr<T>& shared) {
            return shared.use_count();
        }
    }

} // namespace CppVerseHub::Memory

#endif // SMARTPOINTERS_HPP