#include "cursor_reach.h"
#include <algorithm>
#include <animation/procedural/cursor/cursor_collision.h>
#include <animation/procedural/cursor/cursor_transforms.h>
#include <animation/procedural/jump/cursor_jump.h>
#include <cmath>

namespace enishi::animation {
    using enum CursorPoseState::Joint;
    using enum CursorPoseState::BodyBone;
    namespace {
        constexpr float EPSILON = 0.00001f;
        constexpr std::size_t LEFT_SIDE = 0;
        constexpr std::size_t RIGHT_SIDE = 1;
        constexpr std::size_t ARM_COUNT = 2;
        constexpr glm::length_t TRANSLATION_COLUMN = 3;
        constexpr glm::vec3 X_AXIS(1, 0, 0);
        constexpr glm::vec3 UP_AXIS(0, 1, 0);
        constexpr glm::vec3 FORWARD_AXIS(0, 0, -1);
        constexpr float LEFT_DIRECTION_SIGN = 1.0f;
        constexpr float RIGHT_DIRECTION_SIGN = -1.0f;
        constexpr float MIDPOINT_WEIGHT = 0.5f;
        constexpr float BACKTRACK_FACTOR = 0.5f;
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
                auto axis = glm::cross(from, UP_AXIS);
                if (!(glm::length(axis) > EPSILON)) {
                    axis = glm::cross(from, X_AXIS);
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

    void CursorReach::initialize_arms(CursorPoseState& state,
        const component::ModelComponent& model,
        const BoneNameDictionary& bone_names) {
        state.bind_rotations.fill(IDENTITY);
        state.hinge_axes.fill(UP_AXIS);
        state.upper_axes.fill(X_AXIS);
        state.body_bones.fill(types::INVALID_BONE_INDEX);
        state.body_bones = {bone_names.find(model.bone_names, BoneRole::LeftUpperArm),
            bone_names.find(model.bone_names, BoneRole::RightUpperArm),
            bone_names.find(model.bone_names, BoneRole::Neck),
            bone_names.find(model.bone_names, BoneRole::Head),
            bone_names.find(model.bone_names, BoneRole::Hips)};
        auto ancestor = state.body_bones[BodyLeft];
        if (!(ancestor < model.bone_node.size())) {
            ancestor = state.body_bones[BodyRight];
        }
        for (std::size_t step = 0;
             step < model.bone_node.size() && ancestor < model.bone_node.size();
             ++step) {
            ancestor = model.bone_node[ancestor].parent;
            auto other = state.body_bones[BodyRight];
            for (std::size_t depth = 0;
                 depth < model.bone_node.size() && other < model.bone_node.size();
                 ++depth) {
                if (other == ancestor) {
                    state.body_anchor = ancestor;
                    break;
                }
                other = model.bone_node[other].parent;
            }
            if (state.body_anchor < model.bone_node.size()) {
                break;
            }
        }
        if (state.body_anchor < model.bind_bone.size()) {
            // Procedural axes use inverse(rest global), not the skin's inverse bind matrix.
            // VRM skin bind space can differ from the node's initial pose.
            state.body_bind_inverse =
                glm::inverse(glm::mat3(model.bind_bone[state.body_anchor].global));
        }
        if (state.body_bones[BodyHead] < model.bind_bone.size()) {
            state.head_local_up =
                glm::inverse(glm::mat3(model.bind_bone[state.body_bones[BodyHead]].global)) *
                UP_AXIS;
        }
        for (std::size_t joint = 0; joint < Count; ++joint) {
            if (state.joints[joint] < model.bind_bone.size()) {
                state.bind_rotations[joint] =
                    rotation_of(model.bind_bone[state.joints[joint]].local);
            }
        }
        for (std::size_t side = 0; side < state.wrists.size(); ++side) {
            const auto arm = side == LEFT_SIDE ? LeftArm : RightArm;
            const auto elbow = side == LEFT_SIDE ? LeftElbow : RightElbow;
            if (!(state.wrists[side] < model.bind_bone.size()) ||
                !(state.joints[arm] < model.bind_bone.size()) ||
                !(state.joints[elbow] < model.bind_bone.size())) {
                continue;
            }
            const auto& shoulder = model.bind_bone[state.joints[arm]];
            const auto& joint = model.bind_bone[state.joints[elbow]];
            const auto upper =
                glm::vec3(joint.global[TRANSLATION_COLUMN] - shoulder.global[TRANSLATION_COLUMN]);
            const auto lower =
                glm::vec3(model.bind_bone[state.wrists[side]].global[TRANSLATION_COLUMN] -
                          joint.global[TRANSLATION_COLUMN]);
            auto axis = glm::cross(upper, FORWARD_AXIS);
            if (!(glm::length(upper) > EPSILON) || !(glm::length(lower) > EPSILON) ||
                !(glm::length(axis) > EPSILON)) {
                continue;
            }
            axis = glm::normalize(axis);
            // Model-space directions become joint-local axes: axis_local = inverse(G_rest) * axis.
            state.hinge_axes[side] = glm::normalize(glm::inverse(glm::mat3(joint.global)) * axis);
            state.upper_axes[side] =
                glm::normalize(glm::inverse(glm::mat3(shoulder.global)) * upper);
            // Signed bind flex measured around the forward-bending anatomical axis.
            state.bind_flex[side] =
                std::atan2(glm::dot(glm::cross(glm::normalize(upper), glm::normalize(lower)), axis),
                    glm::dot(glm::normalize(upper), glm::normalize(lower)));
        }
    }

    bool CursorReach::can_reach(
        const CursorPoseState& state, std::size_t side, const glm::vec3& target) {
        if (!(state.wrists[side] < state.transforms.globals.size())) {
            return false;
        }
        const auto arm = state.joints[side == LEFT_SIDE ? LeftArm : RightArm];
        const auto elbow = state.joints[side == LEFT_SIDE ? LeftElbow : RightElbow];
        const auto shoulder = glm::vec3(state.transforms.globals[arm][TRANSLATION_COLUMN]);
        const auto joint = glm::vec3(state.transforms.globals[elbow][TRANSLATION_COLUMN]);
        const float upper = glm::length(joint - shoulder);
        const float lower = glm::length(
            glm::vec3(state.transforms.globals[state.wrists[side]][TRANSLATION_COLUMN]) - joint);
        // Each arm owns its range: |target-shoulder| < upper+lower. A 5% release
        // margin avoids repeatedly entering/leaving when the cursor grazes the boundary.
        const float release = state.active_arms[side] ? REACH_RELEASE_RATIO : 1.0f;
        if (!(upper > EPSILON) || !(lower > EPSILON) ||
            !(glm::length(target - shoulder) < (upper + lower) * release)) {
            return false;
        }
        if (state.body_bones[BodyLeft] < state.transforms.globals.size() &&
            state.body_bones[BodyRight] < state.transforms.globals.size()) {
            const auto left =
                glm::vec3(state.transforms.globals[state.body_bones[BodyLeft]][TRANSLATION_COLUMN]);
            const auto right = glm::vec3(
                state.transforms.globals[state.body_bones[BodyRight]][TRANSLATION_COLUMN]);
            const auto across = left - right;
            const float width = glm::length(across);
            if (width > EPSILON) {
                // Project onto the shoulder axis; both ranges overlap across the torso.
                // s=dot(target-midpoint, outward), allowed when s > -width*margin.
                const float sign = side == LEFT_SIDE ? LEFT_DIRECTION_SIGN : RIGHT_DIRECTION_SIGN;
                const float lateral =
                    sign * glm::dot(target - (left + right) * MIDPOINT_WEIGHT, across / width);
                const float margin =
                    state.active_arms[side] ? CROSS_BODY_RELEASE_RATIO : CROSS_BODY_RATIO;
                return lateral > -width * margin;
            }
        }
        return true;
    }

    CursorArmFrame CursorReach::arm_frame(const CursorPoseState& state, std::size_t side) {
        const auto arm = state.joints[side == LEFT_SIDE ? LeftArm : RightArm];
        const auto elbow = state.joints[side == LEFT_SIDE ? LeftElbow : RightElbow];
        const auto shoulder = glm::vec3(state.transforms.globals[arm][TRANSLATION_COLUMN]);
        const float upper =
            glm::length(glm::vec3(state.transforms.globals[elbow][TRANSLATION_COLUMN]) - shoulder);
        const float lower =
            glm::length(glm::vec3(state.transforms.globals[state.wrists[side]][TRANSLATION_COLUMN] -
                                  state.transforms.globals[elbow][TRANSLATION_COLUMN]));
        const auto basis =
            state.body_anchor < state.transforms.globals.size()
                ? glm::mat3(state.transforms.globals[state.body_anchor]) * state.body_bind_inverse
                : glm::mat3(1);
        CursorArmFrame frame{{},
            shoulder,
            glm::normalize(basis * X_AXIS),
            glm::normalize(basis * UP_AXIS),
            glm::normalize(basis * FORWARD_AXIS),
            upper + lower,
            lower * ARM_MARGIN_RATIO,
            lower * HAND_MARGIN_RATIO};
        float width = upper;
        if (state.body_bones[BodyLeft] < state.transforms.globals.size() &&
            state.body_bones[BodyRight] < state.transforms.globals.size()) {
            const auto left =
                glm::vec3(state.transforms.globals[state.body_bones[BodyLeft]][TRANSLATION_COLUMN]);
            const auto right = glm::vec3(
                state.transforms.globals[state.body_bones[BodyRight]][TRANSLATION_COLUMN]);
            frame.center = (left + right) * MIDPOINT_WEIGHT;
            width = glm::length(left - right);
            if (width > EPSILON) {
                // Anatomical left = (left shoulder - right shoulder) / shoulder width.
                // Humanoid rigs do not all place the left arm on the positive X side.
                frame.outward = (left - right) / width;
            }
        } else {
            const float sign = side == LEFT_SIDE ? LEFT_DIRECTION_SIGN : RIGHT_DIRECTION_SIGN;
            frame.center -= frame.outward * sign * width * MIDPOINT_WEIGHT;
        }
        frame.outward *= side == LEFT_SIDE ? LEFT_DIRECTION_SIGN : RIGHT_DIRECTION_SIGN;
        const auto hips =
            state.body_bones[BodyHips] < state.transforms.globals.size()
                ? glm::vec3(
                      state.transforms.globals[state.body_bones[BodyHips]][TRANSLATION_COLUMN])
                : frame.center - frame.up * frame.length * TORSO_HEIGHT_RATIO;
        const auto neck =
            state.body_bones[BodyNeck] < state.transforms.globals.size()
                ? glm::vec3(
                      state.transforms.globals[state.body_bones[BodyNeck]][TRANSLATION_COLUMN])
                : frame.center;
        frame.obstacles.push_back({hips, neck, width * TORSO_RADIUS_RATIO});
        if (state.body_bones[BodyHead] < state.transforms.globals.size()) {
            const auto head =
                glm::vec3(state.transforms.globals[state.body_bones[BodyHead]][TRANSLATION_COLUMN]);
            const auto head_up =
                glm::normalize(glm::mat3(state.transforms.globals[state.body_bones[BodyHead]]) *
                               state.head_local_up);
            frame.obstacles.push_back({head,
                head + head_up * lower * HEAD_HEIGHT_RATIO,
                (std::max)(width, lower) * HEAD_RADIUS_RATIO});
        }
        return frame;
    }

    void CursorReach::solve_arm(CursorPoseState& state,
        const component::ModelComponent& model,
        component::AnimationComponent& animation,
        std::size_t side,
        const glm::vec3& target,
        const CursorArmFrame& frame) {
        const auto arm = side == LEFT_SIDE ? LeftArm : RightArm;
        const auto elbow = side == LEFT_SIDE ? LeftElbow : RightElbow;
        const auto shoulder =
            glm::vec3(state.transforms.globals[state.joints[arm]][TRANSLATION_COLUMN]);
        const auto joint =
            glm::vec3(state.transforms.globals[state.joints[elbow]][TRANSLATION_COLUMN]);
        const float upper = glm::length(joint - shoulder);
        const float lower = glm::length(
            glm::vec3(state.transforms.globals[state.wrists[side]][TRANSLATION_COLUMN]) - joint);
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
        animation.animation[state.joints[elbow]].rotation =
            state.bind_rotations[elbow] *
            glm::angleAxis(flex - state.bind_flex[side], state.hinge_axes[side]);
        CursorTransforms::update_globals(state.transforms, model, animation);
        const auto current_upper =
            glm::vec3(state.transforms.globals[state.joints[elbow]][TRANSLATION_COLUMN]) - shoulder;
        const auto current_lower =
            glm::vec3(state.transforms.globals[state.wrists[side]][TRANSLATION_COLUMN] -
                      state.transforms.globals[state.joints[elbow]][TRANSLATION_COLUMN]);
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
        const auto global_rotation = rotation_of(state.transforms.globals[state.joints[arm]]);
        animation.animation[state.joints[arm]].rotation *=
            glm::inverse(global_rotation) * correction * global_rotation;
        CursorReach::constrain_arm(state, animation, side);
        CursorTransforms::update_globals(state.transforms, model, animation);
    }

    void CursorReach::limit_arm_speed(CursorPoseState& state,
        const component::ModelComponent& model,
        component::AnimationComponent& animation,
        const std::array<glm::quat, Count>& previous,
        float seconds) {
        const float elapsed = std::isfinite(seconds) ? (std::max)(0.0f, seconds) : 0.0f;
        const float step = glm::radians(state.config.arm_speed) * elapsed;
        constexpr std::size_t BACKTRACK_STEPS = 12;
        for (std::size_t side = 0; side < state.wrists.size(); ++side) {
            const auto arm = side == LEFT_SIDE ? LeftArm : RightArm;
            const auto elbow = side == LEFT_SIDE ? LeftElbow : RightElbow;
            if (!(state.wrists[side] < state.transforms.globals.size())) {
                continue;
            }
            const auto desired_upper = state.offsets[arm];
            const auto desired_lower = state.offsets[elbow];
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
                state.offsets[arm] = upper;
                state.offsets[elbow] = lower;
                animation.animation[state.joints[arm]].rotation =
                    glm::normalize(state.originals[arm] * upper);
                animation.animation[state.joints[elbow]].rotation =
                    glm::normalize(state.originals[elbow] * lower);
                CursorTransforms::update_globals(state.transforms, model, animation);
            };
            float fraction = step / angle;
            bool accepted = false;
            for (std::size_t attempt = 0; attempt < BACKTRACK_STEPS; ++attempt) {
                set_offsets(glm::normalize(glm::slerp(previous[arm], desired_upper, fraction)),
                    glm::normalize(glm::slerp(previous[elbow], desired_lower, fraction)));
                if (CursorReach::arm_clear(state, side, CursorReach::arm_frame(state, side))) {
                    accepted = true;
                    break;
                }
                // A safe destination does not imply a safe interpolation. Backtrack t/=2.
                fraction *= BACKTRACK_FACTOR;
            }
            if (!accepted) {
                set_offsets(previous[arm], previous[elbow]);
                if (!CursorReach::arm_clear(state, side, CursorReach::arm_frame(state, side)) &&
                    elapsed > 0) {
                    // An animated torso/head may move into the previous pose. In that case
                    // clearance takes precedence over speed; never retain an intersecting arm.
                    set_offsets(desired_upper, desired_lower);
                }
            }
        }
    }

