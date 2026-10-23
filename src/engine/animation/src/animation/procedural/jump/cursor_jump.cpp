#include "cursor_jump.h"
#include <algorithm>
#include <animation/procedural/cursor/cursor_reach.h>
#include <animation/procedural/cursor/cursor_transforms.h>
#include <animation/procedural/legs/cursor_legs.h>
#include <cmath>

namespace enishi::animation {
    using enum CursorPoseState::Joint;
    using enum CursorPoseState::BodyBone;
    namespace {
        constexpr float EPSILON = 0.00001f;
        constexpr float REACH_MARGIN = 0.95f;
        constexpr float MAX_FRAME_SECONDS = 0.1f;
        constexpr float PREPARATION_SECONDS = 0.24f;
        constexpr float LANDING_SECONDS = 0.20f;
    } // namespace

    bool CursorJump::cursor_in_arm_reach(const CursorPoseState& state, std::size_t side) {
        if (!state.reach_ray.has_value()) {
            return false;
        }
        const auto& ray = *state.reach_ray;
        const float direction_squared = glm::dot(ray.direction, ray.direction);
        if (!(direction_squared > EPSILON)) {
            return false;
        }
        if (!(state.wrists[side] < state.transforms.globals.size())) {
            return false;
        }
        const auto shoulder =
            glm::vec3(state.transforms.globals[state.joints[side == 0 ? LeftArm : RightArm]][3]);
        const auto elbow = glm::vec3(
            state.transforms.globals[state.joints[side == 0 ? LeftElbow : RightElbow]][3]);
        const auto wrist = glm::vec3(state.transforms.globals[state.wrists[side]][3]);
        const float upper = glm::length(elbow - shoulder);
        const float lower = glm::length(wrist - elbow);
        if (!(upper > EPSILON) || !(lower > EPSILON)) {
            return false;
        }
        float hand = 0;
        for (const auto bone : state.hand_reach_bones[side]) {
            hand =
                (std::max)(hand, glm::length(glm::vec3(state.transforms.globals[bone][3]) - wrist));
        }
        // Cap attachments below the wrist so an accessory cannot enlarge the hand indefinitely.
        constexpr float MAX_HAND_FOREARM_RATIO = 0.5f;
        const float radius = upper + lower + (std::min)(hand, lower * MAX_HAND_FOREARM_RATIO);
        // The mouse has no depth. Test its ray against the fully extended arm sphere:
        // t=max(0,dot(shoulder-origin,d)/dot(d,d)), distance=|origin+t*d-shoulder|.
        // This avoids treating the arbitrary target-plane depth as a reason to jump.
        const float along =
            (std::max)(0.0f, glm::dot(shoulder - ray.origin, ray.direction) / direction_squared);
        const float distance = glm::length(ray.origin + ray.direction * along - shoulder);
        return !(distance > radius + EPSILON);
    }

    float CursorJump::desired_jump_height(const CursorPoseState& state, const glm::vec3& point) {
        if (!state.config.jump_enabled || state.transforms.globals.empty() ||
            CursorReach::can_reach(state, 0, point) || CursorReach::can_reach(state, 1, point) ||
            CursorJump::cursor_in_arm_reach(state, 0) ||
            CursorJump::cursor_in_arm_reach(state, 1)) {
            return 0;
        }
        float low = state.transforms.globals.front()[3].y;
        float high = low;
        for (const auto& matrix : state.transforms.globals) {
            low = (std::min)(low, matrix[3].y);
            high = (std::max)(high, matrix[3].y);
        }
        // Maximum lift scales with the current skeleton: Hmax = height * ratio.
        const float maximum = (high - low) * state.config.jump_height_ratio;
        float result = 0;
        for (std::size_t side = 0; side < state.wrists.size(); ++side) {
            if (!(state.wrists[side] < state.transforms.globals.size())) {
                continue;
            }
            const auto shoulder = glm::vec3(
                state.transforms.globals[state.joints[side == 0 ? LeftArm : RightArm]][3]);
            const auto elbow = glm::vec3(
                state.transforms.globals[state.joints[side == 0 ? LeftElbow : RightElbow]][3]);
            const auto wrist = glm::vec3(state.transforms.globals[state.wrists[side]][3]);
            const float radius =
                (glm::length(elbow - shoulder) + glm::length(wrist - elbow)) * REACH_MARGIN;
            const auto delta = point - shoulder;
            const float horizontal_squared = delta.x * delta.x + delta.z * delta.z;
            if (!(delta.y > 0) || !(horizontal_squared < radius * radius)) {
                continue;
            }
            // |target-(shoulder+Y*h)|=r gives h=dy-sqrt(r*r-dx*dx-dz*dz).
            const float lift = delta.y - std::sqrt(radius * radius - horizontal_squared);
            if (lift > EPSILON && !(lift > maximum) &&
                CursorReach::can_reach(state, side, point - glm::vec3(0, lift, 0)) &&
                (!(result > 0) || lift < result)) {
                result = lift;
            }
        }
        return result;
    }

