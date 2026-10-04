# Benchmarking Methodology

Each module ships a Google Benchmark executable under `benchmarks/<module>/`. This document
explains how to run them and how to read the results.

## Running

```bash
cmake --preset default
cmake --build --preset default
cmake --build --preset default --target run_benchmarks   # JSON reports in build/default/benchmark-results/
```

Run one suite directly for more control:

```bash
./build/default/bin/benchmarks/concurrency_benchmarks \
    --benchmark_filter='SpscRing|BoundedQueue' \
    --benchmark_repetitions=10 --benchmark_report_aggregates_only=true
```

## Rules for trustworthy numbers

- **Use a Release build.** Debug and sanitizer builds measure the instrumentation.
- **Repeat and aggregate.** Report the median of at least ten repetitions and the coefficient of variation.
- **Control the machine.** Close other workloads, keep the machine on mains power and, on Linux,
  pin the CPU governor to `performance` and the process to one core set with `taskset`.
- **Defeat the optimiser deliberately.** Benchmarks use `benchmark::DoNotOptimize` and
  `benchmark::ClobberMemory` so the measured work is not removed as dead code.
- **Compare like with like.** Every custom component is benchmarked next to its standard-library
  counterpart in the same binary, with the same input and seed.
- **Fit complexity, not single points.** Algorithm benchmarks sweep input sizes with `Range` and
  `RangeMultiplier` and call `Complexity()`, so the report includes the fitted asymptotic class.

## Interpreting contended benchmarks

Multi-threaded cases use `->Threads(n)->UseRealTime()`. Wall-clock time is the meaningful metric
there, since CPU time sums across threads. Results depend strongly on core count, cache topology
and the operating-system scheduler, so contended numbers should be compared only within one machine.

A lock-free structure is not automatically faster than a mutex. The bundled
`LockFreeStack` is slower than the mutex-protected stack under four-thread contention on Apple
silicon, because every operation contends on one cache line and pays for deferred reclamation.
The benchmark is kept because that negative result is itself instructive.
