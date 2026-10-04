# core module: space-fleet simulation domain model
cppversehub_add_module(
    NAME  core
    ALIAS Core
    SOURCES
        ColonizationMission.cpp
        CombatMission.cpp
        Demo.cpp
        Entity.cpp
        EventSystem.cpp
        ExplorationMission.cpp
        Factory.cpp
        Fleet.cpp
        Galaxy.cpp
        Mission.cpp
        Planet.cpp
        ResourceManager.cpp
        Resources.cpp
        Scenario.cpp
        Ship.cpp
        SimulationEngine.cpp
    DEPENDS
        nlohmann_json::nlohmann_json
)
