#pragma once
#include "bone.h"
#include <optional>

namespace enishi::types {
    struct BoneConstraint {
        BoneIndex bone{INVALID_BONE_INDEX};
        BoneIndex source{INVALID_BONE_INDEX};
        float rotation_weight{};
        float translation_weight{};
        bool local_space{};
        bool after_physics{};
        std::int32_t evaluation_order{};
        std::optional<glm::vec3> rotation_axis;
        std::optional<glm::mat3> local_axes;
    };

    struct AddonBoneConstraints {
        std::vector<BoneConstraint> constraints;
    };
} // namespace enishi::types
