// File: tests/unit_tests/stl_tests/AlgorithmTests.cpp
// Algorithm implementation tests for CppVerseHub STL showcase functionality

#include <catch2/catch.hpp>
#include <vector>
#include <algorithm>
#include <numeric>
#include <functional>
#include <iterator>
#include <random>
#include <string>
#include <map>
#include <set>
#include <memory>
#include <chrono>
#include <execution>

// Include the STL showcase headers
#include "Algorithms.hpp"
#include "Planet.hpp"
#include "Fleet.hpp"
#include "Entity.hpp"

using namespace CppVerseHub::STL;
using namespace CppVerseHub::Core;

/**
 * @brief Test fixture for STL algorithm tests
 */
class AlgorithmTestFixture {
public:
    AlgorithmTestFixture() {
        setupTestData();
    }
    
    ~AlgorithmTestFixture() {
        testPlanets.clear();
        testFleets.clear();
    }
    
protected:
    void setupTestData() {
        // Create test planets with varying properties
        for (int i = 0; i < 20; ++i) {
            auto planet = std::make_unique<Planet>(
                "Planet_" + std::to_string(i),
                Vector3D{i * 50.0, i * 50.0, i * 50.0}
            );
            
            planet->setResourceAmount(ResourceType::MINERALS, 500 + i * 100);
            planet->setResourceAmount(ResourceType::ENERGY, 300 + i * 50);
            planet->setResourceAmount(ResourceType::FOOD, 200 + i * 25);
            planet->setHabitabilityRating(0.2 + (i % 10) * 0.08);
            planet->setPopulation(1000 + i * 500);
            
            testPlanets.push_back(std::move(planet));
        }
        
        // Create test fleets
        for (int i = 0; i < 10; ++i) {
            auto fleet = std::make_unique<Fleet>(
                "Fleet_" + std::to_string(i),
                Vector3D{i * 100.0, i * 100.0, i * 100.0}
            );
            
            fleet->addShips(ShipType::FIGHTER, 5 + i * 2);
            fleet->addShips(ShipType::CRUISER, 2 + i);
            if (i % 3 == 0) {
                fleet->addShips(ShipType::BATTLESHIP, 1 + i / 3);
            }
            
            testFleets.push_back(std::move(fleet));
        }
        
        // Setup numeric test data
        testNumbers = {64, 34, 25, 12, 22, 11, 90, 5, 77, 30};
        testStrings = {"gamma", "alpha", "delta", "beta", "epsilon", "zeta", "eta", "theta"};
        
        // Generate larger datasets for performance testing
        largeNumbers.reserve(100000);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(1, 1000000);
        
        for (int i = 0; i < 100000; ++i) {
            largeNumbers.push_back(dist(gen));
        }
    }
    
    std::vector<std::unique_ptr<Planet>> testPlanets;
    std::vector<std::unique_ptr<Fleet>> testFleets;
    std::vector<int> testNumbers;
    std::vector<std::string> testStrings;
    std::vector<int> largeNumbers;
};

