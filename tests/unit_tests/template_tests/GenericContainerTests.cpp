// File: tests/unit_tests/template_tests/GenericContainerTests.cpp
// Template class tests for CppVerseHub template programming showcase

#include <catch2/catch.hpp>
#include <vector>
#include <string>
#include <memory>
#include <type_traits>
#include <algorithm>
#include <functional>

// Include the template showcase headers
#include "GenericContainers.hpp"
#include "Planet.hpp"
#include "Fleet.hpp"
#include "Entity.hpp"

using namespace CppVerseHub::Templates;
using namespace CppVerseHub::Core;

/**
 * @brief Test fixture for generic container tests
 */
class GenericContainerTestFixture {
public:
    GenericContainerTestFixture() {
        setupTestData();
    }
    
    ~GenericContainerTestFixture() {
        testPlanets.clear();
        testFleets.clear();
    }
    
protected:
    void setupTestData() {
        // Create test planets
        for (int i = 0; i < 5; ++i) {
            auto planet = std::make_unique<Planet>(
                "Planet_" + std::to_string(i),
                Vector3D{i * 100.0, i * 100.0, i * 100.0}
            );
            planet->setResourceAmount(ResourceType::MINERALS, 1000 + i * 200);
            planet->setHabitabilityRating(0.4 + i * 0.1);
            testPlanets.push_back(std::move(planet));
        }
        
        // Create test fleets
        for (int i = 0; i < 3; ++i) {
            auto fleet = std::make_unique<Fleet>(
                "Fleet_" + std::to_string(i),
                Vector3D{i * 150.0, i * 150.0, i * 150.0}
            );
            fleet->addShips(ShipType::FIGHTER, 5 + i * 3);
            testFleets.push_back(std::move(fleet));
        }
    }
    
    std::vector<std::unique_ptr<Planet>> testPlanets;
    std::vector<std::unique_ptr<Fleet>> testFleets;
};

/**
 * @brief Generic stack template implementation
 */
template<typename T, typename Container = std::vector<T>>
class GenericStack {
private:
    Container container_;
    
public:
    using value_type = T;
    using size_type = typename Container::size_type;
    using reference = typename Container::reference;
    using const_reference = typename Container::const_reference;
    
    // Basic operations
    void push(const T& value) {
        container_.push_back(value);
    }
    
    void push(T&& value) {
        container_.push_back(std::move(value));
    }
    
    template<typename... Args>
    void emplace(Args&&... args) {
        container_.emplace_back(std::forward<Args>(args)...);
    }
    
    void pop() {
        if (!empty()) {
            container_.pop_back();
        }
    }
    
    reference top() {
        return container_.back();
    }
    
    const_reference top() const {
        return container_.back();
    }
    
    bool empty() const {
        return container_.empty();
    }
    
    size_type size() const {
        return container_.size();
    }
    
    void clear() {
        container_.clear();
    }
    
    // Iterator access to underlying container (for testing)
    auto begin() { return container_.begin(); }
    auto end() { return container_.end(); }
    auto begin() const { return container_.begin(); }
    auto end() const { return container_.end(); }
};

/**
 * @brief Generic queue template with priority support
 */
template<typename T, typename Compare = std::less<T>>
class GenericPriorityQueue {
private:
    std::vector<T> heap_;
    Compare comp_;
    
    void heapify_up(size_t index) {
        while (index > 0) {
            size_t parent = (index - 1) / 2;
            if (!comp_(heap_[parent], heap_[index])) {
                break;
            }
            std::swap(heap_[parent], heap_[index]);
            index = parent;
        }
    }
    
    void heapify_down(size_t index) {
        size_t size = heap_.size();
        while (true) {
            size_t largest = index;
            size_t left = 2 * index + 1;
            size_t right = 2 * index + 2;
            
            if (left < size && comp_(heap_[largest], heap_[left])) {
                largest = left;
            }
            
            if (right < size && comp_(heap_[largest], heap_[right])) {
                largest = right;
            }
            
            if (largest == index) {
                break;
            }
            
            std::swap(heap_[index], heap_[largest]);
            index = largest;
        }
    }
    
public:
    using value_type = T;
    using size_type = std::size_t;
    
