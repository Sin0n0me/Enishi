#include "vrm_mesh.h"
#include "vrm_material.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <glm/gtc/type_ptr.hpp>
#include <limits>

namespace enishi::assets_system {
    namespace {
        constexpr std::size_t POSITION_COMPONENTS = 3;
        constexpr std::size_t UV_COMPONENTS = 2;
        constexpr std::size_t INFLUENCES = 4;
        constexpr std::size_t TRIANGLE_INDICES = 3;

        const cgltf_accessor* attribute(
            const cgltf_attribute* attributes, std::size_t count, cgltf_attribute_type type) {
            for (std::size_t index = 0; index < count; ++index) {
                if (attributes[index].type == type && attributes[index].index == 0) {
                    return attributes[index].data;
                }
            }
            return nullptr;
        }

        bool unpack(const cgltf_accessor* accessor,
            std::size_t count,
            std::size_t components,
            std::vector<float>& values,
            float fallback = 0) {
            values.assign(count * components, fallback);
            if (accessor == nullptr) {
                return true;
            }
            if (accessor->count != count || cgltf_num_components(accessor->type) != components ||
                cgltf_accessor_unpack_floats(accessor, values.data(), values.size()) !=
                    values.size()) {
                return false;
            }
            return std::ranges::all_of(values, [](float value) { return std::isfinite(value); });
        }

        bool make_skin(VRMContext& context, const cgltf_skin& skin, std::vector<bool>& assigned) {
            auto& bones = std::get<types::AddonBones>(context.model.addons.front());
            if (skin.inverse_bind_matrices != nullptr &&
                (skin.inverse_bind_matrices->count != skin.joints_count ||
                    skin.inverse_bind_matrices->type != cgltf_type_mat4)) {
                return context.fail("Invalid VRM inverse bind matrices");
            }
            for (std::size_t joint = 0; joint < skin.joints_count; ++joint) {
                glm::mat4 inverse(1);
                if (skin.inverse_bind_matrices != nullptr &&
                    cgltf_accessor_read_float(
                        skin.inverse_bind_matrices, joint, glm::value_ptr(inverse), 16) == 0) {
                    return context.fail("Unreadable VRM inverse bind matrix");
                }
                // Skinned glTF vertices are in skin bind space; C * IBM * inverse(C)
                // makes the engine's animated_global * inverse_bind product equivalent.
                inverse = context.basis * inverse * glm::inverse(context.basis);
                const auto index =
                    static_cast<std::size_t>(skin.joints[joint] - context.source.nodes);
                const auto previous = bones[index].bind_bone.global_inverse;
                for (glm::length_t column = 0; column < 4; ++column) {
                    for (glm::length_t row = 0; row < 4; ++row) {
                        constexpr float MATRIX_TOLERANCE = 0.0001f;
                        if (!std::isfinite(inverse[column][row]) ||
                            (assigned[index] &&
                                std::abs(previous[column][row] - inverse[column][row]) >
                                    MATRIX_TOLERANCE)) {
                            return context.fail(
                                "Incompatible VRM inverse bind matrices for a shared joint");
                        }
                    }
                }
                bones[index].bind_bone.global_inverse = inverse;
                assigned[index] = true;
            }
            return true;
        }

        bool make_indices(VRMContext& context,
            const cgltf_primitive& primitive,
            std::size_t count,
            std::uint32_t base) {
            if (primitive.type != cgltf_primitive_type_triangles) {
                return context.fail("VRM primitive must contain triangles");
            }
            const auto index_count =
                primitive.indices != nullptr ? primitive.indices->count : count;
            auto& indices = std::get<std::vector<std::uint32_t>>(context.model.indices);
            if (index_count % TRIANGLE_INDICES != 0 ||
                index_count > std::numeric_limits<std::uint32_t>::max() - indices.size()) {
                return context.fail("Invalid VRM triangle count");
            }
            for (std::size_t index = 0; index < index_count; ++index) {
                const auto vertex = primitive.indices != nullptr
                                        ? cgltf_accessor_read_index(primitive.indices, index)
                                        : index;
                if (!(vertex < count)) {
                    return context.fail("VRM index is outside the vertex buffer");
                }
                indices.push_back(base + static_cast<std::uint32_t>(vertex));
            }
            return true;
        }

