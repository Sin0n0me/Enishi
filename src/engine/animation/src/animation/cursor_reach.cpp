#include "cursor_pose.h"
#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <string_view>

namespace enishi::animation {
    namespace {
        constexpr float EPSILON = 0.00001f;
        constexpr glm::quat IDENTITY(1, 0, 0, 0);
        constexpr float MAX_FLEX = 135.0f;
        constexpr float MIN_FLEX = 8.0f;
        constexpr float MAX_SWING = 100.0f;
        constexpr float MAX_TWIST = 80.0f;
        constexpr float TORSO_RADIUS_RATIO = 0.32f;
        constexpr float HEAD_RADIUS_RATIO = 0.32f;
        constexpr float ARM_MARGIN_RATIO = 0.1f;
        constexpr float HAND_MARGIN_RATIO = 0.45f;
        constexpr float HEAD_HEIGHT_RATIO = 0.55f;
        constexpr float TORSO_HEIGHT_RATIO = 0.8f;
        constexpr float FRONT_REACH_RATIO = 0.15f;
        constexpr float OUTWARD_POLE_WEIGHT = 1.0f;
        constexpr float DOWNWARD_POLE_WEIGHT = 0.45f;
        constexpr float AVOID_FORWARD_STEP = 0.1f;
        constexpr float AVOID_SIDE_STEP = 0.04f;
        constexpr std::size_t AVOID_ATTEMPTS = 12;
        constexpr float REACH_RELEASE_RATIO = 1.05f;
        constexpr float CROSS_BODY_RATIO = 0.5f;
        constexpr float CROSS_BODY_RELEASE_RATIO = 0.6f;

        glm::quat rotation_of(const glm::mat4& transform) {
            glm::mat3 axes(transform);
            for (glm::length_t column = 0; column < axes.length(); ++column) {
                if (!(glm::length(axes[column]) > EPSILON)) {
                    return IDENTITY;
                }
                axes[column] = glm::normalize(axes[column]);
            }
            return glm::normalize(glm::quat_cast(axes));
        }

        glm::quat between(glm::vec3 from, glm::vec3 to) {
            if (!(glm::length(from) > EPSILON) || !(glm::length(to) > EPSILON)) {
                return IDENTITY;
            }
            from = glm::normalize(from);
            to = glm::normalize(to);
            const float cosine = std::clamp(glm::dot(from, to), -1.0f, 1.0f);
            if (cosine < -1.0f + EPSILON) {
                auto axis = glm::cross(from, glm::vec3(0, 1, 0));
                if (!(glm::length(axis) > EPSILON)) {
                    axis = glm::cross(from, glm::vec3(1, 0, 0));
                }
                return glm::angleAxis(glm::pi<float>(), glm::normalize(axis));
            }
            // Shortest arc q = normalize((1 + dot(a,b), cross(a,b))).
            return glm::normalize(glm::quat(1.0f + cosine, glm::cross(from, to)));
        }

        float twist_angle(glm::quat rotation, const glm::vec3& axis) {
            if (rotation.w < 0) {
                rotation = -rotation;
            }
            // Project q's vector part onto the hinge axis; signed twist = 2*atan2(v.axis,w).
            const float projected = glm::dot(glm::vec3(rotation.x, rotation.y, rotation.z), axis);
            return 2.0f * std::atan2(projected, rotation.w);
        }

        glm::quat limit_shoulder(glm::quat rotation, const glm::vec3& axis) {
            const float angle = twist_angle(rotation, axis);
            const auto twist = glm::angleAxis(angle, axis);
            auto swing = glm::normalize(rotation * glm::inverse(twist));
            if (swing.w < 0) {
                swing = -swing;
            }
            // q = swing * twist. Limit swing from the bind arm and twist around its length.
            const float swing_angle = 2.0f * std::acos(std::clamp(swing.w, 0.0f, 1.0f));
            if (swing_angle > glm::radians(MAX_SWING)) {
                swing = glm::slerp(IDENTITY, swing, glm::radians(MAX_SWING) / swing_angle);
            }
            return swing *
                   glm::angleAxis(
                       std::clamp(angle, -glm::radians(MAX_TWIST), glm::radians(MAX_TWIST)), axis);
        }

