// File: tests/unit_tests/memory_tests/SmartPointerTests.cpp
// Smart pointer usage tests for CppVerseHub memory management showcase

#include <catch2/catch.hpp>
#include <memory>
#include <vector>
#include <string>
#include <atomic>
#include <thread>
#include <functional>
#include <weak_ptr>

// Include core classes for testing
#include "Planet.hpp"
#include "Fleet.hpp"
#include "Mission.hpp"
#include "MemoryTracker.hpp"

using namespace CppVerseHub::Core;
using namespace CppVerseHub::Utils;

/**
 * @brief Test fixture for smart pointer tests
 */
class SmartPointerTestFixture {
public:
    SmartPointerTestFixture() {
        MemoryTracker::resetCounters();
    }
    
    ~SmartPointerTestFixture() {
        MemoryTracker::printMemoryStats("Test completion");
    }
};

/**
 * @brief Test class with tracking capabilities
 */
class TrackedObject {
private:
    static std::atomic<int> instanceCount_;
    static std::atomic<int> totalCreated_;
    int id_;
    std::string name_;
    
public:
    explicit TrackedObject(const std::string& name = "TrackedObject")
        : id_(totalCreated_.fetch_add(1) + 1), name_(name) {
        instanceCount_.fetch_add(1);
    }
    
    TrackedObject(const TrackedObject& other)
        : id_(totalCreated_.fetch_add(1) + 1), name_(other.name_ + "_copy") {
        instanceCount_.fetch_add(1);
    }
    
    ~TrackedObject() {
        instanceCount_.fetch_sub(1);
    }
    
    int getId() const { return id_; }
    const std::string& getName() const { return name_; }
    
    void setName(const std::string& name) { name_ = name; }
    
    static int getInstanceCount() { return instanceCount_.load(); }
    static int getTotalCreated() { return totalCreated_.load(); }
    static void resetCounters() { 
        instanceCount_.store(0); 
        totalCreated_.store(0); 
    }
};

std::atomic<int> TrackedObject::instanceCount_{0};
std::atomic<int> TrackedObject::totalCreated_{0};

/**
 * @brief Test class demonstrating resource management
 */
class ResourceHolder {
private:
    std::unique_ptr<int[]> data_;
    size_t size_;
    std::string name_;
    
public:
    ResourceHolder(const std::string& name, size_t size)
        : data_(std::make_unique<int[]>(size)), size_(size), name_(name) {
        // Initialize data
        for (size_t i = 0; i < size_; ++i) {
            data_[i] = static_cast<int>(i);
        }
    }
    
    // Move constructor
    ResourceHolder(ResourceHolder&& other) noexcept
        : data_(std::move(other.data_)), size_(other.size_), name_(std::move(other.name_)) {
        other.size_ = 0;
    }
    
    // Move assignment
    ResourceHolder& operator=(ResourceHolder&& other) noexcept {
        if (this != &other) {
            data_ = std::move(other.data_);
            size_ = other.size_;
            name_ = std::move(other.name_);
            other.size_ = 0;
        }
        return *this;
    }
    
    // Disable copy operations
    ResourceHolder(const ResourceHolder&) = delete;
    ResourceHolder& operator=(const ResourceHolder&) = delete;
    
    size_t getSize() const { return size_; }
    const std::string& getName() const { return name_; }
    
    int getData(size_t index) const {
        return (index < size_) ? data_[index] : -1;
    }
    
    void setData(size_t index, int value) {
        if (index < size_) {
            data_[index] = value;
        }
    }
};

