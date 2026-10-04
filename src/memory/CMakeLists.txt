# memory module: custom allocators, memory pools, RAII and smart pointers
cppversehub_add_module(
    NAME  memory
    ALIAS Memory
    SOURCES
    CustomAllocators.cpp
    Demo.cpp
    MemoryPools.cpp
    RAII_Examples.cpp
    SmartPointers.cpp
)