        float segment_distance_squared(
            const glm::vec3& p, const glm::vec3& q, const glm::vec3& start, const glm::vec3& end) {
            const auto u = q - p;
            const auto v = end - start;
            const auto r = p - start;
            const float a = glm::dot(u, u);
            const float e = glm::dot(v, v);
            const float b = glm::dot(u, v);
            const float c = glm::dot(u, r);
            const float f = glm::dot(v, r);
            float s = 0;
            float t = 0;
            // Minimize |r+s*u-t*v|² on [0,1]², clamping and resolving boundary minima.
            if (a > EPSILON && e > EPSILON) {
                const float denominator = a * e - b * b;
                if (denominator > EPSILON) {
                    s = std::clamp((b * f - c * e) / denominator, 0.0f, 1.0f);
                }
                t = (b * s + f) / e;
                if (t < 0 || t > 1) {
                    t = std::clamp(t, 0.0f, 1.0f);
                    s = std::clamp((b * t - c) / a, 0.0f, 1.0f);
                }
            } else if (a > EPSILON) {
                s = std::clamp(-c / a, 0.0f, 1.0f);
            } else if (e > EPSILON) {
                t = std::clamp(f / e, 0.0f, 1.0f);
            }
            const auto separation = r + s * u - t * v;
            return glm::dot(separation, separation);
        }
    } // namespace

    void CursorPose::initialize_arms(const component::ModelComponent& model) {
        this->bind_rotations.fill(IDENTITY);
        this->hinge_axes.fill(glm::vec3(0, 1, 0));
        this->upper_axes.fill(glm::vec3(1, 0, 0));
        this->body_bones.fill(types::INVALID_BONE_INDEX);
        const auto find = [&](std::initializer_list<std::string_view> names) {
            for (const auto name : names) {
                const auto found = std::ranges::find(model.bone_names, name);
                if (found != model.bone_names.end()) {
                    return static_cast<types::BoneIndex>(found - model.bone_names.begin());
                }
            }
            return types::INVALID_BONE_INDEX;
        };
        this->body_bones = {find({"左腕", "leftUpperArm", "LeftArm"}),
            find({"右腕", "rightUpperArm", "RightArm"}),
            find({"首", "neck", "Neck"}),
            find({"頭", "head", "Head"}),
            find({"下半身", "hips", "Hips"})};
        auto ancestor = this->body_bones[BodyLeft];
        if (!(ancestor < model.bone_node.size())) {
            ancestor = this->body_bones[BodyRight];
        }
        for (std::size_t step = 0;
             step < model.bone_node.size() && ancestor < model.bone_node.size();
             ++step) {
            ancestor = model.bone_node[ancestor].parent;
            auto other = this->body_bones[BodyRight];
            for (std::size_t depth = 0;
                 depth < model.bone_node.size() && other < model.bone_node.size();
                 ++depth) {
                if (other == ancestor) {
                    this->body_anchor = ancestor;
                    break;
                }
                other = model.bone_node[other].parent;
            }
            if (this->body_anchor < model.bone_node.size()) {
                break;
            }
        }
        if (this->body_anchor < model.bind_bone.size()) {
            this->body_bind_inverse = glm::mat3(model.bind_bone[this->body_anchor].global_inverse);
        }
        if (this->body_bones[BodyHead] < model.bind_bone.size()) {
            this->head_local_up =
                glm::mat3(model.bind_bone[this->body_bones[BodyHead]].global_inverse) *
                glm::vec3(0, 1, 0);
        }
        for (std::size_t joint = 0; joint < Count; ++joint) {
            if (this->joints[joint] < model.bind_bone.size()) {
                this->bind_rotations[joint] =
                    rotation_of(model.bind_bone[this->joints[joint]].local);
            }
        }
        for (std::size_t side = 0; side < this->wrists.size(); ++side) {
            const auto arm = side == 0 ? LeftArm : RightArm;
            const auto elbow = side == 0 ? LeftElbow : RightElbow;
            if (!(this->wrists[side] < model.bind_bone.size()) ||
                !(this->joints[arm] < model.bind_bone.size()) ||
                !(this->joints[elbow] < model.bind_bone.size())) {
                continue;
            }
            const auto& shoulder = model.bind_bone[this->joints[arm]];
            const auto& joint = model.bind_bone[this->joints[elbow]];
            const auto upper = glm::vec3(joint.global[3] - shoulder.global[3]);
            const auto lower =
                glm::vec3(model.bind_bone[this->wrists[side]].global[3] - joint.global[3]);
            auto axis = glm::cross(upper, glm::vec3(0, 0, -1));
            if (!(glm::length(upper) > EPSILON) || !(glm::length(lower) > EPSILON) ||
                !(glm::length(axis) > EPSILON)) {
                continue;
            }
            axis = glm::normalize(axis);
            this->hinge_axes[side] = glm::normalize(glm::mat3(joint.global_inverse) * axis);
            this->upper_axes[side] = glm::normalize(glm::mat3(shoulder.global_inverse) * upper);
            // Signed bind flex measured around the forward-bending anatomical axis.
            this->bind_flex[side] =
                std::atan2(glm::dot(glm::cross(glm::normalize(upper), glm::normalize(lower)), axis),
                    glm::dot(glm::normalize(upper), glm::normalize(lower)));
        }
    }