TEST_CASE_METHOD(SmartPointerTestFixture, "Unique Pointer Basics", "[smart-pointers][unique_ptr][basic]") {
    
    SECTION("Basic unique_ptr usage") {
        TrackedObject::resetCounters();
        
        {
            auto obj = std::make_unique<TrackedObject>("UniqueTest");
            REQUIRE(TrackedObject::getInstanceCount() == 1);
            REQUIRE(obj->getId() == 1);
            REQUIRE(obj->getName() == "UniqueTest");
        } // obj goes out of scope, destructor called
        
        REQUIRE(TrackedObject::getInstanceCount() == 0);
    }
    
    SECTION("unique_ptr move semantics") {
        TrackedObject::resetCounters();
        
        std::unique_ptr<TrackedObject> ptr1 = std::make_unique<TrackedObject>("MoveTest");
        REQUIRE(ptr1 != nullptr);
        REQUIRE(TrackedObject::getInstanceCount() == 1);
        
        // Move to another unique_ptr
        std::unique_ptr<TrackedObject> ptr2 = std::move(ptr1);
        REQUIRE(ptr1 == nullptr);
        REQUIRE(ptr2 != nullptr);
        REQUIRE(ptr2->getName() == "MoveTest");
        REQUIRE(TrackedObject::getInstanceCount() == 1); // Still only one instance
    }
    
    SECTION("unique_ptr with arrays") {
        const size_t arraySize = 100;
        auto intArray = std::make_unique<int[]>(arraySize);
        
        // Initialize array
        for (size_t i = 0; i < arraySize; ++i) {
            intArray[i] = static_cast<int>(i * 2);
        }
        
        // Verify array contents
        for (size_t i = 0; i < arraySize; ++i) {
            REQUIRE(intArray[i] == static_cast<int>(i * 2));
        }
    }
    
    SECTION("unique_ptr with custom deleter") {
        bool customDeleterCalled = false;
        
        {
            auto customDeleter = [&customDeleterCalled](TrackedObject* obj) {
                customDeleterCalled = true;
                delete obj;
            };
            
            std::unique_ptr<TrackedObject, decltype(customDeleter)> ptr(
                new TrackedObject("CustomDeleterTest"), customDeleter);
            
            REQUIRE(ptr != nullptr);
        } // Custom deleter should be called here
        
        REQUIRE(customDeleterCalled);
    }
    
    SECTION("unique_ptr in containers") {
        std::vector<std::unique_ptr<TrackedObject>> container;
        
        TrackedObject::resetCounters();
        
        // Add objects to container
        for (int i = 0; i < 5; ++i) {
            container.push_back(std::make_unique<TrackedObject>("Container_" + std::to_string(i)));
        }
        
        REQUIRE(container.size() == 5);
        REQUIRE(TrackedObject::getInstanceCount() == 5);
        
        // Access objects
        for (size_t i = 0; i < container.size(); ++i) {
            REQUIRE(container[i]->getName() == "Container_" + std::to_string(i));
        }
        
        // Remove middle element
        container.erase(container.begin() + 2);
        REQUIRE(container.size() == 4);
        REQUIRE(TrackedObject::getInstanceCount() == 4);
        
        container.clear();
        REQUIRE(TrackedObject::getInstanceCount() == 0);
    }
}

