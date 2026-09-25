#pragma once
#include <engine_types/assets/model/addons/bone.h>
#include <engine_types/assets/model/addons/ik.h>
#include <engine_types/handle/renderer/render_handle.h>
#include <vector>

namespace enishi::component {
    struct ModelComponent {
        types::RenderHandle render_handle;
        std::vector<types::BoneNode> bone_node; // 接続先などの情報
        std::vector<types::BindBone> bind_bone; // バインドボーン
        // Evaluation order references the original bone indices; storage is never reordered.
        std::vector<types::BoneIndex> evaluation_order;
        std::vector<types::IK> iks;
    };
} // namespace enishi::component
