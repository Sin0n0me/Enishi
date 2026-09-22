#pragma once
#include <engine_types/physics/joint/physics_joint.h>
#include <engine_types/physics/rigid_body/physics_rigid_body.h>
#include <glm/glm.hpp>
#include <vector>

namespace enishi::component {
    struct PhysicsComponent {
        std::vector<glm::mat4> local;
        std::vector<glm::mat4> global;
    };
} // namespace enishi::component
