// File: tests/unit_tests/stl_tests/IteratorTests.cpp
// Custom iterator tests for CppVerseHub STL showcase functionality

#include <catch2/catch.hpp>
#include <vector>
#include <list>
#include <iterator>
#include <algorithm>
#include <memory>
#include <type_traits>
#include <iostream>
#include <sstream>

// Include the STL showcase headers
#include "Iterators.hpp"
#include "Planet.hpp"
#include "Fleet.hpp"
#include "Entity.hpp"

using namespace CppVerseHub::STL;
using namespace CppVerseHub::Core;

/**
 * @brief Test fixture for iterator tests
 */
class IteratorTestFixture {
public:
    IteratorTestFixture() {
        setupTestData();
    }
    
    ~IteratorTestFixture() {
        testPlanets.clear();
        testFleets.clear();
    }
    
protected:
    void setupTestData() {
        // Create test planets
        for (int i = 0; i < 10; ++i) {
            auto planet = std::make_unique<Planet>(
                "Planet_" + std::to_string(i),
                Vector3D{i * 100.0, i * 100.0, i * 100.0}
            );
            
            planet->setResourceAmount(ResourceType::MINERALS, 500 + i * 100);
            planet->setResourceAmount(ResourceType::ENERGY, 300 + i * 50);
            planet->setHabitabilityRating(0.3 + i * 0.07);
            
            testPlanets.push_back(std::move(planet));
        }
        
        // Create test fleets
        for (int i = 0; i < 5; ++i) {
            auto fleet = std::make_unique<Fleet>(
                "Fleet_" + std::to_string(i),
                Vector3D{i * 150.0, i * 150.0, i * 150.0}
            );
            
            fleet->addShips(ShipType::FIGHTER, 5 + i * 2);
            fleet->addShips(ShipType::CRUISER, 2 + i);
            
            testFleets.push_back(std::move(fleet));
        }
        
        // Setup test data
        testNumbers = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    }
    
    std::vector<std::unique_ptr<Planet>> testPlanets;
    std::vector<std::unique_ptr<Fleet>> testFleets;
    std::vector<int> testNumbers;
};

/**
 * @brief Custom iterator for filtering planets by habitability
 */
template<typename Iterator>
class HabitabilityFilterIterator {
private:
    Iterator current_;
    Iterator end_;
    double minHabitability_;
    
    void advance_to_valid() {
        while (current_ != end_ && (*current_)->getHabitabilityRating() < minHabitability_) {
            ++current_;
        }
    }
    
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = typename std::iterator_traits<Iterator>::value_type;
    using difference_type = typename std::iterator_traits<Iterator>::difference_type;
    using pointer = typename std::iterator_traits<Iterator>::pointer;
    using reference = typename std::iterator_traits<Iterator>::reference;
    
    HabitabilityFilterIterator(Iterator begin, Iterator end, double minHabitability)
        : current_(begin), end_(end), minHabitability_(minHabitability) {
        advance_to_valid();
    }
    
    reference operator*() const { return *current_; }
    pointer operator->() const { return current_.operator->(); }
    
    HabitabilityFilterIterator& operator++() {
        ++current_;
        advance_to_valid();
        return *this;
    }
    
    HabitabilityFilterIterator operator++(int) {
        auto temp = *this;
        ++(*this);
        return temp;
    }
    
    bool operator==(const HabitabilityFilterIterator& other) const {
        return current_ == other.current_;
    }
    
    bool operator!=(const HabitabilityFilterIterator& other) const {
        return !(*this == other);
    }
    
    Iterator base() const { return current_; }
};

/**
 * @brief Helper function to create habitability filter iterators
 */
template<typename Iterator>
auto make_habitability_filter_iterator(Iterator begin, Iterator end, double minHabitability) {
    return HabitabilityFilterIterator<Iterator>(begin, end, minHabitability);
}

/**
 * @brief Custom iterator for accessing planet resources in sequence
 */
class PlanetResourceIterator {
private:
    std::vector<std::unique_ptr<Planet>>::const_iterator planetIt_;
    std::vector<std::unique_ptr<Planet>>::const_iterator planetEnd_;
    ResourceType currentResource_;
    
    void advance_to_next_valid() {
        while (planetIt_ != planetEnd_) {
            switch (currentResource_) {
                case ResourceType::MINERALS:
                    currentResource_ = ResourceType::ENERGY;
                    return;
                case ResourceType::ENERGY:
                    currentResource_ = ResourceType::FOOD;
                    return;
                case ResourceType::FOOD:
                    ++planetIt_;
                    currentResource_ = ResourceType::MINERALS;
                    break;
            }
        }
    }
    
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = std::pair<const Planet*, std::pair<ResourceType, int>>;
    using difference_type = std::ptrdiff_t;
    using pointer = value_type*;
    using reference = value_type;
    
    PlanetResourceIterator(std::vector<std::unique_ptr<Planet>>::const_iterator begin,
                          std::vector<std::unique_ptr<Planet>>::const_iterator end)
        : planetIt_(begin), planetEnd_(end), currentResource_(ResourceType::MINERALS) {}
    
    reference operator*() const {
        return std::make_pair(planetIt_->get(), 
                             std::make_pair(currentResource_, 
                                          (*planetIt_)->getResourceAmount(currentResource_)));
    }
    
    PlanetResourceIterator& operator++() {
        advance_to_next_valid();
        return *this;
    }
    
    PlanetResourceIterator operator++(int) {
        auto temp = *this;
        ++(*this);
        return temp;
    }
    
    bool operator==(const PlanetResourceIterator& other) const {
        return planetIt_ == other.planetIt_ && currentResource_ == other.currentResource_;
    }
    
    bool operator!=(const PlanetResourceIterator& other) const {
        return !(*this == other);
    }
};

/**
 * @brief Range class for planet resources
 */
