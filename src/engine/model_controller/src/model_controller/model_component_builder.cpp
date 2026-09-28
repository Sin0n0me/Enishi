#include "model_component_builder.h"
#include <algorithm>
#include <numeric>

namespace enishi::model_controller {
    component::ModelComponent make_model_component(
        const types::ModelData& data, types::RenderHandle handle) {
        component::ModelComponent model;
        model.render_handle = handle;
        for (const auto& addon : data.addons) {
            if (const auto* bones = std::get_if<types::AddonBones>(&addon); bones != nullptr) {
                for (const auto& bone : *bones) {
                    model.bone_node.push_back(bone.bone_node);
                    model.bind_bone.push_back(bone.bind_bone);
                    model.bone_names.push_back(bone.name);
                }
            } else if (const auto* iks = std::get_if<types::AddonIKs>(&addon); iks != nullptr) {
                model.iks = *iks;
            } else if (const auto* morphs = std::get_if<types::AddonMorphTargets>(&addon);
                       morphs != nullptr) {
                model.morph_targets = *morphs;
            }
        }
        if (!model.morph_targets.targets.empty()) {
            model.morph_base_positions.resize(data.vertices.size());
            model.morph_base_uvs.emplace_back(data.vertices.size(), glm::vec4(0));
            model.morph_base_uvs.insert(model.morph_base_uvs.end(),
                data.additional_uv_channels.begin(),
                data.additional_uv_channels.end());
            for (std::size_t index = 0; index < data.vertices.size(); ++index) {
                for (const auto& attribute : data.vertices[index]) {
                    if (const auto* vertex = std::get_if<types::Vertex>(&attribute);
                        vertex != nullptr) {
                        model.morph_base_positions[index] = vertex->position;
                        model.morph_base_uvs.front()[index] = glm::vec4(vertex->uv, 0, 0);
                    } else if (const auto* position =
                                   std::get_if<types::VertexPosition>(&attribute);
                               position != nullptr) {
                        model.morph_base_positions[index] = position->position;
                    }
                }
            }
        }
        std::vector<std::int32_t> priorities(model.bone_node.size());
        for (const auto& addon : data.addons) {
            if (const auto* constraints = std::get_if<types::AddonBoneConstraints>(&addon);
                constraints != nullptr) {
                model.bone_constraints = *constraints;
                for (const auto& constraint : constraints->constraints) {
                    if (constraint.bone < priorities.size()) {
                        priorities[constraint.bone] = constraint.evaluation_order;
                    }
                }
            }
        }
        model.evaluation_order.resize(model.bone_node.size());
        std::iota(model.evaluation_order.begin(), model.evaluation_order.end(), types::BoneIndex{});
        std::stable_sort(model.evaluation_order.begin(),
            model.evaluation_order.end(),
            [&priorities](auto a, auto b) { return priorities[a] < priorities[b]; });
        return model;
    }
} // namespace enishi::model_controller
