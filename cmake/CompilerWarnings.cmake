# Attach a curated, compiler-specific warning set to an INTERFACE target.
#
# Usage: cppversehub_set_warnings(<target> <warnings_as_errors:BOOL>)
function(cppversehub_set_warnings target warnings_as_errors)
    set(msvc_warnings
        /W4
        /permissive-
        /w14242 # conversion with possible loss of data
        /w14254 # operator: conversion from larger to smaller bit field
        /w14263 # member function does not override any base class virtual
        /w14265 # class has virtual functions but destructor is not virtual
        /w14287 # unsigned/negative constant mismatch
        /w14296 # expression is always true/false
        /w14311 # pointer truncation
        /w14545 /w14546 /w14547 /w14549 /w14555 # suspicious expressions
        /w14619 # pragma warning: there is no warning number
        /w14640 # thread-unsafe static member initialisation
        /w14826 # sign-extended conversion
        /w14905 /w14906 # wide/narrow string literal cast
        /w14928 # illegal copy-initialisation
        /Zc:__cplusplus
        /utf-8
    )

    set(clang_gcc_warnings
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wnon-virtual-dtor
        -Wold-style-cast
        -Wcast-align
        -Wunused
        -Woverloaded-virtual
        -Wnull-dereference
        -Wdouble-promotion
        -Wformat=2
        -Wimplicit-fallthrough
    )

    set(gcc_only_warnings
        -Wmisleading-indentation
        -Wduplicated-cond
        -Wduplicated-branches
        -Wlogical-op
        -Wuseless-cast
    )

    if(warnings_as_errors)
        list(APPEND clang_gcc_warnings -Werror)
        list(APPEND msvc_warnings /WX)
    endif()

    if(MSVC)
        set(project_warnings ${msvc_warnings})
    elseif(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
        set(project_warnings ${clang_gcc_warnings})
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(project_warnings ${clang_gcc_warnings} ${gcc_only_warnings})
    else()
        message(AUTHOR_WARNING "No warning set known for compiler '${CMAKE_CXX_COMPILER_ID}'")
    endif()

    target_compile_options(${target} INTERFACE ${project_warnings})
endfunction()
