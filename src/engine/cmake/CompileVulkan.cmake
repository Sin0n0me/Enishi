include("${CMAKE_CURRENT_LIST_DIR}/TranspileShaders.cmake")
function(enishi_compile_vulkan_shaders target_name source_directory output_directory)
    enishi_transpile_shaders(${target_name} "${source_directory}" "${output_directory}" vulkan)
endfunction()
