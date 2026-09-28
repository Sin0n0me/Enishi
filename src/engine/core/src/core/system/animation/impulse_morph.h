#pragma once
#include "morph_weights.h"

namespace enishi::core {
    struct MorphImpulse {
        glm::vec3 world_velocity{};
        glm::vec3 world_torque{};
        glm::vec3 local_velocity{};
        glm::vec3 local_torque{};
        bool reset_velocity{};
    };

    [[nodiscard]] foundation::Result<std::vector<MorphImpulse>, MorphError> evaluate_impulse_morphs(
        std::size_t body_count,
        std::span<const types::MorphTarget> targets,
        std::span<const float> weights);
} // namespace enishi::core
