// Micro-benchmarks for the concurrency module: executor overhead, queue throughput,
// lock cost under contention, atomic memory orders, lock-free vs locked structures,
// and coroutine overhead.

#include <benchmark/benchmark.h>

#include <atomic>
#include <cstdint>
#include <future>
#include <mutex>
#include <stack>
#include <thread>
#include <vector>

#include "concurrency/Atomics.hpp"
#include "concurrency/ConditionalVariables.hpp"
#include "concurrency/CoroutinesDemo.hpp"
#include "concurrency/ThreadPool.hpp"

using namespace CppVerseHub::Concurrency;

namespace {

    constexpr int kBatch = 1000;

    // ------------------------------------------------------------------ executors

    template <typename Pool>
    void run_batch(Pool& pool) {
        std::vector<std::future<int>> fs;
        fs.reserve(kBatch);
        for (int i = 0; i < kBatch; ++i) {
            fs.push_back(pool.submit([i] { return i; }));
        }
        long long sum = 0;
        for (auto& f : fs) {
            sum += f.get();
        }
        benchmark::DoNotOptimize(sum);
    }

    void BM_ThreadPoolSubmitBatch(benchmark::State& state) {
        ThreadPool pool(static_cast<std::size_t>(state.range(0)));
        for (auto _ : state) {
            run_batch(pool);
        }
        state.SetItemsProcessed(state.iterations() * kBatch);
    }
    BENCHMARK(BM_ThreadPoolSubmitBatch)->Arg(1)->Arg(4)->UseRealTime();

    void BM_WorkStealingSubmitBatch(benchmark::State& state) {
        WorkStealingThreadPool pool(static_cast<std::size_t>(state.range(0)));
        for (auto _ : state) {
            run_batch(pool);
        }
        state.SetItemsProcessed(state.iterations() * kBatch);
    }
    BENCHMARK(BM_WorkStealingSubmitBatch)->Arg(1)->Arg(4)->UseRealTime();

    void BM_PriorityPoolSubmitBatch(benchmark::State& state) {
        PriorityThreadPool pool(4);
        for (auto _ : state) {
            std::vector<std::future<int>> fs;
            fs.reserve(kBatch);
            for (int i = 0; i < kBatch; ++i) {
                fs.push_back(pool.submit(static_cast<TaskPriority>(i % 4), [i] { return i; }));
            }
            for (auto& f : fs) {
                benchmark::DoNotOptimize(f.get());
            }
        }
        state.SetItemsProcessed(state.iterations() * kBatch);
    }
    BENCHMARK(BM_PriorityPoolSubmitBatch)->UseRealTime();

    // ------------------------------------------------------------------ queues

    void BM_BoundedQueueMPMC(benchmark::State& state) {
        const int producers = static_cast<int>(state.range(0));
        constexpr int per_producer = 2000;
        for (auto _ : state) {
            BoundedQueue<int> q(64);
            std::vector<std::thread> threads;
            std::atomic<long long> sum{0};
            for (int c = 0; c < producers; ++c) {
                threads.emplace_back([&] {
                    long long local = 0;
                    while (auto v = q.pop()) {
                        local += *v;
                    }
                    sum.fetch_add(local);
                });
            }
            std::vector<std::thread> prod;
            for (int p = 0; p < producers; ++p) {
                prod.emplace_back([&q] {
                    for (int i = 0; i < per_producer; ++i) {
                        static_cast<void>(q.push(i));
                    }
                });
            }
            for (auto& t : prod) {
                t.join();
            }
            q.close();
            for (auto& t : threads) {
                t.join();
            }
            benchmark::DoNotOptimize(sum.load());
        }
        state.SetItemsProcessed(state.iterations() * producers * per_producer);
    }
    BENCHMARK(BM_BoundedQueueMPMC)->Arg(1)->Arg(2)->UseRealTime();

    void BM_SpscRingTransfer(benchmark::State& state) {
        constexpr int items = 20000;
        for (auto _ : state) {
            SpscRingBuffer<int, 1024> ring;
            std::thread consumer([&ring] {
                int received = 0;
                long long sum = 0;
                while (received < items) {
                    if (auto v = ring.try_pop()) {
                        sum += *v;
                        ++received;
                    } else {
                        cpu_relax();
                    }
                }
                benchmark::DoNotOptimize(sum);
            });
            for (int i = 0; i < items; ++i) {
                while (!ring.try_push(i)) {
                    cpu_relax();
                }
            }
            consumer.join();
        }
        state.SetItemsProcessed(state.iterations() * items);
    }
    BENCHMARK(BM_SpscRingTransfer)->UseRealTime();