    void CursorReach::protect_arms(CursorPoseState& state,
        const component::ModelComponent& model,
        component::AnimationComponent& animation) {
        CursorTransforms::update_globals(state.transforms, model, animation);
        for (std::size_t side = 0; side < state.wrists.size(); ++side) {
            const auto arm = side == LEFT_SIDE ? LeftArm : RightArm;
            const auto elbow = side == LEFT_SIDE ? LeftElbow : RightElbow;
            if (!(state.wrists[side] < state.transforms.globals.size()) ||
                (glm::length(state.offsets[arm] - IDENTITY) < EPSILON &&
                    glm::length(state.offsets[elbow] - IDENTITY) < EPSILON)) {
                continue;
            }
            // Releasing an arm must converge to the authored animation, even when that
            // animation intentionally contains rotations outside the procedural hinge limits.
            if (state.active_arms[side]) {
                CursorReach::constrain_arm(state, animation, side);
            }
            CursorTransforms::update_globals(state.transforms, model, animation);
            const auto frame = CursorReach::arm_frame(state, side);
            if (!CursorReach::arm_clear(state, side, frame)) {
                // Interpolation between two safe poses can cut through the body. Re-solve its
                // actual intermediate hand position and keep the corrected offset for next frame.
                const auto wrist =
                    glm::vec3(state.transforms.globals[state.wrists[side]][TRANSLATION_COLUMN]);
                if (!CursorReach::reach_side(state, model, animation, side, wrist)) {
                    animation.animation[state.joints[arm]].rotation = state.originals[arm];
                    animation.animation[state.joints[elbow]].rotation = state.originals[elbow];
                    CursorTransforms::update_globals(state.transforms, model, animation);
                }
            }
            state.offsets[arm] = glm::normalize(glm::inverse(state.originals[arm]) *
                                                animation.animation[state.joints[arm]].rotation);
            state.offsets[elbow] =
                glm::normalize(glm::inverse(state.originals[elbow]) *
                               animation.animation[state.joints[elbow]].rotation);
        }
    }