TEST_CASE_METHOD(AlgorithmTestFixture, "Non-Modifying Sequence Operations", "[algorithms][stl][non-modifying]") {
    
    SECTION("Search Algorithms") {
        SECTION("find and find_if") {
            // Find specific planet by name
            auto it = std::find_if(testPlanets.begin(), testPlanets.end(),
                                  [](const std::unique_ptr<Planet>& p) {
                                      return p->getName() == "Planet_5";
                                  });
            
            REQUIRE(it != testPlanets.end());
            REQUIRE((*it)->getName() == "Planet_5");
            
            // Find planet with specific habitability
            auto habitableIt = std::find_if(testPlanets.begin(), testPlanets.end(),
                                           [](const std::unique_ptr<Planet>& p) {
                                               return p->getHabitabilityRating() > 0.7;
                                           });
            
            if (habitableIt != testPlanets.end()) {
                REQUIRE((*habitableIt)->getHabitabilityRating() > 0.7);
            }
            
            // Find first planet with high mineral content
            auto mineralRichIt = std::find_if(testPlanets.begin(), testPlanets.end(),
                                             [](const std::unique_ptr<Planet>& p) {
                                                 return p->getResourceAmount(ResourceType::MINERALS) > 1500;
                                             });
            
            if (mineralRichIt != testPlanets.end()) {
                REQUIRE((*mineralRichIt)->getResourceAmount(ResourceType::MINERALS) > 1500);
            }
        }
        
        SECTION("search and search_n") {
            // Search for sequence of numbers
            std::vector<int> sequence = {22, 11};
            auto seqIt = std::search(testNumbers.begin(), testNumbers.end(),
                                    sequence.begin(), sequence.end());
            
            // Check if sequence was found
            if (seqIt != testNumbers.end()) {
                REQUIRE(*seqIt == 22);
                REQUIRE(*(seqIt + 1) == 11);
            }
            
            // Search for consecutive equal elements
            std::vector<int> duplicates = {5, 5, 5, 10, 10};
            auto consecutive = std::search_n(duplicates.begin(), duplicates.end(), 3, 5);
            
            REQUIRE(consecutive != duplicates.end());
            REQUIRE(*consecutive == 5);
        }
        
        SECTION("binary_search on sorted data") {
            // Create sorted vector of planet mineral amounts
            std::vector<int> mineralAmounts;
            for (const auto& planet : testPlanets) {
                mineralAmounts.push_back(planet->getResourceAmount(ResourceType::MINERALS));
            }
            std::sort(mineralAmounts.begin(), mineralAmounts.end());
            
            // Binary search for specific amounts
            bool found1000 = std::binary_search(mineralAmounts.begin(), mineralAmounts.end(), 1000);
            bool found9999 = std::binary_search(mineralAmounts.begin(), mineralAmounts.end(), 9999);
            
            REQUIRE_FALSE(found9999); // Should not exist
            
            // Use lower_bound and upper_bound
            auto lower = std::lower_bound(mineralAmounts.begin(), mineralAmounts.end(), 1000);
            auto upper = std::upper_bound(mineralAmounts.begin(), mineralAmounts.end(), 1500);
            
            // Count elements in range [1000, 1500]
            int countInRange = std::distance(lower, upper);
            REQUIRE(countInRange >= 0);
        }
    }
    
    SECTION("Counting and Comparison") {
        SECTION("count and count_if") {
            // Count planets with high habitability
            int habitableCount = std::count_if(testPlanets.begin(), testPlanets.end(),
                                              [](const std::unique_ptr<Planet>& p) {
                                                  return p->getHabitabilityRating() > 0.6;
                                              });
            
            REQUIRE(habitableCount >= 0);
            REQUIRE(habitableCount <= static_cast<int>(testPlanets.size()));
            
            // Count fleets with battleships
            int battleshipFleets = std::count_if(testFleets.begin(), testFleets.end(),
                                                [](const std::unique_ptr<Fleet>& f) {
                                                    return f->getShipCount(ShipType::BATTLESHIP) > 0;
                                                });
            
            REQUIRE(battleshipFleets >= 0);
            REQUIRE(battleshipFleets <= static_cast<int>(testFleets.size()));
            
            // Count specific numbers
            int count25 = std::count(testNumbers.begin(), testNumbers.end(), 25);
            REQUIRE(count25 >= 0);
        }
        
        SECTION("all_of, any_of, none_of") {
            // Check if all planets have positive population
            bool allHavePopulation = std::all_of(testPlanets.begin(), testPlanets.end(),
                                                [](const std::unique_ptr<Planet>& p) {
                                                    return p->getPopulation() > 0;
                                                });
            
            REQUIRE(allHavePopulation);
            
            // Check if any planet is extremely habitable
            bool anyExtremelyHabitable = std::any_of(testPlanets.begin(), testPlanets.end(),
                                                    [](const std::unique_ptr<Planet>& p) {
                                                        return p->getHabitabilityRating() > 0.95;
                                                    });
            
            INFO("Any extremely habitable: " << anyExtremelyHabitable);
            
            // Check if no planet has negative resources
            bool noneHaveNegativeResources = std::none_of(testPlanets.begin(), testPlanets.end(),
                                                          [](const std::unique_ptr<Planet>& p) {
                                                              return p->getResourceAmount(ResourceType::MINERALS) < 0 ||
                                                                     p->getResourceAmount(ResourceType::ENERGY) < 0 ||
                                                                     p->getResourceAmount(ResourceType::FOOD) < 0;
                                                          });
            
            REQUIRE(noneHaveNegativeResources);
        }
        
        SECTION("equal and mismatch") {
            // Compare two planet collections
            std::vector<Planet*> planetPtrs1, planetPtrs2;
            
            for (const auto& planet : testPlanets) {
                planetPtrs1.push_back(planet.get());
                planetPtrs2.push_back(planet.get());
            }
            
            // Should be equal
            bool areEqual = std::equal(planetPtrs1.begin(), planetPtrs1.end(), planetPtrs2.begin());
            REQUIRE(areEqual);
            
            // Modify one element
            if (!planetPtrs2.empty()) {
                planetPtrs2[0] = nullptr;
                
                auto mismatchPair = std::mismatch(planetPtrs1.begin(), planetPtrs1.end(), planetPtrs2.begin());
                
                REQUIRE(mismatchPair.first != planetPtrs1.end());
                REQUIRE(mismatchPair.second != planetPtrs2.end());
                REQUIRE(*mismatchPair.first != *mismatchPair.second);
            }
        }
    }
    
    SECTION("Min-Max Operations") {
        SECTION("min_element and max_element") {
            // Find planet with minimum habitability
            auto minHabitabilityIt = std::min_element(testPlanets.begin(), testPlanets.end(),
                                                     [](const std::unique_ptr<Planet>& a, const std::unique_ptr<Planet>& b) {
                                                         return a->getHabitabilityRating() < b->getHabitabilityRating();
                                                     });
            
            REQUIRE(minHabitabilityIt != testPlanets.end());
            
            // Find planet with maximum mineral resources
            auto maxMineralsIt = std::max_element(testPlanets.begin(), testPlanets.end(),
                                                 [](const std::unique_ptr<Planet>& a, const std::unique_ptr<Planet>& b) {
                                                     return a->getResourceAmount(ResourceType::MINERALS) < 
                                                            b->getResourceAmount(ResourceType::MINERALS);
                                                 });
            
            REQUIRE(maxMineralsIt != testPlanets.end());
            
            // Verify it's actually the maximum
            for (const auto& planet : testPlanets) {
                REQUIRE(planet->getResourceAmount(ResourceType::MINERALS) <= 
                       (*maxMineralsIt)->getResourceAmount(ResourceType::MINERALS));
            }
            
            // Find minmax for numbers
            auto minmaxPair = std::minmax_element(testNumbers.begin(), testNumbers.end());
            
            REQUIRE(minmaxPair.first != testNumbers.end());
            REQUIRE(minmaxPair.second != testNumbers.end());
            REQUIRE(*minmaxPair.first <= *minmaxPair.second);
            
            // Verify they are actually min and max
            for (int num : testNumbers) {
                REQUIRE(num >= *minmaxPair.first);
                REQUIRE(num <= *minmaxPair.second);
            }
        }
    }
}

