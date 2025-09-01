// File: tests/unit_tests/stl_tests/ContainerTests.cpp
// STL container usage tests for CppVerseHub STL showcase functionality

#include <catch2/catch.hpp>
#include <vector>
#include <deque>
#include <list>
#include <map>
#include <unordered_map>
#include <set>
#include <unordered_set>
#include <queue>
#include <stack>
#include <array>
#include <string>
#include <algorithm>
#include <numeric>
#include <memory>
#include <chrono>

// Include the STL showcase headers
#include "Containers.hpp"
#include "Entity.hpp"
#include "Planet.hpp"
#include "Fleet.hpp"

using namespace CppVerseHub::STL;
using namespace CppVerseHub::Core;

/**
 * @brief Test fixture for STL container tests
 */
class ContainerTestFixture {
public:
    ContainerTestFixture() {
        setupTestData();
    }
    
    ~ContainerTestFixture() {
        testEntities.clear();
        testPlanets.clear();
        testFleets.clear();
    }
    
protected:
    void setupTestData() {
        // Create test entities
        for (int i = 0; i < 10; ++i) {
            auto planet = std::make_unique<Planet>(
                "Planet_" + std::to_string(i),
                Vector3D{i * 100.0, i * 100.0, i * 100.0}
            );
            planet->setResourceAmount(ResourceType::MINERALS, 1000 + i * 200);
            planet->setResourceAmount(ResourceType::ENERGY, 500 + i * 100);
            planet->setHabitabilityRating(0.3 + (i * 0.07));
            
            testPlanets.push_back(std::move(planet));
        }
        
        for (int i = 0; i < 5; ++i) {
            auto fleet = std::make_unique<Fleet>(
                "Fleet_" + std::to_string(i),
                Vector3D{i * 150.0, i * 150.0, i * 150.0}
            );
            fleet->addShips(ShipType::FIGHTER, 5 + i * 2);
            fleet->addShips(ShipType::CRUISER, 2 + i);
            if (i % 2 == 0) {
                fleet->addShips(ShipType::BATTLESHIP, 1);
            }
            
            testFleets.push_back(std::move(fleet));
        }
        
        // Setup test data collections
        testNumbers = {1, 5, 3, 9, 2, 8, 4, 7, 6};
        testStrings = {"alpha", "beta", "gamma", "delta", "epsilon", "zeta", "eta"};
        testPairs = {{1, "one"}, {3, "three"}, {2, "two"}, {5, "five"}, {4, "four"}};
    }
    
    std::vector<std::unique_ptr<Entity>> testEntities;
    std::vector<std::unique_ptr<Planet>> testPlanets;
    std::vector<std::unique_ptr<Fleet>> testFleets;
    std::vector<int> testNumbers;
    std::vector<std::string> testStrings;
    std::vector<std::pair<int, std::string>> testPairs;
};

