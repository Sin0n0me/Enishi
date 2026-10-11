#include <animation/animation_controller.h>
#include <core/system/animation/clip_sampler.h>
#include <cstdlib>
#include <iostream>
#include <limits>

using namespace enishi;

namespace {
    void check(bool condition, const char* message) {
        if (!condition) {
            std::cerr << message << '\n';
            std::exit(EXIT_FAILURE);
        }
    }
} // namespace

void clip_sampler_tests() {
    auto clip = std::make_shared<animation::AnimationClipData>();
    clip->relative_to_bind_pose = true;
    clip->duration = 2;
    animation::BoneTrack bone{};
    bone.positions.times = {0, 2};
    bone.positions.values = {{0, 0, 0}, {2, 0, 0}};
    clip->bone_tracks.push_back(bone);
    animation::MorphTrack expression{};
    expression.weights.times = {0, 2};
    expression.weights.values = {0, 1};
    clip->morph_tracks.push_back(expression);
    clip->ik_tracks.push_back({0, {1, 2}, {false, true}});
    component::AnimationComponent pose;
    pose.animation.resize(2);
    pose.bind_pose.resize(2);
    pose.bind_pose[0].position = {0, 3, 0};
    pose.bind_pose[1].position = {0, 7, 0};
    component::MorphComponent morph{{0, 1}};
    component::IKComponent ik;
    animation::AnimationController controller;
    controller.add_clip("motion", clip);
    check(controller.play("motion"), "play clip");
    controller.update(1);
    core::sample_model_clip(*clip, controller.get_time(), pose, &morph, &ik);
    check(pose.animation[0].position == glm::vec3(1, 3, 0),
        "relative motion preserves bind translation");
    check(pose.animation[1].position == glm::vec3(0, 7, 0), "unkeyed bone retains rest pose");
    check(morph.weights == std::vector<float>{0.5f, 0}, "sample weights and reset unkeyed morph");
    check(ik.disabled_bones.contains(0), "IK switches off exactly at key time");
    controller.pause();
    controller.update(1);
    core::sample_model_clip(*clip, controller.get_time(), pose, &morph, &ik);
    check(pose.animation[0].position == glm::vec3(1, 3, 0),
        "paused sampling does not accumulate offsets");
    controller.stop();
    core::sample_model_clip(*clip, controller.get_time(), pose, &morph, &ik);
    check(morph.weights[0] == 0 && ik.disabled_bones.empty(),
        "rewind resets morph and pre-key IK state");
    clip->relative_to_bind_pose = false;
    core::sample_model_clip(*clip, 1, pose, nullptr, nullptr);
    check(
        pose.animation[0].position == glm::vec3(1, 0, 0), "absolute local tracks remain supported");
    check(controller.play("motion"), "restart");
    controller.update(3);
    check(controller.get_time() == 2 && !controller.is_playing(), "non-looping clip clamps at end");
    clip->is_looping = true;
    check(controller.play("motion"), "restart looping");
    controller.update(3);
    check(controller.get_time() == 1, "loop wraps duration");
    controller.update(std::numeric_limits<float>::infinity());
    check(controller.get_time() == 1, "invalid elapsed time does not corrupt playback");
}
