// File: tests/unit_tests/core_tests/MissionTests.cpp
// Mission polymorphism tests for CppVerseHub core functionality

#include <catch2/catch.hpp>
#include <memory>
#include <vector>
#include <string>
#include <chrono>
#include <algorithm>

// Include the mission system headers
#include "Mission.hpp"
#include "ExplorationMission.hpp"
#include "ColonizationMission.hpp"
#include "CombatMission.hpp"
#include "Fleet.hpp"
#include "Planet.hpp"
#include "Exceptions.hpp"

using namespace CppVerseHub::Core;

/**
 * @brief Test fixture for Mission system tests
 */
class MissionTestFixture {
public:
    MissionTestFixture() {
        setupTestEnvironment();
    }
    
    ~MissionTestFixture() {
        missions.clear();
        fleets.clear();
        planets.clear();
    }
    
protected:
    void setupTestEnvironment() {
        // Create test fleets
        auto fleet1 = std::make_unique<Fleet>("ExplorerFleet", Vector3D{100, 100, 100});
        fleet1->addShips(ShipType::FIGHTER, 5);
        fleet1->addShips(ShipType::CRUISER, 2);
        fleets.push_back(std::move(fleet1));
        
        auto fleet2 = std::make_unique<Fleet>("ColonyFleet", Vector3D{200, 200, 200});
        fleet2->addShips(ShipType::COLONIZER, 3);
        fleet2->addShips(ShipType::TRANSPORT, 4);
        fleets.push_back(std::move(fleet2));
        
        auto fleet3 = std::make_unique<Fleet>("WarFleet", Vector3D{300, 300, 300});
        fleet3->addShips(ShipType::BATTLESHIP, 2);
        fleet3->addShips(ShipType::CRUISER, 5);
        fleet3->addShips(ShipType::FIGHTER, 10);
        fleets.push_back(std::move(fleet3));
        
        // Create test planets
        auto planet1 = std::make_unique<Planet>("UnexploredWorld", Vector3D{1000, 1000, 1000});
        planet1->setHabitabilityRating(0.8);
        planets.push_back(std::move(planet1));
        
        auto planet2 = std::make_unique<Planet>("HostileWorld", Vector3D{2000, 2000, 2000});
        planet2->setHabitabilityRating(0.3);
        planet2->setPopulation(1000); // Enemy population
        planets.push_back(std::move(planet2));
        
        auto planet3 = std::make_unique<Planet>("ResourceRich", Vector3D{1500, 1500, 1500});
        planet3->setResourceAmount(ResourceType::MINERALS, 5000);
        planet3->setResourceAmount(ResourceType::ENERGY, 3000);
        planet3->setHabitabilityRating(0.9);
        planets.push_back(std::move(planet3));
    }
    
    std::vector<std::unique_ptr<Mission>> missions;
    std::vector<std::unique_ptr<Fleet>> fleets;
    std::vector<std::unique_ptr<Planet>> planets;
};

