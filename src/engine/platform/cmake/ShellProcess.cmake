include_guard(GLOBAL)
add_library(platform_shell INTERFACE)
target_include_directories(platform_shell INTERFACE "${CMAKE_CURRENT_LIST_DIR}/../src")
target_compile_features(platform_shell INTERFACE cxx_std_23)
