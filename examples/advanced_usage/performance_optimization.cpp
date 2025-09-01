// File: examples/advanced_usage/performance_optimization.cpp
// CppVerseHub Performance Optimization Techniques Example
// Advanced demonstration of optimization strategies for high-performance simulation

#include <iostream>
#include <memory>
#include <vector>
#include <chrono>
#include <algorithm>
#include <numeric>
#include <random>
#include <thread>
#include <future>
#include <execution>
#include <immintrin.h>  // For SIMD intrinsics
#include <iomanip>

// Core includes
#include "Entity.hpp"
#include "Planet.hpp" 
#include "Fleet.hpp"
#include "ThreadPool.hpp"
#include "MemoryPool.hpp"
#include "Logger.hpp"

using namespace std;
using namespace chrono;

/**
 * @brief Performance measurement utility
 */
class PerformanceTimer {
private:
    high_resolution_clock::time_point startTime;
    string operationName;
    
public:
    PerformanceTimer(const string& name) : operationName(name) {
        startTime = high_resolution_clock::now();
    }
    
    ~PerformanceTimer() {
        auto endTime = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(endTime - startTime);
        cout << operationName << ": " << duration.count() << " microseconds" << endl;
    }
    
    static double measureOperation(const string& name, function<void()> operation) {
        auto start = high_resolution_clock::now();
        operation();
        auto end = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(end - start).count();
        cout << name << ": " << duration << " microseconds" << endl;
        return duration / 1000.0;  // Return milliseconds
    }
};

/**
 * @brief SIMD-optimized vector operations
 */
namespace SIMDOptimizations {
    
    /**
     * @brief SIMD-optimized distance calculation for multiple points
     */
    void calculateDistancesSIMD(const vector<Vector3D>& points1, 
                                const vector<Vector3D>& points2,
                                vector<double>& results) {
        results.resize(min(points1.size(), points2.size()));
        
        // Process 4 distances at a time using AVX
        size_t simdCount = results.size() & ~3;  // Round down to multiple of 4
        
        for (size_t i = 0; i < simdCount; i += 4) {
            // Load coordinates
            __m256d x1 = _mm256_set_pd(points1[i+3].x, points1[i+2].x, points1[i+1].x, points1[i].x);
            __m256d y1 = _mm256_set_pd(points1[i+3].y, points1[i+2].y, points1[i+1].y, points1[i].y);
            __m256d z1 = _mm256_set_pd(points1[i+3].z, points1[i+2].z, points1[i+1].z, points1[i].z);
            
            __m256d x2 = _mm256_set_pd(points2[i+3].x, points2[i+2].x, points2[i+1].x, points2[i].x);
            __m256d y2 = _mm256_set_pd(points2[i+3].y, points2[i+2].y, points2[i+1].y, points2[i].y);
            __m256d z2 = _mm256_set_pd(points2[i+3].z, points2[i+2].z, points2[i+1].z, points2[i].z);
            
            // Calculate differences
            __m256d dx = _mm256_sub_pd(x2, x1);
            __m256d dy = _mm256_sub_pd(y2, y1);
            __m256d dz = _mm256_sub_pd(z2, z1);
            
            // Square the differences
            __m256d dx2 = _mm256_mul_pd(dx, dx);
            __m256d dy2 = _mm256_mul_pd(dy, dy);
            __m256d dz2 = _mm256_mul_pd(dz, dz);
            
            // Sum squares
            __m256d sum = _mm256_add_pd(_mm256_add_pd(dx2, dy2), dz2);
            
            // Square root
            __m256d distance = _mm256_sqrt_pd(sum);
            
            // Store results
            double temp[4];
            _mm256_storeu_pd(temp, distance);
            
            for (int j = 0; j < 4; ++j) {
                results[i + j] = temp[j];
            }
        }
        
        // Handle remaining elements
        for (size_t i = simdCount; i < results.size(); ++i) {
            results[i] = points1[i].distanceTo(points2[i]);
        }
    }
    
    /**
     * @brief Regular distance calculation for comparison
     */
    void calculateDistancesRegular(const vector<Vector3D>& points1,
                                   const vector<Vector3D>& points2,
                                   vector<double>& results) {
        results.resize(min(points1.size(), points2.size()));
        
        for (size_t i = 0; i < results.size(); ++i) {
            results[i] = points1[i].distanceTo(points2[i]);
        }
    }
}

