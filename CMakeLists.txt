# File: CMakeLists.txt
# Root CMake configuration for CppVerseHub project

cmake_minimum_required(VERSION 3.20)

# Project information
project(CppVerseHub 
    VERSION 1.0.0
    DESCRIPTION "A Comprehensive C++ Learning Hub with Space Fleet Management Simulation"
    LANGUAGES CXX
)

# Require C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Export compile commands for clang-tidy and other tools
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Project options
option(BUILD_TESTING "Build the test suite" ON)
option(BUILD_EXAMPLES "Build example programs" ON)
option(BUILD_DOCUMENTATION "Build documentation" OFF)
option(ENABLE_WARNINGS "Enable compiler warnings" ON)
option(WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)
option(ENABLE_SANITIZERS "Enable sanitizers in debug builds" OFF)
option(ENABLE_COVERAGE "Enable code coverage" OFF)
option(USE_STATIC_ANALYSIS "Enable static analysis" OFF)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Platform-specific configurations
if(WIN32)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    add_definitions(-DNOMINMAX)
endif()

# Compiler-specific configurations
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # GCC and Clang common flags
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG")
    set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -DNDEBUG")
    
    if(ENABLE_WARNINGS)
        list(APPEND COMMON_COMPILE_FLAGS
            -Wall
            -Wextra
            -Wpedantic
            -Wcast-align
            -Wcast-qual
            -Wctor-dtor-privacy
            -Wdisabled-optimization
            -Wformat=2
            -Winit-self
            -Wmissing-declarations
            -Wmissing-include-dirs
            -Wold-style-cast
            -Woverloaded-virtual
            -Wredundant-decls
            -Wshadow
            -Wsign-conversion
            -Wsign-promo
            -Wstrict-overflow=5
            -Wswitch-default
            -Wundef
            -Wunused
        )
        
        # Additional GCC-specific warnings
        if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            list(APPEND COMMON_COMPILE_FLAGS
                -Wlogical-op
                -Wnoexcept
                -Wstrict-null-sentinel
                -Wuseless-cast
            )
        endif()
        
        # Additional Clang-specific warnings
        if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
            list(APPEND COMMON_COMPILE_FLAGS
                -Wdocumentation
                -Wimplicit-fallthrough
                -Wthread-safety
            )
        endif()
        
        if(WARNINGS_AS_ERRORS)
            list(APPEND COMMON_COMPILE_FLAGS -Werror)
        endif()
    endif()
    
    # Sanitizers for debug builds
    if(ENABLE_SANITIZERS AND CMAKE_BUILD_TYPE STREQUAL "Debug")
        list(APPEND COMMON_COMPILE_FLAGS
            -fsanitize=address
            -fsanitize=undefined
            -fno-omit-frame-pointer
        )
        list(APPEND COMMON_LINK_FLAGS
            -fsanitize=address
            -fsanitize=undefined
        )
    endif()
    
    # Coverage flags
    if(ENABLE_COVERAGE AND CMAKE_BUILD_TYPE STREQUAL "Debug")
        list(APPEND COMMON_COMPILE_FLAGS
            --coverage
            -fprofile-arcs
            -ftest-coverage
        )
        list(APPEND COMMON_LINK_FLAGS
            --coverage
            -lgcov
        )
    endif()
    