class PlanetResourceRange {
private:
    const std::vector<std::unique_ptr<Planet>>& planets_;
    
public:
    PlanetResourceRange(const std::vector<std::unique_ptr<Planet>>& planets) : planets_(planets) {}
    
    PlanetResourceIterator begin() const {
        return PlanetResourceIterator(planets_.begin(), planets_.end());
    }
    
    PlanetResourceIterator end() const {
        return PlanetResourceIterator(planets_.end(), planets_.end());
    }
};

TEST_CASE_METHOD(IteratorTestFixture, "Standard Iterator Categories", "[iterators][stl][categories]") {
    
    SECTION("Input Iterator Operations") {
        SECTION("Basic input iterator usage") {
            std::istringstream iss("1 2 3 4 5");
            std::istream_iterator<int> input_it(iss);
            std::istream_iterator<int> input_end;
            
            std::vector<int> values;
            while (input_it != input_end) {
                values.push_back(*input_it);
                ++input_it;
            }
            
            std::vector<int> expected = {1, 2, 3, 4, 5};
            REQUIRE(values == expected);
        }
        
        SECTION("Input iterator with algorithms") {
            std::istringstream iss("10 20 30 40 50");
            std::istream_iterator<int> input_begin(iss);
            std::istream_iterator<int> input_end;
            
            // Use accumulate with input iterators
            int sum = std::accumulate(input_begin, input_end, 0);
            REQUIRE(sum == 150);
        }
    }
    
    SECTION("Output Iterator Operations") {
        SECTION("Basic output iterator usage") {
            std::ostringstream oss;
            std::ostream_iterator<int> output_it(oss, " ");
            
            for (int value : testNumbers) {
                *output_it = value;
                ++output_it;
            }
            
            std::string result = oss.str();
            REQUIRE(!result.empty());
            REQUIRE(result.find("1 2 3") != std::string::npos);
        }
        
        SECTION("Back insert iterator") {
            std::vector<Planet*> planetPtrs;
            std::back_insert_iterator<std::vector<Planet*>> back_it(planetPtrs);
            
            for (const auto& planet : testPlanets) {
                *back_it = planet.get();
                ++back_it;
            }
            
            REQUIRE(planetPtrs.size() == testPlanets.size());
            for (size_t i = 0; i < testPlanets.size(); ++i) {
                REQUIRE(planetPtrs[i] == testPlanets[i].get());
            }
        }
        
        SECTION("Insert iterator") {
            std::vector<int> numbers;
            std::insert_iterator<std::vector<int>> insert_it(numbers, numbers.begin());
            
            for (int value : testNumbers) {
                *insert_it = value;
                ++insert_it;
            }
            
            REQUIRE(numbers == testNumbers);
        }
    }
    
    SECTION("Forward Iterator Operations") {
        SECTION("Forward list iteration") {
            std::forward_list<int> flist(testNumbers.begin(), testNumbers.end());
            
            // Multi-pass iteration
            auto it1 = flist.begin();
            auto it2 = flist.begin();
            
            REQUIRE(*it1 == *it2);
            
            ++it1;
            REQUIRE(*it2 == testNumbers[0]); // it2 unchanged
            REQUIRE(*it1 == testNumbers[1]);
            
            // Forward iteration
            int count = 0;
            for (auto it = flist.begin(); it != flist.end(); ++it) {
                REQUIRE(*it == testNumbers[count]);
                ++count;
            }
            
            REQUIRE(count == static_cast<int>(testNumbers.size()));
        }
    }
    
    SECTION("Bidirectional Iterator Operations") {
        SECTION("List bidirectional iteration") {
            std::list<Planet*> planetList;
            for (const auto& planet : testPlanets) {
                planetList.push_back(planet.get());
            }
            
            // Forward iteration
            auto it = planetList.begin();
            int forwardCount = 0;
            while (it != planetList.end()) {
                REQUIRE(*it == testPlanets[forwardCount].get());
                ++it;
                ++forwardCount;
            }
            
            // Backward iteration
            --it; // Move to last valid element
            int backwardCount = forwardCount - 1;
            while (it != planetList.begin()) {
                REQUIRE(*it == testPlanets[backwardCount].get());
                --it;
                --backwardCount;
            }
            REQUIRE(*it == testPlanets[0].get()); // Check first element
            
            // Reverse iterator
            std::vector<Planet*> reversedPlanets;
            for (auto rit = planetList.rbegin(); rit != planetList.rend(); ++rit) {
                reversedPlanets.push_back(*rit);
            }
            
            // Should be in reverse order
            REQUIRE(reversedPlanets.size() == testPlanets.size());
            for (size_t i = 0; i < testPlanets.size(); ++i) {
                REQUIRE(reversedPlanets[i] == testPlanets[testPlanets.size() - 1 - i].get());
            }
        }
    }
    
    SECTION("Random Access Iterator Operations") {
        SECTION("Vector random access operations") {
            std::vector<Fleet*> fleetPtrs;
            for (const auto& fleet : testFleets) {
                fleetPtrs.push_back(fleet.get());
            }
            
            auto it = fleetPtrs.begin();
            
            // Random access
            REQUIRE(*(it + 2) == testFleets[2].get());
            REQUIRE(*(it + 4) == testFleets[4].get());
            
            // Arithmetic operations
            auto it2 = it + 3;
            auto it3 = it2 - 1;
            
            REQUIRE(*it2 == testFleets[3].get());
            REQUIRE(*it3 == testFleets[2].get());
            
            // Distance calculation
            ptrdiff_t distance = fleetPtrs.end() - fleetPtrs.begin();
            REQUIRE(distance == static_cast<ptrdiff_t>(testFleets.size()));
            
            // Comparison operators
            REQUIRE(it < it2);
            REQUIRE(it2 > it);
            REQUIRE(it <= it3);
            REQUIRE(it2 >= it3);
            
            // Subscript operator
            REQUIRE(it[1] == testFleets[1].get());
            REQUIRE(it[3] == testFleets[3].get());
        }
        
        SECTION("Iterator arithmetic performance") {
            std::vector<int> largeVector(100000);
            std::iota(largeVector.begin(), largeVector.end(), 0);
            
            auto it = largeVector.begin();
            
            // Random access should be O(1)
            auto duration = measurePerformance([&]() {
                volatile int sum = 0;
                for (int i = 0; i < 10000; ++i) {
                    sum += *(it + (i * 10) % largeVector.size());
                }
            }, "Random access iterator operations");
            
            // Should be very fast
            REQUIRE(duration.count() < 50000); // 50ms threshold
        }
    }
}

