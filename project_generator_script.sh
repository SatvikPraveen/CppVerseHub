#!/bin/bash
# CppVerseHub Project Structure Generator
# This script creates the complete directory structure and essential files for the CppVerseHub project

set -e  # Exit on any error

PROJECT_NAME="CppVerseHub"
PROJECT_ROOT="$(pwd)/$PROJECT_NAME"

echo "🚀 Creating CppVerseHub Project Structure..."
echo "Project will be created at: $PROJECT_ROOT"
echo ""

# Create main project directory
mkdir -p "$PROJECT_ROOT"
cd "$PROJECT_ROOT"

# Create main directory structure
echo "📁 Creating main directories..."
mkdir -p docs/{cheat_sheets,UML_diagrams,design_docs,api_reference/html}
mkdir -p src/{core,templates,patterns,stl_showcase,memory,concurrency,modern,algorithms,utils}
mkdir -p src/data/{config,input,logs}
mkdir -p tests/{unit_tests/{core_tests,stl_tests,template_tests,concurrency_tests,pattern_tests,memory_tests},integration_tests,benchmark_tests}
mkdir -p examples/{basic_usage,advanced_usage,tutorials}
mkdir -p scripts
mkdir -p build/{debug,release,tests}
mkdir -p external/{json,catch2,benchmark}
mkdir -p tools/{doxygen,clang-format,static_analysis}

echo "📄 Creating essential files..."

# Root level files
cat > README.md << 'EOF'
# CppVerseHub - Comprehensive C++ Mastery Project

## 🌟 Overview
CppVerseHub is a comprehensive C++ learning and portfolio project that demonstrates mastery of all major C++ concepts through a space exploration simulation. This project serves as both an educational resource and a professional portfolio showcase.

## 🎯 Project Goals
- **Educational Mastery**: Cover all essential C++ concepts from basics to advanced
- **Practical Application**: Real-world style implementation in a space exploration theme
- **Portfolio Showcase**: Professional-grade code organization and documentation
- **Revision Hub**: Quick reference materials and cheat sheets

## 🛸 The Space Exploration Theme
The project simulates managing planets, fleets, missions, and interstellar resources, providing natural contexts for demonstrating complex C++ concepts.

## 🔧 Build Instructions
```bash
mkdir build && cd build
cmake ..
make
```

## 📚 Documentation
See `docs/` directory for comprehensive documentation including:
- Cheat sheets for quick concept revision
- UML diagrams showing system architecture
- Design documentation explaining implementation decisions

## 🧪 Testing
```bash
cd build
make test
```

## 📂 Project Structure
See the complete project organization in the directory tree above.

## 🚀 Getting Started
1. Build the project using CMake
2. Run the main simulation: `./build/CppVerseHub`
3. Explore examples in `examples/` directory
4. Check documentation for concept explanations

## 🎓 Learning Path
1. Start with basic usage examples
2. Review cheat sheets for concept understanding
3. Examine source code implementations
4. Run and modify tests to experiment
5. Create your own extensions

---
*A project dedicated to C++ mastery and continuous learning*
EOF

cat > .gitignore << 'EOF'
# Build directories
build/
*.exe
*.out
*.app

# Compiled Object files
*.o
*.obj

# Precompiled Headers
*.gch
*.pch

# Libraries
*.lib
*.a
*.la
*.lo

# Dynamic libraries
*.dll
*.so
*.so.*
*.dylib

# Qt-es
object_script.*.Release
object_script.*.Debug
*_plugin_import.cpp
/.qmake.cache
/.qmake.stash
*.pro.user
*.pro.user.*
*.qbs.user
*.qbs.user.*
*.moc
moc_*.cpp
moc_*.h
qrc_*.cpp
ui_*.h
*.qmlc
*.jsc
Makefile*
*build-*

# CMake
CMakeCache.txt
CMakeFiles
CMakeScripts
Testing
Makefile
cmake_install.cmake
install_manifest.txt
compile_commands.json
CTestTestfile.cmake

# IDEs
.vscode/
.idea/
*.swp
*.swo
*~

# OS
.DS_Store
Thumbs.db

