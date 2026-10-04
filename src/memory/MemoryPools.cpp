/**
 * @file MemoryPools.cpp
 * @brief FixedSizePool / SmallObjectAllocator implementation and the memory-pool showcase.
 */

#include "memory/MemoryPools.hpp"

#include <algorithm>
#include <list>
#include <map>
#include <string>
#include <thread>

namespace CppVerseHub::Memory {

// ------------------------------------------------------------------ FixedSizePool

FixedSizePool::FixedSizePool(std::size_t block_size, std::size_t block_alignment,
                             std::size_t blocks_per_chunk, std::size_t max_chunks,
                             std::pmr::memory_resource* upstream)
    : block_size_(block_size)
    , alignment_(std::max(block_alignment, alignof(FreeNode)))
    , stride_(0)
    , blocks_per_chunk_(blocks_per_chunk)
    , max_chunks_(max_chunks)
    , upstream_(upstream) {
    if (block_size == 0 || blocks_per_chunk == 0) {
        throw std::invalid_argument("FixedSizePool: block size and blocks per chunk must be non-zero");
    }
    if (!AllocatorUtils::is_power_of_two(block_alignment)) {
        throw std::invalid_argument("FixedSizePool: alignment must be a power of two");
    }
    if (upstream == nullptr) {
        throw std::invalid_argument("FixedSizePool: upstream resource must not be null");
    }
    stride_ = AllocatorUtils::align_up(std::max(block_size, sizeof(FreeNode)), alignment_);
    if (stride_ > std::numeric_limits<std::size_t>::max() / blocks_per_chunk_) {
        throw std::invalid_argument("FixedSizePool: chunk size overflows");
    }
}

FixedSizePool::~FixedSizePool() {
    release();
}

void FixedSizePool::release() noexcept {
    for (std::byte* chunk : chunks_) {
        upstream_->deallocate(chunk, stride_ * blocks_per_chunk_, alignment_);
    }
    chunks_.clear();
    free_list_ = nullptr;
    in_use_ = 0;
}

bool FixedSizePool::grow() noexcept {
    if (max_chunks_ != 0 && chunks_.size() >= max_chunks_) {
        return false;
    }
    try {
        chunks_.reserve(chunks_.size() + 1);
        auto* chunk = static_cast<std::byte*>(upstream_->allocate(stride_ * blocks_per_chunk_, alignment_));
        chunks_.push_back(chunk); // cannot throw after reserve
        // Thread blocks in reverse so that allocation proceeds in ascending address order.
        for (std::size_t i = blocks_per_chunk_; i-- > 0;) {
            free_list_ = ::new (static_cast<void*>(chunk + i * stride_)) FreeNode{free_list_};
        }
        return true;
    } catch (...) {
        return false;
    }
}

void* FixedSizePool::try_allocate() noexcept {
    if (free_list_ == nullptr && !grow()) {
        return nullptr;
    }
    FreeNode* node = free_list_;
    free_list_ = node->next;
    ++in_use_;
    return node;
}

void* FixedSizePool::allocate() {
    void* p = try_allocate();
    if (p == nullptr) {
        throw std::bad_alloc();
    }
    return p;
}

void FixedSizePool::deallocate(void* p) noexcept {
    if (p == nullptr) {
        return;
    }
    free_list_ = ::new (p) FreeNode{free_list_};
    --in_use_;
}

bool FixedSizePool::owns(const void* p) const noexcept {
    const auto* b = static_cast<const std::byte*>(p);
    const std::less<const std::byte*> less;
    const std::size_t chunk_bytes = stride_ * blocks_per_chunk_;
    return std::any_of(chunks_.begin(), chunks_.end(), [&](const std::byte* chunk) {
        return !less(b, chunk) && less(b, chunk + chunk_bytes);
    });
}

// ------------------------------------------------------------- SmallObjectAllocator

SmallObjectAllocator::SmallObjectAllocator(std::pmr::memory_resource* upstream) : upstream_(upstream) {
    for (std::size_t i = 0; i < kClassCount; ++i) {
        const std::size_t block = (i + 1) * kGranularity;
        // Aim for ~4 KiB chunks for small classes, at least 16 blocks for large ones.
        const std::size_t per_chunk = std::max<std::size_t>(16, 4096 / block);
        pools_[i] = std::make_unique<FixedSizePool>(block, kGranularity, per_chunk, 0, upstream);
    }
}

std::size_t SmallObjectAllocator::total_blocks_in_use() const noexcept {
    std::size_t total = 0;
    for (const auto& pool : pools_) {
        total += pool->blocks_in_use();
    }
    return total;
}

void* SmallObjectAllocator::do_allocate(std::size_t bytes, std::size_t alignment) {
    const std::size_t index = size_class(bytes);
    if (index == kClassCount || alignment > kGranularity) {
        void* p = upstream_->allocate(bytes, alignment);
        ++large_allocations_;
        return p;
    }
    void* p = pools_[index]->allocate();
    ++small_allocations_;
    return p;
}

void SmallObjectAllocator::do_deallocate(void* p, std::size_t bytes, std::size_t alignment) {
    const std::size_t index = size_class(bytes);
    if (index == kClassCount || alignment > kGranularity) {
        upstream_->deallocate(p, bytes, alignment);
        return;
    }
    pools_[index]->deallocate(p);
}

// ------------------------------------------------------------------- Showcase

namespace {
struct Particle {
    double x;
    double y;
    double vx;
    double vy;
    Particle(double px, double py, double pvx, double pvy) noexcept : x(px), y(py), vx(pvx), vy(pvy) {}
    void step(double dt) noexcept {
        x += vx * dt;
        y += vy * dt;
    }
};
} // namespace

void demonstrateMemoryPools(std::ostream& out) {
    out << "=== Memory Pools ===\n";

    // 1. Raw fixed-size pool.
    FixedSizePool pool(24, 8, 8);
    std::vector<void*> blocks;
    for (int i = 0; i < 20; ++i) {
        blocks.push_back(pool.allocate());
    }
    out << "FixedSizePool: " << pool.blocks_in_use() << " blocks in use, capacity " << pool.capacity()
        << " in " << pool.chunk_count() << " chunks, stride " << pool.stride() << "\n";
    for (void* b : blocks) {
        pool.deallocate(b);
    }
    out << "After returning blocks: " << pool.blocks_in_use() << " in use\n";

    // 2. Node allocator for std::list / std::map.
    FixedSizePool node_pool(64, alignof(std::max_align_t), 32);
    {
        std::list<int, PoolAllocator<int>> numbers{PoolAllocator<int>(node_pool)};
        for (int i = 0; i < 100; ++i) {
            numbers.push_back(i * i);
        }
        using MapAlloc = PoolAllocator<std::pair<const int, double>>;
        std::map<int, double, std::less<>, MapAlloc> table{MapAlloc(node_pool)};
        for (int i = 0; i < 50; ++i) {
            table.emplace(i, i * 1.5);
        }
        out << "PoolAllocator: list(" << numbers.size() << ") + map(" << table.size() << ") use "
            << node_pool.blocks_in_use() << " pool blocks\n";
    }
    out << "After containers destroyed: " << node_pool.blocks_in_use() << " pool blocks in use\n";

    // 3. Size classes as a pmr resource.
    SmallObjectAllocator small;
    {
        std::pmr::vector<std::pmr::string> names(&small);
        names.reserve(8);
        for (int i = 0; i < 8; ++i) {
            names.emplace_back("satellite-designation-" + std::to_string(i * 1000));
        }
        out << "SmallObjectAllocator: " << small.small_allocations() << " small, "
            << small.large_allocations() << " large allocations; " << small.total_blocks_in_use()
            << " blocks live\n";
    }
    out << "After scope: " << small.total_blocks_in_use() << " blocks live\n";

    // 4. Typed object pool with RAII handles.
    ObjectPool<Particle, 16> particles;
    {
        std::vector<ObjectPool<Particle, 16>::Handle> live;
        for (int i = 0; i < 10; ++i) {
            live.push_back(particles.acquire(i * 1.0, 0.0, 1.0, 2.0));
        }
        for (auto& p : live) {
            p->step(0.5);
        }
        out << "ObjectPool: " << particles.in_use() << "/" << particles.capacity()
            << " particles live; first at (" << live.front()->x << ", " << live.front()->y << ")\n";
    }
    out << "Handles released: " << particles.available() << " slots free\n";

    // 5. Thread-safe pool shared by several threads.
    ThreadSafePool shared(32);
    {
        std::vector<std::thread> workers;
        for (int t = 0; t < 4; ++t) {
            workers.emplace_back([&shared] {
                std::vector<void*> mine;
                for (int i = 0; i < 200; ++i) {
                    mine.push_back(shared.allocate());
                }
                for (void* p : mine) {
                    shared.deallocate(p);
                }
            });
        }
        for (auto& w : workers) {
            w.join();
        }
    }
    out << "ThreadSafePool: 4 threads x 200 blocks; in use afterwards " << shared.blocks_in_use() << "\n";
}

} // namespace CppVerseHub::Memory