TEST_CASE_METHOD(IteratorTestFixture, "Iterator Adapters", "[iterators][stl][adapters]") {
    
    SECTION("Reverse Iterator") {
        SECTION("Basic reverse iteration") {
            std::vector<int> vec = testNumbers;
            
            std::vector<int> reversed;
            for (auto rit = vec.rbegin(); rit != vec.rend(); ++rit) {
                reversed.push_back(*rit);
            }
            
            // Should be reverse of original
            REQUIRE(reversed.size() == vec.size());
            for (size_t i = 0; i < vec.size(); ++i) {
                REQUIRE(reversed[i] == vec[vec.size() - 1 - i]);
            }
        }
        
        SECTION("Reverse iterator base") {
            std::vector<Planet*> planetPtrs;
            for (const auto& planet : testPlanets) {
                planetPtrs.push_back(planet.get());
            }
            
            auto rit = planetPtrs.rbegin() + 2;
            auto base_it = rit.base();
            
            // Base iterator should point to element after reverse iterator
            REQUIRE(*(rit) == *(base_it - 1));
        }
        
        SECTION("Algorithms with reverse iterators") {
            std::vector<int> numbers = testNumbers;
            
            // Sort in descending order using reverse iterators
            std::sort(numbers.rbegin(), numbers.rend());
            
            // Should be sorted in descending order
            for (size_t i = 1; i < numbers.size(); ++i) {
                REQUIRE(numbers[i-1] >= numbers[i]);
            }
        }
    }
    
    SECTION("Insert Iterators") {
        SECTION("Back insert iterator") {
            std::vector<std::string> planetNames;
            auto back_inserter_it = std::back_inserter(planetNames);
            
            for (const auto& planet : testPlanets) {
                *back_inserter_it = planet->getName();
                ++back_inserter_it;
            }
            
            REQUIRE(planetNames.size() == testPlanets.size());
            for (size_t i = 0; i < testPlanets.size(); ++i) {
                REQUIRE(planetNames[i] == testPlanets[i]->getName());
            }
        }
        
        SECTION("Front insert iterator") {
            std::deque<int> deq;
            auto front_inserter_it = std::front_inserter(deq);
            
            for (int value : testNumbers) {
                *front_inserter_it = value;
                ++front_inserter_it;
            }
            
            // Elements should be in reverse order due to front insertion
            REQUIRE(deq.size() == testNumbers.size());
            for (size_t i = 0; i < testNumbers.size(); ++i) {
                REQUIRE(deq[i] == testNumbers[testNumbers.size() - 1 - i]);
            }
        }
        
        SECTION("Insert iterator at specific position") {
            std::vector<int> vec = {1, 5};
            auto insert_it = std::inserter(vec, vec.begin() + 1);
            
            std::vector<int> toInsert = {2, 3, 4};
            for (int value : toInsert) {
                *insert_it = value;
                ++insert_it;
            }
            
            std::vector<int> expected = {1, 2, 3, 4, 5};
            REQUIRE(vec == expected);
        }
    }
    
    SECTION("Move Iterator") {
        SECTION("Moving unique_ptrs with move_iterator") {
            std::vector<std::unique_ptr<Planet>> sourcePlanets;
            
            // Create planets in source vector
            for (int i = 0; i < 3; ++i) {
                sourcePlanets.push_back(std::make_unique<Planet>(
                    "MovePlanet_" + std::to_string(i),
                    Vector3D{i, i, i}
                ));
            }
            
            // Move to destination using move_iterator
            std::vector<std::unique_ptr<Planet>> destPlanets;
            std::move(std::make_move_iterator(sourcePlanets.begin()),
                     std::make_move_iterator(sourcePlanets.end()),
                     std::back_inserter(destPlanets));
            
            // Source should be empty (moved from)
            for (const auto& planet : sourcePlanets) {
                REQUIRE(planet == nullptr);
            }
            
            // Destination should have the planets
            REQUIRE(destPlanets.size() == 3);
            for (size_t i = 0; i < destPlanets.size(); ++i) {
                REQUIRE(destPlanets[i] != nullptr);
                REQUIRE(destPlanets[i]->getName() == "MovePlanet_" + std::to_string(i));
            }
        }
    }
}

