cmake_minimum_required(VERSION 3.20)
project(physics_runtime_tests LANGUAGES CXX)
enable_testing()
add_subdirectory("../bone_layers" bone-layers EXCLUDE_FROM_ALL)
get_target_property(BONE_SOURCES bone_layer_tests SOURCES)
get_target_property(BONE_INCLUDES bone_layer_tests INCLUDE_DIRECTORIES)
list(FILTER BONE_SOURCES INCLUDE REGEX "[/\\\\]src[/\\\\]")
set(ENGINE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../..")
find_path(BULLET_INCLUDE_DIR btBulletDynamicsCommon.h REQUIRED)
find_library(BULLET_DYNAMICS_LIBRARY NAMES BulletDynamics_Debug BulletDynamics REQUIRED)
find_library(BULLET_COLLISION_LIBRARY NAMES BulletCollision_Debug BulletCollision REQUIRED)
find_library(BULLET_MATH_LIBRARY NAMES LinearMath_Debug LinearMath REQUIRED)
file(GLOB_RECURSE PHYSICS_SOURCES CONFIGURE_DEPENDS "${ENGINE_ROOT}/physics/src/*.cpp")
add_executable(physics_runtime_tests physics_runtime_tests.cpp ${BONE_SOURCES} ${PHYSICS_SOURCES}
    "${ENGINE_ROOT}/platform_impl/src/platform_impl/physics/physics_config.cpp"
    "${ENGINE_ROOT}/core/src/core/system/physics/physics_system.cpp"
    "${ENGINE_ROOT}/engine_types/src/engine_types/handle/physics/physics_handle.cpp")
target_compile_features(physics_runtime_tests PRIVATE cxx_std_23)
target_include_directories(physics_runtime_tests PRIVATE ${BONE_INCLUDES} "${BULLET_INCLUDE_DIR}"
    "${ENGINE_ROOT}/physics/src" "${ENGINE_ROOT}/platform_impl/src")
target_link_libraries(physics_runtime_tests PRIVATE
    "${BULLET_DYNAMICS_LIBRARY}" "${BULLET_COLLISION_LIBRARY}" "${BULLET_MATH_LIBRARY}")
if(MSVC)
    target_compile_options(physics_runtime_tests PRIVATE /utf-8 /W4)
endif()
add_test(NAME physics_runtime_tests COMMAND physics_runtime_tests)