    void BM_BoundedQueueSpscTransfer(benchmark::State& state) {
        constexpr int items = 20000;
        for (auto _ : state) {
            BoundedQueue<int> q(1024);
            std::thread consumer([&q] {
                long long sum = 0;
                while (auto v = q.pop()) {
                    sum += *v;
                }
                benchmark::DoNotOptimize(sum);
            });
            for (int i = 0; i < items; ++i) {
                static_cast<void>(q.push(i));
            }
            q.close();
            consumer.join();
        }
        state.SetItemsProcessed(state.iterations() * items);
    }
    BENCHMARK(BM_BoundedQueueSpscTransfer)->UseRealTime();

    // ------------------------------------------------------------------ locks and atomics

    SpinLock g_spin;
    std::mutex g_mutex;
    long long g_spin_counter = 0;
    long long g_mutex_counter = 0;

    void BM_SpinLockContended(benchmark::State& state) {
        for (auto _ : state) {
            std::lock_guard lock(g_spin);
            benchmark::DoNotOptimize(++g_spin_counter);
        }
    }
    BENCHMARK(BM_SpinLockContended)->Threads(1)->Threads(4)->UseRealTime();

    void BM_MutexContended(benchmark::State& state) {
        for (auto _ : state) {
            std::lock_guard lock(g_mutex);
            benchmark::DoNotOptimize(++g_mutex_counter);
        }
    }
    BENCHMARK(BM_MutexContended)->Threads(1)->Threads(4)->UseRealTime();

    std::atomic<std::uint64_t> g_atomic{0};

    void BM_AtomicFetchAddRelaxed(benchmark::State& state) {
        for (auto _ : state) {
            benchmark::DoNotOptimize(g_atomic.fetch_add(1, std::memory_order_relaxed));
        }
    }
    BENCHMARK(BM_AtomicFetchAddRelaxed)->Threads(1)->Threads(4)->UseRealTime();

    void BM_AtomicFetchAddSeqCst(benchmark::State& state) {
        for (auto _ : state) {
            benchmark::DoNotOptimize(g_atomic.fetch_add(1, std::memory_order_seq_cst));
        }
    }
    BENCHMARK(BM_AtomicFetchAddSeqCst)->Threads(1)->Threads(4)->UseRealTime();

    void BM_LockFreeStackPushPop(benchmark::State& state) {
        static LockFreeStack<int> stack;
        for (auto _ : state) {
            stack.push(1);
            benchmark::DoNotOptimize(stack.pop());
        }
    }
    BENCHMARK(BM_LockFreeStackPushPop)->Threads(1)->Threads(4)->UseRealTime();

    void BM_MutexStackPushPop(benchmark::State& state) {
        static std::mutex m;
        static std::stack<int> stack;
        for (auto _ : state) {
            {
                std::lock_guard lock(m);
                stack.push(1);
            }
            std::lock_guard lock(m);
            if (!stack.empty()) {
                benchmark::DoNotOptimize(stack.top());
                stack.pop();
            }
        }
    }
    BENCHMARK(BM_MutexStackPushPop)->Threads(1)->Threads(4)->UseRealTime();

    // ------------------------------------------------------------------ coroutines

    void BM_GeneratorIteration(benchmark::State& state) {
        for (auto _ : state) {
            long long sum = 0;
            for (int v : iota_range(0, 10000)) {
                sum += v;
            }
            benchmark::DoNotOptimize(sum);
        }
        state.SetItemsProcessed(state.iterations() * 10000);
    }
    BENCHMARK(BM_GeneratorIteration);

    void BM_PlainLoopBaseline(benchmark::State& state) {
        for (auto _ : state) {
            long long sum = 0;
            for (int v = 0; v < 10000; ++v) {
                benchmark::DoNotOptimize(sum += v);
            }
            benchmark::DoNotOptimize(sum);
        }
        state.SetItemsProcessed(state.iterations() * 10000);
    }
    BENCHMARK(BM_PlainLoopBaseline);

    Task<int> leaf(int x) { co_return x; }

    Task<long long> sum_tasks(int n) {
        long long total = 0;
        for (int i = 0; i < n; ++i) {
            total += co_await leaf(i);
        }
        co_return total;
    }

    void BM_TaskAwaitChain(benchmark::State& state) {
        for (auto _ : state) {
            benchmark::DoNotOptimize(sync_wait(sum_tasks(1000)));
        }
        state.SetItemsProcessed(state.iterations() * 1000);
    }
    BENCHMARK(BM_TaskAwaitChain);

}  // namespace