/**
 * @brief Memory optimization techniques
 */
namespace MemoryOptimizations {
    
    /**
     * @brief Demonstrates object pooling for frequent allocations
     */
    class OptimizedEntityPool {
    private:
        static constexpr size_t POOL_SIZE = 1000;
        alignas(Planet) char planetPool[POOL_SIZE * sizeof(Planet)];
        alignas(Fleet) char fleetPool[POOL_SIZE * sizeof(Fleet)];
        
        bitset<POOL_SIZE> planetUsed;
        bitset<POOL_SIZE> fleetUsed;
        
        size_t nextPlanetIndex = 0;
        size_t nextFleetIndex = 0;
        
    public:
        Planet* allocatePlanet() {
            for (size_t i = 0; i < POOL_SIZE; ++i) {
                size_t index = (nextPlanetIndex + i) % POOL_SIZE;
                if (!planetUsed[index]) {
                    planetUsed[index] = true;
                    nextPlanetIndex = (index + 1) % POOL_SIZE;
                    return reinterpret_cast<Planet*>(&planetPool[index * sizeof(Planet)]);
                }
            }
            return nullptr;  // Pool exhausted
        }
        
        void deallocatePlanet(Planet* planet) {
            char* poolStart = planetPool;
            char* planetPtr = reinterpret_cast<char*>(planet);
            
            if (planetPtr >= poolStart && planetPtr < poolStart + sizeof(planetPool)) {
                size_t index = (planetPtr - poolStart) / sizeof(Planet);
                if (index < POOL_SIZE) {
                    planet->~Planet();  // Call destructor
                    planetUsed[index] = false;
                }
            }
        }
        
        Fleet* allocateFleet() {
            for (size_t i = 0; i < POOL_SIZE; ++i) {
                size_t index = (nextFleetIndex + i) % POOL_SIZE;
                if (!fleetUsed[index]) {
                    fleetUsed[index] = true;
                    nextFleetIndex = (index + 1) % POOL_SIZE;
                    return reinterpret_cast<Fleet*>(&fleetPool[index * sizeof(Fleet)]);
                }
            }
            return nullptr;
        }
        
        void deallocateFleet(Fleet* fleet) {
            char* poolStart = fleetPool;
            char* fleetPtr = reinterpret_cast<char*>(fleet);
            
            if (fleetPtr >= poolStart && fleetPtr < poolStart + sizeof(fleetPool)) {
                size_t index = (fleetPtr - poolStart) / sizeof(Fleet);
                if (index < POOL_SIZE) {
                    fleet->~Fleet();
                    fleetUsed[index] = false;
                }
            }
        }
        
        void printPoolStats() const {
            size_t planetsUsed = planetUsed.count();
            size_t fleetsUsed = fleetUsed.count();
            
            cout << "Pool Statistics:" << endl;
            cout << "  Planets: " << planetsUsed << "/" << POOL_SIZE << " used" << endl;
            cout << "  Fleets: " << fleetsUsed << "/" << POOL_SIZE << " used" << endl;
        }
    };
    
    /**
     * @brief Cache-friendly data structure for entity processing
     */
    struct EntityDataSoA {  // Structure of Arrays for cache efficiency
        vector<Vector3D> positions;
        vector<string> names;
        vector<int> strengths;
        vector<bool> active;
        
        void reserve(size_t capacity) {
            positions.reserve(capacity);
            names.reserve(capacity);
            strengths.reserve(capacity);
            active.reserve(capacity);
        }
        
        void addEntity(const Vector3D& pos, const string& name, int strength, bool isActive) {
            positions.push_back(pos);
            names.push_back(name);
            strengths.push_back(strength);
            active.push_back(isActive);
        }
        
        size_t size() const { return positions.size(); }
        
        void clear() {
            positions.clear();
            names.clear();
            strengths.clear();
            active.clear();
        }
    };
}

/**
 * @brief Parallel processing optimizations
 */
namespace ParallelOptimizations {
    
    /**
     * @brief Parallel mission processing using thread pool
     */
    class ParallelMissionProcessor {
    private:
        ThreadPool threadPool;
        
    public:
        ParallelMissionProcessor(size_t numThreads = thread::hardware_concurrency()) 
            : threadPool(numThreads) {
            LOG_INFO("Created parallel processor with {} threads", numThreads);
        }
        
