#pragma once
#include <cstdint>
#include <glm/glm.hpp>
#include <vector>

namespace enishi::types {
    using BoneIndex = std::uint16_t;

    constexpr BoneIndex INVALID_BONE_INDEX = UINT16_MAX;

    struct BindBone {
        glm::mat4 local;          // ローカル
        glm::mat4 global;         // グローバル(GPUに渡す値)
        glm::mat4 global_inverse; // スキニング計算に使用
    };

    struct BoneNode {
        BoneIndex parent;
        std::vector<BoneIndex> children;

        [[nodiscard]] bool has_parent(void) const;
    };

    // ボーンを持つモデルのボーンデータは最終的にこの構造体へ共通化
    struct Bone {
        BindBone bind_bone;
        BoneNode bone_node;
    };
} // namespace enishi::types