    bool CursorPose::can_reach(std::size_t side, const glm::vec3& target) const {
        if (!(this->wrists[side] < this->globals.size())) {
            return false;
        }
        const auto arm = this->joints[side == 0 ? LeftArm : RightArm];
        const auto elbow = this->joints[side == 0 ? LeftElbow : RightElbow];
        const auto shoulder = glm::vec3(this->globals[arm][3]);
        const auto joint = glm::vec3(this->globals[elbow][3]);
        const float upper = glm::length(joint - shoulder);
        const float lower = glm::length(glm::vec3(this->globals[this->wrists[side]][3]) - joint);
        // Each arm owns its range: |target-shoulder| < upper+lower. A 5% release
        // margin avoids repeatedly entering/leaving when the cursor grazes the boundary.
        const float release = this->active_arms[side] ? REACH_RELEASE_RATIO : 1.0f;
        if (!(upper > EPSILON) || !(lower > EPSILON) ||
            !(glm::length(target - shoulder) < (upper + lower) * release)) {
            return false;
        }
        if (this->body_bones[BodyLeft] < this->globals.size() &&
            this->body_bones[BodyRight] < this->globals.size()) {
            const auto left = glm::vec3(this->globals[this->body_bones[BodyLeft]][3]);
            const auto right = glm::vec3(this->globals[this->body_bones[BodyRight]][3]);
            const auto across = left - right;
            const float width = glm::length(across);
            if (width > EPSILON) {
                // Project onto the shoulder axis; both ranges overlap across the torso.
                // s=dot(target-midpoint, outward), allowed when s > -width*margin.
                const float sign = side == 0 ? 1.0f : -1.0f;
                const float lateral =
                    sign * glm::dot(target - (left + right) * 0.5f, across / width);
                const float margin =
                    this->active_arms[side] ? CROSS_BODY_RELEASE_RATIO : CROSS_BODY_RATIO;
                return lateral > -width * margin;
            }
        }
        return true;
    }

