# Design Patterns Explained

## Purpose and Scope

This document describes the `CppVerseHub::Patterns` library (`src/patterns/`), which
implements eight of the Gamma et al. (1994) patterns in C++20. For most patterns the module
shows the classic object-oriented form next to one or more modern alternatives (concepts,
`std::variant`, `std::function`, RAII handles, type-state templates), because in
contemporary C++ the pattern *intent* is often better served by a different mechanism than
the 1994 class diagram.

The audience is engineers and researchers who know the patterns and want the specific
decisions, invariants and guarantees of this implementation. The domain vocabulary
(planets, fleets, missions, warp drives) is illustrative only.

| Pattern | Header | Classic form | Modern form(s) |
| --- | --- | --- | --- |
| Singleton | `Singleton.hpp` (header-only) | `Singleton<Derived>` CRTP base | function-local static ("magic static") |
| Observer | `Observer.hpp` | `Subject<Event>` / `IObserver<Event>` | `Signal<Args...>`, `Connection`, `ScopedConnection` |
| Strategy | `Strategy.hpp` | `IRoutingStrategy` + `FleetRouter` | `RoutingPolicy` concept + `StaticRouter<Policy>`; `TargetSelector` (`std::function`) |
| Builder | `Builder.hpp` | `SpacecraftBuilder` + `ShipyardDirector` | `BlueprintBuilder<HasName, HasHull>` type-state builder |
| Command | `Command.hpp` | `ICommand`, `CommandHistory` | `LambdaCommand`, transactional `MacroCommand` |
| Adapter | `Adapter.hpp` | `RadioAdapter` (object), `ThermalSensorAdapter` (class) | `CallbackBridge` (C-callback trampoline) |
| Decorator | `Decorator.hpp` | `MissionDecorator` hierarchy, `decorate<D>()` | `withRetry`, `withCallCounter`, `memoize` |
| State | `State.hpp` | `MissionContext` / `IMissionState` | `VariantStateMachine` + `Overloaded`, `WarpDrive` |

All symbols live in namespace `CppVerseHub::Patterns`. Each header exposes a
`demonstrateX(std::ostream&)` function, and `Demo.hpp` declares `runDemo(std::ostream&)`,
which runs every showcase deterministically and never throws.

### Cross-cutting conventions

- **Interfaces protect their special members.** Abstract bases (`IObserver`,
  `IRoutingStrategy`, `ICommand`, `ICommunicationChannel`, `ITemperatureSensor`, `IMission`,
  `IMissionState`) declare a public virtual destructor and *protected* copy/move operations,
  which prevents slicing through a base reference while leaving derived classes free to be
  copyable.
- **Ownership is explicit.** Owning relationships use `std::unique_ptr` (`CommandPtr`,
  `MissionPtr`, `StateOutcome::next`); non-owning ones use references or raw pointers with a
  documented "must outlive" precondition, or `std::weak_ptr` where lifetimes are genuinely
  independent (Observer).
- **Validation reports, rather than guesses.** Invalid input is rejected with a typed
  exception (`BuildError`, `CommandError`, `std::invalid_argument`) or a `false`/`std::nullopt`
  return, never silently clamped unless the header says so.

## Singleton

**Intent.** Exactly one instance, globally accessible, created on first use.

**Decision.** The Meyers singleton in a CRTP base:

```cpp
template <typename Derived>
class Singleton {
public:
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
    [[nodiscard]] static Derived& instance() {
        static Derived inst;  // thread-safe initialisation guaranteed by the language
        return inst;
    }
protected:
    Singleton() = default;
    ~Singleton() = default;
};
```

Since C++11, initialisation of a block-scope static is performed exactly once even under
concurrent first calls ([stmt.dcl]); the compiler emits the guard (typically a
double-checked flag plus a lock, as in the Itanium C++ ABI `__cxa_guard_acquire`). This
removes the need for `std::call_once`, explicit locking or heap allocation, and the object is
destroyed at program exit in reverse order of construction. Derived classes make their
constructors private and befriend `Singleton<Derived>`.