TEST_CASE_METHOD(ContainerTestFixture, "Sequential Containers", "[containers][stl][sequential]") {
    
    SECTION("Vector Operations") {
        SECTION("Basic vector operations") {
            std::vector<int> vec = {1, 2, 3, 4, 5};
            
            // Size and capacity
            REQUIRE(vec.size() == 5);
            REQUIRE(vec.capacity() >= 5);
            
            // Element access
            REQUIRE(vec[0] == 1);
            REQUIRE(vec.at(4) == 5);
            REQUIRE(vec.front() == 1);
            REQUIRE(vec.back() == 5);
            
            // Modification
            vec.push_back(6);
            REQUIRE(vec.size() == 6);
            REQUIRE(vec.back() == 6);
            
            vec.pop_back();
            REQUIRE(vec.size() == 5);
            REQUIRE(vec.back() == 5);
            
            // Insertion and deletion
            vec.insert(vec.begin() + 2, 10);
            REQUIRE(vec[2] == 10);
            REQUIRE(vec.size() == 6);
            
            vec.erase(vec.begin() + 2);
            REQUIRE(vec[2] == 3);
            REQUIRE(vec.size() == 5);
        }
        
        SECTION("Vector with custom objects") {
            std::vector<Planet*> planetVec;
            
            for (auto& planet : testPlanets) {
                planetVec.push_back(planet.get());
            }
            
            REQUIRE(planetVec.size() == testPlanets.size());
            
            // Sort by habitability
            std::sort(planetVec.begin(), planetVec.end(), 
                     [](const Planet* a, const Planet* b) {
                         return a->getHabitabilityRating() < b->getHabitabilityRating();
                     });
            
            // Verify sorting
            for (size_t i = 1; i < planetVec.size(); ++i) {
                REQUIRE(planetVec[i-1]->getHabitabilityRating() <= 
                       planetVec[i]->getHabitabilityRating());
            }
            
            // Find planet with specific habitability
            auto it = std::find_if(planetVec.begin(), planetVec.end(),
                                  [](const Planet* p) {
                                      return p->getHabitabilityRating() > 0.7;
                                  });
            
            if (it != planetVec.end()) {
                REQUIRE((*it)->getHabitabilityRating() > 0.7);
            }
        }
        
        SECTION("Vector performance characteristics") {
            std::vector<int> perfVec;
            perfVec.reserve(10000); // Pre-allocate for performance
            
            auto duration = measurePerformance([&]() {
                for (int i = 0; i < 10000; ++i) {
                    perfVec.push_back(i);
                }
            }, "Vector push_back 10000 elements");
            
            REQUIRE(perfVec.size() == 10000);
            REQUIRE(duration.count() < 50000); // 50ms threshold
            
            // Random access performance
            auto accessDuration = measurePerformance([&]() {
                volatile int sum = 0;
                for (int i = 0; i < 10000; ++i) {
                    sum += perfVec[i % perfVec.size()];
                }
            }, "Vector random access 10000 times");
            
            REQUIRE(accessDuration.count() < 10000); // 10ms threshold
        }
    }
    
    SECTION("Deque Operations") {
        SECTION("Basic deque operations") {
            std::deque<std::string> deq = {"middle"};
            
            // Double-ended operations
            deq.push_front("front");
            deq.push_back("back");
            
            REQUIRE(deq.front() == "front");
            REQUIRE(deq[1] == "middle");
            REQUIRE(deq.back() == "back");
            REQUIRE(deq.size() == 3);
            
            deq.pop_front();
            deq.pop_back();
            
            REQUIRE(deq.size() == 1);
            REQUIRE(deq.front() == "middle");
        }
        
        SECTION("Deque as queue and stack") {
            std::deque<Fleet*> fleetDeque;
            
            // Use as queue (FIFO)
            for (auto& fleet : testFleets) {
                fleetDeque.push_back(fleet.get());
            }
            
            Fleet* firstFleet = fleetDeque.front();
            fleetDeque.pop_front();
            
            REQUIRE(firstFleet == testFleets[0].get());
            REQUIRE(fleetDeque.size() == testFleets.size() - 1);
            
            // Use as stack (LIFO)
            Fleet* lastFleet = fleetDeque.back();
            fleetDeque.pop_back();
            
            REQUIRE(lastFleet == testFleets[testFleets.size() - 1].get());
            REQUIRE(fleetDeque.size() == testFleets.size() - 2);
        }
        
        SECTION("Deque memory characteristics") {
            std::deque<int> memDeque;
            
            // Deques don't invalidate iterators on push/pop at ends
            memDeque = {1, 2, 3, 4, 5};
            auto it = memDeque.begin() + 2;
            int* ptr = &memDeque[2];
            
            memDeque.push_front(0);
            memDeque.push_back(6);
            
            // Iterator and pointer should still be valid
            REQUIRE(*it == 3);
            REQUIRE(*ptr == 3);
            
            // But not for insertion in middle
            memDeque.insert(memDeque.begin() + 3, 10);
            // it and ptr may now be invalid (implementation dependent)
        }
    }
    
    SECTION("List Operations") {
        SECTION("Basic list operations") {
            std::list<int> lst = {1, 3, 5, 7, 9};
            
            // Size
            REQUIRE(lst.size() == 5);
            
            // Front and back access
            REQUIRE(lst.front() == 1);
            REQUIRE(lst.back() == 9);
            
            // Insertion
            auto it = std::find(lst.begin(), lst.end(), 5);
            lst.insert(it, 4);
            
            // List should now be {1, 3, 4, 5, 7, 9}
            std::vector<int> expected = {1, 3, 4, 5, 7, 9};
            REQUIRE(std::equal(lst.begin(), lst.end(), expected.begin()));
        }
        
        SECTION("List-specific algorithms") {
            std::list<int> lst1 = {1, 3, 5, 7};
            std::list<int> lst2 = {2, 4, 6, 8};
            
            // Sort both lists
            lst1.sort();
            lst2.sort();
            
            // Merge sorted lists
            lst1.merge(lst2);
            
            REQUIRE(lst1.size() == 8);
            REQUIRE(lst2.empty());
            
            // Verify merged list is sorted
            std::vector<int> expectedMerged = {1, 2, 3, 4, 5, 6, 7, 8};
            REQUIRE(std::equal(lst1.begin(), lst1.end(), expectedMerged.begin()));
            
            // Remove duplicates
            lst1.push_back(5);
            lst1.push_back(7);
            lst1.sort();
            lst1.unique();
            
            REQUIRE(lst1.size() == 8); // No change, no duplicates originally
            
            // Remove specific values
            lst1.remove(4);
            REQUIRE(std::find(lst1.begin(), lst1.end(), 4) == lst1.end());
        }
        
        SECTION("List with game entities") {
            std::list<Planet*> planetList;
            
            // Add planets to list
            for (auto& planet : testPlanets) {
                planetList.push_back(planet.get());
            }
            
            // Remove planets with low habitability
            planetList.remove_if([](const Planet* p) {
                return p->getHabitabilityRating() < 0.5;
            });
            
            // All remaining planets should have habitability >= 0.5
            for (const auto& planet : planetList) {
                REQUIRE(planet->getHabitabilityRating() >= 0.5);
            }
            
            // Sort by mineral resources
            planetList.sort([](const Planet* a, const Planet* b) {
                return a->getResourceAmount(ResourceType::MINERALS) < 
                       b->getResourceAmount(ResourceType::MINERALS);
            });
            
            // Verify sorting
            auto prev = planetList.begin();
            for (auto it = std::next(prev); it != planetList.end(); ++it, ++prev) {
                REQUIRE((*prev)->getResourceAmount(ResourceType::MINERALS) <= 
                       (*it)->getResourceAmount(ResourceType::MINERALS));
            }
        }
    }
    
    SECTION("Array Operations") {
        SECTION("std::array basics") {
            std::array<int, 5> arr = {1, 2, 3, 4, 5};
            
            // Size is compile-time constant
            static_assert(arr.size() == 5);
            REQUIRE(arr.size() == 5);
            
            // Element access
            REQUIRE(arr[0] == 1);
            REQUIRE(arr.at(4) == 5);
            REQUIRE(arr.front() == 1);
            REQUIRE(arr.back() == 5);
            
            // Iteration
            int sum = 0;
            for (const auto& elem : arr) {
                sum += elem;
            }
            REQUIRE(sum == 15);
            
            // Fill and swap
            std::array<int, 5> arr2;
            arr2.fill(10);
            
            arr.swap(arr2);
            
            REQUIRE(arr[0] == 10);
            REQUIRE(arr2[0] == 1);
        }
        
        SECTION("Array of game objects") {
            std::array<Vector3D, 4> positions = {{
                {0, 0, 0},
                {100, 100, 100},
                {200, 200, 200},
                {300, 300, 300}
            }};
            
            // Calculate total distance
            double totalDistance = 0;
            for (size_t i = 1; i < positions.size(); ++i) {
                Vector3D diff = {
                    positions[i].x - positions[i-1].x,
                    positions[i].y - positions[i-1].y,
                    positions[i].z - positions[i-1].z
                };
                totalDistance += std::sqrt(diff.x*diff.x + diff.y*diff.y + diff.z*diff.z);
            }
            
            double expected = 3 * std::sqrt(3 * 100 * 100); // 3 * 100 * sqrt(3)
            REQUIRE(totalDistance == Approx(expected));
        }
    }
}

