#pragma once
#include <glm/glm.hpp>

namespace enishi::types {
    struct RigidBodyImpulse {
        glm::vec3 world_velocity{};
        glm::vec3 world_torque{};
        glm::vec3 local_velocity{};
        glm::vec3 local_torque{};
        bool reset_velocity{};
    };
} // namespace enishi::types
