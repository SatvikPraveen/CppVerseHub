// File: tests/unit_tests/pattern_tests/DecoratorTests.cpp
// Decorator pattern tests for CppVerseHub design patterns showcase

#include <catch2/catch.hpp>
#include <memory>
#include <vector>
#include <string>
#include <sstream>

// Include the decorator pattern headers
#include "Decorator.hpp"
#include "Ship.hpp"
#include "Fleet.hpp"
#include "Weapon.hpp"
#include "Shield.hpp"

using namespace CppVerseHub::Patterns;
using namespace CppVerseHub::Core;

/**
 * @brief Test fixture for decorator pattern tests
 */
class DecoratorTestFixture {
public:
    DecoratorTestFixture() {
        setupTestData();
    }
    
    ~DecoratorTestFixture() {
        testShips.clear();
        testFleets.clear();
    }
    
protected:
    void setupTestData() {
        // Create test ships
        for (int i = 0; i < 6; ++i) {
            testShips.push_back(std::make_unique<Ship>(
                "DecoratorShip_" + std::to_string(i),
                static_cast<ShipType>(i % 3), // Cycle through ship types
                Vector3D{i * 50.0, i * 50.0, i * 50.0}
            ));
        }
        
        // Create test fleets
        for (int i = 0; i < 2; ++i) {
            auto fleet = std::make_unique<Fleet>(
                "DecoratorFleet_" + std::to_string(i),
                Vector3D{i * 200.0, i * 200.0, i * 200.0}
            );
            fleet->addShips(ShipType::FIGHTER, 5 + i * 2);
            fleet->addShips(ShipType::CRUISER, 2 + i);
            testFleets.push_back(std::move(fleet));
        }
    }
    
    std::vector<std::unique_ptr<Ship>> testShips;
    std::vector<std::unique_ptr<Fleet>> testFleets;
};

/**
 * @brief Base component interface for testing
 */
class Beverage {
public:
    virtual ~Beverage() = default;
    virtual std::string getDescription() const = 0;
    virtual double getCost() const = 0;
    virtual std::string getSize() const { return "Medium"; }
};

/**
 * @brief Concrete component implementations
 */
class Espresso : public Beverage {
public:
    std::string getDescription() const override {
        return "Espresso";
    }
    
    double getCost() const override {
        return 1.99;
    }
};

class HouseBlend : public Beverage {
public:
    std::string getDescription() const override {
        return "House Blend Coffee";
    }
    
    double getCost() const override {
        return 0.89;
    }
};

class DarkRoast : public Beverage {
public:
    std::string getDescription() const override {
        return "Dark Roast Coffee";
    }
    
    double getCost() const override {
        return 0.99;
    }
};

/**
 * @brief Base decorator class
 */
class CondimentDecorator : public Beverage {
protected:
    std::unique_ptr<Beverage> beverage_;
    
public:
    explicit CondimentDecorator(std::unique_ptr<Beverage> beverage)
        : beverage_(std::move(beverage)) {}
    
    std::string getSize() const override {
        return beverage_ ? beverage_->getSize() : "Unknown";
    }
};

/**
 * @brief Concrete decorators
 */
class Mocha : public CondimentDecorator {
public:
    explicit Mocha(std::unique_ptr<Beverage> beverage)
        : CondimentDecorator(std::move(beverage)) {}
    
    std::string getDescription() const override {
        return beverage_ ? beverage_->getDescription() + ", Mocha" : "Mocha";
    }
    
    double getCost() const override {
        return (beverage_ ? beverage_->getCost() : 0.0) + 0.20;
    }
};

class Whip : public CondimentDecorator {
public:
    explicit Whip(std::unique_ptr<Beverage> beverage)
        : CondimentDecorator(std::move(beverage)) {}
    
    std::string getDescription() const override {
        return beverage_ ? beverage_->getDescription() + ", Whip" : "Whip";
    }
    
    double getCost() const override {
        return (beverage_ ? beverage_->getCost() : 0.0) + 0.10;
    }
};

class Soy : public CondimentDecorator {
public:
    explicit Soy(std::unique_ptr<Beverage> beverage)
        : CondimentDecorator(std::move(beverage)) {}
    
