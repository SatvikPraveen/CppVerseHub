// File: tests/unit_tests/pattern_tests/StrategyTests.cpp
// Strategy pattern tests for CppVerseHub design patterns showcase

#include <catch2/catch.hpp>
#include <memory>
#include <vector>
#include <string>
#include <algorithm>
#include <functional>

// Include the strategy pattern headers
#include "Strategy.hpp"
#include "Planet.hpp"
#include "Fleet.hpp"

using namespace CppVerseHub::Patterns;
using namespace CppVerseHub::Core;

/**
 * @brief Test fixture for strategy pattern tests
 */
class StrategyTestFixture {
public:
    StrategyTestFixture() {
        setupTestData();
    }
    
    ~StrategyTestFixture() {
        testPlanets.clear();
        testFleets.clear();
    }
    
protected:
    void setupTestData() {
        // Create test planets
        for (int i = 0; i < 8; ++i) {
            auto planet = std::make_unique<Planet>(
                "StrategyPlanet_" + std::to_string(i),
                Vector3D{i * 100.0, i * 100.0, i * 100.0}
            );
            planet->setResourceAmount(ResourceType::MINERALS, 1000 + i * 200);
            planet->setResourceAmount(ResourceType::ENERGY, 500 + i * 150);
            planet->setHabitabilityRating(0.3 + i * 0.08);
            testPlanets.push_back(std::move(planet));
        }
        
        // Create test fleets
        for (int i = 0; i < 4; ++i) {
            auto fleet = std::make_unique<Fleet>(
                "StrategyFleet_" + std::to_string(i),
                Vector3D{i * 200.0, i * 200.0, i * 200.0}
            );
            fleet->addShips(ShipType::FIGHTER, 10 + i * 5);
            fleet->addShips(ShipType::CRUISER, 3 + i * 2);
            if (i % 2 == 0) {
                fleet->addShips(ShipType::BATTLESHIP, 1 + i);
            }
            testFleets.push_back(std::move(fleet));
        }
    }
    
    std::vector<std::unique_ptr<Planet>> testPlanets;
    std::vector<std::unique_ptr<Fleet>> testFleets;
};

/**
 * @brief Abstract sorting strategy for testing
 */
template<typename T>
class SortingStrategy {
public:
    virtual ~SortingStrategy() = default;
    virtual void sort(std::vector<T>& data) = 0;
    virtual std::string getName() const = 0;
};

/**
 * @brief Quick sort strategy
 */
template<typename T>
class QuickSortStrategy : public SortingStrategy<T> {
public:
    void sort(std::vector<T>& data) override {
        if (data.size() <= 1) return;
        quickSort(data, 0, data.size() - 1);
    }
    
    std::string getName() const override {
        return "QuickSort";
    }
    
private:
    void quickSort(std::vector<T>& data, int low, int high) {
        if (low < high) {
            int pivot = partition(data, low, high);
            quickSort(data, low, pivot - 1);
            quickSort(data, pivot + 1, high);
        }
    }
    
    int partition(std::vector<T>& data, int low, int high) {
        T pivot = data[high];
        int i = low - 1;
        
        for (int j = low; j < high; ++j) {
            if (data[j] < pivot) {
                ++i;
                std::swap(data[i], data[j]);
            }
        }
        
        std::swap(data[i + 1], data[high]);
        return i + 1;
    }
};

/**
 * @brief Merge sort strategy
 */
template<typename T>
class MergeSortStrategy : public SortingStrategy<T> {
public:
    void sort(std::vector<T>& data) override {
        if (data.size() <= 1) return;
        mergeSort(data, 0, data.size() - 1);
    }
    
    std::string getName() const override {
        return "MergeSort";
    }
    
private:
    void mergeSort(std::vector<T>& data, int left, int right) {
        if (left < right) {
            int mid = left + (right - left) / 2;
            mergeSort(data, left, mid);
            mergeSort(data, mid + 1, right);
            merge(data, left, mid, right);
        }
    }
    
    void merge(std::vector<T>& data, int left, int mid, int right) {
        std::vector<T> temp(right - left + 1);
        int i = left, j = mid + 1, k = 0;
        
        while (i <= mid && j <= right) {
            if (data[i] <= data[j]) {
                temp[k++] = data[i++];
            } else {
                temp[k++] = data[j++];
            }
        }
        
        while (i <= mid) temp[k++] = data[i++];
        while (j <= right) temp[k++] = data[j++];
        
        for (int i = 0; i < k; ++i) {
            data[left + i] = temp[i];
        }
    }
};

/**
 * @brief Bubble sort strategy
 */
