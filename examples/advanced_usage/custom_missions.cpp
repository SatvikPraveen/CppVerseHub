// File: examples/advanced_usage/custom_missions.cpp
// CppVerseHub Custom Mission Types Example
// Advanced demonstration of creating custom mission types with complex behaviors

#include <iostream>
#include <memory>
#include <vector>
#include <chrono>
#include <future>
#include <random>
#include <algorithm>
#include <map>

// Core includes
#include "../../src/core/Mission.hpp"
#include "../../src/core/Fleet.hpp"
#include "../../src/core/Planet.hpp"
#include "../../src/core/ResourceManager.hpp"
#include "../../src/utils/Logger.hpp"
#include "../../src/patterns/Decorator.hpp"
#include "../../src/patterns/State.hpp"

using namespace std;
using namespace chrono;

// Forward declarations
class Ship;
enum class ShipType { SCIENCE, TRANSPORT, COMBAT };
enum class PlanetType { TERRESTRIAL, GAS_GIANT, ICE_WORLD };

/**
 * @brief Dummy Ship class for example
 */
class Ship {
public:
    string name;
    ShipType type;
    int capacity;
    
    Ship(const string& n, ShipType t, int c) : name(n), type(t), capacity(c) {}
};

/**
 * @brief Vector3D for position calculations
 */
struct Vector3D {
    double x, y, z;
    Vector3D(double x = 0, double y = 0, double z = 0) : x(x), y(y), z(z) {}
};

/**
 * @brief Custom Research Mission - Advanced scientific exploration
 */
class ResearchMission : public Mission {
private:
    string researchType;
    int requiredSciencePoints;
    int currentSciencePoints = 0;
    map<string, double> discoveryProbabilities;
    mutable mt19937 rng{random_device{}()};
    
public:
    ResearchMission(Fleet* fleet, Planet* target, const string& description,
                   const string& research_type, int required_points)
        : Mission(MissionType::EXPLORATION, fleet, target, description)
        , researchType(research_type)
        , requiredSciencePoints(required_points) {
        
        // Initialize discovery probabilities based on research type
        if (research_type == "xenobiology") {
            discoveryProbabilities = {
                {"life_forms", 0.3},
                {"genetic_material", 0.2}, 
                {"evolutionary_data", 0.25}
            };
        } else if (research_type == "geology") {
            discoveryProbabilities = {
                {"rare_minerals", 0.4},
                {"geological_formations", 0.35},
                {"seismic_data", 0.3}
            };
        } else if (research_type == "atmospheric") {
            discoveryProbabilities = {
                {"atmospheric_composition", 0.5},
                {"weather_patterns", 0.3},
                {"climate_data", 0.4}
            };
        }
        
        cout << "Created research mission: " << description << " studying " << research_type << endl;
    }
    
    MissionResult execute() override {
        cout << "Executing research mission: " << getDescription() << endl;
        
        MissionResult result;
        result.missionId = getId();
        result.timestamp = system_clock::now();
        
        try {
            // Check if fleet has required science equipment
            if (!hasRequiredEquipment()) {
                result.success = false;
                result.report = "Fleet lacks required scientific equipment for " + researchType + " research";
                return result;
            }
            
            // Simulate research progress
            simulateResearchProgress();
            
            if (currentSciencePoints >= requiredSciencePoints) {
                result.success = true;
                result.report = "Research mission completed successfully. " +
                              to_string(currentSciencePoints) + " science points gathered.";
                
                // Generate discoveries
                generateResearchDiscoveries(result);
                
                // Update planet data if discoveries were made
                if (!result.discoveredResources.empty()) {
                    updatePlanetData(result.discoveredResources);
                }
                
            } else {
                result.success = false;
                result.report = "Research mission failed. Only gathered " +
                              to_string(currentSciencePoints) + "/" +
                              to_string(requiredSciencePoints) + " science points.";
            }
            
        } catch (const exception& e) {
            result.success = false;
            result.report = "Research mission failed due to exception: " + string(e.what());
            cerr << "Research mission exception: " << e.what() << endl;
        }
        
        setComplete(true);
        return result;
    }
    
private:
    bool hasRequiredEquipment() const {
        // In a full implementation, check if fleet has science vessels
        return true; // Simplified for example
    }
    
