/**
 * @file CustomAllocatorsTests.cpp
 * @brief Tests for tracking/stack/arena allocators and pmr resources.
 */

#include "memory/CustomAllocators.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <deque>
#include <iterator>
#include <limits>
#include <list>
#include <map>
#include <memory_resource>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

using namespace CppVerseHub::Memory;

namespace {
struct alignas(64) CacheLine {
    double values[8]{};
};

struct ThrowOnCopy {
    static inline int copies_until_throw = -1;
    int value = 0;
    explicit ThrowOnCopy(int v) : value(v) {}
    ThrowOnCopy(const ThrowOnCopy& other) : value(other.value) {
        if (copies_until_throw == 0) {
            throw std::runtime_error("copy failed");
        }
        if (copies_until_throw > 0) {
            --copies_until_throw;
        }
    }
    ThrowOnCopy& operator=(const ThrowOnCopy&) = default;
    ~ThrowOnCopy() = default;
};
} // namespace

TEST_CASE("AllocatorUtils alignment arithmetic", "[memory][allocators][utils]") {
    STATIC_REQUIRE(AllocatorUtils::is_power_of_two(1));
    STATIC_REQUIRE(AllocatorUtils::is_power_of_two(64));
    STATIC_REQUIRE_FALSE(AllocatorUtils::is_power_of_two(0));
    STATIC_REQUIRE_FALSE(AllocatorUtils::is_power_of_two(48));
    STATIC_REQUIRE(AllocatorUtils::align_up(0, 16) == 0);
    STATIC_REQUIRE(AllocatorUtils::align_up(1, 16) == 16);
    STATIC_REQUIRE(AllocatorUtils::align_up(16, 16) == 16);
    STATIC_REQUIRE(AllocatorUtils::align_up(17, 8) == 24);
    alignas(32) std::byte buffer[64]{};
    CHECK(AllocatorUtils::is_aligned(buffer, 32));
    CHECK_FALSE(AllocatorUtils::is_aligned(buffer + 1, 2));
}

TEST_CASE("Allocators satisfy the Allocator concept and allocator_traits", "[memory][allocators][concepts]") {
    STATIC_REQUIRE(AllocatorUtils::Allocator<TrackingAllocator<std::string>>);
    STATIC_REQUIRE(AllocatorUtils::Allocator<ArenaAllocator<CacheLine>>);
    STATIC_REQUIRE_FALSE(AllocatorUtils::Allocator<int>);
    using Traits = std::allocator_traits<TrackingAllocator<int>>;
    STATIC_REQUIRE(std::is_same_v<Traits::rebind_alloc<double>, TrackingAllocator<double>>);
    STATIC_REQUIRE(Traits::propagate_on_container_move_assignment::value);
    STATIC_REQUIRE_FALSE(Traits::is_always_equal::value);
    using ArenaTraits = std::allocator_traits<ArenaAllocator<int>>;
    STATIC_REQUIRE(std::is_same_v<ArenaTraits::rebind_alloc<char>, ArenaAllocator<char>>);
}

TEST_CASE("AllocationStats counts allocations, bytes and peak", "[memory][allocators][stats]") {
    AllocationStats stats;
    stats.record_allocation(100);
    stats.record_allocation(50);
    CHECK(stats.allocations() == 2);
    CHECK(stats.bytes_in_use() == 150);
    CHECK(stats.peak_bytes() == 150);
    stats.record_deallocation(100);
    CHECK(stats.bytes_in_use() == 50);
    CHECK(stats.peak_bytes() == 150);
    CHECK(stats.outstanding() == 1);
    CHECK(stats.has_leaks());
    stats.record_deallocation(50);
    CHECK_FALSE(stats.has_leaks());
    CHECK(stats.bytes_allocated() == 150);
    CHECK(stats.bytes_deallocated() == 150);
    stats.reset();
    CHECK(stats.allocations() == 0);
    CHECK(stats.peak_bytes() == 0);
}

TEST_CASE("TrackingAllocator with std::vector reports no leaks", "[memory][allocators][tracking]") {
    AllocationStats stats;
    {
        std::vector<int, TrackingAllocator<int>> v{TrackingAllocator<int>(stats)};
        for (int i = 0; i < 1000; ++i) {
            v.push_back(i);
        }
        CHECK(stats.allocations() >= 2); // growth reallocates
        CHECK(stats.bytes_in_use() == v.capacity() * sizeof(int));
        CHECK(v[999] == 999);
        v.shrink_to_fit();
        CHECK(stats.bytes_in_use() == v.capacity() * sizeof(int));
    }
    CHECK(stats.outstanding() == 0);
    CHECK(stats.bytes_in_use() == 0);
    CHECK(stats.allocations() == stats.deallocations());
    CHECK(stats.peak_bytes() >= 1000 * sizeof(int));
}