TEST_CASE_METHOD(MissionTestFixture, "Mission Base Class Functionality", "[mission][core]") {
    
    SECTION("Mission Creation and Basic Properties") {
        SECTION("Exploration mission creation") {
            auto mission = std::make_unique<ExplorationMission>(
                fleets[0].get(), 
                planets[0].get(),
                30.0 // duration
            );
            
            REQUIRE(mission != nullptr);
            REQUIRE(mission->getType() == MissionType::EXPLORATION);
            REQUIRE(mission->getStatus() == MissionStatus::PENDING);
            REQUIRE(mission->getDuration() == Approx(30.0));
            REQUIRE(mission->getProgress() == Approx(0.0));
            REQUIRE(mission->getId() > 0);
        }
        
        SECTION("Mission with invalid parameters") {
            REQUIRE_THROWS_AS(
                std::make_unique<ExplorationMission>(nullptr, planets[0].get(), 10.0),
                InvalidArgumentException
            );
            
            REQUIRE_THROWS_AS(
                std::make_unique<ExplorationMission>(fleets[0].get(), nullptr, 10.0),
                InvalidArgumentException
            );
            
            REQUIRE_THROWS_AS(
                std::make_unique<ExplorationMission>(fleets[0].get(), planets[0].get(), -5.0),
                InvalidArgumentException
            );
        }
    }
    
    SECTION("Mission Status Management") {
        auto mission = std::make_unique<ColonizationMission>(
            fleets[1].get(),
            planets[2].get(),
            45.0
        );
        
        SECTION("Initial status") {
            REQUIRE(mission->getStatus() == MissionStatus::PENDING);
            REQUIRE(!mission->isActive());
            REQUIRE(!mission->isCompleted());
            REQUIRE(!mission->isFailed());
        }
        
        SECTION("Mission lifecycle") {
            mission->start();
            REQUIRE(mission->getStatus() == MissionStatus::IN_PROGRESS);
            REQUIRE(mission->isActive());
            
            // Simulate partial completion
            mission->update(20.0);
            REQUIRE(mission->getProgress() > 0.0);
            REQUIRE(mission->getProgress() < 100.0);
            REQUIRE(mission->getStatus() == MissionStatus::IN_PROGRESS);
            
            // Complete the mission
            mission->update(25.0);
            REQUIRE(mission->getProgress() >= 100.0);
            REQUIRE(mission->getStatus() == MissionStatus::COMPLETED);
            REQUIRE(mission->isCompleted());
        }
        
        SECTION("Mission failure scenarios") {
            mission->start();
            
            // Simulate mission failure
            mission->fail("Fleet destroyed");
            REQUIRE(mission->getStatus() == MissionStatus::FAILED);
            REQUIRE(mission->isFailed());
            REQUIRE(!mission->getFailureReason().empty());
        }
        
        SECTION("Mission cancellation") {
            mission->start();
            mission->cancel();
            
            REQUIRE(mission->getStatus() == MissionStatus::CANCELLED);
            REQUIRE(!mission->isActive());
        }
    }
    
    SECTION("Mission Progress Tracking") {
        auto mission = std::make_unique<ExplorationMission>(
            fleets[0].get(),
            planets[0].get(),
            60.0
        );
        
        mission->start();
        
        SECTION("Linear progress") {
            mission->update(15.0); // 25% progress
            REQUIRE(mission->getProgress() == Approx(25.0).epsilon(0.01));
            
            mission->update(15.0); // 50% progress
            REQUIRE(mission->getProgress() == Approx(50.0).epsilon(0.01));
            
            mission->update(30.0); // 100% progress
            REQUIRE(mission->getProgress() >= 100.0);
            REQUIRE(mission->isCompleted());
        }
        
        SECTION("Progress cannot exceed 100%") {
            mission->update(120.0); // Excessive time
            REQUIRE(mission->getProgress() == Approx(100.0));
        }
    }
}

