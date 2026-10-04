/**
 * @file MemoryPoolsTests.cpp
 * @brief Tests for FixedSizePool, PoolAllocator, ThreadSafePool, SmallObjectAllocator, ObjectPool.
 */

#include "memory/MemoryPools.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <algorithm>
#include <list>
#include <map>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

using namespace CppVerseHub::Memory;

namespace {
struct Counted {
    static inline int live = 0;
    static inline bool throw_next = false;
    int value;
    explicit Counted(int v) : value(v) {
        if (throw_next) {
            throw std::runtime_error("construction failed");
        }
        ++live;
    }
    Counted(const Counted&) = delete;
    Counted& operator=(const Counted&) = delete;
    ~Counted() { --live; }
};

struct alignas(32) Aligned32 {
    float data[8]{};
};
} // namespace

TEST_CASE("FixedSizePool validates its parameters", "[memory][pools][fixed]") {
    CHECK_THROWS_AS(FixedSizePool(0), std::invalid_argument);
    CHECK_THROWS_AS(FixedSizePool(8, 3), std::invalid_argument);
    CHECK_THROWS_AS(FixedSizePool(8, 8, 0), std::invalid_argument);
    CHECK_THROWS_AS(FixedSizePool(8, 8, 4, 0, nullptr), std::invalid_argument);
    const FixedSizePool pool(1, 1);
    CHECK(pool.stride() >= sizeof(void*)); // must hold a free-list link
    CHECK(pool.chunk_count() == 0);        // lazy
}

TEST_CASE("FixedSizePool blocks are distinct, aligned and non-overlapping", "[memory][pools][fixed]") {
    const auto block = GENERATE(as<std::size_t>{}, 1, 8, 13, 24, 64, 100);
    const auto align = GENERATE(as<std::size_t>{}, 8, 16, 64);
    FixedSizePool pool(block, align, 16);
    std::vector<std::byte*> blocks;
    for (int i = 0; i < 50; ++i) {
        auto* p = static_cast<std::byte*>(pool.allocate());
        REQUIRE(AllocatorUtils::is_aligned(p, align));
        REQUIRE(pool.owns(p));
        std::fill_n(p, block, std::byte{0xAB}); // usable for the full block size
        blocks.push_back(p);
    }
    std::sort(blocks.begin(), blocks.end(), std::less<>{});
    for (std::size_t i = 1; i < blocks.size(); ++i) {
        REQUIRE(static_cast<std::size_t>(blocks[i] - blocks[i - 1]) >= block);
    }
    CHECK(pool.blocks_in_use() == 50);
    CHECK(pool.chunk_count() == 4);
    CHECK(pool.capacity() == 64);
    for (auto* p : blocks) {
        pool.deallocate(p);
    }
    CHECK(pool.blocks_in_use() == 0);
}

TEST_CASE("FixedSizePool reuses freed blocks (LIFO) without growing", "[memory][pools][fixed]") {
    FixedSizePool pool(32, 8, 4);
    void* a = pool.allocate();
    void* b = pool.allocate();
    pool.deallocate(a);
    CHECK(pool.allocate() == a);
    pool.deallocate(b);
    pool.deallocate(nullptr);
    CHECK(pool.blocks_in_use() == 1);
    CHECK(pool.chunk_count() == 1);
    int local = 0;
    CHECK_FALSE(pool.owns(&local));
}

TEST_CASE("FixedSizePool respects its chunk limit", "[memory][pools][fixed][exceptions]") {
    FixedSizePool pool(16, 8, 4, 2);
    std::vector<void*> blocks;
    for (int i = 0; i < 8; ++i) {
        blocks.push_back(pool.allocate());
    }
    CHECK_THROWS_AS(pool.allocate(), std::bad_alloc);
    CHECK(pool.try_allocate() == nullptr);
    CHECK(pool.blocks_in_use() == 8);
    pool.deallocate(blocks.back());
    CHECK(pool.try_allocate() == blocks.back());
}

TEST_CASE("FixedSizePool returns all chunks to upstream", "[memory][pools][fixed]") {
    TrackingMemoryResource upstream;
    {
        FixedSizePool pool(48, 16, 8, 0, &upstream);
        for (int i = 0; i < 30; ++i) {
            static_cast<void>(pool.allocate());
        }
        CHECK(upstream.stats().outstanding() == 4);
        pool.release();
        CHECK(upstream.stats().outstanding() == 0);
        CHECK(pool.blocks_in_use() == 0);
        static_cast<void>(pool.allocate());
    }
    CHECK_FALSE(upstream.stats().has_leaks());
    CHECK(upstream.misaligned_count() == 0);
}

TEST_CASE("FixedSizePool survives upstream failure", "[memory][pools][fixed][exceptions]") {
    FixedSizePool pool(16, 8, 4, 0, std::pmr::null_memory_resource());
    CHECK_THROWS_AS(pool.allocate(), std::bad_alloc);
    CHECK(pool.chunk_count() == 0);
    CHECK(pool.blocks_in_use() == 0);
}

