#include "animation_controller.h"
#include <algorithm>
#include <cmath>

namespace enishi::animation {
    void AnimationController::add_clip(
        std::string name, std::shared_ptr<const AnimationClipData> clip) {
        this->clips.insert_or_assign(std::move(name), std::move(clip));
    }

    bool AnimationController::play(const std::string& name, const bool restart) {
        const auto iter = this->clips.find(name);
        if (iter == this->clips.end() || !iter->second) {
            return false;
        }
        if (restart || this->active_clip != iter->second) {
            this->time = 0.0f;
        }
        this->active_clip = iter->second;
        this->playing = true;
        return true;
    }

    void AnimationController::pause(void) {
        this->playing = false;
    }

    void AnimationController::stop(void) {
        this->playing = false;
        this->time = 0.0f;
    }

    void AnimationController::update(const float elapsed_seconds) {
        if (!this->playing || !this->active_clip || elapsed_seconds <= 0.0f) {
            return;
        }
        this->time += elapsed_seconds;
        const float duration = this->active_clip->duration;
        if (duration <= 0.0f) {
            return;
        }
        if (this->active_clip->is_looping) {
            this->time = std::fmod(this->time, duration);
            return;
        }
        this->time = std::min(this->time, duration);
        if (this->time == duration) {
            this->playing = false;
        }
    }

    const AnimationClipData* AnimationController::get_active_clip(void) const noexcept {
        return this->active_clip.get();
    }

    float AnimationController::get_time(void) const noexcept {
        return this->time;
    }

    bool AnimationController::is_playing(void) const noexcept {
        return this->playing;
    }
} // namespace enishi::animation
