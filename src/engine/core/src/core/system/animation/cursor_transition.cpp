#include "cursor_pose.h"

namespace enishi::core {
    CursorPose::PoseStage CursorPose::pose_stage(
        const component::AnimationComponent& animation) const {
        if (animation.motion_playing) {
            return PoseStage::File;
        }
        // Suspension, falling, landing and walking form one continuous action.
        // Likewise, preparation and landing belong to the same jump as its flight.
        if (this->hanging || this->hanging_landing || this->hanging_weight > 0) {
            return PoseStage::Hanging;
        }
        if (this->jump_height > 0) {
            return PoseStage::Jump;
        }
        return PoseStage::Cursor;
    }
    void CursorPose::remember_transition_pose(const component::AnimationComponent& animation) {
        this->transition_display = animation.animation;
        if (this->hanging && !this->roots.empty()) {
            const auto hips = this->body_bones[BodyHips];
            const auto center = hips < this->globals.size() ? hips : this->roots.front();
            this->hanging_display_center = glm::vec3(this->globals[center][3]);
            this->hanging_display_pose = animation.animation;
        }
    }

    void CursorPose::apply_transitioned(const component::ModelComponent& model,
        component::AnimationComponent& animation,
        const std::optional<CursorRay>& cursor,
        float seconds,
        bool idle_enabled) {
        if (this->applied || animation.animation.size() != this->globals.size() ||
            this->traversal.size() != this->globals.size()) {
            animation.pose_transition_active = false;
            return;
        }
        const auto source = animation.animation;
        const auto requested = this->pose_stage(animation);
        const bool interrupted = requested != this->transition_stage ||
                                 animation.motion_revision != this->transition_revision;
        this->transition_frame = false;
        bool started = false;
        if (!this->transition.active() || interrupted) {
            this->apply(model, animation, cursor, seconds, idle_enabled);
            const auto next = this->pose_stage(animation);
            const bool changed = next != this->transition_stage ||
                                 animation.motion_revision != this->transition_revision;
            // New interactive actions have their own continuous entry motion. Blend only
            // completed actions back to cursor posing, or switches involving a file clip.
            const bool blend = next == PoseStage::Cursor || next == PoseStage::File ||
                               this->transition_stage == PoseStage::File;
            if (changed) {
                this->transition.cancel();
            }
            if (!this->transition_display.empty() && changed && blend) {
                this->restore(animation);

                this->transition_initializing = true;
                this->apply(model, animation, cursor, 0, idle_enabled);
                this->transition_initializing = false;
                this->transition.begin(this->transition_display, animation.animation);
                started = true;
            }
            this->transition_stage = next;
            this->transition_revision = animation.motion_revision;
        }
        if (this->transition.active()) {
            // Do not evaluate procedural clocks while the captured destination pose is held.
            this->transition_frame = true;
            this->transition.apply(started ? 0 : seconds, animation.animation);
        }
        this->idle_originals = source;
        this->applied = true;
        animation.pose_transition_active = this->transition.active();
        this->update_globals(model, animation);
        this->remember_transition_pose(animation);
    }
} // namespace enishi::core
