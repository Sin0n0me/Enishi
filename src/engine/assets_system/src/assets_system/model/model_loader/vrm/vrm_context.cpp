#include "vrm_context.h"
#include "vrm_schema.h"
#include <cmath>
#include <glm/gtc/type_ptr.hpp>

namespace enishi::assets_system {
    bool VRMContext::fail(std::string message) {
        this->error = std::move(message);
        return false;
    }

    VRMJson vrm_extension(
        const cgltf_extension* extensions, std::size_t count, std::string_view name) {
        for (std::size_t index = 0; index < count; ++index) {
            if (extensions[index].name != nullptr && extensions[index].name == name) {
                return VRMJson::parse(extensions[index].data);
            }
        }
        return {};
    }

    bool make_vrm_bones(VRMContext& context) {
        types::AddonBones bones(context.source.nodes_count);
        const auto inverse_basis = glm::inverse(context.basis);
        for (std::size_t index = 0; index < bones.size(); ++index) {
            const auto& node = context.source.nodes[index];
            auto& bone = bones[index];
            bone.name = node.name != nullptr ? node.name : "node_" + std::to_string(index);
            if (node.parent != nullptr) {
                bone.bone_node.parent =
                    static_cast<std::size_t>(node.parent - context.source.nodes);
            }
            for (std::size_t child = 0; child < node.children_count; ++child) {
                bone.bone_node.children.push_back(
                    static_cast<std::size_t>(node.children[child] - context.source.nodes));
            }
            cgltf_node_transform_local(&node, glm::value_ptr(bone.bind_bone.local));
            cgltf_node_transform_world(&node, glm::value_ptr(bone.bind_bone.global));
            // A change of basis preserves hierarchy: M_engine = C * M_gltf * inverse(C).
            bone.bind_bone.local = context.basis * bone.bind_bone.local * inverse_basis;
            bone.bind_bone.global = context.basis * bone.bind_bone.global * inverse_basis;
            constexpr glm::length_t MATRIX_DIMENSION = 4;
            for (glm::length_t column = 0; column < MATRIX_DIMENSION; ++column) {
                for (glm::length_t row = 0; row < MATRIX_DIMENSION; ++row) {
                    if (!std::isfinite(bone.bind_bone.local[column][row]) ||
                        !std::isfinite(bone.bind_bone.global[column][row])) {
                        return context.fail("Non-finite VRM node transform");
                    }
                }
            }
            const auto determinant = glm::determinant(bone.bind_bone.global);
            if (!std::isfinite(determinant) || determinant == 0) {
                return context.fail("VRM contains a singular or non-finite node transform");
            }
            bone.bind_bone.global_inverse = glm::inverse(bone.bind_bone.global);
        }
        const auto assign = [&](const std::string& name, const VRMJson& value) {
            const auto index = value[vrm_schema::key::NODE].index();
            if (!(index < bones.size())) {
                return false;
            }
            bones[index].name = name;
            return true;
        };
        const auto& humanoid =
            context.extension[vrm_schema::key::HUMANOID][vrm_schema::key::HUMAN_BONES];
        if (context.legacy) {
            for (const auto& bone : humanoid.elements) {
                if (!assign(bone[vrm_schema::key::BONE].value, bone)) {
                    return context.fail("Invalid VRM humanoid node");
                }
            }
        } else {
            for (const auto& [name, bone] : humanoid.members) {
                if (!assign(name, bone)) {
                    return context.fail("Invalid VRM humanoid node");
                }
            }
        }
        context.model.addons.emplace_back(std::move(bones));
        return true;
    }
} // namespace enishi::assets_system
