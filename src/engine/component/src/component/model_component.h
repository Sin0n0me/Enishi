#pragma once
#include <engine_types/assets/model/addons/bone.h>
#include <engine_types/assets/model/addons/bone_constraints.h>
#include <engine_types/assets/model/addons/ik.h>
#include <engine_types/assets/model/addons/morph_target.h>
#include <engine_types/assets/model/material/material.h>
#include <engine_types/handle/renderer/render_handle.h>
#include <unordered_map>
#include <vector>

namespace enishi::component {
    struct ModelComponent {
        types::RenderHandle render_handle;
        std::vector<types::BoneNode> bone_node; // 接続先などの情報
        std::vector<types::BindBone> bind_bone; // バインドボーン
        std::vector<std::string> bone_names;
        types::AddonBoneConstraints bone_constraints;
        // Unbound slots use identity. Values are model-space transforms applied after parenting.
        std::unordered_map<std::int32_t, glm::mat4> external_transforms;
        // Evaluation order references the original bone indices; storage is never reordered.
        std::vector<types::BoneIndex> evaluation_order;
        std::vector<types::IK> iks;
        std::vector<glm::vec3> morph_base_positions;
        std::vector<types::Material> morph_base_materials;
        std::vector<std::vector<glm::vec4>> morph_base_uvs;
        types::AddonMorphTargets morph_targets;
    };
} // namespace enishi::component