template<typename T>
class BubbleSortStrategy : public SortingStrategy<T> {
public:
    void sort(std::vector<T>& data) override {
        int n = static_cast<int>(data.size());
        for (int i = 0; i < n - 1; ++i) {
            for (int j = 0; j < n - i - 1; ++j) {
                if (data[j] > data[j + 1]) {
                    std::swap(data[j], data[j + 1]);
                }
            }
        }
    }
    
    std::string getName() const override {
        return "BubbleSort";
    }
};

/**
 * @brief Context class that uses sorting strategies
 */
template<typename T>
class SortingContext {
private:
    std::unique_ptr<SortingStrategy<T>> strategy_;
    
public:
    void setStrategy(std::unique_ptr<SortingStrategy<T>> strategy) {
        strategy_ = std::move(strategy);
    }
    
    void sort(std::vector<T>& data) {
        if (strategy_) {
            strategy_->sort(data);
        }
    }
    
    std::string getCurrentStrategyName() const {
        return strategy_ ? strategy_->getName() : "No Strategy";
    }
};

/**
 * @brief Fleet routing strategies
 */
class FleetRoutingStrategy {
public:
    virtual ~FleetRoutingStrategy() = default;
    virtual std::vector<Planet*> planRoute(Fleet* fleet, const std::vector<Planet*>& planets) = 0;
    virtual std::string getStrategyName() const = 0;
};

class NearestPlanetStrategy : public FleetRoutingStrategy {
public:
    std::vector<Planet*> planRoute(Fleet* fleet, const std::vector<Planet*>& planets) override {
        std::vector<Planet*> route;
        std::vector<Planet*> remaining = planets;
        Vector3D currentPos = fleet->getPosition();
        
        while (!remaining.empty()) {
            // Find nearest planet
            auto nearest = std::min_element(remaining.begin(), remaining.end(),
                [&currentPos](const Planet* a, const Planet* b) {
                    double distA = calculateDistance(currentPos, a->getPosition());
                    double distB = calculateDistance(currentPos, b->getPosition());
                    return distA < distB;
                });
            
            route.push_back(*nearest);
            currentPos = (*nearest)->getPosition();
            remaining.erase(nearest);
        }
        
        return route;
    }
    
    std::string getStrategyName() const override {
        return "NearestPlanet";
    }
    
private:
    double calculateDistance(const Vector3D& a, const Vector3D& b) {
        double dx = a.x - b.x;
        double dy = a.y - b.y;
        double dz = a.z - b.z;
        return std::sqrt(dx*dx + dy*dy + dz*dz);
    }
};

class HighestValueStrategy : public FleetRoutingStrategy {
public:
    std::vector<Planet*> planRoute(Fleet* fleet, const std::vector<Planet*>& planets) override {
        std::vector<Planet*> route = planets;
        
        // Sort by resource value (descending)
        std::sort(route.begin(), route.end(), [](const Planet* a, const Planet* b) {
            int valueA = a->getResourceAmount(ResourceType::MINERALS) + 
                        a->getResourceAmount(ResourceType::ENERGY);
            int valueB = b->getResourceAmount(ResourceType::MINERALS) + 
                        b->getResourceAmount(ResourceType::ENERGY);
            return valueA > valueB;
        });
        
        return route;
    }
    
    std::string getStrategyName() const override {
        return "HighestValue";
    }
};

class BalancedStrategy : public FleetRoutingStrategy {
public:
    std::vector<Planet*> planRoute(Fleet* fleet, const std::vector<Planet*>& planets) override {
        std::vector<std::pair<Planet*, double>> scored;
        Vector3D fleetPos = fleet->getPosition();
        
        // Calculate score based on value/distance ratio
        for (Planet* planet : planets) {
            double distance = calculateDistance(fleetPos, planet->getPosition());
            double value = planet->getResourceAmount(ResourceType::MINERALS) + 
                          planet->getResourceAmount(ResourceType::ENERGY);
            double score = distance > 0 ? value / distance : value;
            scored.emplace_back(planet, score);
        }
        
        // Sort by score (descending)
        std::sort(scored.begin(), scored.end(), [](const auto& a, const auto& b) {
            return a.second > b.second;
        });
        
        std::vector<Planet*> route;
        for (const auto& pair : scored) {
            route.push_back(pair.first);
        }
        
        return route;
    }
    
    std::string getStrategyName() const override {
        return "Balanced";
    }
    
private:
    double calculateDistance(const Vector3D& a, const Vector3D& b) {
        double dx = a.x - b.x;
        double dy = a.y - b.y;
        double dz = a.z - b.z;
        return std::sqrt(dx*dx + dy*dy + dz*dz);
    }
};

class FleetRouter {
private:
    std::unique_ptr<FleetRoutingStrategy> strategy_;
    
public:
    void setStrategy(std::unique_ptr<FleetRoutingStrategy> strategy) {
        strategy_ = std::move(strategy);
    }
    
