#include "cursor_pose.h"
#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <initializer_list>
#include <string_view>

namespace enishi::animation {
    namespace {
        constexpr glm::quat IDENTITY(1, 0, 0, 0);
        constexpr float EPSILON = 0.00001f;
        constexpr float RESPONSE_RATE = 8.0f;
        constexpr float PROXIMITY_RATIO = 0.18f;
        constexpr float TARGET_DEPTH_RATIO = 0.12f;
        constexpr float GAZE_DEPTH_RATIO = 0.25f;
        constexpr float FACE_HEIGHT_RATIO = 0.5f;

        types::BoneIndex find_bone(
            const component::ModelComponent& model, std::initializer_list<std::string_view> names) {
            for (const auto name : names) {
                const auto found = std::ranges::find(model.bone_names, name);
                if (found != model.bone_names.end()) {
                    return static_cast<types::BoneIndex>(found - model.bone_names.begin());
                }
            }
            return types::INVALID_BONE_INDEX;
        }

        bool descendant(const component::ModelComponent& model,
            types::BoneIndex child,
            types::BoneIndex parent) {
            for (std::size_t step = 0; step < model.bone_node.size(); ++step) {
                if (!(child < model.bone_node.size())) {
                    return false;
                }
                child = model.bone_node[child].parent;
                if (child == parent && parent < model.bone_node.size()) {
                    return true;
                }
            }
            return false;
        }

    } // namespace

    CursorPose::CursorPose(const component::ModelComponent& model, CursorPoseConfig config)
        : joints{find_bone(model, {"首", "neck", "Neck"}),
              find_bone(model, {"頭", "head", "Head"}),
              find_bone(model, {"左腕", "leftUpperArm", "LeftArm"}),
              find_bone(model, {"左ひじ", "左肘", "leftLowerArm", "LeftForeArm"}),
              find_bone(model, {"右腕", "rightUpperArm", "RightArm"}),
              find_bone(model, {"右ひじ", "右肘", "rightLowerArm", "RightForeArm"})}
        , wrists{find_bone(model, {"左手首", "leftHand", "LeftHand"}),
              find_bone(model, {"右手首", "rightHand", "RightHand"})} {
        this->config = config;
        this->eyes = {find_bone(model, {"左目", "leftEye", "LeftEye"}),
            find_bone(model, {"右目", "rightEye", "RightEye"})};
        this->offsets.fill(IDENTITY);
        this->originals.fill(IDENTITY);
        for (auto& index : this->joints) {
            for (const auto& body : model.rigid_bodies) {
                if (body.relate_bone_index == index &&
                    body.kind != types::RigidBodyKind::Kinematic) {
                    index = types::INVALID_BONE_INDEX;
                }
            }
            for (const auto& constraint : model.bone_constraints.constraints) {
                if (constraint.bone == index && constraint.rotation_axis.has_value()) {
                    index = types::INVALID_BONE_INDEX;
                }
            }
        }
        for (std::size_t side = 0; side < this->wrists.size(); ++side) {
            const auto arm = side == 0 ? LeftArm : RightArm;
            const auto elbow = side == 0 ? LeftElbow : RightElbow;
            if (!descendant(model, this->joints[elbow], this->joints[arm]) ||
                !descendant(model, this->wrists[side], this->joints[elbow])) {
                this->joints[arm] = types::INVALID_BONE_INDEX;
                this->joints[elbow] = types::INVALID_BONE_INDEX;
                this->wrists[side] = types::INVALID_BONE_INDEX;
            }
        }
        this->initialize_arms(model);
        for (std::size_t side = 0; side < this->wrists.size(); ++side) {
            for (types::BoneIndex bone = 0; bone < model.bone_node.size(); ++bone) {
                if (descendant(model, bone, this->wrists[side])) {
                    this->hand_reach_bones[side].push_back(bone);
                }
            }
        }
        this->initialize_legs(model);
        this->idle.torso = find_bone(model, {"上半身", "spine", "Spine", "chest", "Chest"});
        for (const auto& body : model.rigid_bodies) {
            if (body.relate_bone_index == this->idle.torso &&
                body.kind != types::RigidBodyKind::Kinematic) {
                this->idle.torso = types::INVALID_BONE_INDEX;
            }
        }
        for (const auto& constraint : model.bone_constraints.constraints) {
            if (constraint.bone == this->idle.torso && constraint.rotation_axis.has_value()) {
                this->idle.torso = types::INVALID_BONE_INDEX;
            }
        }
        if (this->config.experimental_obb_collision) {
            this->initialize_collision(model);
        }
        // Build a parent-first traversal without assuming source bone indices are sorted.
        std::vector<std::vector<types::BoneIndex>> children(model.bone_node.size());
        for (types::BoneIndex index = 0; index < model.bone_node.size(); ++index) {
            const auto parent = model.bone_node[index].parent;
            if (parent < children.size()) {
                children[parent].push_back(index);
            } else {
                this->transforms.traversal.push_back(index);
                this->locomotion.roots.push_back(index);
            }
        }
        for (std::size_t cursor = 0; cursor < this->transforms.traversal.size(); ++cursor) {
            const auto& next = children[this->transforms.traversal[cursor]];
            this->transforms.traversal.insert(
                this->transforms.traversal.end(), next.begin(), next.end());
        }
        this->transforms.globals.resize(model.bone_node.size(), glm::mat4(1));
        this->locomotion.original_root_positions.resize(this->locomotion.roots.size());
        this->transforms.external.resize(model.bone_node.size(), glm::mat4(1));
        this->transforms.cached_local.resize(model.bone_node.size());
        this->transforms.cached_external.resize(model.bone_node.size(), glm::mat4(1));
        this->transforms.changed_globals.resize(model.bone_node.size(), false);
    }