    GenericPriorityQueue() = default;
    explicit GenericPriorityQueue(const Compare& comp) : comp_(comp) {}
    
    void push(const T& value) {
        heap_.push_back(value);
        heapify_up(heap_.size() - 1);
    }
    
    void push(T&& value) {
        heap_.push_back(std::move(value));
        heapify_up(heap_.size() - 1);
    }
    
    template<typename... Args>
    void emplace(Args&&... args) {
        heap_.emplace_back(std::forward<Args>(args)...);
        heapify_up(heap_.size() - 1);
    }
    
    void pop() {
        if (!empty()) {
            heap_[0] = std::move(heap_.back());
            heap_.pop_back();
            if (!empty()) {
                heapify_down(0);
            }
        }
    }
    
    const T& top() const {
        return heap_[0];
    }
    
    bool empty() const {
        return heap_.empty();
    }
    
    size_type size() const {
        return heap_.size();
    }
    
    void clear() {
        heap_.clear();
    }
};

/**
 * @brief Generic binary search tree template
 */
template<typename Key, typename Value, typename Compare = std::less<Key>>
class GenericBST {
private:
    struct Node {
        Key key;
        Value value;
        std::unique_ptr<Node> left;
        std::unique_ptr<Node> right;
        
        Node(const Key& k, const Value& v) : key(k), value(v) {}
        Node(Key&& k, Value&& v) : key(std::move(k)), value(std::move(v)) {}
    };
    
    std::unique_ptr<Node> root_;
    Compare comp_;
    std::size_t size_;
    
    std::unique_ptr<Node>& find_node(std::unique_ptr<Node>& node, const Key& key) {
        if (!node) {
            return node;
        }
        
        if (comp_(key, node->key)) {
            return find_node(node->left, key);
        } else if (comp_(node->key, key)) {
            return find_node(node->right, key);
        } else {
            return node;
        }
    }
    
    const std::unique_ptr<Node>& find_node(const std::unique_ptr<Node>& node, const Key& key) const {
        static const std::unique_ptr<Node> null_node = nullptr;
        
        if (!node) {
            return null_node;
        }
        
        if (comp_(key, node->key)) {
            return find_node(node->left, key);
        } else if (comp_(node->key, key)) {
            return find_node(node->right, key);
        } else {
            return node;
        }
    }
    
    void insert_node(std::unique_ptr<Node>& node, const Key& key, const Value& value) {
        if (!node) {
            node = std::make_unique<Node>(key, value);
            ++size_;
            return;
        }
        
        if (comp_(key, node->key)) {
            insert_node(node->left, key, value);
        } else if (comp_(node->key, key)) {
            insert_node(node->right, key, value);
        } else {
            node->value = value; // Update existing
        }
    }
    
    void inorder_traversal(const std::unique_ptr<Node>& node, std::vector<std::pair<Key, Value>>& result) const {
        if (node) {
            inorder_traversal(node->left, result);
            result.emplace_back(node->key, node->value);
            inorder_traversal(node->right, result);
        }
    }
    
public:
    GenericBST() : size_(0) {}
    explicit GenericBST(const Compare& comp) : comp_(comp), size_(0) {}
    
    void insert(const Key& key, const Value& value) {
        insert_node(root_, key, value);
    }
    
    void insert(Key&& key, Value&& value) {
        // Simplified version - would need more complex implementation for perfect forwarding
        insert_node(root_, key, value);
    }
    
    Value* find(const Key& key) {
        auto& node = find_node(root_, key);
        return node ? &node->value : nullptr;
    }
    
    const Value* find(const Key& key) const {
        const auto& node = find_node(root_, key);
        return node ? &node->value : nullptr;
    }
    
    bool contains(const Key& key) const {
        return find(key) != nullptr;
    }
    
    std::size_t size() const {
        return size_;
    }
    
    bool empty() const {
        return size_ == 0;
    }
    
    std::vector<std::pair<Key, Value>> inorder() const {
        std::vector<std::pair<Key, Value>> result;
        inorder_traversal(root_, result);
        return result;
    }
    
