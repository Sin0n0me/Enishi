#include "bone_morph.h"
#include <cmath>

namespace enishi::core {
    foundation::Result<std::vector<BoneMorphDelta>, MorphError> evaluate_bone_morphs(
        std::size_t bone_count,
        std::span<const types::MorphTarget> targets,
        std::span<const float> weights) {
        const auto effective = evaluate_morph_weights(targets, weights);
        if (effective.is_err()) {
            return effective.propagation(MorphError::InvalidWeight);
        }
        std::vector<BoneMorphDelta> pose(bone_count);
        for (std::size_t index = 0; index < targets.size(); ++index) {
            const auto weight = effective.unwrap()[index];
            if (weight == 0.0f) {
                continue;
            }
            for (const auto& offset : targets[index].offsets) {
                const auto* bone = std::get_if<types::BoneMorphOffset>(&offset);
                if (bone == nullptr) {
                    continue;
                }
                const auto length = glm::length(bone->rotation);
                if (!(bone->bone < pose.size()) || !std::isfinite(length) || !(length > 0.0f)) {
                    return foundation::Error(
                        MorphError::InvalidReference, "Invalid bone morph reference or rotation");
                }
                auto& delta = pose[bone->bone];
                delta.translation += bone->translation * weight;
                delta.rotation = glm::normalize(
                    delta.rotation *
                    glm::slerp(glm::quat(1, 0, 0, 0), bone->rotation / length, weight));
                for (glm::length_t axis = 0; axis < delta.translation.length(); ++axis) {
                    if (!std::isfinite(delta.translation[axis])) {
                        return foundation::Error(
                            MorphError::InvalidWeight, "Non-finite bone morph translation");
                    }
                }
                if (!std::isfinite(glm::length(delta.rotation))) {
                    return foundation::Error(
                        MorphError::InvalidWeight, "Non-finite bone morph rotation");
                }
            }
        }
        return pose;
    }
} // namespace enishi::core