TEST_CASE_METHOD(ContainerTestFixture, "Associative Containers", "[containers][stl][associative]") {
    
    SECTION("Map Operations") {
        SECTION("Basic map operations") {
            std::map<int, std::string> resourceMap;
            
            // Insertion
            resourceMap[1] = "Minerals";
            resourceMap[2] = "Energy";
            resourceMap[3] = "Food";
            resourceMap.insert({4, "Water"});
            
            REQUIRE(resourceMap.size() == 4);
            REQUIRE(resourceMap[1] == "Minerals");
            REQUIRE(resourceMap.at(2) == "Energy");
            
            // Find
            auto it = resourceMap.find(3);
            REQUIRE(it != resourceMap.end());
            REQUIRE(it->second == "Food");
            
            // Erase
            resourceMap.erase(4);
            REQUIRE(resourceMap.find(4) == resourceMap.end());
            REQUIRE(resourceMap.size() == 3);
            
            // Iteration (sorted by key)
            std::vector<int> keys;
            for (const auto& pair : resourceMap) {
                keys.push_back(pair.first);
            }
            REQUIRE(std::is_sorted(keys.begin(), keys.end()));
        }
        
        SECTION("Map with custom key/value types") {
            std::map<std::string, Planet*> planetRegistry;
            
            // Register planets by name
            for (auto& planet : testPlanets) {
                planetRegistry[planet->getName()] = planet.get();
            }
            
            REQUIRE(planetRegistry.size() == testPlanets.size());
            
            // Find specific planet
            auto it = planetRegistry.find("Planet_5");
            REQUIRE(it != planetRegistry.end());
            REQUIRE(it->second == testPlanets[5].get());
            
            // Range queries
            auto range = planetRegistry.equal_range("Planet_3");
            int count = std::distance(range.first, range.second);
            REQUIRE(count == 1);
            
            // Find planets with prefix
            auto lower = planetRegistry.lower_bound("Planet_2");
            auto upper = planetRegistry.upper_bound("Planet_7");
            
            std::vector<Planet*> planetsInRange;
            for (auto it = lower; it != upper; ++it) {
                planetsInRange.push_back(it->second);
            }
            
            REQUIRE(planetsInRange.size() >= 6); // Planet_2 through Planet_7
        }
        
        SECTION("Multi-map operations") {
            std::multimap<double, Fleet*> fleetsByPower;
            
            // Group fleets by combat power
            for (auto& fleet : testFleets) {
                double power = fleet->getCombatPower();
                fleetsByPower.insert({power, fleet.get()});
            }
            
            REQUIRE(fleetsByPower.size() == testFleets.size());
            
            // Find fleets with specific power
            if (!fleetsByPower.empty()) {
                double somePower = fleetsByPower.begin()->first;
                auto range = fleetsByPower.equal_range(somePower);
                
                int fleetsWithSamePower = std::distance(range.first, range.second);
                REQUIRE(fleetsWithSamePower >= 1);
            }
        }
    }
    
    SECTION("Unordered Map Operations") {
        SECTION("Basic unordered_map operations") {
            std::unordered_map<int, std::string> hashMap;
            
            // Insertion and access
            hashMap[1] = "One";
            hashMap[2] = "Two";
            hashMap[3] = "Three";
            
            REQUIRE(hashMap.size() == 3);
            REQUIRE(hashMap[2] == "Two");
            
            // Performance characteristics
            auto duration = measurePerformance([&]() {
                for (int i = 0; i < 10000; ++i) {
                    hashMap[i] = "Value" + std::to_string(i);
                }
            }, "Unordered map insertion of 10000 elements");
            
            REQUIRE(hashMap.size() >= 10003);
            REQUIRE(duration.count() < 100000); // Should be faster than ordered map
            
            // Lookup performance
            auto lookupDuration = measurePerformance([&]() {
                volatile int found = 0;
                for (int i = 0; i < 10000; ++i) {
                    if (hashMap.find(i) != hashMap.end()) {
                        found++;
                    }
                }
            }, "Unordered map lookup 10000 times");
            
            REQUIRE(lookupDuration.count() < 50000); // Fast O(1) average lookup
        }
        
        SECTION("Custom hash and equality") {
            struct Vector3DHash {
                std::size_t operator()(const Vector3D& v) const {
                    return std::hash<double>()(v.x) ^ 
                           (std::hash<double>()(v.y) << 1) ^ 
                           (std::hash<double>()(v.z) << 2);
                }
            };
            
            struct Vector3DEqual {
                bool operator()(const Vector3D& a, const Vector3D& b) const {
                    return std::abs(a.x - b.x) < 0.001 &&
                           std::abs(a.y - b.y) < 0.001 &&
                           std::abs(a.z - b.z) < 0.001;
                }
            };
            
            std::unordered_map<Vector3D, Planet*, Vector3DHash, Vector3DEqual> spatialIndex;
            
            // Index planets by position
            for (auto& planet : testPlanets) {
                spatialIndex[planet->getPosition()] = planet.get();
            }
            
            REQUIRE(spatialIndex.size() == testPlanets.size());
            
            // Find planet at specific position
            Vector3D searchPos{300, 300, 300};
            auto it = spatialIndex.find(searchPos);
            
            if (it != spatialIndex.end()) {
                REQUIRE(it->second->getName() == "Planet_3");
            }
        }
    }
    
    SECTION("Set Operations") {
        SECTION("Basic set operations") {
            std::set<int> numbers = {5, 2, 8, 1, 9, 3};
            
            // Sets automatically sort and eliminate duplicates
            REQUIRE(numbers.size() == 6);
            
            // Verify sorted order
            std::vector<int> sortedNumbers(numbers.begin(), numbers.end());
            std::vector<int> expected = {1, 2, 3, 5, 8, 9};
            REQUIRE(sortedNumbers == expected);
            
            // Set operations
            std::set<int> other = {3, 6, 9, 12};
            std::set<int> intersection, unionSet, difference;
            
            std::set_intersection(numbers.begin(), numbers.end(),
                                other.begin(), other.end(),
                                std::inserter(intersection, intersection.begin()));
            REQUIRE(intersection == std::set<int>{3, 9});
            
            std::set_union(numbers.begin(), numbers.end(),
                          other.begin(), other.end(),
                          std::inserter(unionSet, unionSet.begin()));
            REQUIRE(unionSet.size() == 9); // All unique elements
            
            std::set_difference(numbers.begin(), numbers.end(),
                               other.begin(), other.end(),
                               std::inserter(difference, difference.begin()));
            REQUIRE(difference == std::set<int>{1, 2, 5, 8});
        }
        
        SECTION("Set with custom comparator") {
            struct PlanetResourceComparator {
                bool operator()(const Planet* a, const Planet* b) const {
                    return a->getResourceAmount(ResourceType::MINERALS) < 
                           b->getResourceAmount(ResourceType::MINERALS);
                }
            };
            
            std::set<Planet*, PlanetResourceComparator> richestPlanets;
            
            for (auto& planet : testPlanets) {
                richestPlanets.insert(planet.get());
            }
            
            // Planets should be sorted by mineral resources
            auto prev = richestPlanets.begin();
            for (auto it = std::next(prev); it != richestPlanets.end(); ++it, ++prev) {
                REQUIRE((*prev)->getResourceAmount(ResourceType::MINERALS) <= 
                       (*it)->getResourceAmount(ResourceType::MINERALS));
            }
        }
    }
    
    SECTION("Unordered Set Operations") {
        SECTION("Basic unordered_set operations") {
            std::unordered_set<std::string> visited;
            
            // Track visited planets
            for (const auto& planet : testPlanets) {
                visited.insert(planet->getName());
            }
            
            REQUIRE(visited.size() == testPlanets.size());
            
            // Fast lookup
            REQUIRE(visited.find("Planet_5") != visited.end());
            REQUIRE(visited.find("Planet_99") == visited.end());
            
            // Erase
            visited.erase("Planet_0");
            REQUIRE(visited.find("Planet_0") == visited.end());
            REQUIRE(visited.size() == testPlanets.size() - 1);
        }
        
        SECTION("Performance comparison") {
            const int elementCount = 10000;
            
            std::set<int> orderedSet;
            std::unordered_set<int> unorderedSet;
            
            // Insertion performance
            auto orderedDuration = measurePerformance([&]() {
                for (int i = 0; i < elementCount; ++i) {
                    orderedSet.insert(i);
                }
            }, "Ordered set insertion");
            
            auto unorderedDuration = measurePerformance([&]() {
                for (int i = 0; i < elementCount; ++i) {
                    unorderedSet.insert(i);
                }
            }, "Unordered set insertion");
            
            // Unordered set should generally be faster for insertion
            INFO("Ordered set insertion: " << orderedDuration.count() << "μs");
            INFO("Unordered set insertion: " << unorderedDuration.count() << "μs");
            
            // Lookup performance
            auto orderedLookup = measurePerformance([&]() {
                volatile int found = 0;
                for (int i = 0; i < elementCount; ++i) {
                    if (orderedSet.find(i) != orderedSet.end()) found++;
                }
            }, "Ordered set lookup");
            
            auto unorderedLookup = measurePerformance([&]() {
                volatile int found = 0;
                for (int i = 0; i < elementCount; ++i) {
                    if (unorderedSet.find(i) != unorderedSet.end()) found++;
                }
            }, "Unordered set lookup");
            
            // Unordered set should be much faster for lookup
            INFO("Ordered set lookup: " << orderedLookup.count() << "μs");
            INFO("Unordered set lookup: " << unorderedLookup.count() << "μs");
        }
    }
}

