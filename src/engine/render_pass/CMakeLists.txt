cmake_minimum_required (VERSION 3.14)
project(render_pass)

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

if(USE_VULKAN)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_VULKAN)
elseif(USE_OPENGL40)
    target_compile_definitions(${PROJECT_NAME} PRIVATE "USE_OPENGL40")
endif()

# 以下依存設定

set(DEPENDENCIES)
include(FetchContent)

FetchContent_Declare(ENISHI_DEBUG SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../debug)
FetchContent_MakeAvailable(ENISHI_DEBUG)

FetchContent_Declare(
    PLATFORM
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../platform
)
FetchContent_MakeAvailable(PLATFORM)

set(DEPENDENCIES
    PRIVATE platform
    PRIVATE engine_types
    PRIVATE foundation
    PRIVATE glm
)


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

target_link_libraries(${PROJECT_NAME} PRIVATE enishi::debug)
