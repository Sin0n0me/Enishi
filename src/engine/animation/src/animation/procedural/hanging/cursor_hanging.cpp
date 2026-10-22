#include "../cursor/cursor_pose.h"
#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

namespace enishi::animation {
    namespace {
        constexpr float EPSILON = 0.00001f;
        constexpr float RESPONSE = 8.0f;
        constexpr float ENTRY_SECONDS = 0.5f;
        constexpr float LIFT_ARM_RATIO = 0.12f;
        constexpr float WALK_RETURN_SECONDS = 1.0f;
        constexpr float WALK_STRIDE_RATIO = 0.8f;
        constexpr float WALK_STANCE_RATIO = 2.0f;
        constexpr float WALK_FOOT_LIFT_RATIO = 0.06f;
        constexpr float WALK_CROUCH_WEIGHT = 1.2f;
        constexpr float MAX_DT = 0.05f;
        constexpr float HAND_RADIUS_RATIO = 0.18f;
        constexpr float SWAY_RATIO = 0.3f;
        constexpr float MAX_BODY_LEAN_DEGREES = 10.0f;
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
        if (!this->transforms.valid || this->hanging_state.active ||
            this->hanging_state.weight > EPSILON ||
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
            this->transforms.cached_local.size() != animation.animation.size() ||
            !(this->joints[Head] < animation.global.size())) {
            return;
        }
        this->hanging_state.side = side;
        this->hanging_state.landing = false;
        this->hanging_state.landing_time = 0;
        this->hanging_state.display_pose.clear();
        // The transform cache may still contain a temporary suspension/jump pose. Rebuild
        // the entry pose from authored animation: q_entry = q_source * q_cursor_offset.
        this->hanging_state.pose = animation.animation;
        for (std::size_t joint = 0; joint < Count; ++joint) {
            if (this->joints[joint] < this->hanging_state.pose.size()) {
                this->hanging_state.pose[this->joints[joint]].rotation = glm::normalize(
                    this->hanging_state.pose[this->joints[joint]].rotation * this->offsets[joint]);
            }
        }
        if (!this->transition_state.display.empty()) {
            this->hanging_state.pose = this->transition_state.display;
        }
        this->hanging_state.anchor = glm::vec3(animation.global[this->wrists[side]][3]);
        this->hanging_state.normal = cursor.plane_normal;
        this->hanging_state.target = this->hanging_state.anchor;
        this->hanging_state.lag = this->hanging_state.anchor;
        this->hanging_state.weight = 0;
        this->hanging_state.entry_time = 0;
        // Keep the entry threshold fixed; moving the suspended head must not move the threshold.
        this->hanging_state.min_height = animation.global[this->joints[Head]][3].y +
                                         this->arm_frame(side).length * HEAD_CLEARANCE_RATIO;
        this->hanging_state.active = true;
        this->locomotion.height = 0;
        this->locomotion.wait = 0;
    }

    void CursorPose::end_hanging() {
        if (this->hanging_state.active && this->hanging_state.weight > EPSILON &&
            !this->hanging_state.display_pose.empty()) {
            this->hanging_state.landing = true;
            this->hanging_state.landing_time = 0;
        }
        this->hanging_state.active = false;
        if (!(this->hanging_state.weight > EPSILON)) {
            this->hanging_state.weight = 0;
            this->hanging_state.entry_time = 0;
            this->hanging_state.landing = false;
            this->hanging_state.landing_time = 0;
            this->hanging_state.pose.clear();
            this->hanging_state.return_offset = glm::vec3(0);
            this->hanging_state.display_center = glm::vec3(0);
            this->hanging_state.return_cycles = 1;
            this->hanging_state.display_pose.clear();
            this->hanging_state.anchor = glm::vec3(0);
            this->hanging_state.target = glm::vec3(0);
            this->hanging_state.lag = glm::vec3(0);
            this->hanging_state.min_height = 0;
        }
    }
    bool CursorPose::is_hanging() const {
        return this->hanging_state.active;
    }

    bool CursorPose::suppress_hanging_ik(types::BoneIndex bone) const {
        // File poses still need their authored IK to resolve the interpolated goal positions.
        const bool procedural_transition =
            this->transition_state.frame && this->transition_state.stage != PoseStage::File;
        return (this->hanging_state.weight > EPSILON || this->idle.applied ||
                   procedural_transition) &&
               std::ranges::find(this->locomotion.leg_ik_goals, bone) !=
                   this->locomotion.leg_ik_goals.end();
    }

