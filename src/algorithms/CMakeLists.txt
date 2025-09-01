# CMakeLists.txt for CppVerseHub Algorithms Module
# File location: src/algorithms/CMakeLists.txt

cmake_minimum_required(VERSION 3.20)

# Set C++ standard requirements
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Module information
set(MODULE_NAME "Algorithms")
set(MODULE_DESCRIPTION "Comprehensive algorithm implementations and demonstrations")

message(STATUS "Configuring ${MODULE_NAME} module...")

# ============================================================================
# Compiler-specific optimizations and warnings
# ============================================================================

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(ALGORITHMS_COMPILE_FLAGS
        -Wall -Wextra -Wpedantic
        -Wconversion -Wsign-conversion
        -Wcast-qual -Wcast-align
        -Wfloat-equal -Wredundant-decls
        -Winit-self -Wswitch-default
        -Wundef -Wshadow
        -O3 -march=native
        -ffast-math -funroll-loops
    )
    
    # Enable additional optimizations for Release builds
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG -flto")
    
    # Debug flags
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -fsanitize=address,undefined")
    
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set(ALGORITHMS_COMPILE_FLAGS
        /W4 /WX
        /O2 /Ob2 /Oi /Ot /Oy /GT
        /fp:fast /arch:AVX2
    )
    
    # MSVC specific optimizations
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /DNDEBUG /GL /LTCG")
endif()

# ============================================================================
# Find required packages and dependencies
# ============================================================================

# Threading support for parallel algorithms
find_package(Threads REQUIRED)

# Optional: OpenMP for parallel processing
find_package(OpenMP QUIET)
if(OpenMP_CXX_FOUND)
    message(STATUS "OpenMP found - enabling parallel algorithms")
    set(ENABLE_OPENMP ON)
else()
    message(STATUS "OpenMP not found - using standard threading")
    set(ENABLE_OPENMP OFF)
endif()

# Optional: Google Benchmark for performance testing
find_package(benchmark QUIET)
if(benchmark_FOUND)
    message(STATUS "Google Benchmark found - enabling performance benchmarks")
    set(ENABLE_BENCHMARKS ON)
else()
    message(STATUS "Google Benchmark not found - skipping benchmarks")
    set(ENABLE_BENCHMARKS OFF)
endif()

# ============================================================================
# Source files configuration
# ============================================================================

# Header files
set(ALGORITHMS_HEADERS
    SortingAlgorithms.hpp
    SearchAlgorithms.hpp
    GraphAlgorithms.hpp
    DataStructures.hpp
)

# Implementation files
set(ALGORITHMS_SOURCES
    SortingAlgorithms.cpp
    SearchAlgorithms.cpp
    GraphAlgorithms.cpp
    DataStructures.cpp
)

# Demo and test files (if they exist)
set(ALGORITHMS_DEMOS
    # Add demo files here if needed
)

# ============================================================================
# Main algorithms library
# ============================================================================

# Create the main algorithms library
add_library(cppversehub_algorithms STATIC
    ${ALGORITHMS_SOURCES}
    ${ALGORITHMS_HEADERS}
)

# Create an alias for consistent naming
add_library(CppVerseHub::Algorithms ALIAS cppversehub_algorithms)