TEST_CASE_METHOD(MissionTestFixture, "Exploration Mission Specialization", "[exploration][mission][core]") {
    
    SECTION("Exploration Mission Creation") {
        auto mission = std::make_unique<ExplorationMission>(
            fleets[0].get(),
            planets[0].get(),
            40.0
        );
        
        REQUIRE(mission->getType() == MissionType::EXPLORATION);
        REQUIRE(mission->getTargetPlanet() == planets[0].get());
        REQUIRE(mission->getAssignedFleet() == fleets[0].get());
    }
    
    SECTION("Exploration Requirements") {
        auto mission = std::make_unique<ExplorationMission>(
            fleets[0].get(),
            planets[0].get(),
            30.0
        );
        
        SECTION("Fleet capability check") {
            // Fleet should have exploration capabilities
            bool hasScouts = mission->hasRequiredCapabilities();
            REQUIRE(hasScouts); // Assuming our test fleet has scouts/fighters
        }
        
        SECTION("Distance considerations") {
            double distance = mission->calculateTravelTime();
            REQUIRE(distance > 0.0);
            
            // Longer distances should take more time
            auto distantPlanet = std::make_unique<Planet>("DistantWorld", Vector3D{5000, 5000, 5000});
            auto distantMission = std::make_unique<ExplorationMission>(
                fleets[0].get(),
                distantPlanet.get(),
                30.0
            );
            
            double distantTravelTime = distantMission->calculateTravelTime();
            REQUIRE(distantTravelTime > distance);
        }
    }
    
    SECTION("Exploration Results") {
        auto mission = std::make_unique<ExplorationMission>(
            fleets[0].get(),
            planets[0].get(),
            20.0
        );
        
        mission->start();
        
        SECTION("Successful exploration") {
            mission->update(20.0); // Complete mission
            
            REQUIRE(mission->isCompleted());
            
            // Check exploration results
            auto results = mission->getExplorationResults();
            REQUIRE(results.planetExplored == true);
            REQUIRE(results.habitabilityRating >= 0.0);
            REQUIRE(results.habitabilityRating <= 1.0);
            REQUIRE(results.resourcesDiscovered.size() > 0);
        }
        
        SECTION("Exploration hazards") {
            // Simulate dangerous exploration
            mission->encounterHazard("Hostile wildlife", 0.3);
            
            // Mission might take longer or fail
            REQUIRE(mission->getHazardLevel() > 0.0);
            
            // Fleet might take damage
            double initialHealth = fleets[0]->getHealth();
            mission->update(10.0);
            REQUIRE(fleets[0]->getHealth() <= initialHealth);
        }
    }
    
    SECTION("Advanced Exploration Features") {
        auto mission = std::make_unique<ExplorationMission>(
            fleets[0].get(),
            planets[0].get(),
            35.0
        );
        
        SECTION("Scientific data collection") {
            mission->start();
            mission->update(35.0);
            
            auto scientificData = mission->getScientificData();
            REQUIRE(scientificData.geologicalSamples > 0);
            REQUIRE(scientificData.atmosphericData.temperature > 0);
            REQUIRE(!scientificData.biologicalSigns.empty());
        }
        
        SECTION("Resource survey") {
            mission->start();
            mission->enableResourceSurvey(true);
            mission->update(35.0);
            
            auto resourceSurvey = mission->getResourceSurvey();
            REQUIRE(resourceSurvey.mineralDeposits.size() >= 0);
            REQUIRE(resourceSurvey.energySources.size() >= 0);
            REQUIRE(resourceSurvey.surveyAccuracy > 0.0);
        }
    }
}

