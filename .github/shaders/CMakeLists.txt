cmake_minimum_required(VERSION 3.24)
project(enishi_ci_shaders LANGUAGES C CXX)
get_filename_component(repository "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
include("${repository}/src/engine/cmake/TranspileShaders.cmake")
foreach(backend IN ITEMS opengl vulkan)
    enishi_transpile_shaders(${backend}_shaders "${repository}/src/shader/hlsl"
        "${CMAKE_CURRENT_BINARY_DIR}/${backend}" ${backend})
endforeach()
add_custom_target(portable_shaders ALL DEPENDS opengl_shaders vulkan_shaders)