    std::string getDescription() const override {
        return beverage_ ? beverage_->getDescription() + ", Soy" : "Soy";
    }
    
    double getCost() const override {
        return (beverage_ ? beverage_->getCost() : 0.0) + 0.15;
    }
};

class SteamedMilk : public CondimentDecorator {
public:
    explicit SteamedMilk(std::unique_ptr<Beverage> beverage)
        : CondimentDecorator(std::move(beverage)) {}
    
    std::string getDescription() const override {
        return beverage_ ? beverage_->getDescription() + ", Steamed Milk" : "Steamed Milk";
    }
    
    double getCost() const override {
        return (beverage_ ? beverage_->getCost() : 0.0) + 0.10;
    }
};

/**
 * @brief Size decorator to demonstrate different types of decorators
 */
class SizeDecorator : public CondimentDecorator {
private:
    std::string size_;
    double sizeMultiplier_;
    
public:
    SizeDecorator(std::unique_ptr<Beverage> beverage, const std::string& size, double multiplier)
        : CondimentDecorator(std::move(beverage)), size_(size), sizeMultiplier_(multiplier) {}
    
    std::string getDescription() const override {
        return beverage_ ? beverage_->getDescription() : "Unknown";
    }
    
    double getCost() const override {
        return (beverage_ ? beverage_->getCost() : 0.0) * sizeMultiplier_;
    }
    
    std::string getSize() const override {
        return size_;
    }
};

TEST_CASE_METHOD(DecoratorTestFixture, "Basic Decorator Pattern", "[decorator][patterns][basic]") {
    
    SECTION("Single decorator") {
        auto espresso = std::make_unique<Espresso>();
        double baseCost = espresso->getCost();
        std::string baseDescription = espresso->getDescription();
        
        auto mochaEspresso = std::make_unique<Mocha>(std::move(espresso));
        
        REQUIRE(mochaEspresso->getDescription() == baseDescription + ", Mocha");
        REQUIRE(mochaEspresso->getCost() == Approx(baseCost + 0.20));
    }
    
    SECTION("Multiple decorators") {
        auto houseBlend = std::make_unique<HouseBlend>();
        double baseCost = houseBlend->getCost();
        
        // Chain multiple decorators
        auto decoratedBeverage = std::make_unique<Whip>(
            std::make_unique<Mocha>(
                std::make_unique<Soy>(
                    std::move(houseBlend)
                )
            )
        );
        
        REQUIRE(decoratedBeverage->getDescription() == "House Blend Coffee, Soy, Mocha, Whip");
        REQUIRE(decoratedBeverage->getCost() == Approx(baseCost + 0.15 + 0.20 + 0.10));
    }
    
    SECTION("Different base components with same decorators") {
        // Espresso with Mocha
        auto espressoWithMocha = std::make_unique<Mocha>(std::make_unique<Espresso>());
        
        // Dark Roast with Mocha
        auto darkRoastWithMocha = std::make_unique<Mocha>(std::make_unique<DarkRoast>());
        
        REQUIRE(espressoWithMocha->getDescription() == "Espresso, Mocha");
        REQUIRE(darkRoastWithMocha->getDescription() == "Dark Roast Coffee, Mocha");
        
        // Cost should be base + decorator cost
        REQUIRE(espressoWithMocha->getCost() == Approx(1.99 + 0.20));
        REQUIRE(darkRoastWithMocha->getCost() == Approx(0.99 + 0.20));
    }
    
    SECTION("Order of decorators matters") {
        auto beverage1 = std::make_unique<Whip>(
            std::make_unique<Mocha>(
                std::make_unique<Espresso>()
            )
        );
        
        auto beverage2 = std::make_unique<Mocha>(
            std::make_unique<Whip>(
                std::make_unique<Espresso>()
            )
        );
        
        // Same cost regardless of order
        REQUIRE(beverage1->getCost() == Approx(beverage2->getCost()));
        
        // But different descriptions due to order
        REQUIRE(beverage1->getDescription() == "Espresso, Mocha, Whip");
        REQUIRE(beverage2->getDescription() == "Espresso, Whip, Mocha");
    }
}