        void generate_normals(const cgltf_primitive& primitive,
            const std::vector<float>& positions,
            std::vector<float>& normals) {
            const auto count = primitive.indices != nullptr
                                   ? primitive.indices->count
                                   : positions.size() / POSITION_COMPONENTS;
            for (std::size_t first = 0; first < count; first += TRIANGLE_INDICES) {
                std::array<std::size_t, TRIANGLE_INDICES> vertices;
                for (std::size_t corner = 0; corner < TRIANGLE_INDICES; ++corner) {
                    vertices[corner] =
                        primitive.indices != nullptr
                            ? cgltf_accessor_read_index(primitive.indices, first + corner)
                            : first + corner;
                }
                const auto a = glm::make_vec3(&positions[vertices[0] * POSITION_COMPONENTS]);
                const auto b = glm::make_vec3(&positions[vertices[1] * POSITION_COMPONENTS]);
                const auto c = glm::make_vec3(&positions[vertices[2] * POSITION_COMPONENTS]);
                // Area-weighted normal = sum(cross(edge_1, edge_2)) over adjacent faces.
                const auto face = glm::cross(b - a, c - a);
                for (const auto vertex : vertices) {
                    for (std::size_t component = 0; component < POSITION_COMPONENTS; ++component) {
                        normals[vertex * POSITION_COMPONENTS + component] +=
                            face[static_cast<int>(component)];
                    }
                }
            }
        }

        glm::vec2 transform_uv(const cgltf_primitive& primitive, glm::vec2 uv) {
            if (primitive.material == nullptr) {
                return uv;
            }
            const auto& texture = primitive.material->pbr_metallic_roughness.base_color_texture;
            if (texture.has_transform == 0) {
                return uv;
            }
            const auto& t = texture.transform;
            const auto scaled = uv * glm::make_vec2(t.scale);
            // KHR_texture_transform: uv' = offset + R(rotation) * scale * uv.
            return glm::make_vec2(t.offset) +
                   glm::vec2(std::cos(t.rotation) * scaled.x - std::sin(t.rotation) * scaled.y,
                       std::sin(t.rotation) * scaled.x + std::cos(t.rotation) * scaled.y);
        }

        bool make_vertices(VRMContext& context,
            const cgltf_node& node,
            const cgltf_primitive& primitive,
            const glm::mat4& transform,
            std::size_t count) {
            std::vector<float> positions;
            std::vector<float> normals;
            std::vector<float> uvs;
            std::vector<float> joints;
            std::vector<float> weights;
            const auto get = [&](cgltf_attribute_type type) {
                return attribute(primitive.attributes, primitive.attributes_count, type);
            };
            if (!unpack(
                    get(cgltf_attribute_type_position), count, POSITION_COMPONENTS, positions) ||
                !unpack(get(cgltf_attribute_type_normal), count, POSITION_COMPONENTS, normals) ||
                !unpack(get(cgltf_attribute_type_texcoord), count, UV_COMPONENTS, uvs) ||
                !unpack(get(cgltf_attribute_type_joints), count, INFLUENCES, joints) ||
                !unpack(get(cgltf_attribute_type_weights), count, INFLUENCES, weights)) {
                return context.fail("Invalid VRM vertex attribute");
            }
            if (node.skin != nullptr && (get(cgltf_attribute_type_joints) == nullptr ||
                                            get(cgltf_attribute_type_weights) == nullptr)) {
                return context.fail("VRM skin requires joint indices and weights");
            }
            const auto normal_matrix = glm::transpose(glm::inverse(glm::mat3(transform)));
            if (get(cgltf_attribute_type_normal) == nullptr) {
                generate_normals(primitive, positions, normals);
            }
            for (std::size_t index = 0; index < count; ++index) {
                // Positions use the affine transform; normals use its inverse transpose.
                const auto position = glm::vec3(
                    transform *
                    glm::vec4(glm::make_vec3(&positions[index * POSITION_COMPONENTS]), 1));
                auto normal = normal_matrix * glm::make_vec3(&normals[index * POSITION_COMPONENTS]);
                if (glm::dot(normal, normal) > 0) {
                    normal = glm::normalize(normal);
                } else {
                    normal = glm::vec3(0, 1, 0);
                }
                auto uv = glm::make_vec2(&uvs[index * UV_COMPONENTS]);
                context.raw_uvs.push_back(uv);
                uv = transform_uv(primitive, uv);
                types::Skinning4 skin{
                    glm::u32vec4(static_cast<std::uint32_t>(&node - context.source.nodes)),
                    glm::vec4(1, 0, 0, 0)};
                if (node.skin != nullptr) {
                    skin.bone_weight = glm::make_vec4(&weights[index * INFLUENCES]);
                    for (std::size_t influence = 0; influence < INFLUENCES; ++influence) {
                        const auto joint = joints[index * INFLUENCES + influence];
                        if (joint < 0 || !(joint < static_cast<double>(node.skin->joints_count)) ||
                            std::floor(joint) != joint ||
                            skin.bone_weight[static_cast<int>(influence)] < 0) {
                            return context.fail("Invalid VRM skin influence");
                        }
                        skin.bone_index[static_cast<int>(influence)] = static_cast<std::uint32_t>(
                            node.skin->joints[static_cast<std::size_t>(joint)] -
                            context.source.nodes);
                    }
                    const float total = skin.bone_weight.x + skin.bone_weight.y +
                                        skin.bone_weight.z + skin.bone_weight.w;
                    if (!(total > 0) || !std::isfinite(total)) {
                        return context.fail("Invalid VRM weight sum");
                    }
                    // Normalize weights: w_i' = w_i / sum(w).
                    skin.bone_weight /= total;
                }
                context.model.vertices.push_back(
                    {types::Vertex{position, normal, uv}, skin, types::EdgeFlag{0}});
            }
            return true;
        }