    void clear() {
        root_.reset();
        size_ = 0;
    }
};

TEST_CASE_METHOD(GenericContainerTestFixture, "Generic Stack Implementation", "[templates][containers][stack]") {
    
    SECTION("Basic Stack Operations") {
        SECTION("Integer stack") {
            GenericStack<int> intStack;
            
            // Test empty stack
            REQUIRE(intStack.empty());
            REQUIRE(intStack.size() == 0);
            
            // Push elements
            intStack.push(1);
            intStack.push(2);
            intStack.push(3);
            
            REQUIRE(!intStack.empty());
            REQUIRE(intStack.size() == 3);
            REQUIRE(intStack.top() == 3);
            
            // Pop elements
            intStack.pop();
            REQUIRE(intStack.top() == 2);
            REQUIRE(intStack.size() == 2);
            
            intStack.pop();
            REQUIRE(intStack.top() == 1);
            
            intStack.pop();
            REQUIRE(intStack.empty());
        }
        
        SECTION("String stack") {
            GenericStack<std::string> stringStack;
            
            stringStack.push("first");
            stringStack.push("second");
            stringStack.emplace("third");
            
            REQUIRE(stringStack.top() == "third");
            REQUIRE(stringStack.size() == 3);
            
            stringStack.pop();
            REQUIRE(stringStack.top() == "second");
        }
        
        SECTION("Planet pointer stack") {
            GenericStack<Planet*> planetStack;
            
            for (const auto& planet : testPlanets) {
                planetStack.push(planet.get());
            }
            
            REQUIRE(planetStack.size() == testPlanets.size());
            
            // Verify LIFO order
            for (int i = testPlanets.size() - 1; i >= 0; --i) {
                REQUIRE(planetStack.top() == testPlanets[i].get());
                planetStack.pop();
            }
            
            REQUIRE(planetStack.empty());
        }
    }
    
    SECTION("Stack with Different Underlying Containers") {
        SECTION("Stack with deque") {
            GenericStack<int, std::deque<int>> dequeStack;
            
            for (int i = 1; i <= 5; ++i) {
                dequeStack.push(i);
            }
            
            REQUIRE(dequeStack.size() == 5);
            REQUIRE(dequeStack.top() == 5);
            
            // Should behave the same as vector-based stack
            std::vector<int> poppedValues;
            while (!dequeStack.empty()) {
                poppedValues.push_back(dequeStack.top());
                dequeStack.pop();
            }
            
            std::vector<int> expected = {5, 4, 3, 2, 1};
            REQUIRE(poppedValues == expected);
        }
    }
    
    SECTION("Move Semantics and Perfect Forwarding") {
        SECTION("Move-only types") {
            GenericStack<std::unique_ptr<Planet>> uniquePlanetStack;
            
            // Test move semantics
            auto planet = std::make_unique<Planet>("MovePlanet", Vector3D{0, 0, 0});
            std::string originalName = planet->getName();
            
            uniquePlanetStack.push(std::move(planet));
            REQUIRE(planet == nullptr); // Moved from
            REQUIRE(uniquePlanetStack.top()->getName() == originalName);
            
            // Test emplace
            uniquePlanetStack.emplace(std::make_unique<Planet>("EmplacePlanet", Vector3D{1, 1, 1}));
            REQUIRE(uniquePlanetStack.size() == 2);
            REQUIRE(uniquePlanetStack.top()->getName() == "EmplacePlanet");
        }
    }
}

