include_guard(GLOBAL)
include("${CMAKE_CURRENT_LIST_DIR}/../../platform/cmake/ShellProcess.cmake")
if(WIN32)
    set(enishi_native_shell_sources windows/windows_shell_process.cpp
        windows/windows_terminal_input.cpp windows/windows_shell_socket.cpp)
elseif(APPLE OR CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(enishi_native_shell_sources posix/posix_shell_process.cpp
        posix/posix_terminal_input.cpp posix/posix_shell_terminal.cpp)
    if(APPLE)
        list(APPEND enishi_native_shell_sources macos/macos_process_io.cpp macos/macos_shell_socket.cpp)
    else()
        list(APPEND enishi_native_shell_sources linux/linux_process_io.cpp linux/linux_shell_socket.cpp)
    endif()
else()
    message(FATAL_ERROR "Shell processes require Windows, Linux or macOS")
endif()
list(TRANSFORM enishi_native_shell_sources PREPEND "${CMAKE_CURRENT_LIST_DIR}/../src/platform_impl/process/")
add_library(platform_impl_shell STATIC
    ${enishi_native_shell_sources}
    "${CMAKE_CURRENT_LIST_DIR}/../src/platform_impl/process/shell_socket.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../src/platform_impl/process/terminal_input.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../src/platform_impl/process/shell_terminal.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../src/platform_impl/process/shell_process_factory.cpp")
target_include_directories(platform_impl_shell PUBLIC "${CMAKE_CURRENT_LIST_DIR}/../src")
target_link_libraries(platform_impl_shell PUBLIC platform_shell)
if(MSVC)
    target_compile_options(platform_impl_shell PRIVATE /utf-8 /EHsc /W4)
endif()

if(WIN32)
    target_link_libraries(platform_impl_shell PRIVATE ws2_32)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(platform_impl_shell PRIVATE util)
endif()
