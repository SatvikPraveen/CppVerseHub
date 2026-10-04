# Modern C++ Usage

## Purpose and Scope

This document describes two closely related modules:

- `CppVerseHub::Modern` (`src/modern/`): language features from C++11 to C++20 (concepts,
  compile-time programming, lambdas, move semantics, ranges, structured bindings, and a
  documented emulation of C++20 modules). Compiled as a library; each header has a matching
  `.cpp` except the header-only `ConceptsAdvanced.hpp`, `ConstexprProgramming.hpp` and
  `StructuredBindings.hpp`.
- `CppVerseHub::Templates` (`src/templates/`): generic programming (concepts, SFINAE and the
  detection idiom, specialisation, variadic templates, template metaprogramming, and
  hand-written containers and smart pointers). Header-only.

For each component it records the design decision, the alternatives considered, the
invariants and exception-safety guarantees, and where the claims are verified. The intended
reader already knows the features; the emphasis is on *how this code uses them* and on the
non-obvious details.

| Header | Namespace | Main entities |
| --- | --- | --- |
| `modern/ConceptsAdvanced.hpp` | `Modern::Concepts` | `Numeric`, `Printable`, `Container`, `RandomAccessContainer`, `SpaceEntity`, `MovableSpaceEntity`, `classify`, `profileOf<T>()`, `ConceptFactory<T>`, `RunningStats<T>` |
| `modern/ConstexprProgramming.hpp` | `Modern::ConstexprProgramming` | `power`, `sqrtNewton`, `adaptiveSqrt`, `compileTimeFactorial` (consteval), `FixedString<N>`, `NamedTag<Name>`, `makeTable<N>`, `ConstexprPlanet`, `ConstexprFleet` |
| `modern/LambdaExpressions.hpp` | `Modern::LambdaExpressions` | `compose`, `pipeline`, `curry`, `Overloaded`, `Fix`, `Memoized`, `Beacon`, `EventBus`, `parallelSum` |
| `modern/MoveSemantics.hpp` | `Modern::MoveSemantics` | `TrackedResource`, `OperationCounts`, `Spacecraft`, `categoryOf`, `relayForwarded`, `MoveAwareVector<T>` |
| `modern/RangesDemo.hpp` | `Modern::Ranges` | `toVector`, `EveryNthView<V>`, `everyNth(n)`, domain queries |
| `modern/StructuredBindings.hpp` | `Modern::StructuredBindings` | `ShipRecord` (tuple-like), `SpaceCoordinate`, `FleetStats`, `orbitParameters`, `refuelBelow` |
| `modern/ModulesDemo.hpp` | `Modern::Modules` | `SpaceGame::{Core, Entities, Missions, Fleet, System}`, `ModuleUnit`, `moduleGraph()`, `topologicalBuildOrder()` |
| `templates/ConceptsDemo.hpp` | `Templates::Concepts` | concept hierarchy `Arithmetic` to `Field`, `classify`, `power(T, Unsigned auto)`, `ContainerAdapter`, `MathVector` |
| `templates/SFINAE_Examples.hpp` | `Templates::SFINAE` | `has_size_method`, `is_detected`/`detected_or_t`, tag dispatch, `modern_describe` |
| `templates/TemplateSpecialization.hpp` | `Templates::Specialization` | `Serializer<T>`, `TypeInfo<T>`, `ContainerPrinter`, `TupleProcessor`, `element_type` |
| `templates/VariadicTemplates.hpp` | `Templates::Variadic` | folds, `RecursiveTuple`, `overload`, `multifunction`, `Factory`, `Builder`, `Pipeline` |
| `templates/MetaProgramming.hpp` | `Templates::Meta` | `type_list`, `type_at`, `Ratio`, `dimension`/`quantity`, `VectorExpression`/`ExprVector`, `ConstexprMap`, CRTP mixins |
| `templates/GenericContainers.hpp` | `Templates` | `DynamicArray<T, Allocator>`, `UniquePtr<T, D>`, `SharedPtr<T>`/`WeakPtr<T>`, `make_shared_ptr`, `Optional<T>` |

