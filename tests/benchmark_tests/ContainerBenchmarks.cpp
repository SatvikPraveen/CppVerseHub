// File: tests/benchmark_tests/ContainerBenchmarks.cpp
// STL container performance benchmarks for CppVerseHub showcase

#include <catch2/catch.hpp>
#include <chrono>
#include <vector>
#include <list>
#include <deque>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <queue>
#include <stack>
#include <array>
#include <forward_list>
#include <random>
#include <algorithm>
#include <numeric>

// Include core classes for container testing
#include "Planet.hpp"
#include "Fleet.hpp"
#include "Mission.hpp"

using namespace CppVerseHub::Core;

/**
 * @brief Benchmark fixture for STL container performance tests
 */
class ContainerBenchmarkFixture {
public:
    ContainerBenchmarkFixture() {
        setupBenchmarkData();
    }
    
protected:
    void setupBenchmarkData() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> intDis(1, 1000000);
        
        // Generate test integers
        testIntegers.reserve(100000);
        for (int i = 0; i < 100000; ++i) {
            testIntegers.push_back(intDis(gen));
        }
        
        // Generate test planets
        std::uniform_real_distribution<> posDis(0.0, 1000.0);
        std::uniform_int_distribution<> resDis(100, 5000);
        
        testPlanets.reserve(10000);
        for (int i = 0; i < 10000; ++i) {
            testPlanets.emplace_back(
                "BenchmarkPlanet_" + std::to_string(i),
                Vector3D{posDis(gen), posDis(gen), posDis(gen)}
            );
            testPlanets.back().setResourceAmount(ResourceType::MINERALS, resDis(gen));
            testPlanets.back().setResourceAmount(ResourceType::ENERGY, resDis(gen));
        }
        
        // Generate test fleets
        testFleets.reserve(5000);
        for (int i = 0; i < 5000; ++i) {
            testFleets.emplace_back(
                "BenchmarkFleet_" + std::to_string(i),
                Vector3D{posDis(gen), posDis(gen), posDis(gen)}
            );
            testFleets.back().addShips(ShipType::FIGHTER, 5 + (i % 20));
            testFleets.back().addShips(ShipType::CRUISER, 1 + (i % 5));
        }
        
        // Generate search keys
        searchKeys.reserve(1000);
        std::uniform_int_distribution<> keyDis(0, testIntegers.size() - 1);
        for (int i = 0; i < 1000; ++i) {
            searchKeys.push_back(testIntegers[keyDis(gen)]);
        }
    }
    
    template<typename Container, typename Operation>
    double benchmarkOperation(const std::string& name, Operation op, int iterations = 1) {
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < iterations; ++i) {
            op();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        double avgTime = duration.count() / static_cast<double>(iterations);
        
        return avgTime;
    }
    
    std::vector<int> testIntegers;
    std::vector<Planet> testPlanets;
    std::vector<Fleet> testFleets;
    std::vector<int> searchKeys;
};

