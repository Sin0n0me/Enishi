add_executable(vulkan_render_tests vulkan_render_tests.cpp render_pass_test.cpp shadow_render_test.cpp)
target_compile_features(vulkan_render_tests PRIVATE cxx_std_23)
target_compile_definitions(vulkan_render_tests PRIVATE NOMINMAX
    VULKAN_SHADER_DIRECTORY="${CMAKE_BINARY_DIR}/shader/vulkan")
target_link_libraries(vulkan_render_tests PRIVATE renderer render_pass core platform_impl enishi_imgui SDL3::SDL3 Vulkan::Vulkan)
add_dependencies(vulkan_render_tests enishi_vulkan_shaders)
add_custom_command(TARGET vulkan_render_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL3::SDL3> $<TARGET_FILE_DIR:vulkan_render_tests>)
add_test(NAME vulkan_render_tests COMMAND vulkan_render_tests)
set_tests_properties(vulkan_render_tests PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
