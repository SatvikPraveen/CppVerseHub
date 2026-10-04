cppversehub_add_test(
    NAME    concurrency_tests
    SOURCES
        AsyncTests.cpp
        AtomicsTests.cpp
        CoroutineTests.cpp
        SynchronizationTests.cpp
        ThreadPoolTests.cpp
    DEPENDS CppVerseHub::Concurrency
    LABELS  concurrency
)
