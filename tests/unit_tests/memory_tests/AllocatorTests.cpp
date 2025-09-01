// File: tests/unit_tests/memory_tests/AllocatorTests.cpp
// Custom allocator tests for CppVerseHub memory management showcase

#include <catch2/catch.hpp>
#include <memory>
#include <vector>
#include <list>
#include <deque>
#include <set>
#include <unordered_map>
#include <chrono>
#include <atomic>
#include <algorithm>

// Include custom allocator headers
#include "PoolAllocator.hpp"
#include "StackAllocator.hpp"
#include "TrackingAllocator.hpp"
#include "MemoryTracker.hpp"

// Include core classes for testing
#include "Planet.hpp"
#include "Fleet.hpp"

using namespace CppVerseHub::Memory;
using namespace CppVerseHub::Core;
using namespace CppVerseHub::Utils;

/**
 * @brief Test fixture for allocator tests
 */
class AllocatorTestFixture {
public:
    AllocatorTestFixture() {
        MemoryTracker::resetCounters();
    }
    
    ~AllocatorTestFixture() {
        MemoryTracker::printMemoryStats("Allocator test completion");
    }
};

/**
 * @brief Simple tracking allocator for testing
 */
template<typename T>
class SimpleTrackingAllocator {
public:
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    
    template<typename U>
    struct rebind {
        using other = SimpleTrackingAllocator<U>;
    };
    
private:
    static std::atomic<size_t> allocations_;
    static std::atomic<size_t> deallocations_;
    static std::atomic<size_t> bytesAllocated_;
    static std::atomic<size_t> bytesDeallocated_;
    
public:
    SimpleTrackingAllocator() noexcept = default;
    
    template<typename U>
    SimpleTrackingAllocator(const SimpleTrackingAllocator<U>&) noexcept {}
    
    pointer allocate(size_type n) {
        size_t bytes = n * sizeof(T);
        allocations_.fetch_add(1);
        bytesAllocated_.fetch_add(bytes);
        
        pointer p = static_cast<pointer>(std::malloc(bytes));
        if (!p) {
            throw std::bad_alloc();
        }
        return p;
    }
    
    void deallocate(pointer p, size_type n) noexcept {
        if (p) {
            size_t bytes = n * sizeof(T);
            deallocations_.fetch_add(1);
            bytesDeallocated_.fetch_add(bytes);
            std::free(p);
        }
    }
    
    template<typename U, typename... Args>
    void construct(U* p, Args&&... args) {
        new(p) U(std::forward<Args>(args)...);
    }
    
    template<typename U>
    void destroy(U* p) {
        p->~U();
    }
    
    bool operator==(const SimpleTrackingAllocator& other) const noexcept {
        return true; // All instances are equivalent
    }
    
    bool operator!=(const SimpleTrackingAllocator& other) const noexcept {
        return false;
    }
    
    // Static methods for tracking
    static size_t getAllocations() { return allocations_.load(); }
    static size_t getDeallocations() { return deallocations_.load(); }
    static size_t getBytesAllocated() { return bytesAllocated_.load(); }
    static size_t getBytesDeallocated() { return bytesDeallocated_.load(); }
    static size_t getActiveAllocations() { 
        return getAllocations() - getDeallocations(); 
    }
    static size_t getActiveBytes() { 
        return getBytesAllocated() - getBytesDeallocated(); 
    }
    
    static void resetCounters() {
        allocations_.store(0);
        deallocations_.store(0);
        bytesAllocated_.store(0);
        bytesDeallocated_.store(0);
    }
};

template<typename T>
std::atomic<size_t> SimpleTrackingAllocator<T>::allocations_{0};

template<typename T>
std::atomic<size_t> SimpleTrackingAllocator<T>::deallocations_{0};

template<typename T>
std::atomic<size_t> SimpleTrackingAllocator<T>::bytesAllocated_{0};

template<typename T>
std::atomic<size_t> SimpleTrackingAllocator<T>::bytesDeallocated_{0};

/**
 * @brief Pool allocator for fixed-size objects
 */
template<typename T, size_t PoolSize = 1024>
class PoolAllocator {
public:
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using size_type = std::size_t;
    
