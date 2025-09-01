/**
 * @file SmartPointers.cpp
 * @brief Implementation of smart pointer demonstrations
 * @details File location: src/memory/SmartPointers.cpp
 */

#include "SmartPointers.hpp"
#include <algorithm>
#include <random>
#include <thread>
#include <atomic>
#include <cassert>

namespace CppVerseHub::Memory {

    // Static member initialization
    int Resource::counter_ = 0;

    // Spacecraft observer notification implementation
    void Spacecraft::notifyObservers() {
        auto it = observers_.begin();
        while (it != observers_.end()) {
            if (auto observer = it->lock()) {
                observer->onFuelLevelChanged(name_, current_fuel_);
                ++it;
            } else {
                // Remove expired observers
                it = observers_.erase(it);
            }
        }
    }

    // ResourceFactory implementations
    std::unique_ptr<Resource, std::function<void(Resource*)>>
    ResourceFactory::createWithCustomDeleter(const std::string& type, const std::string& name) {
        auto deleter = [](Resource* r) {
            std::cout << "Custom deleter called for resource: " << r->getName() << "\n";
            delete r;
        };
        
        if (type == "station") {
            return std::unique_ptr<Resource, std::function<void(Resource*)>>(
                new SpaceStation(name), deleter);
        } else if (type == "spacecraft") {
            return std::unique_ptr<Resource, std::function<void(Resource*)>>(
                new Spacecraft(name), deleter);
        }
        
        return nullptr;
    }

    // SmartPointerManager implementations
    void SmartPointerManager::demonstrateUniquePtr() {
        std::cout << "\n=== Unique Pointer Demonstration ===\n";
        
        // Basic unique_ptr usage
        auto station = std::make_unique<SpaceStation>("Alpha Station", 500);
        station->addInhabitants(100);
        station->process();
        
        // Transfer ownership
        auto transferred_station = std::move(station);
        assert(station == nullptr); // Original is now null
        
        transferred_station->addInhabitants(200);
        transferred_station->process();
        
        ++unique_ptr_count_;
        
        // Automatic cleanup when leaving scope
        std::cout << "Leaving unique_ptr scope...\n";
    }

    void SmartPointerManager::demonstrateUniquePtrArrays() {
        std::cout << "\n=== Unique Pointer Arrays ===\n";
        
        // Array of objects
        constexpr size_t array_size = 5;
        auto stations = std::make_unique<SpaceStation[]>(array_size);
        
        // Initialize array elements (not directly possible with make_unique for arrays)
        // Better approach: use vector with unique_ptr
        std::vector<std::unique_ptr<SpaceStation>> station_vector;
        
        for (int i = 0; i < static_cast<int>(array_size); ++i) {
            station_vector.push_back(
                std::make_unique<SpaceStation>("Station-" + std::to_string(i), 100 * (i + 1))
            );
        }
        
        // Process all stations
        for (auto& station : station_vector) {
            station->addInhabitants(50);
            station->process();
        }
        
        unique_ptr_count_ += array_size;
    }

    void SmartPointerManager::demonstrateUniquePtrPolymorphism() {
        std::cout << "\n=== Unique Pointer Polymorphism ===\n";
        
        // Vector of base class pointers
        std::vector<std::unique_ptr<Resource>> resources;
        
        resources.push_back(std::make_unique<SpaceStation>("Beta Station", 750));
        resources.push_back(std::make_unique<Spacecraft>("Explorer-1", 150.0));
        resources.push_back(std::make_unique<SpaceStation>("Gamma Station", 1200));
        
        // Polymorphic processing
        for (auto& resource : resources) {
            resource->process();
            
            // Dynamic casting with unique_ptr
            if (auto* spacecraft = dynamic_cast<Spacecraft*>(resource.get())) {
                spacecraft->consumeFuel(25.0);
                std::cout << "Fuel consumed, remaining: " << spacecraft->getFuelLevel() << "\n";
            }
        }
        
        unique_ptr_count_ += resources.size();
    }

    void SmartPointerManager::demonstrateUniquePtrCustomDeleter() {
        std::cout << "\n=== Unique Pointer Custom Deleter ===\n";
        
        auto resource = ResourceFactory::createWithCustomDeleter("station", "Delta Station");
        if (resource) {
            resource->process();
        }
        
        // Custom deleter for special cleanup
        auto special_cleanup = [](Resource* r) {
            std::cout << "Special cleanup procedure for: " << r->getName() << "\n";
            // Simulate special cleanup
            std::cout << "Saving state to persistent storage...\n";
            std::cout << "Notifying other systems...\n";
            delete r;
        };
        
        std::unique_ptr<Resource, decltype(special_cleanup)> special_resource(
            new Spacecraft("Special Craft"), special_cleanup
        );
        
        special_resource->process();
        
        ++unique_ptr_count_;
    }

