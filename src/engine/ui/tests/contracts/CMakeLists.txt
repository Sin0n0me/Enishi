add_executable(ui_contract_tests ui_tests.cpp)
target_compile_features(ui_contract_tests PRIVATE cxx_std_23)
target_link_libraries(ui_contract_tests PRIVATE ui enishi_imgui)
add_test(NAME ui_contract_tests COMMAND ui_contract_tests)
set_tests_properties(ui_contract_tests PROPERTIES TIMEOUT 30)

include("${CMAKE_CURRENT_LIST_DIR}/../../../test_support/test_support.cmake")
target_link_libraries(ui_contract_tests PRIVATE enishi_test_support)