TEST_CASE_METHOD(DecoratorTestFixture, "Advanced Decorator Features", "[decorator][advanced][features]") {
    
    SECTION("Size decorator affecting cost calculation") {
        auto espresso = std::make_unique<Espresso>();
        double baseCost = espresso->getCost();
        
        REQUIRE(smallEspresso->getCost() == Approx(baseCost * 0.8));
        REQUIRE(smallEspresso->getSize() == "Small");
        REQUIRE(smallEspresso->getDescription() == "Espresso");
        
        // Large size with decorators
        auto largeMochaWhip = std::make_unique<SizeDecorator>(
            std::make_unique<Whip>(
                std::make_unique<Mocha>(
                    std::make_unique<DarkRoast>()
                )
            ), "Large", 1.5);
        
        double expectedCost = (0.99 + 0.20 + 0.10) * 1.5; // Base + Mocha + Whip, then size multiplier
        REQUIRE(largeMochaWhip->getCost() == Approx(expectedCost));
        REQUIRE(largeMochaWhip->getSize() == "Large");
    }
    
    SECTION("Decorator with state") {
        class CountingDecorator : public CondimentDecorator {
        private:
            static int instanceCount_;
            int instanceId_;
            
        public:
            explicit CountingDecorator(std::unique_ptr<Beverage> beverage)
                : CondimentDecorator(std::move(beverage)), instanceId_(++instanceCount_) {}
            
            std::string getDescription() const override {
                return beverage_ ? beverage_->getDescription() + ", Count#" + std::to_string(instanceId_)
                                : "Count#" + std::to_string(instanceId_);
            }
            
            double getCost() const override {
                return (beverage_ ? beverage_->getCost() : 0.0) + 0.05;
            }
            
            int getInstanceId() const { return instanceId_; }
            static int getTotalInstances() { return instanceCount_; }
        };
        
        int CountingDecorator::instanceCount_ = 0;
        
        auto beverage1 = std::make_unique<CountingDecorator>(std::make_unique<Espresso>());
        auto beverage2 = std::make_unique<CountingDecorator>(std::make_unique<HouseBlend>());
        
        REQUIRE(beverage1->getDescription() == "Espresso, Count#1");
        REQUIRE(beverage2->getDescription() == "House Blend Coffee, Count#2");
        REQUIRE(CountingDecorator::getTotalInstances() == 2);
    }
    
    SECTION("Conditional decorator behavior") {
        class ConditionalDecorator : public CondimentDecorator {
        private:
            bool isActive_;
            std::string conditionName_;
            double conditionCost_;
            
        public:
            ConditionalDecorator(std::unique_ptr<Beverage> beverage, 
                               const std::string& name, double cost, bool active = true)
                : CondimentDecorator(std::move(beverage)), 
                  isActive_(active), conditionName_(name), conditionCost_(cost) {}
            
            std::string getDescription() const override {
                if (!isActive_) {
                    return beverage_ ? beverage_->getDescription() : "";
                }
                return beverage_ ? beverage_->getDescription() + ", " + conditionName_ 
                                : conditionName_;
            }
            
            double getCost() const override {
                double baseCost = beverage_ ? beverage_->getCost() : 0.0;
                return isActive_ ? baseCost + conditionCost_ : baseCost;
            }
            
            void setActive(bool active) { isActive_ = active; }
            bool isActive() const { return isActive_; }
        };
        
        auto conditionalBeverage = std::make_unique<ConditionalDecorator>(
            std::make_unique<Espresso>(), "Special Syrup", 0.25, false);
        
        // Initially inactive
        REQUIRE(conditionalBeverage->getDescription() == "Espresso");
        REQUIRE(conditionalBeverage->getCost() == Approx(1.99));
        
        // Activate condition
        conditionalBeverage->setActive(true);
        REQUIRE(conditionalBeverage->getDescription() == "Espresso, Special Syrup");
        REQUIRE(conditionalBeverage->getCost() == Approx(1.99 + 0.25));
    }
}