**Trade-offs considered.** Double-checked locking by hand is error-prone and adds nothing
over the language guarantee; a heap-allocated, never-destroyed instance avoids
destruction-order problems but leaks by design. The chosen form keeps the
*static destruction order* caveat: calling `instance()` from another static object's
destructor after the singleton is destroyed is undefined behaviour.

**Services.** The three concrete singletons each choose a different synchronisation strategy
for their *mutable state*, which the singleton machinery does not protect:

| Class | State | Synchronisation |
| --- | --- | --- |
| `ConfigManager` | `std::unordered_map<std::string, std::string>` | `std::shared_mutex` (concurrent `get`/`contains`/`size`, exclusive `set`/`reset`) |
| `LogManager` | bounded `std::deque<Record>` (`kCapacity = 1024`, oldest dropped) | `std::mutex`; minimum level is a relaxed `std::atomic<LogLevel>` |
| `IdGenerator` | `std::atomic<std::uint64_t>` | lock-free `fetch_add(1, relaxed)`; ids start at 1 and are unique |

Relaxed ordering is sufficient for `IdGenerator` because uniqueness follows from the atomicity
of the read-modify-write, and no other data is published through the counter. Because global
state hampers testing, `ConfigManager` and `LogManager` expose `reset()`; the header
explicitly recommends dependency injection for new code.

## Observer

**Intent.** One-to-many notification without coupling the subject to concrete observers.

### Classic form: `Subject<Event>` with weak registration

`Subject` stores `std::vector<std::weak_ptr<IObserver<Event>>>`. The subject never extends an
observer's lifetime, so a destroyed observer simply stops receiving events and the
dangling-observer bug of the raw-pointer GoF version cannot occur. Expired entries are pruned
lazily during `notify` and `detach`.

`notify(event)` takes a snapshot of live observers (locked `weak_ptr`s) under the mutex and
invokes them without holding it, so observers may attach or detach re-entrantly. If observers
throw, every remaining observer is still notified and the *first* exception is rethrown
afterwards; the return value counts successful deliveries. `attach` rejects null and duplicate
registrations. Complexity: `attach` is O(n) (duplicate check), `notify` O(n).

### Modern form: `Signal<Args...>` with RAII connections

```cpp
template <typename... Args>
class Signal {
    struct Slot {
        std::function<void(Args...)> fn;
        std::atomic<bool> active{true};  // cleared on disconnect
    };
    struct Core final : detail::SignalCore { std::vector<std::shared_ptr<Slot>> slots; /* remove() */ };
    std::shared_ptr<Core> core_ = std::make_shared<Core>();
    ...
};
```

- `connect(fn)` returns a copyable, non-owning `Connection` holding a
  `std::weak_ptr<detail::SignalCore>` and a `std::weak_ptr<const std::atomic<bool>>` built with
  the `shared_ptr` *aliasing constructor* (shares ownership of the slot, points at its flag).
  Because both are weak, `disconnect()` is idempotent and safe even after the `Signal` has been
  destroyed.
- `ScopedConnection` is the move-only RAII owner: it disconnects on destruction or
  move-assignment; `release()` gives up ownership without disconnecting. `connectScoped()`
  returns one directly.
- `emit(args...)` copies the slot vector under the mutex and invokes the copy unlocked. Each
  slot's `active` flag is re-checked (acquire) immediately before invocation, so a slot removed
  earlier in the same emission (including by itself) is skipped.

**Guarantees and limits.** All `Signal` operations are thread safe. `disconnect()` does not
wait for an invocation already in progress on another thread, so a slot may still run once
after a concurrent `disconnect()` returns; callers that destroy captured state must
synchronise separately. Exceptions from a slot propagate out of `emit` and later slots in that
emission are not called (unlike `Subject::notify`). `emit` is O(n) plus one vector copy.

The domain class `ObservablePlanet` publishes `PlanetEvent`s to both a `Subject` and a
`Signal`, and only when a value actually changes; `ResourceMonitor`, `DefenseMonitor` and
`EventLogger` are the concrete observers.

## Strategy

**Intent.** Interchangeable algorithms behind one interface, selectable at run time.

