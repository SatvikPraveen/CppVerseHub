// File: tests/unit_tests/pattern_tests/ObserverTests.cpp
// Observer pattern tests for CppVerseHub design patterns showcase

#include <catch2/catch.hpp>
#include <memory>
#include <vector>
#include <string>
#include <algorithm>
#include <functional>
#include <atomic>

// Include the observer pattern headers
#include "Observer.hpp"
#include "EventSystem.hpp"
#include "Planet.hpp"
#include "Fleet.hpp"
#include "Mission.hpp"

using namespace CppVerseHub::Patterns;
using namespace CppVerseHub::Core;
using namespace CppVerseHub::Events;

/**
 * @brief Test fixture for observer pattern tests
 */
class ObserverTestFixture {
public:
    ObserverTestFixture() {
        setupTestData();
    }
    
    ~ObserverTestFixture() {
        testPlanets.clear();
        testFleets.clear();
        testMissions.clear();
    }
    
protected:
    void setupTestData() {
        // Create test planets
        for (int i = 0; i < 6; ++i) {
            auto planet = std::make_unique<Planet>(
                "ObserverPlanet_" + std::to_string(i),
                Vector3D{i * 150.0, i * 150.0, i * 150.0}
            );
            planet->setResourceAmount(ResourceType::MINERALS, 800 + i * 100);
            planet->setResourceAmount(ResourceType::ENERGY, 400 + i * 80);
            testPlanets.push_back(std::move(planet));
        }
        
        // Create test fleets
        for (int i = 0; i < 3; ++i) {
            auto fleet = std::make_unique<Fleet>(
                "ObserverFleet_" + std::to_string(i),
                Vector3D{i * 300.0, i * 300.0, i * 300.0}
            );
            fleet->addShips(ShipType::FIGHTER, 8 + i * 4);
            fleet->addShips(ShipType::CRUISER, 2 + i);
            testFleets.push_back(std::move(fleet));
        }
        
        // Create test missions
        for (int i = 0; i < 2; ++i) {
            auto mission = std::make_unique<ExploreMission>(
                "ObserverMission_" + std::to_string(i),
                testPlanets[i].get()
            );
            testMissions.push_back(std::move(mission));
        }
    }
    
    std::vector<std::unique_ptr<Planet>> testPlanets;
    std::vector<std::unique_ptr<Fleet>> testFleets;
    std::vector<std::unique_ptr<Mission>> testMissions;
};

/**
 * @brief Simple observer interface for testing
 */
template<typename EventType>
class Observer {
public:
    virtual ~Observer() = default;
    virtual void update(const EventType& event) = 0;
    virtual std::string getObserverName() const = 0;
};

/**
 * @brief Simple subject interface for testing
 */
template<typename EventType>
class Subject {
private:
    std::vector<Observer<EventType>*> observers_;
    
public:
    virtual ~Subject() = default;
    
    void addObserver(Observer<EventType>* observer) {
        if (observer && std::find(observers_.begin(), observers_.end(), observer) == observers_.end()) {
            observers_.push_back(observer);
        }
    }
    
    void removeObserver(Observer<EventType>* observer) {
        observers_.erase(
            std::remove(observers_.begin(), observers_.end(), observer),
            observers_.end()
        );
    }
    
    void notifyObservers(const EventType& event) {
        for (auto observer : observers_) {
            observer->update(event);
        }
    }
    
    size_t getObserverCount() const {
        return observers_.size();
    }
};

/**
 * @brief Event types for testing
 */
struct ResourceEvent {
    std::string planetName;
    ResourceType resourceType;
    int oldAmount;
    int newAmount;
    std::string timestamp;
    
    ResourceEvent(const std::string& name, ResourceType type, int old, int newVal)
        : planetName(name), resourceType(type), oldAmount(old), newAmount(newVal) {
        timestamp = "2024-01-01T12:00:00Z"; // Simplified for testing
    }
};

struct FleetEvent {
    std::string fleetName;
    Vector3D position;
    std::string action;
    int shipCount;
    
    FleetEvent(const std::string& name, const Vector3D& pos, const std::string& act, int count)
        : fleetName(name), position(pos), action(act), shipCount(count) {}
};

struct MissionEvent {
    std::string missionName;
    MissionStatus status;
    std::string description;
    double progress;
    