TEST_CASE_METHOD(DecoratorTestFixture, "Game-Specific Decorator Applications", "[decorator][game][ships]") {
    
    SECTION("Ship enhancement decorators") {
        class ShipComponent {
        public:
            virtual ~ShipComponent() = default;
            virtual std::string getConfiguration() const = 0;
            virtual double getFirepower() const = 0;
            virtual double getDefense() const = 0;
            virtual double getMaintenance() const = 0;
        };
        
        class BasicShip : public ShipComponent {
        private:
            ShipType type_;
            
        public:
            explicit BasicShip(ShipType type) : type_(type) {}
            
            std::string getConfiguration() const override {
                switch (type_) {
                    case ShipType::FIGHTER: return "Basic Fighter";
                    case ShipType::CRUISER: return "Basic Cruiser";
                    case ShipType::BATTLESHIP: return "Basic Battleship";
                    default: return "Unknown Ship";
                }
            }
            
            double getFirepower() const override {
                switch (type_) {
                    case ShipType::FIGHTER: return 10.0;
                    case ShipType::CRUISER: return 25.0;
                    case ShipType::BATTLESHIP: return 50.0;
                    default: return 0.0;
                }
            }
            
            double getDefense() const override {
                switch (type_) {
                    case ShipType::FIGHTER: return 5.0;
                    case ShipType::CRUISER: return 15.0;
                    case ShipType::BATTLESHIP: return 30.0;
                    default: return 0.0;
                }
            }
            
            double getMaintenance() const override {
                switch (type_) {
                    case ShipType::FIGHTER: return 2.0;
                    case ShipType::CRUISER: return 5.0;
                    case ShipType::BATTLESHIP: return 10.0;
                    default: return 0.0;
                }
            }
        };
        
        class ShipDecorator : public ShipComponent {
        protected:
            std::unique_ptr<ShipComponent> ship_;
            
        public:
            explicit ShipDecorator(std::unique_ptr<ShipComponent> ship)
                : ship_(std::move(ship)) {}
        };
        
        class WeaponUpgrade : public ShipDecorator {
        private:
            std::string weaponType_;
            double firePowerBonus_;
            double maintenanceIncrease_;
            
        public:
            WeaponUpgrade(std::unique_ptr<ShipComponent> ship, 
                         const std::string& weapon, double firepower, double maintenance)
                : ShipDecorator(std::move(ship)), weaponType_(weapon), 
                  firePowerBonus_(firepower), maintenanceIncrease_(maintenance) {}
            
            std::string getConfiguration() const override {
                return ship_ ? ship_->getConfiguration() + " + " + weaponType_ : weaponType_;
            }
            
            double getFirepower() const override {
                return (ship_ ? ship_->getFirepower() : 0.0) + firePowerBonus_;
            }
            
            double getDefense() const override {
                return ship_ ? ship_->getDefense() : 0.0;
            }
            
            double getMaintenance() const override {
                return (ship_ ? ship_->getMaintenance() : 0.0) + maintenanceIncrease_;
            }
        };
        
        class ShieldUpgrade : public ShipDecorator {
        private:
            std::string shieldType_;
            double defenseBonus_;
            double maintenanceIncrease_;
            
        public:
            ShieldUpgrade(std::unique_ptr<ShipComponent> ship,
                         const std::string& shield, double defense, double maintenance)
                : ShipDecorator(std::move(ship)), shieldType_(shield),
                  defenseBonus_(defense), maintenanceIncrease_(maintenance) {}
            
            std::string getConfiguration() const override {
                return ship_ ? ship_->getConfiguration() + " + " + shieldType_ : shieldType_;
            }
            
            double getFirepower() const override {
                return ship_ ? ship_->getFirepower() : 0.0;
            }
            
            double getDefense() const override {
                return (ship_ ? ship_->getDefense() : 0.0) + defenseBonus_;
            }
            
            double getMaintenance() const override {
                return (ship_ ? ship_->getMaintenance() : 0.0) + maintenanceIncrease_;
            }
        };
        
        // Test basic ship
        auto basicFighter = std::make_unique<BasicShip>(ShipType::FIGHTER);
        REQUIRE(basicFighter->getConfiguration() == "Basic Fighter");
        REQUIRE(basicFighter->getFirepower() == 10.0);
        REQUIRE(basicFighter->getDefense() == 5.0);
        REQUIRE(basicFighter->getMaintenance() == 2.0);
        
        // Add weapon upgrade
        auto armedFighter = std::make_unique<WeaponUpgrade>(
            std::move(basicFighter), "Laser Cannons", 15.0, 1.5);
        
        REQUIRE(armedFighter->getConfiguration() == "Basic Fighter + Laser Cannons");
        REQUIRE(armedFighter->getFirepower() == 25.0); // 10 + 15
        REQUIRE(armedFighter->getDefense() == 5.0);    // Unchanged
        REQUIRE(armedFighter->getMaintenance() == 3.5); // 2 + 1.5
        
        // Add shield upgrade
        auto fullyUpgraded = std::make_unique<ShieldUpgrade>(
            std::move(armedFighter), "Energy Shield", 8.0, 1.0);
        
        REQUIRE(fullyUpgraded->getConfiguration() == "Basic Fighter + Laser Cannons + Energy Shield");
        REQUIRE(fullyUpgraded->getFirepower() == 25.0);  // Unchanged
        REQUIRE(fullyUpgraded->getDefense() == 13.0);    // 5 + 8
        REQUIRE(fullyUpgraded->getMaintenance() == 4.5); // 3.5 + 1.0
    }
    
    SECTION("Fleet composition decorators") {
        class FleetComponent {
        public:
            virtual ~FleetComponent() = default;
            virtual std::string getComposition() const = 0;
            virtual int getTotalShips() const = 0;
            virtual double getTotalFirepower() const = 0;
            virtual double getMovementSpeed() const = 0;
        };
        
        class BasicFleet : public FleetComponent {
        private:
            Fleet* fleet_;
            
        public:
            explicit BasicFleet(Fleet* fleet) : fleet_(fleet) {}
            
            std::string getComposition() const override {
                return fleet_ ? "Fleet: " + fleet_->getName() : "Empty Fleet";
            }
            
            int getTotalShips() const override {
                return fleet_ ? fleet_->getTotalShipCount() : 0;
            }
            
            double getTotalFirepower() const override {
                if (!fleet_) return 0.0;
                return fleet_->getShipCount(ShipType::FIGHTER) * 10.0 +
                       fleet_->getShipCount(ShipType::CRUISER) * 25.0 +
                       fleet_->getShipCount(ShipType::BATTLESHIP) * 50.0;
            }
            
            double getMovementSpeed() const override {
                return 1.0; // Base speed
            }
        };
        
        class FleetDecorator : public FleetComponent {
        protected:
            std::unique_ptr<FleetComponent> fleet_;
            
        public:
            explicit FleetDecorator(std::unique_ptr<FleetComponent> fleet)
                : fleet_(std::move(fleet)) {}
        };
        
        class FormationDecorator : public FleetDecorator {
        private:
            std::string formation_;
            double firepowerMultiplier_;
            double speedModifier_;
            
        public:
            FormationDecorator(std::unique_ptr<FleetComponent> fleet,
                             const std::string& formation, double firepower, double speed)
                : FleetDecorator(std::move(fleet)), formation_(formation),
                  firepowerMultiplier_(firepower), speedModifier_(speed) {}
            
            std::string getComposition() const override {
                return fleet_ ? fleet_->getComposition() + " [" + formation_ + "]" : "[" + formation_ + "]";
            }
            
            int getTotalShips() const override {
                return fleet_ ? fleet_->getTotalShips() : 0;
            }
            
            double getTotalFirepower() const override {
                return (fleet_ ? fleet_->getTotalFirepower() : 0.0) * firepowerMultiplier_;
            }
            
            double getMovementSpeed() const override {
                return (fleet_ ? fleet_->getMovementSpeed() : 0.0) + speedModifier_;
            }
        };
        
        auto basicFleetComponent = std::make_unique<BasicFleet>(testFleets[0].get());
        double baseFirepower = basicFleetComponent->getTotalFirepower();
        double baseSpeed = basicFleetComponent->getMovementSpeed();
        
        // Apply attack formation
        auto attackFormation = std::make_unique<FormationDecorator>(
            std::move(basicFleetComponent), "Attack Formation", 1.2, -0.1);
        
        REQUIRE(attackFormation->getComposition().find("Attack Formation") != std::string::npos);
        REQUIRE(attackFormation->getTotalFirepower() == Approx(baseFirepower * 1.2));
        REQUIRE(attackFormation->getMovementSpeed() == Approx(baseSpeed - 0.1));
        
        // Apply defensive formation on top
        auto defensiveFormation = std::make_unique<FormationDecorator>(
            std::move(attackFormation), "Defensive Screen", 0.9, 0.05);
        
        REQUIRE(defensiveFormation->getTotalFirepower() == Approx(baseFirepower * 1.2 * 0.9));
        REQUIRE(defensiveFormation->getMovementSpeed() == Approx(baseSpeed - 0.1 + 0.05));
    }
}