    void simulateResearchProgress() {
        uniform_int_distribution<int> progressDist(10, 30);
        
        for (int day = 0; day < 10; ++day) {  // 10-day research mission
            int dailyProgress = progressDist(rng);
            
            // Science vessels would provide bonus in full implementation
            dailyProgress += 15; // Simplified bonus
            
            currentSciencePoints += dailyProgress;
            
            cout << "Research day " << (day + 1) << ": +" << dailyProgress 
                 << " science points (total: " << currentSciencePoints << ")" << endl;
        }
    }
    
    void generateResearchDiscoveries(MissionResult& result) {
        for (const auto& [discovery, probability] : discoveryProbabilities) {
            uniform_real_distribution<double> probDist(0.0, 1.0);
            
            if (probDist(rng) < probability) {
                uniform_int_distribution<int> amountDist(1, 10);
                int amount = amountDist(rng);
                
                result.discoveredResources[discovery] = amount;
                cout << "Discovered: " << discovery << " (amount: " << amount << ")" << endl;
            }
        }
    }
    
    void updatePlanetData(const map<string, int>& discoveries) {
        // Add discovered resources to target planet
        for (const auto& [resource, amount] : discoveries) {
            // In full implementation: targetEntity->addResources(resource, amount);
        }
        
        cout << "Updated planet data with research discoveries" << endl;
    }
};

/**
 * @brief Custom Trade Mission - Complex economic interactions
 */
class TradeMission : public Mission {
private:
    Planet* originPlanet;
    map<string, int> tradeGoods;
    map<string, double> priceModifiers;
    double expectedProfit = 0.0;
    double actualProfit = 0.0;
    
public:
    TradeMission(Fleet* fleet, Planet* origin, Planet* destination,
                const string& description, const map<string, int>& goods)
        : Mission(MissionType::COLONIZATION, fleet, destination, description)  // Using COLONIZATION as base
        , originPlanet(origin)
        , tradeGoods(goods) {
        
        calculateExpectedProfit();
        cout << "Created trade mission: " << description << " with expected profit: " << expectedProfit << endl;
    }
    
    MissionResult execute() override {
        cout << "Executing trade mission: " << getDescription() << endl;
        
        MissionResult result;
        result.missionId = getId();
        result.timestamp = system_clock::now();
        
        try {
            // Step 1: Load goods from origin
            if (!loadTradeGoods()) {
                result.success = false;
                result.report = "Failed to load trade goods from origin planet";
                return result;
            }
            
            // Step 2: Travel to destination (simulated)
            simulateTradeJourney();
            
            // Step 3: Negotiate and execute trade
            if (executeTradeNegotiation()) {
                result.success = true;
                result.report = "Trade mission successful! Profit: " + to_string(actualProfit) + " credits";
                
                // Return profit as "discovered resources"
                result.discoveredResources["credits"] = static_cast<int>(actualProfit);
                
                // Update planet economies
                updatePlanetEconomies();
                
            } else {
                result.success = false;
                result.report = "Trade negotiations failed. Loss: " + to_string(-actualProfit) + " credits";
            }
            
        } catch (const exception& e) {
            result.success = false;
            result.report = "Trade mission failed: " + string(e.what());
            cerr << "Trade mission exception: " << e.what() << endl;
        }
        
        setComplete(true);
        return result;
    }
    
private:
    void calculateExpectedProfit() {
        expectedProfit = 0.0;
        for (const auto& [good, quantity] : tradeGoods) {
            double basePrice = getBasePrice(good);
            expectedProfit += quantity * basePrice * 0.2; // 20% markup
        }
    }
    
    double getBasePrice(const string& good) const {
        static const map<string, double> basePrices = {
            {"minerals", 10.0},
            {"energy", 15.0},
            {"food", 8.0},
            {"technology", 50.0},
            {"luxury_goods", 100.0}
        };
        
        auto it = basePrices.find(good);
        return it != basePrices.end() ? it->second : 20.0;
    }
    
