#pragma once
#include <cstdint>
#include <glm/glm.hpp>
#include <string>

namespace enishi::types {
    enum class JointKind { SpringSixDof, SixDof, PointToPoint, ConeTwist, Slider, Hinge };

    struct JointMotor {
        bool enabled{};
        float velocity{};
        float max_force{};
    };

    struct PhysicsJoint {
        std::string name;                 // 名前
        std::uint32_t rigid_body_a;       // 接続先剛体Ａ
        std::uint32_t rigid_body_b;       // 接続先剛体Ｂ
        glm::vec3 position;               // 位置
        glm::vec3 rotation;               // 回転(Radian)
        glm::vec3 constrain_position_min; // 移動制限-下限
        glm::vec3 constrain_position_max; // 移動制限-上限
        glm::vec3 constrain_rotation_min; // 回転制限-下限
        glm::vec3 constrain_rotation_max; // 回転制限-上限
        glm::vec3 spring_position;        // ばね移動値
        glm::vec3 spring_rotation;        // ばね回転値
        JointKind kind{JointKind::SpringSixDof};
        float softness{}, bias{}, relaxation{}, damping{}, fix_threshold{};
        JointMotor linear_motor, angular_motor;
        glm::vec3 motor_target_rotation{};
        glm::vec3 angular_span{}; // Cone/twist angular ranges around X, Y and Z.
    };
} // namespace enishi::types