TEST_CASE_METHOD(ContainerTestFixture, "Container Adapters", "[containers][stl][adapters]") {
    
    SECTION("Stack Operations") {
        SECTION("Basic stack operations") {
            std::stack<int> stack;
            
            // Push elements
            for (int i = 1; i <= 5; ++i) {
                stack.push(i);
            }
            
            REQUIRE(stack.size() == 5);
            REQUIRE(stack.top() == 5);
            
            // Pop elements (LIFO)
            std::vector<int> popped;
            while (!stack.empty()) {
                popped.push_back(stack.top());
                stack.pop();
            }
            
            std::vector<int> expected = {5, 4, 3, 2, 1};
            REQUIRE(popped == expected);
        }
        
        SECTION("Stack for game state management") {
            std::stack<GameState> stateStack;
            
            // Push game states
            stateStack.push(GameState::MAIN_MENU);
            stateStack.push(GameState::PLAYING);
            stateStack.push(GameState::PAUSED);
            
            // Handle state transitions
            REQUIRE(stateStack.top() == GameState::PAUSED);
            
            // Resume game (pop pause state)
            stateStack.pop();
            REQUIRE(stateStack.top() == GameState::PLAYING);
            
            // Exit to menu
            stateStack.pop();
            REQUIRE(stateStack.top() == GameState::MAIN_MENU);
        }
    }
    
    SECTION("Queue Operations") {
        SECTION("Basic queue operations") {
            std::queue<std::string> queue;
            
            // Enqueue
            queue.push("First");
            queue.push("Second");
            queue.push("Third");
            
            REQUIRE(queue.size() == 3);
            REQUIRE(queue.front() == "First");
            REQUIRE(queue.back() == "Third");
            
            // Dequeue (FIFO)
            std::vector<std::string> dequeued;
            while (!queue.empty()) {
                dequeued.push_back(queue.front());
                queue.pop();
            }
            
            std::vector<std::string> expected = {"First", "Second", "Third"};
            REQUIRE(dequeued == expected);
        }
        
        SECTION("Task queue for mission management") {
            std::queue<Mission*> missionQueue;
            
            // Add missions to queue (this would need actual mission objects)
            // For testing, we'll simulate with mission IDs
            std::queue<int> missionIds;
            
            missionIds.push(1); // Exploration
            missionIds.push(2); // Combat
            missionIds.push(3); // Colonization
            
            // Process missions in order
            std::vector<int> processedMissions;
            while (!missionIds.empty()) {
                int missionId = missionIds.front();
                missionIds.pop();
                
                // Simulate mission processing
                processedMissions.push_back(missionId);
            }
            
            std::vector<int> expected = {1, 2, 3};
            REQUIRE(processedMissions == expected);
        }
    }
    
    SECTION("Priority Queue Operations") {
        SECTION("Basic priority queue operations") {
            std::priority_queue<int> pq;
            
            // Insert elements
            pq.push(3);
            pq.push(1);
            pq.push(4);
            pq.push(1);
            pq.push(5);
            pq.push(9);
            
            REQUIRE(pq.size() == 6);
            REQUIRE(pq.top() == 9); // Highest priority (max heap by default)
            
            // Extract in priority order
            std::vector<int> extracted;
            while (!pq.empty()) {
                extracted.push_back(pq.top());
                pq.pop();
            }
            
            // Should be in descending order
            REQUIRE(std::is_sorted(extracted.rbegin(), extracted.rend()));
        }
        
        SECTION("Priority queue with custom comparator") {
            struct FleetPriorityComparator {
                bool operator()(const Fleet* a, const Fleet* b) const {
                    // Lower combat power = higher priority (min heap behavior)
                    return a->getCombatPower() > b->getCombatPower();
                }
            };
            
            std::priority_queue<Fleet*, std::vector<Fleet*>, FleetPriorityComparator> fleetQueue;
            
            // Add fleets to priority queue
            for (auto& fleet : testFleets) {
                fleetQueue.push(fleet.get());
            }
            
            REQUIRE(fleetQueue.size() == testFleets.size());
            
            // Extract fleets in priority order (weakest first)
            std::vector<Fleet*> priorityOrder;
            while (!fleetQueue.empty()) {
                priorityOrder.push_back(fleetQueue.top());
                fleetQueue.pop();
            }
            
            // Verify priority order (weakest to strongest)
            for (size_t i = 1; i < priorityOrder.size(); ++i) {
                REQUIRE(priorityOrder[i-1]->getCombatPower() <= 
                       priorityOrder[i]->getCombatPower());
            }
        }
        
        SECTION("Emergency response priority queue") {
            struct EmergencyEvent {
                std::string type;
                int severity; // 1-10, higher is more urgent
                double timestamp;
                
                bool operator<(const EmergencyEvent& other) const {
                    // Higher severity = higher priority
                    return severity < other.severity;
                }
            };
            
            std::priority_queue<EmergencyEvent> emergencyQueue;
            
            // Add various emergency events
            emergencyQueue.push({"PlanetAttack", 8, 1.0});
            emergencyQueue.push({"ResourceShortage", 3, 2.0});
            emergencyQueue.push({"FleetDestroyed", 9, 3.0});
            emergencyQueue.push({"ColonyLost", 7, 4.0});
            emergencyQueue.push({"SystemFailure", 5, 5.0});
            
            // Handle emergencies by priority
            std::vector<std::string> handledEvents;
            std::vector<int> severities;
            
            while (!emergencyQueue.empty()) {
                EmergencyEvent event = emergencyQueue.top();
                emergencyQueue.pop();
                
                handledEvents.push_back(event.type);
                severities.push_back(event.severity);
            }
            
            // Should be handled in order of severity
            REQUIRE(handledEvents[0] == "FleetDestroyed"); // severity 9
            REQUIRE(handledEvents[1] == "PlanetAttack");   // severity 8
            REQUIRE(handledEvents[2] == "ColonyLost");     // severity 7
            REQUIRE(severities == std::vector<int>{9, 8, 7, 5, 3});
        }
    }
}

