# File: src/patterns/CMakeLists.txt
# CppVerseHub - Design Patterns Module CMake Configuration

# Add patterns library
add_library(patterns STATIC
    Strategy.cpp
    Observer.cpp
    Decorator.cpp
    Adapter.cpp
    Command.cpp
    Builder.cpp
    State.cpp
)

# Include directories
target_include_directories(patterns PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/core
)

# Link with core library
target_link_libraries(patterns PUBLIC core)

# Set C++ standard
target_compile_features(patterns PUBLIC cxx_std_20)

# Compiler-specific options
if(MSVC)
    target_compile_options(patterns PRIVATE /W4)
else()
    target_compile_options(patterns PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Export include directories for other modules
target_include_directories(patterns INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)