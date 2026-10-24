cmake_minimum_required(VERSION 3.14)
project(debug)

include(${CMAKE_CURRENT_LIST_DIR}/../cmake/EnishiLibrary.cmake)
include(FetchContent)

FetchContent_Declare(FOUNDATION SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../foundation)
FetchContent_MakeAvailable(FOUNDATION)

set(SOURCES
    src/debug/debug.h
    src/debug/log/logger.h
    src/debug/log/logger.cpp
    src/debug/log/log_entry.h
    src/debug/log/log_entry.cpp
    src/debug/log/log_level.h
    src/debug/log/log_level.cpp
)

enishi_add_library(${PROJECT_NAME} ${SOURCES})
# The alias avoids CMake's reserved link keyword "debug".
add_library(enishi::debug ALIAS debug)
enishi_configure_library(${PROJECT_NAME}
    INCLUDE_DIRECTORIES src
    PUBLIC_DEPENDENCIES foundation
)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
endif()

if(USE_VULKAN)
    if(NOT TARGET Vulkan::Vulkan)
        find_package(Vulkan REQUIRED)
    endif()
    target_sources(${PROJECT_NAME} PRIVATE
        src/debug/vulkan/vulkan_diagnostics.h
        src/debug/vulkan/vulkan_diagnostics.cpp
    )
    target_link_libraries(${PROJECT_NAME} PUBLIC Vulkan::Vulkan)
endif()
