# File: examples/tutorials/CMakeLists.txt
# Tutorial Examples Build Configuration

cmake_minimum_required(VERSION 3.20)

# Tutorial examples
set(TUTORIAL_EXAMPLES
    cpp_concepts_walkthrough
    design_patterns_tutorial
    modern_cpp_features
)

# Create executables for each tutorial
foreach(TUTORIAL ${TUTORIAL_EXAMPLES})
    add_cppversehub_example(${TUTORIAL} ${TUTORIAL}.cpp)
    
    # Tutorials may need C++20/23 features
    target_compile_features(${TUTORIAL} PRIVATE cxx_std_20)
    
    # Enable additional warnings for learning
    target_compile_options(${TUTORIAL} PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wextra -Wpedantic>
        $<$<CXX_COMPILER_ID:MSVC>:/W4>
    )
    
    # Add to main examples target
    add_dependencies(examples ${TUTORIAL})
endforeach()

# Create convenience target for tutorials only
add_custom_target(tutorials
    DEPENDS ${TUTORIAL_EXAMPLES}
    COMMENT "Build tutorial examples"
)

# Add learning resources
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/README_Tutorials.md.in
    ${CMAKE_CURRENT_BINARY_DIR}/README_Tutorials.md
    @ONLY
)

message(STATUS "Configured ${list_LENGTH TUTORIAL_EXAMPLES} tutorial examples")