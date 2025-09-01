# File: src/modern/CMakeLists.txt
# Modern C++17/20/23 Features Module

# Set minimum required version for modern features
cmake_minimum_required(VERSION 3.20)

# Create modern features library
add_library(modern_cpp STATIC
    RangesDemo.cpp
    ModulesDemo.cpp
    LambdaExpressions.cpp
    MoveSemantics.cpp
)

# Set C++20 standard for advanced features
target_compile_features(modern_cpp PUBLIC cxx_std_20)

# Enable concepts and ranges support
target_compile_options(modern_cpp PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:-fconcepts -std=c++20>
    $<$<CXX_COMPILER_ID:Clang>:-std=c++20>
    $<$<CXX_COMPILER_ID:MSVC>:/std:c++20>
)

# Link with core utilities
target_link_libraries(modern_cpp PRIVATE utils_lib)

# Include directories
target_include_directories(modern_cpp PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/utils
)

# Set library properties
set_target_properties(modern_cpp PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
)

# Add compiler-specific flags for better optimization
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(modern_cpp PRIVATE -Wall -Wextra -O3)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(modern_cpp PRIVATE -Wall -Wextra -O3)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(modern_cpp PRIVATE /W4 /O2)
endif()

# Export symbols for shared library builds
if(BUILD_SHARED_LIBS)
    target_compile_definitions(modern_cpp PRIVATE BUILDING_MODERN_CPP_LIB)
endif()

# Add subdirectory to parent if needed
if(CMAKE_PROJECT_NAME STREQUAL "CppVerseHub")
    message(STATUS "Configuring Modern C++ Features module")
endif()