    template<typename U>
    struct rebind {
        using other = PoolAllocator<U, PoolSize>;
    };
    
private:
    struct Block {
        alignas(T) char data[sizeof(T)];
        Block* next;
    };
    
    Block* freeList_;
    std::unique_ptr<Block[]> pool_;
    size_t poolIndex_;
    bool initialized_;
    
    void initialize() {
        if (initialized_) return;
        
        pool_ = std::make_unique<Block[]>(PoolSize);
        freeList_ = nullptr;
        poolIndex_ = 0;
        initialized_ = true;
        
        // Initialize free list
        for (size_t i = 0; i < PoolSize - 1; ++i) {
            pool_[i].next = &pool_[i + 1];
        }
        pool_[PoolSize - 1].next = nullptr;
        freeList_ = &pool_[0];
    }
    
public:
    PoolAllocator() : freeList_(nullptr), poolIndex_(0), initialized_(false) {}
    
    template<typename U>
    PoolAllocator(const PoolAllocator<U, PoolSize>& other) noexcept 
        : freeList_(nullptr), poolIndex_(0), initialized_(false) {}
    
    pointer allocate(size_type n) {
        if (n != 1) {
            // Fall back to standard allocation for multi-object requests
            return static_cast<pointer>(std::malloc(n * sizeof(T)));
        }
        
        initialize();
        
        if (freeList_) {
            Block* block = freeList_;
            freeList_ = freeList_->next;
            return reinterpret_cast<pointer>(block);
        }
        
        // Pool exhausted, fall back to standard allocation
        return static_cast<pointer>(std::malloc(sizeof(T)));
    }
    
    void deallocate(pointer p, size_type n) noexcept {
        if (n != 1) {
            std::free(p);
            return;
        }
        
        // Check if pointer is from our pool
        Block* block = reinterpret_cast<Block*>(p);
        if (block >= pool_.get() && block < pool_.get() + PoolSize) {
            block->next = freeList_;
            freeList_ = block;
        } else {
            std::free(p);
        }
    }
    
    template<typename U, typename... Args>
    void construct(U* p, Args&&... args) {
        new(p) U(std::forward<Args>(args)...);
    }
    
    template<typename U>
    void destroy(U* p) {
        p->~U();
    }
    
    bool operator==(const PoolAllocator& other) const noexcept {
        return this == &other;
    }
    
    bool operator!=(const PoolAllocator& other) const noexcept {
        return !(*this == other);
    }
};

/**
 * @brief Stack allocator for linear allocation pattern
 */
template<typename T, size_t StackSize = 4096>
class StackAllocator {
public:
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using size_type = std::size_t;
    
    template<typename U>
    struct rebind {
        using other = StackAllocator<U, StackSize>;
    };
    
private:
    alignas(T) char stack_[StackSize];
    size_t offset_;
    bool initialized_;
    
    void initialize() {
        if (!initialized_) {
            offset_ = 0;
            initialized_ = true;
        }
    }
    
public:
    StackAllocator() : offset_(0), initialized_(false) {}
    
    template<typename U>
    StackAllocator(const StackAllocator<U, StackSize>& other) noexcept 
        : offset_(0), initialized_(false) {}
    
    pointer allocate(size_type n) {
        initialize();
        
        size_t bytes = n * sizeof(T);
        size_t aligned_bytes = (bytes + alignof(T) - 1) & ~(alignof(T) - 1);
        
        if (offset_ + aligned_bytes > StackSize) {
            // Stack exhausted, fall back to heap allocation
            return static_cast<pointer>(std::malloc(bytes));
        }
        
        pointer p = reinterpret_cast<pointer>(stack_ + offset_);
        offset_ += aligned_bytes;
        return p;
    }
    
    void deallocate(pointer p, size_type n) noexcept {
        // For stack allocator, we typically don't deallocate individual objects
        // In a real implementation, you might track allocation order for proper cleanup
        char* ptr = reinterpret_cast<char*>(p);
        if (ptr < stack_ || ptr >= stack_ + StackSize) {
            // Not from our stack, must be heap allocated
            std::free(p);
        }
        // Otherwise, memory will be reclaimed when allocator is destroyed
    }
    
    template<typename U, typename... Args>
    void construct(U* p, Args&&... args) {
        new(p) U(std::forward<Args>(args)...);
    }
    