        vector<double> processDistanceCalculations(const vector<Vector3D>& points1,
                                                  const vector<Vector3D>& points2) {
            const size_t batchSize = 1000;
            const size_t totalSize = min(points1.size(), points2.size());
            
            vector<future<vector<double>>> futures;
            
            // Submit batch jobs to thread pool
            for (size_t i = 0; i < totalSize; i += batchSize) {
                size_t endIdx = min(i + batchSize, totalSize);
                
                auto future = threadPool.enqueue([&points1, &points2, i, endIdx]() {
                    vector<double> batchResults;
                    batchResults.reserve(endIdx - i);
                    
                    for (size_t j = i; j < endIdx; ++j) {
                        batchResults.push_back(points1[j].distanceTo(points2[j]));
                    }
                    
                    return batchResults;
                });
                
                futures.push_back(move(future));
            }
            
            // Collect results
            vector<double> allResults;
            allResults.reserve(totalSize);
            
            for (auto& future : futures) {
                auto batchResults = future.get();
                allResults.insert(allResults.end(), batchResults.begin(), batchResults.end());
            }
            
            return allResults;
        }
        
        /**
         * @brief Parallel STL algorithm example
         */
        void demonstrateParallelSTL(vector<int>& data) {
            // Parallel sort
            sort(execution::par_unseq, data.begin(), data.end());
            
            // Parallel transform
            transform(execution::par_unseq, data.begin(), data.end(), data.begin(),
                     [](int x) { return x * x; });
            
            // Parallel reduce
            int sum = reduce(execution::par_unseq, data.begin(), data.end(), 0);
            cout << "Parallel sum: " << sum << endl;
        }
    };
}

/**
 * @brief Algorithm optimization techniques
 */
namespace AlgorithmOptimizations {
    
    /**
     * @brief Optimized spatial partitioning for collision detection
     */
    class SpatialHashGrid {
    private:
        double cellSize;
        map<pair<int, int>, vector<Entity*>> grid;
        
    public:
        SpatialHashGrid(double cell_size) : cellSize(cell_size) {}
        
        void clear() { grid.clear(); }
        
        void insert(Entity* entity) {
            auto pos = entity->getPosition();
            int x = static_cast<int>(pos.x / cellSize);
            int y = static_cast<int>(pos.y / cellSize);
            
            grid[{x, y}].push_back(entity);
        }
        
        vector<Entity*> getNearbyEntities(const Vector3D& position, double radius) {
            vector<Entity*> nearby;
            
            int minX = static_cast<int>((position.x - radius) / cellSize);
            int maxX = static_cast<int>((position.x + radius) / cellSize);
            int minY = static_cast<int>((position.y - radius) / cellSize);
            int maxY = static_cast<int>((position.y + radius) / cellSize);
            
            for (int x = minX; x <= maxX; ++x) {
                for (int y = minY; y <= maxY; ++y) {
                    auto it = grid.find({x, y});
                    if (it != grid.end()) {
                        for (Entity* entity : it->second) {
                            if (entity->getPosition().distanceTo(position) <= radius) {
                                nearby.push_back(entity);
                            }
                        }
                    }
                }
            }
            
            return nearby;
        }
        
        void printStats() const {
            cout << "Spatial Hash Grid Statistics:" << endl;
            cout << "  Cells used: " << grid.size() << endl;
            
            if (!grid.empty()) {
                vector<size_t> cellSizes;
                for (const auto& [key, entities] : grid) {
                    cellSizes.push_back(entities.size());
                }
                
                auto minMax = minmax_element(cellSizes.begin(), cellSizes.end());
                double average = accumulate(cellSizes.begin(), cellSizes.end(), 0.0) / cellSizes.size();
                
                cout << "  Entities per cell - Min: " << *minMax.first 
                     << ", Max: " << *minMax.second 
                     << ", Avg: " << fixed << setprecision(2) << average << endl;
            }
        }
    };
    
    /**
     * @brief Optimized pathfinding with A* and heuristic improvements
     */
    class OptimizedPathfinder {
    private:
        struct Node {
            Vector3D position;
            double gCost = 0.0;  // Cost from start
            double hCost = 0.0;  // Heuristic cost to goal
            double fCost() const { return gCost + hCost; }
            Node* parent = nullptr;
            
            bool operator>(const Node& other) const {
                return fCost() > other.fCost();
            }
        };
        
