#pragma once
#include <glm/glm.hpp>
#include <optional>

namespace enishi::types {
    struct RigidBodyImpulse {
        glm::vec3 world_velocity{};
        glm::vec3 world_torque{};
        glm::vec3 local_velocity{};
        glm::vec3 local_torque{};
        bool reset_velocity{};
        // Model/world coordinates, before the physics backend's Z reflection.
        std::optional<glm::vec3> world_position;
    };
} // namespace enishi::types