    template<typename U>
    void destroy(U* p) {
        p->~U();
    }
    
    void reset() {
        offset_ = 0;
    }
    
    size_t getBytesUsed() const {
        return offset_;
    }
    
    size_t getBytesAvailable() const {
        return StackSize - offset_;
    }
    
    bool operator==(const StackAllocator& other) const noexcept {
        return this == &other;
    }
    
    bool operator!=(const StackAllocator& other) const noexcept {
        return !(*this == other);
    }
};

TEST_CASE_METHOD(AllocatorTestFixture, "Basic Custom Allocator Usage", "[allocators][basic][usage]") {
    
    SECTION("Simple tracking allocator with vector") {
        SimpleTrackingAllocator<int>::resetCounters();
        
        {
            std::vector<int, SimpleTrackingAllocator<int>> vec;
            
            // Add elements to trigger allocations
            for (int i = 0; i < 100; ++i) {
                vec.push_back(i);
            }
            
            REQUIRE(vec.size() == 100);
            REQUIRE(SimpleTrackingAllocator<int>::getAllocations() > 0);
            REQUIRE(SimpleTrackingAllocator<int>::getBytesAllocated() > 0);
        } // vector destroyed here
        
        // All allocations should be deallocated
        REQUIRE(SimpleTrackingAllocator<int>::getActiveAllocations() == 0);
        REQUIRE(SimpleTrackingAllocator<int>::getActiveBytes() == 0);
    }
    
    SECTION("Tracking allocator with custom objects") {
        SimpleTrackingAllocator<Planet>::resetCounters();
        
        {
            std::vector<Planet, SimpleTrackingAllocator<Planet>> planets;
            
            for (int i = 0; i < 10; ++i) {
                planets.emplace_back("TrackingPlanet_" + std::to_string(i), 
                                   Vector3D{i * 100.0, i * 100.0, i * 100.0});
                planets.back().setResourceAmount(ResourceType::MINERALS, 1000 + i * 100);
            }
            
            REQUIRE(planets.size() == 10);
            REQUIRE(SimpleTrackingAllocator<Planet>::getAllocations() > 0);
            
            // Verify planets are properly constructed
            REQUIRE(planets[5].getName() == "TrackingPlanet_5");
            REQUIRE(planets[5].getResourceAmount(ResourceType::MINERALS) == 1500);
        }
        
        REQUIRE(SimpleTrackingAllocator<Planet>::getActiveAllocations() == 0);
    }
    
    SECTION("Allocator with different container types") {
        SimpleTrackingAllocator<int>::resetCounters();
        
        // Test with list
        {
            std::list<int, SimpleTrackingAllocator<int>> myList;
            for (int i = 0; i < 50; ++i) {
                myList.push_back(i);
            }
            REQUIRE(myList.size() == 50);
        }
        
        size_t listAllocations = SimpleTrackingAllocator<int>::getAllocations();
        
        // Test with deque
        {
            std::deque<int, SimpleTrackingAllocator<int>> myDeque;
            for (int i = 0; i < 50; ++i) {
                myDeque.push_back(i);
            }
            REQUIRE(myDeque.size() == 50);
        }
        
        REQUIRE(SimpleTrackingAllocator<int>::getAllocations() > listAllocations);
        REQUIRE(SimpleTrackingAllocator<int>::getActiveAllocations() == 0);
    }
}

