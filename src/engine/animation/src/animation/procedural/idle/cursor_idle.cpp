#include "cursor_idle.h"
#include <algorithm>
#include <animation/procedural/cursor/cursor_reach.h>
#include <animation/procedural/cursor/cursor_transforms.h>
#include <animation/procedural/hanging/cursor_hanging.h>
#include <animation/procedural/legs/cursor_legs.h>
#include <cmath>

namespace enishi::animation {
    using enum CursorPoseState::Joint;
    using enum CursorPoseState::BodyBone;
    namespace {
        constexpr float EPSILON = 0.00001f;
        constexpr float MAX_DT = 0.05f;
        constexpr float BLEND_RATE = 3.0f;
        constexpr float TIME_WRAP = 120.0f;
        constexpr float SWAY_SECONDS = 6.0f;
        constexpr float BREATH_SECONDS = 4.0f;
        constexpr float FOOT_SECONDS = 10.0f;
        constexpr float TORSO_ROLL_DEGREES = 1.2f;
        constexpr float BREATH_DEGREES = 0.4f;
        constexpr float SHIFT_RATIO = 0.012f;
        constexpr float FOOT_SHIFT_RATIO = 0.015f;
        constexpr float FOOT_LIFT_RATIO = 0.008f;
        constexpr float CROUCH_WEIGHT = 0.15f;
        constexpr float HAND_DOWN_RATIO = 0.978f;
        constexpr float HAND_OUT_RATIO = 0.18f;
        constexpr float HAND_FORWARD_RATIO = 0.06f;
    } // namespace

    void CursorIdle::apply_idle(CursorPoseState& state,
        const component::ModelComponent& model,
        component::AnimationComponent& animation,
        bool enabled,
        float seconds) {
        const float dt = std::isfinite(seconds) ? std::clamp(seconds, 0.0f, MAX_DT) : 0;
        // Exponential blending w'=w+(target-w)*(1-exp(-rate*dt)) avoids abrupt pose switches.
        const float target = enabled ? 1.0f : 0.0f;
        state.idle.weight += (target - state.idle.weight) * (1 - std::exp(-BLEND_RATE * dt));
        if (state.transition_state.initializing) {
            state.idle.weight = target;
        }
        if (animation.motion_playing) {
            state.idle.weight = 0;
        }
        if (!(state.idle.weight > EPSILON)) {
            state.idle.weight = 0;
            return;
        }
        state.idle.time = std::fmod(state.idle.time + dt, TIME_WRAP);
        const bool suspended =
            state.hanging_state.active &&
            state.wrists[state.hanging_state.side] < state.transforms.globals.size();
        const auto anchor =
            suspended
                ? glm::vec3(state.transforms.globals[state.wrists[state.hanging_state.side]][3])
                : glm::vec3(0);
        // Each oscillation is sin(2*pi*t/period); the common wrap preserves every phase.
        const float sway = std::sin(glm::two_pi<float>() * state.idle.time / SWAY_SECONDS);
        const float breath = std::sin(glm::two_pi<float>() * state.idle.time / BREATH_SECONDS);
        if (state.idle.torso < animation.animation.size()) {
            animation.animation[state.idle.torso].rotation *=
                glm::angleAxis(glm::radians(TORSO_ROLL_DEGREES) * sway * state.idle.weight,
                    glm::vec3(0, 0, 1)) *
                glm::angleAxis(
                    glm::radians(BREATH_DEGREES) * breath * state.idle.weight, glm::vec3(1, 0, 0));
            CursorTransforms::update_globals(state.transforms, model, animation);
        }
        if (suspended) {
            // Keep the grasp fixed: root_delta = wrist_before - wrist_after_sway.
            CursorHanging::translate_hanging_roots(state,
                animation,
                anchor -
                    glm::vec3(state.transforms.globals[state.wrists[state.hanging_state.side]][3]));
            CursorTransforms::update_globals(state.transforms, model, animation);
            CursorHanging::update_hanging_ik_targets(state, model, animation);
        }
        // Hanging, landing and walking own the limb poses; breathing continues above them.
        if (state.hanging_state.weight > EPSILON || state.hanging_state.landing) {
            return;
        }
        if (!(state.locomotion.height > 0)) {
            CursorIdle::apply_idle_legs(state, model, animation, sway);
        }
        CursorIdle::apply_idle_arms(state, model, animation, state.idle.weight);
    }

