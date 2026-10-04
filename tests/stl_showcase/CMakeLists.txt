cppversehub_add_test(
    NAME    stl_showcase_tests
    SOURCES
    AlgorithmsTests.cpp
    ContainersTests.cpp
    DemoTests.cpp
    FunctorsTests.cpp
    IteratorsTests.cpp
    STLUtilitiesTests.cpp
    DEPENDS CppVerseHub::STL
    LABELS  stl_showcase
)
