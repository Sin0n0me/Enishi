#include "cursor_pose.h"
#include <algorithm>
#include <cmath>

namespace enishi::core {
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

    void CursorPose::apply_idle(const component::ModelComponent& model,
        component::AnimationComponent& animation,
        bool enabled,
        float seconds) {
        const float dt = std::isfinite(seconds) ? std::clamp(seconds, 0.0f, MAX_DT) : 0;
        // Exponential blending w'=w+(target-w)*(1-exp(-rate*dt)) avoids abrupt pose switches.
        const float target = enabled ? 1.0f : 0.0f;
        this->idle_weight += (target - this->idle_weight) * (1 - std::exp(-BLEND_RATE * dt));
        if (animation.motion_playing) {
            this->idle_weight = 0;
        }
        if (!(this->idle_weight > EPSILON)) {
            this->idle_weight = 0;
            return;
        }
        this->idle_time = std::fmod(this->idle_time + dt, TIME_WRAP);
        const bool suspended =
            this->hanging && this->wrists[this->hanging_side] < this->globals.size();
        const auto anchor = suspended
                                ? glm::vec3(this->globals[this->wrists[this->hanging_side]][3])
                                : glm::vec3(0);
        // Each oscillation is sin(2*pi*t/period); the common wrap preserves every phase.
        const float sway = std::sin(glm::two_pi<float>() * this->idle_time / SWAY_SECONDS);
        const float breath = std::sin(glm::two_pi<float>() * this->idle_time / BREATH_SECONDS);
        if (this->idle_torso < animation.animation.size()) {
            animation.animation[this->idle_torso].rotation *=
                glm::angleAxis(glm::radians(TORSO_ROLL_DEGREES) * sway * this->idle_weight,
                    glm::vec3(0, 0, 1)) *
                glm::angleAxis(
                    glm::radians(BREATH_DEGREES) * breath * this->idle_weight, glm::vec3(1, 0, 0));
            this->update_globals(model, animation);
        }
        if (suspended) {
            // Keep the grasp fixed: root_delta = wrist_before - wrist_after_sway.
            this->translate_hanging_roots(
                animation, anchor - glm::vec3(this->globals[this->wrists[this->hanging_side]][3]));
            this->update_globals(model, animation);
            this->update_hanging_ik_targets(model, animation);
        }
        // Hanging, landing and walking own the limb poses; breathing continues above them.
        if (this->hanging_weight > EPSILON || this->hanging_landing) {
            return;
        }
        if (!(this->jump_height > 0)) {
            this->apply_idle_legs(model, animation, sway);
        }
        this->apply_idle_arms(model, animation);
    }

    void CursorPose::apply_idle_legs(const component::ModelComponent& model,
        component::AnimationComponent& animation,
        float sway) {
        if (this->legs[0][2] < this->globals.size() && this->legs[1][2] < this->globals.size()) {
            this->idle_applied = true;
            const std::array<glm::mat4, 2> feet{
                this->globals[this->legs[0][2]], this->globals[this->legs[1][2]]};
            const auto& leg = this->legs[0];
            const float length =
                glm::length(glm::vec3(this->globals[leg[0]][3] - this->globals[leg[1]][3])) +
                glm::length(glm::vec3(this->globals[leg[1]][3] - this->globals[leg[2]][3]));
            this->translate_hanging_roots(
                animation, glm::vec3(length * SHIFT_RATIO * sway * this->idle_weight, 0, 0));
            this->update_globals(model, animation);
            this->crouch(model, animation, CROUCH_WEIGHT * this->idle_weight);
            for (std::size_t side = 0; side < this->legs.size(); ++side) {
                auto foot = feet[side];
                const float phase = glm::two_pi<float>() * this->idle_time / FOOT_SECONDS +
                                    static_cast<float>(side) * glm::pi<float>();
                // Positive half-wave squared gives alternating, smooth foot adjustments.
                const float wave = (std::max)(0.0f, std::sin(phase));
                const float amount = wave * wave * this->idle_weight;
                const float outward = side == 0 ? 1.0f : -1.0f;
                foot[3].x += outward * length * FOOT_SHIFT_RATIO * amount;
                foot[3].y += length * FOOT_LIFT_RATIO * amount;
                this->bend_leg(model, animation, side, foot);
            }
            this->update_hanging_ik_targets(model, animation);
        }
    }

    void CursorPose::apply_idle_arms(
        const component::ModelComponent& model, component::AnimationComponent& animation) {
        for (std::size_t side = 0; side < this->wrists.size(); ++side) {
            if (!(this->wrists[side] < this->globals.size())) {
                continue;
            }
            const auto arm = this->joints[side == 0 ? LeftArm : RightArm];
            const auto elbow = this->joints[side == 0 ? LeftElbow : RightElbow];
            const auto upper = animation.animation[arm].rotation;
            const auto lower = animation.animation[elbow].rotation;
            const auto frame = this->arm_frame(side);
            // |hand-shoulder|/length = sqrt(down^2+out^2+forward^2), keeping a slight elbow bend.
            const auto hand =
                glm::vec3(this->globals[arm][3]) +
                frame.length * (-frame.up * HAND_DOWN_RATIO + frame.outward * HAND_OUT_RATIO +
                                   frame.forward * HAND_FORWARD_RATIO);
            this->solve_arm(model, animation, side, hand, frame);
            animation.animation[arm].rotation =
                glm::slerp(upper, animation.animation[arm].rotation, this->idle_weight);
            animation.animation[elbow].rotation =
                glm::slerp(lower, animation.animation[elbow].rotation, this->idle_weight);
            this->update_globals(model, animation);
        }
    }
} // namespace enishi::core