    std::vector<Planet*> planRoute(Fleet* fleet, const std::vector<Planet*>& planets) {
        if (!strategy_) {
            return planets; // Default: return as-is
        }
        return strategy_->planRoute(fleet, planets);
    }
    
    std::string getCurrentStrategy() const {
        return strategy_ ? strategy_->getStrategyName() : "Default";
    }
};

TEST_CASE_METHOD(StrategyTestFixture, "Basic Strategy Pattern", "[strategy][patterns][basic]") {
    
    SECTION("Sorting strategies") {
        std::vector<int> testData = {64, 34, 25, 12, 22, 11, 90, 5};
        std::vector<int> expected = {5, 11, 12, 22, 25, 34, 64, 90};
        
        SortingContext<int> context;
        
        SECTION("QuickSort strategy") {
            context.setStrategy(std::make_unique<QuickSortStrategy<int>>());
            
            std::vector<int> data = testData;
            context.sort(data);
            
            REQUIRE(data == expected);
            REQUIRE(context.getCurrentStrategyName() == "QuickSort");
        }
        
        SECTION("MergeSort strategy") {
            context.setStrategy(std::make_unique<MergeSortStrategy<int>>());
            
            std::vector<int> data = testData;
            context.sort(data);
            
            REQUIRE(data == expected);
            REQUIRE(context.getCurrentStrategyName() == "MergeSort");
        }
        
        SECTION("BubbleSort strategy") {
            context.setStrategy(std::make_unique<BubbleSortStrategy<int>>());
            
            std::vector<int> data = testData;
            context.sort(data);
            
            REQUIRE(data == expected);
            REQUIRE(context.getCurrentStrategyName() == "BubbleSort");
        }
        
        SECTION("Strategy switching") {
            std::vector<int> data1 = testData;
            std::vector<int> data2 = testData;
            std::vector<int> data3 = testData;
            
            // Use QuickSort
            context.setStrategy(std::make_unique<QuickSortStrategy<int>>());
            context.sort(data1);
            REQUIRE(data1 == expected);
            
            // Switch to MergeSort
            context.setStrategy(std::make_unique<MergeSortStrategy<int>>());
            context.sort(data2);
            REQUIRE(data2 == expected);
            
            // Switch to BubbleSort
            context.setStrategy(std::make_unique<BubbleSortStrategy<int>>());
            context.sort(data3);
            REQUIRE(data3 == expected);
        }
    }
    
    SECTION("No strategy set") {
        SortingContext<int> context;
        std::vector<int> data = {3, 1, 4, 1, 5};
        
        context.sort(data); // Should do nothing
        
        REQUIRE(data == std::vector<int>{3, 1, 4, 1, 5}); // Unchanged
        REQUIRE(context.getCurrentStrategyName() == "No Strategy");
    }
}

