// File: tests/benchmark_tests/AlgorithmBenchmarks.cpp
// Algorithm performance benchmarks for CppVerseHub showcase

#include <catch2/catch.hpp>
#include <chrono>
#include <vector>
#include <algorithm>
#include <random>
#include <numeric>
#include <string>
#include <functional>
#include <memory>

// Include algorithm implementations
#include "PathfindingAlgorithms.hpp"
#include "SortingAlgorithms.hpp"
#include "SearchAlgorithms.hpp"
#include "Planet.hpp"
#include "Fleet.hpp"
#include "Vector3D.hpp"

using namespace CppVerseHub::Core;
using namespace CppVerseHub::Algorithms;

/**
 * @brief Benchmark fixture for algorithm performance tests
 */
class AlgorithmBenchmarkFixture {
public:
    AlgorithmBenchmarkFixture() {
        setupBenchmarkData();
    }
    
protected:
    void setupBenchmarkData() {
        std::random_device rd;
        std::mt19937 gen(rd());
        
        // Generate random integers for sorting benchmarks
        std::uniform_int_distribution<> intDis(1, 1000000);
        
        smallIntData.reserve(1000);
        mediumIntData.reserve(10000);
        largeIntData.reserve(100000);
        
        for (int i = 0; i < 100000; ++i) {
            int value = intDis(gen);
            if (i < 1000) smallIntData.push_back(value);
            if (i < 10000) mediumIntData.push_back(value);
            largeIntData.push_back(value);
        }
        
        // Generate random doubles
        std::uniform_real_distribution<> doubleDis(0.0, 1000.0);
        
        doubleData.reserve(50000);
        for (int i = 0; i < 50000; ++i) {
            doubleData.push_back(doubleDis(gen));
        }
        
        // Generate planets for pathfinding
        std::uniform_real_distribution<> posDis(0.0, 1000.0);
        
        planets.reserve(500);
        for (int i = 0; i < 500; ++i) {
            planets.emplace_back(
                "BenchmarkPlanet_" + std::to_string(i),
                Vector3D{posDis(gen), posDis(gen), posDis(gen)}
            );
        }
        
        // Generate strings for text algorithms
        std::vector<std::string> words = {
            "algorithm", "benchmark", "performance", "optimization", "computer",
            "science", "data", "structure", "analysis", "complexity",
            "efficiency", "memory", "cache", "processor", "parallel"
        };
        
        std::uniform_int_distribution<> wordDis(0, words.size() - 1);
        std::uniform_int_distribution<> lengthDis(5, 50);
        
        stringData.reserve(10000);
        for (int i = 0; i < 10000; ++i) {
            std::string text;
            int wordCount = lengthDis(gen) / 8; // Approximate word count
            for (int j = 0; j < wordCount; ++j) {
                if (j > 0) text += " ";
                text += words[wordDis(gen)];
            }
            stringData.push_back(text);
        }
    }
    
    template<typename Container, typename Algorithm>
    double benchmarkAlgorithm(Container& data, Algorithm algo, int iterations = 1) {
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < iterations; ++i) {
            Container dataCopy = data; // Fresh copy for each iteration
            algo(dataCopy);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        return duration.count() / static_cast<double>(iterations);
    }
    
    std::vector<int> smallIntData;
    std::vector<int> mediumIntData;
    std::vector<int> largeIntData;
    std::vector<double> doubleData;
    std::vector<Planet> planets;
    std::vector<std::string> stringData;
};