TEST_CASE("TrackingAllocator works with node-based containers via rebinding",
          "[memory][allocators][tracking]") {
    AllocationStats stats;
    {
        std::list<std::string, TrackingAllocator<std::string>> lst{TrackingAllocator<std::string>(stats)};
        lst.emplace_back("alpha");
        lst.emplace_back("beta");
        using MapAlloc = TrackingAllocator<std::pair<const int, std::string>>;
        std::map<int, std::string, std::less<>, MapAlloc> m{MapAlloc(stats)};
        for (int i = 0; i < 10; ++i) {
            m.emplace(i, std::to_string(i));
        }
        std::set<int, std::less<>, TrackingAllocator<int>> s{TrackingAllocator<int>(stats)};
        s.insert({3, 1, 2});
        using UMapAlloc = TrackingAllocator<std::pair<const std::string, int>>;
        std::unordered_map<std::string, int, std::hash<std::string>, std::equal_to<>, UMapAlloc> um{
            8, std::hash<std::string>{}, std::equal_to<>{}, UMapAlloc(stats)};
        um["x"] = 1;
        um["y"] = 2;
        std::deque<int, TrackingAllocator<int>> dq{TrackingAllocator<int>(stats)};
        for (int i = 0; i < 2000; ++i) {
            dq.push_front(i);
        }
        CHECK(stats.outstanding() >= 2 + 10 + 3 + 2);
        CHECK(m.at(7) == "7");
        CHECK(dq.front() == 1999);
    }
    CHECK_FALSE(stats.has_leaks());
}

TEST_CASE("TrackingAllocator equality and propagation semantics", "[memory][allocators][tracking]") {
    AllocationStats s1;
    AllocationStats s2;
    TrackingAllocator<int> a(s1);
    TrackingAllocator<int> b(s1);
    TrackingAllocator<int> c(s2);
    TrackingAllocator<double> rebound(a);
    CHECK(a == b);
    CHECK(a != c);
    CHECK(rebound == a);
    CHECK(rebound.stats() == &s1);
    CHECK(TrackingAllocator<int>().stats() == &AllocationStats::global());

    SECTION("move assignment propagates the allocator") {
        std::vector<int, TrackingAllocator<int>> v1({1, 2, 3}, a);
        std::vector<int, TrackingAllocator<int>> v2(c);
        v2 = std::move(v1);
        CHECK(v2.get_allocator() == a);
        CHECK(v2.size() == 3);
    }
    SECTION("copy assignment propagates the allocator") {
        std::vector<int, TrackingAllocator<int>> v1({1, 2, 3}, a);
        std::vector<int, TrackingAllocator<int>> v2(c);
        v2 = v1;
        CHECK(v2.get_allocator() == a);
    }
    CHECK_FALSE(s1.has_leaks());
    CHECK_FALSE(s2.has_leaks());
}

TEST_CASE("TrackingAllocator honours over-alignment", "[memory][allocators][alignment]") {
    AllocationStats stats;
    {
        std::vector<CacheLine, TrackingAllocator<CacheLine>> lines{TrackingAllocator<CacheLine>(stats)};
        for (int i = 0; i < 33; ++i) {
            lines.emplace_back();
            REQUIRE(AllocatorUtils::is_aligned(lines.data(), alignof(CacheLine)));
        }
        TrackingAllocator<CacheLine> alloc(stats);
        CacheLine* raw = alloc.allocate(3);
        CHECK(AllocatorUtils::is_aligned(raw, 64));
        alloc.deallocate(raw, 3);
    }
    CHECK_FALSE(stats.has_leaks());
}

TEST_CASE("TrackingAllocator rejects impossible sizes without side effects",
          "[memory][allocators][exceptions]") {
    AllocationStats stats;
    TrackingAllocator<CacheLine> alloc(stats);
    CHECK_THROWS_AS(alloc.allocate(alloc.max_size() + 1), std::bad_array_new_length);
    CHECK(stats.allocations() == 0);
    CHECK(stats.bytes_in_use() == 0);
}

TEST_CASE("TrackingAllocator stays balanced when element copies throw", "[memory][allocators][exceptions]") {
    AllocationStats stats;
    {
        using TrackedVector = std::vector<ThrowOnCopy, TrackingAllocator<ThrowOnCopy>>;
        TrackedVector v{TrackingAllocator<ThrowOnCopy>(stats)};
        v.reserve(4);
        for (int i = 0; i < 4; ++i) {
            v.emplace_back(i);
        }
        ThrowOnCopy::copies_until_throw = 2;
        // Copying the vector throws midway: the partially built copy must be released.
        CHECK_THROWS_AS(TrackedVector(v), std::runtime_error);
        ThrowOnCopy::copies_until_throw = -1;
        CHECK(stats.outstanding() == 1);
        CHECK(v.size() == 4);
    }
    CHECK_FALSE(stats.has_leaks());
}

