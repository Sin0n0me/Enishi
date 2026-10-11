#pragma once
#include "morph_weights.h"
#include <engine_types/physics/rigid_body/rigid_body_impulse.h>

namespace enishi::core {
    using MorphImpulse = types::RigidBodyImpulse;

    [[nodiscard]] foundation::Result<std::vector<MorphImpulse>, MorphError> evaluate_impulse_morphs(
        std::size_t body_count,
        std::span<const types::MorphTarget> targets,
        std::span<const float> weights);
} // namespace enishi::core
