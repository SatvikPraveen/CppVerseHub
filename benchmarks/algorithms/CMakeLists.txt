# Micro-benchmarks for the algorithms module (Google Benchmark).
cppversehub_add_benchmark(
    NAME    algorithms_benchmarks
    SOURCES AlgorithmBenchmarks.cpp
    DEPENDS CppVerseHub::Algorithms
)
