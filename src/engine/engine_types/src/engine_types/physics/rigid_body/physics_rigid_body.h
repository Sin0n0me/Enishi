#pragma once
#include "rigid_body_kind.h"
#include "rigid_body_shape.h"
#include <cstdint>
#include <glm/glm.hpp>
#include <string>

namespace enishi::types {
    struct PhysicsRigidBody {
        std::string name;                // 剛体の名前
        std::uint16_t relate_bone_index; // 関連ボーン番号
        std::uint8_t group_index;        // 剛体グループ番号
        std::uint16_t group_target;      // 対象剛体グループマスク
        RigidBodyShape shape;            // 衝突形状
        glm::vec3 position;              // 位置(相対座標のオフセット)
        glm::vec3 rotation;              // 回転(Radian)
        float mass;                      // 質量
        float linear_damping;            // 移動減
        float angular_damping;           // 回転減
        float restitution;               // 反発力
        float friction;                  // 摩擦力
        RigidBodyKind kind;              // 剛体タイプ
    };
} // namespace enishi::types