TEST_CASE_METHOD(GenericContainerTestFixture, "Generic Priority Queue Implementation", "[templates][containers][priority-queue]") {
    
    SECTION("Basic Priority Queue Operations") {
        SECTION("Max heap (default)") {
            GenericPriorityQueue<int> maxHeap;
            
            // Insert elements
            std::vector<int> values = {3, 1, 4, 1, 5, 9, 2, 6};
            for (int value : values) {
                maxHeap.push(value);
            }
            
            REQUIRE(maxHeap.size() == values.size());
            REQUIRE(!maxHeap.empty());
            
            // Extract in descending order
            std::vector<int> extracted;
            while (!maxHeap.empty()) {
                extracted.push_back(maxHeap.top());
                maxHeap.pop();
            }
            
            // Should be sorted in descending order
            REQUIRE(std::is_sorted(extracted.rbegin(), extracted.rend()));
            REQUIRE(extracted.front() == 9); // Maximum element first
        }
        
        SECTION("Min heap") {
            GenericPriorityQueue<int, std::greater<int>> minHeap;
            
            std::vector<int> values = {3, 1, 4, 1, 5, 9, 2, 6};
            for (int value : values) {
                minHeap.push(value);
            }
            
            // Extract in ascending order
            std::vector<int> extracted;
            while (!minHeap.empty()) {
                extracted.push_back(minHeap.top());
                minHeap.pop();
            }
            
            // Should be sorted in ascending order
            REQUIRE(std::is_sorted(extracted.begin(), extracted.end()));
            REQUIRE(extracted.front() == 1); // Minimum element first
        }
        
        SECTION("Custom comparator with game objects") {
            struct PlanetHabitabilityComparator {
                bool operator()(const Planet* a, const Planet* b) const {
                    return a->getHabitabilityRating() < b->getHabitabilityRating();
                }
            };
            
            GenericPriorityQueue<Planet*, PlanetHabitabilityComparator> planetQueue;
            
            // Add all test planets
            for (const auto& planet : testPlanets) {
                planetQueue.push(planet.get());
            }
            
            // Extract planets in order of habitability (highest first)
            std::vector<Planet*> extractedPlanets;
            while (!planetQueue.empty()) {
                extractedPlanets.push_back(planetQueue.top());
                planetQueue.pop();
            }
            
            // Verify descending order by habitability
            for (size_t i = 1; i < extractedPlanets.size(); ++i) {
                REQUIRE(extractedPlanets[i-1]->getHabitabilityRating() >= 
                       extractedPlanets[i]->getHabitabilityRating());
            }
        }
    }
    
    SECTION("Priority Queue Performance") {
        SECTION("Large dataset performance") {
            GenericPriorityQueue<int> largeHeap;
            const int elementCount = 10000;
            
            // Insert many elements
            auto insertDuration = measurePerformance([&]() {
                for (int i = 0; i < elementCount; ++i) {
                    largeHeap.push(rand() % 1000000);
                }
            }, "Priority queue insertion of " + std::to_string(elementCount) + " elements");
            
            REQUIRE(largeHeap.size() == elementCount);
            REQUIRE(insertDuration.count() < 100000); // Should be reasonably fast
            
            // Extract all elements
            auto extractDuration = measurePerformance([&]() {
                while (!largeHeap.empty()) {
                    largeHeap.pop();
                }
            }, "Priority queue extraction of " + std::to_string(elementCount) + " elements");
            
            REQUIRE(largeHeap.empty());
            REQUIRE(extractDuration.count() < 100000);
        }
    }
    
    SECTION("Priority Queue with Complex Objects") {
        SECTION("Fleet priority by combat power") {
            struct FleetCombatComparator {
                bool operator()(const Fleet* a, const Fleet* b) const {
                    return a->getCombatPower() < b->getCombatPower();
                }
            };
            
            GenericPriorityQueue<Fleet*, FleetCombatComparator> fleetQueue;
            
            for (const auto& fleet : testFleets) {
                fleetQueue.push(fleet.get());
            }
            
            REQUIRE(fleetQueue.size() == testFleets.size());
            
            // Top should be the fleet with highest combat power
            Fleet* topFleet = fleetQueue.top();
            for (const auto& fleet : testFleets) {
                REQUIRE(topFleet->getCombatPower() >= fleet->getCombatPower());
            }
            
            // Extract and verify order
            std::vector<double> combatPowers;
            while (!fleetQueue.empty()) {
                combatPowers.push_back(fleetQueue.top()->getCombatPower());
                fleetQueue.pop();
            }
            
            // Should be in descending order
            for (size_t i = 1; i < combatPowers.size(); ++i) {
                REQUIRE(combatPowers[i-1] >= combatPowers[i]);
            }
        }
    }
}