TEST_CASE_METHOD(ContainerTestFixture, "Container Performance Comparisons", "[containers][stl][performance]") {
    
    SECTION("Insertion Performance") {
        const int elementCount = 50000;
        
        SECTION("Sequential insertion") {
            std::vector<int> vec;
            std::deque<int> deq;
            std::list<int> lst;
            
            auto vecDuration = measurePerformance([&]() {
                for (int i = 0; i < elementCount; ++i) {
                    vec.push_back(i);
                }
            }, "Vector sequential insertion");
            
            auto deqDuration = measurePerformance([&]() {
                for (int i = 0; i < elementCount; ++i) {
                    deq.push_back(i);
                }
            }, "Deque sequential insertion");
            
            auto lstDuration = measurePerformance([&]() {
                for (int i = 0; i < elementCount; ++i) {
                    lst.push_back(i);
                }
            }, "List sequential insertion");
            
            REQUIRE(vec.size() == elementCount);
            REQUIRE(deq.size() == elementCount);
            REQUIRE(lst.size() == elementCount);
            
            INFO("Vector: " << vecDuration.count() << "μs");
            INFO("Deque: " << deqDuration.count() << "μs");
            INFO("List: " << lstDuration.count() << "μs");
        }
        
        SECTION("Front insertion") {
            const int frontInsertCount = 10000; // Smaller for vector performance
            
            std::vector<int> vec;
            std::deque<int> deq;
            std::list<int> lst;
            
            auto vecDuration = measurePerformance([&]() {
                for (int i = 0; i < frontInsertCount; ++i) {
                    vec.insert(vec.begin(), i);
                }
            }, "Vector front insertion");
            
            auto deqDuration = measurePerformance([&]() {
                for (int i = 0; i < frontInsertCount; ++i) {
                    deq.push_front(i);
                }
            }, "Deque front insertion");
            
            auto lstDuration = measurePerformance([&]() {
                for (int i = 0; i < frontInsertCount; ++i) {
                    lst.push_front(i);
                }
            }, "List front insertion");
            
            // Deque and list should be much faster than vector for front insertion
            REQUIRE(deqDuration.count() < vecDuration.count());
            REQUIRE(lstDuration.count() < vecDuration.count());
        }
    }
    
    SECTION("Access Performance") {
        const int accessCount = 100000;
        
        // Setup containers with data
        std::vector<int> vec(accessCount);
        std::deque<int> deq(accessCount);
        std::list<int> lst;
        
        std::iota(vec.begin(), vec.end(), 0);
        std::iota(deq.begin(), deq.end(), 0);
        for (int i = 0; i < accessCount; ++i) {
            lst.push_back(i);
        }
        
        SECTION("Random access") {
            auto vecDuration = measurePerformance([&]() {
                volatile int sum = 0;
                for (int i = 0; i < accessCount; ++i) {
                    sum += vec[i % vec.size()];
                }
            }, "Vector random access");
            
            auto deqDuration = measurePerformance([&]() {
                volatile int sum = 0;
                for (int i = 0; i < accessCount; ++i) {
                    sum += deq[i % deq.size()];
                }
            }, "Deque random access");
            
            // Vector should be fastest for random access
            REQUIRE(vecDuration.count() < deqDuration.count() * 2);
            
            INFO("Vector random access: " << vecDuration.count() << "μs");
            INFO("Deque random access: " << deqDuration.count() << "μs");
        }
        
        SECTION("Sequential access") {
            auto vecDuration = measurePerformance([&]() {
                volatile int sum = 0;
                for (const auto& elem : vec) {
                    sum += elem;
                }
            }, "Vector sequential access");
            
            auto deqDuration = measurePerformance([&]() {
                volatile int sum = 0;
                for (const auto& elem : deq) {
                    sum += elem;
                }
            }, "Deque sequential access");
            
            auto lstDuration = measurePerformance([&]() {
                volatile int sum = 0;
                for (const auto& elem : lst) {
                    sum += elem;
                }
            }, "List sequential access");
            
            // All should be relatively fast for sequential access
            INFO("Vector sequential: " << vecDuration.count() << "μs");
            INFO("Deque sequential: " << deqDuration.count() << "μs");
            INFO("List sequential: " << lstDuration.count() << "μs");
        }
    }
    
    SECTION("Search Performance") {
        const int searchSize = 10000;
        
        std::vector<int> sortedVec(searchSize);
        std::set<int> orderedSet;
        std::unordered_set<int> hashSet;
        
        std::iota(sortedVec.begin(), sortedVec.end(), 0);
        for (int i = 0; i < searchSize; ++i) {
            orderedSet.insert(i);
            hashSet.insert(i);
        }
        
        SECTION("Linear search vs binary search") {
            const int searchCount = 1000;
            
            auto linearDuration = measurePerformance([&]() {
                volatile int found = 0;
                for (int i = 0; i < searchCount; ++i) {
                    int target = i % searchSize;
                    if (std::find(sortedVec.begin(), sortedVec.end(), target) != sortedVec.end()) {
                        found++;
                    }
                }
            }, "Linear search in vector");
            
            auto binaryDuration = measurePerformance([&]() {
                volatile int found = 0;
                for (int i = 0; i < searchCount; ++i) {
                    int target = i % searchSize;
                    if (std::binary_search(sortedVec.begin(), sortedVec.end(), target)) {
                        found++;
                    }
                }
            }, "Binary search in vector");
            
            // Binary search should be much faster
            REQUIRE(binaryDuration.count() < linearDuration.count());
        }
        
        SECTION("Set vs unordered_set search") {
            const int searchCount = 10000;
            
            auto setDuration = measurePerformance([&]() {
                volatile int found = 0;
                for (int i = 0; i < searchCount; ++i) {
                    int target = i % searchSize;
                    if (orderedSet.find(target) != orderedSet.end()) {
                        found++;
                    }
                }
            }, "Search in ordered set");
            
            auto hashDuration = measurePerformance([&]() {
                volatile int found = 0;
                for (int i = 0; i < searchCount; ++i) {
                    int target = i % searchSize;
                    if (hashSet.find(target) != hashSet.end()) {
                        found++;
                    }
                }
            }, "Search in unordered set");
            
            // Hash set should generally be faster
            INFO("Ordered set search: " << setDuration.count() << "μs");
            INFO("Unordered set search: " << hashDuration.count() << "μs");
        }
    }
}