TEST_CASE_METHOD(AllocatorTestFixture, "Pool Allocator Implementation", "[allocators][pool][performance]") {
    
    SECTION("Basic pool allocator functionality") {
        {
            std::vector<int, PoolAllocator<int, 100>> poolVec;
            
            // Add elements within pool capacity
            for (int i = 0; i < 50; ++i) {
                poolVec.push_back(i);
            }
            
            REQUIRE(poolVec.size() == 50);
            
            // Verify elements are correct
            for (size_t i = 0; i < poolVec.size(); ++i) {
                REQUIRE(poolVec[i] == static_cast<int>(i));
            }
        }
    }
    
    SECTION("Pool allocator with game objects") {
        const size_t poolSize = 20;
        std::vector<Planet, PoolAllocator<Planet, poolSize>> pooledPlanets;
        
        // Create planets within pool capacity
        for (int i = 0; i < 15; ++i) {
            pooledPlanets.emplace_back("PoolPlanet_" + std::to_string(i),
                                     Vector3D{i * 50.0, i * 50.0, i * 50.0});
        }
        
        REQUIRE(pooledPlanets.size() == 15);
        
        // Verify planets are properly constructed
        REQUIRE(pooledPlanets[0].getName() == "PoolPlanet_0");
        REQUIRE(pooledPlanets[14].getName() == "PoolPlanet_14");
        
        // Test pool overflow (should fall back to standard allocation)
        for (int i = 15; i < 25; ++i) {
            pooledPlanets.emplace_back("OverflowPlanet_" + std::to_string(i),
                                     Vector3D{i * 50.0, i * 50.0, i * 50.0});
        }
        
        REQUIRE(pooledPlanets.size() == 25);
        REQUIRE(pooledPlanets[20].getName() == "OverflowPlanet_20");
    }
    
    SECTION("Pool allocator performance comparison") {
        const int iterations = 10000;
        
        // Test standard allocator performance
        auto start = std::chrono::high_resolution_clock::now();
        
        {
            std::vector<int> standardVec;
            for (int i = 0; i < iterations; ++i) {
                standardVec.push_back(i);
                if (i % 100 == 0) {
                    standardVec.clear(); // Force reallocation
                }
            }
        }
        
        auto standardTime = std::chrono::high_resolution_clock::now() - start;
        
        // Test pool allocator performance
        start = std::chrono::high_resolution_clock::now();
        
        {
            std::vector<int, PoolAllocator<int, 1000>> poolVec;
            for (int i = 0; i < iterations; ++i) {
                poolVec.push_back(i);
                if (i % 100 == 0) {
                    poolVec.clear(); // Pool memory reused
                }
            }
        }
        
        auto poolTime = std::chrono::high_resolution_clock::now() - start;
        
        auto standardMs = std::chrono::duration_cast<std::chrono::milliseconds>(standardTime).count();
        auto poolMs = std::chrono::duration_cast<std::chrono::milliseconds>(poolTime).count();
        
        INFO("Standard allocator time: " << standardMs << "ms");
        INFO("Pool allocator time: " << poolMs << "ms");
        
        // Both should complete in reasonable time
        REQUIRE(standardMs < 1000); // Less than 1 second
        REQUIRE(poolMs < 1000);
    }
}

TEST_CASE_METHOD(AllocatorTestFixture, "Stack Allocator Implementation", "[allocators][stack][linear]") {
    
    SECTION("Basic stack allocator functionality") {
        StackAllocator<int, 1024> stackAlloc;
        
        {
            std::vector<int, StackAllocator<int, 1024>> stackVec(stackAlloc);
            
            // Add elements that fit in stack
            for (int i = 0; i < 50; ++i) {
                stackVec.push_back(i);
            }
            
            REQUIRE(stackVec.size() == 50);
            REQUIRE(stackAlloc.getBytesUsed() > 0);
            REQUIRE(stackAlloc.getBytesAvailable() < 1024);
        }
        
        // After vector destruction, stack space is still used
        // (typical behavior for stack allocators)
    }
    
    SECTION("Stack allocator with game objects") {
        StackAllocator<Fleet, 2048> stackAlloc;
        
        {
            std::vector<Fleet, StackAllocator<Fleet, 2048>> stackFleets(stackAlloc);
            
            for (int i = 0; i < 5; ++i) {
                stackFleets.emplace_back("StackFleet_" + std::to_string(i),
                                       Vector3D{i * 100.0, i * 100.0, i * 100.0});
                stackFleets.back().addShips(ShipType::FIGHTER, 10 + i);
            }
            
            REQUIRE(stackFleets.size() == 5);
            REQUIRE(stackFleets[2].getName() == "StackFleet_2");
            REQUIRE(stackFleets[3].getShipCount(ShipType::FIGHTER) == 13);
        }
    }
    
    SECTION("Stack allocator reset functionality") {
        StackAllocator<int, 512> stackAlloc;
        
        // First allocation batch
        {
            std::vector<int, StackAllocator<int, 512>> batch1(stackAlloc);
            for (int i = 0; i < 20; ++i) {
                batch1.push_back(i);
            }
            REQUIRE(batch1.size() == 20);
        }
        
        size_t usedAfterFirst = stackAlloc.getBytesUsed();
        REQUIRE(usedAfterFirst > 0);
        
        // Reset the stack allocator
        stackAlloc.reset();
        REQUIRE(stackAlloc.getBytesUsed() == 0);
        REQUIRE(stackAlloc.getBytesAvailable() == 512);
        
        // Second allocation batch after reset
        {
            std::vector<int, StackAllocator<int, 512>> batch2(stackAlloc);
            for (int i = 0; i < 30; ++i) {
                batch2.push_back(i * 2);
            }
            REQUIRE(batch2.size() == 30);
        }
    }
}

