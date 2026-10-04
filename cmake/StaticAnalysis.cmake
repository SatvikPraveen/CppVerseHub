# Optional integration of clang-tidy and cppcheck into the build.
if(CPPVERSEHUB_ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES clang-tidy clang-tidy-18 clang-tidy-17)
    if(CLANG_TIDY_EXE)
        set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE};--extra-arg=-Wno-unknown-warning-option" CACHE STRING "" FORCE)
        message(STATUS "clang-tidy enabled: ${CLANG_TIDY_EXE}")
    else()
        message(WARNING "CPPVERSEHUB_ENABLE_CLANG_TIDY is ON but clang-tidy was not found")
    endif()
endif()

if(CPPVERSEHUB_ENABLE_CPPCHECK)
    find_program(CPPCHECK_EXE NAMES cppcheck)
    if(CPPCHECK_EXE)
        set(CMAKE_CXX_CPPCHECK
            "${CPPCHECK_EXE}"
            --enable=warning,style,performance,portability
            --std=c++20
            --inline-suppr
            --suppress=missingIncludeSystem
            --suppress=unmatchedSuppression
            --quiet
            CACHE STRING "" FORCE)
        message(STATUS "cppcheck enabled: ${CPPCHECK_EXE}")
    else()
        message(WARNING "CPPVERSEHUB_ENABLE_CPPCHECK is ON but cppcheck was not found")
    endif()
endif()
