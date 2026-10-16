#include "cursor_pose.h"
#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

namespace enishi::core {
    namespace {
        constexpr float EPSILON = 0.00001f;
        constexpr float RESPONSE = 8.0f;
        constexpr float MAX_DT = 0.05f;
        constexpr float HAND_RADIUS_RATIO = 0.18f;
        constexpr float SWAY_RATIO = 0.3f;
        constexpr float HEAD_CLEARANCE_RATIO = 0.05f;
        const glm::vec3 DOWN(0, -1, 0);

        glm::quat rotation(const glm::mat4& matrix) {
            auto axes = glm::mat3(matrix);
            for (glm::length_t column = 0; column < 3; ++column) {
                if (!(glm::length(axes[column]) > EPSILON)) {
                    return glm::quat(1, 0, 0, 0);
                }
                axes[column] = glm::normalize(axes[column]);
            }
            return glm::normalize(glm::quat_cast(axes));
        }

        glm::quat align(const glm::vec3& from, const glm::vec3& to) {
            if (!(glm::length(from) > EPSILON) || !(glm::length(to) > EPSILON)) {
                return glm::quat(1, 0, 0, 0);
            }
            const auto a = glm::normalize(from);
            const auto b = glm::normalize(to);
            if (glm::dot(a, b) < -1.0f + EPSILON) {
                auto axis = glm::cross(a, glm::vec3(1, 0, 0));
                if (!(glm::length(axis) > EPSILON)) {
                    axis = glm::cross(a, glm::vec3(0, 0, 1));
                }
                return glm::angleAxis(glm::pi<float>(), glm::normalize(axis));
            }
            // Shortest arc q = normalize((1 + dot(a,b), cross(a,b))).
            return glm::normalize(glm::quat(1 + glm::dot(a, b), glm::cross(a, b)));
        }
    } // namespace

    std::optional<std::pair<std::size_t, float>> CursorPose::pick_hand(
        const component::AnimationComponent& animation, const CursorRay& cursor) const {
        if (!this->globals_cached || this->hanging || this->hanging_weight > EPSILON ||
            !(this->joints[Head] < animation.global.size())) {
            return std::nullopt;
        }
        std::optional<std::pair<std::size_t, float>> result;
        const float squared = glm::dot(cursor.ray.direction, cursor.ray.direction);
        if (!(squared > EPSILON)) {
            return result;
        }
        for (std::size_t side = 0; side < this->wrists.size(); ++side) {
            if (!this->active_arms[side] || !(this->wrists[side] < animation.global.size())) {
                continue;
            }
            const auto hand = glm::vec3(animation.global[this->wrists[side]][3]);
            // Closest point on ray: t=dot(hand-origin,d)/dot(d,d).
            const float t = glm::dot(hand - cursor.ray.origin, cursor.ray.direction) / squared;
            const float radius = this->arm_frame(side).length * HAND_RADIUS_RATIO;
            const auto point = cursor.ray.origin + t * cursor.ray.direction;
            const float minimum_height = animation.global[this->joints[Head]][3].y +
                                         this->arm_frame(side).length * HEAD_CLEARANCE_RATIO;
            if (point.y > minimum_height && t > 0 &&
                glm::length(cursor.ray.origin + t * cursor.ray.direction - hand) < radius &&
                (!result.has_value() || t < result->second)) {
                result = std::pair{side, t};
            }
        }
        return result;
    }

