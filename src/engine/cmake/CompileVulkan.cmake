function(enishi_compile_vulkan_shaders target_name source_directory output_directory)
    file(GLOB shader_sources CONFIGURE_DEPENDS "${source_directory}/*.glsl")
    set(outputs)
    foreach(source IN LISTS shader_sources)
        get_filename_component(name "${source}" NAME_WE)
        if(name MATCHES "^vs_")
            set(stage vert)
        elseif(name MATCHES "^ps_")
            set(stage frag)
        else()
            message(FATAL_ERROR "Unknown Vulkan shader stage: ${source}")
        endif()
        set(output "${output_directory}/${name}.spv")
        add_custom_command(OUTPUT "${output}"
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${output_directory}"
            COMMAND Vulkan::glslc --target-env=vulkan1.1 "-fshader-stage=${stage}" "${source}" -o "${output}"
            DEPENDS "${source}"
            VERBATIM)
        list(APPEND outputs "${output}")
    endforeach()
    # Compile the same skinning source for both projections, preventing deformation drift.
    set(shadow_output "${output_directory}/vs_shadow_map_vk.spv")
    add_custom_command(OUTPUT "${shadow_output}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${output_directory}"
        COMMAND Vulkan::glslc --target-env=vulkan1.1 -fshader-stage=vert -DENISHI_SHADOW_PASS
            "${source_directory}/vs_model_vk.glsl" -o "${shadow_output}"
        DEPENDS "${source_directory}/vs_model_vk.glsl"
        VERBATIM)
    list(APPEND outputs "${shadow_output}")
    add_custom_target(${target_name} DEPENDS ${outputs})
endfunction()
