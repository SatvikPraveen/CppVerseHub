# Concurrency Design

## Purpose and Scope

This document describes the design of the `CppVerseHub::Concurrency` library
(`src/concurrency/`). It is written for readers who already know the C++ memory model
and want to know *why* each component is built the way it is, which guarantees it
gives, and where those guarantees are checked.

The module is organised as one header/source pair per topic:

| File | Components |
| --- | --- |
| `ThreadPool.hpp/.cpp` | `UniqueTask`, `PoolShutdownError`, `ThreadPool`, `PriorityThreadPool`, `TaskPriority`, `WorkStealingThreadPool` |
| `MutexExamples.hpp/.cpp` | `HierarchicalMutex`, `Synchronized<T>`, `ThreadSafeQueue<T>`, `ThreadSafeMap<K, V>`, `BankAccount` + `transfer()`, `run_dining_philosophers()`, `LazyValue<T>` |
| `ConditionalVariables.hpp/.cpp` | `BoundedQueue<T>`, `CountingSemaphore`, `CountDownLatch`, `CyclicBarrier`, `ManualResetEvent`, `ResourcePool<T>` |
| `Atomics.hpp/.cpp` | `SpinLock`, `SpscRingBuffer<T, Capacity>`, `LockFreeStack<T>`, `AtomicStatistics`, `atomic_fetch_max/min`, `ConcurrentBloomFilter`, `OneShotEvent` |
| `AsyncMissions.hpp/.cpp` | `CancellationSource`/`CancellationToken`, `AsyncMission<T>`, `MissionCoordinator`, `parallel_transform`, `when_all`, `Pipeline<T>` |
| `AsyncComms.hpp/.cpp` | `MessageBus`, `Actor<Msg>`, `RequestResponseServer` |
| `CoroutinesDemo.hpp/.cpp` | `Generator<T>`, `Task<T>`, `sync_wait`, `schedule_on`/`ScheduleOnAwaiter`, `RoundRobinScheduler` |
| `Demo.hpp/.cpp` | `runDemo(std::ostream&)`, the CLI entry point |

Everything lives in namespace `CppVerseHub::Concurrency` and requires C++20
(`<coroutine>`, `std::atomic<T>::wait`, `std::has_single_bit`, concepts). Only standard
library facilities are used; there is no dependency on platform threading APIs except the
`cpu_relax()` pause/yield intrinsic in `Atomics.hpp`.

### Cross-cutting conventions

- **Predicate waits only.** Every `condition_variable::wait` uses the predicate overload, and
  state read by a predicate is only modified under the associated mutex.
- **Notify after the state change**, usually after releasing the lock, to avoid waking a
  thread only for it to block on the mutex again.