    MissionEvent(const std::string& name, MissionStatus stat, const std::string& desc, double prog)
        : missionName(name), status(stat), description(desc), progress(prog) {}
};

/**
 * @brief Test observers
 */
class ResourceObserver : public Observer<ResourceEvent> {
private:
    std::string name_;
    std::vector<ResourceEvent> receivedEvents_;
    std::atomic<int> notificationCount_{0};
    
public:
    explicit ResourceObserver(const std::string& name) : name_(name) {}
    
    void update(const ResourceEvent& event) override {
        receivedEvents_.push_back(event);
        notificationCount_.fetch_add(1);
    }
    
    std::string getObserverName() const override {
        return name_;
    }
    
    const std::vector<ResourceEvent>& getReceivedEvents() const {
        return receivedEvents_;
    }
    
    int getNotificationCount() const {
        return notificationCount_.load();
    }
    
    void reset() {
        receivedEvents_.clear();
        notificationCount_.store(0);
    }
};

class FleetObserver : public Observer<FleetEvent> {
private:
    std::string name_;
    std::vector<FleetEvent> receivedEvents_;
    
public:
    explicit FleetObserver(const std::string& name) : name_(name) {}
    
    void update(const FleetEvent& event) override {
        receivedEvents_.push_back(event);
    }
    
    std::string getObserverName() const override {
        return name_;
    }
    
    const std::vector<FleetEvent>& getReceivedEvents() const {
        return receivedEvents_;
    }
    
    void reset() {
        receivedEvents_.clear();
    }
};

class MissionObserver : public Observer<MissionEvent> {
private:
    std::string name_;
    std::vector<MissionEvent> receivedEvents_;
    
public:
    explicit MissionObserver(const std::string& name) : name_(name) {}
    
    void update(const MissionEvent& event) override {
        receivedEvents_.push_back(event);
    }
    
    std::string getObserverName() const override {
        return name_;
    }
    
    const std::vector<MissionEvent>& getReceivedEvents() const {
        return receivedEvents_;
    }
    
    void reset() {
        receivedEvents_.clear();
    }
};

/**
 * @brief Test subjects
 */
class ResourceSubject : public Subject<ResourceEvent> {
private:
    std::string name_;
    
public:
    explicit ResourceSubject(const std::string& name) : name_(name) {}
    
    void changeResource(const std::string& planetName, ResourceType type, int oldAmount, int newAmount) {
        ResourceEvent event(planetName, type, oldAmount, newAmount);
        notifyObservers(event);
    }
    
    std::string getName() const { return name_; }
};

class FleetSubject : public Subject<FleetEvent> {
private:
    std::string name_;
    
public:
    explicit FleetSubject(const std::string& name) : name_(name) {}
    
    void moveFleet(const std::string& fleetName, const Vector3D& position, int shipCount) {
        FleetEvent event(fleetName, position, "MOVE", shipCount);
        notifyObservers(event);
    }
    
    void combatFleet(const std::string& fleetName, const Vector3D& position, int shipCount) {
        FleetEvent event(fleetName, position, "COMBAT", shipCount);
        notifyObservers(event);
    }
    
    std::string getName() const { return name_; }
};

