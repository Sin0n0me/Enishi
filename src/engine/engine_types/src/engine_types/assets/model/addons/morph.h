#pragma once
#include "bone.h"
#include <cstdint>
#include <glm/glm.hpp>
#include <vector>

namespace enishi::types {
    using MorphIndex = std::uint16_t;

    constexpr MorphIndex INVALID_MORPH_INDEX = UINT16_MAX;

    struct MorphVertex {
        std::uint64_t index; // ベースの配列インデックス
        glm::vec3 offset;    // インデックスと対応する要素とのオフセット
    };

    struct AddonMorphs {
        std::vector<MorphVertex> base_vertices;
        std::vector<std::vector<MorphVertex>> vertices; // このモーフが動かす頂点群
    };
} // namespace enishi::types