elseif(MSVC)
    # MSVC specific flags
    set(CMAKE_CXX_FLAGS_DEBUG "/Od /Zi /RTC1")
    set(CMAKE_CXX_FLAGS_RELEASE "/O2 /DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/O2 /Zi /DNDEBUG")
    set(CMAKE_CXX_FLAGS_MINSIZEREL "/O1 /DNDEBUG")
    
    if(ENABLE_WARNINGS)
        list(APPEND COMMON_COMPILE_FLAGS
            /W4
            /w14242  # 'identifier': conversion from 'type1' to 'type2', possible loss of data
            /w14254  # 'operator': conversion from 'type1:field_bits' to 'type2:field_bits', possible loss of data
            /w14263  # 'function': member function does not override any base class virtual member function
            /w14265  # 'classname': class has virtual functions, but destructor is not virtual
            /w14287  # 'operator': unsigned/negative constant mismatch
            /we4289  # nonstandard extension used: 'variable': loop control variable declared in the for-loop is used outside the for-loop scope
            /w14296  # 'operator': expression is always 'boolean_value'
            /w14311  # 'variable': pointer truncation from 'type1' to 'type2'
            /w14545  # expression before comma evaluates to a function which is missing an argument list
            /w14546  # function call before comma missing argument list
            /w14547  # 'operator': operator before comma has no effect; expected operator with side-effect
            /w14549  # 'operator': operator before comma has no effect; did you intend 'operator'?
            /w14555  # expression has no effect; expected expression with side-effect
            /w14619  # pragma warning: there is no warning number 'number'
            /w14640  # Enable warning on thread un-safe static member initialization
            /w14826  # Conversion from 'type1' to 'type2' is sign-extended
            /w14928  # illegal copy-initialization; more than one user-defined conversion has been implicitly applied
        )
        
        if(WARNINGS_AS_ERRORS)
            list(APPEND COMMON_COMPILE_FLAGS /WX)
        endif()
    endif()
    
    # MSVC specific definitions
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-D_SCL_SECURE_NO_WARNINGS)
endif()

# Find required packages
find_package(Threads REQUIRED)

# Include external dependencies
include(FetchContent)

# JSON library (nlohmann/json)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(json)

# If testing is enabled, fetch Catch2
if(BUILD_TESTING)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.4.0
    )
    FetchContent_MakeAvailable(Catch2)
    
    # Include Catch2 CMake modules
    list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
    include(Catch)
endif()

# Google Benchmark for performance testing
FetchContent_Declare(
    benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG v1.8.3
)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(benchmark)

# Function to create a library target with common settings
function(create_library_target target_name)
    # Apply common compile flags
    if(COMMON_COMPILE_FLAGS)
        target_compile_options(${target_name} PRIVATE ${COMMON_COMPILE_FLAGS})
    endif()
    
    # Apply common link flags
    if(COMMON_LINK_FLAGS)
        target_link_options(${target_name} PRIVATE ${COMMON_LINK_FLAGS})
    endif()
    
    # Set common properties
    set_target_properties(${target_name} PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        POSITION_INDEPENDENT_CODE ON
    )
endfunction()

# Function to create an executable target with common settings
function(create_executable_target target_name)
    # Apply common compile flags
    if(COMMON_COMPILE_FLAGS)
        target_compile_options(${target_name} PRIVATE ${COMMON_COMPILE_FLAGS})
    endif()
    
    # Apply common link flags
    if(COMMON_LINK_FLAGS)
        target_link_options(${target_name} PRIVATE ${COMMON_LINK_FLAGS})
    endif()
    
    # Set common properties
    set_target_properties(${target_name} PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )
    
    # Link with threading library
    target_link_libraries(${target_name} PRIVATE Threads::Threads)
endfunction()

# Add subdirectories
add_subdirectory(src)

if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()

if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Documentation target
if(BUILD_DOCUMENTATION)
    find_package(Doxygen QUIET)
    if(DOXYGEN_FOUND)
        set(DOXYGEN_PROJECT_NAME ${PROJECT_NAME})
        set(DOXYGEN_PROJECT_VERSION ${PROJECT_VERSION})
        set(DOXYGEN_PROJECT_BRIEF ${PROJECT_DESCRIPTION})
        
        doxygen_add_docs(
            docs
            ${CMAKE_SOURCE_DIR}/src
            ${CMAKE_SOURCE_DIR}/README.md
            COMMENT "Generating API documentation with Doxygen"
        )
    else()
        message(WARNING "Doxygen not found, documentation will not be built")
    endif()