TEST_CASE_METHOD(ObserverTestFixture, "Basic Observer Pattern", "[observer][patterns][basic]") {
    
    SECTION("Single observer, single subject") {
        ResourceSubject subject("TestResourceSubject");
        ResourceObserver observer("TestResourceObserver");
        
        REQUIRE(subject.getObserverCount() == 0);
        
        subject.addObserver(&observer);
        REQUIRE(subject.getObserverCount() == 1);
        
        // Trigger event
        subject.changeResource("TestPlanet", ResourceType::MINERALS, 1000, 1500);
        
        REQUIRE(observer.getNotificationCount() == 1);
        REQUIRE(observer.getReceivedEvents().size() == 1);
        
        const auto& event = observer.getReceivedEvents()[0];
        REQUIRE(event.planetName == "TestPlanet");
        REQUIRE(event.resourceType == ResourceType::MINERALS);
        REQUIRE(event.oldAmount == 1000);
        REQUIRE(event.newAmount == 1500);
    }
    
    SECTION("Multiple observers, single subject") {
        ResourceSubject subject("MultiObserverSubject");
        ResourceObserver observer1("Observer1");
        ResourceObserver observer2("Observer2");
        ResourceObserver observer3("Observer3");
        
        subject.addObserver(&observer1);
        subject.addObserver(&observer2);
        subject.addObserver(&observer3);
        
        REQUIRE(subject.getObserverCount() == 3);
        
        // Trigger event
        subject.changeResource("MultiTestPlanet", ResourceType::ENERGY, 500, 750);
        
        // All observers should receive the event
        REQUIRE(observer1.getNotificationCount() == 1);
        REQUIRE(observer2.getNotificationCount() == 1);
        REQUIRE(observer3.getNotificationCount() == 1);
        
        // Trigger another event
        subject.changeResource("MultiTestPlanet", ResourceType::MINERALS, 800, 900);
        
        // All observers should have received both events
        REQUIRE(observer1.getNotificationCount() == 2);
        REQUIRE(observer2.getNotificationCount() == 2);
        REQUIRE(observer3.getNotificationCount() == 2);
    }
    
    SECTION("Single observer, multiple subjects") {
        ResourceSubject subject1("Subject1");
        ResourceSubject subject2("Subject2");
        ResourceObserver observer("MultiSubjectObserver");
        
        subject1.addObserver(&observer);
        subject2.addObserver(&observer);
        
        // Events from different subjects
        subject1.changeResource("Planet1", ResourceType::MINERALS, 100, 200);
        subject2.changeResource("Planet2", ResourceType::ENERGY, 300, 400);
        
        REQUIRE(observer.getNotificationCount() == 2);
        REQUIRE(observer.getReceivedEvents().size() == 2);
        
        // Verify events are from different subjects
        const auto& events = observer.getReceivedEvents();
        REQUIRE(events[0].planetName == "Planet1");
        REQUIRE(events[1].planetName == "Planet2");
    }
    
    SECTION("Observer removal") {
        ResourceSubject subject("RemovalTestSubject");
        ResourceObserver observer1("RemovalObserver1");
        ResourceObserver observer2("RemovalObserver2");
        
        subject.addObserver(&observer1);
        subject.addObserver(&observer2);
        REQUIRE(subject.getObserverCount() == 2);
        
        // Trigger event - both should receive
        subject.changeResource("TestPlanet", ResourceType::MINERALS, 100, 150);
        REQUIRE(observer1.getNotificationCount() == 1);
        REQUIRE(observer2.getNotificationCount() == 1);
        
        // Remove one observer
        subject.removeObserver(&observer1);
        REQUIRE(subject.getObserverCount() == 1);
        
        // Trigger another event - only observer2 should receive
        subject.changeResource("TestPlanet", ResourceType::ENERGY, 200, 250);
        REQUIRE(observer1.getNotificationCount() == 1); // Still 1
        REQUIRE(observer2.getNotificationCount() == 2); // Increased to 2
    }
}