TEST_CASE_METHOD(StrategyTestFixture, "Fleet Routing Strategies", "[strategy][fleet][routing]") {
    
    SECTION("Nearest Planet Strategy") {
        FleetRouter router;
        router.setStrategy(std::make_unique<NearestPlanetStrategy>());
        
        Fleet* testFleet = testFleets[0].get();
        
        // Create planet pointers for routing
        std::vector<Planet*> planets;
        for (const auto& planet : testPlanets) {
            planets.push_back(planet.get());
        }
        
        std::vector<Planet*> route = router.planRoute(testFleet, planets);
        
        REQUIRE(route.size() == planets.size());
        REQUIRE(router.getCurrentStrategy() == "NearestPlanet");
        
        // Verify that the first planet in route is closest to fleet
        Vector3D fleetPos = testFleet->getPosition();
        Planet* firstInRoute = route[0];
        
        for (Planet* planet : planets) {
            if (planet != firstInRoute) {
                double distToFirst = calculateDistance(fleetPos, firstInRoute->getPosition());
                double distToCurrent = calculateDistance(fleetPos, planet->getPosition());
                REQUIRE(distToFirst <= distToCurrent);
            }
        }
    }
    
    SECTION("Highest Value Strategy") {
        FleetRouter router;
        router.setStrategy(std::make_unique<HighestValueStrategy>());
        
        Fleet* testFleet = testFleets[1].get();
        
        std::vector<Planet*> planets;
        for (const auto& planet : testPlanets) {
            planets.push_back(planet.get());
        }
        
        std::vector<Planet*> route = router.planRoute(testFleet, planets);
        
        REQUIRE(route.size() == planets.size());
        REQUIRE(router.getCurrentStrategy() == "HighestValue");
        
        // Verify planets are sorted by descending resource value
        for (size_t i = 1; i < route.size(); ++i) {
            int valueI_1 = route[i-1]->getResourceAmount(ResourceType::MINERALS) + 
                          route[i-1]->getResourceAmount(ResourceType::ENERGY);
            int valueI = route[i]->getResourceAmount(ResourceType::MINERALS) + 
                        route[i]->getResourceAmount(ResourceType::ENERGY);
            
            REQUIRE(valueI_1 >= valueI);
        }
    }
    
    SECTION("Balanced Strategy") {
        FleetRouter router;
        router.setStrategy(std::make_unique<BalancedStrategy>());
        
        Fleet* testFleet = testFleets[2].get();
        
        std::vector<Planet*> planets;
        for (const auto& planet : testPlanets) {
            planets.push_back(planet.get());
        }
        
        std::vector<Planet*> route = router.planRoute(testFleet, planets);
        
        REQUIRE(route.size() == planets.size());
        REQUIRE(router.getCurrentStrategy() == "Balanced");
        
        // Verify the route considers both distance and value
        // The balanced strategy should not be identical to pure distance or value sorting
        std::vector<Planet*> distanceRoute;
        std::vector<Planet*> valueRoute;
        
        // Create distance-sorted route
        NearestPlanetStrategy nearestStrategy;
        distanceRoute = nearestStrategy.planRoute(testFleet, planets);
        
        // Create value-sorted route
        HighestValueStrategy valueStrategy;
        valueRoute = valueStrategy.planRoute(testFleet, planets);
        
        // Balanced route should be different from both pure strategies
        // (unless by coincidence they happen to be the same)
        bool differentFromDistance = (route != distanceRoute);
        bool differentFromValue = (route != valueRoute);
        
        // At least one should be different (in most cases both will be)
        REQUIRE((differentFromDistance || differentFromValue));
    }
    
    SECTION("Strategy switching during runtime") {
        FleetRouter router;
        Fleet* testFleet = testFleets[3].get();
        
        std::vector<Planet*> planets;
        for (size_t i = 0; i < 4; ++i) { // Use subset for clearer testing
            planets.push_back(testPlanets[i].get());
        }
        
        // Start with nearest planet strategy
        router.setStrategy(std::make_unique<NearestPlanetStrategy>());
        std::vector<Planet*> route1 = router.planRoute(testFleet, planets);
        REQUIRE(router.getCurrentStrategy() == "NearestPlanet");
        
        // Switch to highest value strategy
        router.setStrategy(std::make_unique<HighestValueStrategy>());
        std::vector<Planet*> route2 = router.planRoute(testFleet, planets);
        REQUIRE(router.getCurrentStrategy() == "HighestValue");
        
        // Switch to balanced strategy
        router.setStrategy(std::make_unique<BalancedStrategy>());
        std::vector<Planet*> route3 = router.planRoute(testFleet, planets);
        REQUIRE(router.getCurrentStrategy() == "Balanced");
        
        // All routes should contain the same planets but potentially in different orders
        REQUIRE(route1.size() == planets.size());
        REQUIRE(route2.size() == planets.size());
        REQUIRE(route3.size() == planets.size());
        
        // Verify all planets are present in each route
        for (Planet* planet : planets) {
            REQUIRE(std::find(route1.begin(), route1.end(), planet) != route1.end());
            REQUIRE(std::find(route2.begin(), route2.end(), planet) != route2.end());
            REQUIRE(std::find(route3.begin(), route3.end(), planet) != route3.end());
        }
    }
    
private:
    double calculateDistance(const Vector3D& a, const Vector3D& b) {
        double dx = a.x - b.x;
        double dy = a.y - b.y;
        double dz = a.z - b.z;
        return std::sqrt(dx*dx + dy*dy + dz*dz);
    }
};

/**
 * @brief Combat strategies for testing
 */
class CombatStrategy {
public:
    virtual ~CombatStrategy() = default;
    virtual std::string planAttack(Fleet* attacker, Fleet* defender) = 0;
    virtual std::string getStrategyName() const = 0;
};

class AggressiveStrategy : public CombatStrategy {
public:
    std::string planAttack(Fleet* attacker, Fleet* defender) override {
        int attackerBattleships = attacker->getShipCount(ShipType::BATTLESHIP);
        int defenderBattleships = defender->getShipCount(ShipType::BATTLESHIP);
        
        if (attackerBattleships > defenderBattleships) {
            return "FULL_ASSAULT";
        } else if (attacker->getTotalShipCount() > defender->getTotalShipCount()) {
            return "NUMERICAL_ADVANTAGE";
        } else {
            return "DESPERATE_ATTACK";
        }
    }
    
    std::string getStrategyName() const override {
        return "Aggressive";
    }
};

