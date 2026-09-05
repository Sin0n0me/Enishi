#pragma once
#include <glm/glm.hpp>

namespace enishi::component {
    enum class ShapeType {};

    struct RigidBody {
        glm::mat4 offset; // bulletとのオフセット
    };

    struct PhysicsComponent {};
} // namespace enishi::component