        static double heuristic(const Vector3D& a, const Vector3D& b) {
            // Manhattan distance for faster computation
            return abs(a.x - b.x) + abs(a.y - b.y) + abs(a.z - b.z);
        }
        
    public:
        vector<Vector3D> findPath(const Vector3D& start, const Vector3D& goal,
                                 const vector<Vector3D>& obstacles) {
            priority_queue<Node, vector<Node>, greater<Node>> openSet;
            set<Vector3D> closedSet;
            
            Node startNode;
            startNode.position = start;
            startNode.gCost = 0.0;
            startNode.hCost = heuristic(start, goal);
            
            openSet.push(startNode);
            
            while (!openSet.empty()) {
                Node current = openSet.top();
                openSet.pop();
                
                if (current.position.distanceTo(goal) < 1.0) {
                    // Reconstruct path
                    vector<Vector3D> path;
                    Node* node = &current;
                    while (node != nullptr) {
                        path.push_back(node->position);
                        node = node->parent;
                    }
                    reverse(path.begin(), path.end());
                    return path;
                }
                
                closedSet.insert(current.position);
                
                // Generate neighbors (simplified for demo)
                vector<Vector3D> directions = {
                    {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, 
                    {0, -1, 0}, {0, 0, 1}, {0, 0, -1}
                };
                
                for (const auto& dir : directions) {
                    Vector3D neighborPos = current.position + dir * 10.0;  // 10 unit steps
                    
                    if (closedSet.count(neighborPos) || isObstructed(neighborPos, obstacles)) {
                        continue;
                    }
                    
                    Node neighbor;
                    neighbor.position = neighborPos;
                    neighbor.gCost = current.gCost + 10.0;
                    neighbor.hCost = heuristic(neighborPos, goal);
                    neighbor.parent = new Node(current);  // Note: Memory leak for simplicity
                    
                    openSet.push(neighbor);
                }
            }
            
            return {};  // No path found
        }
        
    private:
        bool isObstructed(const Vector3D& position, const vector<Vector3D>& obstacles) {
            for (const auto& obstacle : obstacles) {
                if (position.distanceTo(obstacle) < 5.0) {  // 5 unit obstacle radius
                    return true;
                }
            }
            return false;
        }
    };
}

/**
 * @brief Performance demonstration class
 */
class PerformanceDemo {
private:
    mt19937 rng{random_device{}()};
    
public:
    void runAllOptimizationTests() {
        cout << "=== Performance Optimization Demonstration ===" << endl;
        
        // Test 1: SIMD optimizations
        testSIMDOptimizations();
        
        // Test 2: Memory pool optimizations
        testMemoryPoolOptimizations();
        
        // Test 3: Parallel processing
        testParallelProcessing();
        
        // Test 4: Spatial partitioning
        testSpatialPartitioning();
        
        // Test 5: Cache optimization
        testCacheOptimization();
        
        cout << "\n=== Performance Tests Complete ===" << endl;
    }
    
private:
    void testSIMDOptimizations() {
        cout << "\n--- SIMD Optimization Test ---" << endl;
        
        const size_t numPoints = 10000;
        vector<Vector3D> points1, points2;
        
        // Generate random points
        uniform_real_distribution<double> posDist(-1000.0, 1000.0);
        for (size_t i = 0; i < numPoints; ++i) {
            points1.emplace_back(posDist(rng), posDist(rng), posDist(rng));
            points2.emplace_back(posDist(rng), posDist(rng), posDist(rng));
        }
        
        vector<double> regularResults, simdResults;
        
        // Test regular implementation
        double regularTime = PerformanceTimer::measureOperation("Regular Distance Calculation", [&]() {
            SIMDOptimizations::calculateDistancesRegular(points1, points2, regularResults);
        });
        
        // Test SIMD implementation
        double simdTime = PerformanceTimer::measureOperation("SIMD Distance Calculation", [&]() {
            SIMDOptimizations::calculateDistancesSIMD(points1, points2, simdResults);
        });
        
        // Verify results are similar
        double maxDiff = 0.0;
        for (size_t i = 0; i < regularResults.size(); ++i) {
            maxDiff = max(maxDiff, abs(regularResults[i] - simdResults[i]));
        }
        
        cout << "Results verification - Max difference: " << maxDiff << endl;
        cout << "SIMD speedup: " << fixed << setprecision(2) 
             << (regularTime / simdTime) << "x" << endl;
    }
    
