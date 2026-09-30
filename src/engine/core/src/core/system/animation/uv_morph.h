#pragma once
#include "morph_weights.h"

namespace enishi::core {
    using UVChannels = std::vector<std::vector<glm::vec4>>;
    [[nodiscard]] foundation::Result<UVChannels, MorphError> evaluate_uv_morphs(
        const UVChannels& base,
        std::span<const types::MorphTarget> targets,
        std::span<const float> weights);
} // namespace enishi::core
