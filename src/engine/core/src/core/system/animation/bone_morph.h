#pragma once
#include "morph_weights.h"

namespace enishi::core {
    struct BoneMorphDelta {
        glm::vec3 translation{};
        glm::quat rotation{1, 0, 0, 0};
    };

    [[nodiscard]] foundation::Result<std::vector<BoneMorphDelta>, MorphError> evaluate_bone_morphs(
        std::size_t bone_count,
        std::span<const types::MorphTarget> targets,
        std::span<const float> weights);
} // namespace enishi::core