    void SmartPointerManager::demonstrateSharedPtr() {
        std::cout << "\n=== Shared Pointer Demonstration ===\n";
        
        // Create shared resource
        auto shared_station = std::make_shared<SpaceStation>("Shared Station", 1000);
        std::cout << "Initial use count: " << shared_station.use_count() << "\n";
        
        // Share ownership
        std::vector<std::shared_ptr<SpaceStation>> multiple_owners;
        multiple_owners.push_back(shared_station);
        multiple_owners.push_back(shared_station);
        multiple_owners.push_back(shared_station);
        
        std::cout << "After sharing with vector: " << shared_station.use_count() << "\n";
        
        // Use the resource
        shared_station->addInhabitants(300);
        shared_station->process();
        
        // Remove some owners
        multiple_owners.pop_back();
        multiple_owners.pop_back();
        
        std::cout << "After removing owners: " << shared_station.use_count() << "\n";
        
        ++shared_ptr_count_;
    }

    void SmartPointerManager::demonstrateSharedPtrCircularReference() {
        std::cout << "\n=== Shared Pointer Circular Reference ===\n";
        
        // Demonstrate potential memory leak with circular references
        class Node {
        public:
            int value;
            std::shared_ptr<Node> next;
            std::weak_ptr<Node> parent; // Use weak_ptr to break cycle
            
            explicit Node(int v) : value(v) {
                std::cout << "Node " << value << " created\n";
            }
            
            ~Node() {
                std::cout << "Node " << value << " destroyed\n";
            }
        };
        
        auto node1 = std::make_shared<Node>(1);
        auto node2 = std::make_shared<Node>(2);
        auto node3 = std::make_shared<Node>(3);
        
        // Create chain without circular reference
        node1->next = node2;
        node2->next = node3;
        node2->parent = node1; // weak_ptr doesn't increase ref count
        node3->parent = node2;
        
        std::cout << "Node1 use count: " << node1.use_count() << "\n";
        std::cout << "Node2 use count: " << node2.use_count() << "\n";
        std::cout << "Node3 use count: " << node3.use_count() << "\n";
        
        // Nodes will be properly cleaned up when leaving scope
        shared_ptr_count_ += 3;
    }

    void SmartPointerManager::demonstrateSharedPtrCustomDeleter() {
        std::cout << "\n=== Shared Pointer Custom Deleter ===\n";
        
        auto custom_deleter = [](Resource* r) {
            std::cout << "Shared pointer custom deleter for: " << r->getName() << "\n";
            delete r;
        };
        
        auto shared_resource = std::shared_ptr<Resource>(
            new Spacecraft("Shared Craft"), custom_deleter
        );
        
        // Create multiple copies
        auto copy1 = shared_resource;
        auto copy2 = shared_resource;
        
        std::cout << "Shared resource use count: " << shared_resource.use_count() << "\n";
        
        shared_resource->process();
        
        ++shared_ptr_count_;
    }

    void SmartPointerManager::demonstrateSharedPtrAliasing() {
        std::cout << "\n=== Shared Pointer Aliasing ===\n";
        
        struct ComplexResource {
            std::string name;
            std::vector<int> data;
            double value;
            
            ComplexResource(const std::string& n) : name(n), data{1, 2, 3, 4, 5}, value(42.0) {}
        };
        
        auto complex = std::make_shared<ComplexResource>("Complex Resource");
        
        // Aliasing constructor - shared_ptr that shares ownership with complex
        // but points to a member of the managed object
        std::shared_ptr<std::string> name_ptr(complex, &complex->name);
        std::shared_ptr<std::vector<int>> data_ptr(complex, &complex->data);
        std::shared_ptr<double> value_ptr(complex, &complex->value);
        
        std::cout << "Complex resource use count: " << complex.use_count() << "\n";
        std::cout << "Name through aliased pointer: " << *name_ptr << "\n";
        std::cout << "Data size through aliased pointer: " << data_ptr->size() << "\n";
        std::cout << "Value through aliased pointer: " << *value_ptr << "\n";
        
        // The complex object won't be destroyed until all aliased pointers are gone
        complex.reset();
        std::cout << "After resetting complex, name still valid: " << *name_ptr << "\n";
        
        ++shared_ptr_count_;
    }