    void CursorPose::update_globals(
        const component::ModelComponent& model, const component::AnimationComponent& animation) {
        for (const auto index : this->transforms.traversal) {
            const auto& local = animation.animation[index];
            const auto parent = model.bone_node[index].parent;
            const auto& cached = this->transforms.cached_local[index];
            const bool changed =
                !this->transforms.valid || local.position != cached.position ||
                local.rotation != cached.rotation || local.scale != cached.scale ||
                this->transforms.external[index] != this->transforms.cached_external[index] ||
                (parent < this->transforms.globals.size() &&
                    this->transforms.changed_globals[parent]);
            this->transforms.changed_globals[index] = changed;
            if (!changed) {
                continue;
            }
            this->transforms.cached_local[index] = local;
            this->transforms.cached_external[index] = this->transforms.external[index];
            const auto parent_global = parent < this->transforms.globals.size()
                                           ? this->transforms.globals[parent]
                                           : glm::mat4(1);
            // G = external * G_parent * T * R * S, matching the skinning hierarchy.
            this->transforms.globals[index] = this->transforms.external[index] * parent_global *
                                              glm::translate(glm::mat4(1), local.position) *
                                              glm::mat4_cast(local.rotation) *
                                              glm::scale(glm::mat4(1), local.scale);
        }
        this->transforms.valid = true;
    }

    std::optional<glm::vec3> CursorPose::target(const CursorRay& cursor) const {
        if (this->transforms.globals.empty()) {
            return std::nullopt;
        }
        auto low = glm::vec3(this->transforms.globals.front()[3]);
        auto high = low;
        for (const auto& global : this->transforms.globals) {
            low = (glm::min)(low, glm::vec3(global[3]));
            high = (glm::max)(high, glm::vec3(global[3]));
        }
        const float height = high.y - low.y;
        const float denominator = glm::dot(cursor.ray.direction, cursor.plane_normal);
        if (!(height > EPSILON) || !(std::abs(denominator) > EPSILON)) {
            return std::nullopt;
        }
        // Intersect the cursor ray with a camera-facing plane just in front of the body:
        // t = dot(center - origin, normal) / dot(direction, normal), target = origin + t*direction.
        const auto center = (low + high) * 0.5f - cursor.plane_normal * height * TARGET_DEPTH_RATIO;
        const float distance =
            glm::dot(center - cursor.ray.origin, cursor.plane_normal) / denominator;
        const auto point = cursor.ray.origin + cursor.ray.direction * distance;
        if (!(distance > 0) || !std::isfinite(distance)) {
            return std::nullopt;
        }
        // Ignore depth in the proximity test: the target plane is deliberately in front.
        const auto planar = glm::vec2(point);
        auto proximity_high = high;
        if (this->config.jump_enabled) {
            proximity_high.y += height * this->config.jump_height_ratio;
        }
        const auto closest = glm::clamp(planar, glm::vec2(low), glm::vec2(proximity_high));
        if (glm::length(planar - closest) > height * PROXIMITY_RATIO) {
            return std::nullopt;
        }
        return point;
    }

