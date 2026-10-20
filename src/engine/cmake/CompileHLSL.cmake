function(enishi_compile_hlsl_shaders target_name source_directory output_directory)
    file(GLOB sdk_bins "C:/Program Files (x86)/Windows Kits/10/bin/*/x64")
    list(SORT sdk_bins COMPARE NATURAL ORDER DESCENDING)
    find_program(ENISHI_FXC_EXECUTABLE NAMES fxc
        HINTS "$ENV{WindowsSdkVerBinPath}/x64" ${sdk_bins}
        REQUIRED)
    file(GLOB_RECURSE shader_includes CONFIGURE_DEPENDS "${source_directory}/common/*.hlsl")
    set(outputs)
    foreach(name IN ITEMS vs_model ps_model vs_shadow_map ps_shadow_map vs_clear_wall ps_clear_wall)
        if(name MATCHES "^vs_")
            set(profile vs_5_0)
        else()
            set(profile ps_5_0)
        endif()
        set(source "${source_directory}/${name}.hlsl")
        set(output "${output_directory}/${name}.dxbc")
        add_custom_command(OUTPUT "${output}"
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${output_directory}"
            COMMAND "${ENISHI_FXC_EXECUTABLE}" /nologo /T "${profile}" /E main
                /Fo "${output}" "${source}"
            DEPENDS "${source}" ${shader_includes}
            VERBATIM)
        list(APPEND outputs "${output}")
    endforeach()
    add_custom_target(${target_name} DEPENDS ${outputs})
endfunction()