All namespaces are nested in `CppVerseHub`. Both modules expose a `runDemo(std::ostream&)`
entry point (`modern/Demo.hpp`, `templates/Demo.hpp`) that is deterministic and never throws.
Library code never prints; only the `demonstrate*` functions write to a stream.

### Portability policy

Only C++20 library facilities that are available in both libc++ and libstdc++ are used. C++23
facilities are replaced by local equivalents and named as such: `toVector` stands in for
`std::ranges::to`, `EveryNthView` for `std::views::stride`, and `Optional::transform`/`and_then`
mirror the C++23 monadic interface of `std::optional`. Feature-test macros guard optional library support,
for example `__cpp_lib_constexpr_vector` in `sumOfSquaresViaVector`.

## Concepts (`ConceptsAdvanced.hpp`, `ConceptsDemo.hpp`)

**Decision.** Constraints are expressed as named concepts built by *conjunction* of smaller
concepts, so that the compiler can order overloads by subsumption ([temp.constr.order]). Both
headers implement `classify(const T&)`, an overload set in which the most constrained viable
candidate wins without SFINAE or tag dispatch:

```cpp
template <Numeric T>
[[nodiscard]] constexpr std::string_view classify(const T&) noexcept { return "numeric"; }

template <Numeric T>
    requires std::integral<T>      // Numeric<T> && integral<T> subsumes Numeric<T>
[[nodiscard]] constexpr std::string_view classify(const T&) noexcept { return "integral"; }

template <RandomAccessContainer T> // defined as Container<T> && ..., subsumes Container
[[nodiscard]] constexpr std::string_view classify(const T&) noexcept { return "random-access container"; }
```

Subsumption only works between *atomic constraints that are the same expression from the same
concept definition*; this is why `RandomAccessContainer` is defined in terms of `Container`
rather than restating its requirements. `Numeric` deliberately excludes `bool`
(`!std::same_as<std::remove_cv_t<T>, bool>`).

Other techniques:

- `requires`-expressions that check nested types, expression validity and return-type
  constraints (`Container`, `Entity`, `Positionable`, `Movable`, `Resource`).
- `profileOf<T>()` is `consteval` and returns a `ConceptProfile` of booleans, a small form of
  compile-time reflection over concept satisfaction.
- `ConceptFactory<T>` constrains the class (`std::is_object_v<T>`) and each member separately
  (`std::constructible_from<T, Args...>`), so an unsupported construction fails at the call,
  not at class instantiation.
- `templates/ConceptsDemo.hpp` adds an algebraic hierarchy (`Additive`, `Multiplicative`,
  `Ring`, `Field`) and an abbreviated function template,
  `power(T base, Unsigned auto exponent)` (exponentiation by squaring, O(log n)).

**Trade-off.** Concepts check syntax, not semantics: `Ring` cannot verify associativity. The
diagnostic quality is much better than SFINAE (the unsatisfied requirement is named), which is
why `SFINAE_Examples.hpp` keeps the older techniques side by side for comparison.

## Compile-Time Programming (`ConstexprProgramming.hpp`)

The header is self-verifying: it ends with forty `static_assert`s, so if it compiles, the
computations are correct.

- **`constexpr` functions with loops and local state**: `power` (repeated squaring),
  `factorial` (saturates at `UINT64_MAX` for n > 20), `sqrtNewton`, `sinTaylor` (range
  reduction to [-pi, pi] and a 20-term series), `isPrime`, `gcd`/`lcm` (`lcm` divides before
  multiplying to avoid overflow), FNV-1a hashing, Caesar ciphers, sorting and binary search on
  `std::array`.
- **`std::is_constant_evaluated()`**: `adaptiveSqrt` uses Newton iteration during constant
  evaluation and `std::sqrt` at run time, because `std::sqrt` is not `constexpr` before C++26.
  `sqrtNewton` terminates when the iterate stops decreasing, which is guaranteed because
  Newton's method for the square root converges monotonically from above.