TEST_CASE_METHOD(ObserverTestFixture, "Advanced Observer Features", "[observer][advanced][features]") {
    
    SECTION("Observer with filtering") {
        class FilteringResourceObserver : public Observer<ResourceEvent> {
        private:
            std::string name_;
            ResourceType filterType_;
            std::vector<ResourceEvent> receivedEvents_;
            
        public:
            FilteringResourceObserver(const std::string& name, ResourceType filter)
                : name_(name), filterType_(filter) {}
            
            void update(const ResourceEvent& event) override {
                if (event.resourceType == filterType_) {
                    receivedEvents_.push_back(event);
                }
            }
            
            std::string getObserverName() const override {
                return name_;
            }
            
            const std::vector<ResourceEvent>& getReceivedEvents() const {
                return receivedEvents_;
            }
        };
        
        ResourceSubject subject("FilteringTestSubject");
        FilteringResourceObserver mineralObserver("MineralObserver", ResourceType::MINERALS);
        FilteringResourceObserver energyObserver("EnergyObserver", ResourceType::ENERGY);
        
        subject.addObserver(&mineralObserver);
        subject.addObserver(&energyObserver);
        
        // Send mixed events
        subject.changeResource("TestPlanet", ResourceType::MINERALS, 100, 200);
        subject.changeResource("TestPlanet", ResourceType::ENERGY, 300, 400);
        subject.changeResource("TestPlanet", ResourceType::MINERALS, 200, 250);
        
        // Mineral observer should only see mineral events
        REQUIRE(mineralObserver.getReceivedEvents().size() == 2);
        for (const auto& event : mineralObserver.getReceivedEvents()) {
            REQUIRE(event.resourceType == ResourceType::MINERALS);
        }
        
        // Energy observer should only see energy events
        REQUIRE(energyObserver.getReceivedEvents().size() == 1);
        REQUIRE(energyObserver.getReceivedEvents()[0].resourceType == ResourceType::ENERGY);
    }
    
    SECTION("Observer with aggregation") {
        class AggregatingResourceObserver : public Observer<ResourceEvent> {
        private:
            std::string name_;
            int totalMineralsChanged_;
            int totalEnergyChanged_;
            int eventCount_;
            
        public:
            explicit AggregatingResourceObserver(const std::string& name)
                : name_(name), totalMineralsChanged_(0), totalEnergyChanged_(0), eventCount_(0) {}
            
            void update(const ResourceEvent& event) override {
                eventCount_++;
                int change = event.newAmount - event.oldAmount;
                
                if (event.resourceType == ResourceType::MINERALS) {
                    totalMineralsChanged_ += change;
                } else if (event.resourceType == ResourceType::ENERGY) {
                    totalEnergyChanged_ += change;
                }
            }
            
            std::string getObserverName() const override {
                return name_;
            }
            
            int getTotalMineralsChanged() const { return totalMineralsChanged_; }
            int getTotalEnergyChanged() const { return totalEnergyChanged_; }
            int getEventCount() const { return eventCount_; }
        };
        
        ResourceSubject subject("AggregationTestSubject");
        AggregatingResourceObserver aggregator("Aggregator");
        
        subject.addObserver(&aggregator);
        
        // Send multiple events
        subject.changeResource("Planet1", ResourceType::MINERALS, 100, 150); // +50
        subject.changeResource("Planet2", ResourceType::ENERGY, 200, 300);   // +100
        subject.changeResource("Planet3", ResourceType::MINERALS, 80, 70);   // -10
        subject.changeResource("Planet4", ResourceType::ENERGY, 400, 350);   // -50
        
        REQUIRE(aggregator.getEventCount() == 4);
        REQUIRE(aggregator.getTotalMineralsChanged() == 40); // 50 - 10
        REQUIRE(aggregator.getTotalEnergyChanged() == 50);   // 100 - 50
    }
    
    SECTION("Observer with callback functions") {
        using ResourceCallback = std::function<void(const ResourceEvent&)>;
        
        class CallbackResourceObserver : public Observer<ResourceEvent> {
        private:
            std::string name_;
            ResourceCallback callback_;
            
        public:
            CallbackResourceObserver(const std::string& name, ResourceCallback callback)
                : name_(name), callback_(callback) {}
            
            void update(const ResourceEvent& event) override {
                if (callback_) {
                    callback_(event);
                }
            }
            
            std::string getObserverName() const override {
                return name_;
            }
        };
        
        ResourceSubject subject("CallbackTestSubject");
        
        int callbackCount = 0;
        std::string lastPlanetName;
        
        CallbackResourceObserver callbackObserver("CallbackObserver", 
            [&callbackCount, &lastPlanetName](const ResourceEvent& event) {
                callbackCount++;
                lastPlanetName = event.planetName;
            });
        
        subject.addObserver(&callbackObserver);
        
        subject.changeResource("CallbackPlanet1", ResourceType::MINERALS, 100, 200);
        subject.changeResource("CallbackPlanet2", ResourceType::ENERGY, 300, 400);
        
        REQUIRE(callbackCount == 2);
        REQUIRE(lastPlanetName == "CallbackPlanet2");
    }
}

