cmake_minimum_required (VERSION 3.14)
project(core)

include(${CMAKE_CURRENT_LIST_DIR}/../cmake/EnishiLibrary.cmake)

# 以下依存設定
set(DEPENDENCIES)
include(FetchContent)

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

FetchContent_Declare(
    ECS
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../ecs
)
FetchContent_MakeAvailable(ECS)

FetchContent_Declare(
    COMPONENT
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../component
)
FetchContent_MakeAvailable(COMPONENT)

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
    MODEL_CONTROLLER
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../model_controller
)
FetchContent_MakeAvailable(MODEL_CONTROLLER)

FetchContent_Declare(
    PHYSICS
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../physics
)
FetchContent_MakeAvailable(PHYSICS)

set(DEPENDENCIES
    PUBLIC platform
    PUBLIC sub_system
    PUBLIC ecs
    PUBLIC component
    PUBLIC animation
    PUBLIC assets_system
    PUBLIC skinning_system
    PUBLIC ik_system
    PUBLIC model_controller
    PUBLIC physics
    PRIVATE glm
    PRIVATE engine_types
    PRIVATE foundation
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