        bool make_morphs(VRMContext& context,
            const cgltf_node& node,
            const cgltf_primitive& primitive,
            const glm::mat4& transform,
            std::size_t count,
            std::uint32_t base) {
            for (std::size_t index = 0; index < primitive.targets_count; ++index) {
                const auto key =
                    std::make_pair(static_cast<std::size_t>(&node - context.source.nodes), index);
                if (!context.morphs.contains(key)) {
                    context.morphs.emplace(
                        key, static_cast<std::uint32_t>(context.targets.targets.size()));
                    types::MorphTarget target;
                    target.name =
                        "vrm_node_" + std::to_string(key.first) + "_morph_" + std::to_string(index);
                    if (index < node.weights_count) {
                        target.default_weight = node.weights[index];
                    } else if (index < node.mesh->weights_count) {
                        target.default_weight = node.mesh->weights[index];
                    }
                    if (!std::isfinite(target.default_weight)) {
                        return context.fail("Non-finite VRM default morph weight");
                    }
                    context.targets.targets.push_back(std::move(target));
                }
                auto& target = context.targets.targets[context.morphs.at(key)];
                const auto& source = primitive.targets[index];
                std::vector<float> offsets;
                if (!unpack(attribute(source.attributes,
                                source.attributes_count,
                                cgltf_attribute_type_position),
                        count,
                        POSITION_COMPONENTS,
                        offsets)) {
                    return context.fail("Invalid VRM morph target");
                }
                for (std::size_t vertex = 0; vertex < count; ++vertex) {
                    // A morph is a displacement, so only the linear part transforms it.
                    const auto offset = glm::mat3(transform) *
                                        glm::make_vec3(&offsets[vertex * POSITION_COMPONENTS]);
                    target.offsets.emplace_back(types::VertexMorphOffset{
                        base + static_cast<std::uint32_t>(vertex), offset});
                }
            }
            return true;
        }
    } // namespace

    bool make_vrm_meshes(VRMContext& context) {
        context.model.indices = std::vector<std::uint32_t>{};
        std::vector<bool> assigned(context.source.nodes_count);
        for (std::size_t node_index = 0; node_index < context.source.nodes_count; ++node_index) {
            const auto& node = context.source.nodes[node_index];
            if (!context.active_nodes[node_index] || node.mesh == nullptr) {
                continue;
            }
            glm::mat4 transform = context.basis;
            if (node.skin != nullptr) {
                if (!make_skin(context, *node.skin, assigned)) {
                    return false;
                }
            } else {
                glm::mat4 world(1);
                cgltf_node_transform_world(&node, glm::value_ptr(world));
                transform *= world;
            }
            for (std::size_t index = 0; index < node.mesh->primitives_count; ++index) {
                const auto& primitive = node.mesh->primitives[index];
                const auto* positions = attribute(primitive.attributes,
                    primitive.attributes_count,
                    cgltf_attribute_type_position);
                const auto base = context.model.vertices.size();
                if (positions == nullptr || positions->count == 0 ||
                    positions->count > std::numeric_limits<std::uint32_t>::max() - base) {
                    return context.fail("Missing or oversized VRM positions");
                }
                const auto first =
                    std::get<std::vector<std::uint32_t>>(context.model.indices).size();
                if (!make_indices(
                        context, primitive, positions->count, static_cast<std::uint32_t>(base)) ||
                    !make_vertices(context, node, primitive, transform, positions->count) ||
                    !make_morphs(context,
                        node,
                        primitive,
                        transform,
                        positions->count,
                        static_cast<std::uint32_t>(base))) {
                    return false;
                }
                const auto count =
                    std::get<std::vector<std::uint32_t>>(context.model.indices).size() - first;
                if (!make_vrm_material(context,
                        primitive.material,
                        static_cast<std::uint32_t>(first),
                        static_cast<std::uint32_t>(count))) {
                    return false;
                }
            }
        }
        return !context.model.vertices.empty() || context.fail("VRM scene contains no mesh");
    }
} // namespace enishi::assets_system
