include_guard(GLOBAL)
if(NOT TARGET enishi_test_support)
    add_subdirectory("${CMAKE_CURRENT_LIST_DIR}" "${CMAKE_BINARY_DIR}/enishi-test-support")
endif()