    void testMemoryPoolOptimizations() {
        cout << "\n--- Memory Pool Optimization Test ---" << endl;
        
        const size_t numAllocations = 1000;
        MemoryOptimizations::OptimizedEntityPool pool;
        
        // Test pool allocation vs regular allocation
        double poolTime = PerformanceTimer::measureOperation("Pool Allocation", [&]() {
            vector<Planet*> planets;
            
            for (size_t i = 0; i < numAllocations; ++i) {
                Planet* planet = pool.allocatePlanet();
                if (planet) {
                    new (planet) Planet("Planet-" + to_string(i), Vector3D{0, 0, 0}, PlanetType::TERRESTRIAL);
                    planets.push_back(planet);
                }
            }
            
            // Cleanup
            for (Planet* planet : planets) {
                pool.deallocatePlanet(planet);
            }
        });
        
        double regularTime = PerformanceTimer::measureOperation("Regular Allocation", [&]() {
            vector<unique_ptr<Planet>> planets;
            
            for (size_t i = 0; i < numAllocations; ++i) {
                planets.push_back(make_unique<Planet>(
                    "Planet-" + to_string(i), Vector3D{0, 0, 0}, PlanetType::TERRESTRIAL
                ));
            }
            // Automatic cleanup via destructors
        });
        
        pool.printPoolStats();
        cout << "Pool allocation speedup: " << fixed << setprecision(2) 
             << (regularTime / poolTime) << "x" << endl;
    }
    
    void testParallelProcessing() {
        cout << "\n--- Parallel Processing Test ---" << endl;
        
        const size_t numPoints = 100000;
        vector<Vector3D> points1, points2;
        
        uniform_real_distribution<double> posDist(-1000.0, 1000.0);
        for (size_t i = 0; i < numPoints; ++i) {
            points1.emplace_back(posDist(rng), posDist(rng), posDist(rng));
            points2.emplace_back(posDist(rng), posDist(rng), posDist(rng));
        }
        
        ParallelOptimizations::ParallelMissionProcessor processor;
        
        // Sequential processing
        vector<double> sequentialResults;
        double sequentialTime = PerformanceTimer::measureOperation("Sequential Processing", [&]() {
            sequentialResults.resize(numPoints);
            for (size_t i = 0; i < numPoints; ++i) {
                sequentialResults[i] = points1[i].distanceTo(points2[i]);
            }
        });
        
        // Parallel processing
        vector<double> parallelResults;
        double parallelTime = PerformanceTimer::measureOperation("Parallel Processing", [&]() {
            parallelResults = processor.processDistanceCalculations(points1, points2);
        });
        
        cout << "Parallel speedup: " << fixed << setprecision(2) 
             << (sequentialTime / parallelTime) << "x" << endl;
        
        // Test parallel STL algorithms
        vector<int> data(100000);
        iota(data.begin(), data.end(), 1);
        shuffle(data.begin(), data.end(), rng);
        
        PerformanceTimer::measureOperation("Parallel STL Operations", [&]() {
            processor.demonstrateParallelSTL(data);
        });
    }
    
    void testSpatialPartitioning() {
        cout << "\n--- Spatial Partitioning Test ---" << endl;
        
        const size_t numEntities = 5000;
        vector<unique_ptr<Planet>> planets;
        
        // Create entities in a spatial distribution
        uniform_real_distribution<double> posDist(-500.0, 500.0);
        for (size_t i = 0; i < numEntities; ++i) {
            planets.push_back(make_unique<Planet>(
                "Planet-" + to_string(i),
                Vector3D{posDist(rng), posDist(rng), posDist(rng)},
                PlanetType::TERRESTRIAL
            ));
        }
        
        AlgorithmOptimizations::SpatialHashGrid spatialGrid(50.0);  // 50-unit cells
        
        // Insert entities into spatial grid
        PerformanceTimer::measureOperation("Spatial Grid Population", [&]() {
            for (const auto& planet : planets) {
                spatialGrid.insert(planet.get());
            }
        });
        
        // Test query performance
        const size_t numQueries = 1000;
        Vector3D queryPoint{0, 0, 0};
        double queryRadius = 100.0;
        
        // Brute force search
        double bruteForceTime = PerformanceTimer::measureOperation("Brute Force Search", [&]() {
            for (size_t i = 0; i < numQueries; ++i) {
                vector<Entity*> nearby;
                for (const auto& planet : planets) {
                    if (planet->getPosition().distanceTo(queryPoint) <= queryRadius) {
                        nearby.push_back(planet.get());
                    }
                }
            }
        });
        
        // Spatial grid search
        double spatialTime = PerformanceTimer::measureOperation("Spatial Grid Search", [&]() {
            for (size_t i = 0; i < numQueries; ++i) {
                auto nearby = spatialGrid.getNearbyEntities(queryPoint, queryRadius);
            }
        });
        
        spatialGrid.printStats();
        cout << "Spatial grid speedup: " << fixed << setprecision(2) 
             << (bruteForceTime / spatialTime) << "x" << endl;
    }
    