    void CursorReach::reach(CursorPoseState& state,
        const component::ModelComponent& model,
        component::AnimationComponent& animation,
        const glm::vec3& target) {
        std::array<glm::vec3, ARM_COUNT> goals{target, target};
        // Cooldown gates the next jump, not the effort to reach its target after landing.
        const bool stretch = state.locomotion.wait > 0 || state.locomotion.height > 0 ||
                             CursorJump::desired_jump_height(state, target) > 0;
        for (std::size_t side = 0; side < state.wrists.size(); ++side) {
            if ((!stretch && !CursorJump::cursor_in_arm_reach(state, side)) ||
                !(state.wrists[side] < state.transforms.globals.size()) ||
                CursorReach::can_reach(state, side, target)) {
                continue;
            }
            const auto shoulder = glm::vec3(
                state.transforms.globals[state.joints[side == LEFT_SIDE ? LeftArm : RightArm]]
                                        [TRANSLATION_COLUMN]);
            const auto delta = target - shoulder;
            if (!(delta.y > 0)) {
                continue;
            }
            // Stretch toward the cursor within the reach sphere, including after landing:
            // goal = shoulder + normalize(target-shoulder) * arm_length * margin.
            constexpr float JUMP_REACH_MARGIN = 0.95f;
            goals[side] = shoulder + glm::normalize(delta) *
                                         CursorReach::arm_frame(state, side).length *
                                         JUMP_REACH_MARGIN;
        }
        const std::array<bool, ARM_COUNT> in_range{
            CursorReach::can_reach(state, LEFT_SIDE, goals[LEFT_SIDE]),
            CursorReach::can_reach(state, RIGHT_SIDE, goals[RIGHT_SIDE])};
        const bool both = in_range[LEFT_SIDE] && in_range[RIGHT_SIDE];
        for (std::size_t side = 0; side < state.wrists.size(); ++side) {
            auto goal = goals[side];
            if (both) {
                // Separate goals by the sum of the hand envelopes, plus a small gap.
                constexpr float HAND_GAP_RATIO = 1.1f;
                goal += CursorReach::arm_frame(state, side).outward *
                        CursorCollision::hand_radius(state, side) * HAND_GAP_RATIO;
            }
            state.active_arms[side] =
                in_range[side] && CursorReach::reach_side(state, model, animation, side, goal);
        }
    }

