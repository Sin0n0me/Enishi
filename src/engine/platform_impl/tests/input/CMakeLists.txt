add_executable(platform_input_tests input_tests.cpp ../../src/platform_impl/input/sdl/sdl_input.cpp)
target_compile_features(platform_input_tests PRIVATE cxx_std_23)
target_include_directories(platform_input_tests PRIVATE ../../src)
target_link_libraries(platform_input_tests PRIVATE platform engine_types foundation SDL3::Headers)
add_test(NAME platform_input_tests COMMAND platform_input_tests)
set_tests_properties(platform_input_tests PROPERTIES TIMEOUT 30)

include("${CMAKE_CURRENT_LIST_DIR}/../../../test_support/test_support.cmake")
target_link_libraries(platform_input_tests PRIVATE enishi_test_support)
