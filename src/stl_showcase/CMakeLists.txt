# File: src/stl_showcase/CMakeLists.txt
# CppVerseHub - STL Showcase Module CMake Configuration

# Add STL showcase library
add_library(stl_showcase STATIC
    Containers.cpp
    Algorithms.cpp
    Iterators.cpp
    Functors.cpp
    STLUtilities.cpp
)

# Include directories
target_include_directories(stl_showcase PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/core
)

# Link with core library
target_link_libraries(stl_showcase PUBLIC core)

# Set C++ standard
target_compile_features(stl_showcase PUBLIC cxx_std_20)

# Compiler-specific options
if(MSVC)
    target_compile_options(stl_showcase PRIVATE /W4 /wd4996)
else()
    target_compile_options(stl_showcase PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Export include directories for other modules
target_include_directories(stl_showcase INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)