    void CursorPose::begin_hanging(
        std::size_t side, const component::AnimationComponent& animation, const CursorRay& cursor) {
        if (!(side < this->wrists.size()) || !(this->wrists[side] < animation.global.size()) ||
            this->cached_local.size() != animation.animation.size() ||
            !(this->joints[Head] < animation.global.size())) {
            return;
        }
        this->hanging_side = side;
        this->hanging_pose = this->cached_local;
        this->hanging_anchor = glm::vec3(animation.global[this->wrists[side]][3]);
        this->hanging_normal = cursor.plane_normal;
        this->hanging_target = this->hanging_anchor;
        this->hanging_lag = this->hanging_anchor;
        this->hanging_weight = 0;
        // Keep the entry threshold fixed; moving the suspended head must not move the threshold.
        this->hanging_min_height = animation.global[this->joints[Head]][3].y +
                                   this->arm_frame(side).length * HEAD_CLEARANCE_RATIO;
        this->hanging = true;
        this->jump_height = 0;
        this->jump_wait = 0;
    }

    void CursorPose::end_hanging() {
        this->hanging = false;
    }
    bool CursorPose::is_hanging() const {
        return this->hanging;
    }

    bool CursorPose::suppress_hanging_ik(types::BoneIndex bone) const {
        return this->hanging_weight > EPSILON &&
               std::ranges::find(this->leg_ik_goals, bone) != this->leg_ik_goals.end();
    }

    void CursorPose::prepare_hanging_limbs(
        const component::ModelComponent& model, component::AnimationComponent& animation) {
        const auto elbow = this->hanging_side == 0 ? LeftElbow : RightElbow;
        // flex=0 straightens the support arm in its hinge plane without hyperextension.
        animation.animation[this->joints[elbow]].rotation =
            this->bind_rotations[elbow] * glm::angleAxis(-this->bind_flex[this->hanging_side],
                                              this->hinge_axes[this->hanging_side]);
        // Capturing a jumping/crouching pose also captures knee bends. Start dangling legs from
        // their bind axes; rotating only the hip would otherwise preserve the wrong bend plane.
        for (const auto& leg : this->legs) {
            for (const auto bone : leg) {
                if (bone < model.bind_bone.size()) {
                    animation.animation[bone].rotation = rotation(model.bind_bone[bone].local);
                }
            }
        }
        this->update_globals(model, animation);
    }

    void CursorPose::hang_limb(const component::ModelComponent& model,
        component::AnimationComponent& animation,
        types::BoneIndex start,
        types::BoneIndex end) {
        if (!(start < this->globals.size()) || !(end < this->globals.size())) {
            return;
        }
        const auto direction = glm::vec3(this->globals[end][3] - this->globals[start][3]);
        const auto q = rotation(this->globals[start]);
        // Convert a world-space gravity alignment into the bone's local rotation.
        animation.animation[start].rotation *= glm::inverse(q) * align(direction, DOWN) * q;
        this->update_globals(model, animation);
    }

    void CursorPose::update_hanging_ik_targets(
        const component::ModelComponent& model, component::AnimationComponent& animation) {
        // Foot IK targets must follow the dangling feet instead of pinning them to the ground.
        for (const auto& ik : model.iks) {
            const auto& ccd = std::get<types::CCDIK>(ik.method);
            if (std::ranges::find(this->leg_ik_goals, ccd.ik_bone) == this->leg_ik_goals.end()) {
                continue;
            }
            const auto parent = model.bone_node[ccd.ik_bone].parent;
            const auto base =
                this->external[ccd.ik_bone] *
                (parent < this->globals.size() ? this->globals[parent] : glm::mat4(1));
            if (ccd.target < this->globals.size() && std::abs(glm::determinant(base)) > EPSILON) {
                animation.animation[ccd.ik_bone].position =
                    glm::vec3(glm::inverse(base) * this->globals[ccd.target][3]);
            }
        }
    }

