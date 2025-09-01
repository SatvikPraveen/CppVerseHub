# File: src/utils/CMakeLists.txt
# Utility Classes and Helpers Module

# Set minimum required version
cmake_minimum_required(VERSION 3.16)

# Create utilities library
add_library(utils_lib STATIC
    Logger.cpp
    FileParser.cpp
    ConfigManager.cpp
    MathUtils.cpp
    StringUtils.cpp
    TimeUtils.cpp
)

# Set C++17 standard for utilities
target_compile_features(utils_lib PUBLIC cxx_std_17)

# Include directories
target_include_directories(utils_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

# Find and link required packages
find_package(Threads REQUIRED)

# Link with system libraries
target_link_libraries(utils_lib 
    PUBLIC 
        Threads::Threads
    PRIVATE
        $<$<PLATFORM_ID:Linux>:stdc++fs>  # Filesystem library for older GCC
)

# Compiler-specific options
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(utils_lib PRIVATE 
        -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion
        -O3 -march=native
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(utils_lib PRIVATE 
        -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion
        -O3 -march=native
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(utils_lib PRIVATE 
        /W4 /O2 /GL
    )
    target_compile_definitions(utils_lib PRIVATE
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
endif()

# Platform-specific definitions
if(WIN32)
    target_compile_definitions(utils_lib PRIVATE CPPVERSEHUB_WINDOWS)
elseif(UNIX AND NOT APPLE)
    target_compile_definitions(utils_lib PRIVATE CPPVERSEHUB_LINUX)
elseif(APPLE)
    target_compile_definitions(utils_lib PRIVATE CPPVERSEHUB_MACOS)
endif()

# Debug/Release specific settings
target_compile_definitions(utils_lib PRIVATE
    $<$<CONFIG:Debug>:CPPVERSEHUB_DEBUG>
    $<$<CONFIG:Release>:CPPVERSEHUB_RELEASE>
    $<$<CONFIG:RelWithDebInfo>:CPPVERSEHUB_RELEASE_WITH_DEBUG>
)

# Set library properties
set_target_properties(utils_lib PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

# Export symbols for shared library builds
if(BUILD_SHARED_LIBS)
    target_compile_definitions(utils_lib PRIVATE BUILDING_UTILS_LIB)
    set_target_properties(utils_lib PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS ON
    )
endif()

# Install targets
install(TARGETS utils_lib
    EXPORT CppVerseHubTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

# Install headers
install(FILES
    Logger.hpp
    FileParser.hpp
    ConfigManager.hpp
    MathUtils.hpp
    StringUtils.hpp
    TimeUtils.hpp
    DESTINATION include/CppVerseHub/utils
)

# Add subdirectory to parent if needed
if(CMAKE_PROJECT_NAME STREQUAL "CppVerseHub")
    message(STATUS "Configuring Utilities module")
    
    # Optional: Enable testing for utilities
    if(BUILD_TESTING)
        enable_testing()
        add_subdirectory(tests OPTIONAL)
    endif()
endif()