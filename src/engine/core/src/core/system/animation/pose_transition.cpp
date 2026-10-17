#include "pose_transition.h"
#include <algorithm>
#include <cassert>
#include <cmath>

namespace enishi::core {
    void PoseTransition::begin(std::span<const component::AnimationBuffer> from,
        std::span<const component::AnimationBuffer> to) {
        assert(from.size() == to.size());
        this->source.assign(from.begin(), from.end());
        this->destination.assign(to.begin(), to.end());
        this->elapsed = 0;
        this->running = true;
    }

    void PoseTransition::apply(float seconds, std::vector<component::AnimationBuffer>& output) {
        if (!this->running) {
            return;
        }
        if (std::isfinite(seconds) && seconds > 0) {
            this->elapsed = (std::min)(DURATION, this->elapsed + static_cast<double>(seconds));
        }
        // Linear pose interpolation: p(t)=(1-a)*p0+a*p1, a=clamp(t/0.5,0,1).
        const float weight = static_cast<float>(this->elapsed / DURATION);
        output.resize(this->destination.size());
        for (std::size_t bone = 0; bone < output.size(); ++bone) {
            const auto& from = this->source[bone];
            const auto& to = this->destination[bone];
            auto& result = output[bone];
            result.position = glm::mix(from.position, to.position, weight);
            result.scale = glm::mix(from.scale, to.scale, weight);
            // q and -q represent the same rotation. Shortest-hemisphere normalized lerp
            // avoids both the long arc and the non-unit rotations of raw component lerp.
            const auto end = glm::dot(from.rotation, to.rotation) < 0 ? -to.rotation : to.rotation;
            result.rotation = glm::normalize(from.rotation * (1 - weight) + end * weight);
        }
        if (!(this->elapsed < DURATION)) {
            output = this->destination;
            this->running = false;
        }
    }

    bool PoseTransition::active() const {
        return this->running;
    }

    void PoseTransition::cancel() {
        this->running = false;
    }
} // namespace enishi::core