TEST_CASE_METHOD(IteratorTestFixture, "Custom Iterator Implementation", "[iterators][custom][implementation]") {
    
    SECTION("Filter Iterator") {
        SECTION("Habitability filter iterator") {
            double minHabitability = 0.6;
            
            auto filter_begin = make_habitability_filter_iterator(
                testPlanets.begin(), testPlanets.end(), minHabitability);
            auto filter_end = make_habitability_filter_iterator(
                testPlanets.end(), testPlanets.end(), minHabitability);
            
            std::vector<Planet*> filteredPlanets;
            for (auto it = filter_begin; it != filter_end; ++it) {
                filteredPlanets.push_back(it->get());
            }
            
            // All filtered planets should meet the criteria
            for (const Planet* planet : filteredPlanets) {
                REQUIRE(planet->getHabitabilityRating() >= minHabitability);
            }
            
            // Count should match manual filter
            int manualCount = std::count_if(testPlanets.begin(), testPlanets.end(),
                                          [minHabitability](const std::unique_ptr<Planet>& p) {
                                              return p->getHabitabilityRating() >= minHabitability;
                                          });
            
            REQUIRE(static_cast<int>(filteredPlanets.size()) == manualCount);
        }
        
        SECTION("Filter iterator with algorithms") {
            double minHabitability = 0.5;
            
            auto filter_begin = make_habitability_filter_iterator(
                testPlanets.begin(), testPlanets.end(), minHabitability);
            auto filter_end = make_habitability_filter_iterator(
                testPlanets.end(), testPlanets.end(), minHabitability);
            
            // Count filtered elements
            int count = std::distance(filter_begin, filter_end);
            REQUIRE(count >= 0);
            
            // Find maximum habitability among filtered planets
            if (filter_begin != filter_end) {
                auto maxIt = std::max_element(filter_begin, filter_end,
                                            [](const std::unique_ptr<Planet>& a, const std::unique_ptr<Planet>& b) {
                                                return a->getHabitabilityRating() < b->getHabitabilityRating();
                                            });
                
                REQUIRE(maxIt != filter_end);
                REQUIRE((*maxIt)->getHabitabilityRating() >= minHabitability);
            }
        }
    }
    
    SECTION("Resource Iterator") {
        SECTION("Planet resource iterator") {
            PlanetResourceRange resourceRange(testPlanets);
            
            std::vector<std::pair<const Planet*, std::pair<ResourceType, int>>> resources;
            for (const auto& resource : resourceRange) {
                resources.push_back(resource);
            }
            
            // Should have 3 resources per planet
            REQUIRE(resources.size() == testPlanets.size() * 3);
            
            // Verify resource structure
            for (size_t i = 0; i < testPlanets.size(); ++i) {
                const Planet* planet = testPlanets[i].get();
                
                // Check minerals resource
                auto mineralRes = resources[i * 3];
                REQUIRE(mineralRes.first == planet);
                REQUIRE(mineralRes.second.first == ResourceType::MINERALS);
                REQUIRE(mineralRes.second.second == planet->getResourceAmount(ResourceType::MINERALS));
                
                // Check energy resource
                auto energyRes = resources[i * 3 + 1];
                REQUIRE(energyRes.first == planet);
                REQUIRE(energyRes.second.first == ResourceType::ENERGY);
                REQUIRE(energyRes.second.second == planet->getResourceAmount(ResourceType::ENERGY));
                
                // Check food resource
                auto foodRes = resources[i * 3 + 2];
                REQUIRE(foodRes.first == planet);
                REQUIRE(foodRes.second.first == ResourceType::FOOD);
                REQUIRE(foodRes.second.second == planet->getResourceAmount(ResourceType::FOOD));
            }
        }
        
        SECTION("Resource iterator algorithms") {
            PlanetResourceRange resourceRange(testPlanets);
            
            // Count total resources
            int totalResourceCount = std::distance(resourceRange.begin(), resourceRange.end());
            REQUIRE(totalResourceCount == static_cast<int>(testPlanets.size() * 3));
            
            // Find maximum resource amount
            auto maxResourceIt = std::max_element(resourceRange.begin(), resourceRange.end(),
                                                [](const auto& a, const auto& b) {
                                                    return a.second.second < b.second.second;
                                                });
            
            if (maxResourceIt != resourceRange.end()) {
                REQUIRE(maxResourceIt->second.second >= 0);
                
                // Verify it's actually the maximum
                for (const auto& resource : resourceRange) {
                    REQUIRE(resource.second.second <= maxResourceIt->second.second);
                }
            }
            
            // Count mineral resources
            int mineralCount = std::count_if(resourceRange.begin(), resourceRange.end(),
                                           [](const auto& resource) {
                                               return resource.second.first == ResourceType::MINERALS;
                                           });
            
            REQUIRE(mineralCount == static_cast<int>(testPlanets.size()));
        }
    }
}