TEST_CASE_METHOD(MissionTestFixture, "Colonization Mission Specialization", "[colonization][mission][core]") {
    
    SECTION("Colonization Mission Creation") {
        auto mission = std::make_unique<ColonizationMission>(
            fleets[1].get(),
            planets[2].get(),
            60.0
        );
        
        REQUIRE(mission->getType() == MissionType::COLONIZATION);
        REQUIRE(mission->getTargetPlanet() == planets[2].get());
        REQUIRE(mission->getColonistCount() > 0);
    }
    
    SECTION("Colonization Requirements") {
        auto mission = std::make_unique<ColonizationMission>(
            fleets[1].get(),
            planets[2].get(),
            50.0
        );
        
        SECTION("Fleet composition check") {
            bool hasColonizers = mission->hasColonizerShips();
            REQUIRE(hasColonizers); // Our test fleet has colonizer ships
            
            bool hasTransports = mission->hasTransportShips();
            REQUIRE(hasTransports); // Our test fleet has transport ships
        }
        
        SECTION("Planet habitability check") {
            bool suitable = mission->isPlanetSuitable();
            REQUIRE(suitable); // Our test planet has good habitability
            
            // Test with unsuitable planet
            auto hostilePlanet = std::make_unique<Planet>("Hostile", Vector3D{0, 0, 0});
            hostilePlanet->setHabitabilityRating(0.1);
            
            auto unsuitableMission = std::make_unique<ColonizationMission>(
                fleets[1].get(),
                hostilePlanet.get(),
                50.0
            );
            
            REQUIRE(!unsuitableMission->isPlanetSuitable());
        }
        
        SECTION("Resource requirements") {
            auto requirements = mission->getResourceRequirements();
            REQUIRE(requirements.food > 0);
            REQUIRE(requirements.materials > 0);
            REQUIRE(requirements.energy > 0);
        }
    }
    
    SECTION("Colonization Process") {
        auto mission = std::make_unique<ColonizationMission>(
            fleets[1].get(),
            planets[2].get(),
            80.0
        );
        
        mission->start();
        
        SECTION("Progressive colonization") {
            // Phase 1: Landing
            mission->update(20.0);
            REQUIRE(mission->getCurrentPhase() == ColonizationPhase::LANDING);
            REQUIRE(mission->getProgress() > 0);
            
            // Phase 2: Infrastructure
            mission->update(20.0);
            REQUIRE(mission->getCurrentPhase() == ColonizationPhase::INFRASTRUCTURE);
            
            // Phase 3: Population growth
            mission->update(40.0);
            REQUIRE(mission->getCurrentPhase() == ColonizationPhase::ESTABLISHMENT);
            REQUIRE(mission->isCompleted());
        }
        
        SECTION("Colony establishment success") {
            mission->update(80.0);
            
            REQUIRE(mission->isCompleted());
            REQUIRE(planets[2]->getPopulation() > 0);
            REQUIRE(planets[2]->hasColony());
            
            auto colonyInfo = mission->getColonyInfo();
            REQUIRE(colonyInfo.establishedPopulation > 0);
            REQUIRE(colonyInfo.infrastructureLevel > 0);
            REQUIRE(!colonyInfo.colonyName.empty());
        }
        
        SECTION("Colonization challenges") {
            mission->addChallenge("Hostile native life", 0.4);
            mission->addChallenge("Severe weather", 0.2);
            
            // Challenges should affect progress
            mission->update(40.0);
            REQUIRE(mission->getProgress() < 50.0); // Slower due to challenges
            
            // But mission can still succeed
            mission->update(60.0);
            REQUIRE(mission->isCompleted());
        }
    }
    
    SECTION("Post-Colonization Effects") {
        auto mission = std::make_unique<ColonizationMission>(
            fleets[1].get(),
            planets[2].get(),
            40.0
        );
        
        mission->start();
        mission->update(40.0);
        
        SECTION("Planet transformation") {
            // Planet should have increased population
            REQUIRE(planets[2]->getPopulation() > 0);
            
            // Planet should have colony infrastructure
            REQUIRE(planets[2]->hasColony());
            
            // Planet should start producing resources
            REQUIRE(planets[2]->getResourceProduction(ResourceType::FOOD) > 0);
        }
        
        SECTION("Fleet status after colonization") {
            // Some ships might be left as colony defenders
            int remainingShips = fleets[1]->getShipCount();
            REQUIRE(remainingShips >= 0);
            
            // Fleet should be available for new missions
            REQUIRE(fleets[1]->getStatus() == EntityStatus::ACTIVE);
        }
    }
}

