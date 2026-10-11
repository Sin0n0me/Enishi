#include "model_component_builder.h"
#include <algorithm>
#include <numeric>

namespace enishi::model_controller {
    namespace {
        void collect_collision_vertices(
            component::ModelComponent& model, const types::ModelData& data) {
            model.collision_vertices.resize(model.bind_bone.size());
            for (const auto& attributes : data.vertices) {
                glm::vec3 position(0);
                bool has_position = false;
                glm::u32vec4 indices(UINT32_MAX);
                glm::vec4 weights(0);
                for (const auto& attribute : attributes) {
                    if (const auto* vertex = std::get_if<types::Vertex>(&attribute);
                        vertex != nullptr) {
                        position = vertex->position;
                        has_position = true;
                    } else if (const auto* vertex = std::get_if<types::VertexPosition>(&attribute);
                               vertex != nullptr) {
                        position = vertex->position;
                        has_position = true;
                    } else if (const auto* skin = std::get_if<types::Skinning>(&attribute);
                               skin != nullptr) {
                        indices = glm::u32vec4(skin->bone_index, 0, 0);
                        weights = glm::vec4(skin->bone_weight, 0, 0);
                    } else if (const auto* skin = std::get_if<types::Skinning4>(&attribute);
                               skin != nullptr) {
                        indices = skin->bone_index;
                        weights = skin->bone_weight;
                    }
                }
                if (!has_position) {
                    continue;
                }
                for (glm::length_t influence = 0; influence < indices.length(); ++influence) {
                    const auto bone = indices[influence];
                    if (!(weights[influence] > 0) || !(bone < model.bind_bone.size())) {
                        continue;
                    }
                    // p_bone = inverse(bind_global) * p_model; animated_global restores world
                    // space.
                    model.collision_vertices[bone].push_back(
                        glm::vec3(model.bind_bone[bone].global_inverse * glm::vec4(position, 1)));
                }
            }
        }
    } // namespace

    component::ModelComponent make_model_component(
        const types::ModelData& data, types::RenderHandle handle) {
        component::ModelComponent model;
        model.name = data.name;
        const auto source_path = data.path.generic_u8string();
        model.source_path.assign(source_path.begin(), source_path.end());
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
            } else if (const auto* bodies = std::get_if<types::AddonRigidBodies>(&addon);
                       bodies != nullptr) {
                model.rigid_bodies = *bodies;
            } else if (const auto* joints = std::get_if<types::AddonPhysicsJoints>(&addon);
                       joints != nullptr) {
                model.physics_joints = *joints;
            }
        }
        collect_collision_vertices(model, data);
        if (!model.morph_targets.targets.empty()) {
            model.morph_base_materials = data.materials;
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
        std::vector<bool> after_physics(model.bone_node.size());
        for (const auto& addon : data.addons) {
            if (const auto* constraints = std::get_if<types::AddonBoneConstraints>(&addon);
                constraints != nullptr) {
                model.bone_constraints = *constraints;
                for (const auto& constraint : constraints->constraints) {
                    if (constraint.bone < priorities.size()) {
                        priorities[constraint.bone] = constraint.evaluation_order;
                        after_physics[constraint.bone] = constraint.after_physics;
                    }
                }
            }
        }
        model.evaluation_order.resize(model.bone_node.size());
        std::iota(model.evaluation_order.begin(), model.evaluation_order.end(), types::BoneIndex{});
        std::stable_sort(model.evaluation_order.begin(),
            model.evaluation_order.end(),
            [&priorities, &after_physics](auto a, auto b) {
                if (after_physics[a] != after_physics[b]) {
                    return !after_physics[a];
                }
                return priorities[a] < priorities[b];
            });
        return model;
    }
} // namespace enishi::model_controller
