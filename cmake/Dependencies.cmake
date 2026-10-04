# Third-party dependencies.
#
# Every dependency is resolved with `find_package` first (system or vcpkg/Conan
# install) and fetched from upstream only as a fallback, so the project builds
# both in hermetic CI and on a developer workstation without network access.
include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

# nlohmann/json - scenario serialisation and configuration files
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.12.0
    GIT_SHALLOW    TRUE
    FIND_PACKAGE_ARGS 3.11 NAMES nlohmann_json
)
set(JSON_BuildTests OFF CACHE INTERNAL "")
set(JSON_Install ON CACHE INTERNAL "")  # fetched copy must be installable: core exports a dependency on it
FetchContent_MakeAvailable(nlohmann_json)

if(CPPVERSEHUB_BUILD_TESTS)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.8.1
        GIT_SHALLOW    TRUE
        FIND_PACKAGE_ARGS 3.4 NAMES Catch2
    )
    set(CATCH_INSTALL_DOCS OFF CACHE INTERNAL "")
    set(CATCH_INSTALL_EXTRAS OFF CACHE INTERNAL "")
    FetchContent_MakeAvailable(Catch2)
    if(catch2_SOURCE_DIR)
        list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")
    endif()
    include(Catch)
endif()

if(CPPVERSEHUB_BUILD_BENCHMARKS)
    FetchContent_Declare(
        benchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG        v1.9.4
        GIT_SHALLOW    TRUE
        FIND_PACKAGE_ARGS 1.8 NAMES benchmark
    )
    set(BENCHMARK_ENABLE_TESTING OFF CACHE INTERNAL "")
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE INTERNAL "")
    set(BENCHMARK_ENABLE_WERROR OFF CACHE INTERNAL "")
    set(BENCHMARK_ENABLE_INSTALL OFF CACHE INTERNAL "")
    set(BENCHMARK_INSTALL_DOCS OFF CACHE INTERNAL "")
    FetchContent_MakeAvailable(benchmark)
endif()
