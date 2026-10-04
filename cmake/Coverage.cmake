# Source-level code coverage (gcov / llvm-cov) for GCC and Clang.
function(cppversehub_enable_coverage target)
    if(NOT CPPVERSEHUB_ENABLE_COVERAGE)
        return()
    endif()

    if(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang|GNU")
        message(STATUS "Code coverage instrumentation enabled")
        target_compile_options(${target} INTERFACE --coverage -O0 -g -fno-inline)
        target_link_options(${target} INTERFACE --coverage)
    else()
        message(WARNING "Coverage is only supported with GCC or Clang")
    endif()
endfunction()