TEST_CASE_METHOD(IteratorTestFixture, "Iterator Traits and Concepts", "[iterators][traits][concepts]") {
    
    SECTION("Iterator Traits") {
        SECTION("Standard iterator traits") {
            using VecIterator = std::vector<int>::iterator;
            using ListIterator = std::list<int>::iterator;
            
            // Check iterator categories
            static_assert(std::is_same_v<
                std::iterator_traits<VecIterator>::iterator_category,
                std::random_access_iterator_tag
            >);
            
            static_assert(std::is_same_v<
                std::iterator_traits<ListIterator>::iterator_category,
                std::bidirectional_iterator_tag
            >);
            
            // Check value types
            static_assert(std::is_same_v<
                std::iterator_traits<VecIterator>::value_type,
                int
            >);
            
            // Check pointer types
            static_assert(std::is_same_v<
                std::iterator_traits<VecIterator>::pointer,
                int*
            >);
        }
        
        SECTION("Custom iterator traits") {
            using FilterIterator = HabitabilityFilterIterator<std::vector<std::unique_ptr<Planet>>::iterator>;
            
            // Check that our custom iterator has proper traits
            static_assert(std::is_same_v<
                std::iterator_traits<FilterIterator>::iterator_category,
                std::forward_iterator_tag
            >);
            
            static_assert(std::is_same_v<
                std::iterator_traits<FilterIterator>::value_type,
                std::unique_ptr<Planet>
            >);
        }
        
        SECTION("Iterator concept checking") {
            // Runtime checks for iterator concepts
            using VecIterator = std::vector<Planet*>::iterator;
            using ListIterator = std::list<Planet*>::iterator;
            
            std::vector<Planet*> vecTest;
            std::list<Planet*> listTest;
            
            // Test iterator requirements
            VecIterator vecIt = vecTest.begin();
            ListIterator listIt = listTest.begin();
            
            // All iterators should be copyable and assignable
            VecIterator vecIt2 = vecIt;
            ListIterator listIt2 = listIt;
            
            vecIt2 = vecIt;
            listIt2 = listIt;
            
            // Test dereferenceable (when not end iterator)
            if (!vecTest.empty()) {
                auto& ref = *vecIt;
                (void)ref; // Suppress unused variable warning
            }
        }
    }
    
    SECTION("SFINAE with Iterator Types") {
        SECTION("Template specialization based on iterator category") {
            // Helper templates to test iterator category detection
            template<typename Iterator>
            constexpr bool is_random_access_v = std::is_same_v<
                typename std::iterator_traits<Iterator>::iterator_category,
                std::random_access_iterator_tag
            >;
            
            template<typename Iterator>
            constexpr bool is_bidirectional_v = std::is_same_v<
                typename std::iterator_traits<Iterator>::iterator_category,
                std::bidirectional_iterator_tag
            >;
            
            // Test with different iterator types
            REQUIRE(is_random_access_v<std::vector<int>::iterator>);
            REQUIRE_FALSE(is_random_access_v<std::list<int>::iterator>);
            
            REQUIRE(is_bidirectional_v<std::list<int>::iterator>);
            REQUIRE_FALSE(is_bidirectional_v<std::forward_list<int>::iterator>);
        }
        
        SECTION("Algorithm optimization based on iterator category") {
            // Function that behaves differently based on iterator category
            auto optimized_distance = [](auto first, auto last) {
                using iterator_category = typename std::iterator_traits<decltype(first)>::iterator_category;
                
                if constexpr (std::is_same_v<iterator_category, std::random_access_iterator_tag>) {
                    // O(1) for random access iterators
                    return last - first;
                } else {
                    // O(n) for other iterator types
                    std::ptrdiff_t count = 0;
                    while (first != last) {
                        ++first;
                        ++count;
                    }
                    return count;
                }
            };
            
            std::vector<int> vec = testNumbers;
            std::list<int> lst(testNumbers.begin(), testNumbers.end());
            
            // Both should return same result
            auto vecDistance = optimized_distance(vec.begin(), vec.end());
            auto listDistance = optimized_distance(lst.begin(), lst.end());
            
            REQUIRE(vecDistance == static_cast<std::ptrdiff_t>(testNumbers.size()));
            REQUIRE(listDistance == static_cast<std::ptrdiff_t>(testNumbers.size()));
            REQUIRE(vecDistance == listDistance);
        }
    }
}

TEST_CASE_METHOD(IteratorTestFixture, "Iterator Performance and Optimization", "[iterators][performance][optimization]") {
    
    SECTION("Iterator vs Index Performance") {
        SECTION("Vector iteration comparison") {
            const size_t size = 1000000;
            std::vector<int> largeVec(size);
            std::iota(largeVec.begin(), largeVec.end(), 0);
            
            // Iterator-based sum
            auto iteratorDuration = measurePerformance([&]() {
                volatile long long sum = 0;
                for (auto it = largeVec.begin(); it != largeVec.end(); ++it) {
                    sum += *it;
                }
            }, "Iterator-based sum");
            
            // Index-based sum
            auto indexDuration = measurePerformance([&]() {
                volatile long long sum = 0;
                for (size_t i = 0; i < largeVec.size(); ++i) {
                    sum += largeVec[i];
                }
            }, "Index-based sum");
            
            // Range-based for loop
            auto rangeDuration = measurePerformance([&]() {
                volatile long long sum = 0;
                for (const auto& value : largeVec) {
                    sum += value;
                }
            }, "Range-based for loop sum");
            
            INFO("Iterator: " << iteratorDuration.count() << "μs");
            INFO("Index: " << indexDuration.count() << "μs");
            INFO("Range: " << rangeDuration.count() << "μs");
            
            // All should be reasonably fast and similar
            REQUIRE(iteratorDuration.count() < 100000); // 100ms
            REQUIRE(indexDuration.count() < 100000);
            REQUIRE(rangeDuration.count() < 100000);
        }
    }
    
    SECTION("Custom Iterator Performance") {
        SECTION("Filter iterator vs manual filtering") {
            double minHabitability = 0.5;
            
            // Custom filter iterator
            auto filterDuration = measurePerformance([&]() {
                auto filter_begin = make_habitability_filter_iterator(
                    testPlanets.begin(), testPlanets.end(), minHabitability);
                auto filter_end = make_habitability_filter_iterator(
                    testPlanets.end(), testPlanets.end(), minHabitability);
                
                volatile int count = 0;
                for (auto it = filter_begin; it != filter_end; ++it) {
                    count++;
                }
            }, "Custom filter iterator");
            
            // Manual filtering with copy_if
            auto manualDuration = measurePerformance([&]() {
                std::vector<Planet*> filtered;
                std::copy_if(testPlanets.begin(), testPlanets.end(), std::back_inserter(filtered),
                           [minHabitability](const std::unique_ptr<Planet>& p) {
                               return p->getHabitabilityRating() >= minHabitability;
                           });
                volatile size_t count = filtered.size();
            }, "Manual filtering with copy_if");
            
            INFO("Filter iterator: " << filterDuration.count() << "μs");
            INFO("Manual filtering: " << manualDuration.count() << "μs");
            
            // Both methods should be reasonably fast
            REQUIRE(filterDuration.count() < 10000); // 10ms
            REQUIRE(manualDuration.count() < 10000);
        }
    }
}