TEST_CASE_METHOD(GenericContainerTestFixture, "Generic Binary Search Tree", "[templates][containers][bst]") {
    
    SECTION("Basic BST Operations") {
        SECTION("Integer BST") {
            GenericBST<int, std::string> intBST;
            
            // Insert key-value pairs
            intBST.insert(5, "five");
            intBST.insert(3, "three");
            intBST.insert(7, "seven");
            intBST.insert(1, "one");
            intBST.insert(9, "nine");
            
            REQUIRE(intBST.size() == 5);
            REQUIRE(!intBST.empty());
            
            // Test find operations
            auto* value = intBST.find(5);
            REQUIRE(value != nullptr);
            REQUIRE(*value == "five");
            
            auto* notFound = intBST.find(10);
            REQUIRE(notFound == nullptr);
            
            // Test contains
            REQUIRE(intBST.contains(3));
            REQUIRE(intBST.contains(7));
            REQUIRE(!intBST.contains(10));
            
            // Test inorder traversal (should be sorted)
            auto inorder = intBST.inorder();
            std::vector<int> keys;
            for (const auto& pair : inorder) {
                keys.push_back(pair.first);
            }
            
            std::vector<int> expectedKeys = {1, 3, 5, 7, 9};
            REQUIRE(keys == expectedKeys);
        }
        
        SECTION("String key BST") {
            GenericBST<std::string, Planet*> planetBST;
            
            // Insert planets by name
            for (const auto& planet : testPlanets) {
                planetBST.insert(planet->getName(), planet.get());
            }
            
            REQUIRE(planetBST.size() == testPlanets.size());
            
            // Find specific planet
            auto* foundPlanet = planetBST.find("Planet_2");
            REQUIRE(foundPlanet != nullptr);
            REQUIRE(*foundPlanet == testPlanets[2].get());
            
            // Test inorder traversal (alphabetical order by name)
            auto inorder = planetBST.inorder();
            std::vector<std::string> names;
            for (const auto& pair : inorder) {
                names.push_back(pair.first);
            }
            
            // Should be sorted alphabetically
            REQUIRE(std::is_sorted(names.begin(), names.end()));
        }
        
        SECTION("Custom comparator BST") {
            struct DescendingComparator {
                bool operator()(int a, int b) const {
                    return a > b; // Reverse order
                }
            };
            
            GenericBST<int, std::string, DescendingComparator> descendingBST;
            
            descendingBST.insert(5, "five");
            descendingBST.insert(3, "three");
            descendingBST.insert(7, "seven");
            descendingBST.insert(1, "one");
            descendingBST.insert(9, "nine");
            
            // Inorder traversal should be in descending order
            auto inorder = descendingBST.inorder();
            std::vector<int> keys;
            for (const auto& pair : inorder) {
                keys.push_back(pair.first);
            }
            
            // Should be sorted in descending order
            std::vector<int> expectedKeys = {9, 7, 5, 3, 1};
            REQUIRE(keys == expectedKeys);
        }
    }
    
    SECTION("BST Update and Modification") {
        SECTION("Update existing values") {
            GenericBST<int, std::string> bst;
            
            bst.insert(5, "original");
            REQUIRE(*bst.find(5) == "original");
            
            // Insert with same key should update
            bst.insert(5, "updated");
            REQUIRE(*bst.find(5) == "updated");
            REQUIRE(bst.size() == 1); // Size should not increase
        }
        
        SECTION("Clear BST") {
            GenericBST<int, std::string> bst;
            
            for (int i = 1; i <= 10; ++i) {
                bst.insert(i, "value" + std::to_string(i));
            }
            
            REQUIRE(bst.size() == 10);
            REQUIRE(!bst.empty());
            
            bst.clear();
            
            REQUIRE(bst.size() == 0);
            REQUIRE(bst.empty());
            REQUIRE(bst.find(5) == nullptr);
        }
    }
    
    SECTION("BST with Complex Objects") {
        SECTION("Planet as key") {
            struct PlanetNameComparator {
                bool operator()(const Planet* a, const Planet* b) const {
                    return a->getName() < b->getName();
                }
            };
            
            GenericBST<Planet*, double, PlanetNameComparator> habitabilityBST;
            
            // Use planets as keys, habitability as values
            for (const auto& planet : testPlanets) {
                habitabilityBST.insert(planet.get(), planet->getHabitabilityRating());
            }
            
            REQUIRE(habitabilityBST.size() == testPlanets.size());
            
            // Find habitability by planet
            auto* habitability = habitabilityBST.find(testPlanets[2].get());
            REQUIRE(habitability != nullptr);
            REQUIRE(*habitability == testPlanets[2]->getHabitabilityRating());
            
            // Verify inorder traversal by planet name
            auto inorder = habitabilityBST.inorder();
            for (size_t i = 1; i < inorder.size(); ++i) {
                REQUIRE(inorder[i-1].first->getName() <= inorder[i].first->getName());
            }
        }
    }
}