class DefensiveStrategy : public CombatStrategy {
public:
    std::string planAttack(Fleet* attacker, Fleet* defender) override {
        double attackerStrength = calculateFleetStrength(attacker);
        double defenderStrength = calculateFleetStrength(defender);
        
        if (attackerStrength > defenderStrength * 2.0) {
            return "OVERWHELMING_FORCE";
        } else if (attackerStrength > defenderStrength * 1.5) {
            return "CAUTIOUS_ADVANCE";
        } else {
            return "AVOID_COMBAT";
        }
    }
    
    std::string getStrategyName() const override {
        return "Defensive";
    }
    
private:
    double calculateFleetStrength(Fleet* fleet) {
        return fleet->getShipCount(ShipType::FIGHTER) * 1.0 +
               fleet->getShipCount(ShipType::CRUISER) * 3.0 +
               fleet->getShipCount(ShipType::BATTLESHIP) * 10.0;
    }
};

class BalancedCombatStrategy : public CombatStrategy {
public:
    std::string planAttack(Fleet* attacker, Fleet* defender) override {
        int attackerTotal = attacker->getTotalShipCount();
        int defenderTotal = defender->getTotalShipCount();
        
        double ratio = static_cast<double>(attackerTotal) / 
                      std::max(1, defenderTotal);
        
        if (ratio > 2.0) {
            return "SUPERIOR_NUMBERS";
        } else if (ratio > 1.2) {
            return "TACTICAL_STRIKE";
        } else if (ratio > 0.8) {
            return "EVEN_ENGAGEMENT";
        } else {
            return "STRATEGIC_WITHDRAWAL";
        }
    }
    
    std::string getStrategyName() const override {
        return "Balanced";
    }
};

class CombatPlanner {
private:
    std::unique_ptr<CombatStrategy> strategy_;
    
public:
    void setStrategy(std::unique_ptr<CombatStrategy> strategy) {
        strategy_ = std::move(strategy);
    }
    
    std::string planCombat(Fleet* attacker, Fleet* defender) {
        if (!strategy_) {
            return "NO_STRATEGY";
        }
        return strategy_->planAttack(attacker, defender);
    }
    
    std::string getCurrentStrategy() const {
        return strategy_ ? strategy_->getStrategyName() : "None";
    }
};

TEST_CASE_METHOD(StrategyTestFixture, "Combat Strategy System", "[strategy][combat][planning]") {
    
    SECTION("Aggressive combat strategy") {
        CombatPlanner planner;
        planner.setStrategy(std::make_unique<AggressiveStrategy>());
        
        Fleet* attacker = testFleets[0].get(); // Has fighters and cruisers
        Fleet* defender = testFleets[1].get(); // Has fewer ships
        
        std::string plan = planner.planCombat(attacker, defender);
        
        REQUIRE(planner.getCurrentStrategy() == "Aggressive");
        REQUIRE(!plan.empty());
        REQUIRE((plan == "FULL_ASSAULT" || plan == "NUMERICAL_ADVANTAGE" || plan == "DESPERATE_ATTACK"));
    }
    
    SECTION("Defensive combat strategy") {
        CombatPlanner planner;
        planner.setStrategy(std::make_unique<DefensiveStrategy>());
        
        Fleet* attacker = testFleets[2].get(); // Has battleships
        Fleet* defender = testFleets[3].get(); // Smaller fleet
        
        std::string plan = planner.planCombat(attacker, defender);
        
        REQUIRE(planner.getCurrentStrategy() == "Defensive");
        REQUIRE(!plan.empty());
        REQUIRE((plan == "OVERWHELMING_FORCE" || plan == "CAUTIOUS_ADVANCE" || plan == "AVOID_COMBAT"));
    }
    
    SECTION("Balanced combat strategy") {
        CombatPlanner planner;
        planner.setStrategy(std::make_unique<BalancedCombatStrategy>());
        
        Fleet* attacker = testFleets[0].get();
        Fleet* defender = testFleets[1].get();
        
        std::string plan = planner.planCombat(attacker, defender);
        
        REQUIRE(planner.getCurrentStrategy() == "Balanced");
        REQUIRE(!plan.empty());
        REQUIRE((plan == "SUPERIOR_NUMBERS" || plan == "TACTICAL_STRIKE" || 
                plan == "EVEN_ENGAGEMENT" || plan == "STRATEGIC_WITHDRAWAL"));
    }
    
    SECTION("Strategy comparison") {
        Fleet* strongAttacker = testFleets[2].get(); // With battleships
        Fleet* weakDefender = testFleets[3].get();   // Smaller fleet
        
        CombatPlanner aggressivePlanner;
        CombatPlanner defensivePlanner;
        CombatPlanner balancedPlanner;
        
        aggressivePlanner.setStrategy(std::make_unique<AggressiveStrategy>());
        defensivePlanner.setStrategy(std::make_unique<DefensiveStrategy>());
        balancedPlanner.setStrategy(std::make_unique<BalancedCombatStrategy>());
        
        std::string aggressivePlan = aggressivePlanner.planCombat(strongAttacker, weakDefender);
        std::string defensivePlan = defensivePlanner.planCombat(strongAttacker, weakDefender);
        std::string balancedPlan = balancedPlanner.planCombat(strongAttacker, weakDefender);
        
        // All strategies should provide some plan
        REQUIRE(!aggressivePlan.empty());
        REQUIRE(!defensivePlan.empty());
        REQUIRE(!balancedPlan.empty());
        
        // Strategies might produce different plans for the same situation
        INFO("Aggressive strategy: " << aggressivePlan);
        INFO("Defensive strategy: " << defensivePlan);
        INFO("Balanced strategy: " << balancedPlan);
    }
}