    bool CursorReach::arm_clear(
        const CursorPoseState& state, std::size_t side, const CursorArmFrame& frame) {
        const auto arm = state.joints[side == LEFT_SIDE ? LeftArm : RightArm];
        const auto elbow = state.joints[side == LEFT_SIDE ? LeftElbow : RightElbow];
        const auto shoulder = glm::vec3(state.transforms.globals[arm][TRANSLATION_COLUMN]);
        const auto joint = glm::vec3(state.transforms.globals[elbow][TRANSLATION_COLUMN]);
        const auto wrist =
            glm::vec3(state.transforms.globals[state.wrists[side]][TRANSLATION_COLUMN]);
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
        const std::size_t other = side == LEFT_SIDE ? RIGHT_SIDE : LEFT_SIDE;
        if (state.wrists[other] < state.transforms.globals.size()) {
            const auto other_elbow = state.joints[other == LEFT_SIDE ? LeftElbow : RightElbow];
            const auto other_joint =
                glm::vec3(state.transforms.globals[other_elbow][TRANSLATION_COLUMN]);
            const auto other_wrist =
                glm::vec3(state.transforms.globals[state.wrists[other]][TRANSLATION_COLUMN]);
            const auto other_frame = CursorReach::arm_frame(state, other);
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
        return CursorCollision::mesh_arm_clear(state, side);
    }

    void CursorReach::constrain_arm(
        const CursorPoseState& state, component::AnimationComponent& animation, std::size_t side) {
        const auto arm = side == LEFT_SIDE ? LeftArm : RightArm;
        const auto elbow = side == LEFT_SIDE ? LeftElbow : RightElbow;
        auto& upper = animation.animation[state.joints[arm]].rotation;
        auto& lower = animation.animation[state.joints[elbow]].rotation;
        upper = glm::normalize(state.bind_rotations[arm] *
                               limit_shoulder(glm::inverse(state.bind_rotations[arm]) * upper,
                                   state.upper_axes[side]));
        const float flex =
            twist_angle(glm::inverse(state.bind_rotations[elbow]) * lower, state.hinge_axes[side]);
        lower = state.bind_rotations[elbow] *
                glm::angleAxis(std::clamp(flex,
                                   -state.bind_flex[side],
                                   glm::radians(MAX_FLEX) - state.bind_flex[side]),
                    state.hinge_axes[side]);
    }

    bool CursorReach::reach_side(CursorPoseState& state,
        const component::ModelComponent& model,
        component::AnimationComponent& animation,
        std::size_t side,
        const glm::vec3& target) {
        const auto frame = CursorReach::arm_frame(state, side);
        auto safe_target = target;
        // Never chase a cursor through or behind the body. Shift the goal forward instead.
        safe_target += frame.forward * (std::max)(0.0f,
                                           frame.length * FRONT_REACH_RATIO -
                                               glm::dot(safe_target - frame.center, frame.forward));
        const auto arm = state.joints[side == LEFT_SIDE ? LeftArm : RightArm];
        const auto elbow = state.joints[side == LEFT_SIDE ? LeftElbow : RightElbow];
        const auto original_upper = animation.animation[arm].rotation;
        const auto original_lower = animation.animation[elbow].rotation;
        for (std::size_t attempt = 0; attempt < AVOID_ATTEMPTS; ++attempt) {
            animation.animation[arm].rotation = original_upper;
            animation.animation[elbow].rotation = original_lower;
            CursorTransforms::update_globals(state.transforms, model, animation);
            // Search forward and outward if either complete arm segment intersects a capsule.
            const auto candidate = safe_target + static_cast<float>(attempt) * frame.length *
                                                     (frame.forward * AVOID_FORWARD_STEP +
                                                         frame.outward * AVOID_SIDE_STEP);
            CursorReach::solve_arm(state, model, animation, side, candidate, frame);
            if (CursorReach::arm_clear(state, side, frame)) {
                return true;
            }
        }
        animation.animation[arm].rotation = original_upper;
        animation.animation[elbow].rotation = original_lower;
        CursorTransforms::update_globals(state.transforms, model, animation);
        return false;
    }
} // namespace enishi::animation
