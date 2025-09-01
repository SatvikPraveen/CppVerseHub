# Location: CppVerseHub/src/CMakeLists.txt
# Main source directory CMake configuration

# Set minimum CMake version
cmake_minimum_required(VERSION 3.16)

# Include directories for all subdirectories
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Add all subdirectories containing libraries
message(STATUS "Configuring CppVerseHub source modules...")

# Core library - fundamental entities and systems
add_subdirectory(core)
message(STATUS "  ✓ Core module configured")

# Template programming showcase
add_subdirectory(templates)
message(STATUS "  ✓ Templates module configured")

# Design patterns implementations
add_subdirectory(patterns)
message(STATUS "  ✓ Patterns module configured")

# STL usage demonstrations
add_subdirectory(stl_showcase)
message(STATUS "  ✓ STL Showcase module configured")

# Memory management examples
add_subdirectory(memory)
message(STATUS "  ✓ Memory module configured")

# Concurrency and multithreading
add_subdirectory(concurrency)
message(STATUS "  ✓ Concurrency module configured")

# Modern C++ features (C++17/20/23)
add_subdirectory(modern)
message(STATUS "  ✓ Modern C++ module configured")

# Algorithm implementations
add_subdirectory(algorithms)
message(STATUS "  ✓ Algorithms module configured")

# Utility classes and helpers
add_subdirectory(utils)
message(STATUS "  ✓ Utils module configured")

# Main executable
set(MAIN_EXECUTABLE "CppVerseHub")

# Create main executable
add_executable(${MAIN_EXECUTABLE} main.cpp)

# Set target properties for main executable
set_target_properties(${MAIN_EXECUTABLE} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    OUTPUT_NAME "CppVerseHub"
)

# Link all libraries to main executable
target_link_libraries(${MAIN_EXECUTABLE}
    PRIVATE
        CppVerseHub::Core
        CppVerseHub::Templates
        CppVerseHub::Patterns
        CppVerseHub::STL
        CppVerseHub::Memory
        CppVerseHub::Concurrency
        CppVerseHub::Modern
        CppVerseHub::Algorithms
        CppVerseHub::Utils
        Threads::Threads
)

# Compiler-specific settings for main executable
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${MAIN_EXECUTABLE} PRIVATE
        -Wall -Wextra -Wpedantic
        -Wno-unused-parameter
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(${MAIN_EXECUTABLE} PRIVATE
        /W4
        /wd4996
    )
endif()

# Include data files directory
add_subdirectory(data)

