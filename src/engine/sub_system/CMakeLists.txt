cmake_minimum_required(VERSION 3.12)
project(sub_system)

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

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    "src/*.h"
    "src/*.c"
    "src/*.hpp"
    "src/*.cpp"
)

add_library(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC src)
target_link_libraries(${PROJECT_NAME} PRIVATE engine_types foundation glm)

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
)