TEST_CASE_METHOD(AlgorithmTestFixture, "Modifying Sequence Operations", "[algorithms][stl][modifying]") {
    
    SECTION("Copy and Move Operations") {
        SECTION("copy and copy_if") {
            // Copy all planet names
            std::vector<std::string> planetNames;
            std::transform(testPlanets.begin(), testPlanets.end(), std::back_inserter(planetNames),
                          [](const std::unique_ptr<Planet>& p) {
                              return p->getName();
                          });
            
            REQUIRE(planetNames.size() == testPlanets.size());
            
            // Copy only habitable planets
            std::vector<Planet*> habitablePlanets;
            std::copy_if(testPlanets.begin(), testPlanets.end(), std::back_inserter(habitablePlanets),
                        [](const std::unique_ptr<Planet>& p) {
                            return p->getHabitabilityRating() > 0.5;
                        });
            
            // Verify all copied planets meet criteria
            for (const auto& planet : habitablePlanets) {
                REQUIRE(planet->getHabitabilityRating() > 0.5);
            }
            
            // Copy numbers with condition
            std::vector<int> evenNumbers;
            std::copy_if(testNumbers.begin(), testNumbers.end(), std::back_inserter(evenNumbers),
                        [](int n) { return n % 2 == 0; });
            
            // Verify all copied numbers are even
            for (int num : evenNumbers) {
                REQUIRE(num % 2 == 0);
            }
        }
        
        SECTION("transform operations") {
            // Transform planet populations to population density
            std::vector<double> populationDensities;
            std::transform(testPlanets.begin(), testPlanets.end(), std::back_inserter(populationDensities),
                          [](const std::unique_ptr<Planet>& p) {
                              // Simplified calculation: population / (habitability * 1000)
                              double habitability = std::max(0.1, p->getHabitabilityRating());
                              return p->getPopulation() / (habitability * 1000.0);
                          });
            
            REQUIRE(populationDensities.size() == testPlanets.size());
            
            // All densities should be positive
            for (double density : populationDensities) {
                REQUIRE(density > 0);
            }
            
            // Transform numbers to their squares
            std::vector<int> squares;
            std::transform(testNumbers.begin(), testNumbers.end(), std::back_inserter(squares),
                          [](int n) { return n * n; });
            
            REQUIRE(squares.size() == testNumbers.size());
            
            // Verify transformation
            for (size_t i = 0; i < testNumbers.size(); ++i) {
                REQUIRE(squares[i] == testNumbers[i] * testNumbers[i]);
            }
            
            // Binary transform: combine fleet and planet data
            if (!testFleets.empty() && !testPlanets.empty()) {
                std::vector<std::string> combinedInfo;
                size_t minSize = std::min(testFleets.size(), testPlanets.size());
                
                std::transform(testFleets.begin(), testFleets.begin() + minSize,
                              testPlanets.begin(),
                              std::back_inserter(combinedInfo),
                              [](const std::unique_ptr<Fleet>& f, const std::unique_ptr<Planet>& p) {
                                  return f->getName() + " orbiting " + p->getName();
                              });
                
                REQUIRE(combinedInfo.size() == minSize);
                REQUIRE(combinedInfo[0].find("Fleet_0") != std::string::npos);
                REQUIRE(combinedInfo[0].find("Planet_0") != std::string::npos);
            }
        }
    }
    
    SECTION("Fill and Generate Operations") {
        SECTION("fill and fill_n") {
            std::vector<int> fillTest(10);
            
            // Fill entire vector
            std::fill(fillTest.begin(), fillTest.end(), 42);
            
            for (int val : fillTest) {
                REQUIRE(val == 42);
            }
            
            // Fill first 5 elements
            std::fill_n(fillTest.begin(), 5, 99);
            
            for (size_t i = 0; i < 5; ++i) {
                REQUIRE(fillTest[i] == 99);
            }
            for (size_t i = 5; i < fillTest.size(); ++i) {
                REQUIRE(fillTest[i] == 42);
            }
        }
        
        SECTION("generate and generate_n") {
            std::vector<int> randomNums(100);
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dist(1, 100);
            
            // Generate random numbers
            std::generate(randomNums.begin(), randomNums.end(), [&]() {
                return dist(gen);
            });
            
            // All numbers should be in range [1, 100]
            for (int num : randomNums) {
                REQUIRE(num >= 1);
                REQUIRE(num <= 100);
            }
            
            // Generate sequence
            std::vector<int> sequence(20);
            int counter = 0;
            std::generate_n(sequence.begin(), 10, [&]() {
                return counter++;
            });
            
            // First 10 elements should be 0-9
            for (int i = 0; i < 10; ++i) {
                REQUIRE(sequence[i] == i);
            }
        }
    }
    
    SECTION("Removal Operations") {
        SECTION("remove and remove_if") {
            std::vector<int> removeTest = testNumbers;
            
            // Remove specific value
            auto newEnd = std::remove(removeTest.begin(), removeTest.end(), 25);
            removeTest.erase(newEnd, removeTest.end());
            
            // 25 should not be in the vector anymore
            REQUIRE(std::find(removeTest.begin(), removeTest.end(), 25) == removeTest.end());
            
            // Remove even numbers
            std::vector<int> oddOnly = testNumbers;
            auto oddEnd = std::remove_if(oddOnly.begin(), oddOnly.end(), [](int n) {
                return n % 2 == 0;
            });
            oddOnly.erase(oddEnd, oddOnly.end());
            
            // All remaining numbers should be odd
            for (int num : oddOnly) {
                REQUIRE(num % 2 != 0);
            }
        }
        
        SECTION("unique operations") {
            std::vector<int> duplicates = {1, 1, 2, 2, 2, 3, 4, 4, 5};
            
            // Remove consecutive duplicates
            auto uniqueEnd = std::unique(duplicates.begin(), duplicates.end());
            duplicates.erase(uniqueEnd, duplicates.end());
            
            // Should be {1, 2, 3, 4, 5}
            std::vector<int> expected = {1, 2, 3, 4, 5};
            REQUIRE(duplicates == expected);
            
            // Remove duplicates with custom predicate
            std::vector<Planet*> planetPtrs;
            for (const auto& planet : testPlanets) {
                planetPtrs.push_back(planet.get());
                planetPtrs.push_back(planet.get()); // Add duplicate
            }
            
            std::sort(planetPtrs.begin(), planetPtrs.end());
            auto planetUniqueEnd = std::unique(planetPtrs.begin(), planetPtrs.end());
            planetPtrs.erase(planetUniqueEnd, planetPtrs.end());
            
            REQUIRE(planetPtrs.size() == testPlanets.size());
        }
    }
    
    SECTION("Replacement Operations") {
        SECTION("replace and replace_if") {
            std::vector<int> replaceTest = testNumbers;
            
            // Replace specific value
            std::replace(replaceTest.begin(), replaceTest.end(), 25, 250);
            
            // 25 should be replaced with 250
            REQUIRE(std::find(replaceTest.begin(), replaceTest.end(), 25) == replaceTest.end());
            
            int count250 = std::count(replaceTest.begin(), replaceTest.end(), 250);
            REQUIRE(count250 > 0);
            
            // Replace values with condition
            std::replace_if(replaceTest.begin(), replaceTest.end(), 
                           [](int n) { return n > 50; }, 9999);
            
            // All values > 50 should now be 9999
            for (int num : replaceTest) {
                if (num != 9999 && num != 250) {
                    REQUIRE(num <= 50);
                }
            }
        }
        
        SECTION("replace_copy variants") {
            std::vector<int> original = testNumbers;
            std::vector<int> replaced;
            
            // Replace and copy to new container
            std::replace_copy(original.begin(), original.end(), std::back_inserter(replaced), 
                             25, 250);
            
            REQUIRE(replaced.size() == original.size());
            
            // Original should be unchanged
            REQUIRE(std::find(original.begin(), original.end(), 25) != original.end());
            
            // Replaced should have 250 instead of 25
            REQUIRE(std::find(replaced.begin(), replaced.end(), 25) == replaced.end());
        }
    }
}