TEST_CASE_METHOD(AllocatorTestFixture, "Allocator Performance Analysis", "[allocators][performance][comparison]") {
    
    SECTION("Allocation pattern analysis") {
        const int objectCount = 1000;
        
        // Test frequent allocation/deallocation pattern
        SimpleTrackingAllocator<int>::resetCounters();
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < objectCount; ++i) {
            std::vector<int, SimpleTrackingAllocator<int>> temp;
            temp.reserve(100); // Pre-allocate to reduce reallocations
            for (int j = 0; j < 100; ++j) {
                temp.push_back(j);
            }
            // temp destructor deallocates
        }
        
        auto trackingTime = std::chrono::high_resolution_clock::now() - start;
        
        size_t totalAllocations = SimpleTrackingAllocator<int>::getAllocations();
        size_t totalBytes = SimpleTrackingAllocator<int>::getBytesAllocated();
        
        INFO("Tracking allocator - " << objectCount << " iterations:");
        INFO("Total allocations: " << totalAllocations);
        INFO("Total bytes allocated: " << totalBytes);
        INFO("Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(trackingTime).count() << "ms");
        
        REQUIRE(SimpleTrackingAllocator<int>::getActiveAllocations() == 0);
        REQUIRE(totalAllocations == totalBytes / sizeof(int) / 100);
    }
    
    SECTION("Memory fragmentation simulation") {
        SimpleTrackingAllocator<char>::resetCounters();
        
        std::vector<std::unique_ptr<char[], void(*)(char*)>> allocations;
        
        // Allocate various sizes to simulate fragmentation
        std::vector<size_t> sizes = {64, 128, 256, 512, 1024, 2048};
        
        for (int round = 0; round < 100; ++round) {
            for (size_t size : sizes) {
                SimpleTrackingAllocator<char> alloc;
                char* ptr = alloc.allocate(size);
                
                // Wrap in unique_ptr with custom deleter
                allocations.emplace_back(
                    ptr,
                    [size](char* p) {
                        SimpleTrackingAllocator<char> alloc;
                        alloc.deallocate(p, size);
                    }
                );
            }
        }
        
        size_t peakAllocations = SimpleTrackingAllocator<char>::getAllocations();
        size_t peakBytes = SimpleTrackingAllocator<char>::getBytesAllocated();
        
        // Deallocate half randomly to simulate fragmentation
        std::random_device rd;
        std::mt19937 gen(rd());
        std::shuffle(allocations.begin(), allocations.end(), gen);
        
        size_t halfSize = allocations.size() / 2;
        allocations.erase(allocations.begin(), allocations.begin() + halfSize);
        
        INFO("Peak allocations: " << peakAllocations);
        INFO("Peak bytes: " << peakBytes);
        INFO("Active after partial cleanup: " << SimpleTrackingAllocator<char>::getActiveAllocations());
        
        allocations.clear(); // Clean up remaining
        REQUIRE(SimpleTrackingAllocator<char>::getActiveAllocations() == 0);
    }
    
    SECTION("Container growth patterns") {
        SimpleTrackingAllocator<int>::resetCounters();
        
        {
            std::vector<int, SimpleTrackingAllocator<int>> growingVector;
            
            // Track how many allocations occur during growth
            for (int i = 0; i < 1000; ++i) {
                size_t oldAllocations = SimpleTrackingAllocator<int>::getAllocations();
                growingVector.push_back(i);
                size_t newAllocations = SimpleTrackingAllocator<int>::getAllocations();
                
                if (newAllocations > oldAllocations) {
                    INFO("Reallocation occurred at size " << growingVector.size() << 
                         " (capacity: " << growingVector.capacity() << ")");
                }
            }
            
            REQUIRE(growingVector.size() == 1000);
        }
        
        size_t vectorAllocations = SimpleTrackingAllocator<int>::getAllocations();
        
        // Compare with pre-reserved vector
        SimpleTrackingAllocator<int>::resetCounters();
        
        {
            std::vector<int, SimpleTrackingAllocator<int>> reservedVector;
            reservedVector.reserve(1000); // Pre-allocate
            
            for (int i = 0; i < 1000; ++i) {
                reservedVector.push_back(i);
            }
            
            REQUIRE(reservedVector.size() == 1000);
        }
        
        size_t reservedAllocations = SimpleTrackingAllocator<int>::getAllocations();
        
        INFO("Growing vector allocations: " << vectorAllocations);
        INFO("Reserved vector allocations: " << reservedAllocations);
        
        // Reserved vector should have fewer allocations
        REQUIRE(reservedAllocations <= vectorAllocations);
    }
}

