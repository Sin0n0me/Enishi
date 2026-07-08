#pragma once
#include <cstdint>
#include <glm/glm.hpp>
#include <variant>
#include <vector>

namespace enishi::types {
    enum class RigidBodyType : std::uint8_t {
        Kinematic = 0,         // ボーン追従
        Dynamic = 1,           // 物理演算
        DynamicAdjustBone = 2, // 物理演算(Bone位置合わせ)
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

    using RigidBodyShape = std::variant<std::monostate, ShapeSphere, ShapeBox, ShapeCapsule>;

    struct RigidBody {
        std::uint16_t relate_bone_index; // 関連ボーン番号
        std::uint16_t group;             // 剛体グループ番号
        std::uint16_t group_mask;        // 対象剛体グループマスク
        RigidBodyType rigid_body_type;   // 剛体のタイプ
        RigidBodyShape shape;            // 形状
        glm::mat4 offset;                // ボーンとのオフセット
        float mass;                      // 質量
        glm::vec3 position;              // 位置(相対座標のオフセット)
        glm::vec3 rotation;              // 回転(Radian)
        float linear_damping;            // 移動減
        float angular_damping;           // 回転減
        float restitution;               // 反発力
        float friction;                  // 摩擦力
    };
} // namespace enishi::types