TEST_CASE_METHOD(AlgorithmTestFixture, "Sorting and Permutation Algorithms", "[algorithms][stl][sorting]") {
    
    SECTION("Sorting Operations") {
        SECTION("sort with different comparators") {
            // Sort planets by habitability (ascending)
            std::vector<Planet*> planetsByHabitability;
            for (const auto& planet : testPlanets) {
                planetsByHabitability.push_back(planet.get());
            }
            
            std::sort(planetsByHabitability.begin(), planetsByHabitability.end(),
                     [](const Planet* a, const Planet* b) {
                         return a->getHabitabilityRating() < b->getHabitabilityRating();
                     });
            
            // Verify sorting
            for (size_t i = 1; i < planetsByHabitability.size(); ++i) {
                REQUIRE(planetsByHabitability[i-1]->getHabitabilityRating() <= 
                       planetsByHabitability[i]->getHabitabilityRating());
            }
            
            // Sort fleets by combat power (descending)
            std::vector<Fleet*> fleetsByCombatPower;
            for (const auto& fleet : testFleets) {
                fleetsByCombatPower.push_back(fleet.get());
            }
            
            std::sort(fleetsByCombatPower.begin(), fleetsByCombatPower.end(),
                     [](const Fleet* a, const Fleet* b) {
                         return a->getCombatPower() > b->getCombatPower(); // Descending
                     });
            
            // Verify descending order
            for (size_t i = 1; i < fleetsByCombatPower.size(); ++i) {
                REQUIRE(fleetsByCombatPower[i-1]->getCombatPower() >= 
                       fleetsByCombatPower[i]->getCombatPower());
            }
        }
        
        SECTION("partial_sort for top-k elements") {
            std::vector<int> numbers = testNumbers;
            
            // Get top 3 largest numbers
            std::partial_sort(numbers.begin(), numbers.begin() + 3, numbers.end(), 
                             std::greater<int>());
            
            // First 3 elements should be the largest, in descending order
            for (int i = 1; i < 3; ++i) {
                REQUIRE(numbers[i-1] >= numbers[i]);
            }
            
            // These should be among the largest in the original vector
            std::vector<int> sortedOriginal = testNumbers;
            std::sort(sortedOriginal.rbegin(), sortedOriginal.rend());
            
            for (int i = 0; i < 3; ++i) {
                REQUIRE(numbers[i] == sortedOriginal[i]);
            }
        }
        
        SECTION("nth_element for median/quantiles") {
            std::vector<double> habitabilityRatings;
            for (const auto& planet : testPlanets) {
                habitabilityRatings.push_back(planet->getHabitabilityRating());
            }
            
            // Find median
            size_t medianPos = habitabilityRatings.size() / 2;
            std::nth_element(habitabilityRatings.begin(), 
                           habitabilityRatings.begin() + medianPos,
                           habitabilityRatings.end());
            
            double median = habitabilityRatings[medianPos];
            
            // Elements before median should be <= median
            for (size_t i = 0; i < medianPos; ++i) {
                REQUIRE(habitabilityRatings[i] <= median);
            }
            
            // Elements after median should be >= median
            for (size_t i = medianPos + 1; i < habitabilityRatings.size(); ++i) {
                REQUIRE(habitabilityRatings[i] >= median);
            }
        }
        
        SECTION("stable_sort preserves relative order") {
            struct PlanetWithIndex {
                Planet* planet;
                int originalIndex;
            };
            
            std::vector<PlanetWithIndex> indexedPlanets;
            for (size_t i = 0; i < testPlanets.size(); ++i) {
                indexedPlanets.push_back({testPlanets[i].get(), static_cast<int>(i)});
            }
            
            // Stable sort by habitability (rounded to nearest 0.1)
            std::stable_sort(indexedPlanets.begin(), indexedPlanets.end(),
                            [](const PlanetWithIndex& a, const PlanetWithIndex& b) {
                                int aRounded = static_cast<int>(a.planet->getHabitabilityRating() * 10);
                                int bRounded = static_cast<int>(b.planet->getHabitabilityRating() * 10);
                                return aRounded < bRounded;
                            });
            
            // Among planets with same rounded habitability, original order should be preserved
            for (size_t i = 1; i < indexedPlanets.size(); ++i) {
                int prevRounded = static_cast<int>(indexedPlanets[i-1].planet->getHabitabilityRating() * 10);
                int currRounded = static_cast<int>(indexedPlanets[i].planet->getHabitabilityRating() * 10);
                
                if (prevRounded == currRounded) {
                    REQUIRE(indexedPlanets[i-1].originalIndex < indexedPlanets[i].originalIndex);
                }
            }
        }
    }
    
    SECTION("Permutation Operations") {
        SECTION("next_permutation and prev_permutation") {
            std::vector<int> smallSet = {1, 2, 3};
            std::vector<std::vector<int>> permutations;
            
            // Generate all permutations
            do {
                permutations.push_back(smallSet);
            } while (std::next_permutation(smallSet.begin(), smallSet.end()));
            
            // Should have 3! = 6 permutations
            REQUIRE(permutations.size() == 6);
            
            // All permutations should be unique
            std::set<std::vector<int>> uniquePerms(permutations.begin(), permutations.end());
            REQUIRE(uniquePerms.size() == 6);
            
            // Each permutation should contain the same elements
            for (const auto& perm : permutations) {
                REQUIRE(perm.size() == 3);
                REQUIRE(std::find(perm.begin(), perm.end(), 1) != perm.end());
                REQUIRE(std::find(perm.begin(), perm.end(), 2) != perm.end());
                REQUIRE(std::find(perm.begin(), perm.end(), 3) != perm.end());
            }
        }
        
        SECTION("is_permutation") {
            std::vector<int> original = {1, 2, 3, 4, 5};
            std::vector<int> permuted = {5, 3, 1, 4, 2};
            std::vector<int> notPermuted = {1, 2, 3, 4, 6};
            
            REQUIRE(std::is_permutation(original.begin(), original.end(), permuted.begin()));
            REQUIRE_FALSE(std::is_permutation(original.begin(), original.end(), notPermuted.begin()));
            
            // Test with fleet IDs
            std::vector<int> fleetIds, shuffledIds;
            for (const auto& fleet : testFleets) {
                fleetIds.push_back(fleet->getId());
                shuffledIds.push_back(fleet->getId());
            }
            
            std::random_device rd;
            std::mt19937 g(rd());
            std::shuffle(shuffledIds.begin(), shuffledIds.end(), g);
            
            REQUIRE(std::is_permutation(fleetIds.begin(), fleetIds.end(), shuffledIds.begin()));
        }
        
        SECTION("random_shuffle and shuffle") {
            std::vector<Planet*> planetPtrs;
            for (const auto& planet : testPlanets) {
                planetPtrs.push_back(planet.get());
            }
            
            std::vector<Planet*> originalOrder = planetPtrs;
            
            // Shuffle the vector
            std::random_device rd;
            std::mt19937 g(rd());
            std::shuffle(planetPtrs.begin(), planetPtrs.end(), g);
            
            // Should contain the same elements
            REQUIRE(std::is_permutation(originalOrder.begin(), originalOrder.end(), planetPtrs.begin()));
            
            // Very likely to be in different order (unless very small)
            if (planetPtrs.size() > 2) {
                bool different = false;
                for (size_t i = 0; i < planetPtrs.size(); ++i) {
                    if (planetPtrs[i] != originalOrder[i]) {
                        different = true;
                        break;
                    }
                }
                // Note: There's a tiny chance this could fail due to random chance
                INFO("Shuffle created different order: " << different);
            }
        }
    }
}