    void CursorJump::apply_jump(CursorPoseState& state,
        const component::ModelComponent& model,
        component::AnimationComponent& animation,
        const std::optional<glm::vec3>& point,
        float seconds) {
        for (std::size_t index = 0; index < state.locomotion.roots.size(); ++index) {
            state.locomotion.original_root_positions[index] =
                animation.animation[state.locomotion.roots[index]].position;
        }
        for (auto& [index, original] : state.locomotion.leg_originals) {
            original = animation.animation[index];
        }
        const bool has_legs = state.locomotion.legs[0][0] < state.transforms.globals.size() &&
                              state.locomotion.legs[1][0] < state.transforms.globals.size();
        const float landing = has_legs ? LANDING_SECONDS : 0;
        // Bound elapsed simulation time after a stall, avoiding a teleport through the flight.
        const float dt = std::isfinite(seconds) ? std::clamp(seconds, 0.0f, MAX_FRAME_SECONDS) : 0;
        if (state.locomotion.height > 0) {
            state.locomotion.time += dt;
            if (!(state.locomotion.time < state.config.jump_duration + landing)) {
                state.locomotion.height = 0;
                state.locomotion.rest = state.config.jump_cooldown;
                state.locomotion.wait = 0;
            }
        } else if (state.locomotion.rest > 0) {
            state.locomotion.rest = (std::max)(0.0f, state.locomotion.rest - dt);
            state.locomotion.wait = 0;
        } else {
            const float desired =
                point.has_value() ? CursorJump::desired_jump_height(state, *point) : 0;
            state.locomotion.wait = desired > 0 ? state.locomotion.wait + dt : 0;
            if (desired > 0 && !(state.locomotion.wait < state.config.jump_delay)) {
                state.locomotion.height = desired;
                state.locomotion.time = has_legs ? -PREPARATION_SECONDS : 0;
                state.locomotion.wait = 0;
            }
        }
        if (!(state.locomotion.height > 0)) {
            return;
        }
        // Ballistic arc y=4*H*u*(1-u), u=t/T; exactly zero at takeoff and landing.
        const float phase =
            std::clamp(state.locomotion.time / state.config.jump_duration, 0.0f, 1.0f);
        const glm::vec3 lift(0, 4.0f * state.locomotion.height * phase * (1.0f - phase), 0);
        for (const auto root : state.locomotion.roots) {
            const auto axes = glm::mat3(state.transforms.external[root]);
            if (!(std::abs(glm::determinant(axes)) > EPSILON)) {
                continue;
            }
            // External transforms precede local translation: delta_local = inverse(E) *
            // delta_world.
            animation.animation[root].position += glm::inverse(axes) * lift;
        }
        CursorTransforms::update_globals(state.transforms, model, animation);
        float bend = 0;
        if (state.locomotion.time < 0) {
            bend =
                std::sin(glm::pi<float>() * (1.0f + state.locomotion.time / PREPARATION_SECONDS));
        } else if (state.locomotion.time > state.config.jump_duration && has_legs) {
            bend = std::sin(
                glm::pi<float>() * (state.locomotion.time - state.config.jump_duration) / landing);
        }
        // sin²(pi*t/T) gives zero bend and zero bend velocity at both ends of each phase.
        CursorLegs::crouch(state, model, animation, bend * bend);
    }
} // namespace enishi::animation
