# Reuse the application's dependency versions without building the application itself.
include(FetchContent)
FetchContent_GetProperties(GLM SOURCE_DIR GLM_INCLUDE_DIR)
FetchContent_GetProperties(NOTHINGS_STD SOURCE_DIR enishi_stb_directory)
get_filename_component(STB_INCLUDE_DIR "${enishi_stb_directory}" DIRECTORY)
FetchContent_GetProperties(BULLET3 SOURCE_DIR enishi_bullet_directory)
set(BULLET_INCLUDE_DIR "${enishi_bullet_directory}/src")
find_package(ICU REQUIRED COMPONENTS uc data)
set(ICU_INCLUDE_DIR "${ICU_INCLUDE_DIRS}")
set(ICU_UC_LIBRARY ICU::uc)

foreach(suite foundation/tests/headers foundation/tests/option foundation/tests/result assets_system/tests/pmd assets_system/tests/pmx
        assets_system/tests/vrm assets_system/tests/vrm_json
        core/tests/morphs renderer/tests/materials animation/tests/runtime
        core/tests/bone_layers core/tests/physics skinning_system/tests/ik)
    add_subdirectory("../${suite}" "${suite}")
endforeach()

add_custom_target(enishi_common_tests DEPENDS
    foundation_header_checks foundation_option_tests foundation_result_tests
    pmd_morph_tests pmx_tests vrm_tests vrm_json_tests vertex_morph_tests
    material_tests animation_runtime_tests bone_layer_tests physics_runtime_tests ik_tests)

if(WIN32)
    add_subdirectory(../renderer/tests/skinning renderer/tests/skinning)
    add_dependencies(enishi_common_tests skinning_tests shadow_tests)
endif()