Three styles are implemented for two problems (route planning and target selection), so the
costs can be compared directly:

1. **Run-time polymorphism.** `IRoutingStrategy::plan(from, to, ctx)` is implemented by
   `DirectLineStrategy`, `FuelOptimizedStrategy` (reduced throttle; fuel scales with
   throttle squared in `evaluateRoute`), `SafeRouteStrategy` (inserts detour waypoints at
   `margin * radius` from any hazard a leg crosses, refining at most 32 times and skipping
   hazards that contain an endpoint) and `BalancedStrategy` (a meta-strategy that runs the
   other three and returns the lowest `score(route, RouteWeights)`). `FleetRouter` is the
   context; `setStrategy` rejects null with `std::invalid_argument`. `makeRoutingStrategy`
   is a simple factory over `RoutingStrategyType`.
2. **Compile-time policy.** A concept replaces the interface:

   ```cpp
   template <typename P>
   concept RoutingPolicy = requires(const P& p, const Coordinate3D& c, const NavigationContext& ctx) {
       { p.plan(c, c, ctx) } -> std::same_as<Route>;
   };
   template <RoutingPolicy Policy> class StaticRouter { /* policy_.plan(from, to, ctx_) */ };
   ```

   Dispatch is static and inlinable, there is no heap allocation, and an unsuitable policy is
   rejected at the point of instantiation with a concept diagnostic. The cost is that the
   algorithm cannot change at run time and each policy instantiates a new router type
   (Alexandrescu's policy-based design, 2001, ch. 1).
3. **Function objects.** `TargetSelector` is
   `std::function<std::optional<std::size_t>(std::span<const PlanetTarget>, const Coordinate3D&)>`.
   `nearestTarget`, `highestValueTarget` and `bestValueRatioTarget` are free functions;
   `weakerThan(maxDefense, inner)` composes selectors and returns indices into the
   *original* span. This is the lightest-weight form for stateless algorithms.

All algorithms are deterministic. `segmentIntersects` uses the closest point on the segment to
the sphere centre, so `evaluateRoute` is O(waypoints x hazards).

## Builder

**Intent.** Separate the construction of a complex object from its representation, and
enforce invariants that a constructor cannot express readably.

- **Fluent builder.** `SpacecraftBuilder` accumulates a name, `HullClass`, crew and
  `Component`s; every setter returns `*this`. `validate()` returns *all* violated invariants
  at once (missing name or hull, no engine, crew below `minCrew(hull)`, total mass above
  `maxMass(hull)`, negative component mass, power deficit), and `build()` throws `BuildError`
  carrying that list. `BuildError` derives from `std::invalid_argument`. `build()` is `const`
  and copies the parts, so one configured builder can stamp out several ships. The product
  `Spacecraft` has a private constructor and is reachable only through the builder, so every
  `Spacecraft` that exists satisfies the invariants.
- **Director.** `ShipyardDirector::buildScout/buildFrigate/buildCarrier` encode standard
  recipes so clients need not know the steps.
- **Hull tables.** `maxMass`, `minCrew` and `hullMass` are `constexpr` functions, so the
  limits are usable in constant expressions and in tests.
- **Aggregate builder.** `FleetBuilder` composes `Spacecraft` products into a `Fleet` and
  validates that the fleet is named and non-empty.

**Type-state builder.** `BlueprintBuilder<HasName, HasHull>` moves the "mandatory step"
check from run time to compile time:

```cpp
template <bool HasName = false, bool HasHull = false>
class BlueprintBuilder {
public:
    [[nodiscard]] BlueprintBuilder<true, HasHull> withName(std::string name) &&;
    [[nodiscard]] BlueprintBuilder<HasName, true> withHull(HullClass hull) &&;
    [[nodiscard]] BlueprintBuilder withHardpoints(std::size_t n) &&;
    [[nodiscard]] Blueprint build() && requires(HasName && HasHull);
};
template <typename B>
concept BuildableBlueprint = requires(B b) { std::move(b).build(); };
```

Each mandatory step returns a builder of a different type; `build()` is constrained by a
`requires` clause, so `BlueprintBuilder<>{}.withName("x").build()` does not compile. All
steps are `&&`-qualified, which forces a single linear chain and prevents reuse of a
moved-from builder. `BuildableBlueprint` lets tests assert the guarantee with
`static_assert`. The trade-off is one template instantiation per state and less flexible
composition (a builder cannot be stored in a variable whose type is independent of progress).

## Command

**Intent.** Encapsulate a request as an object so it can be queued, logged and undone.

- `ICommand` has `execute()`, `undo()` and `name()`. Each concrete command stores exactly the
  data needed to reverse itself: `MoveFleetCommand` remembers the previous location and refunds
  fuel; `AttackCommand` stores a full `FleetStatus` snapshot and restores it via
  `FleetReceiver::restore`; `ReinforceCommand` applies the inverse delta.
- **Strong exception guarantee.** `FleetReceiver::move/attack/reinforce` validate before
  mutating and throw `CommandError` on failure, so `execute()` either succeeds or leaves the
  receiver untouched.
- **Transactional macro.** `MacroCommand` executes children in order; if child *k* throws,
  children 0..k-1 are undone in reverse order before the exception propagates:

  ```cpp
  void MacroCommand::execute() {
      std::size_t done = 0;
      try {
          for (; done < children_.size(); ++done) children_[done]->execute();
      } catch (...) {
          while (done > 0) { --done; children_[done]->undo(); }
          throw;
      }
  }
  ```

  This assumes `undo()` of a successfully executed child does not itself fail.
- `LambdaCommand` builds a command from two `std::function<void()>`s; both are required.
- **Invoker.** `CommandHistory(capacity)` holds a `std::deque<CommandPtr>` undo stack and a
  `std::vector<CommandPtr>` redo stack. `execute` runs the command first and records it only on
  success; recording clears the redo stack (editor semantics). When the undo stack exceeds
  `capacity()`, the oldest entry is evicted (`evicted()` counts them), bounding memory at
  O(capacity). If `undo()` throws, the command stays on the undo stack. `capacity == 0` throws
  `std::invalid_argument`. All operations are amortised O(1).

The history is not thread safe; it is an invoker for a single control thread.

## Adapter

**Intent.** Convert the interface of an existing class into the one clients expect.

Three flavours, each solving a different integration problem:

- **Object adapter (composition).** `RadioAdapter` implements `ICommunicationChannel` on top of
  a non-owned `LegacyRadio` whose API uses `const char*` frames, a `kMaxFrame = 256` byte limit
  and negative integer error codes. `encodeFrame`/`decodeFrame` define a reversible
  `TO|FROM|P|BODY` wire format with backslash escaping of `|` and `\`, so arbitrary bodies
  round-trip. Error codes become `false` returns; malformed frames are dropped and counted in
  `malformedFrames()`.
- **Class adapter (private inheritance).** `ThermalSensorAdapter` inherits
  `ITemperatureSensor` publicly and `LegacyThermalSensor` *privately*: the adaptee is an
  implementation detail, not an is-a relationship. Centi-kelvin integers become degrees
  Celsius; `using LegacyThermalSensor::readCentiKelvin;` selectively re-exposes one adaptee
  member. Compared with the object adapter this avoids an indirection and allows overriding
  adaptee behaviour, at the price of tighter coupling.
- **Callback adapter.** `CallbackBridge` lets a `std::function<void(int)>` receive events from a
  `LegacyEventPump` that accepts only `void (*)(int, void*)` plus a user-data pointer, using the
  standard trampoline:

  ```cpp
  id_ = pump_->registerCallback(&CallbackBridge::trampoline, this);
  void CallbackBridge::trampoline(int code, void* self) {
      static_cast<CallbackBridge*>(self)->handler_(code);
  }
  ```

  Because the pump stores `this`, the bridge is non-copyable and non-movable, and it
  unregisters itself in its destructor (RAII), so the pump can never call into a destroyed
  bridge. An empty handler is rejected with `std::invalid_argument`.

## Decorator

**Intent.** Attach responsibilities to an object dynamically, without a combinatorial
explosion of subclasses.

- **Object decorators.** `MissionDecorator` owns its inner `MissionPtr` (null rejected) and
  forwards every `IMission` call. `StealthEnhancement`, `SpeedBoost`, `HeavyArmament` and
  `MedicalSupport` override selected members; for example `SpeedBoost` multiplies duration by
  0.7 and cost by 1.25. Success bonuses are applied as a fraction of the remaining failure
  probability, `p + (1 - p) * f` with both clamped to [0, 1], so stacking decorators can never
  push probability above 1. `enhancements()` lists the applied decorators innermost first.
  `decorate<D>(inner, args...)` is a constrained factory
  (`requires std::is_base_of_v<MissionDecorator, D>`). With *k* decorator types, any
  combination needs *k* classes rather than 2^k.
- **Function decorators.** For cross-cutting concerns on callables, higher-order templates are
  the idiomatic C++ equivalent:
  - `withRetry(fn, attempts)` re-invokes `fn` while it throws and rethrows the last exception.
    Arguments are passed as lvalues on every attempt (not forwarded), so a failed attempt cannot
    leave a moved-from argument for the next one. `attempts == 0` throws `std::invalid_argument`.
  - `withCallCounter(fn, std::shared_ptr<std::size_t>)` increments a shared counter per call;
    the counter is not atomic, so the wrapper is not thread safe.
  - `memoize<R, Args...>(fn)` caches results in a `std::map<std::tuple<std::decay_t<Args>...>, R>`
    behind a mutex shared by all copies. The function is evaluated *outside* the lock; a
    racing duplicate evaluation is harmless for a pure function because `emplace` keeps the
    first value. Lookups are O(log n). `std::type_identity_t` in the parameter prevents
    deduction from the argument, so `R` and `Args` are always given explicitly.

## State

**Intent.** Let an object change its behaviour when its internal state changes.

### Classic form: `MissionContext` / `IMissionState`

`MissionContext` holds a `std::unique_ptr<IMissionState>` and delegates each event (`plan`,
`launch`, `pause`, `resume`, `advance`, `fail`, `abort`) to it. Every handler in the base class
rejects by default, so a concrete state overrides only the events it accepts. Handlers return a
`StateOutcome` (`reject()`, `stay()` or `to(next)`), and `MissionContext::apply` performs the
transition, records a `PhaseTransition` in `history()`, or increments `rejectedEvents()`.
States never replace themselves while executing, which avoids the classic
"`delete this` inside a member function" hazard. Lifecycle:
`Pending -plan-> Planning -launch-> Active <-pause/resume-> Paused`,
`Active --advance to 100%--> Completed`, `Active/Paused -fail-> Failed`, and any non-terminal
phase `-abort-> Aborted`.

### Value form: `VariantStateMachine` over `std::variant`

```cpp
bool dispatch(const EventVariant& event) {
    std::optional<StateVariant> next = std::visit(
        [this](const auto& s, const auto& e) -> std::optional<StateVariant> { return transitions_(s, e); },
        state_, event);
    if (!next) { ++rejected_; return false; }
    state_ = std::move(*next);
    ++accepted_;
    return true;
}
```

The state is a `std::variant` of plain structs, each carrying only the data meaningful in that
state (`Warp::Charging::percent`, `Warp::Jumping::destination`,
`Warp::Cooldown::ticksRemaining`). The transition table `Warp::Transitions` is an overload set;
multi-variant `std::visit` dispatches on the (state, event) pair in O(1) via a compiler-generated
jump table, with no heap allocation and no virtual calls. Overload resolution provides the
precedence rules: non-template exact matches beat templates, and partial ordering makes
`template <S> (const S&, const Shutdown&)` more specialised than the catch-all
`template <S, E> (const S&, const E&)` that rejects every unlisted pair. Because `std::visit`
requires the visitor to be callable for every combination, a missing transition is a
*compile-time* error unless the catch-all handles it, which makes the table exhaustive by
construction. `Overloaded` (with its deduction guide) builds visitors from lambdas.

Exception safety: if the transition function throws, neither the state nor the counters
change. If move-assigning the next state threw, the variant could become
`valueless_by_exception`; for `Warp::State` every alternative (including `Jumping`, whose only
member is a `std::string`) is nothrow-move, so the assignment cannot throw. `WarpDrive` wraps a
concrete machine and records completed jumps in `jumps()` and `log()`.

**Trade-off.** The OO form is open to new states without touching existing code but costs a
heap allocation per transition and virtual dispatch; the variant form is closed (adding a state
recompiles the table) but is value-semantic, allocation-free and checked by the compiler.

## Verification

Tests use Catch2 and live in `tests/patterns/` (built by `tests/patterns/CMakeLists.txt`).

| Claim | Test file and representative cases |
| --- | --- |
| Singleton non-copyable/non-movable/not publicly constructible; concurrent first access constructs once; `ConfigManager` concurrent readers/writers; `IdGenerator` unique across threads; `LogManager` level filter and bound | `tests/patterns/SingletonTests.cpp` |
| `Subject` does not extend lifetime, rejects null/duplicates, keeps notifying after a throw; `ScopedConnection` RAII and move; `Connection` outliving its `Signal`; self-disconnection during emission; concurrent connect/emit/disconnect | `tests/patterns/ObserverTests.cpp` |
| Strategy metrics, hazard avoidance, balanced selection, static policy, selector composition | `tests/patterns/StrategyTests.cpp` |
| `validate` reports every violated invariant; mass/power/crew limits; builder reuse; director recipes; constexpr hull tables; type-state `build()` only after mandatory steps | `tests/patterns/BuilderTests.cpp` |
| Undo/redo semantics, bounded history and eviction, transactional macro rollback, strong guarantee of receiver operations | `tests/patterns/CommandTests.cpp` |
| Frame encode/decode round trip with escaping, legacy error-code mapping, class adapter conversions, callback bridge RAII unregistration | `tests/patterns/AdapterTests.cpp` |
| Decorator stacking and probability bounds, `withRetry`, `withCallCounter`, `memoize` | `tests/patterns/DecoratorTests.cpp` |
| Mission happy path, rejected events, pause/resume, terminal failure, abort from every non-terminal phase; warp charging/jump/cooldown, rejected events, generic `VariantStateMachine`, `Overloaded` | `tests/patterns/StateTests.cpp` |
| `runDemo` runs every showcase and writes to the given stream | `tests/patterns/DemoTests.cpp` |

## References

1. E. Gamma, R. Helm, R. Johnson, J. Vlissides, *Design Patterns: Elements of Reusable
   Object-Oriented Software*, Addison-Wesley, 1994.
2. A. Alexandrescu, *Modern C++ Design: Generic Programming and Design Patterns Applied*,
   Addison-Wesley, 2001 (ch. 1 policy-based design; ch. 6 singletons and lifetime).
3. S. Meyers, *More Effective C++*, Addison-Wesley, 1996, Item 26, and S. Meyers,
   A. Alexandrescu, "C++ and the Perils of Double-Checked Locking", *Dr. Dobb's Journal*, 2004.
4. ISO/IEC 14882:2020, [stmt.dcl] (block-scope static initialisation), [variant.visit],
   [temp.func.order] (partial ordering of function templates).
5. A. Williams, *C++ Concurrency in Action*, 2nd ed., Manning, 2019 (section 3.3 protecting
   shared data during initialisation).
6. J. O. Coplien, "Curiously Recurring Template Patterns", *C++ Report*, 1995.
7. F. M. Hess, D. Gregor, *Boost.Signals2* documentation (thread-safe signals and scoped
   connections).
8. cppreference.com: [`std::variant`](https://en.cppreference.com/w/cpp/utility/variant),
   [`std::visit`](https://en.cppreference.com/w/cpp/utility/variant/visit),
   [`std::weak_ptr`](https://en.cppreference.com/w/cpp/memory/weak_ptr),
   [`std::shared_ptr` aliasing constructor](https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr),
   [Constraints and concepts](https://en.cppreference.com/w/cpp/language/constraints).
