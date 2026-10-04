# Tests for the algorithms module (Catch2 v3).
cppversehub_add_test(
    NAME    algorithms_tests
    SOURCES
        DataStructureTests.cpp
        GraphTests.cpp
        SearchTests.cpp
        SortingTests.cpp
    DEPENDS CppVerseHub::Algorithms
    LABELS  algorithms
)
