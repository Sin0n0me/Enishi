#pragma once
#include <cstdint>
#include <glm/glm.hpp>
#include <variant>
#include <vector>

namespace enishi::types {
    enum class RigidBodyType : std::uint8_t {
        Kinematic = 0,         // ボーン追従
        Dynamic = 1,           //
        DynamicAdjustBone = 2, //
    };

    struct ShapeSphere {
        float radius;
    };

    struct ShapeBox {
        float width;
        float height;
        float depth;
    };

    struct ShapeCapsule {
        float radius;
        float height;
    };

    using RigidBodyShape = std::variant<ShapeSphere, ShapeBox, ShapeCapsule>;

    struct RigidBody {
        std::uint16_t group;
        std::uint16_t group_mask;
        RigidBodyType rigid_body_type; // 剛体のタイプ
        RigidBodyShape shape;          // 形状
        glm::mat4 offset;              // ボーンとのオフセット
    };
} // namespace enishi::types