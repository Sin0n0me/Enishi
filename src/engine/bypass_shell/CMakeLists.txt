cmake_minimum_required(VERSION 3.20)
project(bypass_shell LANGUAGES CXX)
find_package(Threads REQUIRED)
include(${CMAKE_CURRENT_LIST_DIR}/../platform/cmake/ShellProcess.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../platform_impl/cmake/ShellProcess.cmake)
add_library(bypass_shell STATIC
    src/bypass_shell/command_parser.cpp
    src/bypass_shell/shell_connection.cpp
    src/bypass_shell/shell_monitor.cpp
    src/bypass_shell/shell_process.cpp)
target_include_directories(bypass_shell PUBLIC src)
target_compile_features(bypass_shell PUBLIC cxx_std_23)
target_link_libraries(bypass_shell PUBLIC Threads::Threads platform_shell PRIVATE platform_impl_shell)
add_executable(enishi-shell cli/main.cpp)
set_target_properties(enishi-shell PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
target_link_libraries(enishi-shell PRIVATE bypass_shell platform_impl_shell)
if(MSVC)
    target_compile_options(bypass_shell PRIVATE /utf-8 /EHsc /W4)
    target_compile_options(enishi-shell PRIVATE /utf-8 /EHsc /W4)
endif()