TEST_CASE_METHOD(MissionTestFixture, "Combat Mission Specialization", "[combat][mission][core]") {
    
    SECTION("Combat Mission Creation") {
        auto mission = std::make_unique<CombatMission>(
            fleets[2].get(),
            planets[1].get(),
            25.0
        );
        
        REQUIRE(mission->getType() == MissionType::COMBAT);
        REQUIRE(mission->getTargetPlanet() == planets[1].get());
        REQUIRE(mission->getAttackingFleet() == fleets[2].get());
    }
    
    SECTION("Combat Preparation") {
        auto mission = std::make_unique<CombatMission>(
            fleets[2].get(),
            planets[1].get(),
            30.0
        );
        
        SECTION("Fleet combat readiness") {
            double combatPower = mission->getFleetCombatPower();
            REQUIRE(combatPower > 0.0);
            
            bool isReady = mission->isFleetCombatReady();
            REQUIRE(isReady); // Our test fleet has combat ships
        }
        
        SECTION("Target assessment") {
            double defenseStrength = mission->assessTargetDefenses();
            REQUIRE(defenseStrength >= 0.0);
            
            auto combatOdds = mission->calculateCombatOdds();
            REQUIRE(combatOdds.attackerAdvantage >= 0.0);
            REQUIRE(combatOdds.defenderAdvantage >= 0.0);
        }
        
        SECTION("Tactical planning") {
            mission->setTacticalStrategy(CombatStrategy::AGGRESSIVE);
            REQUIRE(mission->getTacticalStrategy() == CombatStrategy::AGGRESSIVE);
            
            mission->setPrimaryObjective(CombatObjective::ORBITAL_BOMBARDMENT);
            REQUIRE(mission->getPrimaryObjective() == CombatObjective::ORBITAL_BOMBARDMENT);
        }
    }
    
    SECTION("Combat Execution") {
        auto mission = std::make_unique<CombatMission>(
            fleets[2].get(),
            planets[1].get(),
            20.0
        );
        
        mission->start();
        
        SECTION("Combat phases") {
            // Phase 1: Approach
            mission->update(5.0);
            REQUIRE(mission->getCurrentCombatPhase() == CombatPhase::APPROACH);
            
            // Phase 2: Engagement
            mission->update(10.0);
            REQUIRE(mission->getCurrentCombatPhase() == CombatPhase::ENGAGEMENT);
            
            // Phase 3: Resolution
            mission->update(5.0);
            REQUIRE(mission->getCurrentCombatPhase() == CombatPhase::RESOLUTION);
            REQUIRE(mission->isCompleted());
        }
        
        SECTION("Combat results") {
            mission->update(20.0);
            
            REQUIRE(mission->isCompleted());
            
            auto combatResults = mission->getCombatResults();
            REQUIRE(combatResults.outcome != CombatOutcome::UNKNOWN);
            REQUIRE(combatResults.attackerLosses >= 0);
            REQUIRE(combatResults.defenderLosses >= 0);
            REQUIRE(combatResults.combatDuration > 0);
        }
        
        SECTION("Fleet damage and losses") {
            double initialHealth = fleets[2]->getHealth();
            int initialShipCount = fleets[2]->getShipCount();
            
            mission->update(20.0);
            
            // Fleet should have taken some damage/losses
            REQUIRE(fleets[2]->getHealth() <= initialHealth);
            REQUIRE(fleets[2]->getShipCount() <= initialShipCount);
        }
        
        SECTION("Target planet effects") {
            double initialPlanetHealth = planets[1]->getHealth();
            int initialPopulation = planets[1]->getPopulation();
            
            mission->update(20.0);
            
            // Planet should show effects of combat
            if (mission->getCombatResults().outcome == CombatOutcome::ATTACKER_VICTORY) {
                REQUIRE(planets[1]->getHealth() <= initialPlanetHealth);
                REQUIRE(planets[1]->getPopulation() <= initialPopulation);
            }
        }
    }
    
    SECTION("Advanced Combat Features") {
        auto mission = std::make_unique<CombatMission>(
            fleets[2].get(),
            planets[1].get(),
            35.0
        );
        
        SECTION("Multi-objective combat") {
            mission->addSecondaryObjective(CombatObjective::RESOURCE_CAPTURE);
            mission->addSecondaryObjective(CombatObjective::INFRASTRUCTURE_DESTRUCTION);
            
            auto objectives = mission->getObjectives();
            REQUIRE(objectives.size() >= 2);
        }
        
        SECTION("Reinforcements") {
            mission->start();
            mission->update(10.0);
            
            // Add reinforcements during combat
            auto reinforcementFleet = std::make_unique<Fleet>("Reinforcements", Vector3D{350, 350, 350});
            reinforcementFleet->addShips(ShipType::CRUISER, 3);
            
            mission->addReinforcements(reinforcementFleet.get());
            
            // Combat should continue with additional forces
            mission->update(25.0);
            
            auto results = mission->getCombatResults();
            REQUIRE(results.reinforcementsUsed == true);
        }
    }
}