TEST_CASE_METHOD(ContainerTestFixture, "Container Memory Characteristics", "[containers][stl][memory]") {
    
    SECTION("Memory Layout and Cache Performance") {
        const int elementCount = 100000;
        
        SECTION("Contiguous vs non-contiguous memory") {
            std::vector<int> vec(elementCount);
            std::list<int> lst;
            
            // Fill containers
            for (int i = 0; i < elementCount; ++i) {
                vec[i] = i;
                lst.push_back(i);
            }
            
            // Test cache-friendly access (vector)
            auto vecDuration = measurePerformance([&]() {
                volatile long long sum = 0;
                for (const auto& elem : vec) {
                    sum += elem;
                }
            }, "Vector cache-friendly traversal");
            
            // Test cache-unfriendly access (list)
            auto lstDuration = measurePerformance([&]() {
                volatile long long sum = 0;
                for (const auto& elem : lst) {
                    sum += elem;
                }
            }, "List traversal");
            
            // Vector should be faster due to better cache locality
            REQUIRE(vecDuration.count() < lstDuration.count());
            
            INFO("Vector traversal: " << vecDuration.count() << "μs");
            INFO("List traversal: " << lstDuration.count() << "μs");
        }
    }
    
    SECTION("Memory Usage Monitoring") {
        MemoryTracker::printMemoryStats("Before container memory test");
        
        {
            std::vector<std::vector<int>> nestedVectors;
            
            // Create nested structure
            for (int i = 0; i < 1000; ++i) {
                nestedVectors.emplace_back(1000, i);
            }
            
            MemoryTracker::printMemoryStats("After creating nested vectors");
            
            // Test container of containers
            std::map<int, std::vector<Planet*>> planetGroups;
            
            for (int group = 0; group < 10; ++group) {
                for (auto& planet : testPlanets) {
                    planetGroups[group].push_back(planet.get());
                }
            }
            
            MemoryTracker::printMemoryStats("After creating planet groups");
            
        } // Containers destroyed here
        
        MemoryTracker::printMemoryStats("After container cleanup");
    }
    
    SECTION("Iterator Invalidation") {
        SECTION("Vector iterator invalidation") {
            std::vector<int> vec = {1, 2, 3, 4, 5};
            auto it = vec.begin() + 2;
            int* ptr = &vec[2];
            
            REQUIRE(*it == 3);
            REQUIRE(*ptr == 3);
            
            // Operations that don't invalidate iterators
            vec[0] = 10;
            REQUIRE(*it == 3); // Still valid
            REQUIRE(*ptr == 3); // Still valid
            
            // Capacity increase invalidates iterators
            vec.reserve(vec.capacity() + 1); // Force reallocation
            // it and ptr may now be invalid
            
            // Safe approach: get new iterators after potential reallocation
            auto newIt = vec.begin() + 2;
            REQUIRE(*newIt == 3);
        }
        
        SECTION("List iterator stability") {
            std::list<int> lst = {1, 2, 3, 4, 5};
            auto it = std::next(lst.begin(), 2);
            
            REQUIRE(*it == 3);
            
            // List iterators remain valid after insertion/deletion elsewhere
            lst.push_front(0);
            lst.push_back(6);
            lst.insert(lst.begin(), -1);
            
            REQUIRE(*it == 3); // Iterator still valid
            
            // Only invalidated when the element itself is erased
            auto eraseIt = lst.begin();
            lst.erase(eraseIt); // Erase first element
            
            REQUIRE(*it == 3); // Still valid, we didn't erase element 3
        }
        
        SECTION("Map iterator stability") {
            std::map<int, std::string> map = {{1, "one"}, {2, "two"}, {3, "three"}};
            auto it = map.find(2);
            
            REQUIRE(it->second == "two");
            
            // Map iterators remain valid after insertion/deletion of other elements
            map[4] = "four";
            map[0] = "zero";
            map.erase(1);
            
            REQUIRE(it->second == "two"); // Iterator still valid
            
            // Only invalidated when the specific element is erased
            map.erase(it);
            // it is now invalid
        }
    }
}

