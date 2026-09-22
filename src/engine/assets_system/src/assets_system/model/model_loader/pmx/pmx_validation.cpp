#include "pmx_model_loader.h"
#include <cmath>

namespace enishi::assets_system {
    namespace {
        bool reference(std::int32_t index, std::size_t count, bool optional = false) {
            return (optional && index == -1) ||
                   (index >= 0 && static_cast<std::size_t>(index) < count);
        }

        bool acyclic(const std::vector<std::vector<std::size_t>>& edges) {
            std::vector<std::uint8_t> state(edges.size());
            std::vector<std::pair<std::size_t, std::size_t>> stack;
            for (std::size_t root = 0; root < edges.size(); ++root) {
                if (state[root] != 0) {
                    continue;
                }
                state[root] = 1;
                stack.emplace_back(root, 0);
                while (!stack.empty()) {
                    auto& [node, next] = stack.back();
                    if (next == edges[node].size()) {
                        state[node] = 2;
                        stack.pop_back();
                        continue;
                    }
                    const auto child = edges[node][next++];
                    if (state[child] == 1) {
                        return false;
                    }
                    if (state[child] == 0) {
                        state[child] = 1;
                        stack.emplace_back(child, 0);
                    }
                }
            }
            return true;
        }
    } // namespace

    foundation::Result<void, AssetError> PMXModelLoader::validate(const PMXData& data) {
        const auto invalid = [](const char* message) {
            return foundation::Error(AssetError::InvalidAssetData, std::string("PMX: ") + message);
        };
        if ((data.version != 2.0f && data.version != 2.1f) || data.additional_uv_count > 4) {
            return invalid("invalid version or additional UV count");
        }
        for (const auto& vertex : data.vertices) {
            if (vertex.deform_type > (data.version == 2.1f ? 4 : 3) ||
                vertex.additional_uvs.size() != data.additional_uv_count) {
                return invalid("invalid vertex deformation or UV count");
            }
            for (std::size_t i = 0; i < vertex.bones.size(); ++i) {
                if (!reference(vertex.bones[i], data.bones.size(), true) ||
                    !std::isfinite(vertex.weights[i]) || vertex.weights[i] < 0) {
                    return invalid("invalid vertex bone or weight");
                }
            }
        }
        if (data.indices.size() % 3 != 0) {
            return invalid("incomplete triangle");
        }
        for (const auto index : data.indices) {
            if (index >= data.vertices.size()) {
                return invalid("vertex index out of range");
            }
        }
        std::size_t total_indices{};
        for (const auto& material : data.materials) {
            if (material.sphere_mode > 3 || material.shared_toon > 1 || material.index_count < 0 ||
                material.index_count % 3 != 0 ||
                static_cast<std::size_t>(material.index_count) >
                    data.indices.size() - total_indices ||
                !reference(material.texture, data.textures.size(), true) ||
                !reference(material.sphere_texture, data.textures.size(), true) ||
                (material.shared_toon == 0 &&
                    !reference(material.toon_texture, data.textures.size(), true)) ||
                (material.shared_toon == 1 && !reference(material.toon_texture, 10))) {
                return invalid("invalid material range or texture reference");
            }
            total_indices += material.index_count;
        }
        if (total_indices != data.indices.size()) {
            return invalid("material ranges do not cover indices");
        }

        std::vector<std::vector<std::size_t>> bone_edges(data.bones.size());
        for (std::size_t i = 0; i < data.bones.size(); ++i) {
            const auto& bone = data.bones[i];
            if (!reference(bone.parent, data.bones.size(), true) ||
                ((bone.flags & 1) && !reference(bone.tail, data.bones.size(), true)) ||
                ((bone.flags & 0x0300) &&
                    !reference(bone.inherit_parent, data.bones.size(), true))) {
                return invalid("bone reference out of range");
            }
            if (bone.parent >= 0) {
                bone_edges[i].push_back(bone.parent);
            }
            if ((bone.flags & 0x0300) && bone.inherit_parent >= 0) {
                bone_edges[i].push_back(bone.inherit_parent);
            }
            if (bone.flags & 0x0020) {
                if (!reference(bone.ik_target, data.bones.size()) || bone.ik_iterations < 0) {
                    return invalid("invalid IK target or iterations");
                }
                for (const auto& link : bone.ik_links) {
                    if (!reference(link.bone, data.bones.size())) {
                        return invalid("IK link out of range");
                    }
                    for (std::size_t axis = 0; axis < 3; ++axis) {
                        if (link.limited && link.lower[axis] > link.upper[axis]) {
                            return invalid("inverted IK angle limits");
                        }
                    }
                }
            }
        }
        if (!acyclic(bone_edges)) {
            return invalid("cyclic bone hierarchy or inheritance");
        }

        std::vector<std::vector<std::size_t>> morph_edges(data.morphs.size());
        for (std::size_t i = 0; i < data.morphs.size(); ++i) {
            const auto& morph = data.morphs[i];
            if (morph.type > (data.version == 2.1f ? 10 : 8)) {
                return invalid("invalid morph type");
            }
            for (const auto& offset : morph.offsets) {
                const auto count = morph.type == 0 || morph.type == 9 ? data.morphs.size()
                                   : morph.type == 2                  ? data.bones.size()
                                   : morph.type == 8                  ? data.materials.size()
                                   : morph.type == 10                 ? data.rigid_bodies.size()
                                                                      : data.vertices.size();
                if (!reference(offset.index, count, morph.type == 8)) {
                    return invalid("morph reference out of range");
                }
                // Discrete selections are evaluated in source order and may refer to themselves.
                if (morph.type == 0) {
                    morph_edges[i].push_back(offset.index);
                }
            }
        }
        if (!acyclic(morph_edges)) {
            return invalid("cyclic morph references");
        }
        for (const auto& frame : data.display_frames) {
            for (const auto& element : frame.elements) {
                if (!reference(element.index,
                        element.type == 0 ? data.bones.size() : data.morphs.size())) {
                    return invalid("display reference out of range");
                }
            }
        }
        for (const auto& body : data.rigid_bodies) {
            if (body.shape > 2 || body.mode > 2 || body.group >= 16 ||
                !reference(body.bone, data.bones.size(), true)) {
                return invalid("rigid body bone out of range");
            }
        }
        for (const auto& joint : data.joints) {
            if (joint.type > (data.version == 2.1f ? 5 : 0) ||
                !reference(joint.body_a, data.rigid_bodies.size(), true) ||
                !reference(joint.body_b, data.rigid_bodies.size(), true)) {
                return invalid("joint body out of range");
            }
        }
        for (const auto& body : data.soft_bodies) {
            if (data.version != 2.1f || body.shape > 1 || body.group >= 16 || body.aero_model < 0 ||
                body.aero_model > 4 || !reference(body.material, data.materials.size())) {
                return invalid("soft body material out of range");
            }
            for (const auto& anchor : body.anchors) {
                if (!reference(anchor.body, data.rigid_bodies.size()) ||
                    !reference(anchor.vertex, data.vertices.size())) {
                    return invalid("soft body anchor out of range");
                }
            }
            for (const auto vertex : body.pinned_vertices) {
                if (!reference(vertex, data.vertices.size())) {
                    return invalid("pinned vertex out of range");
                }
            }
        }
        return {};
    }
} // namespace enishi::assets_system
