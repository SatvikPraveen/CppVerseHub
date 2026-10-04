# Library modules. Each directory defines exactly one library target via
# cppversehub_add_module() and is independently buildable and testable.
#
# Dependency order (leaf modules first):
#   utils -> core -> {patterns, stl_showcase, concurrency, memory, algorithms, modern, templates}
add_subdirectory(utils)
add_subdirectory(core)
add_subdirectory(templates)
add_subdirectory(patterns)
add_subdirectory(stl_showcase)
add_subdirectory(memory)
add_subdirectory(concurrency)
add_subdirectory(modern)
add_subdirectory(algorithms)

# Umbrella target that links every module.
add_library(cppversehub INTERFACE)
add_library(CppVerseHub::CppVerseHub ALIAS cppversehub)
get_property(all_modules GLOBAL PROPERTY CPPVERSEHUB_MODULES)
target_link_libraries(cppversehub INTERFACE ${all_modules})
set_target_properties(cppversehub PROPERTIES EXPORT_NAME "CppVerseHub")
install(TARGETS cppversehub EXPORT CppVerseHubTargets)

# Interactive showcase executable.
cppversehub_add_executable(
    NAME    cppversehub_cli
    SOURCES main.cpp
    DEPENDS cppversehub
    INSTALL
)
set_target_properties(cppversehub_cli PROPERTIES OUTPUT_NAME "cppversehub")