TEST_CASE("TrackingMemoryResource tracks a pmr container", "[memory][allocators][pmr]") {
    TrackingMemoryResource tracker;
    {
        std::pmr::vector<std::pmr::string> v(&tracker);
        for (int i = 0; i < 50; ++i) {
            v.emplace_back("long enough to need a heap allocation " + std::to_string(i));
        }
        CHECK(tracker.stats().allocations() > 50);
        CHECK(v.back().get_allocator().resource() == &tracker);
    }
    CHECK_FALSE(tracker.stats().has_leaks());
    CHECK(tracker.misaligned_count() == 0);
    CHECK(tracker.is_equal(tracker));
    TrackingMemoryResource other;
    CHECK_FALSE(tracker.is_equal(other));
}

TEST_CASE("TrackingMemoryResource as upstream of standard pmr pools", "[memory][allocators][pmr]") {
    TrackingMemoryResource tracker;
    SECTION("unsynchronized_pool_resource returns everything on destruction") {
        {
            std::pmr::unsynchronized_pool_resource pool(&tracker);
            std::pmr::list<int> l(&pool);
            for (int i = 0; i < 500; ++i) {
                l.push_back(i);
            }
            CHECK(tracker.stats().allocations() < 500); // pool batches upstream requests
        }
        CHECK_FALSE(tracker.stats().has_leaks());
    }
    SECTION("monotonic_buffer_resource returns everything on destruction") {
        {
            std::pmr::monotonic_buffer_resource mono(&tracker);
            std::pmr::vector<double> v(&mono);
            for (int i = 0; i < 1000; ++i) {
                v.push_back(i);
            }
        }
        CHECK_FALSE(tracker.stats().has_leaks());
    }
}

TEST_CASE("StackAllocator bump allocation and alignment", "[memory][allocators][stack]") {
    StackAllocator<1024> stack;
    CHECK(stack.capacity() == 1024);
    const auto align = GENERATE(as<std::size_t>{}, 1, 2, 4, 8, 16, 32, 64, 128);
    void* first = stack.allocate(3, 1);
    void* p = stack.allocate(10, align);
    CHECK(AllocatorUtils::is_aligned(p, align));
    CHECK(stack.owns(p));
    CHECK(stack.owns(first));
    CHECK(static_cast<std::byte*>(p) > static_cast<std::byte*>(first));
    CHECK(stack.bytes_used() + stack.bytes_remaining() == 1024);
    int outside = 0;
    CHECK_FALSE(stack.owns(&outside));
}

TEST_CASE("StackAllocator LIFO deallocation, markers and reset", "[memory][allocators][stack]") {
    StackAllocator<256> stack;
    void* a = stack.allocate(16, 8);
    const auto after_a = stack.bytes_used();
    void* b = stack.allocate(32, 8);
    CHECK_FALSE(stack.deallocate(a, 16)); // not on top: ignored
    CHECK(stack.bytes_used() > after_a);
    CHECK(stack.deallocate(b, 32));
    CHECK(stack.bytes_used() == after_a);

    const auto marker = stack.mark();
    static_cast<void>(stack.allocate(64));
    static_cast<void>(stack.allocate(64));
    stack.rewind(marker);
    CHECK(stack.bytes_used() == marker);
    stack.reset();
    CHECK(stack.bytes_used() == 0);
    CHECK_FALSE(stack.deallocate(nullptr, 4));
}

TEST_CASE("StackAllocator throws on exhaustion and bad alignment",
          "[memory][allocators][stack][exceptions]") {
    StackAllocator<64> stack;
    static_cast<void>(stack.allocate(60, 1));
    const auto used = stack.bytes_used();
    CHECK_THROWS_AS(stack.allocate(8, 1), std::bad_alloc);
    CHECK(stack.bytes_used() == used); // strong guarantee
    CHECK_THROWS_AS(stack.allocate(1, 3), std::invalid_argument);
    stack.reset();
    CHECK_NOTHROW(static_cast<void>(stack.allocate(64, 1)));
}

