add_executable(render_pass_contract_tests render_pass_tests.cpp)
target_compile_features(render_pass_contract_tests PRIVATE cxx_std_23)
target_link_libraries(render_pass_contract_tests PRIVATE render_pass platform engine_types foundation)
add_test(NAME render_pass_contract_tests COMMAND render_pass_contract_tests)
set_tests_properties(render_pass_contract_tests PROPERTIES TIMEOUT 30)

include("${CMAKE_CURRENT_LIST_DIR}/../../../test_support/test_support.cmake")
target_link_libraries(render_pass_contract_tests PRIVATE enishi_test_support)