    void SmartPointerManager::demonstrateWeakPtr() {
        std::cout << "\n=== Weak Pointer Demonstration ===\n";
        
        std::weak_ptr<SpaceStation> weak_station;
        
        {
            auto shared_station = std::make_shared<SpaceStation>("Temporary Station", 500);
            weak_station = shared_station;
            
            std::cout << "Weak pointer expired: " << weak_station.expired() << "\n";
            std::cout << "Weak pointer use count: " << weak_station.use_count() << "\n";
            
            // Convert weak_ptr to shared_ptr
            if (auto locked = weak_station.lock()) {
                locked->addInhabitants(100);
                locked->process();
            }
        }
        
        // shared_ptr is now destroyed
        std::cout << "After scope, weak pointer expired: " << weak_station.expired() << "\n";
        
        // Trying to lock expired weak_ptr returns null
        if (auto locked = weak_station.lock()) {
            std::cout << "This won't print\n";
        } else {
            std::cout << "Weak pointer is expired, cannot lock\n";
        }
        
        ++weak_ptr_count_;
    }

    void SmartPointerManager::demonstrateWeakPtrObserver() {
        std::cout << "\n=== Weak Pointer Observer Pattern ===\n";
        
        auto spacecraft = std::make_shared<Spacecraft>("Observer Craft", 200.0);
        auto mission_control = std::make_shared<MissionControl>("Houston");
        
        // Add observer using weak_ptr
        spacecraft->addObserver(mission_control);
        
        spacecraft->process(); // Initial fuel level
        spacecraft->consumeFuel(50.0);
        spacecraft->process(); // After consumption
        spacecraft->consumeFuel(140.0); // Should trigger low fuel warning
        spacecraft->process();
        
        ++weak_ptr_count_;
    }

    void SmartPointerManager::demonstrateWeakPtrCache() {
        std::cout << "\n=== Weak Pointer Cache ===\n";
        
        // Simulate cache with weak_ptr to avoid keeping objects alive
        addToCache("cached_station", std::make_shared<SpaceStation>("Cached Station", 800));
        
        // Retrieve from cache
        if (auto cached = getCachedResource("cached_station")) {
            cached->process();
            std::cout << "Retrieved from cache successfully\n";
        }
        
        // After some time, the cached object might expire
        cleanupExpiredReferences();
        
        ++weak_ptr_count_;
    }

    void SmartPointerManager::demonstrateSmartPtrConversions() {
        std::cout << "\n=== Smart Pointer Conversions ===\n";
        
        // unique_ptr to shared_ptr
        auto unique_station = std::make_unique<SpaceStation>("Conversion Station", 600);
        auto shared_station = SmartPtrUtils::to_shared(std::move(unique_station));
        
        std::cout << "Converted unique_ptr to shared_ptr, use count: " 
                  << shared_station.use_count() << "\n";
        
        // shared_ptr to weak_ptr
        std::weak_ptr<SpaceStation> weak_station = shared_station;
        
        // Dynamic casting
        auto unique_resource = std::make_unique<SpaceStation>("Dynamic Station", 400);
        auto dynamic_station = SmartPtrUtils::dynamic_unique_cast<SpaceStation>(
            std::move(unique_resource));
        
        if (dynamic_station) {
            std::cout << "Dynamic cast successful\n";
            dynamic_station->process();
        }
        
        ++unique_ptr_count_;
        ++shared_ptr_count_;
        ++weak_ptr_count_;
    }

    void SmartPointerManager::demonstrateSmartPtrPerformance() {
        std::cout << "\n=== Smart Pointer Performance ===\n";
        
        SmartPtrBenchmark::comparePerformance(1000);
    }

    void SmartPointerManager::demonstratePimplIdiom() {
        std::cout << "\n=== PIMPL Idiom with Smart Pointers ===\n";
        
        PimplExample pimpl;
        pimpl.setValue(42);
        pimpl.doSomething();
        std::cout << "PIMPL value: " << pimpl.getValue() << "\n";
        
        // Demonstrate move semantics
        auto moved_pimpl = std::move(pimpl);
        moved_pimpl.doSomething();
        
        ++unique_ptr_count_;
    }