TEST_CASE_METHOD(StrategyTestFixture, "Advanced Strategy Applications", "[strategy][advanced][applications]") {
    
    SECTION("Function-based strategies") {
        // Using std::function as strategy
        using SortFunction = std::function<void(std::vector<int>&)>;
        
        class FunctionSortingContext {
        private:
            SortFunction strategy_;
            std::string strategyName_;
            
        public:
            void setStrategy(SortFunction strategy, const std::string& name) {
                strategy_ = strategy;
                strategyName_ = name;
            }
            
            void sort(std::vector<int>& data) {
                if (strategy_) {
                    strategy_(data);
                }
            }
            
            std::string getCurrentStrategyName() const {
                return strategyName_;
            }
        };
        
        FunctionSortingContext context;
        std::vector<int> testData = {64, 34, 25, 12, 22, 11, 90, 5};
        std::vector<int> expected = {5, 11, 12, 22, 25, 34, 64, 90};
        
        // Lambda strategy
        context.setStrategy([](std::vector<int>& data) {
            std::sort(data.begin(), data.end());
        }, "STL_Sort");
        
        std::vector<int> data = testData;
        context.sort(data);
        
        REQUIRE(data == expected);
        REQUIRE(context.getCurrentStrategyName() == "STL_Sort");
        
        // Reverse sort strategy
        context.setStrategy([](std::vector<int>& data) {
            std::sort(data.begin(), data.end(), std::greater<int>());
        }, "Reverse_Sort");
        
        data = testData;
        context.sort(data);
        
        std::vector<int> reverseExpected = {90, 64, 34, 25, 22, 12, 11, 5};
        REQUIRE(data == reverseExpected);
        REQUIRE(context.getCurrentStrategyName() == "Reverse_Sort");
    }
    
    SECTION("Template-based strategies") {
        template<typename Comparator>
        class ComparatorSortStrategy : public SortingStrategy<int> {
        private:
            Comparator comp_;
            std::string name_;
            
        public:
            ComparatorSortStrategy(Comparator comp, const std::string& name) 
                : comp_(comp), name_(name) {}
            
            void sort(std::vector<int>& data) override {
                std::sort(data.begin(), data.end(), comp_);
            }
            
            std::string getName() const override {
                return name_;
            }
        };
        
        SortingContext<int> context;
        std::vector<int> testData = {64, 34, 25, 12, 22, 11, 90, 5};
        
        // Ascending sort
        context.setStrategy(std::make_unique<ComparatorSortStrategy<std::less<int>>>(
            std::less<int>(), "Template_Ascending"));
        
        std::vector<int> data = testData;
        context.sort(data);
        
        std::vector<int> expected = {5, 11, 12, 22, 25, 34, 64, 90};
        REQUIRE(data == expected);
        
        // Descending sort
        context.setStrategy(std::make_unique<ComparatorSortStrategy<std::greater<int>>>(
            std::greater<int>(), "Template_Descending"));
        
        data = testData;
        context.sort(data);
        
        std::vector<int> reverseExpected = {90, 64, 34, 25, 22, 12, 11, 5};
        REQUIRE(data == reverseExpected);
    }
    
    SECTION("Strategy with configuration") {
        class ConfigurableStrategy : public FleetRoutingStrategy {
        private:
            double distanceWeight_;
            double valueWeight_;
            std::string name_;
            
        public:
            ConfigurableStrategy(double distWeight, double valWeight) 
                : distanceWeight_(distWeight), valueWeight_(valWeight) {
                name_ = "Configurable_" + std::to_string(distWeight) + "_" + std::to_string(valWeight);
            }
            
            std::vector<Planet*> planRoute(Fleet* fleet, const std::vector<Planet*>& planets) override {
                std::vector<std::pair<Planet*, double>> scored;
                Vector3D fleetPos = fleet->getPosition();
                
                for (Planet* planet : planets) {
                    double distance = calculateDistance(fleetPos, planet->getPosition());
                    double value = planet->getResourceAmount(ResourceType::MINERALS) + 
                                  planet->getResourceAmount(ResourceType::ENERGY);
                    
                    // Configurable scoring function
                    double score = (valueWeight_ * value) - (distanceWeight_ * distance);
                    scored.emplace_back(planet, score);
                }
                
                std::sort(scored.begin(), scored.end(), [](const auto& a, const auto& b) {
                    return a.second > b.second;
                });
                
                std::vector<Planet*> route;
                for (const auto& pair : scored) {
                    route.push_back(pair.first);
                }
                
                return route;
            }
            
            std::string getStrategyName() const override {
                return name_;
            }
            
        private:
            double calculateDistance(const Vector3D& a, const Vector3D& b) {
                double dx = a.x - b.x;
                double dy = a.y - b.y;
                double dz = a.z - b.z;
                return std::sqrt(dx*dx + dy*dy + dz*dz);
            }
        };
        
        FleetRouter router;
        Fleet* testFleet = testFleets[0].get();
        
        std::vector<Planet*> planets;
        for (size_t i = 0; i < 4; ++i) {
            planets.push_back(testPlanets[i].get());
        }
        
        // Value-focused configuration
        router.setStrategy(std::make_unique<ConfigurableStrategy>(0.1, 1.0));
        std::vector<Planet*> valueRoute = router.planRoute(testFleet, planets);
        
        // Distance-focused configuration
        router.setStrategy(std::make_unique<ConfigurableStrategy>(1.0, 0.1));
        std::vector<Planet*> distanceRoute = router.planRoute(testFleet, planets);
        
        // Routes should be different with different configurations
        REQUIRE(valueRoute.size() == planets.size());
        REQUIRE(distanceRoute.size() == planets.size());
        
        // In most cases, the routes should differ
        bool routesDiffer = false;
        for (size_t i = 0; i < valueRoute.size(); ++i) {
            if (valueRoute[i] != distanceRoute[i]) {
                routesDiffer = true;
                break;
            }
        }
        REQUIRE(routesDiffer);
    }
}

