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

enishi_configure_library(${PROJECT_NAME}
    INCLUDE_DIRECTORIES src
    LINK_DEPENDENCIES ${DEPENDENCIES}
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
