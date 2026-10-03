cmake_minimum_required(VERSION 3.20)
project(skinning_tests LANGUAGES CXX)
find_path(GLM_INCLUDE_DIR glm/glm.hpp REQUIRED)
find_path(ICU_INCLUDE_DIR unicode/unistr.h REQUIRED)
set(ENGINE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../..")
add_executable(skinning_tests skinning_tests.cpp
    "${ENGINE_ROOT}/renderer/src/renderer/common/converter/skinned_vertices.cpp")
target_compile_features(skinning_tests PRIVATE cxx_std_23)
target_include_directories(skinning_tests PRIVATE
    "${ENGINE_ROOT}/renderer/src" "${ENGINE_ROOT}/engine_types/src"
    "${ENGINE_ROOT}/foundation/src" "${GLM_INCLUDE_DIR}" "${ICU_INCLUDE_DIR}")
target_compile_definitions(skinning_tests PRIVATE NOMINMAX
    SHADER_ROOT="${ENGINE_ROOT}/../shader")
if(MSVC)
    target_compile_options(skinning_tests PRIVATE /utf-8 /W4)
endif()
target_link_libraries(skinning_tests PRIVATE d3d11 d3dcompiler dxguid)
enable_testing()
add_test(NAME skinning_tests COMMAND skinning_tests)
