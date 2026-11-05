cmake_minimum_required(VERSION 3.20)
project(foundation_runtime_tests LANGUAGES CXX)
find_package(ICU REQUIRED COMPONENTS uc data)
find_package(Threads REQUIRED)
add_executable(foundation_runtime_tests runtime_tests.cpp
    ../../src/foundation/algorithm/resolve_dependencies.cpp
    ../../src/foundation/text/config_line_reader.cpp
    ../../src/foundation/thread/single_thread_executor.cpp)
target_compile_features(foundation_runtime_tests PRIVATE cxx_std_23)
target_include_directories(foundation_runtime_tests PRIVATE ../../src)
target_link_libraries(foundation_runtime_tests PRIVATE ICU::uc ICU::data Threads::Threads)
if(MSVC)
    target_compile_options(foundation_runtime_tests PRIVATE /utf-8 /W4 /EHsc)
endif()
enable_testing()
add_test(NAME foundation_runtime_tests COMMAND foundation_runtime_tests)
set_tests_properties(foundation_runtime_tests PROPERTIES TIMEOUT 30)