TEST_CASE_METHOD(DecoratorTestFixture, "Decorator Pattern Performance", "[decorator][performance][analysis]") {
    
    SECTION("Deep decorator chain performance") {
        const int chainDepth = 1000;
        
        auto beverage = std::make_unique<Espresso>();
        auto start = std::chrono::high_resolution_clock::now();
        
        // Build deep chain
        for (int i = 0; i < chainDepth; ++i) {
            if (i % 4 == 0) {
                beverage = std::make_unique<Mocha>(std::move(beverage));
            } else if (i % 4 == 1) {
                beverage = std::make_unique<Whip>(std::move(beverage));
            } else if (i % 4 == 2) {
                beverage = std::make_unique<Soy>(std::move(beverage));
            } else {
                beverage = std::make_unique<SteamedMilk>(std::move(beverage));
            }
        }
        
        auto buildTime = std::chrono::high_resolution_clock::now() - start;
        
        // Test method calls on deep chain
        start = std::chrono::high_resolution_clock::now();
        
        std::string description = beverage->getDescription();
        double cost = beverage->getCost();
        
        auto accessTime = std::chrono::high_resolution_clock::now() - start;
        
        REQUIRE(!description.empty());
        REQUIRE(cost > 0.0);
        
        INFO("Build time for " << chainDepth << " decorators: " << 
             std::chrono::duration_cast<std::chrono::microseconds>(buildTime).count() << "μs");
        INFO("Access time for deep chain: " << 
             std::chrono::duration_cast<std::chrono::microseconds>(accessTime).count() << "μs");
        
        // Should complete in reasonable time
        REQUIRE(std::chrono::duration_cast<std::chrono::milliseconds>(buildTime).count() < 100);
        REQUIRE(std::chrono::duration_cast<std::chrono::microseconds>(accessTime).count() < 10000);
    }
    
    SECTION("Memory usage with many decorators") {
        const int decoratorCount = 10000;
        std::vector<std::unique_ptr<Beverage>> beverages;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // Create many decorated beverages
        for (int i = 0; i < decoratorCount; ++i) {
            auto beverage = std::make_unique<HouseBlend>();
            beverage = std::make_unique<Mocha>(std::move(beverage));
            beverage = std::make_unique<Whip>(std::move(beverage));
            beverages.push_back(std::move(beverage));
        }
        
        auto creationTime = std::chrono::high_resolution_clock::now() - start;
        
        // Access all beverages
        start = std::chrono::high_resolution_clock::now();
        double totalCost = 0.0;
        
        for (const auto& beverage : beverages) {
            totalCost += beverage->getCost();
        }
        
        auto accessTime = std::chrono::high_resolution_clock::now() - start;
        
        REQUIRE(beverages.size() == decoratorCount);
        REQUIRE(totalCost > 0.0);
        
        INFO("Creation time for " << decoratorCount << " decorated beverages: " << 
             std::chrono::duration_cast<std::chrono::milliseconds>(creationTime).count() << "ms");
        INFO("Access time for " << decoratorCount << " beverages: " << 
             std::chrono::duration_cast<std::chrono::microseconds>(accessTime).count() << "μs");
    }
}

