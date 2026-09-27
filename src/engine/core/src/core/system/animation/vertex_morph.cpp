#include "vertex_morph.h"
#include <cmath>

namespace enishi::core {
    foundation::Result<std::vector<glm::vec3>, MorphError> evaluate_vertex_morphs(
        std::span<const glm::vec3> base_positions,
        std::span<const types::MorphTarget> targets,
        std::span<const float> weights) {
        if (targets.size() != weights.size()) {
            return foundation::Error(
                MorphError::InvalidWeight, "Morph weight count differs from target count");
        }
        // Rebuild from bind-space positions so weight changes never accumulate across frames.
        std::vector<glm::vec3> positions(base_positions.begin(), base_positions.end());
        for (std::size_t index = 0; index < targets.size(); ++index) {
            const auto weight = weights[index];
            if (!std::isfinite(weight)) {
                return foundation::Error(MorphError::InvalidWeight, "Non-finite morph weight");
            }
            if (weight == 0.0f) {
                continue;
            }
            for (const auto& offset : targets[index].offsets) {
                const auto* vertex = std::get_if<types::VertexMorphOffset>(&offset);
                if (vertex == nullptr) {
                    return foundation::Error(
                        MorphError::UnsupportedOffset, "Expected a vertex morph offset");
                }
                if (!(vertex->vertex < positions.size())) {
                    return foundation::Error(
                        MorphError::InvalidVertex, "Morph vertex is out of range");
                }
                positions[vertex->vertex] += vertex->translation * weight;
            }
        }
        return positions;
    }
} // namespace enishi::core