TEST_CASE_METHOD(AllocatorTestFixture, "Advanced Allocator Features", "[allocators][advanced][features]") {
    
    SECTION("Allocator rebinding") {
        // Test that allocator can be rebound to different types
        using IntAllocator = SimpleTrackingAllocator<int>;
        using StringAllocator = typename IntAllocator::template rebind<std::string>::other;
        
        SimpleTrackingAllocator<int>::resetCounters();
        SimpleTrackingAllocator<std::string>::resetCounters();
        
        {
            std::vector<int, IntAllocator> intVec;
            std::vector<std::string, StringAllocator> stringVec;
            
            for (int i = 0; i < 10; ++i) {
                intVec.push_back(i);
                stringVec.push_back("String_" + std::to_string(i));
            }
            
            REQUIRE(intVec.size() == 10);
            REQUIRE(stringVec.size() == 10);
        }
        
        // Both allocator types should show activity
        REQUIRE(SimpleTrackingAllocator<int>::getAllocations() > 0);
        REQUIRE(SimpleTrackingAllocator<std::string>::getAllocations() > 0);
    }
    
    SECTION("Stateful allocator behavior") {
        class StatefulAllocator {
        private:
            std::shared_ptr<size_t> allocationCount_;
            
        public:
            using value_type = int;
            
            StatefulAllocator() : allocationCount_(std::make_shared<size_t>(0)) {}
            
            template<typename U>
            StatefulAllocator(const StatefulAllocator& other) 
                : allocationCount_(other.allocationCount_) {}
            
            int* allocate(size_t n) {
                (*allocationCount_)++;
                return static_cast<int*>(std::malloc(n * sizeof(int)));
            }
            
            void deallocate(int* p, size_t n) {
                std::free(p);
            }
            
            size_t getAllocationCount() const {
                return *allocationCount_;
            }
            
            bool operator==(const StatefulAllocator& other) const {
                return allocationCount_ == other.allocationCount_;
            }
            
            bool operator!=(const StatefulAllocator& other) const {
                return !(*this == other);
            }
        };
        
        StatefulAllocator alloc1;
        StatefulAllocator alloc2(alloc1); // Share state
        
        std::vector<int, StatefulAllocator> vec1(alloc1);
        std::vector<int, StatefulAllocator> vec2(alloc2);
        
        vec1.push_back(1);
        vec2.push_back(2);
        
        // Both vectors share allocation counter
        REQUIRE(alloc1.getAllocationCount() == alloc2.getAllocationCount());
        REQUIRE(alloc1.getAllocationCount() >= 2); // At least 2 allocations
    }
    
    SECTION("Memory alignment requirements") {
        struct AlignedStruct {
            alignas(64) double data[8]; // Force 64-byte alignment
            int id;
            
            AlignedStruct(int i) : id(i) {
                for (int j = 0; j < 8; ++j) {
                    data[j] = i * j;
                }
            }
        };
        
        std::vector<AlignedStruct, SimpleTrackingAllocator<AlignedStruct>> alignedVec;
        
        for (int i = 0; i < 10; ++i) {
            alignedVec.emplace_back(i);
        }
        
        REQUIRE(alignedVec.size() == 10);
        
        // Verify alignment
        for (size_t i = 0; i < alignedVec.size(); ++i) {
            void* ptr = &alignedVec[i];
            REQUIRE(reinterpret_cast<uintptr_t>(ptr) % 64 == 0);
            REQUIRE(alignedVec[i].id == static_cast<int>(i));
        }
    }
}

