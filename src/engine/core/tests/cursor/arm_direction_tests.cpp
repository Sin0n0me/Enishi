#include "arm_direction_tests.h"
#include "../test_runner.h"
#include <animation/procedural/cursor/cursor_reach.h>
#include <glm/gtc/matrix_transform.hpp>

namespace {
    constexpr std::size_t LEFT = 0;
    constexpr std::size_t RIGHT = 1;
    constexpr std::size_t LEFT_ARM = 0;
    constexpr std::size_t LEFT_ELBOW = 1;
    constexpr std::size_t LEFT_HAND = 2;
    constexpr std::size_t RIGHT_ARM = 3;
    constexpr std::size_t RIGHT_ELBOW = 4;
    constexpr std::size_t RIGHT_HAND = 5;
    constexpr float TOLERANCE = 0.0001f;
    constexpr float SHOULDER_X = 1;
    constexpr float ELBOW_X = 3;
    constexpr float HAND_X = 5;
    constexpr float SHOULDER_HEIGHT = 7;
    constexpr float TURN_DEGREES = 65;
    constexpr glm::length_t TRANSLATION_COLUMN = 3;

    void check_direction(float left_sign, float turn) {
        using State = enishi::animation::CursorPoseState;
        State state{};
        state.joints[State::LeftArm] = LEFT_ARM;
        state.joints[State::LeftElbow] = LEFT_ELBOW;
        state.joints[State::RightArm] = RIGHT_ARM;
        state.joints[State::RightElbow] = RIGHT_ELBOW;
        state.wrists = {LEFT_HAND, RIGHT_HAND};
        state.body_bones.fill(enishi::types::INVALID_BONE_INDEX);
        state.body_bones[State::BodyLeft] = LEFT_ARM;
        state.body_bones[State::BodyRight] = RIGHT_ARM;
        const auto rotation = glm::rotate(glm::mat4(1), glm::radians(turn), glm::vec3(0, 1, 0));
        for (const auto x : {SHOULDER_X, ELBOW_X, HAND_X, -SHOULDER_X, -ELBOW_X, -HAND_X}) {
            state.transforms.globals.push_back(
                rotation *
                glm::translate(glm::mat4(1), glm::vec3(x * left_sign, SHOULDER_HEIGHT, 0)));
        }
        for (const auto side : {LEFT, RIGHT}) {
            const auto frame = enishi::animation::CursorReach::arm_frame(state, side);
            const auto shoulder_index = side == LEFT ? LEFT_ARM : RIGHT_ARM;
            const auto shoulder =
                glm::vec3(state.transforms.globals[shoulder_index][TRANSLATION_COLUMN]);
            // Outward direction = (shoulder - torso center) / distance from torso center.
            const auto expected = glm::normalize(shoulder - frame.center);
            enishi::tests::check(glm::dot(frame.outward, expected) > 1.0f - TOLERANCE,
                "elbow poles and collision search must point away from the torso");
        }
    }
} // namespace

void arm_direction_tests() {
    enishi::tests::run_test("arm direction for both humanoid X conventions", [] {
        for (const auto sign : {1.0f, -1.0f}) {
            check_direction(sign, 0);
            check_direction(sign, TURN_DEGREES);
        }
    });
}
