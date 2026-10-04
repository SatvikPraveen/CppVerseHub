cmake_minimum_required(VERSION 3.25...4.0)

project(CppVerseHub
    VERSION 2.0.0
    DESCRIPTION "A research-grade Modern C++20 reference implementation built around a space-fleet simulation domain"
    HOMEPAGE_URL "https://github.com/SatvikPraveen/CppVerseHub"
    LANGUAGES CXX
)

# ---------------------------------------------------------------------------
# Global configuration
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
# The project uses headers, not C++20 named modules; dependency scanning only adds cost and
# breaks clang-tidy, which cannot read the generated .modmap response files.
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if(PROJECT_IS_TOP_LEVEL AND NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
option(CPPVERSEHUB_BUILD_TESTS        "Build the Catch2 test suite"                 ${PROJECT_IS_TOP_LEVEL})
option(CPPVERSEHUB_BUILD_BENCHMARKS   "Build the Google Benchmark micro-benchmarks" ${PROJECT_IS_TOP_LEVEL})
option(CPPVERSEHUB_BUILD_EXAMPLES     "Build example programs"                      ${PROJECT_IS_TOP_LEVEL})
option(CPPVERSEHUB_BUILD_DOCS         "Build Doxygen API documentation"             OFF)
option(CPPVERSEHUB_WARNINGS_AS_ERRORS "Treat compiler warnings as errors"           OFF)
option(CPPVERSEHUB_ENABLE_SANITIZERS  "Enable Address/Undefined sanitizers"         OFF)
option(CPPVERSEHUB_ENABLE_TSAN        "Enable ThreadSanitizer (exclusive with ASan)" OFF)
option(CPPVERSEHUB_ENABLE_COVERAGE    "Enable source-level code coverage"           OFF)
option(CPPVERSEHUB_ENABLE_LTO         "Enable link-time optimisation in Release"    OFF)
option(CPPVERSEHUB_ENABLE_CLANG_TIDY  "Run clang-tidy as part of the build"         OFF)
option(CPPVERSEHUB_ENABLE_CPPCHECK    "Run cppcheck as part of the build"           OFF)

# ---------------------------------------------------------------------------
# Toolchain helpers
# ---------------------------------------------------------------------------
include(CompilerWarnings)
include(Sanitizers)
include(Coverage)
include(StaticAnalysis)
include(Atomics)
include(ModuleHelpers)
include(Dependencies)

find_package(Threads REQUIRED)

# Project-wide interface target carrying warnings, sanitizer and coverage flags.
add_library(cppversehub_options INTERFACE)
add_library(CppVerseHub::Options ALIAS cppversehub_options)
cppversehub_set_warnings(cppversehub_options ${CPPVERSEHUB_WARNINGS_AS_ERRORS})
cppversehub_enable_sanitizers(cppversehub_options)
cppversehub_enable_coverage(cppversehub_options)
target_compile_features(cppversehub_options INTERFACE cxx_std_20)

if(CPPVERSEHUB_ENABLE_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_supported OUTPUT ipo_message)
    if(ipo_supported)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
    else()
        message(WARNING "LTO requested but not supported: ${ipo_message}")
    endif()
endif()

# ---------------------------------------------------------------------------
# Source tree
# ---------------------------------------------------------------------------
add_subdirectory(src)

if(CPPVERSEHUB_BUILD_TESTS)
    include(CTest)
    enable_testing()
    add_subdirectory(tests)
endif()

if(CPPVERSEHUB_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

if(CPPVERSEHUB_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(CPPVERSEHUB_BUILD_DOCS)
    add_subdirectory(docs)
endif()

# ---------------------------------------------------------------------------
# Installation and packaging
# ---------------------------------------------------------------------------
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/src/"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/cppversehub"
    FILES_MATCHING PATTERN "*.hpp"
)

install(EXPORT CppVerseHubTargets
    NAMESPACE CppVerseHub::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/CppVerseHub"
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CppVerseHubConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/CppVerseHubConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/CppVerseHub"
)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/CppVerseHubConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/CppVerseHubConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/CppVerseHubConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/CppVerseHub"
)

set(CPACK_PACKAGE_VENDOR "Satvik Praveen")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_SOURCE_IGNORE_FILES "/build.*/" "/\\.git/" "/\\.cache/")
include(CPack)

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------
message(STATUS "")
message(STATUS "CppVerseHub ${PROJECT_VERSION}")
message(STATUS "  Compiler ........... ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Build type ......... ${CMAKE_BUILD_TYPE}")
message(STATUS "  Tests .............. ${CPPVERSEHUB_BUILD_TESTS}")
message(STATUS "  Benchmarks ......... ${CPPVERSEHUB_BUILD_BENCHMARKS}")
message(STATUS "  Examples ........... ${CPPVERSEHUB_BUILD_EXAMPLES}")
message(STATUS "  Docs ............... ${CPPVERSEHUB_BUILD_DOCS}")
message(STATUS "  Warnings as errors . ${CPPVERSEHUB_WARNINGS_AS_ERRORS}")
message(STATUS "  Sanitizers ......... ${CPPVERSEHUB_ENABLE_SANITIZERS} (TSan: ${CPPVERSEHUB_ENABLE_TSAN})")
message(STATUS "  Coverage ........... ${CPPVERSEHUB_ENABLE_COVERAGE}")
message(STATUS "  LTO ................ ${CPPVERSEHUB_ENABLE_LTO}")
message(STATUS "")