# Install main executable
install(TARGETS ${MAIN_EXECUTABLE}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Copy data files to build directory for development
add_custom_command(TARGET ${MAIN_EXECUTABLE} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/data
    $<TARGET_FILE_DIR:${MAIN_EXECUTABLE}>/data
    COMMENT "Copying data files to build directory"
)

# Install data files
install(DIRECTORY data/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/CppVerseHub
    FILES_MATCHING 
    PATTERN "*.json"
    PATTERN "*.csv"
    PATTERN "*.xml"
    PATTERN "logs" EXCLUDE
)

# Create convenience targets for building specific modules
add_custom_target(build_core
    DEPENDS CppVerseHub_Core
    COMMENT "Building only core module"
)

add_custom_target(build_patterns
    DEPENDS CppVerseHub_Patterns
    COMMENT "Building only patterns module"
)

add_custom_target(build_templates
    DEPENDS CppVerseHub_Templates
    COMMENT "Building only templates module"
)

add_custom_target(build_modern
    DEPENDS CppVerseHub_Modern
    COMMENT "Building only modern C++ module"
)

add_custom_target(build_all_libs
    DEPENDS 
        CppVerseHub_Core
        CppVerseHub_Templates  
        CppVerseHub_Patterns
        CppVerseHub_STL
        CppVerseHub_Memory
        CppVerseHub_Concurrency
        CppVerseHub_Modern
        CppVerseHub_Algorithms
        CppVerseHub_Utils
    COMMENT "Building all CppVerseHub libraries"
)

# Development and debugging targets
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    # Debug executable with sanitizers
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_executable(${MAIN_EXECUTABLE}_debug main.cpp)
        
        set_target_properties(${MAIN_EXECUTABLE}_debug PROPERTIES
            CXX_STANDARD 20
            CXX_STANDARD_REQUIRED ON
            CXX_EXTENSIONS OFF
        )
        
        target_compile_options(${MAIN_EXECUTABLE}_debug PRIVATE
            -fsanitize=address,undefined
            -fno-omit-frame-pointer
            -g3
            -O0
        )
        
        target_link_options(${MAIN_EXECUTABLE}_debug PRIVATE
            -fsanitize=address,undefined
        )
        
        target_link_libraries(${MAIN_EXECUTABLE}_debug PRIVATE
            CppVerseHub::Core
            CppVerseHub::Templates
            CppVerseHub::Patterns
            CppVerseHub::STL
            CppVerseHub::Memory
            CppVerseHub::Concurrency
            CppVerseHub::Modern
            CppVerseHub::Algorithms
            CppVerseHub::Utils
            Threads::Threads
        )
    endif()
    
    # Memory debugging target
    add_custom_target(debug_memory
        COMMAND ${CMAKE_COMMAND} -E env ASAN_OPTIONS=detect_leaks=1:abort_on_error=1 
                $<TARGET_FILE:${MAIN_EXECUTABLE}>
        DEPENDS ${MAIN_EXECUTABLE}
        COMMENT "Running with AddressSanitizer"
        VERBATIM
    )
endif()

# Performance profiling targets (Release builds)
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    # Performance benchmark target
    add_custom_target(benchmark
        COMMAND ${CMAKE_COMMAND} -E env 
                $<TARGET_FILE:${MAIN_EXECUTABLE}> --benchmark
        DEPENDS ${MAIN_EXECUTABLE}
        COMMENT "Running performance benchmarks"
    )
    
    # CPU profiling with perf (Linux only)
    if(UNIX AND NOT APPLE)
        find_program(PERF_PROGRAM perf)
        if(PERF_PROGRAM)
            add_custom_target(profile_cpu
                COMMAND ${PERF_PROGRAM} record -g 
                        $<TARGET_FILE:${MAIN_EXECUTABLE}> --profile
                DEPENDS ${MAIN_EXECUTABLE}
                COMMENT "CPU profiling with perf"
            )
        endif()
    endif()
endif()

# Documentation generation for source code
if(DOXYGEN_FOUND)
    set(DOXYGEN_PROJECT_NAME "CppVerseHub")
    set(DOXYGEN_PROJECT_BRIEF "C++ Learning and Demonstration Platform")
    set(DOXYGEN_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/docs")
    
    # Configure Doxygen
    set(DOXYGEN_RECURSIVE YES)
    set(DOXYGEN_EXTRACT_ALL YES)
    set(DOXYGEN_EXTRACT_PRIVATE YES)
    set(DOXYGEN_EXTRACT_STATIC YES)
    set(DOXYGEN_GENERATE_LATEX NO)
    set(DOXYGEN_GENERATE_HTML YES)
    set(DOXYGEN_HTML_OUTPUT html)
    set(DOXYGEN_USE_MDFILE_AS_MAINPAGE "README.md")
    
    doxygen_add_docs(src_docs
        ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating source documentation"
    )
endif()

# Code formatting target
find_program(CLANG_FORMAT_PROGRAM clang-format)
if(CLANG_FORMAT_PROGRAM)
    # Find all source files
    file(GLOB_RECURSE ALL_SOURCE_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/*.h
    )
    
    add_custom_target(format_all
        COMMAND ${CLANG_FORMAT_PROGRAM} -i ${ALL_SOURCE_FILES}
        COMMENT "Formatting all source files"
    )
endif()

# Static analysis target
find_program(CLANG_TIDY_PROGRAM clang-tidy)
if(CLANG_TIDY_PROGRAM)
    add_custom_target(analyze_all
        COMMAND ${CLANG_TIDY_PROGRAM} 
                ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
                ${CMAKE_CURRENT_SOURCE_DIR}/*/*.cpp
                -- -std=c++20 -I${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Running static analysis on all source files"
    )
endif()

# Clean target for generated files
add_custom_target(clean_generated
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}/docs
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}/data
    COMMENT "Cleaning generated files"
)

# Summary information
message(STATUS "=== CppVerseHub Source Configuration ===")
message(STATUS "Main executable: ${MAIN_EXECUTABLE}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")

# List all configured libraries
get_property(CONFIGURED_LIBS GLOBAL PROPERTY CONFIGURED_LIBRARIES)
if(CONFIGURED_LIBS)
    message(STATUS "Configured libraries: ${CONFIGURED_LIBS}")
endif()

# Development tools status
if(CLANG_FORMAT_PROGRAM)
    message(STATUS "Code formatting: Available (${CLANG_FORMAT_PROGRAM})")
else()
    message(STATUS "Code formatting: Not available")
endif()

if(CLANG_TIDY_PROGRAM)
    message(STATUS "Static analysis: Available (${CLANG_TIDY_PROGRAM})")
else()
    message(STATUS "Static analysis: Not available")
endif()

if(DOXYGEN_FOUND)
    message(STATUS "Documentation: Available")
else()
    message(STATUS "Documentation: Not available (Doxygen not found)")
endif()

message(STATUS "========================================")