cmake_minimum_required (VERSION 3.14)
project(renderer)

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

if(USE_OPENGL40)
    list(FILTER SOURCES EXCLUDE REGEX ".*/directx/.*")
else()
    list(FILTER SOURCES EXCLUDE REGEX ".*/opengl/.*")
endif()

enishi_add_library(${PROJECT_NAME} ${SOURCES})

# 以下依存設定

set(DEPENDENCIES)
include(FetchContent)

FetchContent_Declare(
    PLATFORM
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../platform
)
FetchContent_MakeAvailable(PLATFORM)

set(DEPENDENCIES
    glm
    platform
    engine_types
    foundation
)

# 各プラットフォームに応じた使用ライブラリの変更
if(USE_DIRECTX_11)
    target_compile_definitions(${PROJECT_NAME} PRIVATE "USE_DIRECTX_11")

    set(DEPENDENCIES 
        ${DEPENDENCIES} 
        PUBLIC d3d11
        PUBLIC dxgi
        PUBLIC dxguid
        PUBLIC dcomp
        PUBLIC dwmapi
        PUBLIC d3dcompiler
    )
elseif(USE_DIRECTX_12)
    target_compile_definitions(${PROJECT_NAME} PRIVATE "USE_DIRECTX_12")    
    
    set(DEPENDENCIES 
        ${DEPENDENCIES} 
        PUBLIC d3d12
        PUBLIC dxgi
        PUBLIC dxguid
        PUBLIC dcomp
        PUBLIC dwmapi
        PUBLIC d3dcompiler
    )
elseif(USE_OPENGL40)
    target_compile_definitions(${PROJECT_NAME} PUBLIC "USE_OPENGL40")

    FetchContent_Declare(
        glad
        GIT_REPOSITORY https://github.com/Dav1dde/glad.git
        GIT_TAG v2.0.8
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(glad)
    glad_add_library(glad_gl_core_40 REPRODUCIBLE API gl:core=4.0)

    set(DEPENDENCIES
        ${DEPENDENCIES}
        PUBLIC glad_gl_core_40
        PUBLIC SDL3::SDL3
    )

elseif(USE_VULKAN)
    target_compile_definitions(${PROJECT_NAME} PRIVATE "USE_VULKAN")
    
    set(DEPENDENCIES 
        ${DEPENDENCIES} 
        PUBLIC Vulkan::Vulkan
    )
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

    # テスト用exeにメインのライブラリ(lib)をリンクする
    #target_link_libraries(tests PRIVATE my_lib)

    # CTestにテストケースを登録
    add_test(NAME RunAllTests COMMAND tests)
endif()