    void CursorPose::prepare_hanging_limbs(
        const component::ModelComponent& model, component::AnimationComponent& animation) {
        const auto elbow = this->hanging_state.side == 0 ? LeftElbow : RightElbow;
        // flex=0 straightens the support arm in its hinge plane without hyperextension.
        animation.animation[this->joints[elbow]].rotation =
            this->bind_rotations[elbow] * glm::angleAxis(-this->bind_flex[this->hanging_state.side],
                                              this->hinge_axes[this->hanging_state.side]);
        // Capturing a jumping/crouching pose also captures knee bends. Start dangling legs from
        // their bind axes; rotating only the hip would otherwise preserve the wrong bend plane.
        for (const auto& leg : this->locomotion.legs) {
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
        types::BoneIndex end,
        const glm::vec3& target_direction) {
        if (!(start < this->transforms.globals.size()) ||
            !(end < this->transforms.globals.size())) {
            return;
        }
        const auto direction =
            glm::vec3(this->transforms.globals[end][3] - this->transforms.globals[start][3]);
        const auto q = rotation(this->transforms.globals[start]);
        // Convert model-space alignment A into the bone frame: q_local'=q_local*q^-1*A*q.
        animation.animation[start].rotation *=
            glm::inverse(q) * align(direction, target_direction) * q;
        this->update_globals(model, animation);
    }

    void CursorPose::update_hanging_ik_targets(
        const component::ModelComponent& model, component::AnimationComponent& animation) {
        // Foot IK targets must follow the dangling feet instead of pinning them to the ground.
        for (const auto& ik : model.iks) {
            const auto& ccd = std::get<types::CCDIK>(ik.method);
            if (std::ranges::find(this->locomotion.leg_ik_goals, ccd.ik_bone) ==
                this->locomotion.leg_ik_goals.end()) {
                continue;
            }
            const auto parent = model.bone_node[ccd.ik_bone].parent;
            const auto base =
                this->transforms.external[ccd.ik_bone] * (parent < this->transforms.globals.size()
                                                                 ? this->transforms.globals[parent]
                                                                 : glm::mat4(1));
            if (ccd.target < this->transforms.globals.size() &&
                std::abs(glm::determinant(base)) > EPSILON) {
                animation.animation[ccd.ik_bone].position =
                    glm::vec3(glm::inverse(base) * this->transforms.globals[ccd.target][3]);
            }
        }
    }

    void CursorPose::blend_hanging_pose(component::AnimationComponent& animation) const {
        // smoothstep(t)=t*t*(3-2*t) has zero slope at both ends of entry.
        const float weight = glm::smoothstep(0.0f, 1.0f, this->hanging_state.weight);
        const float entry =
            1 - glm::smoothstep(0.0f, ENTRY_SECONDS, this->hanging_state.entry_time);
        for (std::size_t bone = 0; bone < animation.animation.size(); ++bone) {
            auto& pose = animation.animation[bone];
            const auto& original = this->hanging_state.originals[bone];
            const auto& start = this->hanging_state.pose[bone];
            // Retain the reached pose at entry, fading its contribution even if released early.
            const auto position = glm::mix(original.position, start.position, entry);
            const auto scale = glm::mix(original.scale, start.scale, entry);
            const auto rotation = glm::slerp(original.rotation, start.rotation, entry);
            pose.position = glm::mix(position, pose.position, weight);
            pose.scale = glm::mix(scale, pose.scale, weight);
            pose.rotation = glm::slerp(rotation, pose.rotation, weight);
        }
    }

    void CursorPose::translate_hanging_roots(
        component::AnimationComponent& animation, const glm::vec3& offset) const {
        for (const auto root : this->locomotion.roots) {
            const auto axes = glm::mat3(this->transforms.external[root]);
            if (std::abs(glm::determinant(axes)) > EPSILON) {
                // Model-space displacement maps to local space as inverse(external.axes)*d.
                animation.animation[root].position += glm::inverse(axes) * offset;
            }
        }
    }

    void CursorPose::apply_hanging_walk(const component::ModelComponent& model,
        component::AnimationComponent& animation,
        float seconds) {
        // Fit all distance-dependent gait cycles into the fixed return duration: c=t*N/T.
        const float cycle = seconds * this->hanging_state.return_cycles / WALK_RETURN_SECONDS;
        const float whole = std::floor(cycle);
        const float phase = cycle - whole;
        const float progress =
            (whole + glm::smoothstep(0.0f, 1.0f, phase)) / this->hanging_state.return_cycles;
        std::array<glm::mat4, 2> feet{glm::mat4(1), glm::mat4(1)};
        for (std::size_t side = 0; side < this->locomotion.legs.size(); ++side) {
            if (this->locomotion.legs[side][2] < this->transforms.globals.size()) {
                feet[side] = this->transforms.globals[this->locomotion.legs[side][2]];
            }
        }
        this->translate_hanging_roots(
            animation, this->hanging_state.return_offset * (1 - progress));
        this->update_globals(model, animation);
        // Flex during each stride: sin^2(pi*phase) provides reach slack and ends at standing.
        const float envelope = std::sin(glm::pi<float>() * phase);
        this->crouch(model, animation, WALK_CROUCH_WEIGHT * envelope * envelope);
        // Lead with the foot on the return side: direction=-release_offset.
        const std::size_t leading =
            glm::dot(glm::vec3(feet[0][3] - feet[1][3]), this->hanging_state.return_offset) > 0 ? 1
                                                                                                : 0;
        for (std::size_t side = 0; side < this->locomotion.legs.size(); ++side) {
            const auto& leg = this->locomotion.legs[side];
            if (!(leg[2] < this->transforms.globals.size())) {
                continue;
            }
            // Alternate half-cycle swings. Outside its swing a foot stays at a fixed ground point.
            const float swing_start = side == leading ? 0.0f : 1.0f;
            const float swing = std::clamp(2 * phase - swing_start, 0.0f, 1.0f);
            const float foot_progress =
                (whole + glm::smoothstep(0.0f, 1.0f, swing)) / this->hanging_state.return_cycles;
            auto target = feet[side];
            target[3] += glm::vec4(this->hanging_state.return_offset * (1 - foot_progress), 0);
            const float length = glm::length(glm::vec3(this->transforms.globals[leg[0]][3] -
                                                       this->transforms.globals[leg[1]][3])) +
                                 glm::length(glm::vec3(this->transforms.globals[leg[1]][3] -
                                                       this->transforms.globals[leg[2]][3]));
            // Foot arc h=L*ratio*sin^2(pi*s), zero height and vertical speed at contact.
            const float lift = std::sin(glm::pi<float>() * swing);
            target[3].y += length * WALK_FOOT_LIFT_RATIO * lift * lift;
            this->bend_leg(model, animation, side, target);
        }
    }

    bool CursorPose::apply_hanging_landing(const component::ModelComponent& model,
        component::AnimationComponent& animation,
        float seconds) {
        const float dt = std::isfinite(seconds) ? std::clamp(seconds, 0.0f, MAX_DT) : 0;
        const auto hips = this->body_bones[BodyHips];
        const auto center_bone =
            hips < this->transforms.globals.size() ? hips : this->locomotion.roots.front();
        if (this->hanging_state.landing_time == 0) {
            // Anchor the visible body, not a potentially distant model root/pivot.
            this->hanging_state.return_offset = this->hanging_state.display_center -
                                                glm::vec3(this->transforms.globals[center_bone][3]);
            this->hanging_state.return_offset.y = 0;
            float stride = this->arm_frame(this->hanging_state.side).length * WALK_STRIDE_RATIO;
            const auto& leg = this->locomotion.legs[0];
            if (leg[2] < this->transforms.globals.size()) {
                stride = (glm::length(glm::vec3(this->transforms.globals[leg[0]][3] -
                                                this->transforms.globals[leg[1]][3])) +
                             glm::length(glm::vec3(this->transforms.globals[leg[1]][3] -
                                                   this->transforms.globals[leg[2]][3]))) *
                         WALK_STRIDE_RATIO;
            }
            if (this->locomotion.legs[0][2] < this->transforms.globals.size() &&
                this->locomotion.legs[1][2] < this->transforms.globals.size()) {
                auto separation =
                    glm::vec3(this->transforms.globals[this->locomotion.legs[0][2]][3] -
                              this->transforms.globals[this->locomotion.legs[1][2]][3]);
                separation.y = 0;
                if (glm::length(separation) > EPSILON) {
                    // Bound each stride relative to stance width as well as leg length.
                    stride = (std::min)(stride, glm::length(separation) * WALK_STANCE_RATIO);
                }
            }
            // Integer gait cycles keep both feet grounded at the destination; stride <= L*ratio.
            this->hanging_state.return_cycles = std::ceil(
                glm::length(this->hanging_state.return_offset) / (std::max)(stride, EPSILON));
        }
        this->hanging_state.landing_time += dt;
        const float walk_time =
            this->hanging_state.landing_time - HANGING_DROP_SECONDS - HANGING_LANDING_SECONDS;
        const float walk_duration = this->hanging_state.return_cycles > 0 ? WALK_RETURN_SECONDS : 0;
        if (!(walk_time < walk_duration)) {
            this->hanging_state.landing = false;
            this->hanging_state.weight = 0;
            this->end_hanging();
            this->locomotion.rest = this->config.jump_cooldown;
            return false;
        }
        this->hanging_state.originals = animation.animation;
        if (!animation.motion_playing) {
            // Blend the released pose toward lowered arms, rather than through the bind pose.
            this->apply_idle_arms(model, animation, 1.0f);
            for (const auto joint : {LeftArm, LeftElbow, RightArm, RightElbow}) {
                const auto bone = this->joints[joint];
                if (bone < animation.animation.size()) {
                    // Carry q_offset=inverse(q_source)*q_idle into normal pose smoothing.
                    this->offsets[joint] = glm::normalize(
                        glm::inverse(this->originals[joint]) * animation.animation[bone].rotation);
                }
            }
        }
        if (walk_time > 0) {
            this->apply_hanging_walk(model, animation, walk_time);
            this->update_hanging_ik_targets(model, animation);
            return true;
        }
        this->translate_hanging_roots(animation, this->hanging_state.return_offset);
        const float phase =
            std::clamp(this->hanging_state.landing_time / HANGING_DROP_SECONDS, 0.0f, 1.0f);
        const float rotation_weight = glm::smoothstep(0.0f, 1.0f, phase);
        // Falling displacement grows as t^2; preserve release X/Z until the walking phase.
        const float position_weight = phase * phase;
        for (std::size_t bone = 0; bone < animation.animation.size(); ++bone) {
            auto& pose = animation.animation[bone];
            const auto& start = this->hanging_state.display_pose[bone];
            pose.position = glm::mix(start.position, pose.position, position_weight);
            pose.rotation = glm::slerp(start.rotation, pose.rotation, rotation_weight);
            pose.scale = glm::mix(start.scale, pose.scale, rotation_weight);
        }
        this->update_globals(model, animation);
        // Undo only the horizontal sweep caused by straightening around the root pivot.
        auto sweep = this->hanging_state.display_center -
                     glm::vec3(this->transforms.globals[center_bone][3]);
        sweep.y = 0;
        this->translate_hanging_roots(animation, sweep);
        this->update_globals(model, animation);
        if (this->hanging_state.landing_time > HANGING_DROP_SECONDS) {
            // sin^2(pi*t/T) bends the knees on contact and smoothly returns to standing.
            const float bend = std::sin(glm::pi<float>() *
                                        (this->hanging_state.landing_time - HANGING_DROP_SECONDS) /
                                        HANGING_LANDING_SECONDS);
            this->crouch(model, animation, bend * bend);
        }
        this->update_hanging_ik_targets(model, animation);
        return true;
    }

    bool CursorPose::apply_hanging(const component::ModelComponent& model,
        component::AnimationComponent& animation,
        const std::optional<CursorRay>& cursor,
        float seconds) {
        if (this->hanging_state.landing) {
            return this->apply_hanging_landing(model, animation, seconds);
        }
        if (!this->hanging_state.active && !(this->hanging_state.weight > EPSILON)) {
            return false;
        }
        const float dt = std::isfinite(seconds) ? std::clamp(seconds, 0.0f, MAX_DT) : 0;
        // Exponential damping alpha=1-exp(-rate*dt), independent of frame rate.
        const float alpha = 1 - std::exp(-RESPONSE * dt);
        if (this->hanging_state.active && cursor.has_value()) {
            const float denominator = glm::dot(cursor->ray.direction, this->hanging_state.normal);
            if (std::abs(denominator) > EPSILON) {
                // Intersect the pointer with the fixed camera-facing plane through the grabbed
                // hand.
                const float t = glm::dot(this->hanging_state.anchor - cursor->ray.origin,
                                    this->hanging_state.normal) /
                                denominator;
                if (t > 0 && std::isfinite(t)) {
                    this->hanging_state.target = cursor->ray.origin + t * cursor->ray.direction;
                }
            }
        }
        const bool above_head = this->hanging_state.active &&
                                this->hanging_state.target.y > this->hanging_state.min_height;
        // Entry progress t=min(1, elapsed/ENTRY_SECONDS); release retains exponential damping.
        this->hanging_state.entry_time =
            (std::min)(ENTRY_SECONDS, this->hanging_state.entry_time + dt);
        this->hanging_state.weight =
            above_head ? (std::min)(1.0f, this->hanging_state.weight + dt / ENTRY_SECONDS)
                       : this->hanging_state.weight * (1 - alpha);
        if (!(this->hanging_state.weight > EPSILON)) {
            this->hanging_state.weight = 0;
            if (!this->hanging_state.active) {
                this->end_hanging();
            }
            return false;
        }
        this->hanging_state.originals = animation.animation;
        animation.animation = this->hanging_state.pose;
        this->prepare_hanging_limbs(model, animation);
        const auto wrist = this->wrists[this->hanging_state.side];
        const auto hips = this->body_bones[BodyHips];
        const auto center =
            hips < this->transforms.globals.size()
                ? glm::vec3(this->transforms.globals[hips][3])
                : glm::vec3(this->transforms.globals[this->locomotion.roots.front()][3]);
        const auto lever = center - glm::vec3(this->transforms.globals[wrist][3]);
        this->hanging_state.lag += alpha * (this->hanging_state.target - this->hanging_state.lag);
        // A lagging support creates sway; cap displacement relative to the suspension length.
        auto sway = this->hanging_state.lag - this->hanging_state.target;
        const float length = (std::max)(glm::length(lever), EPSILON);
        if (glm::length(sway) > length * SWAY_RATIO) {
            sway = glm::normalize(sway) * length * SWAY_RATIO;
        }
        auto correction = align(lever, DOWN * length + sway);
        // Legs are aligned to gravity below. Bound the torso correction so a lateral
        // grasp cannot create a large torso/leg angle: theta=2*acos(|q.w|), t=limit/theta.
        const float angle = 2 * std::acos(std::clamp(std::abs(correction.w), 0.0f, 1.0f));
        const float limit = glm::radians(MAX_BODY_LEAN_DEGREES);
        if (angle > limit) {
            correction = glm::slerp(glm::quat(1, 0, 0, 0), correction, limit / angle);
        }
        const auto origin = glm::vec3(this->transforms.globals[this->locomotion.roots.front()][3]);
        for (const auto root : this->locomotion.roots) {
            const auto axes = glm::mat3(this->transforms.external[root]);
            if (std::abs(glm::determinant(axes)) > EPSILON) {
                // Independent roots share the same pivot: p'=origin+R*(p-origin).
                const auto offset = glm::vec3(this->transforms.globals[root][3]) - origin;
                animation.animation[root].position +=
                    glm::inverse(axes) * (correction * offset - offset);
            }
            const auto q = rotation(this->transforms.globals[root]);
            animation.animation[root].rotation *= glm::inverse(q) * correction * q;
        }
        this->update_globals(model, animation);
        const auto free_side = 1 - this->hanging_state.side;
        // Straightening the elbow alone preserves the captured sideways shoulder direction.
        // Align the support upward independently of the bounded torso lean before anchoring it.
        this->hang_limb(model,
            animation,
            this->joints[this->hanging_state.side == 0 ? LeftArm : RightArm],
            wrist,
            -DOWN);
        this->hang_limb(model,
            animation,
            this->joints[free_side == 0 ? LeftArm : RightArm],
            this->wrists[free_side],
            DOWN);
        for (const auto& leg : this->locomotion.legs) {
            this->hang_limb(model, animation, leg[0], leg[2], DOWN);
        }
        // Scale the small suspension lift to the arm length, independent of model units.
        const glm::vec3 lift(
            0, this->arm_frame(this->hanging_state.side).length * LIFT_ARM_RATIO, 0);
        const auto delta =
            this->hanging_state.target + lift - glm::vec3(this->transforms.globals[wrist][3]);
        for (const auto root : this->locomotion.roots) {
            const auto axes = glm::mat3(this->transforms.external[root]);
            if (std::abs(glm::determinant(axes)) > EPSILON) {
                // G=external*T*R*S, so local displacement=inverse(external.axes)*world
                // displacement.
                animation.animation[root].position += glm::inverse(axes) * delta;
            }
        }
        this->blend_hanging_pose(animation);
        this->update_globals(model, animation);
        if (above_head) {
            // Blending joint rotations moves the wrist off its anchor. Correct translation
            // after blending: delta=target+smoothstep(weight)*lift-blended_wrist.
            // Only the lift eases in; dragging must follow the pointer from the first frame.
            const float weight = glm::smoothstep(0.0f, 1.0f, this->hanging_state.weight);
            this->translate_hanging_roots(animation,
                this->hanging_state.target + weight * lift -
                    glm::vec3(this->transforms.globals[wrist][3]));
            this->update_globals(model, animation);
        }
        this->hanging_state.display_center =
            glm::vec3(this->transforms.globals[hips < this->transforms.globals.size()
                                                   ? hips
                                                   : this->locomotion.roots.front()][3]);
        this->update_hanging_ik_targets(model, animation);
        this->hanging_state.display_pose = animation.animation;
        return true;
    }
} // namespace enishi::animation
