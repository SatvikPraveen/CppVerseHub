# Helper functions that keep every module, test and benchmark target consistent.
#
# Include layout: the project include root is `src/`, so every header is included
# as  #include "<module>/<Header>.hpp"  (e.g. #include "core/Fleet.hpp").

set(CPPVERSEHUB_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/src")

# cppversehub_add_module(
#     NAME      <short name, e.g. core>          -> target cppversehub_core
#     ALIAS     <CamelCase alias, e.g. Core>     -> CppVerseHub::Core
#     [INTERFACE]                                 header-only module
#     [SOURCES   <file.cpp> ...]
#     [DEPENDS   <target> ...]                    public link dependencies
# )
function(cppversehub_add_module)
    cmake_parse_arguments(ARG "INTERFACE" "NAME;ALIAS" "SOURCES;DEPENDS" ${ARGN})
    if(NOT ARG_NAME OR NOT ARG_ALIAS)
        message(FATAL_ERROR "cppversehub_add_module requires NAME and ALIAS")
    endif()

    set(target "cppversehub_${ARG_NAME}")

    if(ARG_INTERFACE)
        add_library(${target} INTERFACE)
        target_include_directories(${target} INTERFACE
            $<BUILD_INTERFACE:${CPPVERSEHUB_INCLUDE_DIR}>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/cppversehub>
        )
        target_link_libraries(${target} INTERFACE Threads::Threads ${CPPVERSEHUB_ATOMIC_LIBRARY} ${ARG_DEPENDS})
        target_compile_features(${target} INTERFACE cxx_std_20)
    else()
        add_library(${target} STATIC ${ARG_SOURCES})
        target_include_directories(${target} PUBLIC
            $<BUILD_INTERFACE:${CPPVERSEHUB_INCLUDE_DIR}>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/cppversehub>
        )
        target_link_libraries(${target}
            PUBLIC  Threads::Threads ${CPPVERSEHUB_ATOMIC_LIBRARY} ${ARG_DEPENDS}
            PRIVATE $<BUILD_INTERFACE:cppversehub_options>
        )
        target_compile_features(${target} PUBLIC cxx_std_20)
        set_target_properties(${target} PROPERTIES
            OUTPUT_NAME "cppversehub_${ARG_NAME}"
            EXPORT_NAME "${ARG_ALIAS}"
        )
    endif()

    add_library("CppVerseHub::${ARG_ALIAS}" ALIAS ${target})

    install(TARGETS ${target}
        EXPORT CppVerseHubTargets
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    )

    set_property(GLOBAL APPEND PROPERTY CPPVERSEHUB_MODULES ${target})
endfunction()

# cppversehub_add_executable(NAME <name> SOURCES <...> [DEPENDS <...>] [INSTALL])
function(cppversehub_add_executable)
    cmake_parse_arguments(ARG "INSTALL" "NAME" "SOURCES;DEPENDS" ${ARGN})
    add_executable(${ARG_NAME} ${ARG_SOURCES})
    target_link_libraries(${ARG_NAME} PRIVATE cppversehub_options Threads::Threads ${ARG_DEPENDS})
    target_include_directories(${ARG_NAME} PRIVATE "${CPPVERSEHUB_INCLUDE_DIR}")
    if(ARG_INSTALL)
        install(TARGETS ${ARG_NAME} RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
    endif()
endfunction()

# cppversehub_add_test(NAME <name> SOURCES <...> DEPENDS <...> [LABELS <...>])
#
# Builds a Catch2 v3 executable and registers every TEST_CASE with CTest.
function(cppversehub_add_test)
    cmake_parse_arguments(ARG "" "NAME" "SOURCES;DEPENDS;LABELS" ${ARGN})
    add_executable(${ARG_NAME} ${ARG_SOURCES})
    target_link_libraries(${ARG_NAME} PRIVATE
        cppversehub_options
        Catch2::Catch2WithMain
        ${ARG_DEPENDS}
    )
    target_include_directories(${ARG_NAME} PRIVATE "${CPPVERSEHUB_INCLUDE_DIR}" "${PROJECT_SOURCE_DIR}/tests")
    set_target_properties(${ARG_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/tests")
    catch_discover_tests(${ARG_NAME}
        TEST_PREFIX "${ARG_NAME}."
        PROPERTIES LABELS "${ARG_LABELS}"
        DISCOVERY_MODE PRE_TEST
    )
endfunction()

# cppversehub_add_benchmark(NAME <name> SOURCES <...> DEPENDS <...>)
function(cppversehub_add_benchmark)
    cmake_parse_arguments(ARG "" "NAME" "SOURCES;DEPENDS" ${ARGN})
    add_executable(${ARG_NAME} ${ARG_SOURCES})
    target_link_libraries(${ARG_NAME} PRIVATE
        cppversehub_options
        benchmark::benchmark_main
        ${ARG_DEPENDS}
    )
    target_include_directories(${ARG_NAME} PRIVATE "${CPPVERSEHUB_INCLUDE_DIR}")
    set_target_properties(${ARG_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/benchmarks")
    set_property(GLOBAL APPEND PROPERTY CPPVERSEHUB_BENCHMARKS ${ARG_NAME})
endfunction()
