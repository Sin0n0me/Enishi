cmake_minimum_required(VERSION 3.20)
project(bone_layer_tests LANGUAGES CXX)
find_path(GLM_INCLUDE_DIR glm/glm.hpp REQUIRED)
find_path(ICU_INCLUDE_DIR unicode/unistr.h REQUIRED)
set(ENGINE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../..")
set(CORE_ROOT "${ENGINE_ROOT}/core/src/core")
set(PMX_ROOT "${ENGINE_ROOT}/assets_system/src/assets_system/model/model_loader/pmx")
file(GLOB SKINNING_SOURCES CONFIGURE_DEPENDS "${ENGINE_ROOT}/skinning_system/src/skinning_system/*/*.cpp")
add_executable(bone_layer_tests bone_layer_tests.cpp bone_inheritance_tests.cpp bone_morph_tests.cpp
    external_transform_tests.cpp
    ${SKINNING_SOURCES}
    "${CORE_ROOT}/system/skinning/skinning_system.cpp"
    "${CORE_ROOT}/system/skinning/skinning_physics.cpp"
    "${CORE_ROOT}/system/skinning/bone_inheritance.cpp"
    "${CORE_ROOT}/system/animation/bone_morph.cpp"
    "${CORE_ROOT}/system/animation/morph_weights.cpp"
    "${CORE_ROOT}/system/skinning/bone_view_factory.cpp"
    "${CORE_ROOT}/system/skinning/model_pose_initializer.cpp"
    "${CORE_ROOT}/system/physics/physics_body_factory.cpp"
    "${CORE_ROOT}/system/render/bone_matrix_upload.cpp"
    "${ENGINE_ROOT}/model_controller/src/model_controller/model_component_builder.cpp"
    "${ENGINE_ROOT}/ik_system/src/ik_system/ik_solver.cpp"
    "${ENGINE_ROOT}/ecs/src/ecs/registry.cpp"
    "${ENGINE_ROOT}/engine_types/src/engine_types/handle/handle_allocator.cpp"
    "${ENGINE_ROOT}/engine_types/src/engine_types/handle/renderer/render_handle.cpp"
    "${ENGINE_ROOT}/engine_types/src/engine_types/handle/physics/physics_handle.cpp"
    "${ENGINE_ROOT}/engine_types/src/engine_types/assets/model/addons/bone.cpp"
    "${ENGINE_ROOT}/foundation/src/foundation/log/logger.cpp"
    "${ENGINE_ROOT}/foundation/src/foundation/log/log_entry.cpp"
    "${ENGINE_ROOT}/foundation/src/foundation/log/log_level.cpp"
    "${PMX_ROOT}/pmx_to_model_data.cpp" "${PMX_ROOT}/pmx_validation.cpp"
    "${PMX_ROOT}/pmx_runtime_support.cpp")
target_compile_features(bone_layer_tests PRIVATE cxx_std_23)
foreach(module core component ecs platform sub_system skinning_system ik_system model_controller engine_types foundation assets_system)
    target_include_directories(bone_layer_tests PRIVATE "${ENGINE_ROOT}/${module}/src")
endforeach()
target_include_directories(bone_layer_tests PRIVATE "${GLM_INCLUDE_DIR}" "${ICU_INCLUDE_DIR}")
if(MSVC)
    target_compile_options(bone_layer_tests PRIVATE /utf-8 /W4)
endif()
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    enable_testing()
    add_test(NAME bone_layer_tests COMMAND bone_layer_tests)
endif()
