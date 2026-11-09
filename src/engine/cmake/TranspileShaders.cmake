include_guard(GLOBAL)

function(enishi_transpile_shaders target_name source_directory output_directory backend)
    get_filename_component(repository "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../../.." ABSOLUTE)
    set(shader_names vs_model ps_model vs_shadow_map ps_shadow_map vs_clear_wall ps_clear_wall vs_crack ps_crack)
    set(ENISHI_PRECOMPILED_SHADER_DIRECTORY "" CACHE PATH
        "Validated shader bundle from another host; must match the current shader sources")
    file(GLOB_RECURSE shader_includes CONFIGURE_DEPENDS "${source_directory}/common/*.hlsl")
    set(generator "${repository}/tools/transpile_shader.py")
    set(signature_inputs ${shader_includes} "${generator}")
    foreach(name IN LISTS shader_names)
        list(APPEND signature_inputs "${source_directory}/${name}.hlsl")
    endforeach()
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${signature_inputs})
    set(source_hashes)
    foreach(source IN LISTS signature_inputs)
        file(READ "${source}" source_contents)
        string(REPLACE "\r\n" "\n" source_contents "${source_contents}")
        string(SHA256 source_hash "${source_contents}")
        string(APPEND source_hashes "${source_hash}")
    endforeach()
    string(SHA256 source_signature "${backend}${source_hashes}")
    if(backend STREQUAL "opengl")
        set(suffix _gl.glsl)
    else()
        set(suffix _vk.spv)
    endif()
    if(ENISHI_PRECOMPILED_SHADER_DIRECTORY)
        set(bundle_manifest "${ENISHI_PRECOMPILED_SHADER_DIRECTORY}/source.sha256")
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${bundle_manifest}")
        file(READ "${bundle_manifest}" bundle_signature)
        string(STRIP "${bundle_signature}" bundle_signature)
        if(NOT bundle_signature STREQUAL source_signature)
            message(FATAL_ERROR "Precompiled shaders do not match the current shader sources and backend")
        endif()
        set(outputs)
        foreach(name IN LISTS shader_names)
            set(input "${ENISHI_PRECOMPILED_SHADER_DIRECTORY}/${name}${suffix}")
            if(NOT EXISTS "${input}")
                message(FATAL_ERROR "Missing precompiled shader: ${input}")
            endif()
            set(output "${output_directory}/${name}${suffix}")
            add_custom_command(OUTPUT "${output}"
                COMMAND "${CMAKE_COMMAND}" -E make_directory "${output_directory}"
                COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${input}" "${output}"
                DEPENDS "${input}" VERBATIM)
            list(APPEND outputs "${output}")
        endforeach()
        add_custom_target(${target_name} DEPENDS ${outputs})
        return()
    endif()
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
    endif()
    set(outputs)
    foreach(name IN LISTS shader_names)
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
    file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${target_name}-source.sha256"
        CONTENT "${source_signature}\n")
    add_custom_command(OUTPUT "${output_directory}/source.sha256"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${CMAKE_CURRENT_BINARY_DIR}/${target_name}-source.sha256" "${output_directory}/source.sha256"
        DEPENDS ${outputs} "${CMAKE_CURRENT_BINARY_DIR}/${target_name}-source.sha256" VERBATIM)
    add_custom_target(${target_name} DEPENDS ${outputs} "${output_directory}/source.sha256")
endfunction()
