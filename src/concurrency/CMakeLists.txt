# File: src/concurrency/CMakeLists.txt
# Concurrency Module Build Configuration

# Add concurrency library
add_library(concurrency
    ThreadPool.cpp
    AsyncComms.cpp
    AsyncMissions.cpp
    MutexExamples.cpp
    ConditionVariables.cpp
    Atomics.cpp
    CoroutinesDemo.cpp
)

# Set target properties
set_target_properties(concurrency PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

# Link with core library and threading
target_link_libraries(concurrency
    PRIVATE
        core
    PUBLIC
        ${CMAKE_THREAD_LIBS_INIT}
)

# Include directories
target_include_directories(concurrency
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)

# Find required threading package
find_package(Threads REQUIRED)
target_link_libraries(concurrency PUBLIC Threads::Threads)

# Compiler-specific options
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(concurrency PRIVATE
        -Wall -Wextra -Wpedantic
        -pthread
        -Wno-unused-parameter
    )
endif()

# Enable Thread Sanitizer for debug builds
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(concurrency PRIVATE -fsanitize=thread)
        target_link_options(concurrency PRIVATE -fsanitize=thread)
    endif()
endif()

# Install targets
install(TARGETS concurrency
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

# Install headers
install(FILES
    ThreadPool.hpp
    AsyncComms.hpp
    AsyncMissions.hpp
    MutexExamples.hpp
    ConditionVariables.hpp
    Atomics.hpp
    CoroutinesDemo.hpp
    DESTINATION include/concurrency
)