TEST_CASE_METHOD(SmartPointerTestFixture, "Shared Pointer Basics", "[smart-pointers][shared_ptr][basic]") {
    
    SECTION("Basic shared_ptr usage") {
        TrackedObject::resetCounters();
        
        {
            auto obj = std::make_shared<TrackedObject>("SharedTest");
            REQUIRE(obj.use_count() == 1);
            REQUIRE(TrackedObject::getInstanceCount() == 1);
            
            {
                auto obj2 = obj; // Share ownership
                REQUIRE(obj.use_count() == 2);
                REQUIRE(obj2.use_count() == 2);
                REQUIRE(TrackedObject::getInstanceCount() == 1); // Still one object
            } // obj2 goes out of scope
            
            REQUIRE(obj.use_count() == 1);
            REQUIRE(TrackedObject::getInstanceCount() == 1);
        } // obj goes out of scope, object destroyed
        
        REQUIRE(TrackedObject::getInstanceCount() == 0);
    }
    
    SECTION("shared_ptr copying and assignment") {
        TrackedObject::resetCounters();
        
        std::shared_ptr<TrackedObject> ptr1 = std::make_shared<TrackedObject>("CopyTest");
        std::shared_ptr<TrackedObject> ptr2;
        std::shared_ptr<TrackedObject> ptr3;
        
        REQUIRE(ptr1.use_count() == 1);
        
        ptr2 = ptr1; // Assignment
        REQUIRE(ptr1.use_count() == 2);
        REQUIRE(ptr2.use_count() == 2);
        
        ptr3 = std::shared_ptr<TrackedObject>(ptr1); // Copy constructor
        REQUIRE(ptr1.use_count() == 3);
        REQUIRE(ptr2.use_count() == 3);
        REQUIRE(ptr3.use_count() == 3);
        
        // All point to the same object
        REQUIRE(ptr1.get() == ptr2.get());
        REQUIRE(ptr1.get() == ptr3.get());
        
        ptr2.reset();
        REQUIRE(ptr1.use_count() == 2);
        REQUIRE(ptr3.use_count() == 2);
        REQUIRE(TrackedObject::getInstanceCount() == 1);
    }
    
    SECTION("shared_ptr with custom deleter") {
        bool customDeleterCalled = false;
        
        {
            auto customDeleter = [&customDeleterCalled](TrackedObject* obj) {
                customDeleterCalled = true;
                delete obj;
            };
            
            std::shared_ptr<TrackedObject> ptr(
                new TrackedObject("SharedCustomDeleterTest"), customDeleter);
            
            REQUIRE(ptr.use_count() == 1);
            
            // Create another shared_ptr
            auto ptr2 = ptr;
            REQUIRE(ptr.use_count() == 2);
        } // Both ptr and ptr2 go out of scope
        
        REQUIRE(customDeleterCalled);
    }
    
    SECTION("shared_ptr thread safety") {
        const int threadCount = 10;
        const int operationsPerThread = 1000;
        
        auto sharedObj = std::make_shared<TrackedObject>("ThreadTest");
        std::vector<std::thread> threads;
        std::atomic<int> totalOperations{0};
        
        for (int i = 0; i < threadCount; ++i) {
            threads.emplace_back([sharedObj, operationsPerThread, &totalOperations] {
                for (int j = 0; j < operationsPerThread; ++j) {
                    auto localCopy = sharedObj; // Thread-safe copy
                    totalOperations.fetch_add(1);
                } // localCopy destructor is thread-safe
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        REQUIRE(totalOperations.load() == threadCount * operationsPerThread);
        REQUIRE(TrackedObject::getInstanceCount() == 1); // Original object still exists
    }
}

TEST_CASE_METHOD(SmartPointerTestFixture, "Weak Pointer Usage", "[smart-pointers][weak_ptr][basic]") {
    
    SECTION("Basic weak_ptr functionality") {
        std::weak_ptr<TrackedObject> weakPtr;
        
        {
            auto sharedPtr = std::make_shared<TrackedObject>("WeakTest");
            weakPtr = sharedPtr;
            
            REQUIRE(!weakPtr.expired());
            REQUIRE(weakPtr.use_count() == 1);
            
            // Lock to get shared_ptr
            auto lockedPtr = weakPtr.lock();
            REQUIRE(lockedPtr != nullptr);
            REQUIRE(lockedPtr->getName() == "WeakTest");
            REQUIRE(weakPtr.use_count() == 2); // sharedPtr + lockedPtr
        } // sharedPtr goes out of scope
        
        REQUIRE(weakPtr.expired());
        REQUIRE(weakPtr.use_count() == 0);
        
        auto expiredLock = weakPtr.lock();
        REQUIRE(expiredLock == nullptr);
    }
    
    SECTION("Breaking circular references with weak_ptr") {
        class Node {
        public:
            std::shared_ptr<Node> next;
            std::weak_ptr<Node> parent; // Use weak_ptr to avoid circular reference
            std::string name;
            
            explicit Node(const std::string& n) : name(n) {}
        };
        
        TrackedObject::resetCounters();
        
        {
            auto root = std::make_shared<Node>("Root");
            auto child1 = std::make_shared<Node>("Child1");
            auto child2 = std::make_shared<Node>("Child2");
            
            // Create hierarchy
            root->next = child1;
            child1->parent = root; // weak_ptr - no circular reference
            child1->next = child2;
            child2->parent = child1; // weak_ptr - no circular reference
            
            // Verify relationships
            REQUIRE(root->next->name == "Child1");
            REQUIRE(!child1->parent.expired());
            REQUIRE(child1->parent.lock()->name == "Root");
            REQUIRE(child2->parent.lock()->name == "Child1");
            
        } // All nodes should be properly destroyed
        
        // If we had used shared_ptr for parent, we'd have circular references
        // and memory leaks. With weak_ptr, everything cleans up properly.
    }
    
    SECTION("Observer pattern with weak_ptr") {
        class Subject;
        
        class Observer {
        public:
            virtual ~Observer() = default;
            virtual void notify(const std::string& message) = 0;
        };
        
        class Subject {
        private:
            std::vector<std::weak_ptr<Observer>> observers_;
            
        public:
            void addObserver(std::shared_ptr<Observer> observer) {
                observers_.push_back(observer);
            }
            
            void notifyObservers(const std::string& message) {
                // Clean up expired observers
                observers_.erase(
                    std::remove_if(observers_.begin(), observers_.end(),
                        [](const std::weak_ptr<Observer>& wp) { return wp.expired(); }),
                    observers_.end()
                );
                
                // Notify remaining observers
                for (auto& weakObs : observers_) {
                    if (auto obs = weakObs.lock()) {
                        obs->notify(message);
                    }
                }
            }
            
            size_t getObserverCount() const {
                return observers_.size();
            }
        };
        
        class ConcreteObserver : public Observer {
        private:
            std::string name_;
            std::vector<std::string> receivedMessages_;
            
        public:
            explicit ConcreteObserver(const std::string& name) : name_(name) {}
            
            void notify(const std::string& message) override {
                receivedMessages_.push_back(message);
            }
            
            const std::vector<std::string>& getMessages() const {
                return receivedMessages_;
            }
        };
        
        Subject subject;
        
        {
            auto observer1 = std::make_shared<ConcreteObserver>("Observer1");
            auto observer2 = std::make_shared<ConcreteObserver>("Observer2");
            
            subject.addObserver(observer1);
            subject.addObserver(observer2);
            
            REQUIRE(subject.getObserverCount() == 2);
            
            subject.notifyObservers("Test Message");
            
            REQUIRE(observer1->getMessages().size() == 1);
            REQUIRE(observer2->getMessages().size() == 1);
            
            // observer2 goes out of scope
        }
        
        // After cleanup, only expired observers remain
        subject.notifyObservers("Second Message");
        REQUIRE(subject.getObserverCount() == 0); // Expired observers removed
    }
}

TEST_CASE_METHOD(SmartPointerTestFixture, "Real-World Smart Pointer Applications", "[smart-pointers][real-world][applications]") {
    
    SECTION("Planet management with unique_ptr") {
        std::vector<std::unique_ptr<Planet>> planets;
        
        // Create planets
        for (int i = 0; i < 5; ++i) {
            auto planet = std::make_unique<Planet>(
                "SmartPlanet_" + std::to_string(i),
                Vector3D{i * 100.0, i * 100.0, i * 100.0}
            );
            planet->setResourceAmount(ResourceType::MINERALS, 1000 + i * 200);
            planets.push_back(std::move(planet));
        }
        
        REQUIRE(planets.size() == 5);
        
        // Find richest planet
        auto richest = std::max_element(planets.begin(), planets.end(),
            [](const std::unique_ptr<Planet>& a, const std::unique_ptr<Planet>& b) {
                return a->getResourceAmount(ResourceType::MINERALS) < 
                       b->getResourceAmount(ResourceType::MINERALS);
            });
        
        REQUIRE(richest != planets.end());
        REQUIRE((*richest)->getResourceAmount(ResourceType::MINERALS) == 1800);
        
        // Remove planets with low resources
        planets.erase(
            std::remove_if(planets.begin(), planets.end(),
                [](const std::unique_ptr<Planet>& p) {
                    return p->getResourceAmount(ResourceType::MINERALS) < 1200;
                }),
            planets.end()
        );
        
        REQUIRE(planets.size() == 3); // Removed first 2 planets
    }
    
    SECTION("Fleet sharing with shared_ptr") {
        std::vector<std::shared_ptr<Fleet>> fleets;
        std::vector<std::shared_ptr<Fleet>> taskForces;
        
        // Create fleets
        for (int i = 0; i < 3; ++i) {
            auto fleet = std::make_shared<Fleet>(
                "SharedFleet_" + std::to_string(i),
                Vector3D{i * 200.0, i * 200.0, i * 200.0}
            );
            fleet->addShips(ShipType::FIGHTER, 10 + i * 5);
            fleets.push_back(fleet);
        }
        
        // Create task forces that share some fleets
        taskForces.push_back(fleets[0]); // Task force 1 uses fleet 0
        taskForces.push_back(fleets[1]); // Task force 2 uses fleet 1
        taskForces.push_back(fleets[0]); // Task force 3 also uses fleet 0
        
        REQUIRE(fleets[0].use_count() == 3); // fleet, taskForce[0], taskForce[2]
        REQUIRE(fleets[1].use_count() == 2); // fleet, taskForce[1]
        REQUIRE(fleets[2].use_count() == 1); // Only in fleets vector
        
        // Remove a fleet from main collection
        fleets.erase(fleets.begin()); // Remove fleet 0
        
        REQUIRE(fleets.size() == 2);
        REQUIRE(taskForces[0].use_count() == 2); // Still shared by 2 task forces
        
        // Task forces can still access the fleet
        REQUIRE(taskForces[0]->getName() == "SharedFleet_0");
        REQUIRE(taskForces[2]->getName() == "SharedFleet_0");
    }
    
    SECTION("Mission dependency management") {
        class MissionDependency {
        public:
            std::shared_ptr<Mission> prerequisite;
            std::weak_ptr<Mission> dependent; // Avoid circular references
            
            MissionDependency(std::shared_ptr<Mission> prereq, std::shared_ptr<Mission> dep)
                : prerequisite(prereq), dependent(dep) {}
            
            bool canExecuteDependent() const {
                auto dep = dependent.lock();
                return dep && prerequisite && 
                       prerequisite->getStatus() == MissionStatus::COMPLETED;
            }
        };
        
        // Create planets for missions
        auto planet1 = std::make_unique<Planet>("MissionPlanet1", Vector3D{0, 0, 0});
        auto planet2 = std::make_unique<Planet>("MissionPlanet2", Vector3D{100, 100, 100});
        
        // Create missions
        auto scoutMission = std::make_shared<ExploreMission>("ScoutMission", planet1.get());
        auto colonizeMission = std::make_shared<ExploreMission>("ColonizeMission", planet2.get());
        
        // Set up dependency: colonize depends on scout
        MissionDependency dependency(scoutMission, colonizeMission);
        
        REQUIRE(!dependency.canExecuteDependent()); // Scout not completed
        
        // Complete prerequisite
        scoutMission->complete();
        
        REQUIRE(dependency.canExecuteDependent()); // Now can execute colonize
        
        // Test that dependent mission can be destroyed without affecting prerequisite
        colonizeMission.reset();
        REQUIRE(dependency.dependent.expired());
        REQUIRE(scoutMission.use_count() == 1); // Only held by dependency.prerequisite
    }
    
    SECTION("Resource holder with unique_ptr") {
        std::vector<ResourceHolder> holders;
        
        // Create resource holders (move-only objects)
        for (int i = 0; i < 3; ++i) {
            holders.emplace_back("Holder_" + std::to_string(i), 1000 + i * 500);
        }
        
        REQUIRE(holders.size() == 3);
        REQUIRE(holders[0].getSize() == 1000);
        REQUIRE(holders[1].getSize() == 1500);
        REQUIRE(holders[2].getSize() == 2000);
        
        // Test data access
        REQUIRE(holders[0].getData(0) == 0);
        REQUIRE(holders[0].getData(99) == 99);
        REQUIRE(holders[1].getData(500) == 500);
        
        // Move a holder
        ResourceHolder movedHolder = std::move(holders[1]);
        REQUIRE(movedHolder.getSize() == 1500);
        REQUIRE(movedHolder.getName() == "Holder_1");
        REQUIRE(holders[1].getSize() == 0); // Moved from object
    }
}

TEST_CASE_METHOD(SmartPointerTestFixture, "Smart Pointer Performance Analysis", "[smart-pointers][performance][analysis]") {
    
    SECTION("unique_ptr vs raw pointer performance") {
        const int iterations = 1000000;
        
        // Test unique_ptr performance
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < iterations; ++i) {
            auto ptr = std::make_unique<TrackedObject>("PerformanceTest");
            ptr->setName("Modified");
            // Automatic cleanup
        }
        
        auto uniquePtrTime = std::chrono::high_resolution_clock::now() - start;
        
        // Test raw pointer performance (for comparison)
        start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < iterations; ++i) {
            TrackedObject* ptr = new TrackedObject("PerformanceTest");
            ptr->setName("Modified");
            delete ptr; // Manual cleanup
        }
        
        auto rawPtrTime = std::chrono::high_resolution_clock::now() - start;
        
        auto uniqueMs = std::chrono::duration_cast<std::chrono::milliseconds>(uniquePtrTime).count();
        auto rawMs = std::chrono::duration_cast<std::chrono::milliseconds>(rawPtrTime).count();
        
        INFO("unique_ptr time: " << uniqueMs << "ms");
        INFO("raw pointer time: " << rawMs << "ms");
        
        // unique_ptr should have minimal overhead compared to raw pointers
        REQUIRE(uniqueMs < rawMs * 2); // Should not be more than 2x slower
    }
    
    SECTION("shared_ptr vs unique_ptr performance") {
        const int iterations = 100000;
        
        // Test unique_ptr
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<std::unique_ptr<TrackedObject>> uniqueVec;
        for (int i = 0; i < iterations; ++i) {
            uniqueVec.push_back(std::make_unique<TrackedObject>("UniquePerf"));
        }
        uniqueVec.clear();
        
        auto uniqueTime = std::chrono::high_resolution_clock::now() - start;
        
        // Test shared_ptr
        start = std::chrono::high_resolution_clock::now();
        
        std::vector<std::shared_ptr<TrackedObject>> sharedVec;
        for (int i = 0; i < iterations; ++i) {
            sharedVec.push_back(std::make_shared<TrackedObject>("SharedPerf"));
        }
        sharedVec.clear();
        
        auto sharedTime = std::chrono::high_resolution_clock::now() - start;
        
        auto uniqueMs = std::chrono::duration_cast<std::chrono::milliseconds>(uniqueTime).count();
        auto sharedMs = std::chrono::duration_cast<std::chrono::milliseconds>(sharedTime).count();
        
        INFO("unique_ptr time: " << uniqueMs << "ms");
        INFO("shared_ptr time: " << sharedMs << "ms");
        
        // Both should complete in reasonable time
        REQUIRE(uniqueMs < 5000); // Less than 5 seconds
        REQUIRE(sharedMs < 5000); // Less than 5 seconds
    }
    
    SECTION("Memory usage comparison") {
        const int objectCount = 10000;
        
        // Measure unique_ptr memory pattern
        std::vector<std::unique_ptr<TrackedObject>> uniqueObjects;
        
        for (int i = 0; i < objectCount; ++i) {
            uniqueObjects.push_back(std::make_unique<TrackedObject>("MemTest"));
        }
        
        REQUIRE(uniqueObjects.size() == objectCount);
        REQUIRE(TrackedObject::getInstanceCount() == objectCount);
        
        uniqueObjects.clear();
        REQUIRE(TrackedObject::getInstanceCount() == 0);
        
        // Measure shared_ptr memory pattern
        std::vector<std::shared_ptr<TrackedObject>> sharedObjects;
        
        for (int i = 0; i < objectCount; ++i) {
            sharedObjects.push_back(std::make_shared<TrackedObject>("MemTest"));
        }
        
        REQUIRE(sharedObjects.size() == objectCount);
        REQUIRE(TrackedObject::getInstanceCount() == objectCount);
        
        // Create additional references
        auto additionalRefs = sharedObjects; // Copy vector - double references
        REQUIRE(TrackedObject::getInstanceCount() == objectCount); // Same objects
        
        sharedObjects.clear();
        REQUIRE(TrackedObject::getInstanceCount() == objectCount); // Still referenced
        
        additionalRefs.clear();
        REQUIRE(TrackedObject::getInstanceCount() == 0); // Now fully cleaned up
    }
}

TEST_CASE_METHOD(SmartPointerTestFixture, "Smart Pointer Best Practices", "[smart-pointers][best-practices][patterns]") {
    
    SECTION("Factory pattern with smart pointers") {
        class PlanetFactory {
        public:
            static std::unique_ptr<Planet> createPlanet(const std::string& type, 
                                                       const std::string& name,
                                                       const Vector3D& position) {
                auto planet = std::make_unique<Planet>(name, position);
                
                if (type == "mineral_rich") {
                    planet->setResourceAmount(ResourceType::MINERALS, 2000);
                    planet->setResourceAmount(ResourceType::ENERGY, 500);
                } else if (type == "energy_rich") {
                    planet->setResourceAmount(ResourceType::MINERALS, 500);
                    planet->setResourceAmount(ResourceType::ENERGY, 2000);
                } else if (type == "balanced") {
                    planet->setResourceAmount(ResourceType::MINERALS, 1000);
                    planet->setResourceAmount(ResourceType::ENERGY, 1000);
                }
                
                return planet;
            }
            
            static std::shared_ptr<Fleet> createFleet(const std::string& type,
                                                     const std::string& name,
                                                     const Vector3D& position) {
                auto fleet = std::make_shared<Fleet>(name, position);
                
                if (type == "scout") {
                    fleet->addShips(ShipType::FIGHTER, 5);
                } else if (type == "assault") {
                    fleet->addShips(ShipType::FIGHTER, 20);
                    fleet->addShips(ShipType::CRUISER, 5);
                    fleet->addShips(ShipType::BATTLESHIP, 2);
                } else if (type == "defense") {
                    fleet->addShips(ShipType::CRUISER, 10);
                    fleet->addShips(ShipType::BATTLESHIP, 5);
                }
                
                return fleet;
            }
        };
        
        // Create different types of planets
        auto mineralPlanet = PlanetFactory::createPlanet("mineral_rich", "MineralWorld", Vector3D{0, 0, 0});
        auto energyPlanet = PlanetFactory::createPlanet("energy_rich", "EnergyWorld", Vector3D{100, 100, 100});
        auto balancedPlanet = PlanetFactory::createPlanet("balanced", "BalancedWorld", Vector3D{200, 200, 200});
        
        REQUIRE(mineralPlanet->getResourceAmount(ResourceType::MINERALS) == 2000);
        REQUIRE(energyPlanet->getResourceAmount(ResourceType::ENERGY) == 2000);
        REQUIRE(balancedPlanet->getResourceAmount(ResourceType::MINERALS) == 1000);
        REQUIRE(balancedPlanet->getResourceAmount(ResourceType::ENERGY) == 1000);
        
        // Create different types of fleets
        auto scoutFleet = PlanetFactory::createFleet("scout", "ScoutFleet", Vector3D{0, 0, 0});
        auto assaultFleet = PlanetFactory::createFleet("assault", "AssaultFleet", Vector3D{50, 50, 50});
        
        REQUIRE(scoutFleet->getShipCount(ShipType::FIGHTER) == 5);
        REQUIRE(assaultFleet->getTotalShipCount() == 27); // 20 + 5 + 2
    }
    
    SECTION("RAII with smart pointers") {
        class ResourceManager {
        private:
            std::vector<std::unique_ptr<ResourceHolder>> resources_;
            
        public:
            void addResource(const std::string& name, size_t size) {
                resources_.emplace_back(std::make_unique<ResourceHolder>(name, size));
            }
            
            ResourceHolder* getResource(const std::string& name) {
                auto it = std::find_if(resources_.begin(), resources_.end(),
                    [&name](const std::unique_ptr<ResourceHolder>& holder) {
                        return holder->getName() == name;
                    });
                
                return (it != resources_.end()) ? it->get() : nullptr;
            }
            
            size_t getResourceCount() const {
                return resources_.size();
            }
            
            void removeResource(const std::string& name) {
                resources_.erase(
                    std::remove_if(resources_.begin(), resources_.end(),
                        [&name](const std::unique_ptr<ResourceHolder>& holder) {
                            return holder->getName() == name;
                        }),
                    resources_.end()
                );
            }
            
            ~ResourceManager() {
                // All resources automatically cleaned up by unique_ptr destructors
            }
        };
        
        ResourceManager manager;
        
        // Add resources
        manager.addResource("Database", 10000);
        manager.addResource("Cache", 5000);
        manager.addResource("Buffer", 2000);
        
        REQUIRE(manager.getResourceCount() == 3);
        
        // Access resources
        ResourceHolder* db = manager.getResource("Database");
        REQUIRE(db != nullptr);
        REQUIRE(db->getSize() == 10000);
        
        // Remove a resource
        manager.removeResource("Cache");
        REQUIRE(manager.getResourceCount() == 2);
        REQUIRE(manager.getResource("Cache") == nullptr);
        
        // Manager destruction will automatically clean up all resources
    }
    
    SECTION("Exception safety with smart pointers") {
        class ThrowingObject {
        private:
            static int constructionCount_;
            
        public:
            ThrowingObject() {
                constructionCount_++;
                if (constructionCount_ % 3 == 0) {
                    throw std::runtime_error("Construction failed");
                }
            }
            
            static int getConstructionCount() { return constructionCount_; }
            static void resetCount() { constructionCount_ = 0; }
        };
        
        int ThrowingObject::constructionCount_ = 0;
        
        ThrowingObject::resetCount();
        std::vector<std::unique_ptr<ThrowingObject>> objects;
        
        int successCount = 0;
        int failureCount = 0;
        
        // Try to create objects, some will throw
        for (int i = 0; i < 10; ++i) {
            try {
                objects.push_back(std::make_unique<ThrowingObject>());
                successCount++;
            } catch (const std::runtime_error&) {
                failureCount++;
                // No memory leak even when construction fails
            }
        }
        
        REQUIRE(successCount + failureCount == 10);
        REQUIRE(objects.size() == successCount);
        REQUIRE(failureCount > 0); // Some should have failed
        
        // All successfully created objects will be properly cleaned up
    }
    
    SECTION("Polymorphic deletion with smart pointers") {
        class Base {
        public:
            virtual ~Base() = default;
            virtual std::string getType() const { return "Base"; }
        };
        
        class Derived1 : public Base {
        public:
            std::string getType() const override { return "Derived1"; }
        };
        
        class Derived2 : public Base {
        public:
            std::string getType() const override { return "Derived2"; }
        };
        
        std::vector<std::unique_ptr<Base>> polymorphicObjects;
        
        // Add different derived types
        polymorphicObjects.push_back(std::make_unique<Derived1>());
        polymorphicObjects.push_back(std::make_unique<Derived2>());
        polymorphicObjects.push_back(std::make_unique<Derived1>());
        
        // Verify polymorphic behavior
        REQUIRE(polymorphicObjects[0]->getType() == "Derived1");
        REQUIRE(polymorphicObjects[1]->getType() == "Derived2");
        REQUIRE(polymorphicObjects[2]->getType() == "Derived1");
        
        // All objects will be properly destroyed with correct destructors
        polymorphicObjects.clear();
    }
}

TEST_CASE_METHOD(SmartPointerTestFixture, "Common Smart Pointer Pitfalls", "[smart-pointers][pitfalls][mistakes]") {
    
    SECTION("Double deletion prevention") {
        TrackedObject* rawPtr = new TrackedObject("DoubleDeleteTest");
        
        // This would be dangerous with raw pointers:
        // delete rawPtr; 
        // delete rawPtr; // Double deletion!
        
        // With smart pointers, this is safe:
        std::unique_ptr<TrackedObject> smartPtr(rawPtr);
        rawPtr = nullptr; // Clear raw pointer to avoid confusion
        
        // smartPtr will handle deletion automatically and safely
        REQUIRE(smartPtr != nullptr);
    }
    
    SECTION("Circular reference detection") {
        class CircularA;
        class CircularB;
        
        class CircularA {
        public:
            std::shared_ptr<CircularB> b;
            std::string name;
            
            explicit CircularA(const std::string& n) : name(n) {}
            ~CircularA() { /* Destructor for verification */ }
        };
        
        class CircularB {
        public:
            std::weak_ptr<CircularA> a; // Use weak_ptr to break cycle
            std::string name;
            
            explicit CircularB(const std::string& n) : name(n) {}
            ~CircularB() { /* Destructor for verification */ }
        };
        
        {
            auto objA = std::make_shared<CircularA>("A");
            auto objB = std::make_shared<CircularB>("B");
            
            objA->b = objB;      // shared_ptr
            objB->a = objA;      // weak_ptr - breaks circular reference
            
            REQUIRE(objA.use_count() == 1); // Only held by local variable
            REQUIRE(objB.use_count() == 2); // Local variable + objA->b
            REQUIRE(!objB->a.expired());
            
        } // Both objects should be destroyed properly
        
        // If we had used shared_ptr for both directions, we'd have a memory leak
    }
    
    SECTION("Thread safety considerations") {
        // shared_ptr control block operations are thread-safe,
        // but object access is not automatically thread-safe
        
        auto sharedObj = std::make_shared<TrackedObject>("ThreadSafetyTest");
        const int threadCount = 10;
        std::vector<std::thread> threads;
        std::atomic<int> accessCount{0};
        
        // This is safe - copying shared_ptr is thread-safe
        for (int i = 0; i < threadCount; ++i) {
            threads.emplace_back([sharedObj, &accessCount] {
                auto localCopy = sharedObj; // Thread-safe copy
                accessCount.fetch_add(1);
                
                // However, accessing the object itself requires synchronization
                // if multiple threads modify it simultaneously
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        REQUIRE(accessCount.load() == threadCount);
        REQUIRE(sharedObj.use_count() == 1); // Local copies destroyed
    }
    
    SECTION("Performance considerations") {
        const int iterations = 100000;
        
        // Demonstrate that make_unique/make_shared are preferred
        // over new with unique_ptr/shared_ptr constructor
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // Preferred approach
        std::vector<std::unique_ptr<TrackedObject>> preferredApproach;
        for (int i = 0; i < iterations; ++i) {
            preferredApproach.push_back(std::make_unique<TrackedObject>("MakeUnique"));
        }
        
        auto makeUniqueTime = std::chrono::high_resolution_clock::now() - start;
        
        start = std::chrono::high_resolution_clock::now();
        
        // Less efficient approach
        std::vector<std::unique_ptr<TrackedObject>> lessEfficientApproach;
        for (int i = 0; i < iterations; ++i) {
            lessEfficientApproach.push_back(std::unique_ptr<TrackedObject>(new TrackedObject("NewConstructor")));
        }
        
        auto newConstructorTime = std::chrono::high_resolution_clock::now() - start;
        
        // make_unique should be at least as fast, often faster
        INFO("make_unique time: " << std::chrono::duration_cast<std::chrono::milliseconds>(makeUniqueTime).count() << "ms");
        INFO("new constructor time: " << std::chrono::duration_cast<std::chrono::milliseconds>(newConstructorTime).count() << "ms");
        
        REQUIRE(preferredApproach.size() == iterations);
        REQUIRE(lessEfficientApproach.size() == iterations);
    }
}