cmake_minimum_required (VERSION 3.14)
project(script)

include(${CMAKE_CURRENT_LIST_DIR}/../cmake/EnishiLibrary.cmake)

# 以下依存設定
set(DEPENDENCIES)
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

set(DEPENDENCIES
    engine_types
    foundation
    sub_system
)

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Musubu/compiler/musubu_engine/Cargo.toml")
    message(FATAL_ERROR "Musubu is missing. Run git submodule update --init --recursive")
endif()
find_program(CARGO_EXECUTABLE cargo REQUIRED)
option(ENISHI_CARGO_OFFLINE "Build Musubu using cached Cargo dependencies only" OFF)
set(cargo_network_options)
if(ENISHI_CARGO_OFFLINE)
    list(APPEND cargo_network_options --offline)
endif()
set(musubu_target_dir "${CMAKE_CURRENT_BINARY_DIR}/cargo")
if(DEFINED ENV{CARGO_TARGET_DIR} AND NOT "$ENV{CARGO_TARGET_DIR}" STREQUAL "")
    file(TO_CMAKE_PATH "$ENV{CARGO_TARGET_DIR}" musubu_target_dir)
endif()
set(cargo_target_options)
if(MINGW)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(musubu_rust_target x86_64-pc-windows-gnu)
    else()
        set(musubu_rust_target i686-pc-windows-gnu)
    endif()
    list(APPEND cargo_target_options --target ${musubu_rust_target})
    # C++側とRust側でGNU ABIを揃え、MSVC用のライブラリとの混在を避けます。
    set(musubu_library "${musubu_target_dir}/${musubu_rust_target}/release/libenishi_musubu_bridge.a")
elseif(WIN32)
    set(musubu_library "${musubu_target_dir}/release/enishi_musubu_bridge.lib")
else()
    set(musubu_library "${musubu_target_dir}/release/libenishi_musubu_bridge.a")
endif()
# Cargo tracks Rust sources and the pinned submodule, including changes after checkout.
add_custom_target(enishi_musubu_build
    COMMAND ${CMAKE_COMMAND} -E env RUSTUP_TOOLCHAIN=stable
        "${CARGO_EXECUTABLE}" build --release --locked ${cargo_network_options} ${cargo_target_options}
        --manifest-path "${CMAKE_CURRENT_SOURCE_DIR}/bridge/Cargo.toml"
        --target-dir "${musubu_target_dir}"
    BYPRODUCTS "${musubu_library}"
    VERBATIM)
add_library(enishi_musubu STATIC IMPORTED GLOBAL)
set_target_properties(enishi_musubu PROPERTIES IMPORTED_LOCATION "${musubu_library}")
add_dependencies(enishi_musubu enishi_musubu_build)
if(WIN32)
    target_link_libraries(enishi_musubu INTERFACE ws2_32 userenv bcrypt ntdll)
else()
    find_package(Threads REQUIRED)
    target_link_libraries(enishi_musubu INTERFACE Threads::Threads ${CMAKE_DL_LIBS} m)
endif()
list(APPEND DEPENDENCIES enishi_musubu)

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
    DEPENDENCIES ${DEPENDENCIES}
    PUBLIC_DEPENDENCIES sub_system
)

# MSVCコンパイラのホットリロードを有効に
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()
