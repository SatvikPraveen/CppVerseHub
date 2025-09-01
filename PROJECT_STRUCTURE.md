.
├── .gitignore
├── .vscode
│   └── settings.json
├── CMakeLists.txt
├── docs
│   ├── api_reference
│   │   └── html
│   │       ├── concurrency
│   │       │   └── index.html
│   │       ├── core
│   │       │   └── index.html
│   │       ├── index.html
│   │       ├── memory
│   │       │   └── index.html
│   │       ├── modern
│   │       │   └── index.html
│   │       ├── patterns
│   │       │   └── index.html
│   │       ├── stl
│   │       │   └── index.html
│   │       ├── templates
│   │       │   └── index.html
│   │       └── utils
│   │           └── index.html
│   ├── cheat_sheets
│   │   ├── Concurrency_CheatSheet.md
│   │   ├── DesignPatterns_CheatSheet.md
│   │   ├── Exceptions_CheatSheet.md
│   │   ├── MemoryManagement_CheatSheet.md
│   │   ├── ModernCpp_CheatSheet.md
│   │   ├── OOP_CheatSheet.md
│   │   ├── STL_CheatSheet.md
│   │   └── Templates_CheatSheet.md
│   ├── design_docs
│   │   ├── architecture_overview.md
│   │   ├── concurrency_design.md
│   │   ├── design_patterns_explained.md
│   │   └── modern_cpp_usage.md
│   ├── README.md
│   └── UML_diagrams
│       ├── class_hierarchy_uml.svg
│       ├── design_patterns_uml.svg
│       ├── mission_polymorphism_uml.svg
│       └── system_architecture.svg
├── examples
│   ├── advanced_usage
│   │   ├── CMakeLists.txt
│   │   ├── custom_missions.cpp
│   │   ├── performance_optimization.cpp
│   │   └── plugin_systems.cpp
│   ├── basic_usage
│   │   ├── CMakeLists.txt
│   │   ├── entity_creation.cpp
│   │   ├── getting_started.cpp
│   │   └── simple_simulation.cpp
│   ├── CMakeLists.txt
│   └── tutorials
│       ├── CMakeLists.txt
│       ├── cpp_concepts_walkthrough.cpp
│       ├── design_patterns_tutorial.cpp
│       └── modern_cpp_features.cpp
├── external
│   ├── benchmark
│   │   ├── benchmark_fixtures.hpp
│   │   ├── benchmark_utils.hpp
│   │   └── CMakeLists.txt
│   ├── catch2
│   │   ├── CMakeLists.txt
│   │   ├── test_fixtures.hpp
│   │   └── test_utils.hpp
│   └── json
│       ├── CMakeLists.txt
│       └── json_utils.hpp
├── LICENSE
├── project_generator_script.sh
├── PROJECT_STRUCTURE.md
├── README.md
├── scripts
│   ├── build.sh
│   ├── clean.sh
│   ├── generate_docs.sh
│   ├── generate_project.sh
│   ├── run_tests.sh
│   └── setup_dev_env.sh
├── src
│   ├── algorithms
│   │   ├── CMakeLists.txt
│   │   ├── DataStructures.cpp
│   │   ├── DataStructures.hpp
│   │   ├── GraphAlgorithms.cpp
│   │   ├── GraphAlgorithms.hpp
│   │   ├── SearchAlgorithms.cpp
│   │   ├── SearchAlgorithms.hpp
│   │   ├── SortingAlgorithms.cpp
│   │   └── SortingAlgorithms.hpp
│   ├── CMakeLists.txt
│   ├── concurrency
│   │   ├── AsyncComms.cpp
│   │   ├── AsyncComms.hpp
│   │   ├── AsyncMissions.cpp
│   │   ├── AsyncMissions.hpp
│   │   ├── Atomics.cpp
│   │   ├── Atomics.hpp
│   │   ├── CMakeLists.txt
│   │   ├── ConditionalVariables.cpp
│   │   ├── ConditionalVariables.hpp
│   │   ├── CoroutinesDemo.cpp
│   │   ├── CoroutinesDemo.hpp
│   │   ├── MutexExamples.cpp
│   │   ├── MutexExamples.hpp
│   │   ├── ThreadPool.cpp
│   │   └── ThreadPool.hpp
│   ├── core
│   │   ├── CMakeLists.txt
│   │   ├── ColonizationMission.cpp
│   │   ├── ColonizationMission.hpp
│   │   ├── CombatMission.cpp
│   │   ├── CombatMission.hpp
│   │   ├── Entity.hpp
│   │   ├── Exceptions.hpp
│   │   ├── ExplorationMission.cpp
│   │   ├── ExplorationMission.hpp
│   │   ├── Factory.cpp
│   │   ├── Factory.hpp
│   │   ├── Fleet.cpp
│   │   ├── Fleet.hpp
│   │   ├── GameEngine.cpp
│   │   ├── GameEngine.hpp
│   │   ├── Mission.cpp
│   │   ├── Mission.hpp
│   │   ├── Planet.cpp
│   │   ├── Planet.hpp
│   │   ├── ResourceManager.cpp
│   │   └── ResourceManager.hpp
│   ├── data
│   │   ├── config
│   │   │   ├── app_config.json
│   │   │   ├── fleet_templates.json
│   │   │   └── planets.json
│   │   ├── input
│   │   │   ├── fleet_data.csv
│   │   │   ├── missions.csv
│   │   │   └── resources.csv
│   │   └── logs
│   │       ├── errors.log
│   │       ├── performance.log
│   │       └── simulation.log
│   ├── main.cpp
│   ├── memory
│   │   ├── CMakeLists.txt
│   │   ├── CustomAllocators.cpp
│   │   ├── CustomAllocators.hpp
│   │   ├── MemoryPools.cpp
│   │   ├── MemoryPools.hpp
│   │   ├── RAII_Examples.cpp
│   │   ├── RAII_Examples.hpp
│   │   ├── SmartPointers.cpp
│   │   └── SmartPointers.hpp
│   ├── modern
│   │   ├── CMakeLists.txt
│   │   ├── ConceptsAdvanced.hpp
│   │   ├── ContexprProgramming.hpp
│   │   ├── LambdaExpressions.cpp
│   │   ├── LambdaExpressions.hpp
│   │   ├── ModulesDemo.cpp
│   │   ├── ModulesDemo.hpp
│   │   ├── MoveSemantics.cpp
│   │   ├── MoveSemantics.hpp
│   │   ├── RangesDemo.cpp
│   │   ├── RangesDemo.hpp
│   │   └── StructuredBindings.hpp
│   ├── patterns
│   │   ├── Adapter.cpp
│   │   ├── Adapter.hpp
│   │   ├── Builder.cpp
│   │   ├── Builder.hpp
│   │   ├── CMakeLists.txt
│   │   ├── Command.cpp
│   │   ├── Command.hpp
│   │   ├── Decorator.cpp
│   │   ├── Decorator.hpp
│   │   ├── Observer.cpp
│   │   ├── Observer.hpp
│   │   ├── Singleton.hpp
│   │   ├── State.cpp
│   │   ├── State.hpp
│   │   ├── Strategy.cpp
│   │   └── Strategy.hpp
│   ├── stl_showcase
│   │   ├── Algorithms.cpp
│   │   ├── Algorithms.hpp
│   │   ├── CMakeLists.txt
│   │   ├── Containers.cpp
│   │   ├── Containers.hpp
│   │   ├── Functors.cpp
│   │   ├── Functors.hpp
│   │   ├── Iterators.cpp
│   │   ├── Iterators.hpp
│   │   ├── STLUtilities.cpp
│   │   └── STLUtilities.hpp
│   ├── templates
│   │   ├── CMakeLists.txt
│   │   ├── ConceptsDemo.hpp
│   │   ├── GenericContainers.hpp
│   │   ├── MetaProgramming.hpp
│   │   ├── SFINAE_Examples.hpp
│   │   ├── TemplateSpecialization.hpp
│   │   └── VariadicTemplates.hpp
│   └── utils
│       ├── CMakeLists.txt
│       ├── ConfigManager.cpp
│       ├── ConfigManager.hpp
│       ├── FileParser.cpp
│       ├── FileParser.hpp
│       ├── Logger.cpp
│       ├── Logger.hpp
│       ├── MathUtils.cpp
│       ├── MathUtils.hpp
│       ├── StringUtils.cpp
│       ├── StringUtils.hpp
│       ├── TimeUtils.cpp
│       └── TimeUtils.hpp
├── tests
│   ├── benchmark_tests
│   │   ├── AlgorithmBenchmarks.cpp
│   │   ├── ConcurrencyBenchmarks.cpp
│   │   └── ContainerBenchmarks.cpp
│   ├── CMakeLists.txt
│   ├── integration_tests
│   │   ├── ConcurrencyIntegrationTests.cpp
│   │   ├── FileIOIntegrationTests.cpp
│   │   └── SimulationIntegrationTests.cpp
│   ├── test_main.cpp
│   └── unit_tests
│       ├── concurrency_tests
│       │   ├── AsyncTests.cpp
│       │   ├── SynchronizationTests.cpp
│       │   └── ThreadPoolTests.cpp
│       ├── core_tests
│       │   ├── EntityTests.cpp
│       │   ├── FactoryTests.cpp
│       │   ├── MissionTests.cpp
│       │   └── ResourceManagerTests.cpp
│       ├── memory_tests
│       │   ├── AllocatorTests.cpp
│       │   ├── RAIITests.cpp
│       │   └── SmartPointerTest.cpp
│       ├── pattern_tests
│       │   ├── DecoratorTests.cpp
│       │   ├── ObserverTests.cpp
│       │   └── StrategyTests.cpp
│       ├── stl_tests
│       │   ├── AlgorithmTests.cpp
│       │   ├── ContainerTests.cpp
│       │   └── IteratorTests.cpp
│       └── template_tests
│           ├── ConceptTests.cpp
│           ├── GenericContainerTests.cpp
│           └── MetaProgrammingTests.cpp
└── tools
    ├── clang-format
    │   └── .clang-format
    ├── doxygen
    │   ├── custom.css
    │   └── Doxyfile
    └── static_analysis
        ├── .clang-tidy
        └── run_analysis.sh

53 directories, 216 files
