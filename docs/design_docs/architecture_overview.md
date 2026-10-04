# Architecture Overview

This document describes how CppVerseHub is organised, how its modules depend on one another,
and which engineering rules every module follows.

## Goals

CppVerseHub is a reference implementation of modern C++20 techniques. Three goals shape the design.

1. **Each technique is real code, not prose.** Every concept is implemented as a reusable library
   component with tests and, where performance is the point, a benchmark.
2. **Claims are checked.** Correctness properties are asserted in tests, compile-time properties in
   `static_assert`, and performance claims in Google Benchmark suites whose results are reproducible.
3. **Modules are independent.** A reader can study, build and test one module without the others.

## Module graph

```text
                       +------------------+
                       |  cppversehub CLI |   src/main.cpp
                       +--------+---------+
                                |
                     CppVerseHub::CppVerseHub   (umbrella INTERFACE target)
                                |
   +-------+---------+----------+---------+--------+--------+------------+-------+
   |       |         |          |         |        |        |            |       |
  core  templates  patterns    stl     memory  concurrency modern  algorithms  utils
   |
 nlohmann_json
```

Every module is a leaf: no module links another project module. This was a deliberate choice.
It keeps compile times short, lets each module be reviewed in isolation, and means a defect in one
area cannot break the build of another. Domain vocabulary that several modules need, such as a
planet or a fleet, is defined locally and kept small rather than shared through a common base.

| Module | Namespace | Library target | Kind |
|--------|-----------|----------------|------|
| `core` | `CppVerseHub::Core` | `CppVerseHub::Core` | static |
| `templates` | `CppVerseHub::Templates` | `CppVerseHub::Templates` | header-only |
| `patterns` | `CppVerseHub::Patterns` | `CppVerseHub::Patterns` | static |
| `stl_showcase` | `CppVerseHub::STL` | `CppVerseHub::STL` | static |
| `memory` | `CppVerseHub::Memory` | `CppVerseHub::Memory` | static |
| `concurrency` | `CppVerseHub::Concurrency` | `CppVerseHub::Concurrency` | static |
| `modern` | `CppVerseHub::Modern` | `CppVerseHub::Modern` | static |
| `algorithms` | `CppVerseHub::Algorithms` | `CppVerseHub::Algorithms` | static |
| `utils` | `CppVerseHub::Utils` | `CppVerseHub::Utils` | static |

## Repository layout

```text
src/<module>/          library sources; headers are included as "<module>/<Header>.hpp"
src/<module>/Demo.hpp  runDemo(std::ostream&) - end-to-end showcase used by the CLI
tests/<module>/        Catch2 v3 test suite, one executable per module
benchmarks/<module>/   Google Benchmark suite, one executable per module
examples/              standalone programs, each also registered as a CTest smoke test
cmake/                 warnings, sanitizers, coverage, static analysis, dependency helpers
docs/                  design documents, cheat sheets, Doxygen configuration
```

## Engineering rules

These rules apply to every module and are enforced by review, by the compiler, or by CI.

- **Warning-free under a strict set.** `-Wall -Wextra -Wpedantic -Wshadow -Wold-style-cast
  -Wnon-virtual-dtor -Woverloaded-virtual -Wnull-dereference -Wdouble-promotion` and more, with
  `-Werror` in CI. GCC builds add `-Wduplicated-cond -Wduplicated-branches -Wlogical-op`.
- **Quiet libraries.** Library code never writes to standard output. Showcase functions take a
  `std::ostream&`, so tests capture their output and the CLI decides where it goes.
- **RAII and value semantics.** No owning raw pointers. Special members follow the rule of zero
  or the rule of five. Resources are released in destructors and nowhere else.
- **Stated guarantees.** Public functions document complexity and, where relevant, their
  exception-safety level and thread-safety contract.
- **Determinism in tests.** Random inputs come from seeded generators. Concurrency tests order
  events with latches and promises rather than sleeps, so they are stable under ThreadSanitizer.

## The simulation core

`core` is the domain model that ties the project together: planets, fleets, missions, resources
and a simulation engine. Its central property is determinism.

`SimulationEngine::step()` performs one fixed time step in a fixed order:

1. start missions that are pending;
2. update every mission, in mission-id order;
3. update every entity, in entity-id order;
4. advance the resource economy;
5. remove destroyed entities;
6. publish a `StepCompleted` event.

All randomness flows through one `DeterministicRng`, a seeded `std::mt19937_64` with its own
distribution functions. The standard distributions are not used because the standard leaves their
output implementation-defined, which would make results differ between libstdc++, libc++ and the
MSVC library. Containers are ordered maps keyed by strong identifiers, so iteration order never
depends on hashing or allocation addresses.

`stateDigest()` serialises the full engine state to JSON and hashes it with 64-bit FNV-1a.
Equal digests therefore mean equal states, which gives three testable properties:

- the same seed and step count produce the same digest;
- saving at step *k*, reloading and running to step *n* gives the same digest as running to *n*;
- save, load, save produces byte-identical JSON.

The `deterministic_replay` example and the core test suite check all three.
Results are bit-identical for one binary. Across compilers or CPUs, floating-point contraction
into fused multiply-add instructions can change rounding. Build with `-ffp-contract=off` when
cross-platform identity is required.

## Build system

The build uses target-based CMake. `cmake/ModuleHelpers.cmake` provides four functions that keep
every target consistent: `cppversehub_add_module`, `cppversehub_add_executable`,
`cppversehub_add_test` and `cppversehub_add_benchmark`. Compiler flags live on the
`cppversehub_options` interface target, which is linked privately and never exported, so
consumers of the installed package do not inherit project warnings or sanitizer flags.

Dependencies are resolved with `find_package` first and `FetchContent` as a fallback, so the same
build works offline with system packages and in a clean CI runner.

Presets in `CMakePresets.json` define the supported configurations: `default`, `debug`, `asan`,
`tsan`, `coverage`, `strict` and `tidy`. The CI workflow runs the build matrix and the sanitizer,
coverage, static-analysis and documentation jobs from the same presets.
