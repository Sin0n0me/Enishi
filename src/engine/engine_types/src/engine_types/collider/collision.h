#pragma once
#include <cstdint>
#include <engine_types/assets/model/addons/bone.h>

namespace enishi::types {
    // Identifies a model instance, not its shared mesh/asset.
    using CollisionModelId = std::uint64_t;

    struct OBBContact {
        glm::vec3 position; // World-space representative point inside the OBB intersection.
        glm::vec3 normal;   // From the receiving model toward the other model.
        float penetration_depth;
    };

    struct Collision {
        CollisionModelId other_model;
        BoneIndex other_bone;
        OBBContact contact;
    };
} // namespace enishi::types
