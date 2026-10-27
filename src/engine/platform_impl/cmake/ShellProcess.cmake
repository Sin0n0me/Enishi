include_guard(GLOBAL)
include("${CMAKE_CURRENT_LIST_DIR}/../../platform/cmake/ShellProcess.cmake")
if(WIN32)
    set(enishi_shell_process_source windows/windows_shell_process.cpp)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(enishi_shell_process_source linux/linux_shell_process.cpp)
else()
    message(FATAL_ERROR "Shell processes require Windows or Linux")
endif()
add_library(platform_impl_shell STATIC
    "${CMAKE_CURRENT_LIST_DIR}/../src/platform_impl/process/shell_socket.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../src/platform_impl/process/shell_process_factory.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../src/platform_impl/process/${enishi_shell_process_source}")
target_include_directories(platform_impl_shell PUBLIC "${CMAKE_CURRENT_LIST_DIR}/../src")
target_link_libraries(platform_impl_shell PUBLIC platform_shell)
if(MSVC)
    target_compile_options(platform_impl_shell PRIVATE /utf-8 /EHsc /W4)
endif()

if(WIN32)
    target_link_libraries(platform_impl_shell PRIVATE ws2_32)
endif()
