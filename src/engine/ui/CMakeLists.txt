cmake_minimum_required (VERSION 3.14)
project(ui)

include(${CMAKE_CURRENT_LIST_DIR}/../cmake/EnishiLibrary.cmake)

# 以下依存設定
set(DEPENDENCIES)
set(UI_DEPENDENT_LIBS "${CMAKE_BINARY_DIR}/libs")

# 外部
include(FetchContent)

FetchContent_Declare(
    ImGui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG "v1.92.8"
    GIT_SHALLOW TRUE
    GIT_PROGRESS ON
    SOURCE_DIR "${UI_DEPENDENT_LIBS}/imgui"
)
FetchContent_MakeAvailable(ImGui)

FetchContent_Declare(
    PLATFORM
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../platform
)
FetchContent_MakeAvailable(PLATFORM)

set(DEPENDENCIES
    platform
    engine_types
    foundation
    PUBLIC ${UI_DEPENDENT_LIBS}
)

#
# 以降は触らなくて良い(include汚染対策で他と少し設定が違う)
#

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
target_sources(${PROJECT_NAME} PRIVATE
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
)
if(USE_OPENGL40)
    target_sources(${PROJECT_NAME} PRIVATE ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_OPENGL40)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${CMAKE_DL_LIBS})
else()
    target_sources(${PROJECT_NAME} PRIVATE ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp)
    target_link_libraries(${PROJECT_NAME} PRIVATE d3d11 d3dcompiler)
endif()
target_include_directories(${PROJECT_NAME} PRIVATE ${imgui_SOURCE_DIR})
enishi_configure_library(${PROJECT_NAME}
    INCLUDE_DIRECTORIES src
    PUBLIC_DEPENDENCIES platform sub_system engine_types foundation
    DEPENDENCIES SDL3::SDL3
)

# MSVCコンパイラのホットリロードを有効に
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()


# 以下テスト用設定
option(BUILD_TESTING OFF)

if(BUILD_TESTING)
    enable_testing()

    # テスト用実行ファイルの生成
    file(GLOB TEST_SOURCES
        "tests/*.h"
        "tests/*.cpp" 
        "tests/*.hpp" 
        "tests/*.cpp"
    )

    add_executable(${PROJECT_NAME}_test ${SOURCES} ${TEST_SOURCES})

    # CTestにテストケースを登録
    add_test(NAME RunAllTests COMMAND tests)
endif()
