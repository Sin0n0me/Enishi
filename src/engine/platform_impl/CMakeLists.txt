cmake_minimum_required (VERSION 3.14)
project(platform_impl)

include(${CMAKE_CURRENT_LIST_DIR}/../cmake/EnishiLibrary.cmake)

# MSVCコンパイラの場合のみ、ソースコードと実行バイナリの文字セットをUTF-8に設定
if(MSVC)
    add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:/utf-8>")
    add_compile_options("$<$<COMPILE_LANGUAGE:C>:/utf-8>")
endif()

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS 
    "src/*.h"
    "src/*.c"
    "src/*.hpp"
    "src/*.cpp"
)

enishi_add_library(${PROJECT_NAME} ${SOURCES})


# 以下依存設定
set(DEPENDENCIES)
include(FetchContent)

FetchContent_Declare(ENISHI_DEBUG SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../debug)
FetchContent_MakeAvailable(ENISHI_DEBUG)
include(ExternalProject)

FetchContent_Declare(
    PLATFORM
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../platform
)
FetchContent_MakeAvailable(PLATFORM)

FetchContent_Declare(
    SUB_SYSTEM
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../sub_system
)
FetchContent_MakeAvailable(SUB_SYSTEM)

set(DEPENDENCIES
    platform
    sub_system
    foundation
    engine_types
)

# 各プラットフォームに応じた使用ライブラリの変更
if(USE_SDL3)
    target_compile_definitions(${PROJECT_NAME} PUBLIC "USE_SDL3")

    set(SDL_TESTS OFF CACHE BOOL "Disable SDL3 tests" FORCE)
    set(SDL_TESTS_BUILD OFF CACHE BOOL "Disable SDL3 test builds" FORCE)
    set(SDL_TEST_LIBRARY OFF CACHE BOOL "Disable SDL3 test library" FORCE)
    set(SDL_INSTALL_TESTS OFF CACHE BOOL "Do not install SDL3 tests" FORCE)
    FetchContent_Declare(
        SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG "release-3.4.10"
        GIT_SHALLOW TRUE
        GIT_PROGRESS ON
    )
    FetchContent_MakeAvailable(SDL3)

    set(DEPENDENCIES 
        ${DEPENDENCIES} 
        PUBLIC SDL3::SDL3
    )
elseif(USE_WINDOWS)

elseif(USE_X11)

elseif(USE_WAYLAND)

else()

endif()

#
# 以降は触らなくて良い
#

FetchContent_Declare(
    UI
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../ui
)
FetchContent_MakeAvailable(UI)

FetchContent_Declare(
    RENDERER
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../renderer
)
FetchContent_MakeAvailable(RENDERER)

FetchContent_GetProperties(ImGui SOURCE_DIR IMGUI_SOURCE_DIR)
target_sources(${PROJECT_NAME} PRIVATE ${IMGUI_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE ui renderer enishi_imgui effect render_pass)
if(USE_VULKAN)
    target_sources(${PROJECT_NAME} PRIVATE ${IMGUI_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp)
    target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan)
elseif(USE_OPENGL40)
    target_sources(${PROJECT_NAME} PRIVATE ${IMGUI_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_OPENGL40)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${CMAKE_DL_LIBS})
else()
    target_sources(${PROJECT_NAME} PRIVATE ${IMGUI_SOURCE_DIR}/backends/imgui_impl_dx11.cpp)
    target_link_libraries(${PROJECT_NAME} PRIVATE d3d11 d3dcompiler)
endif()

enishi_configure_library(${PROJECT_NAME}
    INCLUDE_DIRECTORIES src
    LINK_DEPENDENCIES ${DEPENDENCIES}
)

# MSVCコンパイラのホットリロードを有効に
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC enishi::debug)
