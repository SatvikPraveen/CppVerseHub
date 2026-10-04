<div align="center">

# CppVerseHub

**A tested, benchmarked reference implementation of modern C++20 systems-programming techniques**

[![CI](https://github.com/SatvikPraveen/CppVerseHub/actions/workflows/ci.yml/badge.svg)](https://github.com/SatvikPraveen/CppVerseHub/actions/workflows/ci.yml)
[![Static Analysis](https://github.com/SatvikPraveen/CppVerseHub/actions/workflows/static-analysis.yml/badge.svg)](https://github.com/SatvikPraveen/CppVerseHub/actions/workflows/static-analysis.yml)
[![Docs](https://github.com/SatvikPraveen/CppVerseHub/actions/workflows/docs.yml/badge.svg)](https://satvikpraveen.github.io/CppVerseHub/)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-00599C.svg?logo=cplusplus)](https://en.cppreference.com/w/cpp/20)
[![CMake](https://img.shields.io/badge/CMake-3.25%2B-064F8C.svg?logo=cmake)](https://cmake.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![Release](https://img.shields.io/badge/release-v2.0.0-blue.svg)](CHANGELOG.md)

[GitHub repository](https://github.com/SatvikPraveen/CppVerseHub) ·
[API reference](https://satvikpraveen.github.io/CppVerseHub/) ·
[Overview](#overview) ·
[Quick start](#quick-start) ·
[Modules](#modules) ·
[Determinism](#deterministic-simulation) ·
[Verification](#verification) ·
[Benchmarks](#benchmarks) ·
[Documentation](#documentation) ·
[Citing](#citing)

</div>

---

## Overview

CppVerseHub implements the core techniques of modern C++ as nine independent libraries.
They cover templates and concepts, design patterns, the standard library, memory management,
concurrency, C++20 language features, classical algorithms and general utilities.
A deterministic space-fleet simulation ties them together as a shared domain.

The project is built to be read, reused and checked:

- **Every technique is library code.** Each component has a documented interface, stated complexity and explicit thread-safety and exception-safety guarantees.
- **Every claim is checked.** 834 Catch2 test cases, more than 500 `static_assert` checks and two example smoke tests run in CI on GCC, Clang, Apple Clang and MSVC.
- **Concurrency and memory code is sanitizer-clean.** Tests run under AddressSanitizer, UndefinedBehaviorSanitizer and ThreadSanitizer.
- **Performance claims are measured.** Nine Google Benchmark suites compare each custom component with its standard-library counterpart.
- **The simulation is reproducible.** A seed and a step count fully determine the final state, which is verifiable through a 64-bit state digest.

## Quick start

**Requirements:** a C++20 compiler (GCC 13+, Clang 18+, Apple Clang 16+ or MSVC 2022 17.8+), CMake 3.25+ and Ninja.
Catch2, Google Benchmark and nlohmann/json are found on the system or fetched automatically.

```bash
git clone https://github.com/SatvikPraveen/CppVerseHub.git
cd CppVerseHub

cmake --workflow --preset default     # configure, build and run the full test suite
```

Other presets:

| Preset | Purpose |
|--------|---------|
| `default` | Release build with tests, benchmarks and examples |
| `debug` | Debug build |
| `asan` | AddressSanitizer and UndefinedBehaviorSanitizer |
| `tsan` | ThreadSanitizer |
| `coverage` | gcov/llvm-cov instrumentation |
| `strict` | Release with warnings treated as errors |
| `tidy` | clang-tidy run as part of the build |

### Command-line interface

```bash
./build/default/bin/cppversehub list                 # list the module demonstrations
./build/default/bin/cppversehub demo concurrency     # run one module's demonstration
./build/default/bin/cppversehub demo all             # run all of them

# Reproducible simulation with a checkpoint
./build/default/bin/cppversehub simulate --seed 42 --planets 8 --fleets 4 --steps 1500 --save run.json
./build/default/bin/cppversehub replay run.json --steps 500
```

`simulate` ends with a summary like this:

```text
Simulated time ...... 150 s (1500 ticks)
Missions started .... 4
Missions completed .. 4
Missions failed ..... 0
State digest ........ 0x6cd377aeae97ad0c
```

### Using the library from another project

```cmake
find_package(CppVerseHub 2.0 REQUIRED)
target_link_libraries(my_app PRIVATE CppVerseHub::Concurrency CppVerseHub::Memory)
```

Each module is exported as its own target, and `CppVerseHub::CppVerseHub` links all of them.

## Modules

| Module | Highlights | Tests |
|--------|-----------|------:|
| [`core`](src/core) | Fixed-timestep `SimulationEngine` driven by a seeded RNG; planets, fleets and three mission types; thread-safe `ResourceManager` with conservation invariants; typed `EventBus` with RAII subscriptions; JSON scenario save and load | 74 |
| [`templates`](src/templates) | Concepts, SFINAE detection idioms, variadic folds, unit-checked `quantity` arithmetic, expression templates, `constexpr` map, allocator-aware `DynamicArray` with the strong exception guarantee | 81 |
| [`patterns`](src/patterns) | Observer with weak references and scoped connections; Command with bounded undo and redo; thread-safe CRTP Singleton; State as both class hierarchy and `std::variant` machine; Strategy in runtime, policy and function forms; Builder checked at compile time | 91 |
| [`stl_showcase`](src/stl_showcase) | Custom contiguous and forward iterators verified against the C++20 iterator concepts; a filtering `std::ranges::view`; LRU cache; container, algorithm and functor studies | 85 |
| [`memory`](src/memory) | Standard-conforming tracking allocator; `std::pmr` monotonic arena; fixed-size, thread-safe and small-object pools; stack allocator; RAII handles and scope guards; smart-pointer idioms | 70 |
| [`concurrency`](src/concurrency) | Thread pools, including work-stealing and priority variants; bounded multi-producer/multi-consumer queue; lock-free SPSC ring buffer and stack; latch, barrier and semaphore; cancellable futures; actors; C++20 coroutine `Generator` and `Task` | 83 |
| [`modern`](src/modern) | Concept-based overload ranking, `consteval`, strings as template parameters, compile-time tables, move semantics with `move_if_noexcept`, custom range adaptors, structured bindings for user types | 113 |
| [`algorithms`](src/algorithms) | 17 concept-constrained sorts with projections; KMP, Boyer-Moore-Horspool, Rabin-Karp, Z and Aho-Corasick search; suffix array; KD-tree; Dijkstra, A*, Bellman-Ford, Floyd-Warshall, Tarjan SCC, Kruskal, Prim, Edmonds-Karp and Held-Karp; skip list, Bloom filter, trie, union-find | 125 |
| [`utils`](src/utils) | Thread-safe logger with pluggable sinks; configuration manager; JSON, CSV and XML parsers; vectors, matrices, Perlin noise and an N-body integrator; string and time utilities | 110 |

## Deterministic simulation

Each step of `SimulationEngine` runs in a fixed order: start pending missions, update missions, update entities, advance the economy, remove destroyed entities.
All randomness comes from one seeded `std::mt19937_64` with project-defined distributions.
This matters because the standard library distributions differ between libstdc++, libc++ and MSVC.
Entities and missions live in ordered maps keyed by strong identifiers, so iteration order never depends on hashing or memory addresses.

These choices make three properties testable:

1. The same seed and step count always produce the same state digest.
2. Checkpointing at step *k*, restoring and running to step *n* matches an uninterrupted run to *n*.
3. Saving, loading and saving again produces byte-identical JSON.

The [`deterministic_replay`](examples/deterministic_replay.cpp) example checks the first two at run time.
The core test suite checks all three.
See the [architecture overview](docs/design_docs/architecture_overview.md) for the full design.

## Verification

| Layer | What runs | Where |
|-------|-----------|-------|
| Unit and property tests | 834 Catch2 test cases, including sorts checked against `std::sort` on seeded random inputs and graph algorithms checked against brute force | `tests/<module>/` |
| Compile-time checks | More than 500 `static_assert` checks on concepts, traits and `constexpr` results | headers and tests |
| Sanitizers | ASan with UBSan, and TSan, over the full test suite | `asan` and `tsan` presets |
| Static analysis | clang-tidy and cppcheck with zero findings (any new finding fails CI), CodeQL, and an enforced clang-format check | `.github/workflows/static-analysis.yml` |
| Portability | GCC 13 and 14, Clang 18, Apple Clang 16 and MSVC 2022 | `.github/workflows/ci.yml` |
| Coverage | gcovr report uploaded as a CI artifact | `coverage` preset |

Every push to `main` builds and tests the project on these toolchains, with warnings treated as errors on all of them:

| Platform | Compiler | Standard library | Build |
|----------|----------|------------------|-------|
| Ubuntu 24.04 | GCC 13 | libstdc++ | Release |
| Ubuntu 24.04 | GCC 14 | libstdc++ | Debug |
| Ubuntu 24.04 | Clang 18 | libstdc++ | Release, ASan + UBSan, TSan |
| macOS 15 | Apple Clang 16 | libc++ | Release |
| Windows Server 2022 | MSVC 19.4x | MSVC STL | Release |

Run one module's tests:

```bash
ctest --preset default -L concurrency --output-on-failure
```

## Benchmarks

```bash
cmake --build --preset default --target run_benchmarks   # JSON results in build/default/benchmark-results/
```

Sample results from a Release build on an Apple M5 are shown below.
They illustrate the comparisons the suites make; they are not portable performance claims.
See the [benchmarking methodology](docs/design_docs/benchmarking_methodology.md) before drawing conclusions.

| Comparison | Baseline | Variant | Result |
|------------|----------|---------|--------|
| `std::list` push of 1,000 nodes | default allocator: 11.8 µs | `PoolAllocator`: 3.9 µs | 3.0x faster |
| Vector expression `a + b * c - 2a`, 64 Ki elements | eager temporaries: 56.9 µs | expression templates: 14.1 µs | 4.0x faster |
| SPSC transfer of 20 K items | mutex-based `BoundedQueue`: 29.6 M items/s | lock-free `SpscRingBuffer`: 96.5 M items/s | 3.3x throughput |
| Stack push and pop, 4 threads | mutex stack: 111 ns | `LockFreeStack`: 1,047 ns | 9.4x slower |

The last row is a negative result and is reported on purpose.
Lock-free is not automatically faster: under contention, every operation competes for one cache line and pays for deferred reclamation.

## Repository layout

```text
src/<module>/          library code, included as "<module>/<Header>.hpp"
src/main.cpp           cppversehub command-line interface
tests/<module>/        Catch2 v3 test suites
benchmarks/<module>/   Google Benchmark suites
examples/              standalone programs, each also run as a CTest smoke test
cmake/                 warning, sanitizer, coverage, analysis and dependency helpers
docs/                  design documents, cheat sheets and Doxygen configuration
.github/workflows/     CI, static analysis and documentation pipelines
```

## Documentation

- [Architecture overview](docs/design_docs/architecture_overview.md)
- [Concurrency design](docs/design_docs/concurrency_design.md)
- [Design patterns explained](docs/design_docs/design_patterns_explained.md)
- [Modern C++ usage](docs/design_docs/modern_cpp_usage.md)
- [Benchmarking methodology](docs/design_docs/benchmarking_methodology.md)
- [Cheat sheets](docs/cheat_sheets)
- [API reference](https://satvikpraveen.github.io/CppVerseHub/), generated with Doxygen and published on every push to `main`. To build it locally:

```bash
cmake -S . -B build -DCPPVERSEHUB_BUILD_DOCS=ON
cmake --build build --target docs      # open build/docs/html/index.html
```

## Citing

If CppVerseHub is useful in your research or teaching, please cite it.
GitHub's "Cite this repository" button reads [`CITATION.cff`](CITATION.cff).

```bibtex
@software{praveen_cppversehub_2026,
  author  = {Praveen, Satvik},
  title   = {{CppVerseHub: A Modern C++20 Reference Implementation of Systems Programming Techniques}},
  year    = {2026},
  version = {2.0.0},
  url     = {https://github.com/SatvikPraveen/CppVerseHub},
  license = {MIT}
}
```

## Project status

The current release is v2.0.0. See the [changelog](CHANGELOG.md) for what changed from the original scaffold.

## Contributing

Contributions are welcome. Please read [CONTRIBUTING.md](CONTRIBUTING.md) and the [code of conduct](CODE_OF_CONDUCT.md).
Report security issues as described in [SECURITY.md](SECURITY.md).

## License

Released under the [MIT License](LICENSE). Copyright © 2025-2026 Satvik Praveen.
