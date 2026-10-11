#pragma once
#include "morph_weights.h"
#include <engine_types/renderer/uniform_buffer/material.h>

namespace enishi::core {
    [[nodiscard]] foundation::Result<std::vector<types::UniformMaterial>, MorphError>
    evaluate_material_morphs(std::span<const types::Material> materials,
        std::span<const types::MorphTarget> targets,
        std::span<const float> weights);
}
