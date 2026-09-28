cmake_minimum_required(VERSION 3.20)
project(vertex_morph_tests LANGUAGES CXX)
find_path(GLM_INCLUDE_DIR glm/glm.hpp REQUIRED)
find_path(ICU_INCLUDE_DIR unicode/unistr.h REQUIRED)
set(ENGINE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../..")
add_executable(vertex_morph_tests vertex_morph_tests.cpp morph_weight_tests.cpp
    "${ENGINE_ROOT}/core/src/core/system/animation/vertex_morph.cpp"
    "${ENGINE_ROOT}/core/src/core/system/animation/morph_weights.cpp"
    "${ENGINE_ROOT}/core/src/core/system/animation/bone_morph.cpp"
    "${ENGINE_ROOT}/core/src/core/system/render/vertex_morph_upload.cpp"
    "${ENGINE_ROOT}/renderer/src/renderer/common/vertex_buffer_updater.cpp"
    "${ENGINE_ROOT}/model_controller/src/model_controller/model_component_builder.cpp"
    "${ENGINE_ROOT}/engine_types/src/engine_types/handle/renderer/render_handle.cpp")
target_compile_features(vertex_morph_tests PRIVATE cxx_std_23)
foreach(module core component model_controller engine_types foundation platform renderer)
    target_include_directories(vertex_morph_tests PRIVATE "${ENGINE_ROOT}/${module}/src")
endforeach()
target_include_directories(vertex_morph_tests PRIVATE "${GLM_INCLUDE_DIR}" "${ICU_INCLUDE_DIR}")
if(MSVC)
    target_compile_options(vertex_morph_tests PRIVATE /utf-8 /W4)
endif()
enable_testing()
add_test(NAME vertex_morph_tests COMMAND vertex_morph_tests)
