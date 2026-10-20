#include <animation/procedural/cursor/cursor_pose.h>
#include <animation/transition/pose_transition.h>
#include <cstdlib>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

using namespace enishi;
namespace {
    constexpr float TOLERANCE = 0.001f;
    void check(bool condition, const char* message) {
        if (!condition) {
            std::cerr << message << '\n';
            std::exit(EXIT_FAILURE);
        }
    }
    void linear_pose_transition() {
        std::vector<component::AnimationBuffer> from(1);
        auto to = from;
        to[0].position = {4, 8, 12};
        to[0].scale = {3, 3, 3};
        to[0].rotation = glm::angleAxis(glm::half_pi<float>(), glm::vec3(1, 0, 0));
        animation::PoseTransition transition;
        transition.begin(from, to);
        auto output = from;
        transition.apply(0.25f, output);
        check(output[0].position == to[0].position * 0.5f && output[0].scale == glm::vec3(2),
            "Pose transition linearly interpolates translation and scale at half time");
        check(std::abs(glm::angle(output[0].rotation) - glm::quarter_pi<float>()) < TOLERANCE,
            "Pose transition interpolates normalized shortest-arc rotation");
        transition.apply(0.25f, output);
        check(!transition.active() && output[0].position == to[0].position &&
                  output[0].rotation == to[0].rotation,
            "Pose transition reaches the captured initial pose at exactly half a second");
        transition.begin(from, to);
        transition.apply(0.25f, output);
        const auto interrupted = output;
        transition.begin(output, from);
        transition.apply(0, output);
        check(output[0].position == interrupted[0].position,
            "Interrupted transitions start from the currently displayed pose");
        to = from;
        to[0].rotation = -from[0].rotation;
        transition.begin(from, to);
        transition.apply(0.25f, output);
        check(std::abs(glm::dot(output[0].rotation, from[0].rotation)) > 1 - TOLERANCE,
            "Equivalent quaternion signs do not produce invalid intermediate rotations");
        transition.cancel();
        check(!transition.active(), "An interactive action can cancel a pending transition");
    }

    void head_tracking_without_core() {
        constexpr std::size_t BONE_COUNT = 3;
        constexpr std::size_t HEAD = 2;
        constexpr int FRAMES = 60;
        constexpr float DT = 1.0f / 60;
        component::ModelComponent model;
        component::AnimationComponent pose;
        model.bone_names = {"root", "neck", "head"};
        model.bone_node.resize(BONE_COUNT);
        model.bind_bone.resize(BONE_COUNT);
        pose.animation.resize(BONE_COUNT);
        pose.global.resize(BONE_COUNT, glm::mat4(1));
        for (std::size_t bone = 0; bone < BONE_COUNT; ++bone) {
            const auto parent = bone == 0 ? types::INVALID_BONE_INDEX : bone - 1;
            model.bone_node[bone].parent = parent;
            if (parent < BONE_COUNT) {
                model.bone_node[parent].children.push_back(bone);
            }
            const glm::vec3 offset(0, static_cast<float>(bone), 0);
            pose.animation[bone].position = offset;
            const auto local = glm::translate(glm::mat4(1), offset);
            pose.global[bone] = parent < BONE_COUNT ? pose.global[parent] * local : local;
            model.bind_bone[bone].local = local;
            model.bind_bone[bone].global = pose.global[bone];
            model.bind_bone[bone].global_inverse = glm::inverse(pose.global[bone]);
        }
        const auto original = pose.animation;
        animation::CursorPoseConfig config;
        config.jump_enabled = false;
        animation::CursorPose tracker(model, config);
        const animation::CursorRay target{{{0.5f, 3, -20}, {0, 0, 1}}, {0, 0, 1}};
        bool turned = false;
        for (int frame = 0; frame < FRAMES; ++frame) {
            tracker.apply(model, pose, target, DT);
            turned = turned || glm::length(pose.animation[HEAD].rotation -
                                           original[HEAD].rotation) > TOLERANCE;
            tracker.restore(pose);
            for (std::size_t bone = 0; bone < BONE_COUNT; ++bone) {
                check(pose.animation[bone].position == original[bone].position &&
                          pose.animation[bone].rotation == original[bone].rotation,
                    "Procedural layers restore the source without an ECS or window");
            }
        }
        check(turned, "Model-space targets animate the head without core integration");
    }
} // namespace
int main() {
    linear_pose_transition();
    head_tracking_without_core();
}