    void CursorIdle::apply_idle_arms(CursorPoseState& state,
        const component::ModelComponent& model,
        component::AnimationComponent& animation,
        float weight) {
        for (std::size_t side = 0; side < state.wrists.size(); ++side) {
            if (!(state.wrists[side] < state.transforms.globals.size())) {
                continue;
            }
            const auto arm = state.joints[side == 0 ? LeftArm : RightArm];
            const auto elbow = state.joints[side == 0 ? LeftElbow : RightElbow];
            const auto upper = animation.animation[arm].rotation;
            const auto lower = animation.animation[elbow].rotation;
            const auto frame = CursorReach::arm_frame(state, side);
            // |hand-shoulder|/length = sqrt(down^2+out^2+forward^2), keeping a slight elbow bend.
            const auto hand =
                glm::vec3(state.transforms.globals[arm][3]) +
                frame.length * (-frame.up * HAND_DOWN_RATIO + frame.outward * HAND_OUT_RATIO +
                                   frame.forward * HAND_FORWARD_RATIO);
            CursorReach::solve_arm(state, model, animation, side, hand, frame);
            animation.animation[arm].rotation =
                glm::slerp(upper, animation.animation[arm].rotation, weight);
            animation.animation[elbow].rotation =
                glm::slerp(lower, animation.animation[elbow].rotation, weight);
            CursorTransforms::update_globals(state.transforms, model, animation);
        }
    }

    void CursorIdle::apply_idle_legs(CursorPoseState& state,
        const component::ModelComponent& model,
        component::AnimationComponent& animation,
        float sway) {
        if (state.locomotion.legs[0][2] < state.transforms.globals.size() &&
            state.locomotion.legs[1][2] < state.transforms.globals.size()) {
            state.idle.applied = true;
            const std::array<glm::mat4, 2> feet{
                state.transforms.globals[state.locomotion.legs[0][2]],
                state.transforms.globals[state.locomotion.legs[1][2]]};
            const auto& leg = state.locomotion.legs[0];
            const float length = glm::length(glm::vec3(state.transforms.globals[leg[0]][3] -
                                                       state.transforms.globals[leg[1]][3])) +
                                 glm::length(glm::vec3(state.transforms.globals[leg[1]][3] -
                                                       state.transforms.globals[leg[2]][3]));
            CursorHanging::translate_hanging_roots(
                state, animation, glm::vec3(length * SHIFT_RATIO * sway * state.idle.weight, 0, 0));
            CursorTransforms::update_globals(state.transforms, model, animation);
            CursorLegs::crouch(state, model, animation, CROUCH_WEIGHT * state.idle.weight);
            for (std::size_t side = 0; side < state.locomotion.legs.size(); ++side) {
                auto foot = feet[side];
                const float phase = glm::two_pi<float>() * state.idle.time / FOOT_SECONDS +
                                    static_cast<float>(side) * glm::pi<float>();
                // Positive half-wave squared gives alternating, smooth foot adjustments.
                const float wave = (std::max)(0.0f, std::sin(phase));
                const float amount = wave * wave * state.idle.weight;
                const float outward = side == 0 ? 1.0f : -1.0f;
                foot[3].x += outward * length * FOOT_SHIFT_RATIO * amount;
                foot[3].y += length * FOOT_LIFT_RATIO * amount;
                CursorLegs::bend_leg(state, model, animation, side, foot);
            }
            CursorHanging::update_hanging_ik_targets(state, model, animation);
        }
    }
} // namespace enishi::animation