TEST_CASE("MonotonicArena serves aligned requests and grows chunks", "[memory][allocators][arena]") {
    TrackingMemoryResource upstream;
    {
        MonotonicArena arena(128, &upstream);
        std::mt19937 rng(42);
        std::uniform_int_distribution<std::size_t> size_dist(1, 300);
        std::uniform_int_distribution<int> align_exp(0, 7);
        for (int i = 0; i < 200; ++i) {
            const std::size_t size = size_dist(rng);
            const std::size_t align = std::size_t{1} << align_exp(rng);
            void* p = arena.allocate(size, align);
            REQUIRE(AllocatorUtils::is_aligned(p, align));
        }
        CHECK(arena.allocation_count() == 200);
        CHECK(arena.chunk_count() == upstream.stats().outstanding());
        CHECK(arena.chunk_count() > 1);
        const auto chunks = arena.chunk_count();
        const auto handed_out = arena.bytes_allocated();
        void* last = arena.allocate(8, 8);
        arena.deallocate(last, 8, 8); // no-op for a monotonic resource
        CHECK(arena.bytes_allocated() == handed_out + 8);
        CHECK(arena.chunk_count() >= chunks);
    }
    CHECK_FALSE(upstream.stats().has_leaks());
    CHECK(upstream.misaligned_count() == 0);
}

TEST_CASE("MonotonicArena handles requests larger than a chunk and huge alignments",
          "[memory][allocators][arena]") {
    TrackingMemoryResource upstream;
    MonotonicArena arena(64, &upstream);
    void* big = arena.allocate(10000, 16);
    CHECK(AllocatorUtils::is_aligned(big, 16));
    void* page = arena.allocate(32, 4096);
    CHECK(AllocatorUtils::is_aligned(page, 4096));
    CHECK(arena.bytes_allocated() == 10032);
    std::size_t bad_alignment = 24; // runtime value: not a power of two
    CHECK_THROWS_AS(arena.allocate(8, bad_alignment), std::invalid_argument);
    arena.release();
    CHECK(arena.chunk_count() == 0);
    CHECK(arena.bytes_allocated() == 0);
    CHECK_FALSE(upstream.stats().has_leaks());
}

TEST_CASE("MonotonicArena with an initial buffer avoids upstream allocations",
          "[memory][allocators][arena]") {
    TrackingMemoryResource upstream;
    alignas(std::max_align_t) std::byte buffer[512];
    MonotonicArena arena(buffer, sizeof(buffer), &upstream);
    void* p = arena.allocate(100, 8);
    CHECK(static_cast<std::byte*>(p) >= buffer);
    CHECK(static_cast<std::byte*>(p) < std::end(buffer));
    CHECK(upstream.stats().allocations() == 0);
    static_cast<void>(arena.allocate(1000, 8)); // overflow -> upstream
    CHECK(upstream.stats().allocations() == 1);
    arena.release();
    CHECK(arena.bytes_remaining() == sizeof(buffer));
    CHECK_FALSE(upstream.stats().has_leaks());
}

TEST_CASE("MonotonicArena backs pmr containers and ArenaAllocator containers",
          "[memory][allocators][arena]") {
    TrackingMemoryResource upstream;
    {
        MonotonicArena arena(256, &upstream);
        std::pmr::map<int, std::pmr::string> m(&arena);
        for (int i = 0; i < 100; ++i) {
            m.emplace(i, std::pmr::string("value number " + std::to_string(i) + " padded past SSO", &arena));
        }
        CHECK(m.at(42).find("42") != std::pmr::string::npos);

        std::vector<CacheLine, ArenaAllocator<CacheLine>> lines{ArenaAllocator<CacheLine>(arena)};
        for (int i = 0; i < 20; ++i) {
            lines.emplace_back();
            REQUIRE(AllocatorUtils::is_aligned(lines.data(), 64));
        }
        ArenaAllocator<int> ai(arena);
        ArenaAllocator<long> al(ai);
        CHECK(ai == al);
        MonotonicArena other(64, &upstream);
        CHECK(ai != ArenaAllocator<int>(other));
        CHECK_THROWS_AS(ai.allocate(std::numeric_limits<std::size_t>::max()), std::bad_array_new_length);
    }
    CHECK_FALSE(upstream.stats().has_leaks());
}

TEST_CASE("MonotonicArena leaves state unchanged when upstream fails",
          "[memory][allocators][arena][exceptions]") {
    MonotonicArena arena(64, std::pmr::null_memory_resource());
    CHECK_THROWS_AS(arena.allocate(16, 8), std::bad_alloc);
    CHECK(arena.chunk_count() == 0);
    CHECK(arena.allocation_count() == 0);
}

TEST_CASE("demonstrateCustomAllocators reports no leaks", "[memory][allocators][demo]") {
    std::ostringstream out;
    demonstrateCustomAllocators(out);
    const auto text = out.str();
    CHECK(text.find("(no leaks)") != std::string::npos);
    CHECK(text.find("LEAK") == std::string::npos);
    CHECK(text.find("outstanding 0") != std::string::npos);
}