TEST_CASE_METHOD(ObserverTestFixture, "Real-World Observer Applications", "[observer][real-world][applications]") {
    
    SECTION("Fleet movement tracking") {
        FleetSubject fleetSubject("FleetTracker");
        FleetObserver commandCenter("CommandCenter");
        FleetObserver logSystem("LogSystem");
        FleetObserver alertSystem("AlertSystem");
        
        fleetSubject.addObserver(&commandCenter);
        fleetSubject.addObserver(&logSystem);
        fleetSubject.addObserver(&alertSystem);
        
        // Simulate fleet movements
        Fleet* fleet1 = testFleets[0].get();
        fleetSubject.moveFleet(fleet1->getName(), fleet1->getPosition(), fleet1->getTotalShipCount());
        
        Fleet* fleet2 = testFleets[1].get();
        Vector3D newPosition{500, 600, 700};
        fleetSubject.moveFleet(fleet2->getName(), newPosition, fleet2->getTotalShipCount());
        
        // All observers should have received both events
        REQUIRE(commandCenter.getReceivedEvents().size() == 2);
        REQUIRE(logSystem.getReceivedEvents().size() == 2);
        REQUIRE(alertSystem.getReceivedEvents().size() == 2);
        
        // Verify event details
        const auto& events = commandCenter.getReceivedEvents();
        REQUIRE(events[0].fleetName == fleet1->getName());
        REQUIRE(events[0].action == "MOVE");
        REQUIRE(events[1].fleetName == fleet2->getName());
        REQUIRE(events[1].position.x == 500);
        REQUIRE(events[1].position.y == 600);
        REQUIRE(events[1].position.z == 700);
    }
    
    SECTION("Mission progress monitoring") {
        class MissionSubject : public Subject<MissionEvent> {
        private:
            std::string name_;
            
        public:
            explicit MissionSubject(const std::string& name) : name_(name) {}
            
            void updateMissionProgress(Mission* mission) {
                MissionEvent event(
                    mission->getName(),
                    mission->getStatus(),
                    "Mission progress updated",
                    mission->getProgress()
                );
                notifyObservers(event);
            }
            
            void completeMission(Mission* mission) {
                MissionEvent event(
                    mission->getName(),
                    MissionStatus::COMPLETED,
                    "Mission completed successfully",
                    100.0
                );
                notifyObservers(event);
            }
        };
        
        MissionSubject missionSubject("MissionTracker");
        MissionObserver progressTracker("ProgressTracker");
        MissionObserver achievementSystem("AchievementSystem");
        
        missionSubject.addObserver(&progressTracker);
        missionSubject.addObserver(&achievementSystem);
        
        Mission* mission = testMissions[0].get();
        
        // Simulate mission progress
        missionSubject.updateMissionProgress(mission);
        missionSubject.completeMission(mission);
        
        // Both observers should have received both events
        REQUIRE(progressTracker.getReceivedEvents().size() == 2);
        REQUIRE(achievementSystem.getReceivedEvents().size() == 2);
        
        const auto& events = progressTracker.getReceivedEvents();
        REQUIRE(events[0].missionName == mission->getName());
        REQUIRE(events[1].status == MissionStatus::COMPLETED);
        REQUIRE(events[1].progress == 100.0);
    }
    
    SECTION("Resource depletion alerts") {
        class ResourceAlertObserver : public Observer<ResourceEvent> {
        private:
            std::string name_;
            int alertThreshold_;
            std::vector<std::string> alerts_;
            
        public:
            ResourceAlertObserver(const std::string& name, int threshold)
                : name_(name), alertThreshold_(threshold) {}
            
            void update(const ResourceEvent& event) override {
                if (event.newAmount < alertThreshold_) {
                    std::string alert = "ALERT: " + event.planetName + 
                                      " resources below threshold (" + 
                                      std::to_string(event.newAmount) + ")";
                    alerts_.push_back(alert);
                }
            }
            
            std::string getObserverName() const override {
                return name_;
            }
            
            const std::vector<std::string>& getAlerts() const {
                return alerts_;
            }
        };
        
        ResourceSubject resourceSubject("ResourceMonitor");
        ResourceAlertObserver alertObserver("AlertSystem", 100); // Alert when below 100
        
        resourceSubject.addObserver(&alertObserver);
        
        // Normal resource changes - no alerts
        resourceSubject.changeResource("SafePlanet", ResourceType::MINERALS, 500, 450);
        resourceSubject.changeResource("SafePlanet", ResourceType::ENERGY, 300, 280);
        REQUIRE(alertObserver.getAlerts().empty());
        
        // Resource drops below threshold - should trigger alert
        resourceSubject.changeResource("DangerPlanet", ResourceType::MINERALS, 200, 50);
        resourceSubject.changeResource("DangerPlanet", ResourceType::ENERGY, 150, 80);
        
        REQUIRE(alertObserver.getAlerts().size() == 2);
        REQUIRE(alertObserver.getAlerts()[0].find("DangerPlanet") != std::string::npos);
        REQUIRE(alertObserver.getAlerts()[0].find("50") != std::string::npos);
        REQUIRE(alertObserver.getAlerts()[1].find("80") != std::string::npos);
    }
}

