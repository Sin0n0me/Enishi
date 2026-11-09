add_executable(collider_test collider_test.cpp)
target_link_libraries(collider_test PRIVATE collider)
target_compile_features(collider_test PRIVATE cxx_std_23)
add_test(NAME ColliderTests COMMAND collider_test)
set_tests_properties(ColliderTests PROPERTIES TIMEOUT 120)

include("${CMAKE_CURRENT_LIST_DIR}/../../test_support/test_support.cmake")
target_link_libraries(collider_test PRIVATE enishi_test_support)