- **`consteval`**: `compileTimeFactorial` and `compileTimeFibonacci` are immediate functions;
  calling them with a run-time argument is ill-formed.
- **`constinit`**: the demo's `constinit static std::atomic<int> invocations` is guaranteed to be
  statically initialised (no static-initialisation-order fiasco) while remaining mutable.
- **Class-type non-type template parameters**:

  ```cpp
  template <std::size_t N>
  struct FixedString {
      std::array<char, N> chars{};
      constexpr FixedString(const char (&text)[N]) noexcept { /* copy */ }
      [[nodiscard]] constexpr std::string_view view() const noexcept { return {chars.data(), N - 1}; }
  };
  template <FixedString Name>
  struct NamedTag {
      static constexpr std::string_view name = Name.view();
      static constexpr std::uint32_t id = fnv1a(Name.view());
  };
  static_assert(NamedTag<"Fleet">::id != NamedTag<"Planet">::id);
  ```

  `FixedString` is a structural type (public members, no mutable state), as P1907 requires.
- **Table generation**: `makeTable<N>(f)` evaluates a lambda N times into a `std::array`
  (`SQUARES_TABLE`, `SINE_TABLE`); lambdas are implicitly `constexpr` since C++17.
- **Transient allocation**: `sumOfSquaresViaVector` uses a `std::vector` inside a constant
  expression (P0784); the storage must be released before evaluation ends.
- **Literal domain types**: `ConstexprPlanet` and `ConstexprFleet` have `constexpr` member
  functions (`surfaceGravity`, `density`, `combatPower`), and `SOLAR_SYSTEM` is evaluated
  entirely at compile time.

**Trade-off.** Compile-time evaluation moves cost into the build and is subject to
implementation limits on steps and recursion depth; floating-point results are compared with
`approxEqual` because constant evaluation need not match run-time rounding bit for bit.

## Lambda Expressions (`LambdaExpressions.hpp`)

- **Captures.** `makeOwningReporter` moves a `std::unique_ptr` into an init-capture, making the
  closure move-only. `Beacon::liveReporter()` captures `this` (reference semantics; must not
  outlive the object) whereas `snapshotReporter()` captures `*this` by copy (C++17), which is
  safe to outlive it.
- **Higher-order utilities**, all `constexpr` and verified by `static_assert`:
  `compose(f, g, h)(x) == f(g(h(x)))`, `pipeline(f, g, h)(x) == h(g(f(x)))`, and `curry`, which
  accepts arguments in any grouping until the callable becomes invocable:

  ```cpp
  template <typename F, typename... Bound>
  [[nodiscard]] constexpr auto curry(F f, Bound... bound) {
      return [f = std::move(f), ... bound = std::move(bound)](auto&&... args) {
          if constexpr (std::invocable<const F&, const Bound&..., decltype(args)...>) {
              return std::invoke(f, bound..., std::forward<decltype(args)>(args)...);
          } else {
              return curry(f, bound..., std::decay_t<decltype(args)>(std::forward<decltype(args)>(args))...);
          }
      };
  }
  ```

  The pack init-capture `... bound = std::move(bound)` is C++20 (P0780). Bound arguments are
  stored by value, so a curried closure never dangles.
- **`Fix`** is a fixed-point combinator: the lambda receives itself as its first parameter. It
  avoids the heap allocation, indirect call and dangling-reference hazards of a
  self-capturing `std::function`, and works in constant expressions (`factorialFix(10)`).
- **`Memoized<Arg, Result>`** caches in a `std::map` (requires `std::totally_ordered<Arg>`).
  Because `std::map` nodes are stable, `operator()` returns `const Result&` safely. It is not
  thread safe; contrast `Patterns::memoize`, which is.
- **`sizeInBits`** is a C++20 template lambda (`[]<typename T>(const T&)`).
- **`EventBus`** type-erases heterogeneous handlers in `std::function` and uses a transparent
  comparator (`std::map<std::string, std::vector<Entry>, std::less<>>`). It is synchronous and
  not thread safe, and handlers must not (un)subscribe re-entrantly.