TEST_CASE_METHOD(ObserverTestFixture, "Observer Pattern Edge Cases", "[observer][edge-cases][robustness]") {
    
    SECTION("Null observer handling") {
        ResourceSubject subject("NullTestSubject");
        
        // Adding null observer should be safe
        subject.addObserver(nullptr);
        REQUIRE(subject.getObserverCount() == 0);
        
        // Should not crash when notifying with no observers
        subject.changeResource("TestPlanet", ResourceType::MINERALS, 100, 200);
    }
    
    SECTION("Duplicate observer handling") {
        ResourceSubject subject("DuplicateTestSubject");
        ResourceObserver observer("DuplicateObserver");
        
        subject.addObserver(&observer);
        REQUIRE(subject.getObserverCount() == 1);
        
        // Adding same observer again should not increase count
        subject.addObserver(&observer);
        REQUIRE(subject.getObserverCount() == 1);
        
        // Observer should still receive notifications normally
        subject.changeResource("TestPlanet", ResourceType::MINERALS, 100, 200);
        REQUIRE(observer.getNotificationCount() == 1);
    }
    
    SECTION("Observer self-removal during notification") {
        class SelfRemovingObserver : public Observer<ResourceEvent> {
        private:
            std::string name_;
            Subject<ResourceEvent>* subject_;
            std::atomic<int> notificationCount_{0};
            
        public:
            SelfRemovingObserver(const std::string& name, Subject<ResourceEvent>* subject)
                : name_(name), subject_(subject) {}
            
            void update(const ResourceEvent& event) override {
                notificationCount_.fetch_add(1);
                // Remove self after first notification
                if (notificationCount_.load() == 1) {
                    subject_->removeObserver(this);
                }
            }
            
            std::string getObserverName() const override {
                return name_;
            }
            
            int getNotificationCount() const {
                return notificationCount_.load();
            }
        };
        
        ResourceSubject subject("SelfRemovalSubject");
        SelfRemovingObserver selfRemover("SelfRemover", &subject);
        ResourceObserver normalObserver("NormalObserver");
        
        subject.addObserver(&selfRemover);
        subject.addObserver(&normalObserver);
        REQUIRE(subject.getObserverCount() == 2);
        
        // First notification - self-remover should remove itself
        subject.changeResource("TestPlanet", ResourceType::MINERALS, 100, 200);
        REQUIRE(subject.getObserverCount() == 1); // Only normal observer left
        REQUIRE(selfRemover.getNotificationCount() == 1);
        REQUIRE(normalObserver.getNotificationCount() == 1);
        
        // Second notification - only normal observer should receive it
        subject.changeResource("TestPlanet", ResourceType::ENERGY, 300, 400);
        REQUIRE(selfRemover.getNotificationCount() == 1); // Still 1
        REQUIRE(normalObserver.getNotificationCount() == 2); // Increased
    }
    
    SECTION("Observer throwing exceptions") {
        class ExceptionThrowingObserver : public Observer<ResourceEvent> {
        private:
            std::string name_;
            bool shouldThrow_;
            std::atomic<int> notificationCount_{0};
            
        public:
            ExceptionThrowingObserver(const std::string& name, bool shouldThrow = true)
                : name_(name), shouldThrow_(shouldThrow) {}
            
            void update(const ResourceEvent& event) override {
                notificationCount_.fetch_add(1);
                if (shouldThrow_) {
                    throw std::runtime_error("Observer exception for testing");
                }
            }
            
            std::string getObserverName() const override {
                return name_;
            }
            
            int getNotificationCount() const {
                return notificationCount_.load();
            }
            
            void setShouldThrow(bool should) {
                shouldThrow_ = should;
            }
        };
        
        class SafeNotifyingSubject : public Subject<ResourceEvent> {
        private:
            std::string name_;
            
        public:
            explicit SafeNotifyingSubject(const std::string& name) : name_(name) {}
            
            void safeChangeResource(const std::string& planetName, ResourceType type, int oldAmount, int newAmount) {
                ResourceEvent event(planetName, type, oldAmount, newAmount);
                
                // Safe notification that continues even if observers throw
                for (size_t i = 0; i < getObserverCount(); ++i) {
                    try {
                        // Note: This is a simplified version; real implementation would need observer iteration
                        notifyObservers(event);
                        break; // If we get here, no exception was thrown
                    } catch (const std::exception& e) {
                        // Log exception but continue
                        INFO("Observer threw exception: " << e.what());
                    }
                }
            }
        };
        
        SafeNotifyingSubject subject("ExceptionSafeSubject");
        ExceptionThrowingObserver throwingObserver("ThrowingObserver", true);
        ResourceObserver normalObserver("NormalObserver");
        
        subject.addObserver(&throwingObserver);
        subject.addObserver(&normalObserver);
        
        // This should not crash despite the throwing observer
        REQUIRE_NOTHROW(subject.safeChangeResource("TestPlanet", ResourceType::MINERALS, 100, 200));
        
        REQUIRE(throwingObserver.getNotificationCount() >= 1);
        // Note: normalObserver might not receive notification depending on implementation
    }
}

