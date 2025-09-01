# File: src/templates/CMakeLists.txt
# Build configuration for template programming showcase

cmake_minimum_required(VERSION 3.20)

# Set C++20 standard for concepts and modern template features
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Template library name
set(TEMPLATES_LIB_NAME cppversehub_templates)

# Template header files
set(TEMPLATE_HEADERS
    GenericContainers.hpp
    MetaProgramming.hpp
    SFINAE_Examples.hpp
    VariadicTemplates.hpp
    ConceptsDemo.hpp
    TemplateSpecialization.hpp
)

# Create interface library for header-only templates
add_library(${TEMPLATES_LIB_NAME} INTERFACE)

# Set include directories
target_include_directories(${TEMPLATES_LIB_NAME} INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

# Set compile features and options
target_compile_features(${TEMPLATES_LIB_NAME} INTERFACE cxx_std_20)

# Compiler-specific options
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(${TEMPLATES_LIB_NAME} INTERFACE
        -Wall -Wextra -Wpedantic
        -Wconversion -Wsign-conversion
        -Wold-style-cast
        -fconcepts-diagnostics-depth=2
        -ftemplate-depth=1024
        -ftemplate-backtrace-limit=64
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(${TEMPLATES_LIB_NAME} INTERFACE
        -Wall -Wextra -Wpedantic
        -Wconversion -Wsign-conversion
        -Wold-style-cast
        -ftemplate-depth=1024
        -ftemplate-backtrace-limit=64
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(${TEMPLATES_LIB_NAME} INTERFACE
        /W4
        /permissive-
        /Zc:__cplusplus
        /constexpr:depth1024
    )
endif()

# Enable template instantiation debugging (optional)
option(TEMPLATES_DEBUG_INSTANTIATION "Enable template instantiation debugging" OFF)
if(TEMPLATES_DEBUG_INSTANTIATION)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${TEMPLATES_LIB_NAME} INTERFACE -ftemplate-debug)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(${TEMPLATES_LIB_NAME} INTERFACE -ftemplate-debug)
    endif()
endif()

# Template unit tests (if testing is enabled)
option(BUILD_TEMPLATE_TESTS "Build template unit tests" ON)
if(BUILD_TEMPLATE_TESTS AND BUILD_TESTING)
    # Test executable
    add_executable(template_tests
        tests/test_generic_containers.cpp
        tests/test_meta_programming.cpp
        tests/test_sfinae_examples.cpp
        tests/test_variadic_templates.cpp
        tests/test_concepts_demo.cpp
        tests/test_template_specialization.cpp
        tests/template_test_main.cpp
    )
    
    # Link template library
    target_link_libraries(template_tests PRIVATE ${TEMPLATES_LIB_NAME})
    
    # Add to test suite
    add_test(NAME TemplateTests COMMAND template_tests)
    
    # Set test properties
    set_tests_properties(TemplateTests PROPERTIES
        TIMEOUT 120
        LABELS "templates;unit"
    )
endif()

# Template benchmarks (optional)
option(BUILD_TEMPLATE_BENCHMARKS "Build template performance benchmarks" OFF)
if(BUILD_TEMPLATE_BENCHMARKS)
    find_package(benchmark QUIET)
    if(benchmark_FOUND)
        add_executable(template_benchmarks
            benchmarks/container_benchmarks.cpp
            benchmarks/meta_programming_benchmarks.cpp
            benchmarks/template_benchmark_main.cpp
        )
        
        target_link_libraries(template_benchmarks PRIVATE 
            ${TEMPLATES_LIB_NAME}
            benchmark::benchmark
        )
        
        # Install benchmarks
        install(TARGETS template_benchmarks
            RUNTIME DESTINATION bin/benchmarks
        )
    else()
        message(WARNING "Google Benchmark not found. Template benchmarks will not be built.")
    endif()
endif()

# Install template headers
install(FILES ${TEMPLATE_HEADERS}
    DESTINATION include/cppversehub/templates
    COMPONENT templates-dev
)

# Create template documentation target
option(BUILD_TEMPLATE_DOCS "Build template documentation" OFF)
if(BUILD_TEMPLATE_DOCS)
    find_package(Doxygen QUIET)
    if(DOXYGEN_FOUND)
        # Doxygen configuration
        set(DOXYGEN_PROJECT_NAME "CppVerseHub Templates")
        set(DOXYGEN_PROJECT_BRIEF "Advanced C++ Template Programming Showcase")
        set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/docs)
        set(DOXYGEN_INPUT ${CMAKE_CURRENT_SOURCE_DIR})
        set(DOXYGEN_FILE_PATTERNS "*.hpp")
        set(DOXYGEN_EXTRACT_ALL YES)
        set(DOXYGEN_EXTRACT_PRIVATE YES)
        set(DOXYGEN_EXTRACT_STATIC YES)
        set(DOXYGEN_GENERATE_TREEVIEW YES)
        set(DOXYGEN_HAVE_DOT YES)
        set(DOXYGEN_DOT_IMAGE_FORMAT svg)
        set(DOXYGEN_TEMPLATE_RELATIONS YES)
        set(DOXYGEN_COLLABORATION_GRAPH YES)
        
        doxygen_add_docs(template_docs
            ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "Generating template documentation"
        )
        
        # Add docs to all target
        add_dependencies(docs template_docs)
    else()
        message(WARNING "Doxygen not found. Template documentation will not be built.")
    endif()
endif()

# Template examples executable
option(BUILD_TEMPLATE_EXAMPLES "Build template examples" ON)
if(BUILD_TEMPLATE_EXAMPLES)
    add_executable(template_examples
        examples/template_examples_main.cpp
        examples/containers_example.cpp
        examples/meta_programming_example.cpp
        examples/sfinae_example.cpp
        examples/variadic_example.cpp
        examples/concepts_example.cpp
        examples/specialization_example.cpp
    )
    
    target_link_libraries(template_examples PRIVATE ${TEMPLATES_LIB_NAME})
    
    # Install examples
    install(TARGETS template_examples
        RUNTIME DESTINATION bin/examples
        COMPONENT examples
    )
endif()

# Precompiled headers for faster compilation
option(USE_TEMPLATE_PCH "Use precompiled headers for templates" OFF)
if(USE_TEMPLATE_PCH)
    target_precompile_headers(${TEMPLATES_LIB_NAME} INTERFACE
        <type_traits>
        <concepts>
        <utility>
        <memory>
        <functional>
        <tuple>
        <variant>
        <optional>
        <algorithm>
        <numeric>
        <iterator>
        <ranges>
    )
endif()

# Template compilation statistics
option(COLLECT_TEMPLATE_STATS "Collect template compilation statistics" OFF)
if(COLLECT_TEMPLATE_STATS)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${TEMPLATES_LIB_NAME} INTERFACE 
            -ftime-report
            -fmem-report
        )
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(${TEMPLATES_LIB_NAME} INTERFACE 
            -ftime-trace
            -ftime-trace-granularity=500
        )
    endif()