    CursorPose::ArmFrame CursorPose::arm_frame(std::size_t side) const {
        const auto arm = this->joints[side == 0 ? LeftArm : RightArm];
        const auto elbow = this->joints[side == 0 ? LeftElbow : RightElbow];
        const auto shoulder = glm::vec3(this->globals[arm][3]);
        const float upper = glm::length(glm::vec3(this->globals[elbow][3]) - shoulder);
        const float lower =
            glm::length(glm::vec3(this->globals[this->wrists[side]][3] - this->globals[elbow][3]));
        const auto basis =
            this->body_anchor < this->globals.size()
                ? glm::mat3(this->globals[this->body_anchor]) * this->body_bind_inverse
                : glm::mat3(1);
        ArmFrame frame{shoulder,
            glm::normalize(basis * glm::vec3(1, 0, 0)),
            glm::normalize(basis * glm::vec3(0, 1, 0)),
            glm::normalize(basis * glm::vec3(0, 0, -1)),
            upper + lower,
            lower * ARM_MARGIN_RATIO,
            lower * HAND_MARGIN_RATIO,
            {}};
        float width = upper;
        if (this->body_bones[BodyLeft] < this->globals.size() &&
            this->body_bones[BodyRight] < this->globals.size()) {
            const auto left = glm::vec3(this->globals[this->body_bones[BodyLeft]][3]);
            const auto right = glm::vec3(this->globals[this->body_bones[BodyRight]][3]);
            frame.center = (left + right) * 0.5f;
            width = glm::length(left - right);
        } else {
            const float sign = side == 0 ? 1.0f : -1.0f;
            frame.center -= frame.outward * sign * width * 0.5f;
        }
        frame.outward *= side == 0 ? 1.0f : -1.0f;
        const auto hips = this->body_bones[BodyHips] < this->globals.size()
                              ? glm::vec3(this->globals[this->body_bones[BodyHips]][3])
                              : frame.center - frame.up * frame.length * TORSO_HEIGHT_RATIO;
        const auto neck = this->body_bones[BodyNeck] < this->globals.size()
                              ? glm::vec3(this->globals[this->body_bones[BodyNeck]][3])
                              : frame.center;
        frame.obstacles.push_back({hips, neck, width * TORSO_RADIUS_RATIO});
        if (this->body_bones[BodyHead] < this->globals.size()) {
            const auto head = glm::vec3(this->globals[this->body_bones[BodyHead]][3]);
            const auto head_up = glm::normalize(
                glm::mat3(this->globals[this->body_bones[BodyHead]]) * this->head_local_up);
            frame.obstacles.push_back({head,
                head + head_up * lower * HEAD_HEIGHT_RATIO,
                (std::max)(width, lower) * HEAD_RADIUS_RATIO});
        }
        return frame;
    }

    bool CursorPose::arm_clear(std::size_t side, const ArmFrame& frame) const {
        const auto arm = this->joints[side == 0 ? LeftArm : RightArm];
        const auto elbow = this->joints[side == 0 ? LeftElbow : RightElbow];
        const auto shoulder = glm::vec3(this->globals[arm][3]);
        const auto joint = glm::vec3(this->globals[elbow][3]);
        const auto wrist = glm::vec3(this->globals[this->wrists[side]][3]);
        for (const auto& capsule : frame.obstacles) {
            // Inflate centerline tests by limb thickness, and the wrist by the hand's extent.
            const float squared =
                (capsule.radius + frame.arm_margin) * (capsule.radius + frame.arm_margin);
            const float hand_squared =
                (capsule.radius + frame.hand_margin) * (capsule.radius + frame.hand_margin);
            if (segment_distance_squared(shoulder, joint, capsule.start, capsule.end) < squared ||
                segment_distance_squared(joint, wrist, capsule.start, capsule.end) < squared ||
                segment_distance_squared(wrist, wrist, capsule.start, capsule.end) < hand_squared) {
                return false;
            }
        }
        const std::size_t other = 1 - side;
        if (this->wrists[other] < this->globals.size()) {
            const auto other_elbow = this->joints[other == 0 ? LeftElbow : RightElbow];
            const auto other_joint = glm::vec3(this->globals[other_elbow][3]);
            const auto other_wrist = glm::vec3(this->globals[this->wrists[other]][3]);
            const auto other_frame = this->arm_frame(other);
            const float limb_clearance = frame.arm_margin + other_frame.arm_margin;
            const float hand_clearance = frame.hand_margin + other_frame.hand_margin;
            const float first_hand_clearance = frame.hand_margin + other_frame.arm_margin;
            const float second_hand_clearance = other_frame.hand_margin + frame.arm_margin;
            // Capsules overlap when segment distance squared < (r1+r2)^2.
            if (segment_distance_squared(joint, wrist, other_joint, other_wrist) <
                    limb_clearance * limb_clearance ||
                glm::dot(wrist - other_wrist, wrist - other_wrist) <
                    hand_clearance * hand_clearance ||
                segment_distance_squared(wrist, wrist, other_joint, other_wrist) <
                    first_hand_clearance * first_hand_clearance ||
                segment_distance_squared(other_wrist, other_wrist, joint, wrist) <
                    second_hand_clearance * second_hand_clearance) {
                return false;
            }
        }
        return this->mesh_arm_clear(side);
    }