TEST_CASE_METHOD(ContainerBenchmarkFixture, "Sequence Container Benchmarks", "[benchmark][containers][sequence]") {
    
    SECTION("Vector operations benchmark") {
        const int elementCount = 50000;
        const int iterations = 5;
        
        // Push back benchmark
        auto pushBackTime = benchmarkOperation<std::vector<int>>("vector push_back", [&]() {
            std::vector<int> vec;
            vec.reserve(elementCount); // Pre-allocate for fair comparison
            for (int i = 0; i < elementCount; ++i) {
                vec.push_back(testIntegers[i % testIntegers.size()]);
            }
        }, iterations);
        
        // Random access benchmark
        std::vector<int> vec(testIntegers.begin(), testIntegers.begin() + elementCount);
        auto randomAccessTime = benchmarkOperation<std::vector<int>>("vector random access", [&]() {
            volatile int sum = 0;
            for (int i = 0; i < elementCount; ++i) {
                sum += vec[i % vec.size()];
            }
        }, iterations * 10);
        
        // Insert in middle benchmark
        auto insertMiddleTime = benchmarkOperation<std::vector<int>>("vector insert middle", [&]() {
            std::vector<int> vec(testIntegers.begin(), testIntegers.begin() + 1000);
            for (int i = 0; i < 100; ++i) {
                vec.insert(vec.begin() + vec.size() / 2, testIntegers[i]);
            }
        }, iterations);
        
        INFO("Vector benchmark results:");
        INFO("Push back (" << elementCount << " elements): " << pushBackTime << "μs avg");
        INFO("Random access (" << elementCount << " accesses): " << randomAccessTime << "μs avg");
        INFO("Insert middle (100 insertions): " << insertMiddleTime << "μs avg");
        
        REQUIRE(pushBackTime > 0);
        REQUIRE(randomAccessTime > 0);
        REQUIRE(insertMiddleTime > 0);
    }
    
    SECTION("List operations benchmark") {
        const int elementCount = 50000;
        const int iterations = 5;
        
        // Push back benchmark
        auto pushBackTime = benchmarkOperation<std::list<int>>("list push_back", [&]() {
            std::list<int> lst;
            for (int i = 0; i < elementCount; ++i) {
                lst.push_back(testIntegers[i % testIntegers.size()]);
            }
        }, iterations);
        
        // Sequential access benchmark
        std::list<int> lst(testIntegers.begin(), testIntegers.begin() + elementCount);
        auto sequentialAccessTime = benchmarkOperation<std::list<int>>("list sequential access", [&]() {
            volatile int sum = 0;
            for (auto it = lst.begin(); it != lst.end(); ++it) {
                sum += *it;
            }
        }, iterations * 10);
        
        // Insert in middle benchmark
        auto insertMiddleTime = benchmarkOperation<std::list<int>>("list insert middle", [&]() {
            std::list<int> lst(testIntegers.begin(), testIntegers.begin() + 1000);
            auto middle = lst.begin();
            std::advance(middle, lst.size() / 2);
            for (int i = 0; i < 100; ++i) {
                lst.insert(middle, testIntegers[i]);
            }
        }, iterations);
        
        INFO("List benchmark results:");
        INFO("Push back (" << elementCount << " elements): " << pushBackTime << "μs avg");
        INFO("Sequential access (" << elementCount << " accesses): " << sequentialAccessTime << "μs avg");
        INFO("Insert middle (100 insertions): " << insertMiddleTime << "μs avg");
        
        REQUIRE(pushBackTime > 0);
        REQUIRE(sequentialAccessTime > 0);
        REQUIRE(insertMiddleTime > 0);
    }
    
    SECTION("Deque operations benchmark") {
        const int elementCount = 50000;
        const int iterations = 5;
        
        // Push back/front benchmark
        auto pushTime = benchmarkOperation<std::deque<int>>("deque push operations", [&]() {
            std::deque<int> dq;
            for (int i = 0; i < elementCount / 2; ++i) {
                dq.push_back(testIntegers[i % testIntegers.size()]);
                dq.push_front(testIntegers[(i + elementCount/2) % testIntegers.size()]);
            }
        }, iterations);
        
        // Random access benchmark
        std::deque<int> dq(testIntegers.begin(), testIntegers.begin() + elementCount);
        auto randomAccessTime = benchmarkOperation<std::deque<int>>("deque random access", [&]() {
            volatile int sum = 0;
            for (int i = 0; i < elementCount; ++i) {
                sum += dq[i % dq.size()];
            }
        }, iterations * 10);
        
        INFO("Deque benchmark results:");
        INFO("Push operations (" << elementCount << " elements): " << pushTime << "μs avg");
        INFO("Random access (" << elementCount << " accesses): " << randomAccessTime << "μs avg");
        
        REQUIRE(pushTime > 0);
        REQUIRE(randomAccessTime > 0);
    }
    
    SECTION("Sequence container comparison") {
        const int elementCount = 10000;
        const int iterations = 3;
        
        // Compare insertion performance
        auto vectorInsert = benchmarkOperation<std::vector<int>>("vector insertion", [&]() {
            std::vector<int> vec;
            vec.reserve(elementCount);
            for (int i = 0; i < elementCount; ++i) {
                vec.push_back(testIntegers[i % testIntegers.size()]);
            }
        }, iterations);
        
        auto listInsert = benchmarkOperation<std::list<int>>("list insertion", [&]() {
            std::list<int> lst;
            for (int i = 0; i < elementCount; ++i) {
                lst.push_back(testIntegers[i % testIntegers.size()]);
            }
        }, iterations);
        
        auto dequeInsert = benchmarkOperation<std::deque<int>>("deque insertion", [&]() {
            std::deque<int> dq;
            for (int i = 0; i < elementCount; ++i) {
                dq.push_back(testIntegers[i % testIntegers.size()]);
            }
        }, iterations);
        
        INFO("Sequence container insertion comparison (" << elementCount << " elements):");
        INFO("Vector (with reserve): " << vectorInsert << "μs avg");
        INFO("List: " << listInsert << "μs avg");
        INFO("Deque: " << dequeInsert << "μs avg");
        
        REQUIRE(vectorInsert > 0);
        REQUIRE(listInsert > 0);
        REQUIRE(dequeInsert > 0);
    }
}

