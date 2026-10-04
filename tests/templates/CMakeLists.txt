# Tests for the header-only templates module (Catch2 runtime tests + static_assert compile-time tests).
cppversehub_add_test(
    NAME    templates_tests
    SOURCES ConceptsDemoTests.cpp
            GenericContainersTests.cpp
            MetaProgrammingTests.cpp
            SFINAETests.cpp
            TemplateSpecializationTests.cpp
            VariadicTemplatesTests.cpp
            DemoTests.cpp
    DEPENDS CppVerseHub::Templates
    LABELS  templates
)