TEST_CASE_METHOD(DecoratorTestFixture, "Decorator Edge Cases and Robustness", "[decorator][edge-cases][robustness]") {
    
    SECTION("Null component handling") {
        class SafeDecorator : public CondimentDecorator {
        public:
            explicit SafeDecorator(std::unique_ptr<Beverage> beverage)
                : CondimentDecorator(std::move(beverage)) {}
            
            std::string getDescription() const override {
                return beverage_ ? beverage_->getDescription() + ", Safe Additive" : "Safe Additive Only";
            }
            
            double getCost() const override {
                return (beverage_ ? beverage_->getCost() : 0.0) + 0.30;
            }
        };
        
        // Test with null component
        auto safeDecorator = std::make_unique<SafeDecorator>(nullptr);
        REQUIRE(safeDecorator->getDescription() == "Safe Additive Only");
        REQUIRE(safeDecorator->getCost() == Approx(0.30));
    }
    
    SECTION("Decorator removal simulation") {
        class RemovableDecorator : public CondimentDecorator {
        private:
            bool isRemoved_;
            std::string additiveName_;
            double additiveCost_;
            
        public:
            RemovableDecorator(std::unique_ptr<Beverage> beverage, 
                             const std::string& name, double cost)
                : CondimentDecorator(std::move(beverage)), 
                  isRemoved_(false), additiveName_(name), additiveCost_(cost) {}
            
            std::string getDescription() const override {
                if (isRemoved_) {
                    return beverage_ ? beverage_->getDescription() : "";
                }
                return beverage_ ? beverage_->getDescription() + ", " + additiveName_ : additiveName_;
            }
            
            double getCost() const override {
                double baseCost = beverage_ ? beverage_->getCost() : 0.0;
                return isRemoved_ ? baseCost : baseCost + additiveCost_;
            }
            
            void remove() { isRemoved_ = true; }
            void restore() { isRemoved_ = false; }
            bool isRemoved() const { return isRemoved_; }
        };
        
        auto removableDecorator = std::make_unique<RemovableDecorator>(
            std::make_unique<Espresso>(), "Temporary Syrup", 0.40);
        
        // Initially active
        REQUIRE(removableDecorator->getDescription() == "Espresso, Temporary Syrup");
        REQUIRE(removableDecorator->getCost() == Approx(1.99 + 0.40));
        
        // Remove decorator effect
        removableDecorator->remove();
        REQUIRE(removableDecorator->getDescription() == "Espresso");
        REQUIRE(removableDecorator->getCost() == Approx(1.99));
        
        // Restore decorator effect
        removableDecorator->restore();
        REQUIRE(removableDecorator->getDescription() == "Espresso, Temporary Syrup");
        REQUIRE(removableDecorator->getCost() == Approx(1.99 + 0.40));
    }
    
    SECTION("Recursive decorator detection") {
        class CountingDecorator : public CondimentDecorator {
        private:
            mutable int callCount_;
            int maxCalls_;
            
        public:
            CountingDecorator(std::unique_ptr<Beverage> beverage, int maxCalls = 100)
                : CondimentDecorator(std::move(beverage)), callCount_(0), maxCalls_(maxCalls) {}
            
            std::string getDescription() const override {
                if (++callCount_ > maxCalls_) {
                    return "ERROR: Too many recursive calls";
                }
                
                std::string result = beverage_ ? beverage_->getDescription() + ", Counted" : "Counted";
                --callCount_;
                return result;
            }
            
            double getCost() const override {
                if (++callCount_ > maxCalls_) {
                    --callCount_;
                    return 999.99; // Error cost
                }
                
                double result = (beverage_ ? beverage_->getCost() : 0.0) + 0.05;
                --callCount_;
                return result;
            }
            
            int getCallCount() const { return callCount_; }
        };
        
        auto countingDecorator = std::make_unique<CountingDecorator>(
            std::make_unique<Espresso>(), 5);
        
        // Normal calls should work
        REQUIRE(countingDecorator->getDescription() == "Espresso, Counted");
        REQUIRE(countingDecorator->getCost() == Approx(1.99 + 0.05));
        
        // The call count should be 0 after normal operation
        REQUIRE(countingDecorator->getCallCount() == 0);
    }
}

