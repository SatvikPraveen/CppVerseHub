# File: src/memory/CMakeLists.txt
# Memory Management Module Build Configuration

# Add memory management library
add_library(memory_management
    SmartPointers.cpp
    CustomAllocators.cpp
    RAII_Examples.cpp
    MemoryPools.cpp
)

# Set target properties
set_target_properties(memory_management PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

# Link with core library
target_link_libraries(memory_management
    PRIVATE
        core
)

# Include directories
target_include_directories(memory_management
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)

# Compiler-specific options
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(memory_management PRIVATE
        -Wall -Wextra -Wpedantic
        -Wno-unused-parameter
        -fno-rtti  # Disable RTTI for performance testing
    )
endif()

# Enable AddressSanitizer for debug builds
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(memory_management PRIVATE -fsanitize=address)
        target_link_options(memory_management PRIVATE -fsanitize=address)
    endif()
endif()

# Install targets
install(TARGETS memory_management
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

# Install headers
install(FILES
    SmartPointers.hpp
    CustomAllocators.hpp
    RAII_Examples.hpp
    MemoryPools.hpp
    DESTINATION include/memory
)