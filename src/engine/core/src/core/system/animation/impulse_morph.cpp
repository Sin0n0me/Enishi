#include "impulse_morph.h"
#include <cmath>

namespace enishi::core {
    namespace {
        bool finite(glm::vec3 value) {
            return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
        }
    } // namespace

    foundation::Result<std::vector<MorphImpulse>, MorphError> evaluate_impulse_morphs(
        std::size_t body_count,
        std::span<const types::MorphTarget> targets,
        std::span<const float> weights) {
        const auto effective = evaluate_morph_weights(targets, weights);
        if (effective.is_err()) {
            return effective.propagation(MorphError::InvalidWeight);
        }
        std::vector<MorphImpulse> result(body_count);
        for (std::size_t index = 0; index < targets.size(); ++index) {
            const auto weight = effective.unwrap()[index];
            if (weight == 0.0f) {
                continue;
            }
            for (const auto& variant : targets[index].offsets) {
                const auto* offset = std::get_if<types::ImpulseMorphOffset>(&variant);
                if (offset == nullptr) {
                    continue;
                }
                if (!(offset->rigid_body < body_count)) {
                    return foundation::Error(
                        MorphError::InvalidReference, "Impulse morph body index is out of range");
                }
                auto& impulse = result[offset->rigid_body];
                impulse.reset_velocity = impulse.reset_velocity || offset->reset_velocity;
                if (offset->local_space) {
                    impulse.local_velocity += offset->velocity * weight;
                    impulse.local_torque += offset->torque * weight;
                } else {
                    impulse.world_velocity += offset->velocity * weight;
                    impulse.world_torque += offset->torque * weight;
                }
                if (!finite(impulse.local_velocity) || !finite(impulse.local_torque) ||
                    !finite(impulse.world_velocity) || !finite(impulse.world_torque)) {
                    return foundation::Error(
                        MorphError::InvalidWeight, "Non-finite impulse morph result");
                }
            }
        }
        return result;
    }
} // namespace enishi::core