TEST_CASE_METHOD(AlgorithmTestFixture, "Numeric Algorithms", "[algorithms][stl][numeric]") {
    
    SECTION("Accumulation Operations") {
        SECTION("accumulate with different operations") {
            // Sum all mineral resources
            int totalMinerals = std::accumulate(testPlanets.begin(), testPlanets.end(), 0,
                                               [](int sum, const std::unique_ptr<Planet>& p) {
                                                   return sum + p->getResourceAmount(ResourceType::MINERALS);
                                               });
            
            REQUIRE(totalMinerals > 0);
            
            // Calculate total combat power of all fleets
            double totalCombatPower = std::accumulate(testFleets.begin(), testFleets.end(), 0.0,
                                                     [](double sum, const std::unique_ptr<Fleet>& f) {
                                                         return sum + f->getCombatPower();
                                                     });
            
            REQUIRE(totalCombatPower > 0);
            
            // Find maximum habitability using accumulate
            double maxHabitability = std::accumulate(testPlanets.begin(), testPlanets.end(), 0.0,
                                                    [](double maxVal, const std::unique_ptr<Planet>& p) {
                                                        return std::max(maxVal, p->getHabitabilityRating());
                                                    });
            
            REQUIRE(maxHabitability >= 0.0);
            REQUIRE(maxHabitability <= 1.0);
            
            // String concatenation
            std::string allNames = std::accumulate(testStrings.begin(), testStrings.end(), std::string(""),
                                                  [](const std::string& result, const std::string& str) {
                                                      return result.empty() ? str : result + ", " + str;
                                                  });
            
            REQUIRE(!allNames.empty());
            REQUIRE(allNames.find("alpha") != std::string::npos);
        }
        
        SECTION("inner_product for complex calculations") {
            // Calculate weighted resource value
            std::vector<int> resourceCounts;
            std::vector<double> resourceValues = {10.0, 5.0, 2.0}; // Minerals, Energy, Food values
            
            for (const auto& planet : testPlanets) {
                if (resourceCounts.empty()) {
                    resourceCounts.push_back(planet->getResourceAmount(ResourceType::MINERALS));
                    resourceCounts.push_back(planet->getResourceAmount(ResourceType::ENERGY));
                    resourceCounts.push_back(planet->getResourceAmount(ResourceType::FOOD));
                    break; // Just test with first planet
                }
            }
            
            if (!resourceCounts.empty()) {
                double totalValue = std::inner_product(resourceCounts.begin(), resourceCounts.end(),
                                                      resourceValues.begin(), 0.0);
                
                REQUIRE(totalValue > 0);
                
                // Manual calculation for verification
                double expected = resourceCounts[0] * 10.0 + resourceCounts[1] * 5.0 + resourceCounts[2] * 2.0;
                REQUIRE(totalValue == Approx(expected));
            }
        }
    }
    
    SECTION("Sequence Generation") {
        SECTION("iota for sequence generation") {
            std::vector<int> sequence(10);
            std::iota(sequence.begin(), sequence.end(), 1);
            
            // Should be [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
            for (size_t i = 0; i < sequence.size(); ++i) {
                REQUIRE(sequence[i] == static_cast<int>(i + 1));
            }
            
            // Generate planet IDs
            std::vector<int> planetIds(testPlanets.size());
            std::iota(planetIds.begin(), planetIds.end(), 1000);
            
            for (size_t i = 0; i < planetIds.size(); ++i) {
                REQUIRE(planetIds[i] == static_cast<int>(1000 + i));
            }
        }
        
        SECTION("partial_sum for cumulative calculations") {
            // Calculate cumulative resource totals
            std::vector<int> mineralAmounts;
            for (const auto& planet : testPlanets) {
                mineralAmounts.push_back(planet->getResourceAmount(ResourceType::MINERALS));
            }
            
            std::vector<int> cumulativeMinerals(mineralAmounts.size());
            std::partial_sum(mineralAmounts.begin(), mineralAmounts.end(), cumulativeMinerals.begin());
            
            // Verify cumulative sum
            int runningSum = 0;
            for (size_t i = 0; i < mineralAmounts.size(); ++i) {
                runningSum += mineralAmounts[i];
                REQUIRE(cumulativeMinerals[i] == runningSum);
            }
            
            // Calculate differences
            std::vector<int> differences(mineralAmounts.size());
            std::adjacent_difference(mineralAmounts.begin(), mineralAmounts.end(), differences.begin());
            
            // First element should be unchanged, rest should be differences
            REQUIRE(differences[0] == mineralAmounts[0]);
            for (size_t i = 1; i < differences.size(); ++i) {
                REQUIRE(differences[i] == mineralAmounts[i] - mineralAmounts[i-1]);
            }
        }
    }
    
    SECTION("Statistical Operations") {
        SECTION("Custom statistical calculations") {
            // Calculate mean habitability
            double totalHabitability = std::accumulate(testPlanets.begin(), testPlanets.end(), 0.0,
                                                      [](double sum, const std::unique_ptr<Planet>& p) {
                                                          return sum + p->getHabitabilityRating();
                                                      });
            
            double meanHabitability = totalHabitability / testPlanets.size();
            REQUIRE(meanHabitability >= 0.0);
            REQUIRE(meanHabitability <= 1.0);
            
            // Calculate variance
            double variance = std::accumulate(testPlanets.begin(), testPlanets.end(), 0.0,
                                            [meanHabitability](double sum, const std::unique_ptr<Planet>& p) {
                                                double diff = p->getHabitabilityRating() - meanHabitability;
                                                return sum + diff * diff;
                                            }) / testPlanets.size();
            
            REQUIRE(variance >= 0.0);
            
            double stdDev = std::sqrt(variance);
            INFO("Mean habitability: " << meanHabitability);
            INFO("Standard deviation: " << stdDev);
        }
        
        SECTION("Quantile calculations") {
            std::vector<double> combatPowers;
            for (const auto& fleet : testFleets) {
                combatPowers.push_back(fleet->getCombatPower());
            }
            
            std::sort(combatPowers.begin(), combatPowers.end());
            
            // Calculate quartiles
            auto q1Pos = combatPowers.size() / 4;
            auto q2Pos = combatPowers.size() / 2;  // Median
            auto q3Pos = (combatPowers.size() * 3) / 4;
            
            if (!combatPowers.empty()) {
                double q1 = combatPowers[q1Pos];
                double median = combatPowers[q2Pos];
                double q3 = combatPowers[q3Pos];
                
                REQUIRE(q1 <= median);
                REQUIRE(median <= q3);
                
                INFO("Q1: " << q1 << ", Median: " << median << ", Q3: " << q3);
            }
        }
    }
}