- **`parallelSum(data, chunks)`** splits work across `std::async(std::launch::async, ...)` tasks;
  `chunks` is clamped to [1, data.size()].

## Move Semantics (`MoveSemantics.hpp`)

- **`TrackedResource`** implements the rule of five and reports every special member call into
  a non-owning `OperationCounts*` observer, which lets tests count copies and moves exactly.
  Copy assignment uses copy-and-swap (strong guarantee); move operations are `noexcept`, which
  the header checks with `static_assert(std::is_nothrow_move_constructible_v<TrackedResource>)`.
  A moved-from object is empty (`isMovedFrom()`) but valid and assignable.
- **`Spacecraft`** is move-only (copy deleted) because it uniquely owns its log through
  `std::unique_ptr`; a moved-from instance has id -1. `loadCargo(TrackedResource)` is the sink
  idiom (callers choose copy or move); `emplaceCargo(args...)` constructs in place.
- **Value categories.** `categoryOf(T&&)` classifies an argument from forwarding-reference
  deduction (`T` is `U&` for lvalues). `relayForwarded` versus `relayWithoutForward` shows that a
  named rvalue-reference parameter is an lvalue, so only `std::forward` reaches the
  `accept(std::string&&)` overload.
- **`makeResource`** returns a prvalue; C++17 guaranteed copy elision (P0135) means no copy or
  move occurs, which the tests observe through the counters.

**`MoveAwareVector<T>`** is a minimal growable array that demonstrates why `std::vector`
needs `noexcept` moves:

```cpp
void transferInto(T* dest) {
    size_type built = 0;
    try {
        for (; built < size_; ++built) {
            std::construct_at(dest + built, std::move_if_noexcept(data_[built]));
        }
    } catch (...) {
        std::destroy(dest, dest + built);
        throw;
    }
}
```

Elements are moved only if their move constructor is `noexcept` (or the type is not
copyable); otherwise they are copied, so a throwing transfer leaves the original buffer intact.
This gives `push_back`, `emplace_back` and `reserve` the strong guarantee. `emplace_back`
constructs the new element in the new buffer *before* transferring the old ones, so an argument
that aliases an existing element (`v.emplace_back(v[0])`) is read before it is moved from.
Capacity doubles (amortised O(1) append); `reallocations()` exposes the count for tests.
`T` must be nothrow-destructible (`static_assert`).

## Ranges (`RangesDemo.hpp`)

- Domain queries (`habitablePlanetNames`, `topByPopulation`, `readyFleetIds`,
  `missionIdsByUrgency`, `planetsBySystem`) are written as view pipelines and range algorithms
  with **projections**, for example sorting by `&Planet::population` instead of writing a
  comparator. `missionIdsByUrgency` uses a stable sort so ties keep input order.
- `splitWords` (`views::split`), `flatten` (`views::join`), `squaresOfOdds` (an infinite
  `views::iota` bounded by `take`) and `splitAtFirstNotBelow` (`take_while`/`drop_while`)
  exercise the remaining adaptors.
- **Laziness** is measured, not asserted: `countEvaluationsForFirst` counts how many times the
  `transform` runs when only the first k results of a filtered view are consumed.
- `sumSquaresOfEvensRanges` and `sumSquaresOfEvensLoop` compute the same value for comparison
  with a raw loop.

**Custom view.** `EveryNthView<V>` derives from `std::ranges::view_interface`, requires
`std::ranges::view<V> && std::ranges::forward_range<V>`, and iterates with
`std::ranges::advance(cur_, step_, end_)`, which clamps at the end so the iterator never steps
past it. The end is `std::default_sentinel_t`; a deduction guide wraps any range in
`views::all_t`, and `everyNth(n)` returns an adaptor object with a hidden-friend `operator|`.
The header verifies `std::ranges::forward_range` and `std::ranges::view` with `static_assert`.
Design limits: `begin()` is non-const (so a `const EveryNthView` is not a range), and the view is
forward-only even over random-access bases; both keep the implementation short compared with
`std::views::stride`.

