cmake_minimum_required(VERSION 3.20)
project(ik_tests LANGUAGES CXX)
find_path(GLM_INCLUDE_DIR glm/glm.hpp REQUIRED)
set(ENGINE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../..")
add_executable(ik_tests ik_tests.cpp
    "${ENGINE_ROOT}/ik_system/src/ik_system/ik_solver.cpp"
    "${ENGINE_ROOT}/skinning_system/src/skinning_system/cache/ik_bone_cache.cpp"
    "${ENGINE_ROOT}/skinning_system/src/skinning_system/cache/bind_bone_cache.cpp"
    "${ENGINE_ROOT}/skinning_system/src/skinning_system/views/ik_bone_view.cpp"
    "${ENGINE_ROOT}/skinning_system/src/skinning_system/views/bind_bone_view.cpp"
    "${ENGINE_ROOT}/skinning_system/src/skinning_system/updater/ik_bones_updater.cpp"
    "${ENGINE_ROOT}/engine_types/src/engine_types/assets/model/addons/bone.cpp")
target_compile_features(ik_tests PRIVATE cxx_std_23)
foreach(module ik_system skinning_system engine_types foundation sub_system)
    target_include_directories(ik_tests PRIVATE "${ENGINE_ROOT}/${module}/src")
endforeach()
target_include_directories(ik_tests PRIVATE "${GLM_INCLUDE_DIR}")
if(MSVC)
    target_compile_options(ik_tests PRIVATE /utf-8 /W4)
endif()
enable_testing()
add_test(NAME ik_tests COMMAND ik_tests)