TEST_CASE_METHOD(IteratorTestFixture, "Iterator Safety and Error Handling", "[iterators][safety][errors]") {
    
    SECTION("Iterator Invalidation Awareness") {
        SECTION("Vector iterator invalidation") {
            std::vector<int> vec = {1, 2, 3, 4, 5};
            auto it = vec.begin() + 2;
            
            REQUIRE(*it == 3);
            
            // Non-reallocating operations should keep iterators valid
            vec[0] = 10;
            REQUIRE(*it == 3); // Still valid
            
            // Reallocating operations invalidate iterators
            size_t oldCapacity = vec.capacity();
            while (vec.capacity() == oldCapacity) {
                vec.push_back(0); // Force reallocation
            }
            // it is now potentially invalid - don't use it
            
            // Get new valid iterator
            auto newIt = vec.begin() + 2;
            REQUIRE(*newIt == 3); // Should still be 3
        }
        
        SECTION("List iterator stability") {
            std::list<Planet*> planetList;
            for (const auto& planet : testPlanets) {
                planetList.push_back(planet.get());
            }
            
            auto it = std::next(planetList.begin(), 2);
            Planet* savedPlanet = *it;
            
            // Insert at beginning should not invalidate other iterators
            planetList.push_front(nullptr);
            REQUIRE(*it == savedPlanet); // Should still be valid
            
            // Insert at end should not invalidate other iterators
            planetList.push_back(nullptr);
            REQUIRE(*it == savedPlanet); // Should still be valid
            
            // Only erasing the specific element invalidates its iterator
            planetList.erase(it);
            // it is now invalid - don't use it
        }
    }
    
    SECTION("Bounds Checking and Safety") {
        SECTION("Safe iterator advancement") {
            std::vector<int> vec = testNumbers;
            
            auto safeAdvance = [](auto it, auto end, std::ptrdiff_t n) {
                std::ptrdiff_t maxAdvance = std::distance(it, end);
                std::ptrdiff_t actualAdvance = std::min(n, maxAdvance);
                std::advance(it, actualAdvance);
                return it;
            };
            
            // Safe advancement within bounds
            auto it1 = safeAdvance(vec.begin(), vec.end(), 5);
            REQUIRE(it1 == vec.begin() + 5);
            
            // Safe advancement beyond bounds (should stop at end)
            auto it2 = safeAdvance(vec.begin(), vec.end(), 1000);
            REQUIRE(it2 == vec.end());
            
            // Safe advancement with negative values
            auto it3 = safeAdvance(vec.end(), vec.begin(), -3);
            REQUIRE(it3 == vec.end() - 3);
        }
        
        SECTION("Iterator range validation") {
            std::vector<Planet*> planetPtrs;
            for (const auto& planet : testPlanets) {
                planetPtrs.push_back(planet.get());
            }
            
            auto validateRange = [](auto begin, auto end) {
                // Check if range is valid
                if (std::distance(begin, end) < 0) {
                    throw std::invalid_argument("Invalid iterator range: end before begin");
                }
                return true;
            };
            
            // Valid range
            REQUIRE(validateRange(planetPtrs.begin(), planetPtrs.end()));
            
            // Invalid range
            REQUIRE_THROWS_AS(validateRange(planetPtrs.end(), planetPtrs.begin()), 
                             std::invalid_argument);
        }
    }
    
    SECTION("Exception Safety with Iterators") {
        SECTION("Exception during iteration") {
            std::vector<int> testData = {1, 2, 3, 4, 5, 0, 6, 7, 8}; // 0 will cause exception
            
            auto riskyOperation = [](int value) {
                if (value == 0) {
                    throw std::runtime_error("Division by zero");
                }
                return 10 / value;
            };
            
            std::vector<int> results;
            auto it = testData.begin();
            
            try {
                while (it != testData.end()) {
                    int result = riskyOperation(*it);
                    results.push_back(result);
                    ++it;
                }
                FAIL("Expected exception was not thrown");
            } catch (const std::runtime_error& e) {
                // Exception occurred, iterator should still be valid and pointing to problem element
                REQUIRE(*it == 0);
                
                // Should be able to continue after the problematic element
                ++it;
                while (it != testData.end()) {
                    if (*it != 0) {
                        results.push_back(riskyOperation(*it));
                    }
                    ++it;
                }
                
                // Should have processed elements before and after the exception
                REQUIRE(results.size() > 0);
            }
        }
    }
}