TEST_CASE_METHOD(AllocatorTestFixture, "Real-World Allocator Applications", "[allocators][real-world][applications]") {
    
    SECTION("Game object pool management") {
        // Simulate a game object pool for frequently created/destroyed objects
        using ProjectilePool = PoolAllocator<Vector3D, 1000>;
        
        std::vector<Vector3D, ProjectilePool> activeProjectiles;
        ProjectilePool projectileAlloc;
        
        // Simulate projectile creation and destruction
        for (int frame = 0; frame < 100; ++frame) {
            // Create new projectiles
            for (int i = 0; i < 10; ++i) {
                activeProjectiles.emplace_back(
                    frame * 10.0 + i,
                    frame * 5.0,
                    0.0
                );
            }
            
            // Remove old projectiles (simulate out of bounds)
            if (activeProjectiles.size() > 50) {
                activeProjectiles.erase(
                    activeProjectiles.begin(),
                    activeProjectiles.begin() + 10
                );
            }
        }
        
        REQUIRE(activeProjectiles.size() > 0);
        REQUIRE(activeProjectiles.size() <= 100); // Should be bounded
    }
    
    SECTION("Temporary calculation buffer") {
        // Simulate using stack allocator for temporary calculations
        StackAllocator<double, 8192> calcAlloc;
        
        auto performCalculation = [&calcAlloc](int size) -> double {
            std::vector<double, StackAllocator<double, 8192>> tempBuffer(calcAlloc);
            
            // Fill with calculation data
            for (int i = 0; i < size; ++i) {
                tempBuffer.push_back(std::sin(i * 0.1) * std::cos(i * 0.1));
            }
            
            // Perform some calculation
            double sum = 0.0;
            for (double value : tempBuffer) {
                sum += value;
            }
            
            return sum;
            // tempBuffer destroyed, but stack memory remains allocated
        };
        
        size_t initialBytes = calcAlloc.getBytesUsed();
        
        double result1 = performCalculation(100);
        double result2 = performCalculation(200);
        double result3 = performCalculation(150);
        
        REQUIRE(result1 != 0.0);
        REQUIRE(result2 != 0.0);
        REQUIRE(result3 != 0.0);
        
        // Stack should have grown
        REQUIRE(calcAlloc.getBytesUsed() > initialBytes);
        
        // Reset for next calculation batch
        calcAlloc.reset();
        REQUIRE(calcAlloc.getBytesUsed() == 0);
    }
    
    SECTION("Memory usage profiling") {
        // Demonstrate memory usage profiling with tracking allocator
        SimpleTrackingAllocator<Planet>::resetCounters();
        
        auto createPlanetSystem = []() {
            std::vector<Planet, SimpleTrackingAllocator<Planet>> system;
            
            for (int i = 0; i < 50; ++i) {
                system.emplace_back("SystemPlanet_" + std::to_string(i),
                                  Vector3D{i * 200.0, i * 200.0, i * 200.0});
                system.back().setResourceAmount(ResourceType::MINERALS, 1000 + i * 50);
                system.back().setResourceAmount(ResourceType::ENERGY, 800 + i * 30);
            }
            
            return system.size();
        };
        
        size_t planetCount = createPlanetSystem();
        
        REQUIRE(planetCount == 50);
        
        size_t allocations = SimpleTrackingAllocator<Planet>::getAllocations();
        size_t bytesAllocated = SimpleTrackingAllocator<Planet>::getBytesAllocated();
        size_t bytesPerPlanet = bytesAllocated / planetCount;
        
        INFO("Planet system creation:");
        INFO("Total planets: " << planetCount);
        INFO("Total allocations: " << allocations);
        INFO("Total bytes: " << bytesAllocated);
        INFO("Average bytes per planet: " << bytesPerPlanet);
        
        REQUIRE(SimpleTrackingAllocator<Planet>::getActiveAllocations() == 0);
    }
}