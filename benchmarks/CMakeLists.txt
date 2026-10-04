# Micro-benchmarks (Google Benchmark). Each subdirectory registers its own
# executable through cppversehub_add_benchmark().
file(GLOB bench_dirs LIST_DIRECTORIES true RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/*")
list(SORT bench_dirs)
foreach(dir IN LISTS bench_dirs)
    if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${dir}" AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${dir}/CMakeLists.txt")
        add_subdirectory(${dir})
    endif()
endforeach()

# `cmake --build build --target run_benchmarks` runs every benchmark and
# writes JSON reports to <build>/benchmark-results/.
get_property(all_benchmarks GLOBAL PROPERTY CPPVERSEHUB_BENCHMARKS)
set(bench_commands "")
foreach(bench IN LISTS all_benchmarks)
    list(APPEND bench_commands
        COMMAND $<TARGET_FILE:${bench}>
            --benchmark_out=${CMAKE_BINARY_DIR}/benchmark-results/${bench}.json
            --benchmark_out_format=json
            --benchmark_min_time=0.05s)
endforeach()
add_custom_target(run_benchmarks
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/benchmark-results"
    ${bench_commands}
    DEPENDS ${all_benchmarks}
    COMMENT "Running all micro-benchmarks"
    VERBATIM
)
