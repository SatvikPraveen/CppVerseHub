# File: examples/advanced_usage/CMakeLists.txt
# Advanced Usage Examples Build Configuration

cmake_minimum_required(VERSION 3.20)

# Advanced usage examples
set(ADVANCED_EXAMPLES
    custom_missions
    plugin_systems
    performance_optimization
)

# Create executables for each advanced example
foreach(EXAMPLE ${ADVANCED_EXAMPLES})
    add_cppversehub_example(${EXAMPLE} ${EXAMPLE}.cpp)
    
    # Advanced examples may need additional compiler flags
    target_compile_options(${EXAMPLE} PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-mavx2 -mfma>  # Enable AVX2 and FMA for SIMD
        $<$<CXX_COMPILER_ID:MSVC>:/arch:AVX2>
    )
    
    # Add to main examples target
    add_dependencies(examples ${EXAMPLE})
endforeach()

# Create convenience target for advanced examples only
add_custom_target(advanced_examples
    DEPENDS ${ADVANCED_EXAMPLES}
    COMMENT "Build advanced usage examples"
)

message(STATUS "Configured ${list_LENGTH ADVANCED_EXAMPLES} advanced usage examples")