TEST_CASE_METHOD(GenericContainerTestFixture, "Template Specialization and SFINAE", "[templates][specialization][sfinae]") {
    
    SECTION("Template Specialization") {
        SECTION("Specialized stack for pointers") {
            // Specialized version of GenericStack for pointer types
            template<typename T, typename Container>
            class GenericStack<T*, Container> {
            private:
                Container container_;
                
            public:
                void push(T* value) {
                    if (value != nullptr) {  // Only push non-null pointers
                        container_.push_back(value);
                    }
                }
                
                T* top() {
                    return container_.empty() ? nullptr : container_.back();
                }
                
                void pop() {
                    if (!container_.empty()) {
                        container_.pop_back();
                    }
                }
                
                bool empty() const {
                    return container_.empty();
                }
                
                std::size_t size() const {
                    return container_.size();
                }
                
                // Additional method specific to pointer specialization
                bool contains(T* ptr) const {
                    return std::find(container_.begin(), container_.end(), ptr) != container_.end();
                }
            };
            
            GenericStack<Planet*> pointerStack;
            
            // Test null pointer handling
            pointerStack.push(nullptr);
            REQUIRE(pointerStack.empty()); // Should not add null pointers
            
            pointerStack.push(testPlanets[0].get());
            pointerStack.push(testPlanets[1].get());
            
            REQUIRE(pointerStack.size() == 2);
            REQUIRE(pointerStack.contains(testPlanets[1].get()));
            REQUIRE(!pointerStack.contains(testPlanets[2].get()));
        }
    }
    
    SECTION("SFINAE Techniques") {
        SECTION("Type trait detection") {
            // Helper to detect if type has a getName method
            template<typename T>
            class has_getName {
                template<typename U>
                static auto test(int) -> decltype(std::declval<U>().getName(), std::true_type{});
                
                template<typename>
                static std::false_type test(...);
                
            public:
                static constexpr bool value = decltype(test<T>(0))::value;
            };
            
            // Test the trait
            static_assert(has_getName<Planet>::value, "Planet should have getName method");
            static_assert(has_getName<Fleet>::value, "Fleet should have getName method");
            static_assert(!has_getName<int>::value, "int should not have getName method");
            static_assert(!has_getName<std::string>::value, "string should not have getName method");
            
            REQUIRE(has_getName<Planet>::value);
            REQUIRE(!has_getName<int>::value);
        }
        
        SECTION("SFINAE-based function overloading") {
            // Function that works differently based on whether type has getName
            template<typename T>
            auto getIdentifier(const T& obj) -> 
                typename std::enable_if<has_getName<T>::value, std::string>::type {
                return obj.getName();
            }
            
            template<typename T>
            auto getIdentifier(const T& obj) -> 
                typename std::enable_if<!has_getName<T>::value, std::string>::type {
                return "Unknown_" + std::to_string(reinterpret_cast<uintptr_t>(&obj));
            }
            
            // Test with different types
            std::string planetId = getIdentifier(*testPlanets[0]);
            REQUIRE(planetId == "Planet_0");
            
            int number = 42;
            std::string numberId = getIdentifier(number);
            REQUIRE(numberId.find("Unknown_") == 0);
        }
        
        SECTION("Conditional compilation with if constexpr") {
            template<typename T>
            std::string processObject(const T& obj) {
                if constexpr (has_getName<T>::value) {
                    return "Named object: " + obj.getName();
                } else if constexpr (std::is_arithmetic_v<T>) {
                    return "Numeric value: " + std::to_string(obj);
                } else {
                    return "Unknown object type";
                }
            }
            
            std::string planetResult = processObject(*testPlanets[0]);
            REQUIRE(planetResult == "Named object: Planet_0");
            
            std::string intResult = processObject(42);
            REQUIRE(intResult == "Numeric value: 42");
            
            std::string doubleResult = processObject(3.14);
            REQUIRE(doubleResult == "Numeric value: 3.140000");
        }
    }
}

