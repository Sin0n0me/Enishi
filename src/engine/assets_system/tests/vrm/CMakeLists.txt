cmake_minimum_required(VERSION 3.20)
project(vrm_tests LANGUAGES CXX)

find_path(GLM_INCLUDE_DIR glm/glm.hpp REQUIRED)
find_path(ICU_INCLUDE_DIR unicode/unistr.h REQUIRED)
find_path(STB_INCLUDE_DIR nothings_std/stb_image.h REQUIRED)
set(ENGINE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../..")
set(ASSETS_ROOT "${ENGINE_ROOT}/assets_system/src/assets_system")
set(VRM_ROOT "${ASSETS_ROOT}/model/model_loader/vrm")
add_executable(vrm_tests vrm_tests.cpp vrm_fixture.cpp
    "${VRM_ROOT}/vrm_model_loader.cpp"
    "${VRM_ROOT}/vrm_json.cpp"
    "${VRM_ROOT}/vrm_context.cpp"
    "${VRM_ROOT}/vrm_mesh.cpp"
    "${VRM_ROOT}/vrm_material.cpp"
    "${VRM_ROOT}/vrm_expression.cpp"
    "${VRM_ROOT}/vrm_expression_uv.cpp"
    "${VRM_ROOT}/vrm_spring.cpp"
    "${ASSETS_ROOT}/utility/binary_reader.cpp"
    "${ASSETS_ROOT}/texture/texture_loader.cpp")
target_compile_features(vrm_tests PRIVATE cxx_std_23)
foreach(module assets_system engine_types foundation)
    target_include_directories(vrm_tests PRIVATE "${ENGINE_ROOT}/${module}/src")
endforeach()
target_include_directories(vrm_tests PRIVATE
    "${GLM_INCLUDE_DIR}" "${ICU_INCLUDE_DIR}" "${STB_INCLUDE_DIR}")
target_include_directories(vrm_tests SYSTEM PRIVATE "${ENGINE_ROOT}/../../third_party/cgltf")
if(MSVC)
    target_compile_options(vrm_tests PRIVATE /utf-8 /W4 /EHsc)
endif()
enable_testing()
add_test(NAME vrm_tests COMMAND vrm_tests)
