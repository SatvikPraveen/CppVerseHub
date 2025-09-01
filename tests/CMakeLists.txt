# File: tests/CMakeLists.txt
# Test build configuration for CppVerseHub comprehensive testing suite

cmake_minimum_required(VERSION 3.16)

# Enable testing
enable_testing()

# Find required packages
find_package(Threads REQUIRED)

# Add Catch2 testing framework
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../external/catch2 catch2)

# Add Google Benchmark (for benchmark tests)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../external/benchmark benchmark)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src/core)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src/templates)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src/patterns)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src/stl_showcase)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src/memory)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src/concurrency)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src/modern)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src/algorithms)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src/utils)

# Compiler flags for testing
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -g")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Define source files for the main project (needed for testing)
file(GLOB_RECURSE CORE_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/core/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/patterns/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/stl_showcase/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/memory/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/concurrency/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/modern/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/algorithms/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/utils/*.cpp"
)

# Remove main.cpp from core sources
list(FILTER CORE_SOURCES EXCLUDE REGEX ".*main\\.cpp$")

# Unit test sources
file(GLOB_RECURSE UNIT_TEST_SOURCES 
    "unit_tests/*.cpp"
)

# Integration test sources
file(GLOB_RECURSE INTEGRATION_TEST_SOURCES 
    "integration_tests/*.cpp"
)

# Benchmark test sources
file(GLOB_RECURSE BENCHMARK_TEST_SOURCES 
    "benchmark_tests/*.cpp"
)

# Create unit test executable
add_executable(unit_tests 
    test_main.cpp
    ${UNIT_TEST_SOURCES}
    ${CORE_SOURCES}
)

target_link_libraries(unit_tests 
    Catch2::Catch2
    Threads::Threads
)

target_compile_features(unit_tests PRIVATE cxx_std_20)

# Create integration test executable
add_executable(integration_tests 
    test_main.cpp
    ${INTEGRATION_TEST_SOURCES}
    ${CORE_SOURCES}
)

target_link_libraries(integration_tests 
    Catch2::Catch2
    Threads::Threads
)

target_compile_features(integration_tests PRIVATE cxx_std_20)

# Create benchmark test executable
add_executable(benchmark_tests 
    ${BENCHMARK_TEST_SOURCES}
    ${CORE_SOURCES}
)

target_link_libraries(benchmark_tests 
    benchmark::benchmark
    Threads::Threads
)

target_compile_features(benchmark_tests PRIVATE cxx_std_20)

# Add tests to CTest
add_test(NAME UnitTests COMMAND unit_tests)
add_test(NAME IntegrationTests COMMAND integration_tests)
add_test(NAME BenchmarkTests COMMAND benchmark_tests --benchmark_min_time=1.0)

# Set test properties
set_tests_properties(UnitTests PROPERTIES 
    LABELS "unit"
    TIMEOUT 300
)

set_tests_properties(IntegrationTests PROPERTIES 
    LABELS "integration"
    TIMEOUT 600
)

set_tests_properties(BenchmarkTests PROPERTIES 
    LABELS "benchmark"
    TIMEOUT 1800
)

# Custom targets for running specific test categories
add_custom_target(run_unit_tests
    COMMAND ${CMAKE_CTEST_COMMAND} -L unit -V
    DEPENDS unit_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_custom_target(run_integration_tests
    COMMAND ${CMAKE_CTEST_COMMAND} -L integration -V
    DEPENDS integration_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_custom_target(run_benchmark_tests
    COMMAND ${CMAKE_CTEST_COMMAND} -L benchmark -V
    DEPENDS benchmark_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_custom_target(run_all_tests
    COMMAND ${CMAKE_CTEST_COMMAND} -V
    DEPENDS unit_tests integration_tests benchmark_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Code coverage support (optional)
option(ENABLE_COVERAGE "Enable coverage reporting" OFF)
if(ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(unit_tests PRIVATE --coverage)
    target_link_options(unit_tests PRIVATE --coverage)
    target_compile_options(integration_tests PRIVATE --coverage)
    target_link_options(integration_tests PRIVATE --coverage)
endif()

# Memory sanitizer support (optional)
option(ENABLE_SANITIZER "Enable address sanitizer" OFF)
if(ENABLE_SANITIZER AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(unit_tests PRIVATE -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(unit_tests PRIVATE -fsanitize=address)
    target_compile_options(integration_tests PRIVATE -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(integration_tests PRIVATE -fsanitize=address)
endif()

message(STATUS "Test configuration complete - Unit, Integration, and Benchmark tests configured")