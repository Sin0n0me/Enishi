#include "cursor_pose.h"
#include <algorithm>
#include <animation/procedural/cursor/cursor_collision.h>
#include <animation/procedural/cursor/cursor_gaze.h>
#include <animation/procedural/cursor/cursor_reach.h>
#include <animation/procedural/cursor/cursor_transforms.h>
#include <animation/procedural/hanging/cursor_hanging.h>
#include <animation/procedural/idle/cursor_idle.h>
#include <animation/procedural/jump/cursor_jump.h>
#include <animation/procedural/legs/cursor_legs.h>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

namespace enishi::animation {
    using enum CursorPoseState::Joint;
    using enum CursorPoseState::BodyBone;
    namespace {
        constexpr glm::quat IDENTITY(1, 0, 0, 0);
        constexpr float EPSILON = 0.00001f;
        constexpr float RESPONSE_RATE = 8.0f;

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

    CursorPose::CursorPose(const component::ModelComponent& model,
        CursorPoseConfig config,
        const BoneNameDictionary& bone_names) {
        this->state.joints = {bone_names.find(model.bone_names, BoneRole::Neck),
            bone_names.find(model.bone_names, BoneRole::Head),
            bone_names.find(model.bone_names, BoneRole::LeftUpperArm),
            bone_names.find(model.bone_names, BoneRole::LeftLowerArm),
            bone_names.find(model.bone_names, BoneRole::RightUpperArm),
            bone_names.find(model.bone_names, BoneRole::RightLowerArm)};
        this->state.wrists = {bone_names.find(model.bone_names, BoneRole::LeftHand),
            bone_names.find(model.bone_names, BoneRole::RightHand)};
        this->state.config = config;
        this->state.eyes = {bone_names.find(model.bone_names, BoneRole::LeftEye),
            bone_names.find(model.bone_names, BoneRole::RightEye)};
        this->state.offsets.fill(IDENTITY);
        this->state.originals.fill(IDENTITY);
        for (auto& index : this->state.joints) {
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
        for (std::size_t side = 0; side < this->state.wrists.size(); ++side) {
            const auto arm = side == 0 ? LeftArm : RightArm;
            const auto elbow = side == 0 ? LeftElbow : RightElbow;
            if (!descendant(model, this->state.joints[elbow], this->state.joints[arm]) ||
                !descendant(model, this->state.wrists[side], this->state.joints[elbow])) {
                this->state.joints[arm] = types::INVALID_BONE_INDEX;
                this->state.joints[elbow] = types::INVALID_BONE_INDEX;
                this->state.wrists[side] = types::INVALID_BONE_INDEX;
            }
        }
        CursorReach::initialize_arms(this->state, model, bone_names);
        for (std::size_t side = 0; side < this->state.wrists.size(); ++side) {
            for (types::BoneIndex bone = 0; bone < model.bone_node.size(); ++bone) {
                if (descendant(model, bone, this->state.wrists[side])) {
                    this->state.hand_reach_bones[side].push_back(bone);
                }
            }
        }
        CursorLegs::initialize_legs(this->state, model, bone_names);
        this->state.idle.torso = bone_names.find(model.bone_names, BoneRole::Spine);
        for (const auto& body : model.rigid_bodies) {
            if (body.relate_bone_index == this->state.idle.torso &&
                body.kind != types::RigidBodyKind::Kinematic) {
                this->state.idle.torso = types::INVALID_BONE_INDEX;
            }
        }
        for (const auto& constraint : model.bone_constraints.constraints) {
            if (constraint.bone == this->state.idle.torso && constraint.rotation_axis.has_value()) {
                this->state.idle.torso = types::INVALID_BONE_INDEX;
            }
        }
        if (this->state.config.experimental_obb_collision) {
            CursorCollision::initialize_collision(this->state, model);
        }
        // Build a parent-first traversal without assuming source bone indices are sorted.
        std::vector<std::vector<types::BoneIndex>> children(model.bone_node.size());
        for (types::BoneIndex index = 0; index < model.bone_node.size(); ++index) {
            const auto parent = model.bone_node[index].parent;
            if (parent < children.size()) {
                children[parent].push_back(index);
            } else {
                this->state.transforms.traversal.push_back(index);
                this->state.locomotion.roots.push_back(index);
            }
        }
        for (std::size_t cursor = 0; cursor < this->state.transforms.traversal.size(); ++cursor) {
            const auto& next = children[this->state.transforms.traversal[cursor]];
            this->state.transforms.traversal.insert(
                this->state.transforms.traversal.end(), next.begin(), next.end());
        }
        this->state.transforms.globals.resize(model.bone_node.size(), glm::mat4(1));
        this->state.locomotion.original_root_positions.resize(this->state.locomotion.roots.size());
        this->state.transforms.external.resize(model.bone_node.size(), glm::mat4(1));
        this->state.transforms.cached_local.resize(model.bone_node.size());
        this->state.transforms.cached_external.resize(model.bone_node.size(), glm::mat4(1));
        this->state.transforms.changed_globals.resize(model.bone_node.size(), false);
    }

    void CursorPose::apply_transitioned(const component::ModelComponent& model,
        component::AnimationComponent& animation,
        const std::optional<CursorRay>& cursor,
        float seconds,
        bool idle_enabled) {
        if (this->state.applied ||
            animation.animation.size() != this->state.transforms.globals.size() ||
            this->state.transforms.traversal.size() != this->state.transforms.globals.size()) {
            animation.pose_transition_active = false;
            return;
        }
        const auto source = animation.animation;
        const auto requested = this->pose_stage(animation);
        const bool interrupted = requested != this->state.transition_state.stage ||
                                 animation.motion_revision != this->state.transition_state.revision;
        this->state.transition_state.frame = false;
        bool started = false;
        if (!this->state.transition_state.transition.active() || interrupted) {
            this->apply(model, animation, cursor, seconds, idle_enabled);
            const auto next = this->pose_stage(animation);
            const bool changed = next != this->state.transition_state.stage ||
                                 animation.motion_revision != this->state.transition_state.revision;
            // New interactive actions have their own continuous entry motion. Blend only
            // completed actions back to cursor posing, or switches involving a file clip.
            const bool blend = next == PoseStage::Cursor || next == PoseStage::File ||
                               this->state.transition_state.stage == PoseStage::File;
            if (changed) {
                this->state.transition_state.transition.cancel();
            }
            if (!this->state.transition_state.display.empty() && changed && blend) {
                this->restore(animation);

                this->state.transition_state.initializing = true;
                this->apply(model, animation, cursor, 0, idle_enabled);
                this->state.transition_state.initializing = false;
                this->state.transition_state.transition.begin(
                    this->state.transition_state.display, animation.animation);
                started = true;
            }
            this->state.transition_state.stage = next;
            this->state.transition_state.revision = animation.motion_revision;
        }
        if (this->state.transition_state.transition.active()) {
            // Do not evaluate procedural clocks while the captured destination pose is held.
            this->state.transition_state.frame = true;
            this->state.transition_state.transition.apply(
                started ? 0 : seconds, animation.animation);
        }
        this->state.idle.originals = source;
        this->state.applied = true;
        animation.pose_transition_active = this->state.transition_state.transition.active();
        CursorTransforms::update_globals(this->state.transforms, model, animation);
        this->remember_transition_pose(animation);
    }

    void CursorPose::apply(const component::ModelComponent& model,
        component::AnimationComponent& animation,
        const std::optional<CursorRay>& cursor,
        float seconds,
        bool idle_enabled) {
        if (this->state.applied ||
            animation.animation.size() != this->state.transforms.globals.size() ||
            this->state.transforms.traversal.size() != this->state.transforms.globals.size()) {
            return;
        }
        this->state.idle.applied = false;
        this->state.reach_ray = cursor.has_value() ? std::optional(cursor->ray) : std::nullopt;
        for (std::size_t joint = 0; joint < Count; ++joint) {
            if (this->state.joints[joint] < animation.animation.size()) {
                this->state.originals[joint] =
                    animation.animation[this->state.joints[joint]].rotation;
            }
        }
        std::fill(this->state.transforms.external.begin(),
            this->state.transforms.external.end(),
            glm::mat4(1));
        for (const auto& constraint : model.bone_constraints.constraints) {
            if (constraint.bone < this->state.transforms.external.size() &&
                constraint.external_transform_slot.has_value()) {
                const auto found =
                    model.external_transforms.find(*constraint.external_transform_slot);
                if (found != model.external_transforms.end()) {
                    this->state.transforms.external[constraint.bone] = found->second;
                }
            }
        }
        // Dynamic clothing uses the last completed physics pose, rather than its source
        // animation pose. Candidate arm poses must not move these obstacles during search.
        if (this->state.config.experimental_obb_collision) {
            this->state.collision.physics_globals = animation.global;
        }
        CursorTransforms::update_globals(this->state.transforms, model, animation);
        if (idle_enabled || this->state.idle.weight > EPSILON) {
            this->state.idle.originals = animation.animation;
        }
        if (CursorHanging::apply_hanging(this->state, model, animation, cursor, seconds)) {
            CursorIdle::apply_idle(this->state, model, animation, idle_enabled, seconds);
            if (this->state.hanging_state.active) {
                const auto hips = this->state.body_bones[BodyHips];
                const auto center = hips < this->state.transforms.globals.size()
                                        ? hips
                                        : this->state.locomotion.roots.front();
                this->state.hanging_state.display_center =
                    glm::vec3(this->state.transforms.globals[center][3]);
                this->state.hanging_state.display_pose = animation.animation;
            }
            this->state.applied = true;
            return;
        }
        auto point = cursor.has_value() ? CursorGaze::target(this->state, *cursor) : std::nullopt;
        CursorJump::apply_jump(this->state, model, animation, point, seconds);
        CursorIdle::apply_idle(this->state, model, animation, idle_enabled, seconds);
        if (this->state.locomotion.height > 0 && cursor.has_value()) {
            point = CursorGaze::target(this->state, *cursor);
        }
        if (point.has_value()) {
            const auto gaze = CursorGaze::gaze_offset(this->state, *cursor);
            if (gaze.has_value()) {
                CursorGaze::look(this->state,
                    animation,
                    Neck,
                    *gaze,
                    this->state.config.neck_yaw,
                    this->state.config.neck_pitch);
                CursorTransforms::update_globals(this->state.transforms, model, animation);
                CursorGaze::look(this->state,
                    animation,
                    Head,
                    *gaze,
                    this->state.config.head_yaw,
                    this->state.config.head_pitch);
                CursorTransforms::update_globals(this->state.transforms, model, animation);
            }
            CursorReach::reach(this->state, model, animation, *point);
        } else {
            this->state.active_arms.fill(false);
        }
        const auto previous = this->state.offsets;
        // Exponential smoothing alpha=1-exp(-rate*dt) is independent of frame rate.
        float alpha = std::isfinite(seconds)
                          ? 1.0f - std::exp(-RESPONSE_RATE * (std::max)(0.0f, seconds))
                          : 0.0f;
        if (this->state.transition_state.initializing) {
            alpha = 1;
        }
        for (std::size_t joint = 0; joint < Count; ++joint) {
            const auto index = this->state.joints[joint];
            if (!(index < animation.animation.size())) {
                continue;
            }
            const auto desired = glm::normalize(
                glm::inverse(this->state.originals[joint]) * animation.animation[index].rotation);
            this->state.offsets[joint] =
                glm::normalize(glm::slerp(this->state.offsets[joint], desired, alpha));
            animation.animation[index].rotation =
                glm::normalize(this->state.originals[joint] * this->state.offsets[joint]);
        }
        CursorReach::protect_arms(this->state, model, animation);
        if (!this->state.transition_state.initializing) {
            CursorReach::limit_arm_speed(this->state, model, animation, previous, seconds);
        }
        // Limiting each arm changes the obstacle seen by the other arm. Recheck the pair
        // together; clearance takes precedence if the two limited trajectories intersect.
        CursorReach::protect_arms(this->state, model, animation);
        this->state.applied = true;
    }

    void CursorPose::restore(component::AnimationComponent& animation) {
        if (!this->state.applied) {
            return;
        }
        if (!this->state.idle.originals.empty()) {
            animation.animation = this->state.idle.originals;
            this->state.idle.originals.clear();
            this->state.hanging_state.originals.clear();
            this->state.applied = false;
            return;
        }
        if (!this->state.hanging_state.originals.empty()) {
            animation.animation = this->state.hanging_state.originals;
            this->state.hanging_state.originals.clear();
            this->state.applied = false;
            return;
        }
        for (std::size_t index = 0; index < this->state.locomotion.roots.size(); ++index) {
            animation.animation[this->state.locomotion.roots[index]].position =
                this->state.locomotion.original_root_positions[index];
        }
        for (const auto& [index, original] : this->state.locomotion.leg_originals) {
            animation.animation[index] = original;
        }
        for (std::size_t joint = 0; joint < Count; ++joint) {
            if (this->state.joints[joint] < animation.animation.size()) {
                animation.animation[this->state.joints[joint]].rotation =
                    this->state.originals[joint];
            }
        }
        this->state.applied = false;
    }

    std::optional<std::pair<std::size_t, float>> CursorPose::pick_hand(
        const component::AnimationComponent& animation, const CursorRay& cursor) const {
        return CursorHanging::pick_hand(this->state, animation, cursor);
    }

    void CursorPose::begin_hanging(
        std::size_t side, const component::AnimationComponent& animation, const CursorRay& cursor) {
        CursorHanging::begin_hanging(this->state, side, animation, cursor);
    }

    void CursorPose::end_hanging() {
        CursorHanging::end_hanging(this->state);
    }

    bool CursorPose::is_hanging() const {
        return CursorHanging::is_hanging(this->state);
    }

    bool CursorPose::suppress_hanging_ik(types::BoneIndex bone) const {
        return CursorHanging::suppress_hanging_ik(this->state, bone);
    }

    CursorPose::PoseStage CursorPose::pose_stage(
        const component::AnimationComponent& animation) const {
        if (animation.motion_playing) {
            return PoseStage::File;
        }
        // Suspension, falling, landing and walking form one continuous action.
        // Likewise, preparation and landing belong to the same jump as its flight.
        if (this->state.hanging_state.active || this->state.hanging_state.landing ||
            this->state.hanging_state.weight > 0) {
            return PoseStage::Hanging;
        }
        if (this->state.locomotion.height > 0) {
            return PoseStage::Jump;
        }
        return PoseStage::Cursor;
    }

    void CursorPose::remember_transition_pose(const component::AnimationComponent& animation) {
        this->state.transition_state.display = animation.animation;
        if (this->state.hanging_state.active && !this->state.locomotion.roots.empty()) {
            const auto hips = this->state.body_bones[BodyHips];
            const auto center = hips < this->state.transforms.globals.size()
                                    ? hips
                                    : this->state.locomotion.roots.front();
            this->state.hanging_state.display_center =
                glm::vec3(this->state.transforms.globals[center][3]);
            this->state.hanging_state.display_pose = animation.animation;
        }
    }
} // namespace enishi::animation
