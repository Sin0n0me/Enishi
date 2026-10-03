cmake_minimum_required (VERSION 3.14)
project(collider)

include(${CMAKE_CURRENT_LIST_DIR}/../cmake/EnishiLibrary.cmake)

# 以下依存設定
set(DEPENDENCIES)
include(FetchContent)

# ローカル
FetchContent_Declare(ENGINE_TYPES SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../engine_types)
FetchContent_MakeAvailable(ENGINE_TYPES)
FetchContent_Declare(SUB_SYSTEM SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../sub_system)
FetchContent_MakeAvailable(SUB_SYSTEM)

set(DEPENDENCIES
    glm
    engine_types
    foundation
    sub_system
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
    PUBLIC_DEPENDENCIES ${DEPENDENCIES}
)

# MSVCコンパイラのホットリロードを有効に
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()


# 以下テスト用設定
option(COLLIDER_BUILD_TESTING "Build collider tests" OFF)

if(COLLIDER_BUILD_TESTING)
    enable_testing()

    # テスト用実行ファイルの生成
    file(GLOB TEST_SOURCES
        "tests/*.h"
        "tests/*.cpp" 
        "tests/*.hpp" 
        "tests/*.cpp"
    )

    add_executable(${PROJECT_NAME}_test ${TEST_SOURCES})
    target_link_libraries(${PROJECT_NAME}_test PRIVATE collider)
    target_compile_features(${PROJECT_NAME}_test PRIVATE cxx_std_23)

    # CTestにテストケースを登録
    add_test(NAME ColliderTests COMMAND collider_test)
endif()