    std::optional<glm::vec3> CursorPose::gaze_offset(const CursorRay& cursor) const {
        const auto head = this->body_bones[BodyHead];
        const auto neck = this->body_bones[BodyNeck];
        const auto anchor = head < this->transforms.globals.size() ? head : neck;
        if (!(anchor < this->transforms.globals.size())) {
            return std::nullopt;
        }
        auto center = glm::vec3(this->transforms.globals[anchor][3]);
        if (this->eyes[0] < this->transforms.globals.size() &&
            this->eyes[1] < this->transforms.globals.size()) {
            center = (glm::vec3(this->transforms.globals[this->eyes[0]][3]) +
                         glm::vec3(this->transforms.globals[this->eyes[1]][3])) *
                     0.5f;
        } else if (head < this->transforms.globals.size() &&
                   neck < this->transforms.globals.size()) {
            // Without eye bones, estimate the face center half a neck-to-head length
            // above the head pivot: c = head + up * |head-neck|/2.
            const auto up = glm::mat3(this->transforms.globals[head]) * this->head_local_up;
            if (glm::length(up) > EPSILON) {
                center += glm::normalize(up) * FACE_HEIGHT_RATIO *
                          glm::length(center - glm::vec3(this->transforms.globals[neck][3]));
            }
        }
        auto low = glm::vec3(this->transforms.globals.front()[3]);
        auto high = low;
        for (const auto& global : this->transforms.globals) {
            low = (glm::min)(low, glm::vec3(global[3]));
            high = (glm::max)(high, glm::vec3(global[3]));
        }
        const float depth = (high.y - low.y) * GAZE_DEPTH_RATIO;
        const float denominator = glm::dot(cursor.ray.direction, cursor.plane_normal);
        if (!(depth > EPSILON) || !(std::abs(denominator) > EPSILON)) {
            return std::nullopt;
        }
        // Intersect a camera-facing plane THROUGH the face, not the torso:
        // t=dot(c-origin,n)/dot(ray,n), offset=(origin+t*ray-c)/depth.
        // The projected face center therefore has zero offset at every camera angle.
        const float distance =
            glm::dot(center - cursor.ray.origin, cursor.plane_normal) / denominator;
        if (!(distance > 0) || !std::isfinite(distance)) {
            return std::nullopt;
        }
        return (cursor.ray.origin + distance * cursor.ray.direction - center) / depth;
    }