    void SmartPointerManager::demonstrateSmartPtrThreadSafety() {
        std::cout << "\n=== Smart Pointer Thread Safety ===\n";
        
        auto shared_resource = std::make_shared<SpaceStation>("Thread Safe Station", 1000);
        std::atomic<int> thread_count{0};
        
        // Launch multiple threads that share the resource
        std::vector<std::thread> threads;
        
        for (int i = 0; i < 4; ++i) {
            threads.emplace_back([shared_resource, &thread_count, i]() {
                ++thread_count;
                
                // Each thread gets its own copy of shared_ptr
                auto local_copy = shared_resource;
                
                std::cout << "Thread " << i << " use count: " << local_copy.use_count() << "\n";
                
                // Simulate work
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                
                local_copy->addInhabitants(10);
                --thread_count;
            });
        }
        
        // Wait for all threads
        for (auto& thread : threads) {
            thread.join();
        }
        
        std::cout << "Final station population: " << shared_resource->getPopulation() << "\n";
        std::cout << "Final use count: " << shared_resource.use_count() << "\n";
        
        ++shared_ptr_count_;
    }

    void SmartPointerManager::runAllDemonstrations() {
        std::cout << "\n========== SMART POINTER COMPREHENSIVE DEMO ==========\n";
        
        demonstrateUniquePtr();
        demonstrateUniquePtrArrays();
        demonstrateUniquePtrPolymorphism();
        demonstrateUniquePtrCustomDeleter();
        
        demonstrateSharedPtr();
        demonstrateSharedPtrCircularReference();
        demonstrateSharedPtrCustomDeleter();
        demonstrateSharedPtrAliasing();
        
        demonstrateWeakPtr();
        demonstrateWeakPtrObserver();
        demonstrateWeakPtrCache();
        
        demonstrateSmartPtrConversions();
        demonstrateSmartPtrPerformance();
        demonstratePimplIdiom();
        demonstrateSmartPtrThreadSafety();
        
        printStatistics();
        
        std::cout << "\n========== DEMO COMPLETE ==========\n";
    }

    void SmartPointerManager::printStatistics() const {
        std::cout << "\n=== Smart Pointer Usage Statistics ===\n";
        std::cout << "Unique pointers demonstrated: " << unique_ptr_count_ << "\n";
        std::cout << "Shared pointers demonstrated: " << shared_ptr_count_ << "\n";
        std::cout << "Weak pointers demonstrated: " << weak_ptr_count_ << "\n";
        std::cout << "Total smart pointers: " 
                  << (unique_ptr_count_ + shared_ptr_count_ + weak_ptr_count_) << "\n";
    }

    void SmartPointerManager::cleanupExpiredReferences() {
        auto it = resource_cache_.begin();
        while (it != resource_cache_.end()) {
            if (it->second.expired()) {
                std::cout << "Removing expired cache entry: " << it->first << "\n";
                it = resource_cache_.erase(it);
            } else {
                ++it;
            }
        }
    }

    std::shared_ptr<Resource> SmartPointerManager::getCachedResource(const std::string& name) {
        auto it = resource_cache_.find(name);
        if (it != resource_cache_.end()) {
            if (auto shared = it->second.lock()) {
                return shared;
            } else {
                resource_cache_.erase(it);
            }
        }
        return nullptr;
    }

    void SmartPointerManager::addToCache(const std::string& name, std::shared_ptr<Resource> resource) {
        resource_cache_[name] = resource;
    }

    // PimplExample implementation
    class PimplExample::Implementation {
    public:
        Implementation() : value_(0), data_{"PIMPL", "Implementation", "Demo"} {}
        
        void doSomething() {
            std::cout << "PIMPL doing something with value: " << value_ << "\n";
            for (const auto& str : data_) {
                std::cout << "  - " << str << "\n";
            }
        }
        
        void setValue(int value) { value_ = value; }
        int getValue() const { return value_; }
        
    private:
        int value_;
        std::vector<std::string> data_;
    };

    PimplExample::PimplExample() : pImpl_(std::make_unique<Implementation>()) {}

    PimplExample::~PimplExample() = default;

    PimplExample::PimplExample(PimplExample&&) noexcept = default;
    PimplExample& PimplExample::operator=(PimplExample&&) noexcept = default;

    void PimplExample::doSomething() {
        pImpl_->doSomething();
    }

    void PimplExample::setValue(int value) {
        pImpl_->setValue(value);
    }

    int PimplExample::getValue() const {
        return pImpl_->getValue();
    }