    bool CursorPose::apply_hanging(const component::ModelComponent& model,
        component::AnimationComponent& animation,
        const std::optional<CursorRay>& cursor,
        float seconds) {
        if (!this->hanging && !(this->hanging_weight > EPSILON)) {
            return false;
        }
        const float dt = std::isfinite(seconds) ? std::clamp(seconds, 0.0f, MAX_DT) : 0;
        // Exponential damping alpha=1-exp(-rate*dt), independent of frame rate.
        const float alpha = 1 - std::exp(-RESPONSE * dt);
        if (this->hanging && cursor.has_value()) {
            const float denominator = glm::dot(cursor->ray.direction, this->hanging_normal);
            if (std::abs(denominator) > EPSILON) {
                // Intersect the pointer with the fixed camera-facing plane through the grabbed
                // hand.
                const float t =
                    glm::dot(this->hanging_anchor - cursor->ray.origin, this->hanging_normal) /
                    denominator;
                if (t > 0 && std::isfinite(t)) {
                    this->hanging_target = cursor->ray.origin + t * cursor->ray.direction;
                }
            }
        }
        const bool above_head = this->hanging && this->hanging_target.y > this->hanging_min_height;
        this->hanging_weight = above_head ? (std::min)(1.0f, this->hanging_weight + RESPONSE * dt)
                                          : this->hanging_weight * (1 - alpha);
        if (!(this->hanging_weight > EPSILON)) {
            return false;
        }
        this->hanging_originals = animation.animation;
        animation.animation = this->hanging_pose;
        this->prepare_hanging_limbs(model, animation);
        const auto wrist = this->wrists[this->hanging_side];
        const auto hips = this->body_bones[BodyHips];
        const auto center = hips < this->globals.size()
                                ? glm::vec3(this->globals[hips][3])
                                : glm::vec3(this->globals[this->roots.front()][3]);
        const auto lever = center - glm::vec3(this->globals[wrist][3]);
        this->hanging_lag += alpha * (this->hanging_target - this->hanging_lag);
        // A lagging support creates sway; cap displacement relative to the suspension length.
        auto sway = this->hanging_lag - this->hanging_target;
        const float length = (std::max)(glm::length(lever), EPSILON);
        if (glm::length(sway) > length * SWAY_RATIO) {
            sway = glm::normalize(sway) * length * SWAY_RATIO;
        }
        const auto correction = align(lever, DOWN * length + sway);
        const auto origin = glm::vec3(this->globals[this->roots.front()][3]);
        for (const auto root : this->roots) {
            const auto axes = glm::mat3(this->external[root]);
            if (std::abs(glm::determinant(axes)) > EPSILON) {
                // Independent roots share the same pivot: p'=origin+R*(p-origin).
                const auto offset = glm::vec3(this->globals[root][3]) - origin;
                animation.animation[root].position +=
                    glm::inverse(axes) * (correction * offset - offset);
            }
            const auto q = rotation(this->globals[root]);
            animation.animation[root].rotation *= glm::inverse(q) * correction * q;
        }
        this->update_globals(model, animation);
        const auto free_side = 1 - this->hanging_side;
        this->hang_limb(model,
            animation,
            this->joints[free_side == 0 ? LeftArm : RightArm],
            this->wrists[free_side]);
        for (const auto& leg : this->legs) {
            this->hang_limb(model, animation, leg[0], leg[2]);
        }
        const auto delta = this->hanging_target - glm::vec3(this->globals[wrist][3]);
        for (const auto root : this->roots) {
            const auto axes = glm::mat3(this->external[root]);
            if (std::abs(glm::determinant(axes)) > EPSILON) {
                // G=external*T*R*S, so local displacement=inverse(external.axes)*world
                // displacement.
                animation.animation[root].position += glm::inverse(axes) * delta;
            }
        }
        for (std::size_t bone = 0; bone < animation.animation.size(); ++bone) {
            auto& pose = animation.animation[bone];
            const auto& original = this->hanging_originals[bone];
            pose.position = glm::mix(original.position, pose.position, this->hanging_weight);
            pose.scale = glm::mix(original.scale, pose.scale, this->hanging_weight);
            pose.rotation = glm::slerp(original.rotation, pose.rotation, this->hanging_weight);
        }
        this->update_globals(model, animation);
        this->update_hanging_ik_targets(model, animation);
        return true;
    }
} // namespace enishi::core
