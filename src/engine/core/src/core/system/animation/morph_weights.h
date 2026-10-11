#pragma once
#include "vertex_morph.h"

namespace enishi::core {
    [[nodiscard]] foundation::Result<std::vector<float>, MorphError> evaluate_morph_weights(
        std::span<const types::MorphTarget> targets, std::span<const float> weights);
}
