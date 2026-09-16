cmake_minimum_required (VERSION 3.14)
project(model_controller)

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

add_library(${PROJECT_NAME} ${SOURCES})

# 以下依存設定

set(DEPENDENCIES)
include(FetchContent)

FetchContent_Declare(
    ANIMATION
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../animation
)
FetchContent_MakeAvailable(ANIMATION)

FetchContent_Declare(
    ASSETS_SYSTEM
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../assets_system
)
FetchContent_MakeAvailable(ASSETS_SYSTEM)

FetchContent_Declare(
    SKINNING_SYSTEM
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../skinning_system
)
FetchContent_MakeAvailable(SKINNING_SYSTEM)

FetchContent_Declare(
    IK_SYSTEM
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../ik_system
)
FetchContent_MakeAvailable(IK_SYSTEM)

FetchContent_Declare(
    PLATFORM
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../platform
)
FetchContent_MakeAvailable(PLATFORM)

FetchContent_Declare(
    COMPONENT
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../component
)
FetchContent_MakeAvailable(COMPONENT)

set(DEPENDENCIES
    PRIVATE component
    PRIVATE animation
    PRIVATE assets_system
    PRIVATE skinning_system
    PRIVATE ik_system
    PRIVATE physics
    PRIVATE platform
    PRIVATE engine_types
    PRIVATE foundation
    PRIVATE glm
)


#
# 以降は触らなくて良い
#


target_include_directories(${PROJECT_NAME} PUBLIC src)

target_link_libraries(${PROJECT_NAME} ${DEPENDENCIES})

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
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
