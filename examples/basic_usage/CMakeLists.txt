# File: examples/basic_usage/CMakeLists.txt
# Basic Usage Examples Build Configuration

cmake_minimum_required(VERSION 3.20)

# Basic usage examples
set(BASIC_EXAMPLES
    getting_started
    entity_creation
    simple_simulation
)

# Create executables for each basic example
foreach(EXAMPLE ${BASIC_EXAMPLES})
    add_cppversehub_example(${EXAMPLE} ${EXAMPLE}.cpp)
    
    # Add to main examples target
    add_dependencies(examples ${EXAMPLE})
endforeach()

# Create convenience target for basic examples only
add_custom_target(basic_examples
    DEPENDS ${BASIC_EXAMPLES}
    COMMENT "Build basic usage examples"
)

message(STATUS "Configured ${list_LENGTH BASIC_EXAMPLES} basic usage examples")