#include "cursor_pose.h"

namespace enishi::core {
    void CursorPose::prepare_transition_stage(PoseStage stage) {
        switch (stage) {
            case PoseStage::Hanging:
                this->hanging_weight = 1;
                this->hanging_entry_time = static_cast<float>(PoseTransition::DURATION);
                break;
            case PoseStage::Drop:
                this->hanging_landing_time = 0;
                break;
            case PoseStage::Landing:
                this->hanging_landing_time = HANGING_DROP_SECONDS;
                break;
            case PoseStage::Walk:
                this->hanging_landing_time = HANGING_DROP_SECONDS + HANGING_LANDING_SECONDS;
                break;
            case PoseStage::Jump:
                this->jump_time = 0;
                break;
            case PoseStage::JumpLanding:
                this->jump_time = this->config.jump_duration;
                break;
            default:
                break;
        }
    }

    CursorPose::PoseStage CursorPose::pose_stage(const component::AnimationComponent& animation,
        const std::optional<CursorRay>& cursor) const {
        if (animation.motion_playing) {
            return PoseStage::File;
        }
        if (this->hanging_landing) {
            if (this->hanging_landing_time < HANGING_DROP_SECONDS) {
                return PoseStage::Drop;
            }
            return this->hanging_landing_time < HANGING_DROP_SECONDS + HANGING_LANDING_SECONDS
                       ? PoseStage::Landing
                       : PoseStage::Walk;
        }
        if (this->hanging && this->hanging_weight > 0) {
            return PoseStage::Hanging;
        }
        if (this->jump_height > 0) {
            if (this->jump_time < 0) {
                return PoseStage::JumpPreparation;
            }
            return this->jump_time < this->config.jump_duration ? PoseStage::Jump
                                                                : PoseStage::JumpLanding;
        }
        return cursor.has_value() && this->target(*cursor).has_value() ? PoseStage::Reach
                                                                       : PoseStage::Idle;
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
        const auto requested = this->pose_stage(animation, cursor);
        const bool interrupted = requested != this->transition_stage ||
                                 animation.motion_revision != this->transition_revision;
        this->transition_frame = false;
        bool started = false;
        if (!this->transition.active() || interrupted) {
            this->apply(model, animation, cursor, seconds, idle_enabled);
            const auto next = this->pose_stage(animation, cursor);
            if (!this->transition_display.empty() &&
                (next != this->transition_stage ||
                    animation.motion_revision != this->transition_revision)) {
                this->restore(animation);
                this->prepare_transition_stage(next);
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