TEST_CASE_METHOD(ContainerTestFixture, "Advanced Container Usage Patterns", "[containers][stl][patterns]") {
    
    SECTION("Container Composition Patterns") {
        SECTION("Adjacency list using containers") {
            // Represent a graph of connected star systems
            std::map<std::string, std::vector<std::pair<std::string, double>>> starSystemGraph;
            
            // Add connections (system name -> {connected system, distance})
            starSystemGraph["Sol"] = {{"Alpha Centauri", 4.3}, {"Sirius", 8.6}};
            starSystemGraph["Alpha Centauri"] = {{"Sol", 4.3}, {"Proxima Centauri", 0.2}};
            starSystemGraph["Sirius"] = {{"Sol", 8.6}, {"Vega", 16.2}};
            
            REQUIRE(starSystemGraph["Sol"].size() == 2);
            
            // Find shortest path using BFS (simplified)
            auto findConnections = [&](const std::string& from) {
                std::vector<std::string> connections;
                if (starSystemGraph.find(from) != starSystemGraph.end()) {
                    for (const auto& connection : starSystemGraph[from]) {
                        connections.push_back(connection.first);
                    }
                }
                return connections;
            };
            
            auto solConnections = findConnections("Sol");
            REQUIRE(std::find(solConnections.begin(), solConnections.end(), "Alpha Centauri") != solConnections.end());
            REQUIRE(std::find(solConnections.begin(), solConnections.end(), "Sirius") != solConnections.end());
        }
        
        SECTION("Multi-index container simulation") {
            // Simulate multi-index container behavior with multiple maps
            struct FleetRecord {
                int id;
                std::string name;
                double combatPower;
                Vector3D position;
            };
            
            std::vector<FleetRecord> fleetData;
            std::map<int, size_t> idIndex;           // ID -> index in vector
            std::map<std::string, size_t> nameIndex; // Name -> index in vector
            std::multimap<double, size_t> powerIndex; // Combat power -> index
            
            // Add fleet data
            for (int i = 0; i < 5; ++i) {
                FleetRecord record{
                    i,
                    "Fleet_" + std::to_string(i),
                    100.0 + i * 50.0,
                    {i * 100.0, i * 100.0, 0.0}
                };
                
                size_t index = fleetData.size();
                fleetData.push_back(record);
                
                idIndex[record.id] = index;
                nameIndex[record.name] = index;
                powerIndex.insert({record.combatPower, index});
            }
            
            // Query by ID
            auto idIt = idIndex.find(3);
            REQUIRE(idIt != idIndex.end());
            REQUIRE(fleetData[idIt->second].name == "Fleet_3");
            
            // Query by name
            auto nameIt = nameIndex.find("Fleet_2");
            REQUIRE(nameIt != nameIndex.end());
            REQUIRE(fleetData[nameIt->second].id == 2);
            
            // Query by power range
            auto powerRange = powerIndex.equal_range(200.0);
            int fleetsWithPower200 = std::distance(powerRange.first, powerRange.second);
            REQUIRE(fleetsWithPower200 == 1);
        }
    }
    
    SECTION("Container Algorithms Integration") {
        SECTION("Complex sorting and filtering") {
            // Create a comprehensive planet database
            std::vector<Planet*> planetDatabase;
            for (auto& planet : testPlanets) {
                planetDatabase.push_back(planet.get());
            }
            
            // Multi-criteria sorting
            std::sort(planetDatabase.begin(), planetDatabase.end(), 
                     [](const Planet* a, const Planet* b) {
                         // Primary: habitability (descending)
                         if (std::abs(a->getHabitabilityRating() - b->getHabitabilityRating()) > 0.01) {
                             return a->getHabitabilityRating() > b->getHabitabilityRating();
                         }
                         // Secondary: mineral resources (descending)
                         return a->getResourceAmount(ResourceType::MINERALS) > 
                                b->getResourceAmount(ResourceType::MINERALS);
                     });
            
            // Verify primary sort criterion
            for (size_t i = 1; i < planetDatabase.size(); ++i) {
                double prev = planetDatabase[i-1]->getHabitabilityRating();
                double curr = planetDatabase[i]->getHabitabilityRating();
                
                if (std::abs(prev - curr) > 0.01) {
                    REQUIRE(prev >= curr);
                }
            }
            
            // Partition into habitable and non-habitable
            auto partitionIt = std::partition(planetDatabase.begin(), planetDatabase.end(),
                                            [](const Planet* p) {
                                                return p->getHabitabilityRating() >= 0.6;
                                            });
            
            size_t habitableCount = std::distance(planetDatabase.begin(), partitionIt);
            size_t totalCount = planetDatabase.size();
            
            INFO("Habitable planets: " << habitableCount << " out of " << totalCount);
            
            // All planets before partition should be habitable
            for (auto it = planetDatabase.begin(); it != partitionIt; ++it) {
                REQUIRE((*it)->getHabitabilityRating() >= 0.6);
            }
        }
        
        SECTION("Set operations on game collections") {
            // Create sets of planets based on different criteria
            std::set<Planet*> highHabitability;
            std::set<Planet*> richInMinerals;
            std::set<Planet*> richInEnergy;
            
            for (auto& planet : testPlanets) {
                if (planet->getHabitabilityRating() > 0.7) {
                    highHabitability.insert(planet.get());
                }
                if (planet->getResourceAmount(ResourceType::MINERALS) > 1500) {
                    richInMinerals.insert(planet.get());
                }
                if (planet->getResourceAmount(ResourceType::ENERGY) > 700) {
                    richInEnergy.insert(planet.get());
                }
            }
            
            // Find planets that are both habitable and resource-rich
            std::set<Planet*> idealPlanets;
            std::set_intersection(highHabitability.begin(), highHabitability.end(),
                                richInMinerals.begin(), richInMinerals.end(),
                                std::inserter(idealPlanets, idealPlanets.begin()));
            
            // All planets in idealPlanets should meet both criteria
            for (const auto& planet : idealPlanets) {
                REQUIRE(planet->getHabitabilityRating() > 0.7);
                REQUIRE(planet->getResourceAmount(ResourceType::MINERALS) > 1500);
            }
            
            // Find planets that have either high minerals OR high energy
            std::set<Planet*> resourceRich;
            std::set_union(richInMinerals.begin(), richInMinerals.end(),
                          richInEnergy.begin(), richInEnergy.end(),
                          std::inserter(resourceRich, resourceRich.begin()));
            
            // Should include all planets from both sets
            REQUIRE(resourceRich.size() >= richInMinerals.size());
            REQUIRE(resourceRich.size() >= richInEnergy.size());
        }
    }
    
    SECTION("Container Exception Safety") {
        SECTION("Exception safety during operations") {
            std::vector<std::unique_ptr<Planet>> exceptionTestVec;
            
            try {
                // Add some planets
                for (int i = 0; i < 5; ++i) {
                    exceptionTestVec.push_back(
                        std::make_unique<Planet>("ExceptionPlanet_" + std::to_string(i),
                                                Vector3D{i, i, i})
                    );
                }
                
                size_t sizeBeforeException = exceptionTestVec.size();
                
                // Simulate an operation that might throw
                try {
                    exceptionTestVec.push_back(nullptr); // This might cause issues later
                    
                    // Try to access elements - should not crash even with nullptr
                    for (const auto& planet : exceptionTestVec) {
                        if (planet) { // Safe check
                            INFO("Planet: " << planet->getName());
                        }
                    }
                } catch (const std::exception& e) {
                    // Vector should still be in valid state
                    REQUIRE(exceptionTestVec.size() >= sizeBeforeException);
                }
                
            } catch (...) {
                // Should not reach here with proper exception handling
                FAIL("Unexpected exception in container operations");
            }
        }
        
        SECTION("Strong exception safety guarantee") {
            std::vector<int> safeVec = {1, 2, 3, 4, 5};
            std::vector<int> backup = safeVec;
            
            try {
                // Operation that might throw
                safeVec.resize(1000000000); // Might throw std::bad_alloc
                
            } catch (const std::bad_alloc&) {
                // Vector should be unchanged if resize failed
                REQUIRE(safeVec == backup);
            } catch (...) {
                // Other exceptions - vector might be in modified but valid state
                REQUIRE(safeVec.size() >= 0); // Still valid
            }
        }
    }
}