cmake_minimum_required(VERSION 3.20)
project(animation_runtime_tests LANGUAGES CXX)
find_path(GLM_INCLUDE_DIR glm/glm.hpp REQUIRED)
find_path(ICU_INCLUDE_DIR unicode/unistr.h REQUIRED)
find_library(ICU_UC_LIBRARY NAMES icuuc REQUIRED)
set(ENGINE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../..")
add_executable(animation_runtime_tests animation_runtime_tests.cpp vmd_expression_tests.cpp
    "${ENGINE_ROOT}/animation/src/animation/converter/vmd/vmd_converter.cpp"
    "${ENGINE_ROOT}/animation/src/animation/converter/vmd/vmd_morph_tracks.cpp"
    "${ENGINE_ROOT}/animation/src/animation/converter/vmd/vmd_ik_tracks.cpp"
    "${ENGINE_ROOT}/assets_system/src/assets_system/model/morph/morph_resolver.cpp"
    "${ENGINE_ROOT}/assets_system/src/assets_system/model/morph/morph_name_map.cpp"
    "${ENGINE_ROOT}/animation/src/animation/clip_data/interpolation/vmd/vmd_interpolation_data.cpp"
    "${ENGINE_ROOT}/assets_system/src/assets_system/model/bone/bone_resolver.cpp"
    "${ENGINE_ROOT}/assets_system/src/assets_system/model/bone/bone_name_map.cpp"
    "${ENGINE_ROOT}/foundation/src/foundation/str/to_utf8.cpp"
    "${ENGINE_ROOT}/foundation/src/foundation/log/logger.cpp"
    "${ENGINE_ROOT}/foundation/src/foundation/log/log_entry.cpp"
    "${ENGINE_ROOT}/foundation/src/foundation/log/log_level.cpp")
target_compile_features(animation_runtime_tests PRIVATE cxx_std_23)
foreach(module animation assets_system engine_types foundation)
    target_include_directories(animation_runtime_tests PRIVATE "${ENGINE_ROOT}/${module}/src")
endforeach()
target_include_directories(animation_runtime_tests PRIVATE "${GLM_INCLUDE_DIR}" "${ICU_INCLUDE_DIR}")
target_link_libraries(animation_runtime_tests PRIVATE "${ICU_UC_LIBRARY}")
if(MSVC)
    target_compile_options(animation_runtime_tests PRIVATE /utf-8 /W4)
endif()
enable_testing()
add_test(NAME animation_runtime_tests COMMAND animation_runtime_tests)
