cppversehub_add_test(
    NAME utils_tests
    SOURCES
        ConfigManagerTests.cpp
        DemoTests.cpp
        FileParserTests.cpp
        LoggerTests.cpp
        MathUtilsTests.cpp
        StringUtilsTests.cpp
        TimeUtilsTests.cpp
    DEPENDS CppVerseHub::Utils
    LABELS utils
)