TEST_CASE_METHOD(AlgorithmBenchmarkFixture, "Sorting Algorithm Benchmarks", "[benchmark][algorithms][sorting]") {
    
    SECTION("Standard library sort performance") {
        auto stdSortSmall = benchmarkAlgorithm(smallIntData, [](auto& data) {
            std::sort(data.begin(), data.end());
        });
        
        auto stdSortMedium = benchmarkAlgorithm(mediumIntData, [](auto& data) {
            std::sort(data.begin(), data.end());
        });
        
        auto stdSortLarge = benchmarkAlgorithm(largeIntData, [](auto& data) {
            std::sort(data.begin(), data.end());
        });
        
        INFO("std::sort performance:");
        INFO("Small (1K): " << stdSortSmall << "μs");
        INFO("Medium (10K): " << stdSortMedium << "μs"); 
        INFO("Large (100K): " << stdSortLarge << "μs");
        
        REQUIRE(stdSortSmall > 0);
        REQUIRE(stdSortMedium > stdSortSmall);
        REQUIRE(stdSortLarge > stdSortMedium);
    }
    
    SECTION("Quick sort implementation benchmark") {
        QuickSort quickSort;
        
        auto quickSortSmall = benchmarkAlgorithm(smallIntData, [&quickSort](auto& data) {
            quickSort.sort(data);
        });
        
        auto quickSortMedium = benchmarkAlgorithm(mediumIntData, [&quickSort](auto& data) {
            quickSort.sort(data);
        });
        
        auto quickSortLarge = benchmarkAlgorithm(largeIntData, [&quickSort](auto& data) {
            quickSort.sort(data);
        });
        
        INFO("QuickSort performance:");
        INFO("Small (1K): " << quickSortSmall << "μs");
        INFO("Medium (10K): " << quickSortMedium << "μs");
        INFO("Large (100K): " << quickSortLarge << "μs");
        
        REQUIRE(quickSortSmall > 0);
        REQUIRE(quickSortMedium > quickSortSmall);
        REQUIRE(quickSortLarge > quickSortMedium);
    }
    
    SECTION("Merge sort implementation benchmark") {
        MergeSort mergeSort;
        
        auto mergeSortSmall = benchmarkAlgorithm(smallIntData, [&mergeSort](auto& data) {
            mergeSort.sort(data);
        });
        
        auto mergeSortMedium = benchmarkAlgorithm(mediumIntData, [&mergeSort](auto& data) {
            mergeSort.sort(data);
        });
        
        auto mergeSortLarge = benchmarkAlgorithm(largeIntData, [&mergeSort](auto& data) {
            mergeSort.sort(data);
        });
        
        INFO("MergeSort performance:");
        INFO("Small (1K): " << mergeSortSmall << "μs");
        INFO("Medium (10K): " << mergeSortMedium << "μs");
        INFO("Large (100K): " << mergeSortLarge << "μs");
        
        REQUIRE(mergeSortSmall > 0);
        REQUIRE(mergeSortMedium > mergeSortSmall);
        REQUIRE(mergeSortLarge > mergeSortMedium);
    }
    
    SECTION("Heap sort implementation benchmark") {
        HeapSort heapSort;
        
        auto heapSortSmall = benchmarkAlgorithm(smallIntData, [&heapSort](auto& data) {
            heapSort.sort(data);
        });
        
        auto heapSortMedium = benchmarkAlgorithm(mediumIntData, [&heapSort](auto& data) {
            heapSort.sort(data);
        });
        
        auto heapSortLarge = benchmarkAlgorithm(largeIntData, [&heapSort](auto& data) {
            heapSort.sort(data);
        });
        
        INFO("HeapSort performance:");
        INFO("Small (1K): " << heapSortSmall << "μs");
        INFO("Medium (10K): " << heapSortMedium << "μs");
        INFO("Large (100K): " << heapSortLarge << "μs");
        
        REQUIRE(heapSortSmall > 0);
        REQUIRE(heapSortMedium > heapSortSmall);
        REQUIRE(heapSortLarge > heapSortMedium);
    }
    
    SECTION("Sorting algorithm comparison") {
        const int iterations = 5;
        
        // Benchmark different algorithms on medium dataset
        auto stdTime = benchmarkAlgorithm(mediumIntData, [](auto& data) {
            std::sort(data.begin(), data.end());
        }, iterations);
        
        QuickSort quickSort;
        auto quickTime = benchmarkAlgorithm(mediumIntData, [&quickSort](auto& data) {
            quickSort.sort(data);
        }, iterations);
        
        MergeSort mergeSort;
        auto mergeTime = benchmarkAlgorithm(mediumIntData, [&mergeSort](auto& data) {
            mergeSort.sort(data);
        }, iterations);
        
        HeapSort heapSort;
        auto heapTime = benchmarkAlgorithm(mediumIntData, [&heapSort](auto& data) {
            heapSort.sort(data);
        }, iterations);
        
        INFO("Algorithm comparison (10K elements, avg of " << iterations << " runs):");
        INFO("std::sort: " << stdTime << "μs");
        INFO("QuickSort: " << quickTime << "μs");
        INFO("MergeSort: " << mergeTime << "μs");
        INFO("HeapSort: " << heapTime << "μs");
        
        // Verify all algorithms produce correct results
        std::vector<int> testData = {5, 2, 8, 1, 9, 3};
        std::vector<int> expected = {1, 2, 3, 5, 8, 9};
        
        std::vector<int> quickResult = testData;
        quickSort.sort(quickResult);
        REQUIRE(quickResult == expected);
        
        std::vector<int> mergeResult = testData;
        mergeSort.sort(mergeResult);
        REQUIRE(mergeResult == expected);
        
        std::vector<int> heapResult = testData;
        heapSort.sort(heapResult);
        REQUIRE(heapResult == expected);
    }
}

