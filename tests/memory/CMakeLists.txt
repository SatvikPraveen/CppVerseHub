cppversehub_add_test(
    NAME memory_tests
    SOURCES
    CustomAllocatorsTests.cpp
    DemoTests.cpp
    MemoryPoolsTests.cpp
    RAIITests.cpp
    SmartPointersTests.cpp
    DEPENDS CppVerseHub::Memory
    LABELS memory
)
