#include "cursor_pose.h"

namespace enishi::animation {
    CursorPose::PoseStage CursorPose::pose_stage(
        const component::AnimationComponent& animation) const {
        if (animation.motion_playing) {
            return PoseStage::File;
        }
        // Suspension, falling, landing and walking form one continuous action.
        // Likewise, preparation and landing belong to the same jump as its flight.
        if (this->hanging_state.active || this->hanging_state.landing ||
            this->hanging_state.weight > 0) {
            return PoseStage::Hanging;
        }
        if (this->locomotion.height > 0) {
            return PoseStage::Jump;
        }
        return PoseStage::Cursor;
    }
    void CursorPose::remember_transition_pose(const component::AnimationComponent& animation) {
        this->transition_state.display = animation.animation;
        if (this->hanging_state.active && !this->locomotion.roots.empty()) {
            const auto hips = this->body_bones[BodyHips];
            const auto center =
                hips < this->transforms.globals.size() ? hips : this->locomotion.roots.front();
            this->hanging_state.display_center = glm::vec3(this->transforms.globals[center][3]);
            this->hanging_state.display_pose = animation.animation;
        }
    }

    void CursorPose::apply_transitioned(const component::ModelComponent& model,
        component::AnimationComponent& animation,
        const std::optional<CursorRay>& cursor,
        float seconds,
        bool idle_enabled) {
        if (this->applied || animation.animation.size() != this->transforms.globals.size() ||
            this->transforms.traversal.size() != this->transforms.globals.size()) {
            animation.pose_transition_active = false;
            return;
        }
        const auto source = animation.animation;
        const auto requested = this->pose_stage(animation);
        const bool interrupted = requested != this->transition_state.stage ||
                                 animation.motion_revision != this->transition_state.revision;
        this->transition_state.frame = false;
        bool started = false;
        if (!this->transition_state.transition.active() || interrupted) {
            this->apply(model, animation, cursor, seconds, idle_enabled);
            const auto next = this->pose_stage(animation);
            const bool changed = next != this->transition_state.stage ||
                                 animation.motion_revision != this->transition_state.revision;
            // New interactive actions have their own continuous entry motion. Blend only
            // completed actions back to cursor posing, or switches involving a file clip.
            const bool blend = next == PoseStage::Cursor || next == PoseStage::File ||
                               this->transition_state.stage == PoseStage::File;
            if (changed) {
                this->transition_state.transition.cancel();
            }
            if (!this->transition_state.display.empty() && changed && blend) {
                this->restore(animation);

                this->transition_state.initializing = true;
                this->apply(model, animation, cursor, 0, idle_enabled);
                this->transition_state.initializing = false;
                this->transition_state.transition.begin(
                    this->transition_state.display, animation.animation);
                started = true;
            }
            this->transition_state.stage = next;
            this->transition_state.revision = animation.motion_revision;
        }
        if (this->transition_state.transition.active()) {
            // Do not evaluate procedural clocks while the captured destination pose is held.
            this->transition_state.frame = true;
            this->transition_state.transition.apply(started ? 0 : seconds, animation.animation);
        }
        this->idle.originals = source;
        this->applied = true;
        animation.pose_transition_active = this->transition_state.transition.active();
        this->update_globals(model, animation);
        this->remember_transition_pose(animation);
    }
} // namespace enishi::animation