## Structured Bindings (`StructuredBindings.hpp`)

The header exercises all three binding protocols of [dcl.struct.bind]:

1. **Arrays** (`std::array`, C arrays), e.g. `centerOfMass`.
2. **Tuple-like types**: `std::tuple`/`std::pair` results from `orbitParameters`,
   `jumpDistance` and `missionStats`, plus the user-defined `ShipRecord`, which keeps its data
   *private* and opts in through `std::tuple_size`, `std::tuple_element` and member `get<I>()`
   overloaded on `const&`, `&` and `&&`:

   ```cpp
   template <std::size_t I>
   [[nodiscard]] auto&& get() && noexcept { return std::move(get<I>()); }
   ```

   The rvalue overload lets `auto [id, name, crew] = std::move(record);` move the string out.
3. **Aggregates with public members**: `SpaceCoordinate`, `FleetStats`, `FuelRange`.

`refuelBelow` binds by reference (`auto& [..]`) to mutate elements in place; `shipsByMission`
and `busiestMission` decompose map entries and `insert`/`try_emplace` results;
`makeScaledOffset` captures structured bindings in a lambda, which is permitted since C++20
(P1091, P1381).

## Modules Emulation (`ModulesDemo.hpp`)

**Decision.** Real named modules are not used. The header explains why: they require CMake's
`FILE_SET CXX_MODULES`, a dependency-scanning generator, and compiler-specific binary module
interfaces (Clang `.pcm`, GCC `.gcm`, MSVC `.ifc`) whose maturity differs across AppleClang,
GCC 13 and MSVC. The module *structure* is therefore emulated:

| Module concept | Emulation |
| --- | --- |
| `export module CppVerseHub.SpaceGame.Core;` | namespace `SpaceGame::Core` in the header |
| exported declarations | declarations in the header (the "interface unit") |
| module-linkage (non-exported) names | anonymous namespace in `ModulesDemo.cpp` |
| implementation unit | `ModulesDemo.cpp` |
| `import` edges | `moduleGraph()` metadata (`ModuleUnit{name, imports, exports}`) |
| versioned interface | `inline namespace v1` inside `Core` |

`topologicalBuildOrder()` derives a valid compilation order with Kahn's algorithm and returns
`std::nullopt` on a cycle or unknown import, illustrating that, unlike headers, modules must be
built in dependency order. `moduleInterfaceSketch()` returns the source text the real interface
units would contain. The simulated game (`Planet`, `Starship`, `Mission`, `MissionFactory`,
`FleetFormation`, `GameUniverse`) is real, tested code organised along those boundaries.
`Core::IdGenerator` keeps per-instance state rather than a global counter.

## Generic Programming Techniques (`templates/`)

### SFINAE and the detection idiom (`SFINAE_Examples.hpp`)

Presented in historical order so the progression is visible: overloaded `test(int)`/`test(...)`
detectors (`has_size_method`, `has_begin_method`), `std::void_t` partial specialisation, and the
Library Fundamentals TS v2 detection idiom:

```cpp
template <typename Default, typename AlwaysVoid, template <typename...> class Op, typename... Args>
struct detector { using value_t = std::false_type; using type = Default; };

template <typename Default, template <typename...> class Op, typename... Args>
struct detector<Default, std::void_t<Op<Args...>>, Op, Args...> {
    using value_t = std::true_type; using type = Op<Args...>;
};
template <template <typename...> class Op, typename... Args>
using is_detected = typename detail::detector<nonesuch, void, Op, Args...>::value_t;
```

followed by `std::enable_if_t` overload selection, tag dispatch (`container_tag`,
`arithmetic_tag`, `string_tag`, `generic_tag`), and the concept-based `modern_describe` for
comparison. `nonesuch` has deleted special members so it cannot be used accidentally.