TEST_CASE_METHOD(GenericContainerTestFixture, "Template Metaprogramming", "[templates][metaprogramming][compile-time]") {
    
    SECTION("Compile-time Computations") {
        SECTION("Compile-time factorial") {
            template<int N>
            struct Factorial {
                static constexpr int value = N * Factorial<N - 1>::value;
            };
            
            template<>
            struct Factorial<0> {
                static constexpr int value = 1;
            };
            
            static_assert(Factorial<5>::value == 120);
            static_assert(Factorial<0>::value == 1);
            static_assert(Factorial<1>::value == 1);
            
            REQUIRE(Factorial<5>::value == 120);
        }
        
        SECTION("Compile-time type list operations") {
            template<typename... Types>
            struct TypeList {};
            
            template<typename List>
            struct Length;
            
            template<typename... Types>
            struct Length<TypeList<Types...>> {
                static constexpr std::size_t value = sizeof...(Types);
            };
            
            template<typename List, typename T>
            struct Contains;
            
            template<typename T, typename... Types>
            struct Contains<TypeList<Types...>, T> {
                static constexpr bool value = (std::is_same_v<Types, T> || ...);
            };
            
            using GameTypes = TypeList<Planet, Fleet, int, std::string>;
            
            static_assert(Length<GameTypes>::value == 4);
            static_assert(Contains<GameTypes, Planet>::value);
            static_assert(Contains<GameTypes, Fleet>::value);
            static_assert(!Contains<GameTypes, double>::value);
            
            REQUIRE(Length<GameTypes>::value == 4);
            REQUIRE(Contains<GameTypes, Planet>::value);
            REQUIRE(!Contains<GameTypes, double>::value);
        }
        
        SECTION("Template-based compile-time string") {
            template<char... Chars>
            struct CompileTimeString {
                static constexpr char value[] = {Chars..., '\0'};
                static constexpr std::size_t length = sizeof...(Chars);
            };
            
            using HelloString = CompileTimeString<'H', 'e', 'l', 'l', 'o'>;
            
            static_assert(HelloString::length == 5);
            REQUIRE(std::string(HelloString::value) == "Hello");
        }
    }
    
    SECTION("Template Template Parameters") {
        SECTION("Generic container adapter") {
            template<template<typename, typename...> class Container, typename T>
            class ContainerAdapter {
            private:
                Container<T> container_;
                
            public:
                void add(const T& item) {
                    if constexpr (requires { container_.push_back(item); }) {
                        container_.push_back(item);
                    } else if constexpr (requires { container_.push(item); }) {
                        container_.push(item);
                    } else if constexpr (requires { container_.insert(item); }) {
                        container_.insert(item);
                    }
                }
                
                std::size_t size() const {
                    return container_.size();
                }
                
                bool empty() const {
                    return container_.empty();
                }
                
                // Iterator access when available
                auto begin() -> decltype(container_.begin()) {
                    return container_.begin();
                }
                
                auto end() -> decltype(container_.end()) {
                    return container_.end();
                }
            };
            
            ContainerAdapter<std::vector, int> vectorAdapter;
            ContainerAdapter<std::set, int> setAdapter;
            
            vectorAdapter.add(1);
            vectorAdapter.add(2);
            vectorAdapter.add(3);
            
            setAdapter.add(3);
            setAdapter.add(1);
            setAdapter.add(2);
            setAdapter.add(2); // Duplicate should be ignored by set
            
            REQUIRE(vectorAdapter.size() == 3);
            REQUIRE(setAdapter.size() == 3); // Set ignores duplicates
            
            // Vector maintains insertion order
            std::vector<int> vectorContents(vectorAdapter.begin(), vectorAdapter.end());
            REQUIRE(vectorContents == std::vector<int>{1, 2, 3});
            
            // Set maintains sorted order
            std::vector<int> setContents(setAdapter.begin(), setAdapter.end());
            REQUIRE(setContents == std::vector<int>{1, 2, 3});
        }
    }
    
    SECTION("Variadic Templates") {
        SECTION("Type-safe print function") {
            template<typename... Args>
            std::string format(Args&&... args) {
                std::ostringstream oss;
                ((oss << args << " "), ...); // C++17 fold expression
                std::string result = oss.str();
                if (!result.empty()) {
                    result.pop_back(); // Remove trailing space
                }
                return result;
            }
            
            std::string result1 = format(1, 2, 3);
            REQUIRE(result1 == "1 2 3");
            
            std::string result2 = format("Planet:", testPlanets[0]->getName(), "Habitability:", testPlanets[0]->getHabitabilityRating());
            REQUIRE(result2.find("Planet: Planet_0") != std::string::npos);
        }
        
        SECTION("Variadic template for multiple containers") {
            template<typename... Containers>
            auto combineSizes(const Containers&... containers) {
                return (containers.size() + ...); // C++17 fold expression
            }
            
            std::vector<int> vec = {1, 2, 3};
            std::list<int> lst = {4, 5};
            std::set<int> set = {6, 7, 8, 9};
            
            auto totalSize = combineSizes(vec, lst, set);
            REQUIRE(totalSize == 9);
            
            auto singleSize = combineSizes(vec);
            REQUIRE(singleSize == 3);
        }
    }
}