TEST_CASE_METHOD(IteratorTestFixture, "Advanced Iterator Patterns", "[iterators][advanced][patterns]") {
    
    SECTION("Iterator Composition") {
        SECTION("Chained iterator adapters") {
            // Create a chain: reverse -> filter -> transform
            std::vector<Planet*> planetPtrs;
            for (const auto& planet : testPlanets) {
                planetPtrs.push_back(planet.get());
            }
            
            // Reverse the planet order
            std::vector<Planet*> reversedPlanets(planetPtrs.rbegin(), planetPtrs.rend());
            
            // Filter for habitable planets
            std::vector<Planet*> habitablePlanets;
            std::copy_if(reversedPlanets.begin(), reversedPlanets.end(), 
                        std::back_inserter(habitablePlanets),
                        [](const Planet* p) { return p->getHabitabilityRating() > 0.5; });
            
            // Transform to planet names
            std::vector<std::string> planetNames;
            std::transform(habitablePlanets.begin(), habitablePlanets.end(),
                          std::back_inserter(planetNames),
                          [](const Planet* p) { return p->getName(); });
            
            // Verify the chain worked
            REQUIRE(!planetNames.empty());
            for (const std::string& name : planetNames) {
                REQUIRE(!name.empty());
                REQUIRE(name.find("Planet_") != std::string::npos);
            }
        }
    }
    
    SECTION("Lazy Evaluation with Iterators") {
        SECTION("Generator-like iterator") {
            class FibonacciIterator {
            private:
                int current_ = 0;
                int next_ = 1;
                int count_ = 0;
                int maxCount_;
                
            public:
                using iterator_category = std::input_iterator_tag;
                using value_type = int;
                using difference_type = std::ptrdiff_t;
                using pointer = const int*;
                using reference = const int&;
                
                explicit FibonacciIterator(int maxCount) : maxCount_(maxCount) {}
                FibonacciIterator() : maxCount_(0), count_(maxCount_) {} // End iterator
                
                const int& operator*() const { return current_; }
                const int* operator->() const { return &current_; }
                
                FibonacciIterator& operator++() {
                    if (count_ < maxCount_) {
                        int temp = current_ + next_;
                        current_ = next_;
                        next_ = temp;
                        ++count_;
                    } else {
                        count_ = maxCount_; // Ensure we stay at end
                    }
                    return *this;
                }
                
                FibonacciIterator operator++(int) {
                    auto temp = *this;
                    ++(*this);
                    return temp;
                }
                
                bool operator==(const FibonacciIterator& other) const {
                    return count_ == other.count_ && maxCount_ == other.maxCount_;
                }
                
                bool operator!=(const FibonacciIterator& other) const {
                    return !(*this == other);
                }
            };
            
            // Use the fibonacci iterator
            std::vector<int> fibNumbers;
            FibonacciIterator begin(10);
            FibonacciIterator end;
            
            for (auto it = begin; it != end; ++it) {
                fibNumbers.push_back(*it);
            }
            
            // Verify fibonacci sequence
            REQUIRE(fibNumbers.size() == 10);
            REQUIRE(fibNumbers[0] == 0);
            REQUIRE(fibNumbers[1] == 1);
            
            for (size_t i = 2; i < fibNumbers.size(); ++i) {
                REQUIRE(fibNumbers[i] == fibNumbers[i-1] + fibNumbers[i-2]);
            }
        }
    }
    
    SECTION("Iterator-Based Algorithms") {
        SECTION("Custom find algorithm with iterator concepts") {
            template<typename Iterator, typename Predicate>
            Iterator custom_find_if(Iterator first, Iterator last, Predicate pred) {
                while (first != last) {
                    if (pred(*first)) {
                        return first;
                    }
                    ++first;
                }
                return last;
            }
            
            // Find planet with specific habitability
            auto it = custom_find_if(testPlanets.begin(), testPlanets.end(),
                                   [](const std::unique_ptr<Planet>& p) {
                                       return p->getHabitabilityRating() > 0.8;
                                   });
            
            if (it != testPlanets.end()) {
                REQUIRE((*it)->getHabitabilityRating() > 0.8);
            }
            
            // Test with different iterator types
            std::list<int> testList(testNumbers.begin(), testNumbers.end());
            auto listIt = custom_find_if(testList.begin(), testList.end(),
                                       [](int n) { return n > 5; });
            
            if (listIt != testList.end()) {
                REQUIRE(*listIt > 5);
            }
        }
        
        SECTION("Iterator-based range algorithms") {
            // Custom range class for planets
            class PlanetRange {
            private:
                std::vector<std::unique_ptr<Planet>>::const_iterator begin_;
                std::vector<std::unique_ptr<Planet>>::const_iterator end_;
                
            public:
                PlanetRange(const std::vector<std::unique_ptr<Planet>>& planets)
                    : begin_(planets.begin()), end_(planets.end()) {}
                
                auto begin() const { return begin_; }
                auto end() const { return end_; }
                
                size_t size() const { return std::distance(begin_, end_); }
                bool empty() const { return begin_ == end_; }
                
                // Range-based operations
                template<typename Predicate>
                auto filter(Predicate pred) const {
                    std::vector<Planet*> result;
                    std::copy_if(begin_, end_, std::back_inserter(result),
                               [pred](const std::unique_ptr<Planet>& p) {
                                   return pred(p.get());
                               });
                    return result;
                }
                
                template<typename Transform>
                auto map(Transform transform) const {
                    std::vector<decltype(transform((*begin_).get()))> result;
                    std::transform(begin_, end_, std::back_inserter(result),
                                 [transform](const std::unique_ptr<Planet>& p) {
                                     return transform(p.get());
                                 });
                    return result;
                }
                
                double average_habitability() const {
                    if (empty()) return 0.0;
                    
                    double sum = std::accumulate(begin_, end_, 0.0,
                                               [](double acc, const std::unique_ptr<Planet>& p) {
                                                   return acc + p->getHabitabilityRating();
                                               });
                    return sum / size();
                }
            };
            
            PlanetRange range(testPlanets);
            
            // Test range operations
            REQUIRE(range.size() == testPlanets.size());
            REQUIRE(!range.empty());
            
            // Filter habitable planets
            auto habitablePlanets = range.filter([](const Planet* p) {
                return p->getHabitabilityRating() > 0.6;
            });
            
            for (const Planet* planet : habitablePlanets) {
                REQUIRE(planet->getHabitabilityRating() > 0.6);
            }
            
            // Map to planet names
            auto planetNames = range.map([](const Planet* p) {
                return p->getName();
            });
            
            REQUIRE(planetNames.size() == testPlanets.size());
            for (size_t i = 0; i < planetNames.size(); ++i) {
                REQUIRE(planetNames[i] == testPlanets[i]->getName());
            }
            
            // Calculate average habitability
            double avgHabitability = range.average_habitability();
            REQUIRE(avgHabitability >= 0.0);
            REQUIRE(avgHabitability <= 1.0);
        }
    }
}