    void CursorPose::constrain_arm(
        component::AnimationComponent& animation, std::size_t side) const {
        const auto arm = side == 0 ? LeftArm : RightArm;
        const auto elbow = side == 0 ? LeftElbow : RightElbow;
        auto& upper = animation.animation[this->joints[arm]].rotation;
        auto& lower = animation.animation[this->joints[elbow]].rotation;
        upper = glm::normalize(this->bind_rotations[arm] *
                               limit_shoulder(glm::inverse(this->bind_rotations[arm]) * upper,
                                   this->upper_axes[side]));
        const float flex =
            twist_angle(glm::inverse(this->bind_rotations[elbow]) * lower, this->hinge_axes[side]);
        lower = this->bind_rotations[elbow] *
                glm::angleAxis(std::clamp(flex,
                                   -this->bind_flex[side],
                                   glm::radians(MAX_FLEX) - this->bind_flex[side]),
                    this->hinge_axes[side]);
    }

    void CursorPose::solve_arm(const component::ModelComponent& model,
        component::AnimationComponent& animation,
        std::size_t side,
        const glm::vec3& target,
        const ArmFrame& frame) {
        const auto arm = side == 0 ? LeftArm : RightArm;
        const auto elbow = side == 0 ? LeftElbow : RightElbow;
        const auto shoulder = glm::vec3(this->globals[this->joints[arm]][3]);
        const auto joint = glm::vec3(this->globals[this->joints[elbow]][3]);
        const float upper = glm::length(joint - shoulder);
        const float lower = glm::length(glm::vec3(this->globals[this->wrists[side]][3]) - joint);
        if (!(upper > EPSILON) || !(lower > EPSILON) ||
            !(glm::length(target - shoulder) > EPSILON)) {
            return;
        }
        // d²=a²+b²+2ab*cos(flex). Clamp distance using flex limits, not an almost-folded arm.
        const float minimum = std::sqrt(
            upper * upper + lower * lower + 2 * upper * lower * std::cos(glm::radians(MAX_FLEX)));
        const float maximum = std::sqrt(
            upper * upper + lower * lower + 2 * upper * lower * std::cos(glm::radians(MIN_FLEX)));
        const float distance = std::clamp(glm::length(target - shoulder), minimum, maximum);
        const auto direction = glm::normalize(target - shoulder);
        const auto preferred =
            frame.outward * OUTWARD_POLE_WEIGHT - frame.up * DOWNWARD_POLE_WEIGHT;
        auto pole = preferred - direction * glm::dot(preferred, direction);
        if (!(glm::length(pole) > EPSILON)) {
            pole = frame.forward - direction * glm::dot(frame.forward, direction);
        }
        if (!(glm::length(pole) > EPSILON)) {
            return;
        }
        // Triangle projection x=(a²-b²+d²)/(2d), h=sqrt(a²-x²); outward pole avoids the chest.
        const float along = (upper * upper - lower * lower + distance * distance) / (2 * distance);
        const float height = std::sqrt((std::max)(0.0f, upper * upper - along * along));
        const auto desired_upper = direction * along + glm::normalize(pole) * height;
        const auto desired_lower = direction * distance - desired_upper;
        const float flex = std::acos(
            std::clamp((distance * distance - upper * upper - lower * lower) / (2 * upper * lower),
                -1.0f,
                1.0f));
        animation.animation[this->joints[elbow]].rotation =
            this->bind_rotations[elbow] *
            glm::angleAxis(flex - this->bind_flex[side], this->hinge_axes[side]);
        this->update_globals(model, animation);
        const auto current_upper = glm::vec3(this->globals[this->joints[elbow]][3]) - shoulder;
        const auto current_lower =
            glm::vec3(this->globals[this->wrists[side]][3] - this->globals[this->joints[elbow]][3]);
        auto correction = between(current_upper, desired_upper);
        const auto axis = glm::normalize(desired_upper);
        const auto from = glm::cross(correction * current_upper, correction * current_lower);
        const auto to = glm::cross(desired_upper, desired_lower);
        if (glm::length(from) > EPSILON && glm::length(to) > EPSILON) {
            // Align the bend planes by twisting the shoulder, keeping the elbow a single hinge.
            const float twist =
                std::atan2(glm::dot(axis, glm::cross(glm::normalize(from), glm::normalize(to))),
                    glm::dot(glm::normalize(from), glm::normalize(to)));
            correction = glm::angleAxis(twist, axis) * correction;
        }
        const auto global_rotation = rotation_of(this->globals[this->joints[arm]]);
        animation.animation[this->joints[arm]].rotation *=
            glm::inverse(global_rotation) * correction * global_rotation;
        this->constrain_arm(animation, side);
        this->update_globals(model, animation);
    }