TEST_CASE("PoolAllocator serves std::list nodes from the pool", "[memory][pools][allocator]") {
    TrackingMemoryResource upstream;
    {
        FixedSizePool pool(64, alignof(std::max_align_t), 32, 0, &upstream);
        {
            std::list<int, PoolAllocator<int>> lst{PoolAllocator<int>(pool)};
            for (int i = 0; i < 100; ++i) {
                lst.push_back(i);
            }
            CHECK(pool.blocks_in_use() >= 100);
            lst.remove_if([](int x) { return x % 2 == 0; });
            CHECK(pool.blocks_in_use() >= 50);
            CHECK(pool.blocks_in_use() < 100);
            CHECK(lst.size() == 50);
        }
        CHECK(pool.blocks_in_use() == 0);
    }
    CHECK_FALSE(upstream.stats().has_leaks());
}

TEST_CASE("PoolAllocator with std::map, std::set and std::vector fallback", "[memory][pools][allocator]") {
    FixedSizePool pool(96, alignof(std::max_align_t), 16);
    {
        using MapAlloc = PoolAllocator<std::pair<const int, std::string>>;
        std::map<int, std::string, std::less<>, MapAlloc> m{MapAlloc(pool)};
        std::mt19937 rng(7);
        for (int i = 0; i < 200; ++i) {
            m[static_cast<int>(rng() % 1000)] = "v";
        }
        std::set<double, std::less<>, PoolAllocator<double>> s{PoolAllocator<double>(pool)};
        s.insert({1.0, 2.0, 3.0});
        CHECK(pool.blocks_in_use() >= m.size() + s.size());

        // Arrays (n > 1) bypass the pool.
        const auto before = pool.blocks_in_use();
        std::vector<int, PoolAllocator<int>> v(100, 7, PoolAllocator<int>(pool));
        CHECK(pool.blocks_in_use() == before);
        CHECK(v[99] == 7);
    }
    CHECK(pool.blocks_in_use() == 0);
}

TEST_CASE("PoolAllocator falls back for types that do not fit", "[memory][pools][allocator][alignment]") {
    FixedSizePool small_pool(8, 8, 8);
    PoolAllocator<Aligned32> alloc(small_pool);
    Aligned32* p = alloc.allocate(1);
    CHECK(AllocatorUtils::is_aligned(p, 32));
    CHECK_FALSE(small_pool.owns(p));
    CHECK(small_pool.blocks_in_use() == 0);
    alloc.deallocate(p, 1);

    FixedSizePool other(8);
    PoolAllocator<int> a(small_pool);
    PoolAllocator<double> b(a);
    CHECK(a == b);
    CHECK(a != PoolAllocator<int>(other));
}

TEST_CASE("ThreadSafePool under concurrent use", "[memory][pools][threads]") {
    ThreadSafePool pool(32, 16, 16);
    constexpr int kThreads = 4;
    constexpr int kPerThread = 500;
    std::vector<std::vector<void*>> results(kThreads);
    {
        std::vector<std::thread> workers;
        for (int t = 0; t < kThreads; ++t) {
            workers.emplace_back([&pool, &results, t] {
                for (int i = 0; i < kPerThread; ++i) {
                    results[static_cast<std::size_t>(t)].push_back(pool.allocate());
                    if (i % 3 == 0) {
                        pool.deallocate(results[static_cast<std::size_t>(t)].back());
                        results[static_cast<std::size_t>(t)].pop_back();
                    }
                }
            });
        }
        for (auto& w : workers) {
            w.join();
        }
    }
    std::unordered_set<void*> unique;
    std::size_t total = 0;
    for (const auto& r : results) {
        total += r.size();
        unique.insert(r.begin(), r.end());
    }
    CHECK(unique.size() == total); // no block handed out twice
    CHECK(pool.blocks_in_use() == total);
    for (const auto& r : results) {
        for (void* p : r) {
            pool.deallocate(p);
        }
    }
    CHECK(pool.blocks_in_use() == 0);
    CHECK(pool.capacity() >= total);
}

TEST_CASE("SmallObjectAllocator size classes", "[memory][pools][small]") {
    STATIC_REQUIRE(SmallObjectAllocator::size_class(0) == 0);
    STATIC_REQUIRE(SmallObjectAllocator::size_class(1) == 0);
    STATIC_REQUIRE(SmallObjectAllocator::size_class(16) == 0);
    STATIC_REQUIRE(SmallObjectAllocator::size_class(17) == 1);
    STATIC_REQUIRE(SmallObjectAllocator::size_class(256) == SmallObjectAllocator::kClassCount - 1);
    STATIC_REQUIRE(SmallObjectAllocator::size_class(257) == SmallObjectAllocator::kClassCount);
}

