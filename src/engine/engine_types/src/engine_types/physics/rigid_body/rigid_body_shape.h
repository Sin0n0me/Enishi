#pragma once
#include <cstdint>
#include <glm/glm.hpp>
#include <variant>

namespace enishi::types {
    struct RBShapeBox {
        float width;
        float height;
        float depth;
    };

    struct RBShapeSphere {
        float radius;
    };

    struct RBShapeCapsule {
        float raius;
        float height;
    };

    using RigidBodyShape = std::variant<RBShapeBox, RBShapeSphere, RBShapeCapsule>;
} // namespace enishi::types
