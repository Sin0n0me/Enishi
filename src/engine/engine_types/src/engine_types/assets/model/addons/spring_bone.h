#pragma once
#include "bone.h"

namespace enishi::types {
    struct SpringCollider {
        BoneIndex bone{INVALID_BONE_INDEX};
        glm::vec3 offset{};
        glm::vec3 tail{};
        float radius{};
        bool capsule{};
    };

    struct SpringJoint {
        BoneIndex bone{INVALID_BONE_INDEX};
        glm::vec3 local_tail{};
        float stiffness{1};
        float drag{0.4f};
        float radius{};
        glm::vec3 gravity{};
    };

    struct SpringChain {
        BoneIndex center{INVALID_BONE_INDEX};
        std::vector<SpringJoint> joints;
        std::vector<std::size_t> colliders;
    };

    struct AddonSpringBones {
        std::vector<SpringCollider> colliders;
        std::vector<SpringChain> chains;
    };
} // namespace enishi::types