TEST_CASE_METHOD(DecoratorTestFixture, "Decorator Best Practices and Design", "[decorator][best-practices][design]") {
    
    SECTION("Decorator with factory") {
        enum class CondimentType {
            MOCHA,
            WHIP,
            SOY,
            STEAMED_MILK
        };
        
        class CondimentFactory {
        public:
            static std::unique_ptr<Beverage> addCondiment(std::unique_ptr<Beverage> beverage, 
                                                         CondimentType type) {
                switch (type) {
                    case CondimentType::MOCHA:
                        return std::make_unique<Mocha>(std::move(beverage));
                    case CondimentType::WHIP:
                        return std::make_unique<Whip>(std::move(beverage));
                    case CondimentType::SOY:
                        return std::make_unique<Soy>(std::move(beverage));
                    case CondimentType::STEAMED_MILK:
                        return std::make_unique<SteamedMilk>(std::move(beverage));
                    default:
                        return beverage;
                }
            }
        };
        
        auto beverage = std::make_unique<DarkRoast>();
        
        beverage = CondimentFactory::addCondiment(std::move(beverage), CondimentType::MOCHA);
        beverage = CondimentFactory::addCondiment(std::move(beverage), CondimentType::WHIP);
        
        REQUIRE(beverage->getDescription() == "Dark Roast Coffee, Mocha, Whip");
        REQUIRE(beverage->getCost() == Approx(0.99 + 0.20 + 0.10));
    }
    
    SECTION("Decorator chain builder") {
        class BeverageBuilder {
        private:
            std::unique_ptr<Beverage> beverage_;
            
        public:
            explicit BeverageBuilder(std::unique_ptr<Beverage> base)
                : beverage_(std::move(base)) {}
            
            BeverageBuilder& addMocha() {
                beverage_ = std::make_unique<Mocha>(std::move(beverage_));
                return *this;
            }
            
            BeverageBuilder& addWhip() {
                beverage_ = std::make_unique<Whip>(std::move(beverage_));
                return *this;
            }
            
            BeverageBuilder& addSoy() {
                beverage_ = std::make_unique<Soy>(std::move(beverage_));
                return *this;
            }
            
            BeverageBuilder& addSteamedMilk() {
                beverage_ = std::make_unique<SteamedMilk>(std::move(beverage_));
                return *this;
            }
            
            BeverageBuilder& setSize(const std::string& size, double multiplier) {
                beverage_ = std::make_unique<SizeDecorator>(std::move(beverage_), size, multiplier);
                return *this;
            }
            
            std::unique_ptr<Beverage> build() {
                return std::move(beverage_);
            }
        };
        
        auto complexBeverage = BeverageBuilder(std::make_unique<Espresso>())
            .addMocha()
            .addWhip()
            .addSoy()
            .setSize("Large", 1.3)
            .build();
        
        REQUIRE(complexBeverage->getSize() == "Large");
        REQUIRE(complexBeverage->getDescription() == "Espresso, Mocha, Whip, Soy");
        
        double expectedCost = (1.99 + 0.20 + 0.10 + 0.15) * 1.3;
        REQUIRE(complexBeverage->getCost() == Approx(expectedCost));
    }
    
    SECTION("Decorator with visitor pattern") {
        class BeverageVisitor {
        public:
            virtual ~BeverageVisitor() = default;
            virtual void visitEspresso(const Espresso& espresso) = 0;
            virtual void visitMocha(const Mocha& mocha) = 0;
            virtual void visitWhip(const Whip& whip) = 0;
        };
        
        class VisitableBeverage : public Beverage {
        public:
            virtual void accept(BeverageVisitor& visitor) = 0;
        };
        
        class AnalysisVisitor : public BeverageVisitor {
        private:
            int mochaCount_ = 0;
            int whipCount_ = 0;
            int espressoCount_ = 0;
            
        public:
            void visitEspresso(const Espresso& espresso) override {
                espressoCount_++;
            }
            
            void visitMocha(const Mocha& mocha) override {
                mochaCount_++;
            }
            
            void visitWhip(const Whip& whip) override {
                whipCount_++;
            }
            
            int getMochaCount() const { return mochaCount_; }
            int getWhipCount() const { return whipCount_; }
            int getEspressoCount() const { return espressoCount_; }
        };
        
        // This demonstrates how decorator can be combined with other patterns
        // In practice, this would require extending the existing classes
        // Here we just verify the concept works
        
        AnalysisVisitor visitor;
        REQUIRE(visitor.getMochaCount() == 0);
        REQUIRE(visitor.getWhipCount() == 0);
        REQUIRE(visitor.getEspressoCount() == 0);
    }
}