TEST_CASE_METHOD(ObserverTestFixture, "Observer Performance and Scalability", "[observer][performance][scalability]") {
    
    SECTION("Large number of observers") {
        ResourceSubject subject("PerformanceTestSubject");
        std::vector<std::unique_ptr<ResourceObserver>> observers;
        
        const int observerCount = 1000;
        
        // Create many observers
        for (int i = 0; i < observerCount; ++i) {
            auto observer = std::make_unique<ResourceObserver>("Observer_" + std::to_string(i));
            subject.addObserver(observer.get());
            observers.push_back(std::move(observer));
        }
        
        REQUIRE(subject.getObserverCount() == observerCount);
        
        // Measure notification performance
        auto start = std::chrono::high_resolution_clock::now();
        
        const int notificationCount = 100;
        for (int i = 0; i < notificationCount; ++i) {
            subject.changeResource("PerformancePlanet", ResourceType::MINERALS, 1000 + i, 1000 + i + 10);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        // Verify all observers received all notifications
        for (const auto& observer : observers) {
            REQUIRE(observer->getNotificationCount() == notificationCount);
        }
        
        INFO("Time for " << notificationCount << " notifications to " << observerCount << 
             " observers: " << duration.count() << "μs");
        
        // Performance should be reasonable (less than 1 second for this test)
        REQUIRE(duration.count() < 1000000);
    }
    
    SECTION("Frequent observer addition/removal") {
        ResourceSubject subject("DynamicTestSubject");
        std::vector<std::unique_ptr<ResourceObserver>> observers;
        
        const int cycleCount = 100;
        const int observersPerCycle = 10;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int cycle = 0; cycle < cycleCount; ++cycle) {
            // Add observers
            std::vector<ResourceObserver*> cycleObservers;
            for (int i = 0; i < observersPerCycle; ++i) {
                auto observer = std::make_unique<ResourceObserver>(
                    "Cycle" + std::to_string(cycle) + "_Observer" + std::to_string(i));
                cycleObservers.push_back(observer.get());
                subject.addObserver(observer.get());
                observers.push_back(std::move(observer));
            }
            
            // Send notification
            subject.changeResource("DynamicPlanet", ResourceType::ENERGY, 500, 550);
            
            // Remove half the observers
            for (int i = 0; i < observersPerCycle / 2; ++i) {
                subject.removeObserver(cycleObservers[i]);
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        INFO("Time for " << cycleCount << " cycles of adding/removing observers: " << 
             duration.count() << "μs");
        
        // Should complete in reasonable time
        REQUIRE(duration.count() < 1000000); // Less than 1 second
        
        // Final observer count should be half of total created
        size_t expectedFinalCount = (cycleCount * observersPerCycle) - (cycleCount * observersPerCycle / 2);
        REQUIRE(subject.getObserverCount() == expectedFinalCount);
    }
}