TEST_CASE_METHOD(MissionTestFixture, "Mission Polymorphism and Virtual Dispatch", "[mission][polymorphism][core]") {
    
    SECTION("Polymorphic Mission Collection") {
        // Create collection of different mission types
        std::vector<std::unique_ptr<Mission>> missionList;
        
        missionList.push_back(std::make_unique<ExplorationMission>(
            fleets[0].get(), planets[0].get(), 30.0));
        missionList.push_back(std::make_unique<ColonizationMission>(
            fleets[1].get(), planets[2].get(), 60.0));
        missionList.push_back(std::make_unique<CombatMission>(
            fleets[2].get(), planets[1].get(), 25.0));
        
        SECTION("Virtual function dispatch") {
            for (auto& mission : missionList) {
                // Test polymorphic behavior
                REQUIRE(mission->getId() > 0);
                REQUIRE(mission->getDuration() > 0.0);
                REQUIRE(mission->getStatus() == MissionStatus::PENDING);
                
                // Start all missions
                mission->start();
                REQUIRE(mission->getStatus() == MissionStatus::IN_PROGRESS);
                REQUIRE(mission->isActive());
                
                // Update missions
                mission->update(5.0);
                REQUIRE(mission->getProgress() > 0.0);
                
                // Each mission type should behave differently
                MissionType type = mission->getType();
                REQUIRE((type == MissionType::EXPLORATION || 
                        type == MissionType::COLONIZATION || 
                        type == MissionType::COMBAT));
            }
        }
        
        SECTION("Runtime type identification") {
            for (auto& mission : missionList) {
                if (mission->getType() == MissionType::EXPLORATION) {
                    ExplorationMission* exploration = dynamic_cast<ExplorationMission*>(mission.get());
                    REQUIRE(exploration != nullptr);
                    
                    // Test exploration-specific functionality
                    exploration->enableResourceSurvey(true);
                    REQUIRE(exploration->isResourceSurveyEnabled());
                }
                
                if (mission->getType() == MissionType::COLONIZATION) {
                    ColonizationMission* colonization = dynamic_cast<ColonizationMission*>(mission.get());
                    REQUIRE(colonization != nullptr);
                    
                    // Test colonization-specific functionality
                    int colonists = colonization->getColonistCount();
                    REQUIRE(colonists > 0);
                }
                
                if (mission->getType() == MissionType::COMBAT) {
                    CombatMission* combat = dynamic_cast<CombatMission*>(mission.get());
                    REQUIRE(combat != nullptr);
                    
                    // Test combat-specific functionality
                    combat->setTacticalStrategy(CombatStrategy::DEFENSIVE);
                    REQUIRE(combat->getTacticalStrategy() == CombatStrategy::DEFENSIVE);
                }
            }
        }
    }
    
    SECTION("Mission Factory Pattern Integration") {
        // Test integration with factory pattern if available
        SECTION("Mission creation through factory") {
            // This would test mission factory integration
            // Implementation depends on Factory.hpp being available
            
            MissionParameters explorationParams{
                MissionType::EXPLORATION,
                fleets[0].get(),
                planets[0].get(),
                40.0,
                {}
            };
            
            // auto mission = MissionFactory::createMission(explorationParams);
            // REQUIRE(mission != nullptr);
            // REQUIRE(mission->getType() == MissionType::EXPLORATION);
        }
    }
}

TEST_CASE_METHOD(MissionTestFixture, "Mission Chain and Dependencies", "[mission][chain][core]") {
    
    SECTION("Sequential Mission Execution") {
        // Create a chain of dependent missions
        auto explorationMission = std::make_unique<ExplorationMission>(
            fleets[0].get(), planets[0].get(), 20.0);
        
        auto colonizationMission = std::make_unique<ColonizationMission>(
            fleets[1].get(), planets[0].get(), 60.0);
        
        SECTION("Mission prerequisites") {
            // Colonization should depend on successful exploration
            colonizationMission->addPrerequisite(explorationMission->getId());
            
            // Cannot start colonization before exploration
            REQUIRE_THROWS_AS(
                colonizationMission->start(),
                MissionPrerequisiteException
            );
            
            // Complete exploration first
            explorationMission->start();
            explorationMission->update(20.0);
            REQUIRE(explorationMission->isCompleted());
            
            // Now colonization should be able to start
            colonizationMission->checkPrerequisites();
            REQUIRE_NOTHROW(colonizationMission->start());
        }
        
        SECTION("Mission result propagation") {
            // Exploration results should influence colonization
            explorationMission->start();
            explorationMission->update(20.0);
            
            auto explorationResults = explorationMission->getExplorationResults();
            colonizationMission->useExplorationData(explorationResults);
            
            // Colonization should be more efficient with exploration data
            colonizationMission->start();
            colonizationMission->update(30.0);
            
            REQUIRE(colonizationMission->getProgress() > 50.0); // Faster due to data
        }
    }
    
    SECTION("Parallel Mission Execution") {
        // Multiple fleets can execute missions simultaneously
        auto mission1 = std::make_unique<ExplorationMission>(
            fleets[0].get(), planets[0].get(), 30.0);
        auto mission2 = std::make_unique<CombatMission>(
            fleets[2].get(), planets[1].get(), 25.0);
        
        mission1->start();
        mission2->start();
        
        SECTION("Independent progress") {
            mission1->update(15.0);
            mission2->update(10.0);
            
            // Missions should progress independently
            REQUIRE(mission1->getProgress() == Approx(50.0));
            REQUIRE(mission2->getProgress() == Approx(40.0));
            
            // Different completion times
            mission1->update(15.0);
            mission2->update(15.0);
            
            REQUIRE(mission1->isCompleted());
            REQUIRE(mission2->isCompleted());
        }
        
        SECTION("Resource contention") {
            // If missions compete for resources, behavior should be defined
            // This depends on ResourceManager implementation
            
            mission1->update(30.0);
            mission2->update(25.0);
            
            REQUIRE((mission1->isCompleted() || mission1->isFailed()));
            REQUIRE((mission2->isCompleted() || mission2->isFailed()));
        }
    }
}