    bool loadTradeGoods() {
        cout << "Loading trade goods from origin planet" << endl;
        // In full implementation, check and consume resources from origin
        return true;
    }
    
    void simulateTradeJourney() {
        // Simulate random events during journey
        mt19937 rng{random_device{}()};
        uniform_real_distribution<double> eventChance(0.0, 1.0);
        
        if (eventChance(rng) < 0.1) {  // 10% chance of pirate attack
            cout << "Warning: Pirate attack during trade journey!" << endl;
            // Reduce goods by 10-30%
            uniform_real_distribution<double> lossDist(0.1, 0.3);
            double lossRatio = lossDist(rng);
            
            for (auto& [good, quantity] : tradeGoods) {
                int loss = static_cast<int>(quantity * lossRatio);
                quantity -= loss;
                cout << "Lost " << loss << " units of " << good << " to pirates" << endl;
            }
        }
        
        if (eventChance(rng) < 0.05) {  // 5% chance of finding derelict ship
            cout << "Found derelict ship with valuable technology!" << endl;
            tradeGoods["technology"] += uniform_int_distribution<int>(1, 5)(rng);
        }
    }
    
    bool executeTradeNegotiation() {
        mt19937 rng{random_device{}()};
        uniform_real_distribution<double> negotiationDist(0.8, 1.2);  // 80%-120% of expected
        
        double negotiationModifier = negotiationDist(rng);
        actualProfit = expectedProfit * negotiationModifier;
        
        cout << "Trade negotiation completed with " << (negotiationModifier * 100) << "% efficiency" << endl;
        
        return actualProfit > 0;  // Success if profitable
    }
    
    void updatePlanetEconomies() {
        cout << "Updated economies of both planets" << endl;
    }
};

/**
 * @brief Custom Rescue Mission - Emergency response with time pressure
 */
class RescueMission : public Mission {
private:
    int survivorCount;
    chrono::system_clock::time_point missionDeadline;
    string emergencyType;
    bool timeExpired = false;
    
public:
    RescueMission(Fleet* fleet, Planet* target, const string& description,
                 int survivors, const string& emergency_type, int hours_limit)
        : Mission(MissionType::EXPLORATION, fleet, target, description)
        , survivorCount(survivors)
        , emergencyType(emergency_type)
        , missionDeadline(system_clock::now() + hours{hours_limit}) {
        
        cout << "Created rescue mission: " << description << " - " << survivors 
             << " survivors, " << hours_limit << " hour limit" << endl;
    }
    
    MissionResult execute() override {
        cout << "Executing rescue mission: " << getDescription() << endl;
        
        MissionResult result;
        result.missionId = getId();
        result.timestamp = system_clock::now();
        
        try {
            // Check if mission has expired
            if (system_clock::now() > missionDeadline) {
                timeExpired = true;
                result.success = false;
                result.report = "Rescue mission failed - time limit exceeded";
                cout << "Error: Rescue mission exceeded time limit" << endl;
                return result;
            }
            
            // Check fleet capacity for survivors (simplified)
            if (1000 < survivorCount) { // Assume fleet capacity of 1000
                result.success = false;
                result.report = "Fleet lacks sufficient capacity for all survivors";
                return result;
            }
            
            // Execute rescue based on emergency type
            int rescuedCount = executeRescueOperation();
            
            if (rescuedCount > 0) {
                result.success = true;
                result.report = "Rescue mission successful! Saved " + to_string(rescuedCount) + 
                              "/" + to_string(survivorCount) + " survivors";
                
                result.discoveredResources["rescued_personnel"] = rescuedCount;
                
                // Update fleet with survivors
                updateFleetWithSurvivors(rescuedCount);
                
            } else {
                result.success = false;
                result.report = "Rescue mission failed - no survivors could be saved";
            }
            
        } catch (const exception& e) {
            result.success = false;
            result.report = "Rescue mission failed: " + string(e.what());
            cerr << "Rescue mission exception: " << e.what() << endl;
        }
        
        setComplete(true);
        return result;
    }
    
private:
    int executeRescueOperation() {
        mt19937 rng{random_device{}()};
        
        // Success rate depends on emergency type and fleet composition
        double baseSuccessRate = 0.7;
        
        if (emergencyType == "natural_disaster") {
            baseSuccessRate = 0.8;
        } else if (emergencyType == "hostile_attack") {
            baseSuccessRate = 0.5;
        } else if (emergencyType == "equipment_failure") {
            baseSuccessRate = 0.9;
        }
        
        // Fleet strength improves rescue chances (simplified)
        baseSuccessRate += 0.2; // Assume good fleet
        baseSuccessRate = min(0.95, baseSuccessRate);  // Cap at 95%
        
        // Calculate how many survivors are rescued
        uniform_real_distribution<double> rescueDist(0.0, 1.0);
        int rescuedCount = 0;
        
        for (int i = 0; i < survivorCount; ++i) {
            if (rescueDist(rng) < baseSuccessRate) {
                rescuedCount++;
            }
        }
        
        cout << "Rescue operation saved " << rescuedCount << " out of " << survivorCount << " survivors" << endl;
        return rescuedCount;
    }
    
