cppversehub_add_test(
    NAME    modern_tests
    SOURCES
        ConceptsTests.cpp
        ConstexprTests.cpp
        LambdaTests.cpp
        MoveSemanticsTests.cpp
        RangesTests.cpp
        StructuredBindingsTests.cpp
        ModulesTests.cpp
        DemoTests.cpp
    DEPENDS CppVerseHub::Modern
    LABELS  modern
)