TEST_CASE_METHOD(IteratorTestFixture, "Iterator Design Patterns", "[iterators][design][patterns]") {
    
    SECTION("Iterator Factory Pattern") {
        SECTION("Factory for different iterator types") {
            enum class IteratorType {
                FORWARD,
                REVERSE,
                FILTERED
            };
            
            class IteratorFactory {
            public:
                template<typename Container>
                static auto createIterator(const Container& container, IteratorType type) {
                    switch (type) {
                        case IteratorType::FORWARD:
                            return container.begin();
                        case IteratorType::REVERSE:
                            return container.rbegin();
                        case IteratorType::FILTERED:
                            // Return filtered iterator (simplified)
                            return container.begin();
                        default:
                            return container.begin();
                    }
                }
            };
            
            std::vector<int> vec = testNumbers;
            
            auto forwardIt = IteratorFactory::createIterator(vec, IteratorType::FORWARD);
            auto reverseIt = IteratorFactory::createIterator(vec, IteratorType::REVERSE);
            
            REQUIRE(*forwardIt == testNumbers.front());
            REQUIRE(*reverseIt == testNumbers.back());
        }
    }
    
    SECTION("Iterator Strategy Pattern") {
        SECTION("Different iteration strategies") {
            class IterationStrategy {
            public:
                virtual ~IterationStrategy() = default;
                virtual std::vector<Planet*> iterate(const std::vector<std::unique_ptr<Planet>>& planets) = 0;
            };
            
            class LinearStrategy : public IterationStrategy {
            public:
                std::vector<Planet*> iterate(const std::vector<std::unique_ptr<Planet>>& planets) override {
                    std::vector<Planet*> result;
                    for (const auto& planet : planets) {
                        result.push_back(planet.get());
                    }
                    return result;
                }
            };
            
            class ReverseStrategy : public IterationStrategy {
            public:
                std::vector<Planet*> iterate(const std::vector<std::unique_ptr<Planet>>& planets) override {
                    std::vector<Planet*> result;
                    for (auto it = planets.rbegin(); it != planets.rend(); ++it) {
                        result.push_back(it->get());
                    }
                    return result;
                }
            };
            
            class HabitabilityStrategy : public IterationStrategy {
            public:
                std::vector<Planet*> iterate(const std::vector<std::unique_ptr<Planet>>& planets) override {
                    std::vector<Planet*> result;
                    for (const auto& planet : planets) {
                        result.push_back(planet.get());
                    }
                    
                    std::sort(result.begin(), result.end(),
                             [](const Planet* a, const Planet* b) {
                                 return a->getHabitabilityRating() > b->getHabitabilityRating();
                             });
                    
                    return result;
                }
            };
            
            // Test different strategies
            LinearStrategy linearStrategy;
            ReverseStrategy reverseStrategy;
            HabitabilityStrategy habitabilityStrategy;
            
            auto linear = linearStrategy.iterate(testPlanets);
            auto reverse = reverseStrategy.iterate(testPlanets);
            auto byHabitability = habitabilityStrategy.iterate(testPlanets);
            
            REQUIRE(linear.size() == testPlanets.size());
            REQUIRE(reverse.size() == testPlanets.size());
            REQUIRE(byHabitability.size() == testPlanets.size());
            
            // Linear should match original order
            for (size_t i = 0; i < testPlanets.size(); ++i) {
                REQUIRE(linear[i] == testPlanets[i].get());
            }
            
            // Reverse should be opposite of linear
            for (size_t i = 0; i < testPlanets.size(); ++i) {
                REQUIRE(reverse[i] == testPlanets[testPlanets.size() - 1 - i].get());
            }
            
            // Habitability should be sorted by habitability (descending)
            for (size_t i = 1; i < byHabitability.size(); ++i) {
                REQUIRE(byHabitability[i-1]->getHabitabilityRating() >= 
                       byHabitability[i]->getHabitabilityRating());
            }
        }
    }
    
    SECTION("Iterator Visitor Pattern") {
        SECTION("Visitor with different iterator types") {
            class IteratorVisitor {
            public:
                virtual ~IteratorVisitor() = default;
                virtual void visit(Planet* planet, size_t index) = 0;
            };
            
            class InfoCollectorVisitor : public IteratorVisitor {
            private:
                std::vector<std::string> info_;
                
            public:
                void visit(Planet* planet, size_t index) override {
                    std::ostringstream oss;
                    oss << "Planet " << index << ": " << planet->getName() 
                        << " (Habitability: " << planet->getHabitabilityRating() << ")";
                    info_.push_back(oss.str());
                }
                
                const std::vector<std::string>& getInfo() const { return info_; }
            };
            
            class StatisticsVisitor : public IteratorVisitor {
            private:
                double totalHabitability_ = 0.0;
                int count_ = 0;
                double maxHabitability_ = 0.0;
                double minHabitability_ = 1.0;
                
            public:
                void visit(Planet* planet, size_t /*index*/) override {
                    double hab = planet->getHabitabilityRating();
                    totalHabitability_ += hab;
                    ++count_;
                    maxHabitability_ = std::max(maxHabitability_, hab);
                    minHabitability_ = std::min(minHabitability_, hab);
                }
                
                double getAverageHabitability() const {
                    return count_ > 0 ? totalHabitability_ / count_ : 0.0;
                }
                
                double getMaxHabitability() const { return maxHabitability_; }
                double getMinHabitability() const { return minHabitability_; }
                int getCount() const { return count_; }
            };
            
            auto visitPlanets = [](const std::vector<std::unique_ptr<Planet>>& planets,
                                  IteratorVisitor& visitor) {
                for (size_t i = 0; i < planets.size(); ++i) {
                    visitor.visit(planets[i].get(), i);
                }
            };
            
            // Test info collector
            InfoCollectorVisitor infoVisitor;
            visitPlanets(testPlanets, infoVisitor);
            
            const auto& info = infoVisitor.getInfo();
            REQUIRE(info.size() == testPlanets.size());
            
            for (size_t i = 0; i < info.size(); ++i) {
                REQUIRE(info[i].find("Planet " + std::to_string(i)) != std::string::npos);
                REQUIRE(info[i].find(testPlanets[i]->getName()) != std::string::npos);
            }
            
            // Test statistics visitor
            StatisticsVisitor statsVisitor;
            visitPlanets(testPlanets, statsVisitor);
            
            REQUIRE(statsVisitor.getCount() == static_cast<int>(testPlanets.size()));
            REQUIRE(statsVisitor.getAverageHabitability() >= 0.0);
            REQUIRE(statsVisitor.getAverageHabitability() <= 1.0);
            REQUIRE(statsVisitor.getMaxHabitability() >= statsVisitor.getMinHabitability());
        }
    }
}