    void updateFleetWithSurvivors(int rescued) {
        cout << "Fleet now carrying " << rescued << " rescued personnel" << endl;
        // In a full implementation, this would update fleet passenger manifest
    }
};

/**
 * @brief Mission decorator for adding urgency/priority
 */
class UrgentMissionDecorator : public Mission {
private:
    unique_ptr<Mission> wrappedMission;
    double priorityMultiplier;
    
public:
    UrgentMissionDecorator(unique_ptr<Mission> mission, double priority = 2.0)
        : Mission(mission->getMissionType(), mission->getAssignedFleet(), 
                 mission->getTarget(), "URGENT: " + mission->getDescription())
        , wrappedMission(move(mission))
        , priorityMultiplier(priority) {
        
        cout << "Applied urgent priority to mission: " << getDescription() << endl;
    }
    
    MissionResult execute() override {
        cout << "Executing urgent mission with " << priorityMultiplier << "x priority" << endl;
        
        auto result = wrappedMission->execute();
        
        // Urgent missions may have enhanced rewards
        for (auto& [resource, amount] : result.discoveredResources) {
            amount = static_cast<int>(amount * priorityMultiplier);
        }
        
        if (result.success) {
            result.report += " (URGENT MISSION BONUS APPLIED)";
        }
        
        return result;
    }
    
    double getProgress() const override {
        return wrappedMission->getProgress();
    }
};

/**
 * @brief Create mock planets and fleets for demonstration
 */
void createMockEntities(unique_ptr<Planet>& homeBase, unique_ptr<Planet>& researchTarget,
                       unique_ptr<Planet>& tradeTarget, unique_ptr<Planet>& emergencyPlanet,
                       unique_ptr<Fleet>& scienceExpedition, unique_ptr<Fleet>& tradeConvoy,
                       unique_ptr<Fleet>& rescueSquad) {
    
    // Create test planets
    homeBase = make_unique<Planet>("Alpha Station", Vector3D{0, 0, 0}, PlanetType::TERRESTRIAL);
    researchTarget = make_unique<Planet>("Kepler-438b", Vector3D{100, 50, 25}, PlanetType::TERRESTRIAL);
    tradeTarget = make_unique<Planet>("Merchant Hub", Vector3D{-75, 100, 60}, PlanetType::TERRESTRIAL);
    emergencyPlanet = make_unique<Planet>("Disaster Zone", Vector3D{200, -100, 75}, PlanetType::TERRESTRIAL);
    
    // Create specialized fleets (simplified for example)
    scienceExpedition = make_unique<Fleet>("Science Expedition", Vector3D{0, 0, 0});
    tradeConvoy = make_unique<Fleet>("Trade Convoy", Vector3D{0, 0, 0});
    rescueSquad = make_unique<Fleet>("Rescue Squadron", Vector3D{0, 0, 0});
    
    cout << "Created specialized fleets and planets for mission testing\n" << endl;
}

/**
 * @brief Demonstration of custom mission system
 */