    void CursorPose::look(component::AnimationComponent& animation,
        Joint joint,
        const glm::vec3& offset,
        float yaw_limit,
        float pitch_limit) {
        const auto index = this->joints[joint];
        if (!(index < this->transforms.globals.size())) {
            return;
        }
        const auto& global = this->transforms.globals[index];
        if (!(std::abs(glm::determinant(glm::mat3(global))) > EPSILON)) {
            return;
        }
        auto axes = glm::mat3(global);
        for (glm::length_t column = 0; column < axes.length(); ++column) {
            axes[column] = glm::normalize(axes[column]);
        }
        const auto local = glm::inverse(axes) * offset;
        // Keep the virtual goal strictly ahead: d=(offset.x,offset.y,-1).
        // Unlike a nearby world-space point, this cannot cross atan2's rear-facing branch cut.
        const glm::vec3 direction(local.x, local.y, -1.0f);
        // MMD faces local -Z. yaw=atan2(-x,-z), pitch=atan2(y,sqrt(x*x+z*z)).
        const float yaw = std::clamp(std::atan2(-direction.x, -direction.z),
            -glm::radians(yaw_limit),
            glm::radians(yaw_limit));
        const float pitch =
            std::clamp(std::atan2(direction.y, glm::length(glm::vec2(direction.x, direction.z))),
                -glm::radians(pitch_limit),
                glm::radians(pitch_limit));
        animation.animation[index].rotation *=
            glm::angleAxis(yaw, glm::vec3(0, 1, 0)) * glm::angleAxis(pitch, glm::vec3(1, 0, 0));
    }

    void CursorPose::reach(const component::ModelComponent& model,
        component::AnimationComponent& animation,
        const glm::vec3& target) {
        std::array<glm::vec3, 2> goals{target, target};
        // Cooldown gates the next jump, not the effort to reach its target after landing.
        const bool stretch = this->locomotion.wait > 0 || this->locomotion.height > 0 ||
                             this->desired_jump_height(target) > 0;
        for (std::size_t side = 0; side < this->wrists.size(); ++side) {
            if ((!stretch && !this->cursor_in_arm_reach(side)) ||
                !(this->wrists[side] < this->transforms.globals.size()) ||
                this->can_reach(side, target)) {
                continue;
            }
            const auto shoulder = glm::vec3(
                this->transforms.globals[this->joints[side == 0 ? LeftArm : RightArm]][3]);
            const auto delta = target - shoulder;
            if (!(delta.y > 0)) {
                continue;
            }
            // Stretch toward the cursor within the reach sphere, including after landing:
            // goal = shoulder + normalize(target-shoulder) * arm_length * margin.
            constexpr float JUMP_REACH_MARGIN = 0.95f;
            goals[side] =
                shoulder + glm::normalize(delta) * this->arm_frame(side).length * JUMP_REACH_MARGIN;
        }
        const std::array<bool, 2> in_range{
            this->can_reach(0, goals[0]), this->can_reach(1, goals[1])};
        const bool both = in_range[0] && in_range[1];
        for (std::size_t side = 0; side < this->wrists.size(); ++side) {
            auto goal = goals[side];
            if (both) {
                // Separate goals by the sum of the hand envelopes, plus a small gap.
                constexpr float HAND_GAP_RATIO = 1.1f;
                goal += this->arm_frame(side).outward * this->hand_radius(side) * HAND_GAP_RATIO;
            }
            this->active_arms[side] =
                in_range[side] && this->reach_side(model, animation, side, goal);
        }
    }

