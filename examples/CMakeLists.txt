# Example programs. Every top-level *.cpp in this directory becomes an
# executable named example_<stem> linked against the full library set.
file(GLOB example_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
list(SORT example_sources)

foreach(source IN LISTS example_sources)
    get_filename_component(stem "${source}" NAME_WE)
    cppversehub_add_executable(
        NAME    example_${stem}
        SOURCES "${source}"
        DEPENDS cppversehub
    )
    set_target_properties(example_${stem} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/examples")
    # Every example doubles as a smoke test: it must run to completion.
    if(CPPVERSEHUB_BUILD_TESTS)
        add_test(NAME example.${stem} COMMAND example_${stem})
        set_tests_properties(example.${stem} PROPERTIES LABELS "example" TIMEOUT 120)
    endif()
endforeach()