# Set target properties
set_target_properties(cppversehub_algorithms PROPERTIES
    OUTPUT_NAME "cppversehub_algorithms"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    VERSION 1.0.0
    SOVERSION 1
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

# Apply compiler flags
target_compile_options(cppversehub_algorithms PRIVATE ${ALGORITHMS_COMPILE_FLAGS})

# Include directories
target_include_directories(cppversehub_algorithms
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include/cppversehub/algorithms>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link libraries
target_link_libraries(cppversehub_algorithms
    PUBLIC
        Threads::Threads
    PRIVATE
        $<$<BOOL:${ENABLE_OPENMP}>:OpenMP::OpenMP_CXX>
)

# Preprocessor definitions
target_compile_definitions(cppversehub_algorithms
    PUBLIC
        CPPVERSEHUB_ALGORITHMS_VERSION_MAJOR=1
        CPPVERSEHUB_ALGORITHMS_VERSION_MINOR=0
        CPPVERSEHUB_ALGORITHMS_VERSION_PATCH=0
    PRIVATE
        $<$<BOOL:${ENABLE_OPENMP}>:CPPVERSEHUB_ENABLE_OPENMP>
        $<$<CONFIG:Debug>:CPPVERSEHUB_DEBUG_MODE>
        $<$<CONFIG:Release>:CPPVERSEHUB_OPTIMIZED_BUILD>
)

# ============================================================================
# Individual algorithm component libraries (optional)
# ============================================================================

# Sorting Algorithms Component
add_library(cppversehub_sorting STATIC
    SortingAlgorithms.cpp
    SortingAlgorithms.hpp
)

target_include_directories(cppversehub_sorting
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(cppversehub_sorting PRIVATE ${ALGORITHMS_COMPILE_FLAGS})
target_link_libraries(cppversehub_sorting PUBLIC Threads::Threads)

# Search Algorithms Component
add_library(cppversehub_search STATIC
    SearchAlgorithms.cpp
    SearchAlgorithms.hpp
)

target_include_directories(cppversehub_search
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(cppversehub_search PRIVATE ${ALGORITHMS_COMPILE_FLAGS})
target_link_libraries(cppversehub_search PUBLIC Threads::Threads)

# Graph Algorithms Component
add_library(cppversehub_graph STATIC
    GraphAlgorithms.cpp
    GraphAlgorithms.hpp
)

target_include_directories(cppversehub_graph
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(cppversehub_graph PRIVATE ${ALGORITHMS_COMPILE_FLAGS})
target_link_libraries(cppversehub_graph PUBLIC Threads::Threads)

# Data Structures Component
add_library(cppversehub_datastructures STATIC
    DataStructures.cpp
    DataStructures.hpp
)

target_include_directories(cppversehub_datastructures
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(cppversehub_datastructures PRIVATE ${ALGORITHMS_COMPILE_FLAGS})
target_link_libraries(cppversehub_datastructures PUBLIC Threads::Threads)

# ============================================================================
# Executable demos and examples
# ============================================================================

# Main algorithms demo executable
add_executable(algorithms_demo
    demos/main_demo.cpp
)

target_link_libraries(algorithms_demo
    PRIVATE
        cppversehub_algorithms
        Threads::Threads
)

target_compile_options(algorithms_demo PRIVATE ${ALGORITHMS_COMPILE_FLAGS})

# Set output directory
set_target_properties(algorithms_demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/demos"
)

# Individual component demos
set(DEMO_TARGETS
    sorting_demo
    search_demo
    graph_demo
    datastructures_demo
)

set(DEMO_SOURCES
    demos/sorting_demo.cpp
    demos/search_demo.cpp
    demos/graph_demo.cpp
    demos/datastructures_demo.cpp
)

set(DEMO_LIBRARIES
    cppversehub_sorting
    cppversehub_search
    cppversehub_graph
    cppversehub_datastructures
)

# Create demo executables
foreach(i RANGE 3)
    list(GET DEMO_TARGETS ${i} target)
    list(GET DEMO_SOURCES ${i} source)
    list(GET DEMO_LIBRARIES ${i} library)
    
    # Check if source file exists
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${source}")
        add_executable(${target} ${source})
        target_link_libraries(${target} PRIVATE ${library} Threads::Threads)
        target_compile_options(${target} PRIVATE ${ALGORITHMS_COMPILE_FLAGS})
        
        set_target_properties(${target} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/demos"
        )
    endif()
endforeach()

# ============================================================================
# Performance benchmarks (if Google Benchmark is available)
# ============================================================================

if(ENABLE_BENCHMARKS)
    # Sorting algorithms benchmark
    add_executable(sorting_benchmark
        benchmarks/sorting_benchmark.cpp
    )
    
    target_link_libraries(sorting_benchmark
        PRIVATE
            cppversehub_sorting
            benchmark::benchmark
            benchmark::benchmark_main
            Threads::Threads
    )
    
    target_compile_options(sorting_benchmark PRIVATE ${ALGORITHMS_COMPILE_FLAGS})
    
    # Search algorithms benchmark
    add_executable(search_benchmark
        benchmarks/search_benchmark.cpp
    )
    
    target_link_libraries(search_benchmark
        PRIVATE
            cppversehub_search
            benchmark::benchmark
            benchmark::benchmark_main
            Threads::Threads
    )
    
    target_compile_options(search_benchmark PRIVATE ${ALGORITHMS_COMPILE_FLAGS})
    
    # Graph algorithms benchmark
    add_executable(graph_benchmark
        benchmarks/graph_benchmark.cpp
    )
    
    target_link_libraries(graph_benchmark
        PRIVATE
            cppversehub_graph
            benchmark::benchmark
            benchmark::benchmark_main
            Threads::Threads
    )
    
    target_compile_options(graph_benchmark PRIVATE ${ALGORITHMS_COMPILE_FLAGS})
    
    # Set benchmark output directory
    set_target_properties(
        sorting_benchmark
        search_benchmark
        graph_benchmark
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/benchmarks"
    )
endif()

# ============================================================================
# Unit tests (if testing is enabled)
# ============================================================================

if(BUILD_TESTING)
    enable_testing()
    
    # Find or fetch Google Test
    find_package(GTest QUIET)
    
    if(NOT GTest_FOUND)
        include(FetchContent)
        FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG release-1.12.1
        )
        FetchContent_MakeAvailable(googletest)
    endif()
    
    # Test executables
    set(TEST_SOURCES
        tests/test_sorting_algorithms.cpp
        tests/test_search_algorithms.cpp
        tests/test_graph_algorithms.cpp
        tests/test_data_structures.cpp
    )
    
    set(TEST_TARGETS
        test_sorting
        test_search
        test_graph
        test_datastructures
    )
    
    set(TEST_LIBRARIES
        cppversehub_sorting
        cppversehub_search
        cppversehub_graph
        cppversehub_datastructures
    )
    
    # Create test executables
    foreach(i RANGE 3)
        list(GET TEST_TARGETS ${i} target)
        list(GET TEST_SOURCES ${i} source)
        list(GET TEST_LIBRARIES ${i} library)
        
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${source}")
            add_executable(${target} ${source})
            target_link_libraries(${target}
                PRIVATE
                    ${library}
                    GTest::gtest
                    GTest::gtest_main
                    Threads::Threads
            )
            target_compile_options(${target} PRIVATE ${ALGORITHMS_COMPILE_FLAGS})
            
            set_target_properties(${target} PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/tests"
            )
            
            # Add to CTest
            add_test(NAME ${target} COMMAND ${target})
        endif()
    endforeach()
    
    # Comprehensive algorithm test suite
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/comprehensive_tests.cpp")
        add_executable(algorithms_tests
            tests/comprehensive_tests.cpp
        )
        
        target_link_libraries(algorithms_tests
            PRIVATE
                cppversehub_algorithms
                GTest::gtest
                GTest::gtest_main
                Threads::Threads
        )
        
        target_compile_options(algorithms_tests PRIVATE ${ALGORITHMS_COMPILE_FLAGS})
        
        set_target_properties(algorithms_tests PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/tests"
        )
        
        add_test(NAME algorithms_comprehensive COMMAND algorithms_tests)
    endif()
endif()

# ============================================================================
# Custom targets for development
# ============================================================================

# Format code using clang-format
find_program(CLANG_FORMAT_EXECUTABLE clang-format)
if(CLANG_FORMAT_EXECUTABLE)
    add_custom_target(format_algorithms
        COMMAND ${CLANG_FORMAT_EXECUTABLE} -i -style=file
        ${ALGORITHMS_SOURCES} ${ALGORITHMS_HEADERS}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Formatting algorithms module source code"
    )
endif()

# Static analysis using clang-tidy
find_program(CLANG_TIDY_EXECUTABLE clang-tidy)
if(CLANG_TIDY_EXECUTABLE)
    add_custom_target(tidy_algorithms
        COMMAND ${CLANG_TIDY_EXECUTABLE} ${ALGORITHMS_SOURCES}
        -- -std=c++20 -I${CMAKE_CURRENT_SOURCE_DIR}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Running static analysis on algorithms module"
    )
endif()

# Documentation generation
find_program(DOXYGEN_EXECUTABLE doxygen)
if(DOXYGEN_EXECUTABLE AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile")
    add_custom_target(docs_algorithms
        COMMAND ${DOXYGEN_EXECUTABLE} docs/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating algorithms module documentation"
    )
endif()

# ============================================================================
# Installation configuration
# ============================================================================

include(GNUInstallDirs)

# Install the library
install(TARGETS cppversehub_algorithms
    EXPORT CppVerseHubAlgorithmsTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install component libraries
install(TARGETS
    cppversehub_sorting
    cppversehub_search
    cppversehub_graph
    cppversehub_datastructures
    EXPORT CppVerseHubAlgorithmsTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# Install headers
install(FILES ${ALGORITHMS_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/cppversehub/algorithms
)

# Install demo executables
install(TARGETS algorithms_demo
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/demos
)

# Install the export set
install(EXPORT CppVerseHubAlgorithmsTargets
    FILE CppVerseHubAlgorithmsTargets.cmake
    NAMESPACE CppVerseHub::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/CppVerseHub
)

# Create and install config files
include(CMakePackageConfigHelpers)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/CppVerseHubAlgorithmsConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/CppVerseHubAlgorithmsConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/CppVerseHub
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/CppVerseHubAlgorithmsConfigVersion.cmake"
    VERSION 1.0.0
    COMPATIBILITY AnyNewerVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/CppVerseHubAlgorithmsConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/CppVerseHubAlgorithmsConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/CppVerseHub
)

# ============================================================================
# Summary and status information
# ============================================================================

message(STATUS "")
message(STATUS "========================================")
message(STATUS "CppVerseHub Algorithms Module Summary")
message(STATUS "========================================")
message(STATUS "Module Name: ${MODULE_NAME}")
message(STATUS "C++ Standard: C++${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "OpenMP Support: ${ENABLE_OPENMP}")
message(STATUS "Benchmarks: ${ENABLE_BENCHMARKS}")
message(STATUS "Testing: ${BUILD_TESTING}")
message(STATUS "Install Prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")

# Display found dependencies
message(STATUS "Dependencies:")
message(STATUS "  - Threads: ${CMAKE_THREAD_LIBS_INIT}")
if(OpenMP_CXX_FOUND)
    message(STATUS "  - OpenMP: ${OpenMP_CXX_VERSION}")
endif()
if(benchmark_FOUND)
    message(STATUS "  - Google Benchmark: Found")
endif()
if(GTest_FOUND OR TARGET gtest)
    message(STATUS "  - Google Test: Found")
endif()
message(STATUS "")

message(STATUS "Libraries to be built:")
message(STATUS "  - cppversehub_algorithms (main library)")
message(STATUS "  - cppversehub_sorting")
message(STATUS "  - cppversehub_search")
message(STATUS "  - cppversehub_graph") 
message(STATUS "  - cppversehub_datastructures")
message(STATUS "")

message(STATUS "Executables to be built:")
message(STATUS "  - algorithms_demo")
if(ENABLE_BENCHMARKS)
    message(STATUS "  - sorting_benchmark")
    message(STATUS "  - search_benchmark")
    message(STATUS "  - graph_benchmark")
endif()
message(STATUS "========================================")
message(STATUS "")