void demonstrateCustomMissions() {
    cout << "=== Custom Mission Types Demonstration ===" << endl;
    
    // Create test entities
    unique_ptr<Planet> homeBase, researchTarget, tradeTarget, emergencyPlanet;
    unique_ptr<Fleet> scienceExpedition, tradeConvoy, rescueSquad;
    
    createMockEntities(homeBase, researchTarget, tradeTarget, emergencyPlanet,
                      scienceExpedition, tradeConvoy, rescueSquad);
    
    // 1. Research Mission
    cout << "=== Testing Research Mission ===" << endl;
    auto researchMission = make_unique<ResearchMission>(
        scienceExpedition.get(),
        researchTarget.get(),
        "Xenobiology survey of Kepler-438b",
        "xenobiology",
        150  // Required science points
    );
    
    auto researchResult = researchMission->execute();
    cout << "Research Result: " << (researchResult.success ? "SUCCESS" : "FAILURE") << endl;
    cout << "Report: " << researchResult.report << endl;
    
    if (!researchResult.discoveredResources.empty()) {
        cout << "Research Discoveries:" << endl;
        for (const auto& [discovery, amount] : researchResult.discoveredResources) {
            cout << "  - " << discovery << ": " << amount << endl;
        }
    }
    
    // 2. Trade Mission
    cout << "\n=== Testing Trade Mission ===" << endl;
    map<string, int> tradeGoods = {
        {"minerals", 200},
        {"food", 150},
        {"energy", 100}
    };
    
    auto tradeMission = make_unique<TradeMission>(
        tradeConvoy.get(),
        homeBase.get(),
        tradeTarget.get(),
        "Establish trade route with Merchant Hub",
        tradeGoods
    );
    
    auto tradeResult = tradeMission->execute();
    cout << "Trade Result: " << (tradeResult.success ? "SUCCESS" : "FAILURE") << endl;
    cout << "Report: " << tradeResult.report << endl;
    
    // 3. Rescue Mission
    cout << "\n=== Testing Rescue Mission ===" << endl;
    auto rescueMission = make_unique<RescueMission>(
        rescueSquad.get(),
        emergencyPlanet.get(),
        "Emergency evacuation from disaster zone",
        250,  // Survivors
        "natural_disaster",
        6     // 6 hour limit
    );
    
    auto rescueResult = rescueMission->execute();
    cout << "Rescue Result: " << (rescueResult.success ? "SUCCESS" : "FAILURE") << endl;
    cout << "Report: " << rescueResult.report << endl;
    
    // 4. Urgent Mission with Decorator
    cout << "\n=== Testing Urgent Mission Decorator ===" << endl;
    auto urgentResearch = make_unique<UrgentMissionDecorator>(
        make_unique<ResearchMission>(
            scienceExpedition.get(),
            researchTarget.get(),
            "Critical atmospheric analysis",
            "atmospheric",
            100
        ),
        1.5  // 1.5x priority multiplier
    );
    
    auto urgentResult = urgentResearch->execute();
    cout << "Urgent Mission Result: " << (urgentResult.success ? "SUCCESS" : "FAILURE") << endl;
    cout << "Report: " << urgentResult.report << endl;
    
    cout << "\n=== Custom Mission Demonstration Complete ===" << endl;
}

/**
 * @brief Main function
 */
int main() {
    cout << "CppVerseHub - Custom Mission Types Example" << endl;
    cout << "==========================================" << endl;
    
    try {
        demonstrateCustomMissions();
        
        cout << "\nCustom mission example completed successfully!" << endl;
        cout << "\nThis example demonstrated:" << endl;
        cout << "  - Custom ResearchMission with scientific discovery" << endl;
        cout << "  - Custom TradeMission with economic simulation" << endl;
        cout << "  - Custom RescueMission with time pressure" << endl;
        cout << "  - Mission decorator pattern for urgency" << endl;
        cout << "  - Complex mission result handling" << endl;
        
        return 0;
        
    } catch (const exception& e) {
        cerr << "Error in custom missions example: " << e.what() << endl;
        return 1;
    }
}