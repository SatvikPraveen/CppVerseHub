/**
 * @file MemoryBenchmarks.cpp
 * @brief Allocator, pool and smart-pointer micro-benchmarks.
 */

#include "memory/CustomAllocators.hpp"
#include "memory/MemoryPools.hpp"
#include "memory/SmartPointers.hpp"

#include <benchmark/benchmark.h>

#include <list>
#include <memory>
#include <memory_resource>
#include <vector>

using namespace CppVerseHub::Memory;

namespace {

void BM_ListPushBack_StdAllocator(benchmark::State& state) {
    const auto n = static_cast<int>(state.range(0));
    for (auto _ : state) {
        std::list<int> l;
        for (int i = 0; i < n; ++i) {
            l.push_back(i);
        }
        benchmark::DoNotOptimize(l);
    }
    state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK(BM_ListPushBack_StdAllocator)->Arg(1000);

void BM_ListPushBack_PoolAllocator(benchmark::State& state) {
    const auto n = static_cast<int>(state.range(0));
    FixedSizePool pool(64, alignof(std::max_align_t), 256);
    for (auto _ : state) {
        std::list<int, PoolAllocator<int>> l{PoolAllocator<int>(pool)};
        for (int i = 0; i < n; ++i) {
            l.push_back(i);
        }
        benchmark::DoNotOptimize(l);
    }
    state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK(BM_ListPushBack_PoolAllocator)->Arg(1000);

void BM_ListPushBack_MonotonicArena(benchmark::State& state) {
    const auto n = static_cast<int>(state.range(0));
    MonotonicArena arena(64 * 1024);
    for (auto _ : state) {
        {
            std::pmr::list<int> l(&arena);
            for (int i = 0; i < n; ++i) {
                l.push_back(i);
            }
            benchmark::DoNotOptimize(l);
        }
        arena.release();
    }
    state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK(BM_ListPushBack_MonotonicArena)->Arg(1000);

void BM_ListPushBack_StdPmrPool(benchmark::State& state) {
    const auto n = static_cast<int>(state.range(0));
    std::pmr::unsynchronized_pool_resource pool;
    for (auto _ : state) {
        std::pmr::list<int> l(&pool);
        for (int i = 0; i < n; ++i) {
            l.push_back(i);
        }
        benchmark::DoNotOptimize(l);
    }
    state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK(BM_ListPushBack_StdPmrPool)->Arg(1000);

void BM_VectorPushBack_TrackingAllocator(benchmark::State& state) {
    const auto n = static_cast<int>(state.range(0));
    AllocationStats stats;
    for (auto _ : state) {
        std::vector<int, TrackingAllocator<int>> v{TrackingAllocator<int>(stats)};
        for (int i = 0; i < n; ++i) {
            v.push_back(i);
        }
        benchmark::DoNotOptimize(v.data());
    }
    state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK(BM_VectorPushBack_TrackingAllocator)->Arg(4096);

void BM_FixedSizePool_AllocFree(benchmark::State& state) {
    FixedSizePool pool(32, 16, 1024);
    std::vector<void*> blocks(256);
    for (auto _ : state) {
        for (auto& b : blocks) {
            b = pool.allocate();
        }
        for (auto* b : blocks) {
            pool.deallocate(b);
        }
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations() * 256);
}
BENCHMARK(BM_FixedSizePool_AllocFree);

void BM_OperatorNew_AllocFree(benchmark::State& state) {
    std::vector<void*> blocks(256);
    for (auto _ : state) {
        for (auto& b : blocks) {
            b = ::operator new(32);
        }
        for (auto* b : blocks) {
            ::operator delete(b);
        }
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations() * 256);
}
BENCHMARK(BM_OperatorNew_AllocFree);

void BM_SmallObjectAllocator_Mixed(benchmark::State& state) {
    SmallObjectAllocator alloc;
    std::vector<std::pair<void*, std::size_t>> blocks(256);
    for (auto _ : state) {
        std::size_t size = 8;
        for (auto& [p, s] : blocks) {
            s = size;
            p = alloc.allocate(s, 8);
            size = size % 240 + 8;
        }
        for (auto& [p, s] : blocks) {
            alloc.deallocate(p, s, 8);
        }
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations() * 256);
}
BENCHMARK(BM_SmallObjectAllocator_Mixed);

void BM_StackAllocator_Frame(benchmark::State& state) {
    StackAllocator<64 * 1024> stack;
    for (auto _ : state) {
        const auto marker = stack.mark();
        for (int i = 0; i < 256; ++i) {
            benchmark::DoNotOptimize(stack.allocate(48, 16));
        }
        stack.rewind(marker);
    }
    state.SetItemsProcessed(state.iterations() * 256);
}
BENCHMARK(BM_StackAllocator_Frame);

void BM_ObjectPool_AcquireRelease(benchmark::State& state) {
    auto pool = std::make_unique<ObjectPool<std::pair<double, double>, 256>>();
    std::vector<ObjectPool<std::pair<double, double>, 256>::Handle> handles;
    handles.reserve(256);
    for (auto _ : state) {
        for (int i = 0; i < 256; ++i) {
            handles.push_back(pool->acquire(1.0, 2.0));
        }
        handles.clear();
    }
    state.SetItemsProcessed(state.iterations() * 256);
}
BENCHMARK(BM_ObjectPool_AcquireRelease);

void BM_MakeUnique(benchmark::State& state) {
    for (auto _ : state) {
        auto p = std::make_unique<SpaceStation>("bench", 10);
        benchmark::DoNotOptimize(p.get());
    }
}
BENCHMARK(BM_MakeUnique);

void BM_MakeShared(benchmark::State& state) {
    for (auto _ : state) {
        auto p = std::make_shared<SpaceStation>("bench", 10);
        benchmark::DoNotOptimize(p.get());
    }
}
BENCHMARK(BM_MakeShared);

void BM_SharedPtrCopy(benchmark::State& state) {
    auto p = std::make_shared<int>(42);
    for (auto _ : state) {
        auto copy = p;
        benchmark::DoNotOptimize(copy);
    }
}
BENCHMARK(BM_SharedPtrCopy);

void BM_WeakPtrLock(benchmark::State& state) {
    auto p = std::make_shared<int>(42);
    std::weak_ptr<int> w = p;
    for (auto _ : state) {
        auto locked = w.lock();
        benchmark::DoNotOptimize(locked);
    }
}
BENCHMARK(BM_WeakPtrLock);

} // namespace
