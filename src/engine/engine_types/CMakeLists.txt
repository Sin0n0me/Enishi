cmake_minimum_required (VERSION 3.14)
project(engine_types)

# 依存設定
set(DEPENDENCIES)
include(FetchContent)

FetchContent_Declare(
    GLM
    GIT_REPOSITORY git@github.com:g-truc/glm.git
    GIT_TAG "1.0.3"
    GIT_SHALLOW TRUE
    GIT_PROGRESS ON
)
FetchContent_MakeAvailable(GLM)

set(DEPENDENCIES
    glm
)

#
# 以降は触らなくて良い(基盤なのでpublic)
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

add_library(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC src)

target_link_libraries(${PROJECT_NAME} PUBLIC ${DEPENDENCIES})

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