# Logs
*.log
src/data/logs/*.log

# Documentation build output
docs/api_reference/html/*
!docs/api_reference/html/.gitkeep
EOF

cat > CMakeLists.txt << 'EOF'
cmake_minimum_required(VERSION 3.16)
project(CppVerseHub VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags
set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Find packages
find_package(Threads REQUIRED)

# Include directories
include_directories(src)

# Add subdirectories
add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(examples)

# Main executable
add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} cppverse_core Threads::Threads)

# Enable testing
enable_testing()
add_test(NAME unit_tests COMMAND unit_tests)
add_test(NAME integration_tests COMMAND integration_tests)

# Documentation target
find_package(Doxygen)
if(DOXYGEN_FOUND)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/tools/doxygen/Doxyfile.in
                   ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
    add_custom_target(doc
        ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen" VERBATIM)
endif()
EOF

# Source CMakeLists
cat > src/CMakeLists.txt << 'EOF'
# Core library
add_subdirectory(core)
add_subdirectory(patterns)
add_subdirectory(templates)
add_subdirectory(stl_showcase)
add_subdirectory(memory)
add_subdirectory(concurrency)
add_subdirectory(modern)
add_subdirectory(algorithms)
add_subdirectory(utils)

# Create main library
add_library(cppverse_core STATIC
    $<TARGET_OBJECTS:core>
    $<TARGET_OBJECTS:patterns>
    $<TARGET_OBJECTS:utils>
)

target_link_libraries(cppverse_core Threads::Threads)
EOF

# Create essential core files
cat > src/main.cpp << 'EOF'
/**
 * @file main.cpp
 * @brief CppVerseHub - Space Exploration Command Center
 * @author Your Name
 * @date 2025
 */

#include <iostream>
#include <string>
#include <memory>

#include "core/GameEngine.hpp"
#include "utils/Logger.hpp"
#include "utils/ConfigManager.hpp"

/**
 * @brief Display welcome message and project information
 */
void displayWelcome() {
    std::cout << R"(
    ╔══════════════════════════════════════════════════════════╗
    ║                      CppVerseHub                         ║
    ║              Space Exploration Command Center            ║
    ║                                                          ║
    ║        A Comprehensive C++ Mastery Demonstration        ║
    ╚══════════════════════════════════════════════════════════╝
    )" << std::endl;
}

/**
 * @brief Display main menu options
 */
void displayMenu() {
    std::cout << "\n🚀 Mission Control Menu:\n"
              << "1. Start Space Exploration Simulation\n"
              << "2. View Fleet Status\n"
              << "3. Manage Planets\n"
              << "4. Review Mission History\n"
              << "5. Run C++ Concept Demonstrations\n"
              << "6. Exit\n"
              << "Enter your choice: ";
}

