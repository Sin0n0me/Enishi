add_executable(opengl_shadow_tests opengl_shadow_tests.cpp)
add_dependencies(opengl_shadow_tests enishi_opengl_shaders)
target_compile_features(opengl_shadow_tests PRIVATE cxx_std_23)
target_compile_definitions(opengl_shadow_tests PRIVATE NOMINMAX)
target_link_libraries(opengl_shadow_tests PRIVATE renderer render_pass core platform_impl SDL3::SDL3)
add_test(NAME opengl_shadow_tests COMMAND opengl_shadow_tests)
set_tests_properties(opengl_shadow_tests PROPERTIES WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../../../effect/tests/shader"
    "${CMAKE_CURRENT_BINARY_DIR}/crack-shaders")

if(WIN32)
    add_custom_command(TARGET opengl_shadow_tests POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "$<TARGET_FILE:SDL3::SDL3>" "$<TARGET_FILE_DIR:opengl_shadow_tests>")
endif()
find_package(Python3 REQUIRED COMPONENTS Interpreter)
add_test(NAME opengl_shadow_shaders COMMAND "${Python3_EXECUTABLE}" -B
    "${CMAKE_CURRENT_LIST_DIR}/../skinning/gl_shadow_tests.py"
    "${CMAKE_BINARY_DIR}/shader/opengl")
add_test(NAME opengl_skinning_shaders COMMAND "${Python3_EXECUTABLE}" -B
    "${CMAKE_CURRENT_LIST_DIR}/../skinning/gl_skinning_tests.py"
    "${CMAKE_BINARY_DIR}/shader/opengl")
set_tests_properties(opengl_shadow_shaders opengl_skinning_shaders PROPERTIES TIMEOUT 60)