TEST_CASE_METHOD(AlgorithmTestFixture, "Heap Operations", "[algorithms][stl][heap]") {
    
    SECTION("Heap Creation and Manipulation") {
        SECTION("make_heap and heap operations") {
            std::vector<int> heapData = testNumbers;
            
            // Create max heap
            std::make_heap(heapData.begin(), heapData.end());
            
            // Verify heap property (parent >= children)
            REQUIRE(std::is_heap(heapData.begin(), heapData.end()));
            
            // Extract maximum
            int maxElement = heapData.front();
            std::pop_heap(heapData.begin(), heapData.end());
            heapData.pop_back();
            
            // Remaining should still be a heap
            REQUIRE(std::is_heap(heapData.begin(), heapData.end()));
            
            // All remaining elements should be <= extracted max
            for (int elem : heapData) {
                REQUIRE(elem <= maxElement);
            }
            
            // Add new element
            heapData.push_back(maxElement + 10);
            std::push_heap(heapData.begin(), heapData.end());
            
            REQUIRE(std::is_heap(heapData.begin(), heapData.end()));
            REQUIRE(heapData.front() == maxElement + 10);
        }
        
        SECTION("Custom heap comparator") {
            // Create min heap using fleet combat power
            std::vector<Fleet*> fleetHeap;
            for (const auto& fleet : testFleets) {
                fleetHeap.push_back(fleet.get());
            }
            
            // Min heap comparator
            auto minHeapComp = [](const Fleet* a, const Fleet* b) {
                return a->getCombatPower() > b->getCombatPower();
            };
            
            std::make_heap(fleetHeap.begin(), fleetHeap.end(), minHeapComp);
            REQUIRE(std::is_heap(fleetHeap.begin(), fleetHeap.end(), minHeapComp));
            
            // Extract minimum
            Fleet* minFleet = fleetHeap.front();
            std::pop_heap(fleetHeap.begin(), fleetHeap.end(), minHeapComp);
            fleetHeap.pop_back();
            
            // Verify it was the minimum
            for (const Fleet* fleet : fleetHeap) {
                REQUIRE(fleet->getCombatPower() >= minFleet->getCombatPower());
            }
        }
    }
    
    SECTION("Heap Sort") {
        SECTION("sort_heap operation") {
            std::vector<int> sortData = testNumbers;
            
            // Create heap first
            std::make_heap(sortData.begin(), sortData.end());
            
            // Sort using heap sort
            std::sort_heap(sortData.begin(), sortData.end());
            
            // Should be sorted in ascending order
            REQUIRE(std::is_sorted(sortData.begin(), sortData.end()));
            
            // Should contain same elements as original
            std::vector<int> originalSorted = testNumbers;
            std::sort(originalSorted.begin(), originalSorted.end());
            REQUIRE(sortData == originalSorted);
        }
    }
}

TEST_CASE_METHOD(AlgorithmTestFixture, "Set Operations on Sorted Ranges", "[algorithms][stl][set-ops]") {
    
    SECTION("Set Union and Intersection") {
        SECTION("Basic set operations") {
            std::vector<int> set1 = {1, 3, 5, 7, 9};
            std::vector<int> set2 = {2, 4, 6, 8, 10};
            std::vector<int> set3 = {5, 6, 7, 8, 9};
            
            std::vector<int> unionResult;
            std::set_union(set1.begin(), set1.end(), set2.begin(), set2.end(),
                          std::back_inserter(unionResult));
            
            // Union should contain all elements from both sets
            REQUIRE(unionResult.size() == set1.size() + set2.size());
            REQUIRE(std::is_sorted(unionResult.begin(), unionResult.end()));
            
            std::vector<int> intersectionResult;
            std::set_intersection(set1.begin(), set1.end(), set3.begin(), set3.end(),
                                std::back_inserter(intersectionResult));
            
            // Intersection should be {5, 7, 9}
            std::vector<int> expectedIntersection = {5, 7, 9};
            REQUIRE(intersectionResult == expectedIntersection);
            
            std::vector<int> differenceResult;
            std::set_difference(set1.begin(), set1.end(), set3.begin(), set3.end(),
                              std::back_inserter(differenceResult));
            
            // Difference should be {1, 3}
            std::vector<int> expectedDifference = {1, 3};
            REQUIRE(differenceResult == expectedDifference);
        }
        
        SECTION("Set operations with game entities") {
            // Create sorted vectors of planets by different criteria
            std::vector<Planet*> habitablePlanets;
            std::vector<Planet*> resourceRichPlanets;
            
            for (const auto& planet : testPlanets) {
                if (planet->getHabitabilityRating() > 0.6) {
                    habitablePlanets.push_back(planet.get());
                }
                if (planet->getResourceAmount(ResourceType::MINERALS) > 1500) {
                    resourceRichPlanets.push_back(planet.get());
                }
            }
            
            // Sort both vectors
            std::sort(habitablePlanets.begin(), habitablePlanets.end());
            std::sort(resourceRichPlanets.begin(), resourceRichPlanets.end());
            
            // Find planets that are both habitable and resource-rich
            std::vector<Planet*> idealPlanets;
            std::set_intersection(habitablePlanets.begin(), habitablePlanets.end(),
                                resourceRichPlanets.begin(), resourceRichPlanets.end(),
                                std::back_inserter(idealPlanets));
            
            // All planets in result should meet both criteria
            for (const Planet* planet : idealPlanets) {
                REQUIRE(planet->getHabitabilityRating() > 0.6);
                REQUIRE(planet->getResourceAmount(ResourceType::MINERALS) > 1500);
            }
            
            // Find planets that are habitable but not resource-rich
            std::vector<Planet*> habitableButPoor;
            std::set_difference(habitablePlanets.begin(), habitablePlanets.end(),
                              resourceRichPlanets.begin(), resourceRichPlanets.end(),
                              std::back_inserter(habitableButPoor));
            
            // All planets should be habitable but not resource-rich
            for (const Planet* planet : habitableButPoor) {
                REQUIRE(planet->getHabitabilityRating() > 0.6);
                REQUIRE(planet->getResourceAmount(ResourceType::MINERALS) <= 1500);
            }
        }
    }
    
    SECTION("Set Membership Tests") {
        SECTION("includes operation") {
            std::vector<int> superset = {1, 2, 3, 4, 5, 6, 7, 8, 9};
            std::vector<int> subset1 = {2, 4, 6, 8};
            std::vector<int> subset2 = {1, 3, 5, 7, 9};
            std::vector<int> notSubset = {1, 3, 5, 7, 11};
            
            REQUIRE(std::includes(superset.begin(), superset.end(), subset1.begin(), subset1.end()));
            REQUIRE(std::includes(superset.begin(), superset.end(), subset2.begin(), subset2.end()));
            REQUIRE_FALSE(std::includes(superset.begin(), superset.end(), notSubset.begin(), notSubset.end()));
        }
    }
}