TEST_CASE_METHOD(ContainerBenchmarkFixture, "Associative Container Benchmarks", "[benchmark][containers][associative]") {
    
    SECTION("Set operations benchmark") {
        const int elementCount = 20000;
        const int iterations = 3;
        
        // Insertion benchmark
        auto insertTime = benchmarkOperation<std::set<int>>("set insertion", [&]() {
            std::set<int> s;
            for (int i = 0; i < elementCount; ++i) {
                s.insert(testIntegers[i % testIntegers.size()]);
            }
        }, iterations);
        
        // Search benchmark
        std::set<int> s(testIntegers.begin(), testIntegers.begin() + elementCount);
        auto searchTime = benchmarkOperation<std::set<int>>("set search", [&]() {
            int foundCount = 0;
            for (int key : searchKeys) {
                if (s.find(key) != s.end()) {
                    foundCount++;
                }
            }
        }, iterations * 10);
        
        INFO("Set benchmark results:");
        INFO("Insertion (" << elementCount << " elements): " << insertTime << "μs avg");
        INFO("Search (" << searchKeys.size() << " searches): " << searchTime << "μs avg");
        
        REQUIRE(insertTime > 0);
        REQUIRE(searchTime > 0);
    }
    
    SECTION("Unordered set operations benchmark") {
        const int elementCount = 20000;
        const int iterations = 3;
        
        // Insertion benchmark
        auto insertTime = benchmarkOperation<std::unordered_set<int>>("unordered_set insertion", [&]() {
            std::unordered_set<int> us;
            us.reserve(elementCount); // Pre-allocate buckets
            for (int i = 0; i < elementCount; ++i) {
                us.insert(testIntegers[i % testIntegers.size()]);
            }
        }, iterations);
        
        // Search benchmark
        std::unordered_set<int> us(testIntegers.begin(), testIntegers.begin() + elementCount);
        auto searchTime = benchmarkOperation<std::unordered_set<int>>("unordered_set search", [&]() {
            int foundCount = 0;
            for (int key : searchKeys) {
                if (us.find(key) != us.end()) {
                    foundCount++;
                }
            }
        }, iterations * 10);
        
        INFO("Unordered set benchmark results:");
        INFO("Insertion (" << elementCount << " elements): " << insertTime << "μs avg");
        INFO("Search (" << searchKeys.size() << " searches): " << searchTime << "μs avg");
        
        REQUIRE(insertTime > 0);
        REQUIRE(searchTime > 0);
    }
    
    SECTION("Map operations benchmark") {
        const int elementCount = 20000;
        const int iterations = 3;
        
        // Insertion benchmark
        auto insertTime = benchmarkOperation<std::map<int, int>>("map insertion", [&]() {
            std::map<int, int> m;
            for (int i = 0; i < elementCount; ++i) {
                int key = testIntegers[i % testIntegers.size()];
                m[key] = key * 2;
            }
        }, iterations);
        
        // Search benchmark
        std::map<int, int> m;
        for (int i = 0; i < elementCount; ++i) {
            int key = testIntegers[i % testIntegers.size()];
            m[key] = key * 2;
        }
        
        auto searchTime = benchmarkOperation<std::map<int, int>>("map search", [&]() {
            int foundCount = 0;
            for (int key : searchKeys) {
                if (m.find(key) != m.end()) {
                    foundCount++;
                }
            }
        }, iterations * 10);
        
        INFO("Map benchmark results:");
        INFO("Insertion (" << elementCount << " elements): " << insertTime << "μs avg");
        INFO("Search (" << searchKeys.size() << " searches): " << searchTime << "μs avg");
        
        REQUIRE(insertTime > 0);
        REQUIRE(searchTime > 0);
    }
    
    SECTION("Unordered map operations benchmark") {
        const int elementCount = 20000;
        const int iterations = 3;
        
        // Insertion benchmark
        auto insertTime = benchmarkOperation<std::unordered_map<int, int>>("unordered_map insertion", [&]() {
            std::unordered_map<int, int> um;
            um.reserve(elementCount);
            for (int i = 0; i < elementCount; ++i) {
                int key = testIntegers[i % testIntegers.size()];
                um[key] = key * 2;
            }
        }, iterations);
        
        // Search benchmark
        std::unordered_map<int, int> um;
        um.reserve(elementCount);
        for (int i = 0; i < elementCount; ++i) {
            int key = testIntegers[i % testIntegers.size()];
            um[key] = key * 2;
        }
        
        auto searchTime = benchmarkOperation<std::unordered_map<int, int>>("unordered_map search", [&]() {
            int foundCount = 0;
            for (int key : searchKeys) {
                if (um.find(key) != um.end()) {
                    foundCount++;
                }
            }
        }, iterations * 10);
        
        INFO("Unordered map benchmark results:");
        INFO("Insertion (" << elementCount << " elements): " << insertTime << "μs avg");
        INFO("Search (" << searchKeys.size() << " searches): " << searchTime << "μs avg");
        
        REQUIRE(insertTime > 0);
        REQUIRE(searchTime > 0);
    }
    
    SECTION("Associative container comparison") {
        const int elementCount = 10000;
        const int iterations = 3;
        
        // Compare search performance
        std::set<int> s(testIntegers.begin(), testIntegers.begin() + elementCount);
        std::unordered_set<int> us(testIntegers.begin(), testIntegers.begin() + elementCount);
        
        auto setSearch = benchmarkOperation<std::set<int>>("set search comparison", [&]() {
            int found = 0;
            for (int i = 0; i < 1000; ++i) {
                if (s.find(searchKeys[i % searchKeys.size()]) != s.end()) found++;
            }
        }, iterations * 5);
        
        auto unorderedSetSearch = benchmarkOperation<std::unordered_set<int>>("unordered_set search comparison", [&]() {
            int found = 0;
            for (int i = 0; i < 1000; ++i) {
                if (us.find(searchKeys[i % searchKeys.size()]) != us.end()) found++;
            }
        }, iterations * 5);
        
        INFO("Set vs Unordered Set search comparison (1000 searches):");
        INFO("Set (ordered): " << setSearch << "μs avg");
        INFO("Unordered set (hash): " << unorderedSetSearch << "μs avg");
        INFO("Hash table speedup: " << (setSearch / unorderedSetSearch) << "x");
        
        REQUIRE(setSearch > 0);
        REQUIRE(unorderedSetSearch > 0);
        // Hash table should generally be faster for lookups
        REQUIRE(unorderedSetSearch <= setSearch);
    }
}