int main() {
    try {
        displayWelcome();
        
        // Initialize core systems
        auto logger = std::make_unique<Logger>("simulation.log");
        logger->info("CppVerseHub simulation starting...");
        
        auto config = ConfigManager::getInstance();
        config.loadConfig("src/data/config/app_config.json");
        
        GameEngine engine;
        engine.initialize();
        
        int choice;
        bool running = true;
        
        while (running) {
            displayMenu();
            std::cin >> choice;
            
            switch (choice) {
                case 1:
                    std::cout << "🌌 Starting space exploration simulation...\n";
                    engine.runSimulation();
                    break;
                case 2:
                    std::cout << "🚢 Displaying fleet status...\n";
                    engine.displayFleetStatus();
                    break;
                case 3:
                    std::cout << "🪐 Managing planetary systems...\n";
                    engine.managePlanets();
                    break;
                case 4:
                    std::cout << "📋 Reviewing mission history...\n";
                    engine.displayMissionHistory();
                    break;
                case 5:
                    std::cout << "🧠 Running C++ concept demonstrations...\n";
                    engine.runConceptDemonstrations();
                    break;
                case 6:
                    std::cout << "👋 Exiting CppVerseHub. Safe travels!\n";
                    running = false;
                    break;
                default:
                    std::cout << "❌ Invalid choice. Please try again.\n";
            }
        }
        
        logger->info("CppVerseHub simulation ended.");
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Fatal error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
EOF

# Create core CMakeLists
cat > src/core/CMakeLists.txt << 'EOF'
add_library(core OBJECT
    Entity.cpp
    Planet.cpp
    Fleet.cpp
    Mission.cpp
    ExplorationMission.cpp
    ColonizationMission.cpp
    CombatMission.cpp
    ResourceManager.cpp
    Factory.cpp
    GameEngine.cpp
)

target_include_directories(core PUBLIC ${CMAKE_SOURCE_DIR}/src)
EOF

# Create sample header files
cat > src/core/Entity.hpp << 'EOF'
/**
 * @file Entity.hpp
 * @brief Abstract base class for all space entities
 */

#pragma once

#include <string>
#include <memory>
#include <vector>

namespace CppVerseHub {

/**
 * @brief Abstract base class demonstrating inheritance and polymorphism
 */
class Entity {
public:
    Entity(const std::string& name, int id);
    virtual ~Entity() = default;
    
    // Pure virtual functions (abstract interface)
    virtual void update() = 0;
    virtual void render() const = 0;
    virtual std::string getInfo() const = 0;
    
    // Common interface
    const std::string& getName() const { return name_; }
    int getId() const { return id_; }
    
protected:
    std::string name_;
    int id_;
    bool active_;
    
private:
    static int next_id_;
};

} // namespace CppVerseHub
EOF

# Create documentation files
cat > docs/README.md << 'EOF'
# CppVerseHub Documentation

## 📚 Documentation Structure

This directory contains comprehensive documentation for the CppVerseHub project:

### 🔖 Cheat Sheets (`cheat_sheets/`)
Quick reference guides for each major C++ concept:
- **OOP_CheatSheet.md** - Classes, inheritance, polymorphism
- **STL_CheatSheet.md** - Containers, algorithms, iterators  
- **Templates_CheatSheet.md** - Generic programming
- **Concurrency_CheatSheet.md** - Threading and async programming
- **ModernCpp_CheatSheet.md** - C++17/20/23 features
- **DesignPatterns_CheatSheet.md** - All implemented patterns
- **MemoryManagement_CheatSheet.md** - Smart pointers and RAII
- **Exceptions_CheatSheet.md** - Exception handling

### 🎨 UML Diagrams (`UML_diagrams/`)
Visual representations of system architecture:
- **class_hierarchy.svg** - Entity inheritance tree
- **mission_polymorphism.svg** - Mission type relationships  
- **design_patterns.svg** - Pattern implementations
- **system_architecture.svg** - Overall system design

### 📖 Design Documents (`design_docs/`)
Detailed explanations of design decisions:
- **architecture_overview.md** - System design rationale
- **design_patterns_explained.md** - Pattern selection reasoning
- **concurrency_design.md** - Threading model explanation
- **modern_cpp_usage.md** - Modern C++ feature justifications

### 🔍 API Reference (`api_reference/`)
Auto-generated API documentation using Doxygen.

## 🎯 Using the Documentation

1. **Learning**: Start with cheat sheets for quick concept review
2. **Understanding**: Read design docs for deeper insights  
3. **Visualizing**: Use UML diagrams to understand relationships
4. **Reference**: Use API docs for detailed function information

## 🚀 Quick Start Learning Path

1. Review the architecture overview
2. Study relevant cheat sheets
3. Examine corresponding source code
4. Run related tests to see concepts in action
5. Modify examples to experiment

---
*Documentation is updated with each major release*
EOF

# Create test files
cat > tests/CMakeLists.txt << 'EOF'
# Find testing framework
find_package(Catch2 QUIET)
if(NOT Catch2_FOUND)
    message(STATUS "Catch2 not found, using bundled version")
    add_subdirectory(${CMAKE_SOURCE_DIR}/external/catch2)
endif()

# Unit tests
add_executable(unit_tests
    test_main.cpp
    unit_tests/core_tests/EntityTests.cpp
    unit_tests/core_tests/MissionTests.cpp
    unit_tests/stl_tests/ContainerTests.cpp
    unit_tests/template_tests/GenericContainerTests.cpp
    unit_tests/concurrency_tests/ThreadPoolTests.cpp
    unit_tests/pattern_tests/SingletonTests.cpp
    unit_tests/memory_tests/SmartPointerTests.cpp
)

target_link_libraries(unit_tests 
    cppverse_core 
    Catch2::Catch2WithMain
    Threads::Threads
)

# Integration tests
add_executable(integration_tests
    test_main.cpp
    integration_tests/SimulationIntegrationTests.cpp
    integration_tests/FileIOIntegrationTests.cpp
)

target_link_libraries(integration_tests 
    cppverse_core 
    Catch2::Catch2WithMain
)

# Benchmark tests
add_executable(benchmark_tests
    benchmark_tests/AlgorithmBenchmarks.cpp
    benchmark_tests/ContainerBenchmarks.cpp
)

target_link_libraries(benchmark_tests 
    cppverse_core
    benchmark::benchmark
    Threads::Threads
)
EOF

cat > tests/test_main.cpp << 'EOF'
#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>

// Test main file for CppVerseHub
// Individual test files will be included automatically
EOF

# Create examples
cat > examples/CMakeLists.txt << 'EOF'
# Basic usage examples
add_executable(getting_started basic_usage/getting_started.cpp)
target_link_libraries(getting_started cppverse_core)

add_executable(entity_creation basic_usage/entity_creation.cpp)
target_link_libraries(entity_creation cppverse_core)

add_executable(simple_simulation basic_usage/simple_simulation.cpp)
target_link_libraries(simple_simulation cppverse_core)

# Advanced examples
add_executable(custom_missions advanced_usage/custom_missions.cpp)
target_link_libraries(custom_missions cppverse_core)

add_executable(plugin_systems advanced_usage/plugin_systems.cpp)
target_link_libraries(plugin_systems cppverse_core)

# Tutorial examples
add_executable(cpp_walkthrough tutorials/cpp_concepts_walkthrough.cpp)
target_link_libraries(cpp_walkthrough cppverse_core)
EOF

# Create utility scripts
cat > scripts/build.sh << 'EOF'
#!/bin/bash
# Build script for CppVerseHub

echo "🔨 Building CppVerseHub..."

# Create build directory
mkdir -p build
cd build

# Configure with CMake
cmake -DCMAKE_BUILD_TYPE=Release ..

# Build the project
make -j$(nproc)

echo "✅ Build completed successfully!"
echo "Run the simulation with: ./CppVerseHub"
EOF

cat > scripts/run_tests.sh << 'EOF'
#!/bin/bash
# Test runner script for CppVerseHub

echo "🧪 Running CppVerseHub tests..."

cd build

# Run unit tests
echo "Running unit tests..."
./unit_tests

# Run integration tests  
echo "Running integration tests..."
./integration_tests

# Run benchmarks
echo "Running performance benchmarks..."
./benchmark_tests

echo "✅ All tests completed!"
EOF

cat > scripts/generate_docs.sh << 'EOF'
#!/bin/bash
# Documentation generation script

echo "📚 Generating CppVerseHub documentation..."

# Generate Doxygen documentation
cd build
make doc

echo "✅ Documentation generated in docs/api_reference/html/"
echo "Open docs/api_reference/html/index.html in your browser"
EOF

cat > scripts/clean.sh << 'EOF'
#!/bin/bash
# Clean build artifacts

echo "🧹 Cleaning CppVerseHub build artifacts..."

rm -rf build/
rm -rf docs/api_reference/html/*
find . -name "*.log" -delete
find . -name "*.tmp" -delete

echo "✅ Clean completed!"
EOF

# Make scripts executable
chmod +x scripts/*.sh

# Create sample data files
cat > src/data/config/app_config.json << 'EOF'
{
    "simulation": {
        "max_planets": 100,
        "max_fleets": 50,
        "simulation_speed": 1.0,
        "auto_save_interval": 300
    },
    "logging": {
        "level": "INFO",
        "file": "simulation.log",
        "console_output": true
    },
    "graphics": {
        "enable_visualization": false,
        "fps_limit": 60
    },
    "performance": {
        "thread_pool_size": 4,
        "enable_multithreading": true
    }
}
EOF

cat > src/data/config/planets.json << 'EOF'
{
    "planets": [
        {
            "name": "Earth",
            "type": "terrestrial",
            "resources": {"water": 100, "minerals": 50, "energy": 75},
            "population": 1000000,
            "coordinates": {"x": 0, "y": 0, "z": 0}
        },
        {
            "name": "Mars",
            "type": "terrestrial", 
            "resources": {"water": 20, "minerals": 80, "energy": 30},
            "population": 50000,
            "coordinates": {"x": 100, "y": 50, "z": 10}
        },
        {
            "name": "Titan",
            "type": "moon",
            "resources": {"water": 90, "minerals": 30, "energy": 60},
            "population": 10000,
            "coordinates": {"x": 200, "y": 100, "z": 20}
        }
    ]
}
EOF

# Create placeholder files to maintain directory structure
touch docs/api_reference/html/.gitkeep
touch src/data/logs/.gitkeep
touch build/.gitkeep
touch external/json/.gitkeep
touch external/catch2/.gitkeep
touch external/benchmark/.gitkeep

echo ""
echo "✅ CppVerseHub project structure created successfully!"
echo ""
echo "📁 Project created at: $PROJECT_ROOT"
echo ""
echo "🚀 Next steps:"
echo "1. cd $PROJECT_NAME"
echo "2. ./scripts/build.sh"
echo "3. ./CppVerseHub"
echo ""
echo "📚 Documentation:"
echo "- Check docs/ for learning materials"
echo "- Run ./scripts/generate_docs.sh for API docs"
echo "- Explore examples/ for usage patterns"
echo ""
echo "🧪 Testing:"
echo "- Run ./scripts/run_tests.sh for all tests"
echo "- Individual test binaries in build/"
echo ""
echo "Happy coding! 🎉"