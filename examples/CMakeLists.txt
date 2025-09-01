# File: examples/CMakeLists.txt
# CppVerseHub Examples Build Configuration
# Comprehensive example programs demonstrating library usage

cmake_minimum_required(VERSION 3.20)

# Examples project configuration
project(CppVerseHubExamples
    VERSION 1.0.0
    DESCRIPTION "CppVerseHub Example Programs"
    LANGUAGES CXX
)

# C++ Standard Requirements
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler-specific flags for examples
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(EXAMPLE_CXX_FLAGS
        -Wall -Wextra -Wpedantic
        -Wconversion -Wsign-conversion
        -Wno-unused-parameter
        -fno-rtti  # Examples don't need RTTI
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(EXAMPLE_CXX_FLAGS
        /W4 /WX-  # High warning level, warnings not as errors for examples
        /permissive-  # Conformance mode
        /Zc:__cplusplus  # Correct __cplusplus macro
    )
endif()

# Find required dependencies
find_package(Threads REQUIRED)

# Link to main CppVerseHub library
set(CPPVERSEHUB_LIBRARIES
    CppVerseHub::Core
    CppVerseHub::Templates  
    CppVerseHub::Patterns
    CppVerseHub::STL
    CppVerseHub::Concurrency
    CppVerseHub::Memory
    CppVerseHub::Modern
    CppVerseHub::Utils
    Threads::Threads
)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/utils
)

# Function to create example executable
function(add_cppversehub_example NAME SOURCE_FILE)
    add_executable(${NAME} ${SOURCE_FILE})
    
    target_compile_features(${NAME} PRIVATE cxx_std_20)
    target_compile_options(${NAME} PRIVATE ${EXAMPLE_CXX_FLAGS})
    
    target_link_libraries(${NAME} PRIVATE ${CPPVERSEHUB_LIBRARIES})
    
    # Set output directory
    set_target_properties(${NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/examples/debug
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/examples/release
    )
    
    # Add to examples target group
    set_target_properties(${NAME} PROPERTIES FOLDER "Examples")
    
    message(STATUS "Added example: ${NAME}")
endfunction()

# Basic Usage Examples
message(STATUS "Configuring Basic Usage Examples...")
add_subdirectory(basic_usage)

# Advanced Usage Examples  
message(STATUS "Configuring Advanced Usage Examples...")
add_subdirectory(advanced_usage)

# Tutorial Examples
message(STATUS "Configuring Tutorial Examples...")
add_subdirectory(tutorials)

# Create master examples target
add_custom_target(examples
    COMMENT "Build all CppVerseHub examples"
)

# Add dependencies to master target
add_dependencies(examples
    # Basic examples will be added by subdirectories
)

# Installation rules for examples
install(DIRECTORY ${CMAKE_BINARY_DIR}/examples/
    DESTINATION share/cppversehub/examples
    COMPONENT Examples
    FILES_MATCHING PATTERN "*.exe" PATTERN "*.out" PATTERN "*"
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                GROUP_READ GROUP_EXECUTE
                WORLD_READ WORLD_EXECUTE
)

# Install example source code for reference
install(DIRECTORY basic_usage/ advanced_usage/ tutorials/
    DESTINATION share/cppversehub/examples/src
    COMPONENT ExampleSources
    FILES_MATCHING PATTERN "*.cpp" PATTERN "*.hpp"
)

# Example data files
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/data)
    install(DIRECTORY data/
        DESTINATION share/cppversehub/examples/data
        COMPONENT ExampleData
    )
endif()

# Documentation for examples
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/README.md.in
    ${CMAKE_CURRENT_BINARY_DIR}/README.md
    @ONLY
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/README.md
    DESTINATION share/cppversehub/examples
    COMPONENT Examples
)

message(STATUS "Examples configuration complete")