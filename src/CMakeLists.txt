cmake_minimum_required (VERSION 3.12)
project(enishi_engine)

# MSVCコンパイラの場合のみ、ソースコードと実行バイナリの文字セットをUTF-8に設定
if(MSVC)
    add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:/utf-8>")
    add_compile_options("$<$<COMPILE_LANGUAGE:C>:/utf-8>")
    #add_compile_options("/GS")
    #add_compile_options("/fsanitize=address")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS 
    "application.h"
    "application.cpp"
    "settings.h"
)
set(DEPEND_DIR ${CMAKE_CURRENT_SOURCE_DIR}/engine)

add_executable(${PROJECT_NAME} ${SOURCES})

#
# フラグ設定
#
set(USE_SDL3 ON)
if(WIN32)
    option(USE_OPENGL40 "Use the OpenGL 4.0 renderer" OFF)
else()
    option(USE_OPENGL40 "Use the OpenGL 4.0 renderer" ON)
endif()

if(USE_OPENGL40)
    set(USE_DIRECTX_11 OFF)
else()
    set(USE_DIRECTX_11 ON)
endif()

#
# 以下依存設定
#
include(FetchContent)

FetchContent_Declare(
    CORE
    SOURCE_DIR ${DEPEND_DIR}/core
)
FetchContent_MakeAvailable(CORE)

FetchContent_Declare(
    SUB_SYSTEM
    SOURCE_DIR ${DEPEND_DIR}/sub_system
)
FetchContent_MakeAvailable(SUB_SYSTEM)

FetchContent_Declare(
    PLATFORM_IMPL
    SOURCE_DIR ${DEPEND_DIR}/platform_impl
)
FetchContent_MakeAvailable(PLATFORM_IMPL)

FetchContent_Declare(
    RENDERER
    SOURCE_DIR ${DEPEND_DIR}/renderer
)
FetchContent_MakeAvailable(RENDERER)

FetchContent_Declare(
    RENDER_PASS
    SOURCE_DIR ${DEPEND_DIR}/render_pass
)
FetchContent_MakeAvailable(RENDER_PASS)

FetchContent_MakeAvailable(RENDERER)

FetchContent_Declare(
    MODEL_CONTROLLER
    SOURCE_DIR ${DEPEND_DIR}/model_controller
)
FetchContent_MakeAvailable(MODEL_CONTROLLER)

FetchContent_Declare(
    PHYSICS
    SOURCE_DIR ${DEPEND_DIR}/physics
)
FetchContent_MakeAvailable(PHYSICS)

FetchContent_Declare(
    UI
    SOURCE_DIR ${DEPEND_DIR}/ui
)
FetchContent_MakeAvailable(UI)

FetchContent_Declare(
    SCRIPT
    SOURCE_DIR ${DEPEND_DIR}/script
)
FetchContent_MakeAvailable(SCRIPT)

set(DEPENDENCIES
    PRIVATE core
    PRIVATE platform
    PRIVATE sub_system
    PRIVATE platform_impl
    PRIVATE physics
    PRIVATE skinning_system
    PRIVATE model_controller
    PRIVATE renderer
    PRIVATE render_pass
    PRIVATE script
    PRIVATE ui
    PRIVATE engine_types
    PRIVATE foundation
)

#
# 以降は触らなくて良い
#

target_include_directories(${PROJECT_NAME} PUBLIC ./)

target_link_libraries(${PROJECT_NAME} ${DEPENDENCIES})

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:SDL3::SDL3>
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
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
    add_executable(${PROJECT_NAME} ${SOURCES} ${TEST_SOURCES})

    # テスト用exeにメインのライブラリ(lib)をリンクする
    target_link_libraries(tests PRIVATE my_lib)

    # CTestにテストケースを登録
    add_test(NAME RunAllTests COMMAND tests)
endif()