- **Every memory order is justified in a comment.** The rule used throughout `Atomics.hpp`:
  a store that publishes previously written data is `release`; the load that consumes it is
  `acquire`; a value nobody synchronises through (statistics, a thread's own index) is
  `relaxed`; `seq_cst` is used deliberately where a proof needs a single total order over
  several variables.
- **Graceful shutdown.** Executors and queues distinguish "stop accepting" from "stop
  running": accepted work is always completed or explicitly drained.
- **Non-movable concurrent objects.** Types whose address is captured by worker threads
  (pools, buses, actors, ring buffers, stacks) delete copy and move operations.

## Thread Pools (`ThreadPool.hpp`)

### `UniqueTask`: move-only type erasure

`std::function` requires copyable targets, which excludes `std::packaged_task` and lambdas
capturing `std::unique_ptr`. `UniqueTask` is a minimal `std::move_only_function<void()>`
(C++23) built from the classic Concept/Model pattern:

```cpp
class UniqueTask {
public:
    template <typename F>
        requires(!std::same_as<std::remove_cvref_t<F>, UniqueTask> && std::invocable<std::decay_t<F>&>)
    explicit UniqueTask(F&& fn) : impl_(std::make_unique<Model<std::decay_t<F>>>(std::forward<F>(fn))) {}
    void operator()() { impl_->call(); }
private:
    struct Concept { virtual ~Concept() = default; virtual void call() = 0; };
    template <typename F> struct Model final : Concept { void call() override { std::invoke(fn); } F fn; };
    std::unique_ptr<Concept> impl_;
};
```

Trade-off: one heap allocation per task (no small-buffer optimisation). This keeps the
type trivially correct and is dominated by the `packaged_task` shared state allocation
anyway.

`detail::package_task(fn, args...)` decay-copies the arguments (the same rule as
`std::thread`), wraps the invocation in a `std::packaged_task<R()>`, and returns
`{UniqueTask, std::future<R>}`. Because the packaged task captures any exception into the
shared state, tasks created by `submit()` never throw out of `operator()`.

### `ThreadPool` and `PriorityThreadPool`: one core, two queue disciplines

Both pools are thin facades over `detail::PoolCore<Queue>`, parameterised on a queue
discipline that provides `push`, `pop`, `empty` and `size`. All queue operations happen
under `PoolCore::mutex_`, so the queues themselves are not thread safe.

| Pool | Queue | `push`/`pop` cost | Ordering |
| --- | --- | --- | --- |
| `ThreadPool` | `detail::FifoQueue` (`std::deque<UniqueTask>`) | O(1) | submission order |
| `PriorityThreadPool` | `detail::PriorityQueue` (binary heap in `std::vector`) | O(log n) | highest `TaskPriority` first, FIFO within a priority |

Ties in the priority queue are broken by a monotonically increasing 64-bit sequence number,
not a timestamp: timestamps can collide and are not monotonic under clock adjustment.

```cpp
struct Compare {  // "a has lower precedence than b"
    bool operator()(const Entry& a, const Entry& b) const noexcept {
        if (a.priority != b.priority) return a.priority < b.priority;
        return a.sequence > b.sequence;
    }
};
```

**Worker loop and shutdown invariant.** A worker waits for `stopping_ || !queue_.empty()`
and exits only if the queue is empty after waking:

```cpp
work_cv_.wait(lock, [this] { return stopping_ || !queue_.empty(); });
if (queue_.empty()) {
    return;  // stopping_ and fully drained: graceful exit
}
```

`push` checks `stopping_` and enqueues under the same mutex, so every submission is either
rejected with `PoolShutdownError` or enqueued before any worker can observe `stopping_`.
Together these give the central invariant: **no accepted task is ever dropped**.

Guarantees:

- *Thread safety.* All members may be called concurrently, except that `shutdown()`,
  `wait_idle()` and the destructor must not be called from a worker of the same pool
  (it would wait for itself). `shutdown()` is idempotent; `join_mutex_` serialises
  concurrent callers.
- *Exception safety.* If constructing the N-th worker thread throws, the constructor shuts
  down the already-started workers and rethrows (no leaked joinable `std::thread`). A task
  exception reaches the caller through the `std::future`. Raw tasks given to `post()` that
  throw are swallowed so one faulty task cannot terminate the process.
- *`wait_idle()`* returns when `active_ == 0 && queue_.empty()`; `active_` is incremented
  under the lock in the same critical section that pops the task, so there is no window in
  which a task is neither queued nor counted as active.
- `completed_tasks()` is a relaxed counter; it is a statistic, not a synchronisation point.

### `WorkStealingThreadPool`

Each worker owns a `WorkerQueue` (a mutex plus a `std::deque<UniqueTask>`). The owner pops
LIFO from the back (cache-warm, recently spawned work); thieves steal FIFO from the front
(the oldest, typically largest, piece of work), following the discipline of Blumofe and
Leiserson's work-stealing scheduler. Submissions from outside the pool are distributed
round-robin (`next_queue_`, relaxed); submissions from a worker go to that worker's own
deque, identified through the `thread_local` pair `tls_pool`/`tls_index` in
`ThreadPool.cpp`.

Termination and wake-up correctness rest on one counter, `pending_` (accepted but not yet
popped tasks):

1. `enqueue` checks `stopping_` and increments `pending_` under `sleep_mutex_`, so a task
   is either rejected or counted before shutdown can be observed.
2. A worker only sleeps after re-checking `pending_ == 0` under `sleep_mutex_`, and only
   exits when `stopping_ && pending_ == 0`.
3. If `pending_ > 0` but no deque holds a task, a push is in flight; the worker yields and
   retries rather than sleeping.
4. Before `notify_one`, `enqueue` locks and unlocks `sleep_mutex_`, so a worker that has
   just evaluated its wait predicate cannot miss the notification (the classic lost
   wake-up).

Trade-off: per-deque mutexes make the pool simple and ThreadSanitizer-clean, at the cost of
lock overhead on every pop. The header names the Chase-Lev lock-free deque as the next step
for very fine-grained tasks. `steal_count()` exposes the number of successful steals for
tests and diagnostics.

## Mutual Exclusion (`MutexExamples.hpp`)

### `HierarchicalMutex`

A run-time lock-order checker in the style of Williams (2019, section 3.2.5). Each mutex has
a level; a `thread_local` value `this_thread_hierarchy_level` (initially
`std::numeric_limits<std::uint64_t>::max()`) records the lowest level the thread holds. Locking
a mutex whose level is not strictly lower throws `std::logic_error` *before* blocking.
If all threads respect one global order, the wait-for graph is acyclic and deadlock is
impossible; a violation becomes a deterministic exception instead of a rare hang.
`unlock()` restores the previous level, so mutexes must be released in reverse order.

### Deadlock-free multi-lock: `transfer()` and `run_dining_philosophers()`

`transfer(BankAccount& from, BankAccount& to, std::int64_t amount)` acquires both account
mutexes with `std::scoped_lock`, whose deadlock-avoidance algorithm makes argument order
irrelevant, so concurrent `transfer(a, b)` and `transfer(b, a)` cannot deadlock. It returns
`false` for `amount <= 0`, insufficient funds, or `&from == &to` (which would otherwise
lock the same mutex twice). `run_dining_philosophers(n, meals)` applies the same technique
to the textbook problem and returns the meals eaten per philosopher; it throws
`std::invalid_argument` for `n < 2`.

### Data-structure wrappers

- `Synchronized<T>` couples a value with a `std::shared_mutex`; the value is reachable only
  inside `with_lock(fn)` (exclusive) or `with_shared_lock(fn)` (shared), or by `copy()`.
  This makes "forgot to lock" unrepresentable, but a callback must not leak references.
- `ThreadSafeQueue<T>`: unbounded, single `std::mutex`, blocking `wait_and_pop()` that
  returns `std::nullopt` once the queue is closed and empty.
- `ThreadSafeMap<K, V>`: `std::unordered_map` behind a `std::shared_mutex`. Lookups return
  `std::optional<V>` copies, never references, because a reference would escape the lock.
  `update(key, fn)` is an atomic read-modify-write; `snapshot()` returns a sorted
  `std::map` taken under one shared lock (a consistent cut).
- `LazyValue<T>` uses `std::call_once`; concurrent first callers block until the factory
  finishes. If the factory throws, the `once_flag` is not set and the next `get()` retries
  (the standard's exceptional-call semantics).

## Condition-Variable Primitives (`ConditionalVariables.hpp`)

### `BoundedQueue<T>`

A closable, bounded MPMC FIFO with two condition variables (`not_full_`, `not_empty_`).

- `push` blocks while full (back-pressure); `pop` blocks while empty.
- After `close()`, pushes fail immediately and pops **drain** the remaining elements, then
  return `std::nullopt`; this is the termination signal used by `Actor`, `MessageBus` and
  `Pipeline`.
- Timed variants `push_for`/`pop_for` and non-blocking `try_push`/`try_pop`.
- A failed push does not move from its argument (the element is only forwarded into
  `emplace_back` after the closed check), so callers may retry.
- Notification happens after `lock.unlock()` in `emplace_locked`/`take_locked`.
- `capacity == 0` throws `std::invalid_argument`.

All operations are O(1) amortised. Exception safety is strong: if `emplace_back` throws,
the deque is unchanged.

### Hand-rolled C++20 synchronisation types

`CountingSemaphore`, `CountDownLatch`, `CyclicBarrier` and `ManualResetEvent` mirror
`std::counting_semaphore`, `std::latch`, `std::barrier` and a Win32-style manual-reset
event. Building them from a mutex and a condition variable makes their semantics explicit
and portable to standard libraries that lack the C++20 headers. Notable decisions:

- `CountDownLatch::count_down(n)` saturates at zero instead of exhibiting undefined
  behaviour as `std::latch` does for over-decrement.
- `CyclicBarrier` uses a **generation counter**: a waiter waits for `generation_` to change,
  not for `waiting_` to reach a value, so a fast thread re-entering the next phase cannot be
  confused with stragglers of the previous one. The optional `on_completion` callback is run
  exactly once per phase by the last arriving thread, before any waiter is released;
  `arrive_and_wait()` returns the completed phase index.

### `ResourcePool<T>`

A blocking object pool returning move-only RAII `Lease` handles. `free_` is a stack of
indices reserved to the pool size at construction, so `give_back()` (called from
`Lease::~Lease`) is `noexcept`: `push_back` can never reallocate. The pool must outlive
every lease; this is a documented precondition, not checked.

## Atomics and Lock-Free Structures (`Atomics.hpp`)

### `SpinLock`

Test-and-test-and-set: `exchange(true, acquire)` to acquire; on failure, spin on a
`relaxed` load (keeps the cache line in the shared state instead of bouncing it with RMWs),
calling `cpu_relax()` (`pause` on x86, `yield` on Arm) for 64 iterations and
`std::this_thread::yield()` afterwards. `unlock` is a `release` store, which synchronises with
the next successful acquiring exchange. It meets *Lockable*, so `std::lock_guard<SpinLock>`
works. Appropriate only for very short critical sections; it is not fair.

### `SpscRingBuffer<T, Capacity>`

A bounded, wait-free single-producer/single-consumer queue.

**Layout.** `head_` (consumer-owned) and `tail_` (producer-owned) are on separate 64-byte
cache lines (`kCacheLineSize`; `std::hardware_destructive_interference_size` is avoided
because it is missing from some libraries and GCC warns about its ABI stability). Each side
also keeps a plain, non-atomic *cached copy* of the opposite index on its own line
(`consumer_cached_tail_`, `producer_cached_head_`). Storage is an array of
`alignas(T) std::byte[sizeof(T)]` slots; elements are created with `std::construct_at` and
accessed through `std::launder`, so `T` need not be default-constructible.

**Indices.** `head_` and `tail_` are monotonically increasing `std::size_t` counters masked
with `Capacity - 1` on access. "Empty" is `tail == head` and "full" is
`tail - head == Capacity`, so no slot is wasted. `Capacity` must be a power of two
(`static_assert(std::has_single_bit(Capacity))`); this also makes unsigned wrap-around of the
counters harmless, because 2^64 is a multiple of `Capacity`.

**Memory-ordering argument.**

```cpp
// producer
const std::size_t tail = tail_.load(std::memory_order_relaxed);        // (P1) own index
if (tail - producer_cached_head_ == Capacity) {
    producer_cached_head_ = head_.load(std::memory_order_acquire);      // (P2)
    if (tail - producer_cached_head_ == Capacity) return false;
}
std::construct_at(slot_storage(tail), std::forward<U>(value));        // (P3)
tail_.store(tail + 1, std::memory_order_release);                       // (P4)

// consumer
const std::size_t head = head_.load(std::memory_order_relaxed);        // (C1) own index
if (head == consumer_cached_tail_) {
    consumer_cached_tail_ = tail_.load(std::memory_order_acquire);      // (C2)
    if (head == consumer_cached_tail_) return std::nullopt;
}
std::optional<T> result(std::move(*element));                          // (C3)
std::destroy_at(element);                                               // (C4)
head_.store(head + 1, std::memory_order_release);                       // (C5)
```

1. *Publication.* If (C2) reads the value written by (P4), the release store synchronises
   with the acquire load, so the construction (P3) happens-before the read (C3). Without
   `release`/`acquire` the element access would be a data race.
2. *Slot reuse.* Symmetrically, if (P2) reads the value written by (C5), the move-out and
   destruction (C3, C4) happen-before the producer's next `construct_at` in that slot.
3. *Own index relaxed.* (P1) and (C1) read a variable only the calling thread writes, so
   sequenced-before already orders them; no fence is needed.
4. *Cached indices.* The cached copy is a lower bound on the true opposite index (the
   opposite counter only grows). Acting on a stale cache can only report full/empty
   spuriously, never overrun, and in that case the code re-reads with `acquire`. A stale
   cached value read earlier with `acquire` still carries the happens-before edge for every
   slot it covers. The cache removes the cross-core cache-line transfer on most operations.
5. *No RMW.* Neither side performs a read-modify-write, so each operation completes in a
   bounded number of steps: the structure is wait-free for both endpoints.

`size_approx()` loads both indices with `acquire`; it is exact only when one endpoint calls
it while the other is quiescent. The destructor destroys buffered elements and must not race
with push/pop.

*Exception safety.* If `T`'s constructor throws in (P3), `tail_` is not advanced
(strong guarantee). If the move in (C3) throws, `head_` is not advanced and the element
remains buffered. A failed `try_push` does not consume its argument.

### `LockFreeStack<T>`

A Treiber stack (Treiber 1986) with the "threads in pop" deferred-reclamation scheme of
Williams (2019, section 7.2.2).

**Push** links the new node with a `relaxed` store to `next` (the node is not yet shared)
and publishes it with `compare_exchange_weak(..., release, relaxed)`. The `release` on
success makes the node's contents visible to any popper whose load of `head_` reads it.
Ownership is held in a `std::unique_ptr` until the CAS succeeds, so an allocation or
construction failure leaks nothing.

**Pop** uses default (`seq_cst`) operations for `threads_in_pop_`, `head_` and
`to_be_deleted_`:

```cpp
threads_in_pop_.fetch_add(1);                                  // (1)
Node* old_head = head_.load();                                 // (2)
while (old_head != nullptr &&
       !head_.compare_exchange_weak(old_head, old_head->next.load(std::memory_order_relaxed))) {
}                                                               // (3) unlink
std::optional<T> result;
if (old_head != nullptr) result.emplace(std::move(old_head->value));
try_reclaim(old_head);
```

`Node::next` is `std::atomic<Node*>` because a thread holding a stale `old_head` may read
`next` while the thread that really popped that node re-links it onto the pending list.
The value read in that case is garbage, but the CAS then fails (the node cannot reappear as
head while the reader is inside `pop`), so it is never used.

**Reclamation (`try_reclaim`).**

```cpp
if (threads_in_pop_.load() == 1) {                     // (4) apparently alone
    Node* claimed = to_be_deleted_.exchange(nullptr);  // (5) claim pending list
    if (threads_in_pop_.fetch_sub(1) == 1) {           // (6) still alone?
        delete_chain(claimed);
    } else if (claimed != nullptr) {
        chain_pending(claimed);                        // someone joined: put them back
    }
    std::unique_ptr<Node> reclaim(old_head);           // (7) free own node
} else {
    if (old_head != nullptr) chain_pending(old_head, old_head);
    threads_in_pop_.fetch_sub(1);
}
```

Why this is safe:

- *Own node (7).* Any thread U that might dereference `old_head` must have read it from
  `head_` at step (2) or in a failed CAS, which is before our unlinking CAS (3) in the
  modification order of `head_`. U incremented the counter at (1) before that read. All
  of these are `seq_cst`, so in the single total order S:
  U.(1) < U.(2) < our (3) < our (4). Our load at (4) therefore observes U's increment
  unless U has already decremented, i.e. left `pop` and stopped referencing the node.
  Reading `1` at (4) thus proves no other thread holds `old_head`.
- *Pending list (5)-(6).* Nodes on `to_be_deleted_` have already been unlinked from
  `head_`, so a thread entering `pop` after (4) cannot obtain one through `head_`. It could,
  however, have read a node that a third thread pops and chains onto the list *between*
  (4) and (5). That newcomer is still inside `pop`, so `fetch_sub` at (6) returns a value
  greater than 1 and the claimed chain is put back instead of freed.
- *ABA.* A node's memory is freed only when no concurrent popper can hold its address, so
  an address cannot be recycled by `push` under a popper's feet. The ABA scenario
  (CAS succeeds against a recycled node with a different `next`) is therefore impossible
  without tagged pointers or double-width CAS.

**Trade-offs and progress.** `push` and `pop` are lock-free, not wait-free. Reclamation is
not bounded: under sustained contention `threads_in_pop_` may never drop to 1 and the
pending list grows until a quiescent moment (`pending_reclamation()` is a diagnostic snapshot
valid only when quiescent). The counter is also a shared contention point. Hazard pointers
(Michael 2004) or epoch-based reclamation would bound memory at the cost of considerably
more machinery; for a teaching library the counter scheme gives a complete, verifiable
argument. The destructor frees both chains and must not race with push/pop; `T`'s
destructor is assumed not to throw.

### Smaller atomic components

- `AtomicStatistics` keeps count/sum/min/max as four independent relaxed atomics. min and
  max use CAS loops (`atomic_fetch_min`/`atomic_fetch_max`, portable stand-ins for the
  proposed `fetch_min`/`fetch_max`). A `snapshot()` taken while writers run is not a
  consistent cut across fields; take it after joining the writers.
- `ConcurrentBloomFilter` sets bits with relaxed `fetch_or` on 64-bit words and queries
  with relaxed loads. There are no false negatives once an insertion happens-before the
  query (the caller provides that ordering, e.g. by joining). Hashing is FNV-1a plus double
  hashing so results are identical on every platform.
- `OneShotEvent` uses C++20 `std::atomic<bool>::wait`/`notify_all` (futex-style blocking):
  `set()` is a release store, `wait()` an acquire loop around `flag_.wait(false)`.
- `release_acquire_message_passing(int)` and `relaxed_counter_total(...)` are executable
  demonstrations of message passing through a non-atomic payload and of relaxed counters
  that never lose an update.

## Futures-Based Orchestration (`AsyncMissions.hpp`)

- **Cooperative cancellation.** `CancellationSource` owns a
  `std::shared_ptr<std::atomic<bool>>`; `CancellationToken` holds a
  `shared_ptr<const std::atomic<bool>>`, so it is cheap to copy and outlives its source
  safely. `cancel()` is a release store and `is_cancelled()` an acquire load, so data written
  before cancellation is visible to an observer that sees the flag. It is a callback-free
  analogue of C++20 `std::stop_source`/`std::stop_token`.
- **`AsyncMission<T>`** launches its body with `std::async(policy, ...)` and shares the
  result as `std::shared_future<MissionResult<T>>`. Exceptions become `Failed`,
  `MissionCancelled` becomes `Cancelled`. The destructor waits for a started mission, so the
  object can never be destroyed under its running body; `start()` twice throws
  `std::logic_error`.
- **`MissionCoordinator`** runs a DAG of missions on a `ThreadPool`. `run()` first checks
  acyclicity with Kahn's algorithm (`topological_order()`, smallest id first, O(V + E)) and
  throws `std::logic_error` on a cycle. A mission is launched as soon as its `remaining`
  prerequisite count reaches zero; when a mission fails, all transitive dependents are marked
  `Cancelled` without running, while independent branches continue. `run()` blocks on
  `done_cv_` until `unfinished_ == 0`.
- **`parallel_transform(pool, inputs, fn, chunk_size)`** splits the input into chunks
  (default `size / (threads * 4)`), each writing a disjoint index range of the output, so no
  synchronisation is needed on the output vector. It waits for *every* chunk before
  rethrowing the lowest-indexed error, because the chunks reference the caller's locals.
- **`when_all(std::vector<std::future<T>>)`** joins all futures, preserves order, and rethrows
  the first stored exception after all are ready.
- **`Pipeline<T>`** runs one thread per stage connected by `BoundedQueue<T>`s of
  `queue_capacity`. Each stage is sequential, so output order equals input order, while
  different items occupy different stages concurrently. A stage exception is recorded, every
  queue is closed to unblock all stages, and the first exception is rethrown after all threads
  are joined.

## Message Passing (`AsyncComms.hpp`)

- **`MessageBus`**: topic-based publish/subscribe with a single dispatcher thread, which
  yields a global FIFO delivery order. Subscriptions are stored as
  `std::shared_ptr<const Handler>` under a `std::shared_mutex`; the dispatcher copies the
  matching handler pointers and invokes them with no bus lock held, so handlers may
  `publish`, `subscribe` or `unsubscribe` without deadlock. Publications from the dispatcher
  thread (detected with the `thread_local` `tls_dispatching_bus`) bypass the bounded queue
  into a dispatcher-private `reentrant_` backlog, so a handler can never block on a full
  queue only it could drain. `flush()` waits for quiescence (`delivered_ == published_`)
  rather than for a delivery count, because re-entrant messages are delivered out of queue
  order. Handler exceptions are caught and counted in `handler_failures()`.
- **`Actor<Msg>`**: a `BoundedQueue<Msg>` mailbox drained by a private thread. The behaviour
  runs one message at a time on that thread, so its captured state needs no locks. With a
  `std::variant` message type and a `std::promise` reply slot this implements the "ask"
  pattern. The `std::thread` is the last data member, so it starts only after everything it
  uses is constructed; `stop()` and the destructor process every accepted message.
- **`RequestResponseServer`**: handlers keyed by request type, executed on an internal
  `ThreadPool`. `request()` returns a `Ticket` with a correlation id and a
  `std::future<Response>`; an unknown type fails the future with `UnknownRequestError`. The
  pool is the last member, so it is destroyed (and drained) first while `handlers_` is alive.

## Coroutines (`CoroutinesDemo.hpp`)

C++20 standardised the coroutine machinery (P0057, adopted via P0912) but almost no
coroutine types; this header builds the two fundamental ones.

- **`Generator<T>`** is a lazy, move-only, single-pass input range. `initial_suspend` and
  `final_suspend` both return `std::suspend_always`; the promise stores a *pointer* to the
  yielded object, which is safe because the yielded object (even a temporary) lives until
  the coroutine is resumed. `await_transform` is deleted, making `co_await` inside a generator
  ill-formed. An exception in the body is captured by `unhandled_exception` and rethrown to the
  consumer from `begin()` or `operator++`. The iterator models `std::input_iterator` with
  `std::default_sentinel_t` as the end, so `Generator` satisfies `std::ranges::input_range`.
  Combinators `take` and `filter` and sequences `iota_range`, `fibonacci` and `collatz` are
  themselves generators.
- **`Task<T>`** is lazy (`initial_suspend` is `suspend_always`) and resumes its awaiter via
  **symmetric transfer**: `FinalAwaiter::await_suspend` returns the stored continuation
  handle (default `std::noop_coroutine()`), and `Task::Awaiter::await_suspend` returns the
  task's own handle. Returning a handle instead of calling `resume()` turns nested
  `co_await` chains into tail calls, so their depth does not grow the stack (P0913).
  `Task<T&>` is rejected by `static_assert`.

```cpp
struct FinalAwaiter {
    bool await_ready() const noexcept { return false; }
    template <typename Promise>
    std::coroutine_handle<> await_suspend(std::coroutine_handle<Promise> h) const noexcept {
        return h.promise().continuation;   // symmetric transfer
    }
    void await_resume() const noexcept {}
};
```

- **`sync_wait(Task<T>)`** wraps the task in an internal `SyncWaitTask` coroutine whose
  final awaiter signals a `SyncWaitEvent` *from `await_suspend`*, i.e. after the wrapper
  coroutine is suspended. The waiting thread can therefore destroy the frame as soon as
  `wait()` returns. `SyncWaitEvent::set()` notifies while holding the lock so the event may
  be destroyed immediately after the waiter wakes. `void` results are carried as
  `std::monostate`.
- **`schedule_on(ThreadPool&)`** returns a `ScheduleOnAwaiter` whose `await_suspend` posts
  `handle.resume()` to the pool through `ThreadPool::post`; execution continues on a pool
  thread. If the pool is shut down, `PoolShutdownError` propagates out of `co_await`.
- **`RoundRobinScheduler`** is a single-threaded cooperative scheduler. `spawn` wraps each
  `Task<void>` in an internal driver coroutine that catches exceptions and counts them in
  `failed_tasks()`; `co_await scheduler.yield()` re-queues the current coroutine at the back
  of `ready_`, giving deterministic interleaving. `spawn` reserves vector and deque slots
  before releasing ownership of the driver handle, so an allocation failure cannot leak a
  frame. The destructor destroys unfinished frames. It is not thread safe by design.

## Verification

Tests use Catch2 and are registered as the `concurrency_tests` target in
`tests/concurrency/CMakeLists.txt` (label `concurrency`). Configure with
`-DCPPVERSEHUB_ENABLE_TSAN=ON` (see `cmake/Sanitizers.cmake`) to run them under
ThreadSanitizer.

| Claim | Test file and representative cases |
| --- | --- |
| Futures carry results and exceptions; move-only tasks and arguments; graceful shutdown; no lost task when submitters race with shutdown; priority order with FIFO ties; work stealing actually steals; subtasks spawned from workers | `tests/concurrency/ThreadPoolTests.cpp` ("ThreadPool loses no task when submitters race with shutdown", "PriorityThreadPool runs higher priorities first, FIFO within a priority", "WorkStealingThreadPool idle workers steal from a busy worker") |
| Hierarchy violations throw; opposing transfers neither deadlock nor lose money; philosophers finish; `LazyValue` initialises once; `BoundedQueue` close/back-pressure/MPMC exactly-once; semaphore bounds concurrency; barrier runs completion once per phase; `ResourcePool` blocks when exhausted | `tests/concurrency/SynchronizationTests.cpp` |
| `SpinLock` mutual exclusion; SPSC FIFO, full/empty, non-trivial lifetimes, ordered two-thread stream; `LockFreeStack` delivers every value exactly once under concurrent push/pop; statistics, `fetch_max/min`, Bloom filter has no false negatives; `OneShotEvent` publishes prior writes | `tests/concurrency/AtomicsTests.cpp` |
| Mission status/cancellation; DAG ordering, transitive cancellation, cycle detection; `parallel_transform`, `when_all`, `Pipeline` order and error propagation; `MessageBus` order, re-entrancy, `flush`; `Actor` ask and drain; request/response; `runDemo` output | `tests/concurrency/AsyncTests.cpp` |
| `Generator` models `input_range`, laziness, exception rethrow, `std::views` interop; `Task` laziness, exception propagation, deep symmetric-transfer chains; `schedule_on` resumes on pool threads; deterministic round-robin and failure counting | `tests/concurrency/CoroutineTests.cpp` |

Memory-ordering arguments cannot be proven by testing; the two-thread SPSC stream and the
concurrent stack test are designed to make violations observable under ThreadSanitizer
(which models the C++ happens-before relation) rather than to prove their absence.

## References

1. ISO/IEC 14882:2020, *Programming Languages - C++*, [intro.races] and [atomics.order].
2. G. Nishanov, *Wording for Coroutines*, P0057R8, 2018, and *Merge Coroutines TS into
   C++20 working draft*, P0912R5, 2019.
3. G. Nishanov, *Add symmetric coroutine control transfer*, P0913R1, 2018.
4. A. Williams, *C++ Concurrency in Action*, 2nd ed., Manning, 2019 (section 3.2.5
   hierarchical mutex; section 7.2.2 reference-counted reclamation for a lock-free stack).
5. M. Herlihy, N. Shavit, *The Art of Multiprocessor Programming*, Morgan Kaufmann, 2008
   (ch. 3 linearizability and progress; ch. 7 spin locks; ch. 10-11 queues and stacks).
6. R. K. Treiber, *Systems Programming: Coping with Parallelism*, IBM RJ 5118, 1986.
7. M. M. Michael, "Hazard Pointers: Safe Memory Reclamation for Lock-Free Objects",
   *IEEE TPDS* 15(6), 2004.
8. D. Chase, Y. Lev, "Dynamic Circular Work-Stealing Deque", *SPAA* 2005.
9. R. D. Blumofe, C. E. Leiserson, "Scheduling Multithreaded Computations by Work
   Stealing", *JACM* 46(5), 1999.
10. H. Boehm, S. Adve, "Foundations of the C++ Concurrency Memory Model", *PLDI* 2008.
11. cppreference.com: [`std::memory_order`](https://en.cppreference.com/w/cpp/atomic/memory_order),
    [`std::condition_variable`](https://en.cppreference.com/w/cpp/thread/condition_variable),
    [Coroutines](https://en.cppreference.com/w/cpp/language/coroutines),
    [`std::scoped_lock`](https://en.cppreference.com/w/cpp/thread/scoped_lock).