TEST_CASE_METHOD(AlgorithmBenchmarkFixture, "Search Algorithm Benchmarks", "[benchmark][algorithms][search]") {
    
    SECTION("Linear search vs binary search") {
        // Prepare sorted data for binary search
        std::vector<int> sortedData = mediumIntData;
        std::sort(sortedData.begin(), sortedData.end());
        
        // Search for elements at different positions
        std::vector<int> targets = {
            sortedData[100],           // Early
            sortedData[5000],          // Middle
            sortedData[9000],          // Late
            sortedData[sortedData.size() - 1]  // Last
        };
        
        LinearSearch linearSearch;
        BinarySearch binarySearch;
        
        for (size_t i = 0; i < targets.size(); ++i) {
            int target = targets[i];
            
            // Benchmark linear search
            auto linearTime = benchmarkAlgorithm(sortedData, [&linearSearch, target](const auto& data) {
                return linearSearch.search(data, target);
            }, 100);
            
            // Benchmark binary search
            auto binaryTime = benchmarkAlgorithm(sortedData, [&binarySearch, target](const auto& data) {
                return binarySearch.search(data, target);
            }, 100);
            
            INFO("Search for element at position " << i << ":");
            INFO("Linear search: " << linearTime << "μs");
            INFO("Binary search: " << binaryTime << "μs");
            INFO("Binary search speedup: " << (linearTime / binaryTime) << "x");
            
            REQUIRE(binaryTime < linearTime); // Binary should be faster for large datasets
        }
    }
    
    SECTION("Hash table vs tree search") {
        HashTableSearch hashSearch;
        BinaryTreeSearch treeSearch;
        
        // Build search structures
        for (int value : mediumIntData) {
            hashSearch.insert(value);
            treeSearch.insert(value);
        }
        
        // Search for random elements
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, mediumIntData.size() - 1);
        
        const int searchCount = 1000;
        std::vector<int> searchTargets;
        
        for (int i = 0; i < searchCount; ++i) {
            searchTargets.push_back(mediumIntData[dis(gen)]);
        }
        
        // Benchmark hash table search
        auto start = std::chrono::high_resolution_clock::now();
        for (int target : searchTargets) {
            hashSearch.find(target);
        }
        auto hashTime = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now() - start).count();
        
        // Benchmark tree search
        start = std::chrono::high_resolution_clock::now();
        for (int target : searchTargets) {
            treeSearch.find(target);
        }
        auto treeTime = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now() - start).count();
        
        INFO("Search structure comparison (" << searchCount << " searches):");
        INFO("Hash table: " << hashTime << "μs total, " << (hashTime / searchCount) << "μs avg");
        INFO("Binary tree: " << treeTime << "μs total, " << (treeTime / searchCount) << "μs avg");
        
        REQUIRE(hashTime > 0);
        REQUIRE(treeTime > 0);
    }
    
    SECTION("String search algorithm benchmarks") {
        const std::string pattern = "algorithm";
        const std::string longText = std::accumulate(stringData.begin(), stringData.begin() + 100, 
                                                   std::string{}, [](const std::string& a, const std::string& b) {
                                                       return a + " " + b;
                                                   });
        
        NaiveStringSearch naiveSearch;
        KMPStringSearch kmpSearch;
        BoyerMooreStringSearch bmSearch;
        
        const int iterations = 100;
        
        auto naiveTime = benchmarkAlgorithm(std::vector<std::string>{longText}, 
            [&naiveSearch, &pattern](const auto& data) {
                return naiveSearch.search(data[0], pattern);
            }, iterations);
        
        auto kmpTime = benchmarkAlgorithm(std::vector<std::string>{longText},
            [&kmpSearch, &pattern](const auto& data) {
                return kmpSearch.search(data[0], pattern);
            }, iterations);
        
        auto bmTime = benchmarkAlgorithm(std::vector<std::string>{longText},
            [&bmSearch, &pattern](const auto& data) {
                return bmSearch.search(data[0], pattern);
            }, iterations);
        
        INFO("String search algorithm comparison:");
        INFO("Text length: " << longText.length() << " characters");
        INFO("Pattern: '" << pattern << "'");
        INFO("Naive search: " << naiveTime << "μs avg");
        INFO("KMP search: " << kmpTime << "μs avg");
        INFO("Boyer-Moore search: " << bmTime << "μs avg");
        
        REQUIRE(naiveTime > 0);
        REQUIRE(kmpTime > 0);
        REQUIRE(bmTime > 0);
    }
}

