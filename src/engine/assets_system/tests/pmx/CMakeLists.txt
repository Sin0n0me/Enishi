cmake_minimum_required(VERSION 3.20)
project(pmx_tests LANGUAGES CXX)

# Standalone tests use existing dependencies and never fetch packages.
find_path(GLM_INCLUDE_DIR glm/glm.hpp REQUIRED)
find_path(ICU_INCLUDE_DIR unicode/unistr.h REQUIRED)
set(ENGINE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../..")
set(ASSETS_ROOT "${ENGINE_ROOT}/assets_system/src/assets_system")
add_executable(pmx_tests
    pmx_tests.cpp
    pmx_conversion_tests.cpp
    "${ASSETS_ROOT}/model/model_loader/pmx/pmx_model_loader.cpp"
    "${ASSETS_ROOT}/model/model_loader/pmx/pmx_reader.cpp"
    "${ASSETS_ROOT}/model/model_loader/pmx/pmx_validation.cpp"
    "${ASSETS_ROOT}/model/model_loader/pmx/pmx_to_model_data.cpp"
    "${ASSETS_ROOT}/utility/binary_reader.cpp"
)
target_compile_features(pmx_tests PRIVATE cxx_std_23)
target_include_directories(pmx_tests PRIVATE
    "${ENGINE_ROOT}/assets_system/src"
    "${ENGINE_ROOT}/engine_types/src"
    "${ENGINE_ROOT}/foundation/src"
    "${GLM_INCLUDE_DIR}" "${ICU_INCLUDE_DIR}"
)
if(MSVC)
    target_compile_options(pmx_tests PRIVATE /utf-8 /W4)
endif()
enable_testing()
add_test(NAME pmx_tests COMMAND pmx_tests)