endif()

# Export template library for use by other projects
export(TARGETS ${TEMPLATES_LIB_NAME}
    FILE ${CMAKE_BINARY_DIR}/TemplatesTargets.cmake
)

# Create package configuration files
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    ${CMAKE_BINARY_DIR}/TemplatesConfigVersion.cmake
    VERSION 1.0.0
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/TemplatesConfig.cmake.in
    ${CMAKE_BINARY_DIR}/TemplatesConfig.cmake
    INSTALL_DESTINATION lib/cmake/Templates
)

# Install package configuration
install(FILES 
    ${CMAKE_BINARY_DIR}/TemplatesConfig.cmake
    ${CMAKE_BINARY_DIR}/TemplatesConfigVersion.cmake
    DESTINATION lib/cmake/Templates
    COMPONENT templates-dev
)

# Install targets
install(TARGETS ${TEMPLATES_LIB_NAME}
    EXPORT TemplatesTargets
    COMPONENT templates-dev
)

install(EXPORT TemplatesTargets
    FILE TemplatesTargets.cmake
    DESTINATION lib/cmake/Templates
    COMPONENT templates-dev
)

# Summary
message(STATUS "Template Programming Showcase Configuration:")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Tests: ${BUILD_TEMPLATE_TESTS}")
message(STATUS "  Build Examples: ${BUILD_TEMPLATE_EXAMPLES}")
message(STATUS "  Build Docs: ${BUILD_TEMPLATE_DOCS}")
message(STATUS "  Build Benchmarks: ${BUILD_TEMPLATE_BENCHMARKS}")
message(STATUS "  Use PCH: ${USE_TEMPLATE_PCH}")
message(STATUS "  Debug Instantiation: ${TEMPLATES_DEBUG_INSTANTIATION}")
message(STATUS "  Collect Stats: ${COLLECT_TEMPLATE_STATS}")