endif()

# Static analysis targets
if(USE_STATIC_ANALYSIS)
    # clang-tidy target
    find_program(CLANG_TIDY_EXECUTABLE NAMES clang-tidy)
    if(CLANG_TIDY_EXECUTABLE)
        set(CMAKE_CXX_CLANG_TIDY 
            ${CLANG_TIDY_EXECUTABLE};
            --config-file=${CMAKE_SOURCE_DIR}/tools/static_analysis/.clang-tidy;
            --header-filter=.*
        )
        message(STATUS "clang-tidy found: ${CLANG_TIDY_EXECUTABLE}")
    else()
        message(WARNING "clang-tidy not found")
    endif()
    
    # cppcheck target
    find_program(CPPCHECK_EXECUTABLE NAMES cppcheck)
    if(CPPCHECK_EXECUTABLE)
        add_custom_target(cppcheck
            COMMAND ${CPPCHECK_EXECUTABLE}
                --enable=all
                --std=c++20
                --platform=unix64
                --suppress=missingIncludeSystem
                --suppress=unusedFunction
                --suppress=constParameter
                --quiet
                --verbose
                --error-exitcode=1
                ${CMAKE_SOURCE_DIR}/src
            COMMENT "Running cppcheck static analysis"
            VERBATIM
        )
        message(STATUS "cppcheck found: ${CPPCHECK_EXECUTABLE}")
    else()
        message(WARNING "cppcheck not found")
    endif()
endif()

# Custom targets for development
add_custom_target(format
    COMMAND find ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/tests ${CMAKE_SOURCE_DIR}/examples
            -name "*.cpp" -o -name "*.hpp" -o -name "*.h" -o -name "*.cxx" -o -name "*.hxx"
            | xargs clang-format -i --style=file:${CMAKE_SOURCE_DIR}/tools/clang-format/.clang-format
    COMMENT "Formatting source code with clang-format"
    VERBATIM
)

add_custom_target(analyze
    COMMAND ${CMAKE_SOURCE_DIR}/tools/static_analysis/run_analysis.sh
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running static analysis"
    VERBATIM
)

add_custom_target(clean-all
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target clean
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/docs
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/analysis
    COMMENT "Cleaning build artifacts and generated files"
    VERBATIM
)

# Print configuration summary
message(STATUS "")
message(STATUS "CppVerseHub Configuration Summary:")
message(STATUS "  Project Version: ${PROJECT_VERSION}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard: C++${CMAKE_CXX_STANDARD}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Build Testing: ${BUILD_TESTING}")
message(STATUS "  Build Examples: ${BUILD_EXAMPLES}")
message(STATUS "  Build Documentation: ${BUILD_DOCUMENTATION}")
message(STATUS "  Enable Warnings: ${ENABLE_WARNINGS}")
message(STATUS "  Warnings as Errors: ${WARNINGS_AS_ERRORS}")
message(STATUS "  Enable Sanitizers: ${ENABLE_SANITIZERS}")
message(STATUS "  Enable Coverage: ${ENABLE_COVERAGE}")
message(STATUS "  Static Analysis: ${USE_STATIC_ANALYSIS}")
message(STATUS "")

# Installation
include(GNUInstallDirs)

# Install headers
install(DIRECTORY src/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/cppversehub
    FILES_MATCHING 
    PATTERN "*.hpp"
    PATTERN "*.h"
)

# Install documentation
if(BUILD_DOCUMENTATION)
    install(DIRECTORY ${CMAKE_BINARY_DIR}/docs/
        DESTINATION ${CMAKE_INSTALL_DOCDIR}
        OPTIONAL
    )
endif()

# Create package configuration files
include(CMakePackageConfigHelpers)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CppVerseHubConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/CppVerseHubConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/CppVerseHub
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/CppVerseHubConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/CppVerseHubConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/CppVerseHubConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/CppVerseHub
)