# CppVerseHub Documentation

## Design documents

| Document | Covers |
|----------|--------|
| [Architecture overview](design_docs/architecture_overview.md) | Module graph, engineering rules, deterministic simulation core, build system |
| [Concurrency design](design_docs/concurrency_design.md) | Thread pools, queues, lock-free structures, memory ordering, coroutines |
| [Design patterns explained](design_docs/design_patterns_explained.md) | Pattern implementations and the trade-offs behind them |
| [Modern C++ usage](design_docs/modern_cpp_usage.md) | Concepts, constexpr, ranges, move semantics, metaprogramming |
| [Benchmarking methodology](design_docs/benchmarking_methodology.md) | How to run benchmarks and read the results |

## Cheat sheets

Quick references for the topics the modules cover:
[Modern C++](cheat_sheets/ModernCpp_CheatSheet.md),
[Templates](cheat_sheets/Templates_CheatSheet.md),
[STL](cheat_sheets/STL_CheatSheet.md),
[Concurrency](cheat_sheets/Concurrency_CheatSheet.md),
[Memory management](cheat_sheets/MemoryManagement_CheatSheet.md),
[Design patterns](cheat_sheets/DesignPatterns_CheatSheet.md),
[OOP](cheat_sheets/OOP_CheatSheet.md) and
[Exceptions](cheat_sheets/Exceptions_CheatSheet.md).

## API reference

The API reference is generated from Doxygen comments in the headers:

```bash
cmake -S . -B build -DCPPVERSEHUB_BUILD_DOCS=ON
cmake --build build --target docs      # output in build/docs/html/index.html
```

The CI publishes the same output to GitHub Pages on every push to `main`.
