/**
 * @file CustomAllocators.cpp
 * @brief MonotonicArena implementation and the custom-allocator showcase.
 */

#include "memory/CustomAllocators.hpp"

#include <algorithm>
#include <list>
#include <map>
#include <string>

namespace CppVerseHub::Memory {

namespace {
constexpr std::size_t kMaxChunkSize = std::size_t{1} << 20; // growth cap: 1 MiB
} // namespace

MonotonicArena::MonotonicArena(std::size_t initial_chunk_size, std::pmr::memory_resource* upstream)
    : upstream_(upstream), next_chunk_size_(std::max<std::size_t>(initial_chunk_size, 64)) {}

MonotonicArena::MonotonicArena(void* buffer, std::size_t size, std::pmr::memory_resource* upstream)
    : upstream_(upstream)
    , initial_buffer_(buffer)
    , initial_size_(size)
    , next_chunk_size_(std::max<std::size_t>(size, kDefaultChunkSize))
    , cursor_(static_cast<std::byte*>(buffer))
    , remaining_(size) {}

MonotonicArena::~MonotonicArena() {
    release();
}

void MonotonicArena::release() noexcept {
    for (const Chunk& chunk : chunks_) {
        upstream_->deallocate(chunk.memory, chunk.size, chunk.alignment);
    }
    chunks_.clear();
    cursor_ = static_cast<std::byte*>(initial_buffer_);
    remaining_ = initial_size_;
    bytes_allocated_ = 0;
    allocation_count_ = 0;
}

void* MonotonicArena::try_bump(std::size_t bytes, std::size_t alignment) noexcept {
    if (cursor_ == nullptr) {
        return nullptr;
    }
    void* p = cursor_;
    std::size_t space = remaining_;
    if (std::align(alignment, bytes, p, space) == nullptr) {
        return nullptr;
    }
    cursor_ = static_cast<std::byte*>(p) + bytes;
    remaining_ = space - bytes;
    return p;
}

void MonotonicArena::grow(std::size_t bytes, std::size_t alignment) {
    if (bytes > std::numeric_limits<std::size_t>::max() / 2) {
        throw std::bad_alloc();
    }
    const std::size_t chunk_alignment = std::max(alignment, alignof(std::max_align_t));
    const std::size_t chunk_size = std::max(next_chunk_size_, AllocatorUtils::align_up(bytes, 64));
    chunks_.reserve(chunks_.size() + 1);                             // may throw: nothing changed yet
    void* memory = upstream_->allocate(chunk_size, chunk_alignment); // may throw: nothing changed yet
    chunks_.push_back(Chunk{memory, chunk_size, chunk_alignment});   // cannot throw after reserve
    cursor_ = static_cast<std::byte*>(memory);
    remaining_ = chunk_size;
    next_chunk_size_ = std::min(chunk_size * 2, std::max(kMaxChunkSize, chunk_size));
}

void* MonotonicArena::do_allocate(std::size_t bytes, std::size_t alignment) {
    if (!AllocatorUtils::is_power_of_two(alignment)) {
        throw std::invalid_argument("MonotonicArena: alignment must be a power of two");
    }
    bytes = bytes == 0 ? 1 : bytes;
    void* p = try_bump(bytes, alignment);
    if (p == nullptr) {
        grow(bytes, alignment);
        p = try_bump(bytes, alignment); // a fresh chunk is aligned and large enough
    }
    bytes_allocated_ += bytes;
    ++allocation_count_;
    return p;
}

void MonotonicArena::do_deallocate(void* /*p*/, std::size_t /*bytes*/, std::size_t /*alignment*/) {
    // Monotonic: individual frees are no-ops; memory is reclaimed by release().
}

bool MonotonicArena::do_is_equal(const std::pmr::memory_resource& other) const noexcept {
    return this == &other;
}

void demonstrateCustomAllocators(std::ostream& out) {
    out << "=== Custom Allocators ===\n";

    // 1. TrackingAllocator with several standard containers.
    AllocationStats stats;
    {
        std::vector<int, TrackingAllocator<int>> numbers{TrackingAllocator<int>(stats)};
        for (int i = 0; i < 100; ++i) {
            numbers.push_back(i);
        }
        std::map<int, std::string, std::less<>, TrackingAllocator<std::pair<const int, std::string>>> names{
            TrackingAllocator<std::pair<const int, std::string>>(stats)};
        names.emplace(1, "Mercury");
        names.emplace(2, "Venus");
        names.emplace(3, "Earth");
        out << "TrackingAllocator: vector size " << numbers.size() << ", map size " << names.size()
            << ", allocations so far " << stats.allocations() << ", bytes in use " << stats.bytes_in_use()
            << "\n";
    }
    out << "After scope: outstanding allocations " << stats.outstanding() << ", peak bytes "
        << stats.peak_bytes() << (stats.has_leaks() ? " (LEAK)\n" : " (no leaks)\n");

    // 2. StackAllocator with markers.
    StackAllocator<1024> stack;
    void* a = stack.allocate(100, 8);
    const auto checkpoint = stack.mark();
    void* b = stack.allocate(200, 64);
    out << "StackAllocator: used " << stack.bytes_used()
        << " bytes; 64-byte aligned: " << (AllocatorUtils::is_aligned(b, 64) ? "yes" : "no") << "\n";
    stack.rewind(checkpoint);
    out << "After rewind: used " << stack.bytes_used() << " bytes\n";
    static_cast<void>(stack.deallocate(a, 100));
    out << "After LIFO free: used " << stack.bytes_used() << " bytes\n";

    // 3. MonotonicArena as a pmr resource, with tracked upstream to prove it frees chunks.
    TrackingMemoryResource upstream;
    {
        MonotonicArena arena(256, &upstream);
        std::pmr::vector<std::pmr::string> words(&arena);
        for (int i = 0; i < 20; ++i) {
            words.emplace_back("a fairly long string that defeats SSO #" + std::to_string(i));
        }
        out << "MonotonicArena: " << words.size() << " pmr strings, " << arena.chunk_count() << " chunks, "
            << arena.bytes_allocated() << " bytes handed out\n";

        std::vector<double, ArenaAllocator<double>> values{ArenaAllocator<double>(arena)};
        for (int i = 0; i < 50; ++i) {
            values.push_back(i * 0.5);
        }
        out << "ArenaAllocator vector of " << values.size() << " doubles; last = " << values.back() << "\n";
    }
    out << "Upstream after arena destruction: outstanding " << upstream.stats().outstanding()
        << ", misaligned " << upstream.misaligned_count() << "\n";

    // 4. Standard pmr pool over our tracking resource.
    TrackingMemoryResource pool_upstream;
    {
        std::pmr::unsynchronized_pool_resource pool(&pool_upstream);
        std::pmr::list<int> items(&pool);
        for (int i = 0; i < 1000; ++i) {
            items.push_back(i);
        }
        out << "unsynchronized_pool_resource: list of " << items.size() << " nodes used "
            << pool_upstream.stats().allocations() << " upstream allocations\n";
    }
    out << "Pool upstream outstanding after destruction: " << pool_upstream.stats().outstanding() << "\n";
}

} // namespace CppVerseHub::Memory