    void testCacheOptimization() {
        cout << "\n--- Cache Optimization Test ---" << endl;
        
        const size_t numEntities = 100000;
        
        // Array of Structures (AoS) - poor cache performance
        struct EntityAoS {
            Vector3D position;
            string name;
            int strength;
            bool active;
            char padding[64];  // Simulate larger structures
        };
        vector<EntityAoS> aosData(numEntities);
        
        // Structure of Arrays (SoA) - good cache performance
        MemoryOptimizations::EntityDataSoA soaData;
        soaData.reserve(numEntities);
        
        // Initialize data
        uniform_real_distribution<double> posDist(-1000.0, 1000.0);
        uniform_int_distribution<int> strengthDist(100, 1000);
        
        for (size_t i = 0; i < numEntities; ++i) {
            Vector3D pos{posDist(rng), posDist(rng), posDist(rng)};
            string name = "Entity-" + to_string(i);
            int strength = strengthDist(rng);
            bool active = (i % 3) != 0;
            
            aosData[i] = {pos, name, strength, active, {}};
            soaData.addEntity(pos, name, strength, active);
        }
        
        // Test processing performance
        volatile double sumAoS = 0.0;  // volatile to prevent optimization
        double aosTime = PerformanceTimer::measureOperation("AoS Processing", [&]() {
            for (const auto& entity : aosData) {
                if (entity.active) {
                    sumAoS += entity.position.magnitude() * entity.strength;
                }
            }
        });
        
        volatile double sumSoA = 0.0;
        double soaTime = PerformanceTimer::measureOperation("SoA Processing", [&]() {
            for (size_t i = 0; i < soaData.size(); ++i) {
                if (soaData.active[i]) {
                    sumSoA += soaData.positions[i].magnitude() * soaData.strengths[i];
                }
            }
        });
        
        cout << "Cache optimization speedup: " << fixed << setprecision(2) 
             << (aosTime / soaTime) << "x" << endl;
        cout << "Verification - AoS sum: " << sumAoS << ", SoA sum: " << sumSoA << endl;
    }
};

/**
 * @brief Main function
 */
int main() {
    cout << "CppVerseHub - Performance Optimization Example" << endl;
    cout << "==============================================" << endl;
    
    // Initialize logging
    auto& logger = Logger::getInstance();
    logger.setLevel(Logger::Level::INFO);
    logger.enableConsoleLogging(true);
    
    try {
        PerformanceDemo demo;
        demo.runAllOptimizationTests();
        
        cout << "\nPerformance optimization example completed successfully!" << endl;
        cout << "\nOptimization techniques demonstrated:" << endl;
        cout << "  - SIMD vectorization for mathematical operations" << endl;
        cout << "  - Object pooling for frequent allocations" << endl;
        cout << "  - Parallel processing with thread pools" << endl;
        cout << "  - Spatial partitioning for collision detection" << endl;
        cout << "  - Cache-friendly data structures (SoA vs AoS)" << endl;
        cout << "  - Parallel STL algorithms" << endl;
        cout << "  - Optimized pathfinding algorithms" << endl;
        
        cout << "\nKey takeaways:" << endl;
        cout << "  - Profile before optimizing" << endl;
        cout << "  - Optimize hot paths and bottlenecks first" << endl;
        cout << "  - Consider cache locality in data structure design" << endl;
        cout << "  - Use SIMD for data-parallel operations" << endl;
        cout << "  - Leverage modern C++ parallel algorithms" << endl;
        cout << "  - Memory pools reduce allocation overhead" << endl;
        
        return 0;
        
    } catch (const exception& e) {
        LOG_ERROR("Error in performance optimization example: {}", e.what());
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
}