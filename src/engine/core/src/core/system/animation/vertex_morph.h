#pragma once
#include <engine_types/assets/model/addons/morph_target.h>
#include <foundation/result/result.h>
#include <span>

namespace enishi::core {
    enum class MorphError {
        InvalidWeight,
        InvalidVertex,
        UnsupportedOffset,
        InvalidReference,
        CyclicReference
    };

    [[nodiscard]] foundation::Result<std::vector<glm::vec3>, MorphError> evaluate_vertex_morphs(
        std::span<const glm::vec3> base_positions,
        std::span<const types::MorphTarget> targets,
        std::span<const float> weights);
} // namespace enishi::core
