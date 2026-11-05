cmake_minimum_required(VERSION 3.20)
project(foundation_option_tests LANGUAGES CXX)

function(enishi_check_original_option_alias)
    # Compare only the private alias name, using the same compiler and a source
    # without using-directives. A historical failure is diagnostic, not a failure
    # of the current implementation tested below.
    file(READ "${CMAKE_CURRENT_LIST_DIR}/../../src/foundation/option/option.h" original_header)
    string(REPLACE "OptionalStorage" "Option" original_header "${original_header}")
    set(original_include "${CMAKE_CURRENT_BINARY_DIR}/original-option")
    file(WRITE "${original_include}/foundation/option/option.h" "${original_header}")
    set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
    try_compile(original_alias_compiles "${CMAKE_CURRENT_BINARY_DIR}/original-option-build"
        SOURCES "${CMAKE_CURRENT_LIST_DIR}/option_lookup_tests.cpp"
        COMPILE_DEFINITIONS "-I${original_include}"
        CXX_STANDARD 23 CXX_STANDARD_REQUIRED ON
        OUTPUT_VARIABLE original_alias_output)
    message(STATUS "Option original alias without using-directives (${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}): ${original_alias_compiles}")
    if(NOT original_alias_compiles)
        message(STATUS "Original Option alias compiler diagnostics:\n${original_alias_output}")
    endif()
endfunction()

enishi_check_original_option_alias()

add_executable(foundation_option_tests option_tests.cpp option_lookup_tests.cpp)
target_compile_features(foundation_option_tests PRIVATE cxx_std_23)
target_include_directories(foundation_option_tests PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../../src")
if(MSVC)
    target_compile_options(foundation_option_tests PRIVATE /utf-8 /W4 /EHsc)
endif()
enable_testing()
add_test(NAME foundation_option_tests COMMAND foundation_option_tests)
set_tests_properties(foundation_option_tests PROPERTIES TIMEOUT 30)
