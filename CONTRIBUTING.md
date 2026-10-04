# Contributing to CppVerseHub

Thank you for considering a contribution. This document describes how to build, test and submit changes.

## Prerequisites

- A C++20 compiler: GCC 13+, Clang 17+, Apple Clang 16+ or MSVC 19.38+ (Visual Studio 2022 17.8+)
- CMake 3.25+ and Ninja
- Optional: clang-format 18, clang-tidy 18, cppcheck, Doxygen + Graphviz, gcovr

## Build and test

```bash
cmake --workflow --preset default      # configure, build and run all tests (Release)
cmake --workflow --preset asan         # AddressSanitizer + UndefinedBehaviorSanitizer
cmake --workflow --preset tsan         # ThreadSanitizer
```

Run one module's tests:

```bash
ctest --preset default -L concurrency --output-on-failure
```

## Repository layout

| Path | Purpose |
|------|---------|
| `src/<module>/` | One library per module, namespace `CppVerseHub::<Module>` |
| `tests/<module>/` | Catch2 v3 tests for that module |
| `benchmarks/<module>/` | Google Benchmark micro-benchmarks |
| `examples/` | Self-contained example programs, each also run as a smoke test |
| `cmake/` | Build helpers (warnings, sanitizers, coverage, dependencies) |
| `docs/` | Design documents, cheat sheets and Doxygen configuration |

## Adding code

1. Put library code in the appropriate module and include headers as `"<module>/<Header>.hpp"`.
2. Every public class and function needs a Doxygen comment.
3. Library code must not print. Demonstrations take a `std::ostream&`.
4. Add tests in `tests/<module>/` and, for performance-sensitive code, a benchmark in `benchmarks/<module>/`.
5. The build must stay warning-free: `cmake --preset strict && cmake --build --preset strict`.
6. Format with `clang-format -i` using the repository `.clang-format`.

## Commit messages

Use [Conventional Commits](https://www.conventionalcommits.org/): `feat(concurrency): add work-stealing pool`,
`fix(memory): correct arena alignment`, `docs: ...`, `test: ...`, `build: ...`, `ci: ...`.

## Pull requests

- Keep each pull request focused on one change.
- CI must pass on all platforms, including sanitizer jobs.
- Describe the motivation, the design and how you tested it.

By contributing you agree that your contributions are licensed under the MIT License.
