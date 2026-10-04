# modern module: C++20 language features (concepts, constexpr, lambdas, move semantics, ranges,
# structured bindings, emulated modules). Self-contained: no dependency on other project modules.
cppversehub_add_module(
    NAME  modern
    ALIAS Modern
    SOURCES
        Demo.cpp
        LambdaExpressions.cpp
        ModulesDemo.cpp
        MoveSemantics.cpp
        RangesDemo.cpp
)