TEST_CASE_METHOD(AlgorithmTestFixture, "Algorithm Performance Analysis", "[algorithms][stl][performance]") {
    
    SECTION("Sorting Algorithm Performance") {
        SECTION("Different sorting algorithms comparison") {
            const int testSize = 50000;
            std::vector<int> randomData;
            
            // Generate random data
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dist(1, 1000000);
            
            for (int i = 0; i < testSize; ++i) {
                randomData.push_back(dist(gen));
            }
            
            // Test std::sort
            std::vector<int> sortData = randomData;
            auto sortDuration = measurePerformance([&]() {
                std::sort(sortData.begin(), sortData.end());
            }, "std::sort on " + std::to_string(testSize) + " elements");
            
            REQUIRE(std::is_sorted(sortData.begin(), sortData.end()));
            
            // Test std::stable_sort
            std::vector<int> stableSortData = randomData;
            auto stableSortDuration = measurePerformance([&]() {
                std::stable_sort(stableSortData.begin(), stableSortData.end());
            }, "std::stable_sort on " + std::to_string(testSize) + " elements");
            
            REQUIRE(std::is_sorted(stableSortData.begin(), stableSortData.end()));
            
            // Test heap sort
            std::vector<int> heapSortData = randomData;
            auto heapSortDuration = measurePerformance([&]() {
                std::make_heap(heapSortData.begin(), heapSortData.end());
                std::sort_heap(heapSortData.begin(), heapSortData.end());
            }, "heap sort on " + std::to_string(testSize) + " elements");
            
            REQUIRE(std::is_sorted(heapSortData.begin(), heapSortData.end()));
            
            INFO("Sort: " << sortDuration.count() << "μs");
            INFO("Stable sort: " << stableSortDuration.count() << "μs");
            INFO("Heap sort: " << heapSortDuration.count() << "μs");
        }
        
        SECTION("Partial sorting performance") {
            std::vector<int> largeDataset = largeNumbers;
            const int k = 100; // Top k elements
            
            // Test partial_sort
            std::vector<int> partialSortData = largeDataset;
            auto partialSortDuration = measurePerformance([&]() {
                std::partial_sort(partialSortData.begin(), partialSortData.begin() + k, 
                                partialSortData.end(), std::greater<int>());
            }, "partial_sort for top " + std::to_string(k) + " elements");
            
            // Test full sort for comparison
            std::vector<int> fullSortData = largeDataset;
            auto fullSortDuration = measurePerformance([&]() {
                std::sort(fullSortData.begin(), fullSortData.end(), std::greater<int>());
            }, "full sort for comparison");
            
            // Partial sort should be faster
            REQUIRE(partialSortDuration.count() < fullSortDuration.count());
            
            // Results should be the same for first k elements
            for (int i = 0; i < k; ++i) {
                REQUIRE(partialSortData[i] == fullSortData[i]);
            }
        }
    }
    
    SECTION("Search Algorithm Performance") {
        SECTION("Linear vs Binary Search") {
            std::vector<int> sortedData = largeNumbers;
            std::sort(sortedData.begin(), sortedData.end());
            
            const int searchCount = 1000;
            std::vector<int> searchTargets;
            for (int i = 0; i < searchCount; ++i) {
                searchTargets.push_back(sortedData[i % sortedData.size()]);
            }
            
            // Linear search
            auto linearDuration = measurePerformance([&]() {
                volatile int found = 0;
                for (int target : searchTargets) {
                    if (std::find(sortedData.begin(), sortedData.end(), target) != sortedData.end()) {
                        found++;
                    }
                }
            }, "Linear search " + std::to_string(searchCount) + " times");
            
            // Binary search
            auto binaryDuration = measurePerformance([&]() {
                volatile int found = 0;
                for (int target : searchTargets) {
                    if (std::binary_search(sortedData.begin(), sortedData.end(), target)) {
                        found++;
                    }
                }
            }, "Binary search " + std::to_string(searchCount) + " times");
            
            // Binary search should be much faster
            REQUIRE(binaryDuration.count() < linearDuration.count());
            
            INFO("Linear search: " << linearDuration.count() << "μs");
            INFO("Binary search: " << binaryDuration.count() << "μs");
        }
    }
    
    SECTION("Algorithm Complexity Verification") {
        SECTION("Sort complexity - O(n log n)") {
            std::vector<int> sizes = {1000, 2000, 4000, 8000, 16000};
            std::vector<long long> times;
            
            for (int size : sizes) {
                std::vector<int> data(size);
                std::iota(data.begin(), data.end(), 0);
                std::shuffle(data.begin(), data.end(), std::mt19937{std::random_device{}()});
                
                auto duration = measurePerformance([&]() {
                    std::sort(data.begin(), data.end());
                });
                
                times.push_back(duration.count());
            }
            
            // Verify roughly O(n log n) growth
            for (size_t i = 1; i < sizes.size(); ++i) {
                double sizeRatio = static_cast<double>(sizes[i]) / sizes[i-1];
                double timeRatio = static_cast<double>(times[i]) / times[i-1];
                double expectedRatio = sizeRatio * std::log2(sizeRatio);
                
                INFO("Size ratio: " << sizeRatio << ", Time ratio: " << timeRatio 
                     << ", Expected (n log n): " << expectedRatio);
                
                // Time ratio should be roughly proportional to n log n ratio
                // Allow for significant variance due to implementation details and measurement noise
                REQUIRE(timeRatio > expectedRatio * 0.5);
                REQUIRE(timeRatio < expectedRatio * 3.0);
            }
        }
    }
}

