cmake_minimum_required(VERSION 3.16)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(enishi_crack_shader_tests LANGUAGES NONE)
    enable_testing()
endif()

if(NOT WIN32 AND NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "A graphics test context provider is required for ${CMAKE_SYSTEM_NAME}")
endif()

find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(ENISHI_TEST_SHADER_DIRECTORY "${CMAKE_BINARY_DIR}/shader/opengl" CACHE PATH
    "Directory containing the generated production GLSL shaders")
add_test(NAME opengl_crack_shaders COMMAND "${Python3_EXECUTABLE}" -B
    "${CMAKE_CURRENT_LIST_DIR}/crack_shader_tests.py" "${ENISHI_TEST_SHADER_DIRECTORY}")
set_tests_properties(opengl_crack_shaders PROPERTIES TIMEOUT 60 LABELS "render;shader;opengl")
add_test(NAME opengl_test_context COMMAND "${Python3_EXECUTABLE}" -B
    "${CMAKE_CURRENT_LIST_DIR}/../../../renderer/tests/skinning/gl_context_tests.py")
set_tests_properties(opengl_test_context PROPERTIES TIMEOUT 60 LABELS "render;context;opengl")
