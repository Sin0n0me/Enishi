include("${CMAKE_CURRENT_LIST_DIR}/TranspileShaders.cmake")
function(enishi_prepare_opengl_shaders source_directory output_directory)
    enishi_transpile_shaders(enishi_opengl_shaders "${source_directory}" "${output_directory}" opengl)
endfunction()
