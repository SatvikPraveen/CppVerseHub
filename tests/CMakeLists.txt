# Test suite. Every subdirectory that contains a CMakeLists.txt registers its own
# Catch2 executable through cppversehub_add_test(); this file only discovers them
# so modules can be added without touching shared build files.
file(GLOB module_test_dirs LIST_DIRECTORIES true RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/*")
list(SORT module_test_dirs)
foreach(dir IN LISTS module_test_dirs)
    if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${dir}" AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${dir}/CMakeLists.txt")
        add_subdirectory(${dir})
    endif()
endforeach()