### Specialisation (`TemplateSpecialization.hpp`)

Full specialisations (`Serializer<int>`, `Serializer<bool>`, ...), partial specialisations on
type shape and on template-ids (`std::vector<T>`, `std::array<T, N>`, smart pointers, `tuple`,
`pair`, `optional`, `variant`), and decomposition of function and member-function pointer types
including `noexcept`. Two subtleties are documented in the code: `TypeInfo` reports
cv-qualification through standard traits instead of `const T` specialisations, because
`const T` and `T[N]` would both match `const int[3]` and be ambiguous; and since alias templates
cannot be specialised, `element_type` places the variation in a class template that the alias
forwards to.

### Variadic templates (`VariadicTemplates.hpp`)

Recursive pack processing next to C++17 folds (`print` vs `print_recursive`, `min_fold` and
`max_fold` via comma folds, empty-pack identities for `*`), `RecursiveTuple` built by private
recursive inheritance with index-based `get`, the `overload` idiom (`using Visitors::operator()...`)
contrasted with a *first-match* `multifunction`, and pack storage and replay with `std::apply`
(`Factory`, `Builder`, `Pipeline`). `Variadic::overload` declares the classic C++17 deduction
guide (`overload(Visitors...) -> overload<Visitors...>`), whereas
`Modern::LambdaExpressions::Overloaded` omits it and relies on C++20 class template argument
deduction for aggregates (P1021, worded in P1816); the pair shows both forms.

### Metaprogramming (`MetaProgramming.hpp`)

- **Type lists** (`type_list`, `type_at` by recursive peeling, transform and filter
  metafunctions) in the style of Alexandrescu (2001, ch. 3), alongside the same computations
  written as `constexpr` functions for comparison.
- **`Ratio<Num, Den>`** normalises at compile time (sign in the numerator, lowest terms via
  `std::gcd`); intermediate overflow is documented as unchecked.
- **Dimensional analysis.** `dimension<Mass, Length, Time>` and `quantity<Rep, Dim>` encode
  units in the type: `operator+` and `operator-` are only defined for equal dimensions, while
  `operator*` and `operator/` add and subtract exponents. Adding a velocity to a length is a
  compile error with zero run-time cost (Barton and Nackman, 1994).
- **Expression templates.** `VectorBinaryOp` and `VectorScaleOp` derive from the CRTP base
  `VectorExpression<E>`; `a + b * 2.0` builds a tree that is evaluated in one fused loop when
  assigned to an `ExprVector`, with no temporaries (Veldhuizen, 1995). `expression_storage`
  holds leaves by reference and interior nodes by value, so a full expression is safe; storing
  an expression (`auto e = x + y;`) that outlives its leaf vectors would dangle, the usual
  caveat of this technique. Assignment is element-wise, so `v = v + w` is alias-safe.
- **CRTP** for static polymorphism (`ShapeBase`, `CircleShape`) and operator mixins
  (`TotallyOrdered`), a `ConstexprMap` lookup table, and a `std::variant` state machine.

## Generic Containers and Smart Pointers (`GenericContainers.hpp`)

Teaching implementations of the standard vocabulary types; the header says to prefer the
standard ones in production.