TEST_CASE_METHOD(AlgorithmBenchmarkFixture, "Pathfinding Algorithm Benchmarks", "[benchmark][algorithms][pathfinding]") {
    
    SECTION("A* pathfinding performance") {
        AStarPathfinder pathfinder;
        
        // Create start and end points
        Vector3D start{0, 0, 0};
        Vector3D end{900, 900, 900};
        
        // Convert planets to pathfinding nodes
        std::vector<PathfindingNode> nodes;
        for (size_t i = 0; i < planets.size(); ++i) {
            nodes.emplace_back(i, planets[i].getPosition());
        }
        
        // Benchmark pathfinding with different grid sizes
        std::vector<size_t> gridSizes = {50, 100, 200, 500};
        
        for (size_t gridSize : gridSizes) {
            if (gridSize > nodes.size()) continue;
            
            std::vector<PathfindingNode> gridNodes(nodes.begin(), nodes.begin() + gridSize);
            
            auto pathTime = benchmarkAlgorithm(std::vector<std::vector<PathfindingNode>>{gridNodes},
                [&pathfinder, &start, &end](const auto& data) {
                    return pathfinder.findPath(start, end, data[0]);
                }, 10);
            
            INFO("A* pathfinding (" << gridSize << " nodes): " << pathTime << "μs avg");
            
            REQUIRE(pathTime > 0);
        }
    }
    
    SECTION("Dijkstra vs A* comparison") {
        DijkstraPathfinder dijkstra;
        AStarPathfinder aStar;
        
        Vector3D start{100, 100, 100};
        Vector3D end{800, 800, 800};
        
        // Use subset of nodes for reasonable performance
        std::vector<PathfindingNode> nodes;
        for (size_t i = 0; i < std::min(planets.size(), size_t(100)); ++i) {
            nodes.emplace_back(i, planets[i].getPosition());
        }
        
        const int iterations = 5;
        
        auto dijkstraTime = benchmarkAlgorithm(std::vector<std::vector<PathfindingNode>>{nodes},
            [&dijkstra, &start, &end](const auto& data) {
                return dijkstra.findPath(start, end, data[0]);
            }, iterations);
        
        auto aStarTime = benchmarkAlgorithm(std::vector<std::vector<PathfindingNode>>{nodes},
            [&aStar, &start, &end](const auto& data) {
                return aStar.findPath(start, end, data[0]);
            }, iterations);
        
        INFO("Pathfinding algorithm comparison (" << nodes.size() << " nodes):");
        INFO("Dijkstra: " << dijkstraTime << "μs avg");
        INFO("A*: " << aStarTime << "μs avg");
        INFO("A* speedup: " << (dijkstraTime / aStarTime) << "x");
        
        REQUIRE(dijkstraTime > 0);
        REQUIRE(aStarTime > 0);
        // A* should generally be faster due to heuristic guidance
        REQUIRE(aStarTime <= dijkstraTime);
    }
    
    SECTION("Fleet navigation optimization") {
        FleetNavigator navigator;
        
        // Create test fleet
        Fleet testFleet("BenchmarkFleet", Vector3D{0, 0, 0});
        testFleet.addShips(ShipType::FIGHTER, 10);
        testFleet.addShips(ShipType::CRUISER, 3);
        
        // Create navigation waypoints from planet positions
        std::vector<Vector3D> waypoints;
        for (size_t i = 0; i < std::min(planets.size(), size_t(20)); ++i) {
            waypoints.push_back(planets[i].getPosition());
        }
        
        const int iterations = 20;
        
        auto navTime = benchmarkAlgorithm(std::vector<std::vector<Vector3D>>{waypoints},
            [&navigator, &testFleet](const auto& data) {
                return navigator.planRoute(testFleet, data[0]);
            }, iterations);
        
        INFO("Fleet navigation optimization:");
        INFO("Waypoints: " << waypoints.size());
        INFO("Planning time: " << navTime << "μs avg");
        
        REQUIRE(navTime > 0);
        REQUIRE(navTime < 10000); // Should complete within reasonable time
    }
}

