cmake_minimum_required(VERSION 3.20)
project(pmd_morph_tests LANGUAGES CXX)
find_path(GLM_INCLUDE_DIR glm/glm.hpp REQUIRED)
find_path(ICU_INCLUDE_DIR unicode/unistr.h REQUIRED)
find_library(ICU_UC_LIBRARY NAMES icuuc REQUIRED)
set(ENGINE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../..")
add_executable(pmd_morph_tests pmd_morph_tests.cpp
    "${ENGINE_ROOT}/assets_system/src/assets_system/model/model_loader/pmd/pmd_to_model_data.cpp"
    "${ENGINE_ROOT}/assets_system/src/assets_system/model/bone/bone_resolver.cpp"
    "${ENGINE_ROOT}/assets_system/src/assets_system/model/bone/bone_name_map.cpp"
    "${ENGINE_ROOT}/foundation/src/foundation/log/logger.cpp"
    "${ENGINE_ROOT}/foundation/src/foundation/log/log_entry.cpp"
    "${ENGINE_ROOT}/foundation/src/foundation/log/log_level.cpp"
    "${ENGINE_ROOT}/assets_system/src/assets_system/model/model_loader/pmd/pmd_morph_converter.cpp"
    "${ENGINE_ROOT}/foundation/src/foundation/str/to_utf8.cpp")
target_compile_features(pmd_morph_tests PRIVATE cxx_std_23)
foreach(module assets_system engine_types foundation)
    target_include_directories(pmd_morph_tests PRIVATE "${ENGINE_ROOT}/${module}/src")
endforeach()
target_include_directories(pmd_morph_tests PRIVATE "${GLM_INCLUDE_DIR}" "${ICU_INCLUDE_DIR}")
target_link_libraries(pmd_morph_tests PRIVATE "${ICU_UC_LIBRARY}")
if(MSVC)
    target_compile_options(pmd_morph_tests PRIVATE /utf-8 /W4)
endif()
enable_testing()
add_test(NAME pmd_morph_tests COMMAND pmd_morph_tests)