TEST_CASE_METHOD(StrategyTestFixture, "Strategy Performance Analysis", "[strategy][performance][analysis]") {
    
    SECTION("Sorting strategy performance comparison") {
        const int dataSize = 10000;
        std::vector<int> largeData;
        
        // Generate random data
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(1, 100000);
        
        for (int i = 0; i < dataSize; ++i) {
            largeData.push_back(dis(gen));
        }
        
        SortingContext<int> context;
        
        // Test QuickSort performance
        auto quickSortData = largeData;
        context.setStrategy(std::make_unique<QuickSortStrategy<int>>());
        
        auto start = std::chrono::high_resolution_clock::now();
        context.sort(quickSortData);
        auto quickSortTime = std::chrono::high_resolution_clock::now() - start;
        
        // Test MergeSort performance
        auto mergeSortData = largeData;
        context.setStrategy(std::make_unique<MergeSortStrategy<int>>());
        
        start = std::chrono::high_resolution_clock::now();
        context.sort(mergeSortData);
        auto mergeSortTime = std::chrono::high_resolution_clock::now() - start;
        
        // Verify both sorted correctly
        REQUIRE(std::is_sorted(quickSortData.begin(), quickSortData.end()));
        REQUIRE(std::is_sorted(mergeSortData.begin(), mergeSortData.end()));
        REQUIRE(quickSortData == mergeSortData); // Same result
        
        INFO("QuickSort time: " << std::chrono::duration_cast<std::chrono::microseconds>(quickSortTime).count() << "μs");
        INFO("MergeSort time: " << std::chrono::duration_cast<std::chrono::microseconds>(mergeSortTime).count() << "μs");
    }
    
    SECTION("Fleet routing strategy efficiency") {
        FleetRouter router;
        Fleet* testFleet = testFleets[0].get();
        
        // Use all planets for comprehensive test
        std::vector<Planet*> allPlanets;
        for (const auto& planet : testPlanets) {
            allPlanets.push_back(planet.get());
        }
        
        // Test each strategy's execution time
        std::vector<std::pair<std::string, std::chrono::microseconds>> results;
        
        // Nearest Planet Strategy
        router.setStrategy(std::make_unique<NearestPlanetStrategy>());
        auto start = std::chrono::high_resolution_clock::now();
        auto route1 = router.planRoute(testFleet, allPlanets);
        auto time1 = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now() - start);
        results.emplace_back("NearestPlanet", time1);
        
        // Highest Value Strategy
        router.setStrategy(std::make_unique<HighestValueStrategy>());
        start = std::chrono::high_resolution_clock::now();
        auto route2 = router.planRoute(testFleet, allPlanets);
        auto time2 = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now() - start);
        results.emplace_back("HighestValue", time2);
        
        // Balanced Strategy
        router.setStrategy(std::make_unique<BalancedStrategy>());
        start = std::chrono::high_resolution_clock::now();
        auto route3 = router.planRoute(testFleet, allPlanets);
        auto time3 = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now() - start);
        results.emplace_back("Balanced", time3);
        
        // All routes should have the same size
        REQUIRE(route1.size() == allPlanets.size());
        REQUIRE(route2.size() == allPlanets.size());
        REQUIRE(route3.size() == allPlanets.size());
        
        // Print performance results
        for (const auto& result : results) {
            INFO(result.first << " strategy: " << result.second.count() << "μs");
        }
    }
}