TEST_CASE_METHOD(AlgorithmTestFixture, "Parallel Algorithm Support", "[algorithms][stl][parallel]") {
    
    SECTION("Parallel Execution Policies") {
        SECTION("Parallel sort performance") {
            if constexpr (std::__cpp_lib_parallel_algorithm >= 201603) {
                std::vector<int> sequentialData = largeNumbers;
                std::vector<int> parallelData = largeNumbers;
                
                // Sequential sort
                auto sequentialDuration = measurePerformance([&]() {
                    std::sort(std::execution::seq, sequentialData.begin(), sequentialData.end());
                }, "Sequential sort");
                
                // Parallel sort
                auto parallelDuration = measurePerformance([&]() {
                    std::sort(std::execution::par, parallelData.begin(), parallelData.end());
                }, "Parallel sort");
                
                // Both should produce the same result
                REQUIRE(sequentialData == parallelData);
                REQUIRE(std::is_sorted(sequentialData.begin(), sequentialData.end()));
                REQUIRE(std::is_sorted(parallelData.begin(), parallelData.end()));
                
                INFO("Sequential: " << sequentialDuration.count() << "μs");
                INFO("Parallel: " << parallelDuration.count() << "μs");
                
                // Parallel version might be faster (depends on hardware and data size)
                if (parallelDuration.count() < sequentialDuration.count()) {
                    INFO("Parallel sort was faster");
                } else {
                    INFO("Sequential sort was faster (possibly due to overhead)");
                }
            } else {
                WARN("Parallel algorithms not available in this C++ implementation");
            }
        }
        
        SECTION("Parallel for_each") {
            if constexpr (std::__cpp_lib_parallel_algorithm >= 201603) {
                std::vector<int> data = largeNumbers;
                std::atomic<long long> sum{0};
                
                auto parallelDuration = measurePerformance([&]() {
                    std::for_each(std::execution::par, data.begin(), data.end(), 
                                 [&sum](int n) {
                                     sum += n;
                                 });
                }, "Parallel for_each sum");
                
                // Compare with sequential version
                long long sequentialSum = 0;
                auto sequentialDuration = measurePerformance([&]() {
                    std::for_each(std::execution::seq, data.begin(), data.end(), 
                                 [&sequentialSum](int n) {
                                     sequentialSum += n;
                                 });
                }, "Sequential for_each sum");
                
                REQUIRE(sum.load() == sequentialSum);
                
                INFO("Sequential: " << sequentialDuration.count() << "μs");
                INFO("Parallel: " << parallelDuration.count() << "μs");
            }
        }
    }
}

TEST_CASE_METHOD(AlgorithmTestFixture, "Custom Algorithm Implementations", "[algorithms][custom][implementation]") {
    
    SECTION("Game-Specific Algorithms") {
        SECTION("Spatial clustering algorithm") {
            // Group planets by spatial proximity
            auto spatialCluster = [](const std::vector<std::unique_ptr<Planet>>& planets, 
                                   double maxDistance) {
                std::vector<std::vector<Planet*>> clusters;
                std::vector<bool> visited(planets.size(), false);
                
                for (size_t i = 0; i < planets.size(); ++i) {
                    if (visited[i]) continue;
                    
                    std::vector<Planet*> cluster;
                    std::queue<size_t> queue;
                    queue.push(i);
                    visited[i] = true;
                    
                    while (!queue.empty()) {
                        size_t current = queue.front();
                        queue.pop();
                        cluster.push_back(planets[current].get());
                        
                        for (size_t j = 0; j < planets.size(); ++j) {
                            if (!visited[j]) {
                                double distance = planets[current]->distanceTo(*planets[j]);
                                if (distance <= maxDistance) {
                                    visited[j] = true;
                                    queue.push(j);
                                }
                            }
                        }
                    }
                    
                    clusters.push_back(cluster);
                }
                
                return clusters;
            };
            
            auto clusters = spatialCluster(testPlanets, 200.0);
            
            REQUIRE(!clusters.empty());
            
            // Verify cluster properties
            int totalPlanetsInClusters = 0;
            for (const auto& cluster : clusters) {
                totalPlanetsInClusters += cluster.size();
                
                // All planets in cluster should be within maxDistance of at least one other
                for (const Planet* planet : cluster) {
                    bool hasNearNeighbor = false;
                    for (const Planet* other : cluster) {
                        if (planet != other && planet->distanceTo(*other) <= 200.0) {
                            hasNearNeighbor = true;
                            break;
                        }
                    }
                    // Single-planet clusters are allowed
                    if (cluster.size() > 1) {
                        REQUIRE(hasNearNeighbor);
                    }
                }
            }
            
            REQUIRE(totalPlanetsInClusters == static_cast<int>(testPlanets.size()));
        }
        
        SECTION("Resource optimization algorithm") {
            // Find optimal resource allocation using greedy algorithm
            auto optimizeResourceAllocation = [](const std::vector<std::unique_ptr<Planet>>& planets,
                                                int totalBudget) {
                struct Investment {
                    Planet* planet;
                    int cost;
                    double benefit;
                    double efficiency; // benefit per cost
                };
                
                std::vector<Investment> opportunities;
                
                // Generate investment opportunities
                for (const auto& planet : planets) {
                    int mineralCost = 100;
                    int energyCost = 150;
                    int foodCost = 80;
                    
                    double mineralBenefit = planet->getHabitabilityRating() * 50;
                    double energyBenefit = planet->getHabitabilityRating() * 75;
                    double foodBenefit = planet->getHabitabilityRating() * 40;
                    
                    opportunities.push_back({planet.get(), mineralCost, mineralBenefit, mineralBenefit / mineralCost});
                    opportunities.push_back({planet.get(), energyCost, energyBenefit, energyBenefit / energyCost});
                    opportunities.push_back({planet.get(), foodCost, foodBenefit, foodBenefit / foodCost});
                }
                
                // Sort by efficiency (descending)
                std::sort(opportunities.begin(), opportunities.end(),
                         [](const Investment& a, const Investment& b) {
                             return a.efficiency > b.efficiency;
                         });
                
                // Greedy selection
                std::vector<Investment> selected;
                int remainingBudget = totalBudget;
                
                for (const auto& investment : opportunities) {
                    if (investment.cost <= remainingBudget) {
                        selected.push_back(investment);
                        remainingBudget -= investment.cost;
                    }
                }
                
                return selected;
            };
            
            auto allocations = optimizeResourceAllocation(testPlanets, 5000);
            
            REQUIRE(!allocations.empty());
            
            // Verify budget constraint
            int totalCost = std::accumulate(allocations.begin(), allocations.end(), 0,
                                          [](int sum, const auto& investment) {
                                              return sum + investment.cost;
                                          });
            
            REQUIRE(totalCost <= 5000);
            
            // Calculate total benefit
            double totalBenefit = std::accumulate(allocations.begin(), allocations.end(), 0.0,
                                                [](double sum, const auto& investment) {
                                                    return sum + investment.benefit;
                                                });
            
            REQUIRE(totalBenefit > 0);
            
            INFO("Total cost: " << totalCost << ", Total benefit: " << totalBenefit);
        }
    }
}