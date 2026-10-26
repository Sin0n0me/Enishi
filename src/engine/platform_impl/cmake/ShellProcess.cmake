include_guard(GLOBAL)
include("${CMAKE_CURRENT_LIST_DIR}/../../platform/cmake/ShellProcess.cmake")
add_library(platform_impl_shell STATIC
    "${CMAKE_CURRENT_LIST_DIR}/../src/platform_impl/process/native_shell_process.cpp")
target_include_directories(platform_impl_shell PUBLIC "${CMAKE_CURRENT_LIST_DIR}/../src")
target_link_libraries(platform_impl_shell PUBLIC platform_shell)
if(MSVC)
    target_compile_options(platform_impl_shell PRIVATE /utf-8 /EHsc /W4)
endif()
