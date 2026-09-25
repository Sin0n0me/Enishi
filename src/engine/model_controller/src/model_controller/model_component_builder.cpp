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
                }
            } else if (const auto* iks = std::get_if<types::AddonIKs>(&addon); iks != nullptr) {
                model.iks = *iks;
            }
        }
        std::vector<std::int32_t> priorities(model.bone_node.size());
        for (const auto& addon : data.addons) {
            if (const auto* constraints = std::get_if<types::AddonBoneConstraints>(&addon);
                constraints != nullptr) {
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
