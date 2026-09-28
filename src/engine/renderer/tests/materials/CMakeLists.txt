cmake_minimum_required(VERSION 3.20)
project(material_tests LANGUAGES CXX)
find_path(GLM_INCLUDE_DIR glm/glm.hpp REQUIRED)
find_path(ICU_INCLUDE_DIR unicode/unistr.h REQUIRED)
set(ENGINE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../..")
add_executable(material_tests material_tests.cpp
    "${ENGINE_ROOT}/renderer/src/renderer/common/converter/model_to_mesh.cpp"
    "${ENGINE_ROOT}/renderer/src/renderer/common/converter/skinned_vertices.cpp"
    "${ENGINE_ROOT}/foundation/src/foundation/str/string_builder.cpp")
target_sources(material_tests PRIVATE
    "${ENGINE_ROOT}/engine_types/src/engine_types/renderer/uniform_buffer/material.cpp"
    "${ENGINE_ROOT}/core/src/core/system/animation/morph_weights.cpp"
    "${ENGINE_ROOT}/core/src/core/system/animation/material_morph.cpp"
    "${ENGINE_ROOT}/core/src/core/system/render/material_morph_upload.cpp"
    "${ENGINE_ROOT}/renderer/src/renderer/common/vertex_buffer_updater.cpp"
    material_morph_tests.cpp)
target_compile_features(material_tests PRIVATE cxx_std_23)
foreach(module renderer engine_types foundation platform core component)
    target_include_directories(material_tests PRIVATE "${ENGINE_ROOT}/${module}/src")
endforeach()
target_include_directories(material_tests PRIVATE "${GLM_INCLUDE_DIR}" "${ICU_INCLUDE_DIR}")
if(MSVC)
    target_compile_options(material_tests PRIVATE /utf-8 /W4)
endif()
enable_testing()
add_test(NAME material_tests COMMAND material_tests)