TEST_CASE_METHOD(ContainerBenchmarkFixture, "Container Adapter Benchmarks", "[benchmark][containers][adapters]") {
    
    SECTION("Stack operations benchmark") {
        const int elementCount = 50000;
        const int iterations = 5;
        
        // Stack push/pop benchmark
        auto stackTime = benchmarkOperation<std::stack<int>>("stack operations", [&]() {
            std::stack<int> stk;
            
            // Push elements
            for (int i = 0; i < elementCount; ++i) {
                stk.push(testIntegers[i % testIntegers.size()]);
            }
            
            // Pop elements
            while (!stk.empty()) {
                volatile int val = stk.top();
                stk.pop();
            }
        }, iterations);
        
        INFO("Stack operations (" << elementCount << " push/pop pairs): " << stackTime << "μs avg");
        REQUIRE(stackTime > 0);
    }
    
    SECTION("Queue operations benchmark") {
        const int elementCount = 50000;
        const int iterations = 5;
        
        // Queue push/pop benchmark
        auto queueTime = benchmarkOperation<std::queue<int>>("queue operations", [&]() {
            std::queue<int> q;
            
            // Push elements
            for (int i = 0; i < elementCount; ++i) {
                q.push(testIntegers[i % testIntegers.size()]);
            }
            
            // Pop elements
            while (!q.empty()) {
                volatile int val = q.front();
                q.pop();
            }
        }, iterations);
        
        INFO("Queue operations (" << elementCount << " push/pop pairs): " << queueTime << "μs avg");
        REQUIRE(queueTime > 0);
    }
    
    SECTION("Priority queue operations benchmark") {
        const int elementCount = 20000;
        const int iterations = 3;
        
        // Priority queue push/pop benchmark
        auto priorityQueueTime = benchmarkOperation<std::priority_queue<int>>("priority_queue operations", [&]() {
            std::priority_queue<int> pq;
            
            // Push elements
            for (int i = 0; i < elementCount; ++i) {
                pq.push(testIntegers[i % testIntegers.size()]);
            }
            
            // Pop elements
            while (!pq.empty()) {
                volatile int val = pq.top();
                pq.pop();
            }
        }, iterations);
        
        INFO("Priority queue operations (" << elementCount << " push/pop pairs): " << priorityQueueTime << "μs avg");
        REQUIRE(priorityQueueTime > 0);
    }
}

