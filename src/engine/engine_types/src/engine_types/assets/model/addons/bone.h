#pragma once
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>

namespace enishi::types {
    enum class BoneKind : std::uint8_t {
        Bind,
        Animation,
        Physics,
        Skinning,
        Cache,
    };

    using BoneIndex = std::size_t;

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

    struct ModelBone {
        BindBone bind_bone;
        BoneNode bone_node;
    };

    struct BoneTransform {
        glm::vec3 translation;
        glm::quat rotation;
        glm::vec3 scale;
    };

    // アニメーション書き込み用
    struct AnimationBone {
        BoneTransform transform; // 基本こちらを使用
        glm::mat4 global;
    };

    // 物理演算書き込み用
    struct PhysicsBone {
        glm::mat4 local;
        glm::mat4 global;
    };

    // 中間結果書き込み用
    struct CacheBone {
        glm::mat4 local;
        glm::mat4 global;
    };

    // 描画の書き込み用(最終結果なのでローカルはない)
    struct SkinningBone {
        glm::mat4 global;
    };
} // namespace enishi::types