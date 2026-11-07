cmake_minimum_required(VERSION 3.20)
project(test_support_tests LANGUAGES CXX)
enable_testing()
include("${CMAKE_CURRENT_LIST_DIR}/../test_support.cmake")
find_path(ICU_INCLUDE_DIR unicode/unistr.h REQUIRED)
add_executable(test_support_diagnostics diagnostic_tests.cpp)
target_compile_features(test_support_diagnostics PRIVATE cxx_std_23)
target_include_directories(test_support_diagnostics PRIVATE "../../foundation/src" "${ICU_INCLUDE_DIR}")
target_link_libraries(test_support_diagnostics PRIVATE enishi_test_support)
set(DIAGNOSTIC_RUNNER "${CMAKE_COMMAND}"
    "-DEXECUTABLE=$<TARGET_FILE:test_support_diagnostics>")
set(DIAGNOSTIC_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/check_diagnostics.cmake")
add_test(NAME test_support_success COMMAND ${DIAGNOSTIC_RUNNER}
    -DMODE=success -P "${DIAGNOSTIC_SCRIPT}")
add_test(NAME test_support_result COMMAND ${DIAGNOSTIC_RUNNER} -DMODE=result
    "-DEXPECTED=[FAIL] diagnostic_probe;initialize renderer;error=7;original cause;outer context;diagnostic_tests.cpp:"
    -P "${DIAGNOSTIC_SCRIPT}")
add_test(NAME test_support_empty COMMAND ${DIAGNOSTIC_RUNNER} -DMODE=empty
    "-DEXPECTED=error=7;Result contains no error message;diagnostic_tests.cpp:"
    -P "${DIAGNOSTIC_SCRIPT}")
add_test(NAME test_support_boolean COMMAND ${DIAGNOSTIC_RUNNER} -DMODE=boolean
    "-DEXPECTED=boolean failure;diagnostic_tests.cpp:" -P "${DIAGNOSTIC_SCRIPT}")
add_test(NAME test_support_exception COMMAND ${DIAGNOSTIC_RUNNER} -DMODE=exception
    "-DEXPECTED=exception detail" -P "${DIAGNOSTIC_SCRIPT}")
add_test(NAME test_support_terminate COMMAND ${DIAGNOSTIC_RUNNER} -DMODE=terminate
    "-DEXPECTED=without an active exception" -P "${DIAGNOSTIC_SCRIPT}")
add_test(NAME test_support_accumulator COMMAND ${DIAGNOSTIC_RUNNER} -DMODE=accumulator
    "-DEXPECTED=++evaluations == 0;false;diagnostic_tests.cpp:" -P "${DIAGNOSTIC_SCRIPT}")
add_test(NAME test_support_throw COMMAND ${DIAGNOSTIC_RUNNER} -DMODE=throw
    "-DEXPECTED=throwing assertion;diagnostic_tests.cpp:" -P "${DIAGNOSTIC_SCRIPT}")
