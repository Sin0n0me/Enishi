#pragma once
#include <engine_types/assets/model/addons/bone.h>
#include <engine_types/handle/handle_type.h>
#include <glm/glm.hpp>

namespace enishi::component {
    // 基本的に読み取りのみ
    struct ModelComponent {
        std::vector<types::BoneNode> bone_node;       // 接続先などの情報
        std::vector<glm::mat4> bind_matrices;         // バインドボーン
        std::vector<glm::mat4> inverse_bind_matrices; // 逆行列に変換済みのバインドボーン
    };
} // namespace enishi::component
