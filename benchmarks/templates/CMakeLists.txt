# Micro-benchmarks for the header-only templates module.
cppversehub_add_benchmark(
    NAME    templates_benchmarks
    SOURCES TemplatesBenchmarks.cpp
    DEPENDS CppVerseHub::Templates
)