TEST_CASE_METHOD(ContainerBenchmarkFixture, "Real-World Container Usage", "[benchmark][containers][real-world]") {
    
    SECTION("Planet storage container comparison") {
        const int planetCount = 5000;
        const int iterations = 3;
        
        // Vector-based planet storage
        auto vectorPlanetTime = benchmarkOperation<std::vector<Planet>>("vector planet storage", [&]() {
            std::vector<Planet> planets;
            planets.reserve(planetCount);
            
            for (int i = 0; i < planetCount; ++i) {
                planets.push_back(testPlanets[i % testPlanets.size()]);
            }
            
            // Simulate searches
            for (int i = 0; i < 100; ++i) {
                auto it = std::find_if(planets.begin(), planets.end(), 
                    [i](const Planet& p) { return p.getName().find(std::to_string(i)) != std::string::npos; });
            }
        }, iterations);
        
        // Map-based planet storage (by name)
        auto mapPlanetTime = benchmarkOperation<std::map<std::string, Planet>>("map planet storage", [&]() {
            std::map<std::string, Planet> planets;
            
            for (int i = 0; i < planetCount; ++i) {
                const Planet& planet = testPlanets[i % testPlanets.size()];
                planets[planet.getName()] = planet;
            }
            
            // Simulate searches
            for (int i = 0; i < 100; ++i) {
                std::string searchName = "BenchmarkPlanet_" + std::to_string(i);
                auto it = planets.find(searchName);
            }
        }, iterations);
        
        INFO("Planet storage comparison (" << planetCount << " planets):");
        INFO("Vector-based: " << vectorPlanetTime << "μs avg");
        INFO("Map-based: " << mapPlanetTime << "μs avg");
        
        REQUIRE(vectorPlanetTime > 0);
        REQUIRE(mapPlanetTime > 0);
    }
    
    SECTION("Fleet management container performance") {
        const int fleetCount = 2500;
        const int iterations = 3;
        
        // List-based fleet management (frequent insertions/deletions)
        auto listFleetTime = benchmarkOperation<std::list<Fleet>>("list fleet management", [&]() {
            std::list<Fleet> fleets;
            
            // Add fleets
            for (int i = 0; i < fleetCount; ++i) {
                fleets.push_back(testFleets[i % testFleets.size()]);
            }
            
            // Remove every 10th fleet
            auto it = fleets.begin();
            for (int i = 0; i < fleetCount / 10; ++i) {
                std::advance(it, 10);
                if (it != fleets.end()) {
                    it = fleets.erase(it);
                }
            }
        }, iterations);
        
        // Deque-based fleet management
        auto dequeFleetTime = benchmarkOperation<std::deque<Fleet>>("deque fleet management", [&]() {
            std::deque<Fleet> fleets;
            
            // Add fleets
            for (int i = 0; i < fleetCount; ++i) {
                fleets.push_back(testFleets[i % testFleets.size()]);
            }
            
            // Remove from front and back
            for (int i = 0; i < fleetCount / 20; ++i) {
                if (!fleets.empty()) fleets.pop_front();
                if (!fleets.empty()) fleets.pop_back();
            }
        }, iterations);
        
        INFO("Fleet management comparison (" << fleetCount << " fleets):");
        INFO("List-based: " << listFleetTime << "μs avg");
        INFO("Deque-based: " << dequeFleetTime << "μs avg");
        
        REQUIRE(listFleetTime > 0);
        REQUIRE(dequeFleetTime > 0);
    }
    
    SECTION("Resource tracking with different containers") {
        const int resourceUpdates = 10000;
        const int iterations = 5;
        
        // Map-based resource tracking
        auto mapResourceTime = benchmarkOperation<std::map<std::string, int>>("map resource tracking", [&]() {
            std::map<std::string, int> resources;
            
            for (int i = 0; i < resourceUpdates; ++i) {
                std::string planetName = "Planet_" + std::to_string(i % 100);
                resources[planetName] += (i % 50) + 1;
                
                // Periodic lookups
                if (i % 100 == 0) {
                    auto it = resources.find(planetName);
                    if (it != resources.end()) {
                        volatile int value = it->second;
                    }
                }
            }
        }, iterations);
        
        // Unordered map-based resource tracking
        auto unorderedMapResourceTime = benchmarkOperation<std::unordered_map<std::string, int>>("unordered_map resource tracking", [&]() {
            std::unordered_map<std::string, int> resources;
            resources.reserve(100);
            
            for (int i = 0; i < resourceUpdates; ++i) {
                std::string planetName = "Planet_" + std::to_string(i % 100);
                resources[planetName] += (i % 50) + 1;
                
                // Periodic lookups
                if (i % 100 == 0) {
                    auto it = resources.find(planetName);
                    if (it != resources.end()) {
                        volatile int value = it->second;
                    }
                }
            }
        }, iterations);
        
        INFO("Resource tracking comparison (" << resourceUpdates << " updates):");
        INFO("Map-based: " << mapResourceTime << "μs avg");
        INFO("Unordered map-based: " << unorderedMapResourceTime << "μs avg");
        INFO("Hash table speedup: " << (mapResourceTime / unorderedMapResourceTime) << "x");
        
        REQUIRE(mapResourceTime > 0);
        REQUIRE(unorderedMapResourceTime > 0);
    }
    
    SECTION("Memory usage comparison") {
        const int elementCount = 10000;
        
        // Compare memory overhead of different containers
        // Note: This is approximate and platform-dependent
        
        std::vector<int> vec(elementCount, 42);
        std::list<int> lst(elementCount, 42);
        std::deque<int> dq(elementCount, 42);
        std::set<int> s;
        std::unordered_set<int> us;
        
        for (int i = 0; i < elementCount; ++i) {
            s.insert(i);
            us.insert(i);
        }
        
        INFO("Memory usage analysis (approximate, " << elementCount << " elements):");
        INFO("Vector: " << sizeof(vec) + vec.capacity() * sizeof(int) << " bytes");
        INFO("List: " << sizeof(lst) << " bytes + node overhead");
        INFO("Deque: " << sizeof(dq) << " bytes + block overhead");
        INFO("Set: " << sizeof(s) << " bytes + node overhead");
        INFO("Unordered set: " << sizeof(us) + us.bucket_count() * sizeof(void*) << " bytes + node overhead");
        INFO("Unordered set load factor: " << us.load_factor());
        INFO("Unordered set bucket count: " << us.bucket_count());
        
        // Verify containers work correctly
        REQUIRE(vec.size() == elementCount);
        REQUIRE(lst.size() == elementCount);
        REQUIRE(dq.size() == elementCount);
        REQUIRE(s.size() == elementCount);
        REQUIRE(us.size() == elementCount);
    }
}