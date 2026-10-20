function(enishi_prepare_opengl_shaders source_directory output_directory)
    file(MAKE_DIRECTORY "${output_directory}")
    file(GLOB shader_sources CONFIGURE_DEPENDS "${source_directory}/*.glsl")
    foreach(source IN LISTS shader_sources)
        get_filename_component(name "${source}" NAME)
        configure_file("${source}" "${output_directory}/${name}" COPYONLY)
    endforeach()
    # Both passes use exactly the same deformation code; only clip-space projection differs.
    file(READ "${source_directory}/vs_model_gl.glsl" shadow_vertex)
    string(REPLACE "#version 400 core"
        "#version 400 core
#define ENISHI_SHADOW_PASS"
        shadow_vertex "${shadow_vertex}")
    file(GENERATE OUTPUT "${output_directory}/vs_shadow_map_gl.glsl" CONTENT "${shadow_vertex}")
endfunction()
