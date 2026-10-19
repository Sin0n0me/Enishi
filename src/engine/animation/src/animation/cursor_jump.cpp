#include "cursor_pose.h"
#include <algorithm>
#include <cmath>

namespace enishi::animation {
    namespace {
        constexpr float EPSILON = 0.00001f;
        constexpr float REACH_MARGIN = 0.95f;
        constexpr float MAX_FRAME_SECONDS = 0.1f;
        constexpr float PREPARATION_SECONDS = 0.24f;
        constexpr float LANDING_SECONDS = 0.20f;
    } // namespace

    bool CursorPose::cursor_in_arm_reach(std::size_t side) const {
        if (!this->reach_ray.has_value()) {
            return false;
        }
        const auto& ray = *this->reach_ray;
        const float direction_squared = glm::dot(ray.direction, ray.direction);
        if (!(direction_squared > EPSILON)) {
            return false;
        }
        if (!(this->wrists[side] < this->globals.size())) {
            return false;
        }
        const auto shoulder =
            glm::vec3(this->globals[this->joints[side == 0 ? LeftArm : RightArm]][3]);
        const auto elbow =
            glm::vec3(this->globals[this->joints[side == 0 ? LeftElbow : RightElbow]][3]);
        const auto wrist = glm::vec3(this->globals[this->wrists[side]][3]);
        const float upper = glm::length(elbow - shoulder);
        const float lower = glm::length(wrist - elbow);
        if (!(upper > EPSILON) || !(lower > EPSILON)) {
            return false;
        }
        float hand = 0;
        for (const auto bone : this->hand_reach_bones[side]) {
            hand = (std::max)(hand, glm::length(glm::vec3(this->globals[bone][3]) - wrist));
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

    float CursorPose::desired_jump_height(const glm::vec3& point) const {
        if (!this->config.jump_enabled || this->globals.empty() || this->can_reach(0, point) ||
            this->can_reach(1, point) || this->cursor_in_arm_reach(0) ||
            this->cursor_in_arm_reach(1)) {
            return 0;
        }
        float low = this->globals.front()[3].y;
        float high = low;
        for (const auto& matrix : this->globals) {
            low = (std::min)(low, matrix[3].y);
            high = (std::max)(high, matrix[3].y);
        }
        // Maximum lift scales with the current skeleton: Hmax = height * ratio.
        const float maximum = (high - low) * this->config.jump_height_ratio;
        float result = 0;
        for (std::size_t side = 0; side < this->wrists.size(); ++side) {
            if (!(this->wrists[side] < this->globals.size())) {
                continue;
            }
            const auto shoulder =
                glm::vec3(this->globals[this->joints[side == 0 ? LeftArm : RightArm]][3]);
            const auto elbow =
                glm::vec3(this->globals[this->joints[side == 0 ? LeftElbow : RightElbow]][3]);
            const auto wrist = glm::vec3(this->globals[this->wrists[side]][3]);
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
                this->can_reach(side, point - glm::vec3(0, lift, 0)) &&
                (!(result > 0) || lift < result)) {
                result = lift;
            }
        }
        return result;
    }

    void CursorPose::apply_jump(const component::ModelComponent& model,
        component::AnimationComponent& animation,
        const std::optional<glm::vec3>& point,
        float seconds) {
        for (std::size_t index = 0; index < this->roots.size(); ++index) {
            this->original_root_positions[index] = animation.animation[this->roots[index]].position;
        }
        for (auto& [index, original] : this->leg_originals) {
            original = animation.animation[index];
        }
        const bool has_legs =
            this->legs[0][0] < this->globals.size() && this->legs[1][0] < this->globals.size();
        const float landing = has_legs ? LANDING_SECONDS : 0;
        // Bound elapsed simulation time after a stall, avoiding a teleport through the flight.
        const float dt = std::isfinite(seconds) ? std::clamp(seconds, 0.0f, MAX_FRAME_SECONDS) : 0;
        if (this->jump_height > 0) {
            this->jump_time += dt;
            if (!(this->jump_time < this->config.jump_duration + landing)) {
                this->jump_height = 0;
                this->jump_rest = this->config.jump_cooldown;
                this->jump_wait = 0;
            }
        } else if (this->jump_rest > 0) {
            this->jump_rest = (std::max)(0.0f, this->jump_rest - dt);
            this->jump_wait = 0;
        } else {
            const float desired = point.has_value() ? this->desired_jump_height(*point) : 0;
            this->jump_wait = desired > 0 ? this->jump_wait + dt : 0;
            if (desired > 0 && !(this->jump_wait < this->config.jump_delay)) {
                this->jump_height = desired;
                this->jump_time = has_legs ? -PREPARATION_SECONDS : 0;
                this->jump_wait = 0;
            }
        }
        if (!(this->jump_height > 0)) {
            return;
        }
        // Ballistic arc y=4*H*u*(1-u), u=t/T; exactly zero at takeoff and landing.
        const float phase = std::clamp(this->jump_time / this->config.jump_duration, 0.0f, 1.0f);
        const glm::vec3 lift(0, 4.0f * this->jump_height * phase * (1.0f - phase), 0);
        for (const auto root : this->roots) {
            const auto axes = glm::mat3(this->external[root]);
            if (!(std::abs(glm::determinant(axes)) > EPSILON)) {
                continue;
            }
            // External transforms precede local translation: delta_local = inverse(E) *
            // delta_world.
            animation.animation[root].position += glm::inverse(axes) * lift;
        }
        this->update_globals(model, animation);
        float bend = 0;
        if (this->jump_time < 0) {
            bend = std::sin(glm::pi<float>() * (1.0f + this->jump_time / PREPARATION_SECONDS));
        } else if (this->jump_time > this->config.jump_duration && has_legs) {
            bend = std::sin(
                glm::pi<float>() * (this->jump_time - this->config.jump_duration) / landing);
        }
        // sin²(pi*t/T) gives zero bend and zero bend velocity at both ends of each phase.
        this->crouch(model, animation, bend * bend);
    }
} // namespace enishi::animation