    bool CursorPose::reach_side(const component::ModelComponent& model,
        component::AnimationComponent& animation,
        std::size_t side,
        const glm::vec3& target) {
        const auto frame = this->arm_frame(side);
        auto safe_target = target;
        // Never chase a cursor through or behind the body. Shift the goal forward instead.
        safe_target += frame.forward * (std::max)(0.0f,
                                           frame.length * FRONT_REACH_RATIO -
                                               glm::dot(safe_target - frame.center, frame.forward));
        const auto arm = this->joints[side == 0 ? LeftArm : RightArm];
        const auto elbow = this->joints[side == 0 ? LeftElbow : RightElbow];
        const auto original_upper = animation.animation[arm].rotation;
        const auto original_lower = animation.animation[elbow].rotation;
        for (std::size_t attempt = 0; attempt < AVOID_ATTEMPTS; ++attempt) {
            animation.animation[arm].rotation = original_upper;
            animation.animation[elbow].rotation = original_lower;
            this->update_globals(model, animation);
            // Search forward and outward if either complete arm segment intersects a capsule.
            const auto candidate = safe_target + static_cast<float>(attempt) * frame.length *
                                                     (frame.forward * AVOID_FORWARD_STEP +
                                                         frame.outward * AVOID_SIDE_STEP);
            this->solve_arm(model, animation, side, candidate, frame);
            if (this->arm_clear(side, frame)) {
                return true;
            }
        }
        animation.animation[arm].rotation = original_upper;
        animation.animation[elbow].rotation = original_lower;
        this->update_globals(model, animation);
        return false;
    }

