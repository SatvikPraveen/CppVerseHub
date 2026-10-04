cppversehub_add_test(
    NAME patterns_tests
    SOURCES
        AdapterTests.cpp
        BuilderTests.cpp
        CommandTests.cpp
        DecoratorTests.cpp
        DemoTests.cpp
        ObserverTests.cpp
        SingletonTests.cpp
        StateTests.cpp
        StrategyTests.cpp
    DEPENDS CppVerseHub::Patterns
    LABELS patterns
)