    void CursorPose::apply(const component::ModelComponent& model,
        component::AnimationComponent& animation,
        const std::optional<CursorRay>& cursor,
        float seconds,
        bool idle_enabled) {
        if (this->applied || animation.animation.size() != this->transforms.globals.size() ||
            this->transforms.traversal.size() != this->transforms.globals.size()) {
            return;
        }
        this->idle.applied = false;
        this->reach_ray = cursor.has_value() ? std::optional(cursor->ray) : std::nullopt;
        for (std::size_t joint = 0; joint < Count; ++joint) {
            if (this->joints[joint] < animation.animation.size()) {
                this->originals[joint] = animation.animation[this->joints[joint]].rotation;
            }
        }
        std::fill(this->transforms.external.begin(), this->transforms.external.end(), glm::mat4(1));
        for (const auto& constraint : model.bone_constraints.constraints) {
            if (constraint.bone < this->transforms.external.size() &&
                constraint.external_transform_slot.has_value()) {
                const auto found =
                    model.external_transforms.find(*constraint.external_transform_slot);
                if (found != model.external_transforms.end()) {
                    this->transforms.external[constraint.bone] = found->second;
                }
            }
        }
        // Dynamic clothing uses the last completed physics pose, rather than its source
        // animation pose. Candidate arm poses must not move these obstacles during search.
        if (this->config.experimental_obb_collision) {
            this->collision.physics_globals = animation.global;
        }
        this->update_globals(model, animation);
        if (idle_enabled || this->idle.weight > EPSILON) {
            this->idle.originals = animation.animation;
        }
        if (this->apply_hanging(model, animation, cursor, seconds)) {
            this->apply_idle(model, animation, idle_enabled, seconds);
            if (this->hanging_state.active) {
                const auto hips = this->body_bones[BodyHips];
                const auto center =
                    hips < this->transforms.globals.size() ? hips : this->locomotion.roots.front();
                this->hanging_state.display_center = glm::vec3(this->transforms.globals[center][3]);
                this->hanging_state.display_pose = animation.animation;
            }
            this->applied = true;
            return;
        }
        auto point = cursor.has_value() ? this->target(*cursor) : std::nullopt;
        this->apply_jump(model, animation, point, seconds);
        this->apply_idle(model, animation, idle_enabled, seconds);
        if (this->locomotion.height > 0 && cursor.has_value()) {
            point = this->target(*cursor);
        }
        if (point.has_value()) {
            const auto gaze = this->gaze_offset(*cursor);
            if (gaze.has_value()) {
                this->look(animation, Neck, *gaze, this->config.neck_yaw, this->config.neck_pitch);
                this->update_globals(model, animation);
                this->look(animation, Head, *gaze, this->config.head_yaw, this->config.head_pitch);
                this->update_globals(model, animation);
            }
            this->reach(model, animation, *point);
        } else {
            this->active_arms.fill(false);
        }
        const auto previous = this->offsets;
        // Exponential smoothing alpha=1-exp(-rate*dt) is independent of frame rate.
        float alpha = std::isfinite(seconds)
                          ? 1.0f - std::exp(-RESPONSE_RATE * (std::max)(0.0f, seconds))
                          : 0.0f;
        if (this->transition_state.initializing) {
            alpha = 1;
        }
        for (std::size_t joint = 0; joint < Count; ++joint) {
            const auto index = this->joints[joint];
            if (!(index < animation.animation.size())) {
                continue;
            }
            const auto desired = glm::normalize(
                glm::inverse(this->originals[joint]) * animation.animation[index].rotation);
            this->offsets[joint] = glm::normalize(glm::slerp(this->offsets[joint], desired, alpha));
            animation.animation[index].rotation =
                glm::normalize(this->originals[joint] * this->offsets[joint]);
        }
        this->protect_arms(model, animation);
        if (!this->transition_state.initializing) {
            this->limit_arm_speed(model, animation, previous, seconds);
        }
        // Limiting each arm changes the obstacle seen by the other arm. Recheck the pair
        // together; clearance takes precedence if the two limited trajectories intersect.
        this->protect_arms(model, animation);
        this->applied = true;
    }

    void CursorPose::restore(component::AnimationComponent& animation) {
        if (!this->applied) {
            return;
        }
        if (!this->idle.originals.empty()) {
            animation.animation = this->idle.originals;
            this->idle.originals.clear();
            this->hanging_state.originals.clear();
            this->applied = false;
            return;
        }
        if (!this->hanging_state.originals.empty()) {
            animation.animation = this->hanging_state.originals;
            this->hanging_state.originals.clear();
            this->applied = false;
            return;
        }
        for (std::size_t index = 0; index < this->locomotion.roots.size(); ++index) {
            animation.animation[this->locomotion.roots[index]].position =
                this->locomotion.original_root_positions[index];
        }
        for (const auto& [index, original] : this->locomotion.leg_originals) {
            animation.animation[index] = original;
        }
        for (std::size_t joint = 0; joint < Count; ++joint) {
            if (this->joints[joint] < animation.animation.size()) {
                animation.animation[this->joints[joint]].rotation = this->originals[joint];
            }
        }
        this->applied = false;
    }
} // namespace enishi::animation
