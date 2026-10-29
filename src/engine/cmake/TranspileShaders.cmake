include_guard(GLOBAL)

function(enishi_transpile_shaders target_name source_directory output_directory backend)
    get_filename_component(repository "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../../.." ABSOLUTE)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    find_program(ENISHI_DXC_EXECUTABLE NAMES dxc
        HINTS "$ENV{VULKAN_SDK}/Bin" "${repository}/build/vulkan-sdk/Bin" REQUIRED)
    find_program(ENISHI_SPIRV_VAL_EXECUTABLE NAMES spirv-val
        HINTS "$ENV{VULKAN_SDK}/Bin" "${repository}/build/vulkan-sdk/Bin" REQUIRED)
    set(tool_dependencies)
    set(cross_arguments)
    if(backend STREQUAL "opengl")
        if(NOT EXISTS "${repository}/tools/SPIRV-Cross/CMakeLists.txt")
            message(FATAL_ERROR "Initialize tools/SPIRV-Cross with git submodule update --init tools/SPIRV-Cross")
        endif()
        if(NOT TARGET spirv-cross)
            set(SPIRV_CROSS_CLI ON CACHE BOOL "Build the shader transpiler" FORCE)
            set(SPIRV_CROSS_ENABLE_TESTS OFF CACHE BOOL "Disable upstream shader tests" FORCE)
            set(SPIRV_CROSS_ENABLE_C_API OFF CACHE BOOL "Disable unused C API" FORCE)
            add_subdirectory("${repository}/tools/SPIRV-Cross"
                "${CMAKE_BINARY_DIR}/tools/spirv-cross" EXCLUDE_FROM_ALL)
        endif()
        list(APPEND tool_dependencies spirv-cross)
        set(cross_arguments --cross "$<TARGET_FILE:spirv-cross>")
        set(suffix _gl.glsl)
    else()
        set(suffix _vk.spv)
    endif()
    file(GLOB_RECURSE shader_includes CONFIGURE_DEPENDS "${source_directory}/common/*.hlsl")
    set(generator "${repository}/tools/transpile_shader.py")
    set(outputs)
    foreach(name IN ITEMS vs_model ps_model vs_shadow_map ps_shadow_map vs_clear_wall ps_clear_wall vs_crack ps_crack)
        set(source "${source_directory}/${name}.hlsl")
        set(output "${output_directory}/${name}${suffix}")
        add_custom_command(OUTPUT "${output}"
            COMMAND "${Python3_EXECUTABLE}" -B "${generator}" --backend "${backend}"
                --source "${source}" --output "${output}" --dxc "${ENISHI_DXC_EXECUTABLE}"
                --validator "${ENISHI_SPIRV_VAL_EXECUTABLE}" ${cross_arguments}
            DEPENDS "${source}" ${shader_includes} "${generator}"
                "${ENISHI_DXC_EXECUTABLE}" "${ENISHI_SPIRV_VAL_EXECUTABLE}" ${tool_dependencies}
            VERBATIM)
        list(APPEND outputs "${output}")
    endforeach()
    add_custom_target(${target_name} DEPENDS ${outputs})
endfunction()
