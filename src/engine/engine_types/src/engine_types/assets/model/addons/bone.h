#pragma once
#include <cstdint>
#include <glm/glm.hpp>
#include <vector>

namespace enishi::types {
    using BoneIndex = std::uint16_t;

    constexpr BoneIndex INVALID_BONE_INDEX = UINT16_MAX;

    struct BindBone {
        glm::mat4 local;          // ローカル
        glm::mat4 global;         // グローバル
        glm::mat4 global_inverse; // スキニング計算に使用

        BindBone(void)
            : local(glm::mat4(1.0f))
            , global(glm::mat4(1.0f))
            , global_inverse(glm::mat4(1.0f)) {
        }
    };

    struct BoneNode {
        BoneIndex parent;
        std::vector<BoneIndex> children;

        BoneNode(void)
            : parent(INVALID_BONE_INDEX) {
        }

        [[nodiscard]] bool has_parent(void) const;
    };

    // 更新用
    struct AnimationBone {
        glm::mat4 local;  // ローカル
        glm::mat4 global; // グローバル(GPUに渡す値)
    };

    struct Bone {
        types::AnimationBone animation_bone;
        types::BindBone bind_bone;
        types::BoneNode bone_node;
    };
} // namespace enishi::types