- **`DynamicArray<T, Allocator>`** is allocator-aware through `std::allocator_traits` (and
  `static_assert`s that the allocator's `value_type` is `T` and its pointer is `T*`). Copy and
  move assignment and `swap` honour `propagate_on_container_*` and `is_always_equal`; when
  allocators neither propagate nor compare equal, move assignment falls back to element-wise
  moves. Iterators are a single template `ArrayIterator<IsConst>` modelling
  `std::contiguous_iterator` with defaulted `operator<=>`. Guarantees, as documented in the
  header: `push_back`, `emplace_back`, `reserve` and copy assignment are strong when `T`'s move
  constructor is `noexcept` (or `T` is copyable); `insert` and `erase` are basic. Growth doubles,
  saturating at `max_size()`, and `emplace_back` is safe when an argument aliases an element.
- **`UniquePtr<T, Deleter>`** stores the deleter with `[[no_unique_address]]`, so a stateless
  deleter adds no size; it has a converting move (Derived to Base) and an array partial
  specialisation.
- **`SharedPtr<T>` / `WeakPtr<T>`** share a type-erased `detail::ControlBlockBase` with atomic
  strong and weak counts. The weak count carries one extra reference while any strong reference
  exists, so the block is deleted exactly once by whichever count reaches zero last:

  ```cpp
  void add_strong() noexcept { strong_.fetch_add(1, std::memory_order_relaxed); }
  void release_strong() noexcept {
      if (strong_.fetch_sub(1, std::memory_order_acq_rel) == 1) { dispose(); release_weak(); }
  }
  [[nodiscard]] bool try_add_strong() noexcept {   // used by WeakPtr::lock()
      std::size_t count = strong_.load(std::memory_order_relaxed);
      while (count != 0) {
          if (strong_.compare_exchange_weak(count, count + 1, std::memory_order_acq_rel,
                                            std::memory_order_relaxed)) return true;
      }
      return false;
  }
  ```

  Increments are `relaxed` because a new reference can only be created from an existing one,
  which already keeps the object alive. The decrement is `acq_rel`: the release half orders
  each owner's last use of the object before the decrement, and the acquire half makes all of
  those uses happen-before `dispose()` in the thread that drops the count to zero. `lock()` is
  race-free because the CAS never resurrects a count that has reached zero. `make_shared_ptr`
  uses `InplaceControlBlock<T>`, which stores the object in a union inside the block (one
  allocation); `PointerControlBlock<U, Deleter>` serves externally allocated pointers. The
  aliasing constructor `SharedPtr(const SharedPtr<U>&, T*)` shares ownership while pointing at a
  sub-object. Copying a `SharedPtr` *object* concurrently with modifying the same object is a
  data race, exactly as for `std::shared_ptr`.
- **`Optional<T>`** stores its value in an anonymous union with an `empty_` alternative, so it
  works in constant expressions without `reinterpret_cast`. Constructors are constrained
  (`is_value_arg` rejects `Optional`, `in_place_t` and `nullopt_t`) and use conditional
  `explicit(!std::is_convertible_v<U&&, T>)`. It offers `value_or` and the C++23-style monadic
  `transform` and `and_then`; `Optional<T&>` is rejected by `static_assert`.

## Verification

Tests use Catch2. Many claims are additionally enforced by `static_assert`s inside the headers
themselves (`ConstexprProgramming.hpp`, `LambdaExpressions.hpp`, `MoveSemantics.hpp`,
`RangesDemo.hpp`, `ConceptsAdvanced.hpp`), so a successful build is already a partial
verification.

| Claim | Test file |
| --- | --- |
| Concepts accept and reject the expected types; subsumption picks the most constrained `classify`; domain concepts | `tests/modern/ConceptsTests.cpp` |
| `power`, `factorial` saturation, `sqrtNewton` against `std::sqrt`, tables, literal types | `tests/modern/ConstexprTests.cpp` |
| `compose`/`pipeline` order, `curry` groupings and by-value binding, `Fix`, `Memoized`, captures, `EventBus`, `parallelSum` | `tests/modern/LambdaTests.cpp` |
| Deep copy and counted moves of `TrackedResource`, move-only `Spacecraft`, forwarding, copy elision, `MoveAwareVector` strong guarantee | `tests/modern/MoveSemanticsTests.cpp` |
| View pipelines, projections, laziness counts, `EveryNthView` | `tests/modern/RangesTests.cpp` |
| All three binding protocols, `ShipRecord`, in-place mutation | `tests/modern/StructuredBindingsTests.cpp` |
| Core utilities, `inline namespace v1` versioning, game entities, module graph and build order | `tests/modern/ModulesTests.cpp` |
| `Modern::runDemo` runs without throwing and is deterministic apart from its `constinit` counter | `tests/modern/DemoTests.cpp` |
| Subsumption in `Templates::Concepts::classify`, `power`, constrained containers | `tests/templates/ConceptsDemoTests.cpp` |
| `enable_if` overload selection, detection idiom, tag dispatch | `tests/templates/SFINAETests.cpp` |
| `Serializer` round trips, shape and template-id specialisations | `tests/templates/TemplateSpecializationTests.cpp` |
| Fold versus recursion equivalence, `RecursiveTuple`, `overload`/`multifunction`, pipelines | `tests/templates/VariadicTemplatesTests.cpp` |
| Compile-time arithmetic agrees with run time, type lists, `Ratio`, units, expression templates | `tests/templates/MetaProgrammingTests.cpp` |
| `DynamicArray` growth, aliasing `emplace_back` during reallocation, exception guarantees; smart-pointer counts and `lock()`; `Optional` | `tests/templates/GenericContainersTests.cpp` |
| `Templates::runDemo` runs without throwing and is deterministic | `tests/templates/DemoTests.cpp` |

## References

1. ISO/IEC 14882:2020, *Programming Languages - C++*, in particular [temp.constr.order],
   [dcl.struct.bind], [expr.const], [range.view].
2. A. Sutton, *Wording Paper, C++ extensions for Concepts*, P0734R0, 2017.
3. E. Niebler, C. Carter, C. Di Bella, *The One Ranges Proposal*, P0896R4, 2018.
4. L. Dionne, R. Smith, N. Ranns, D. Vandevoorde, *More constexpr containers*, P0784R7, 2019.
5. R. Smith, A. Sutton, D. Vandevoorde, *Immediate functions*, P1073R3, 2018; E. Fiselier,
   *Adding the constinit keyword*, P1143R2, 2019.
6. J. Maurer, *Class types in non-type template parameters*, P0732R2, 2018, and
   *Inconsistencies with non-type template parameters*, P1907R1, 2019.
7. R. Smith, *Guaranteed copy elision through simplified value categories*, P0135R1, 2016.
8. H. Sutter, B. Stroustrup, G. Dos Reis, *Structured bindings*, P0144R2, 2016; N. Josuttis,
   *Extending structured bindings to be more like variable declarations*, P1091R3, 2019;
   N. Josuttis, *Reference capture of structured bindings*, P1381R1, 2019.
9. B. Revzin, *Allow pack expansion in lambda init-capture*, P0780R2, 2018.
10. M. Spertus, *Filling holes in Class Template Argument Deduction*, P1021R6, 2019, and
    T. Song, *Wording for class template argument deduction for aggregates*, P1816R0, 2019.
11. G. Dos Reis, R. Smith, *Merging Modules*, P1103R3, 2019.
12. D. Vandevoorde, N. M. Josuttis, D. Gregor, *C++ Templates: The Complete Guide*, 2nd ed.,
    Addison-Wesley, 2017.
13. A. Alexandrescu, *Modern C++ Design*, Addison-Wesley, 2001 (ch. 3 typelists).
14. T. Veldhuizen, "Expression Templates", *C++ Report* 7(5), 1995.
15. J. J. Barton, L. R. Nackman, *Scientific and Engineering C++*, Addison-Wesley, 1994.
16. D. Abrahams, "Exception-Safety in Generic Components", *Generic Programming*, LNCS 1766,
    Springer, 2000.
17. A. Williams, *C++ Concurrency in Action*, 2nd ed., Manning, 2019 (ch. 5, memory model, for
    the reference-count ordering argument).
18. cppreference.com: [Constraints and concepts](https://en.cppreference.com/w/cpp/language/constraints),
    [Ranges library](https://en.cppreference.com/w/cpp/ranges),
    [Structured binding](https://en.cppreference.com/w/cpp/language/structured_binding),
    [`std::move_if_noexcept`](https://en.cppreference.com/w/cpp/utility/move_if_noexcept),
    [Modules](https://en.cppreference.com/w/cpp/language/modules).
