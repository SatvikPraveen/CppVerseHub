# Sanitizer support for GCC/Clang builds.
#
# CPPVERSEHUB_ENABLE_SANITIZERS -> AddressSanitizer + UndefinedBehaviorSanitizer
# CPPVERSEHUB_ENABLE_TSAN       -> ThreadSanitizer (cannot be combined with ASan)
function(cppversehub_enable_sanitizers target)
    if(MSVC)
        if(CPPVERSEHUB_ENABLE_SANITIZERS)
            target_compile_options(${target} INTERFACE /fsanitize=address)
        endif()
        return()
    endif()

    if(NOT CMAKE_CXX_COMPILER_ID MATCHES ".*Clang|GNU")
        return()
    endif()

    set(sanitizers "")

    if(CPPVERSEHUB_ENABLE_SANITIZERS AND CPPVERSEHUB_ENABLE_TSAN)
        message(FATAL_ERROR "ThreadSanitizer cannot be combined with AddressSanitizer")
    endif()

    if(CPPVERSEHUB_ENABLE_SANITIZERS)
        list(APPEND sanitizers address undefined)
    endif()

    if(CPPVERSEHUB_ENABLE_TSAN)
        list(APPEND sanitizers thread)
    endif()

    if(sanitizers)
        list(JOIN sanitizers "," sanitizer_list)
        message(STATUS "Sanitizers enabled: ${sanitizer_list}")
        target_compile_options(${target} INTERFACE
            -fsanitize=${sanitizer_list}
            -fno-omit-frame-pointer
            -fno-sanitize-recover=all
        )
        target_link_options(${target} INTERFACE -fsanitize=${sanitizer_list})
    endif()
endfunction()
