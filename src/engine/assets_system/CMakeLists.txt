cmake_minimum_required (VERSION 3.14)
project(assets_system)

include(${CMAKE_CURRENT_LIST_DIR}/../cmake/EnishiLibrary.cmake)

# 依存設定
set(DEPENDENCIES)
set(ASSET_DEPENDENT_LIBS "${CMAKE_BINARY_DIR}/libs")
include(FetchContent)

FetchContent_Declare(
    ENGINE_TYPES
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../engine_types
)
FetchContent_MakeAvailable(ENGINE_TYPES)

FetchContent_Declare(
    FOUNDATION
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../foundation
)
FetchContent_MakeAvailable(FOUNDATION)


set(NOTHINGS_STD_SOURCE_DIR "${ASSET_DEPENDENT_LIBS}/nothings_std")
FetchContent_Declare(
    NOTHINGS_STD
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG "master"
    GIT_SHALLOW TRUE
    GIT_PROGRESS ON
    SOURCE_DIR ${NOTHINGS_STD_SOURCE_DIR}
)
FetchContent_MakeAvailable(NOTHINGS_STD)


FetchContent_Declare(
    MINI_AUDIO
    GIT_REPOSITORY https://github.com/mackron/miniaudio.git
    GIT_TAG "0.11.25"
    GIT_SHALLOW TRUE
    GIT_PROGRESS ON
)
FetchContent_MakeAvailable(MINI_AUDIO)

set(DEPENDENCIES
    PUBLIC miniaudio
    PUBLIC foundation
    PUBLIC engine_types
    PUBLIC ${ASSET_DEPENDENT_LIBS}
)

#
# 以降は触らなくて良い
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
enishi_configure_library(${PROJECT_NAME}
    INCLUDE_DIRECTORIES src ${ASSET_DEPENDENT_LIBS}
    PUBLIC_DEPENDENCIES ${DEPENDENCIES}
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

    # テスト用exeにメインのライブラリ(lib)をリンクする
    #target_link_libraries(tests PRIVATE my_lib)

    # CTestにテストケースを登録
    add_test(NAME RunAllTests COMMAND tests)
endif()