    void CursorPose::limit_arm_speed(const component::ModelComponent& model,
        component::AnimationComponent& animation,
        const std::array<glm::quat, Count>& previous,
        float seconds) {
        const float elapsed = std::isfinite(seconds) ? (std::max)(0.0f, seconds) : 0.0f;
        const float step = glm::radians(this->config.arm_speed) * elapsed;
        constexpr std::size_t BACKTRACK_STEPS = 12;
        for (std::size_t side = 0; side < this->wrists.size(); ++side) {
            const auto arm = side == 0 ? LeftArm : RightArm;
            const auto elbow = side == 0 ? LeftElbow : RightElbow;
            if (!(this->wrists[side] < this->globals.size())) {
                continue;
            }
            const auto desired_upper = this->offsets[arm];
            const auto desired_lower = this->offsets[elbow];
            // Shortest quaternion arc: theta = 2*atan2(|q.xyz|, |q.w|).
            // Both joints use t=min(1, speed*dt/max(theta)), preserving the solved bend.
            const auto upper_delta = glm::inverse(previous[arm]) * desired_upper;
            const auto lower_delta = glm::inverse(previous[elbow]) * desired_lower;
            const float upper_angle =
                2.0f *
                std::atan2(glm::length(glm::vec3(upper_delta.x, upper_delta.y, upper_delta.z)),
                    std::abs(upper_delta.w));
            const float lower_angle =
                2.0f *
                std::atan2(glm::length(glm::vec3(lower_delta.x, lower_delta.y, lower_delta.z)),
                    std::abs(lower_delta.w));
            const float angle = (std::max)(upper_angle, lower_angle);
            if (!(angle > step)) {
                continue;
            }
            const auto set_offsets = [&](const glm::quat& upper, const glm::quat& lower) {
                this->offsets[arm] = upper;
                this->offsets[elbow] = lower;
                animation.animation[this->joints[arm]].rotation =
                    glm::normalize(this->originals[arm] * upper);
                animation.animation[this->joints[elbow]].rotation =
                    glm::normalize(this->originals[elbow] * lower);
                this->update_globals(model, animation);
            };
            float fraction = step / angle;
            bool accepted = false;
            for (std::size_t attempt = 0; attempt < BACKTRACK_STEPS; ++attempt) {
                set_offsets(glm::normalize(glm::slerp(previous[arm], desired_upper, fraction)),
                    glm::normalize(glm::slerp(previous[elbow], desired_lower, fraction)));
                if (this->arm_clear(side, this->arm_frame(side))) {
                    accepted = true;
                    break;
                }
                // A safe destination does not imply a safe interpolation. Backtrack t/=2.
                fraction *= 0.5f;
            }
            if (!accepted) {
                set_offsets(previous[arm], previous[elbow]);
                if (!this->arm_clear(side, this->arm_frame(side)) && elapsed > 0) {
                    // An animated torso/head may move into the previous pose. In that case
                    // clearance takes precedence over speed; never retain an intersecting arm.
                    set_offsets(desired_upper, desired_lower);
                }
            }
        }
    }

    void CursorPose::protect_arms(
        const component::ModelComponent& model, component::AnimationComponent& animation) {
        this->update_globals(model, animation);
        for (std::size_t side = 0; side < this->wrists.size(); ++side) {
            const auto arm = side == 0 ? LeftArm : RightArm;
            const auto elbow = side == 0 ? LeftElbow : RightElbow;
            if (!(this->wrists[side] < this->globals.size()) ||
                (glm::length(this->offsets[arm] - IDENTITY) < EPSILON &&
                    glm::length(this->offsets[elbow] - IDENTITY) < EPSILON)) {
                continue;
            }
            // Releasing an arm must converge to the authored animation, even when that
            // animation intentionally contains rotations outside the procedural hinge limits.
            if (this->active_arms[side]) {
                this->constrain_arm(animation, side);
            }
            this->update_globals(model, animation);
            const auto frame = this->arm_frame(side);
            if (!this->arm_clear(side, frame)) {
                // Interpolation between two safe poses can cut through the body. Re-solve its
                // actual intermediate hand position and keep the corrected offset for next frame.
                const auto wrist = glm::vec3(this->globals[this->wrists[side]][3]);
                if (!this->reach_side(model, animation, side, wrist)) {
                    animation.animation[this->joints[arm]].rotation = this->originals[arm];
                    animation.animation[this->joints[elbow]].rotation = this->originals[elbow];
                    this->update_globals(model, animation);
                }
            }
            this->offsets[arm] = glm::normalize(glm::inverse(this->originals[arm]) *
                                                animation.animation[this->joints[arm]].rotation);
            this->offsets[elbow] =
                glm::normalize(glm::inverse(this->originals[elbow]) *
                               animation.animation[this->joints[elbow]].rotation);
        }
    }
} // namespace enishi::animation