TEST_CASE_METHOD(MissionTestFixture, "Mission Performance and Optimization", "[mission][performance][core]") {
    
    SECTION("Mission Creation Performance") {
        const int missionCount = 1000;
        std::vector<std::unique_ptr<Mission>> missions;
        missions.reserve(missionCount);
        
        auto duration = measurePerformance([&]() {
            for (int i = 0; i < missionCount; ++i) {
                MissionType type = static_cast<MissionType>(i % 3);
                switch (type) {
                    case MissionType::EXPLORATION:
                        missions.push_back(std::make_unique<ExplorationMission>(
                            fleets[0].get(), planets[0].get(), 30.0));
                        break;
                    case MissionType::COLONIZATION:
                        missions.push_back(std::make_unique<ColonizationMission>(
                            fleets[1].get(), planets[2].get(), 60.0));
                        break;
                    case MissionType::COMBAT:
                        missions.push_back(std::make_unique<CombatMission>(
                            fleets[2].get(), planets[1].get(), 25.0));
                        break;
                }
            }
        }, "Creating " + std::to_string(missionCount) + " missions");
        
        REQUIRE(duration.count() < 200000); // 200ms threshold
        REQUIRE(missions.size() == missionCount);
    }
    
    SECTION("Mission Update Performance") {
        std::vector<std::unique_ptr<Mission>> missions;
        for (int i = 0; i < 100; ++i) {
            auto mission = std::make_unique<ExplorationMission>(
                fleets[0].get(), planets[0].get(), 60.0);
            mission->start();
            missions.push_back(std::move(mission));
        }
        
        auto duration = measurePerformance([&]() {
            for (auto& mission : missions) {
                mission->update(1.0);
            }
        }, "Updating 100 missions");
        
        REQUIRE(duration.count() < 50000); // 50ms threshold
    }
    
    SECTION("Memory Usage Optimization") {
        MemoryTracker::printMemoryStats("Before mission batch creation");
        
        {
            std::vector<std::unique_ptr<Mission>> largeMissionBatch;
            for (int i = 0; i < 10000; ++i) {
                largeMissionBatch.push_back(std::make_unique<ExplorationMission>(
                    fleets[0].get(), planets[0].get(), 30.0));
            }
            MemoryTracker::printMemoryStats("After creating 10000 missions");
        } // missions destroyed here
        
        MemoryTracker::printMemoryStats("After mission cleanup");
    }
}