TEST_CASE("SmallObjectAllocator routes small and large requests", "[memory][pools][small]") {
    TrackingMemoryResource upstream;
    {
        SmallObjectAllocator alloc(&upstream);
        void* s = alloc.allocate(24, 8);
        void* l = alloc.allocate(1000, 8);
        void* over = alloc.allocate(32, 64);
        CHECK(AllocatorUtils::is_aligned(s, 8));
        CHECK(AllocatorUtils::is_aligned(over, 64));
        CHECK(alloc.small_allocations() == 1);
        CHECK(alloc.large_allocations() == 2);
        CHECK(alloc.blocks_in_use(1) == 1);
        CHECK(alloc.blocks_in_use(SmallObjectAllocator::kClassCount) == 0);
        alloc.deallocate(s, 24, 8);
        alloc.deallocate(l, 1000, 8);
        alloc.deallocate(over, 32, 64);
        CHECK(alloc.total_blocks_in_use() == 0);
        CHECK(alloc.is_equal(alloc));
    }
    CHECK_FALSE(upstream.stats().has_leaks());
}

TEST_CASE("SmallObjectAllocator backs pmr containers with random workload", "[memory][pools][small][pmr]") {
    TrackingMemoryResource upstream;
    {
        SmallObjectAllocator alloc(&upstream);
        std::mt19937 rng(1234);
        std::uniform_int_distribution<std::size_t> len(0, 400);
        {
            std::pmr::list<std::pmr::string> strings(&alloc);
            for (int i = 0; i < 300; ++i) {
                strings.emplace_back(len(rng), 'x');
                if (i % 5 == 0) {
                    strings.pop_front();
                }
            }
            CHECK(alloc.total_blocks_in_use() >= strings.size());
            CHECK(alloc.large_allocations() > 0);
        }
        CHECK(alloc.total_blocks_in_use() == 0);
    }
    CHECK_FALSE(upstream.stats().has_leaks());
}

TEST_CASE("ObjectPool constructs, recycles and destroys objects", "[memory][pools][object]") {
    Counted::live = 0;
    {
        ObjectPool<Counted, 4> pool;
        CHECK(pool.capacity() == 4);
        auto a = pool.acquire(1);
        auto b = pool.acquire(2);
        CHECK(Counted::live == 2);
        CHECK(pool.in_use() == 2);
        CHECK(pool.owns(a.get()));
        CHECK(a->value == 1);
        Counted* address = b.get();
        b.reset();
        CHECK(Counted::live == 1);
        auto c = pool.acquire(3);
        CHECK(c.get() == address); // slot reused
        CHECK(pool.available() == 2);
    }
    CHECK(Counted::live == 0);
}

TEST_CASE("ObjectPool exhaustion and try_acquire", "[memory][pools][object]") {
    ObjectPool<int, 2> pool;
    auto a = pool.acquire(1);
    auto b = pool.acquire(2);
    CHECK_THROWS_AS(pool.acquire(3), std::bad_alloc);
    auto none = pool.try_acquire(4);
    CHECK_FALSE(none);
    a.reset();
    auto c = pool.try_acquire(5);
    REQUIRE(c);
    CHECK(*c == 5);
    int outside = 0;
    CHECK_FALSE(pool.owns(&outside));
}

TEST_CASE("ObjectPool is exception safe when the constructor throws", "[memory][pools][object][exceptions]") {
    Counted::live = 0;
    ObjectPool<Counted, 2> pool;
    auto keep = pool.acquire(1);
    Counted::throw_next = true;
    CHECK_THROWS_AS(pool.acquire(2), std::runtime_error);
    Counted::throw_next = false;
    CHECK(pool.in_use() == 1);
    CHECK(pool.available() == 1);
    auto next = pool.acquire(3);
    CHECK(next->value == 3);
    CHECK(Counted::live == 2);
}

TEST_CASE("ObjectPool handles live in standard containers", "[memory][pools][object]") {
    ObjectPool<std::string, 8> pool;
    std::vector<ObjectPool<std::string, 8>::Handle> handles;
    for (int i = 0; i < 8; ++i) {
        handles.push_back(pool.acquire(std::to_string(i)));
    }
    CHECK(pool.available() == 0);
    handles.erase(handles.begin(), handles.begin() + 3);
    CHECK(pool.available() == 3);
    CHECK(*handles.front() == "3");
    handles.clear();
    CHECK(pool.in_use() == 0);
}

TEST_CASE("demonstrateMemoryPools releases everything", "[memory][pools][demo]") {
    std::ostringstream out;
    demonstrateMemoryPools(out);
    const auto text = out.str();
    CHECK(text.find("After containers destroyed: 0 pool blocks in use") != std::string::npos);
    CHECK(text.find("in use afterwards 0") != std::string::npos);
    CHECK(text.find("After scope: 0 blocks live") != std::string::npos);
}
