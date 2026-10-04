cppversehub_add_test(
    NAME    core_tests
    SOURCES
        DemoTests.cpp
        EntityTests.cpp
        EventSystemTests.cpp
        FactoryGalaxyTests.cpp
        MissionTests.cpp
        RandomTests.cpp
        ResourceManagerTests.cpp
        ScenarioTests.cpp
        SimulationEngineTests.cpp
        Vector3DTests.cpp
    DEPENDS CppVerseHub::Core
    LABELS  core
)