TEST_CASE_METHOD(MissionTestFixture, "Mission Exception Handling and Edge Cases", "[mission][exceptions][core]") {
    
    SECTION("Invalid Mission States") {
        auto mission = std::make_unique<ExplorationMission>(
            fleets[0].get(), planets[0].get(), 30.0);
        
        SECTION("Operating on non-started mission") {
            // Should not be able to update before starting
            REQUIRE_THROWS_AS(mission->update(10.0), InvalidOperationException);
        }
        
        SECTION("Double start prevention") {
            mission->start();
            REQUIRE_THROWS_AS(mission->start(), InvalidOperationException);
        }
        
        SECTION("Operating on completed mission") {
            mission->start();
            mission->update(30.0);
            REQUIRE(mission->isCompleted());
            
            // Should not be able to update completed mission
            REQUIRE_THROWS_AS(mission->update(5.0), InvalidOperationException);
        }
    }
    
    SECTION("Resource Unavailability") {
        auto mission = std::make_unique<ColonizationMission>(
            fleets[1].get(), planets[2].get(), 50.0);
        
        SECTION("Fleet becomes unavailable") {
            mission->start();
            fleets[1]->setStatus(EntityStatus::DESTROYED);
            
            // Mission should fail when fleet is destroyed
            mission->update(10.0);
            REQUIRE(mission->isFailed());
        }
        
        SECTION("Target becomes inaccessible") {
            mission->start();
            planets[2]->setStatus(EntityStatus::DESTROYED);
            
            // Mission should fail when target is destroyed
            mission->update(10.0);
            REQUIRE(mission->isFailed());
        }
    }
    
    SECTION("Extreme Parameter Values") {
        SECTION("Zero duration mission") {
            REQUIRE_THROWS_AS(
                std::make_unique<ExplorationMission>(fleets[0].get(), planets[0].get(), 0.0),
                InvalidArgumentException
            );
        }
        
        SECTION("Negative duration") {
            REQUIRE_THROWS_AS(
                std::make_unique<ExplorationMission>(fleets[0].get(), planets[0].get(), -10.0),
                InvalidArgumentException
            );
        }
        
        SECTION("Extremely long duration") {
            auto mission = std::make_unique<ExplorationMission>(
                fleets[0].get(), planets[0].get(), 1000000.0);
            
            mission->start();
            mission->update(100.0);
            
            REQUIRE(mission->getProgress() < 1.0); // Very slow progress
            REQUIRE(mission->isActive());
        }
    }
}

TEST_CASE_METHOD(MissionTestFixture, "Mission Event System", "[mission][events][core]") {
    
    SECTION("Mission Event Generation") {
        auto mission = std::make_unique<ExplorationMission>(
            fleets[0].get(), planets[0].get(), 40.0);
        
        SECTION("Lifecycle events") {
            TestHelpers::ThreadSafeCounter eventCounter;
            
            // Register event handlers
            mission->onStatusChanged([&](MissionStatus oldStatus, MissionStatus newStatus) {
                eventCounter.increment();
                REQUIRE(oldStatus != newStatus);
            });
            
            mission->onProgressUpdated([&](double progress) {
                if (progress > 0) eventCounter.increment();
            });
            
            mission->onCompleted([&]() {
                eventCounter.increment();
            });
            
            // Execute mission and verify events
            mission->start(); // Should trigger status change event
            mission->update(20.0); // Should trigger progress events
            mission->update(20.0); // Should trigger completion event
            
            REQUIRE(eventCounter.get() >= 3);
        }
        
        SECTION("Mission-specific events") {
            auto explorationMission = std::make_unique<ExplorationMission>(
                fleets[0].get(), planets[0].get(), 30.0);
            
            bool discoveryEventFired = false;
            explorationMission->onDiscovery([&](const DiscoveryEvent& event) {
                discoveryEventFired = true;
                REQUIRE(!event.discoveryType.empty());
            });
            
            explorationMission->start();
            explorationMission->update(30.0);
            
            REQUIRE(discoveryEventFired);
        }
    }
    
    SECTION("Mission Coordination Events") {
        auto mission1 = std::make_unique<ExplorationMission>(
            fleets[0].get(), planets[0].get(), 25.0);
        auto mission2 = std::make_unique<ColonizationMission>(
            fleets[1].get(), planets[0].get(), 50.0);
        
        SECTION("Cross-mission communication") {
            bool coordinationEventFired = false;
            
            mission1->onCompleted([&]() {
                mission2->receiveExplorationData(mission1->getExplorationResults());
                coordinationEventFired = true;
            });
            
            mission1->start();
            mission1->update(25.0);
            
            REQUIRE(coordinationEventFired);
            REQUIRE(mission2->hasExplorationData());
        }
    }
}