TEST_CASE_METHOD(AlgorithmBenchmarkFixture, "Graph Algorithm Benchmarks", "[benchmark][algorithms][graph]") {
    
    SECTION("Minimum spanning tree algorithms") {
        // Create graph from planet positions
        const size_t nodeCount = std::min(planets.size(), size_t(50));
        GraphBuilder graphBuilder;
        
        for (size_t i = 0; i < nodeCount; ++i) {
            graphBuilder.addNode(i, planets[i].getPosition());
        }
        
        // Connect nearby planets
        const double maxDistance = 200.0;
        for (size_t i = 0; i < nodeCount; ++i) {
            for (size_t j = i + 1; j < nodeCount; ++j) {
                double distance = planets[i].getPosition().distanceTo(planets[j].getPosition());
                if (distance <= maxDistance) {
                    graphBuilder.addEdge(i, j, distance);
                }
            }
        }
        
        Graph graph = graphBuilder.build();
        
        KruskalMST kruskal;
        PrimMST prim;
        
        const int iterations = 10;
        
        auto kruskalTime = benchmarkAlgorithm(std::vector<Graph>{graph},
            [&kruskal](const auto& data) {
                return kruskal.findMST(data[0]);
            }, iterations);
        
        auto primTime = benchmarkAlgorithm(std::vector<Graph>{graph},
            [&prim](const auto& data) {
                return prim.findMST(data[0]);
            }, iterations);
        
        INFO("Minimum Spanning Tree algorithms:");
        INFO("Graph nodes: " << nodeCount);
        INFO("Graph edges: " << graph.getEdgeCount());
        INFO("Kruskal's algorithm: " << kruskalTime << "μs avg");
        INFO("Prim's algorithm: " << primTime << "μs avg");
        
        REQUIRE(kruskalTime > 0);
        REQUIRE(primTime > 0);
    }
    
    SECTION("Shortest path algorithms scalability") {
        std::vector<size_t> graphSizes = {10, 25, 50, 100};
        
        for (size_t size : graphSizes) {
            if (size > planets.size()) continue;
            
            // Build graph of specified size
            GraphBuilder builder;
            for (size_t i = 0; i < size; ++i) {
                builder.addNode(i, planets[i].getPosition());
            }
            
            // Connect each node to several nearby nodes
            const int maxConnections = 5;
            for (size_t i = 0; i < size; ++i) {
                std::vector<std::pair<double, size_t>> distances;
                
                for (size_t j = 0; j < size; ++j) {
                    if (i != j) {
                        double dist = planets[i].getPosition().distanceTo(planets[j].getPosition());
                        distances.emplace_back(dist, j);
                    }
                }
                
                std::sort(distances.begin(), distances.end());
                
                for (int k = 0; k < std::min(maxConnections, static_cast<int>(distances.size())); ++k) {
                    builder.addEdge(i, distances[k].second, distances[k].first);
                }
            }
            
            Graph graph = builder.build();
            FloydWarshall floydWarshall;
            
            auto floydTime = benchmarkAlgorithm(std::vector<Graph>{graph},
                [&floydWarshall](const auto& data) {
                    return floydWarshall.findAllPairs(data[0]);
                }, 3);
            
            INFO("Floyd-Warshall (" << size << " nodes): " << floydTime << "μs avg");
            
            REQUIRE(floydTime > 0);
        }
    }
}