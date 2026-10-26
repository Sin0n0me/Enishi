add_executable(enishi_animation_pose_tests pose_tests.cpp)
# Deliberately do not link core: pose calculation must work without system orchestration.
target_link_libraries(enishi_animation_pose_tests PRIVATE animation)
target_compile_features(enishi_animation_pose_tests PRIVATE cxx_std_23)
if(MSVC)
    target_compile_options(enishi_animation_pose_tests PRIVATE /utf-8)
endif()
add_test(NAME enishi_animation_pose_tests COMMAND enishi_animation_pose_tests)