TEST_CASE_METHOD(GenericContainerTestFixture, "Template Performance and Optimization", "[templates][performance][optimization]") {
    
    SECTION("Template Instantiation Performance") {
        SECTION("Stack performance comparison") {
            const int operationCount = 100000;
            
            // Test GenericStack performance
            GenericStack<int> templateStack;
            auto templateDuration = measurePerformance([&]() {
                for (int i = 0; i < operationCount; ++i) {
                    templateStack.push(i);
                }
                for (int i = 0; i < operationCount; ++i) {
                    templateStack.pop();
                }
            }, "Template stack operations");
            
            // Test std::stack performance
            std::stack<int> stdStack;
            auto stdDuration = measurePerformance([&]() {
                for (int i = 0; i < operationCount; ++i) {
                    stdStack.push(i);
                }
                for (int i = 0; i < operationCount; ++i) {
                    stdStack.pop();
                }
            }, "STL stack operations");
            
            INFO("Template stack: " << templateDuration.count() << "μs");
            INFO("STL stack: " << stdDuration.count() << "μs");
            
            // Both should be reasonably fast and similar
            REQUIRE(templateDuration.count() < 500000); // 500ms
            REQUIRE(stdDuration.count() < 500000);
        }
    }
    
    SECTION("Memory Layout Optimization") {
        SECTION("Template memory usage") {
            MemoryTracker::printMemoryStats("Before template container creation");
            
            {
                GenericStack<Planet*> planetStack;
                GenericPriorityQueue<Fleet*> fleetQueue;
                GenericBST<int, std::string> intBST;
                
                // Fill with data
                for (const auto& planet : testPlanets) {
                    planetStack.push(planet.get());
                }
                
                for (const auto& fleet : testFleets) {
                    fleetQueue.push(fleet.get());
                }
                
                for (int i = 0; i < 1000; ++i) {
                    intBST.insert(i, "value" + std::to_string(i));
                }
                
                MemoryTracker::printMemoryStats("After filling template containers");
                
                REQUIRE(planetStack.size() == testPlanets.size());
                REQUIRE(fleetQueue.size() == testFleets.size());
                REQUIRE(intBST.size() == 1000);
                
            } // Containers destroyed here
            
            MemoryTracker::printMemoryStats("After template container destruction");
        }
    }
}