    // SmartPtrBenchmark implementations
    SmartPtrBenchmark::BenchmarkResult 
    SmartPtrBenchmark::benchmarkRawPointer(size_t iterations) {
        auto start = std::chrono::high_resolution_clock::now();
        
        // Creation benchmark
        std::vector<Resource*> raw_ptrs;
        raw_ptrs.reserve(iterations);
        
        for (size_t i = 0; i < iterations; ++i) {
            raw_ptrs.push_back(new SpaceStation("Raw-" + std::to_string(i)));
        }
        
        auto creation_end = std::chrono::high_resolution_clock::now();
        
        // Access benchmark
        for (auto* ptr : raw_ptrs) {
            ptr->process();
        }
        
        auto access_end = std::chrono::high_resolution_clock::now();
        
        // Destruction benchmark
        for (auto* ptr : raw_ptrs) {
            delete ptr;
        }
        
        auto destruction_end = std::chrono::high_resolution_clock::now();
        
        return {
            std::chrono::duration_cast<std::chrono::microseconds>(creation_end - start),
            std::chrono::duration_cast<std::chrono::microseconds>(access_end - creation_end),
            std::chrono::duration_cast<std::chrono::microseconds>(destruction_end - access_end),
            sizeof(Resource*) * iterations
        };
    }

    SmartPtrBenchmark::BenchmarkResult 
    SmartPtrBenchmark::benchmarkUniquePtr(size_t iterations) {
        auto start = std::chrono::high_resolution_clock::now();
        
        // Creation benchmark
        std::vector<std::unique_ptr<Resource>> unique_ptrs;
        unique_ptrs.reserve(iterations);
        
        for (size_t i = 0; i < iterations; ++i) {
            unique_ptrs.push_back(
                std::make_unique<SpaceStation>("Unique-" + std::to_string(i))
            );
        }
        
        auto creation_end = std::chrono::high_resolution_clock::now();
        
        // Access benchmark
        for (auto& ptr : unique_ptrs) {
            ptr->process();
        }
        
        auto access_end = std::chrono::high_resolution_clock::now();
        
        // Destruction is automatic
        unique_ptrs.clear();
        
        auto destruction_end = std::chrono::high_resolution_clock::now();
        
        return {
            std::chrono::duration_cast<std::chrono::microseconds>(creation_end - start),
            std::chrono::duration_cast<std::chrono::microseconds>(access_end - creation_end),
            std::chrono::duration_cast<std::chrono::microseconds>(destruction_end - access_end),
            sizeof(std::unique_ptr<Resource>) * iterations
        };
    }

    SmartPtrBenchmark::BenchmarkResult 
    SmartPtrBenchmark::benchmarkSharedPtr(size_t iterations) {
        auto start = std::chrono::high_resolution_clock::now();
        
        // Creation benchmark
        std::vector<std::shared_ptr<Resource>> shared_ptrs;
        shared_ptrs.reserve(iterations);
        
        for (size_t i = 0; i < iterations; ++i) {
            shared_ptrs.push_back(
                std::make_shared<SpaceStation>("Shared-" + std::to_string(i))
            );
        }
        
        auto creation_end = std::chrono::high_resolution_clock::now();
        
        // Access benchmark
        for (auto& ptr : shared_ptrs) {
            ptr->process();
        }
        
        auto access_end = std::chrono::high_resolution_clock::now();
        
        // Destruction is automatic
        shared_ptrs.clear();
        
        auto destruction_end = std::chrono::high_resolution_clock::now();
        
        return {
            std::chrono::duration_cast<std::chrono::microseconds>(creation_end - start),
            std::chrono::duration_cast<std::chrono::microseconds>(access_end - creation_end),
            std::chrono::duration_cast<std::chrono::microseconds>(destruction_end - access_end),
            sizeof(std::shared_ptr<Resource>) * iterations
        };
    }

    void SmartPtrBenchmark::comparePerformance(size_t iterations) {
        std::cout << "Benchmarking " << iterations << " iterations...\n";
        
        auto raw_result = benchmarkRawPointer(iterations);
        auto unique_result = benchmarkUniquePtr(iterations);
        auto shared_result = benchmarkSharedPtr(iterations);
        
        std::cout << "\nResults (microseconds):\n";
        std::cout << "                  Creation    Access    Destruction   Memory\n";
        std::cout << "Raw Pointer:      " << raw_result.creation_time.count() 
                  << "        " << raw_result.access_time.count()
                  << "       " << raw_result.destruction_time.count()
                  << "          " << raw_result.memory_usage << " bytes\n";
        
        std::cout << "Unique Pointer:   " << unique_result.creation_time.count()
                  << "        " << unique_result.access_time.count()
                  << "       " << unique_result.destruction_time.count()
                  << "          " << unique_result.memory_usage << " bytes\n";
        
        std::cout << "Shared Pointer:   " << shared_result.creation_time.count()
                  << "        " << shared_result.access_time.count()
                  << "       " << shared_result.destruction_time.count()
                  << "          " << shared_result.memory_usage << " bytes\n";
    }

} // namespace CppVerseHub::Memory