TEST_CASE_METHOD(StrategyTestFixture, "Strategy Pattern Best Practices", "[strategy][best-practices][design]") {
    
    SECTION("Strategy validation and error handling") {
        class ValidatingRouter : public FleetRouter {
        public:
            std::vector<Planet*> planRoute(Fleet* fleet, const std::vector<Planet*>& planets) override {
                if (!fleet) {
                    throw std::invalid_argument("Fleet cannot be null");
                }
                if (planets.empty()) {
                    return {}; // Empty route for empty planet list
                }
                
                return FleetRouter::planRoute(fleet, planets);
            }
        };
        
        ValidatingRouter router;
        router.setStrategy(std::make_unique<NearestPlanetStrategy>());
        
        // Test null fleet
        std::vector<Planet*> planets = {testPlanets[0].get()};
        REQUIRE_THROWS_AS(router.planRoute(nullptr, planets), std::invalid_argument);
        
        // Test empty planet list
        std::vector<Planet*> emptyPlanets;
        auto emptyRoute = router.planRoute(testFleets[0].get(), emptyPlanets);
        REQUIRE(emptyRoute.empty());
        
        // Test valid input
        auto validRoute = router.planRoute(testFleets[0].get(), planets);
        REQUIRE(validRoute.size() == 1);
    }
    
    SECTION("Strategy chain of responsibility") {
        class ChainedStrategy : public FleetRoutingStrategy {
        private:
            std::vector<std::unique_ptr<FleetRoutingStrategy>> strategies_;
            std::string name_;
            
        public:
            ChainedStrategy(const std::string& name) : name_(name) {}
            
            void addStrategy(std::unique_ptr<FleetRoutingStrategy> strategy) {
                strategies_.push_back(std::move(strategy));
            }
            
            std::vector<Planet*> planRoute(Fleet* fleet, const std::vector<Planet*>& planets) override {
                if (strategies_.empty()) {
                    return planets; // Default behavior
                }
                
                // Apply first strategy
                auto result = strategies_[0]->planRoute(fleet, planets);
                
                // If we have multiple strategies, we could combine their results
                // For this example, we'll just use the first one
                return result;
            }
            
            std::string getStrategyName() const override {
                return name_;
            }
        };
        
        auto chainedStrategy = std::make_unique<ChainedStrategy>("Chained");
        chainedStrategy->addStrategy(std::make_unique<NearestPlanetStrategy>());
        chainedStrategy->addStrategy(std::make_unique<HighestValueStrategy>());
        
        FleetRouter router;
        router.setStrategy(std::move(chainedStrategy));
        
        std::vector<Planet*> planets = {testPlanets[0].get(), testPlanets[1].get()};
        auto route = router.planRoute(testFleets[0].get(), planets);
        
        REQUIRE(route.size() == 2);
        REQUIRE(router.getCurrentStrategy() == "Chained");
    }
    
    SECTION("Strategy factory pattern integration") {
        enum class StrategyType {
            NEAREST,
            HIGHEST_VALUE,
            BALANCED
        };
        
        class StrategyFactory {
        public:
            static std::unique_ptr<FleetRoutingStrategy> createStrategy(StrategyType type) {
                switch (type) {
                    case StrategyType::NEAREST:
                        return std::make_unique<NearestPlanetStrategy>();
                    case StrategyType::HIGHEST_VALUE:
                        return std::make_unique<HighestValueStrategy>();
                    case StrategyType::BALANCED:
                        return std::make_unique<BalancedStrategy>();
                    default:
                        return nullptr;
                }
            }
        };
        
        FleetRouter router;
        
        // Test factory creation with different types
        for (auto type : {StrategyType::NEAREST, StrategyType::HIGHEST_VALUE, StrategyType::BALANCED}) {
            auto strategy = StrategyFactory::createStrategy(type);
            REQUIRE(strategy != nullptr);
            
            router.setStrategy(std::move(strategy));
            
            std::vector<Planet*> planets = {testPlanets[0].get(), testPlanets[1].get()};
            auto route = router.planRoute(testFleets[0].get(), planets);
            
            REQUIRE(route.size() == 2);
        }
    }
}