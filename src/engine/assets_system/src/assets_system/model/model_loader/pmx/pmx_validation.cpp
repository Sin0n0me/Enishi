#include "pmx_model_loader.h"
#include <cmath>

namespace enishi::assets_system {
    namespace {
        constexpr std::int32_t NO_PMX_INDEX{-1};
        constexpr float PMX_VERSION_2_0{2.0f};
        constexpr float PMX_VERSION_2_1{2.1f};
        constexpr std::uint8_t MAX_ADDITIONAL_UV_COUNT{4};
        constexpr std::uint8_t PMX_DEFORM_BDEF4{2};
        constexpr std::uint8_t PMX_DEFORM_SDEF{3};
        constexpr std::uint8_t PMX_DEFORM_QDEF{4};
        constexpr std::size_t TRIANGLE_VERTEX_COUNT{3};
        constexpr std::uint8_t MAX_SPHERE_MODE{3};
        constexpr std::uint8_t MAX_SHARED_TOON_MODE{1};
        constexpr std::size_t SHARED_TOON_TEXTURE_COUNT{10};
        constexpr std::uint16_t BONE_FLAG_TAIL_IS_BONE{0x0001};
        constexpr std::uint16_t BONE_FLAG_INHERIT{0x0300};
        constexpr std::uint16_t BONE_FLAG_IK{0x0020};
        constexpr std::size_t AXIS_COUNT{3};
        constexpr std::uint8_t MORPH_TYPE_GROUP{};
        constexpr std::uint8_t MORPH_TYPE_BONE{2};
        constexpr std::uint8_t MORPH_TYPE_MATERIAL{8};
        constexpr std::uint8_t MORPH_TYPE_FLIP{9};
        constexpr std::uint8_t MORPH_TYPE_IMPULSE{10};
        constexpr std::uint8_t MAX_RIGID_BODY_SHAPE{2};
        constexpr std::uint8_t MAX_RIGID_BODY_MODE{2};
        constexpr std::uint8_t MAX_RIGID_BODY_GROUP{15};
        constexpr std::uint8_t MAX_SOFT_BODY_SHAPE{1};
        constexpr std::int32_t MAX_AERODYNAMIC_MODEL{4};
        constexpr std::uint8_t MAX_PMX_2_0_JOINT_TYPE{};
        constexpr std::uint8_t MAX_PMX_2_1_JOINT_TYPE{5};

        enum class VisitState : std::uint8_t {
            Unvisited,
            Visiting,
            Visited,
        };

        bool reference(std::int32_t index, std::size_t count, bool optional = false) {
            return (optional && index == NO_PMX_INDEX) ||
                   (index > NO_PMX_INDEX && static_cast<std::size_t>(index) < count);
        }

        bool acyclic(const std::vector<std::vector<std::size_t>>& edges) {
            std::vector<VisitState> state(edges.size());
            std::vector<std::pair<std::size_t, std::size_t>> stack;
            for (std::size_t root = 0; root < edges.size(); ++root) {
                if (state[root] != VisitState::Unvisited) {
                    continue;
                }
                state[root] = VisitState::Visiting;
                stack.emplace_back(root, 0);
                while (!stack.empty()) {
                    auto& [node, next] = stack.back();
                    if (next == edges[node].size()) {
                        state[node] = VisitState::Visited;
                        stack.pop_back();
                        continue;
                    }
                    const auto child = edges[node][next++];
                    if (state[child] == VisitState::Visiting) {
                        return false;
                    }
                    if (state[child] == VisitState::Unvisited) {
                        state[child] = VisitState::Visiting;
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
        if ((data.version != PMX_VERSION_2_0 && data.version != PMX_VERSION_2_1) ||
            data.additional_uv_count > MAX_ADDITIONAL_UV_COUNT) {
            return invalid("invalid version or additional UV count");
        }
        for (const auto& vertex : data.vertices) {
            if (vertex.deform_type >
                    (data.version == PMX_VERSION_2_1 ? PMX_DEFORM_QDEF : PMX_DEFORM_SDEF) ||
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
        if (data.indices.size() % TRIANGLE_VERTEX_COUNT != 0) {
            return invalid("incomplete triangle");
        }
        for (const auto index : data.indices) {
            if (!(index < data.vertices.size())) {
                return invalid("vertex index out of range");
            }
        }
        std::size_t total_indices{};
        for (const auto& material : data.materials) {
            if (material.sphere_mode > MAX_SPHERE_MODE ||
                material.shared_toon > MAX_SHARED_TOON_MODE || material.index_count < 0 ||
                material.index_count % TRIANGLE_VERTEX_COUNT != 0 ||
                static_cast<std::size_t>(material.index_count) >
                    data.indices.size() - total_indices ||
                !reference(material.texture, data.textures.size(), true) ||
                !reference(material.sphere_texture, data.textures.size(), true) ||
                (material.shared_toon == 0 &&
                    !reference(material.toon_texture, data.textures.size(), true)) ||
                (material.shared_toon == MAX_SHARED_TOON_MODE &&
                    !reference(material.toon_texture, SHARED_TOON_TEXTURE_COUNT))) {
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
                ((bone.flags & BONE_FLAG_TAIL_IS_BONE) != 0 &&
                    !reference(bone.tail, data.bones.size(), true)) ||
                ((bone.flags & BONE_FLAG_INHERIT) != 0 &&
                    !reference(bone.inherit_parent, data.bones.size(), true))) {
                return invalid("bone reference out of range");
            }
            if (bone.parent > NO_PMX_INDEX) {
                bone_edges[i].push_back(bone.parent);
            }
            if ((bone.flags & BONE_FLAG_INHERIT) != 0 && bone.inherit_parent > NO_PMX_INDEX) {
                bone_edges[i].push_back(bone.inherit_parent);
            }
            if ((bone.flags & BONE_FLAG_IK) != 0) {
                if (!reference(bone.ik_target, data.bones.size()) || bone.ik_iterations < 0) {
                    return invalid("invalid IK target or iterations");
                }
                for (const auto& link : bone.ik_links) {
                    if (!reference(link.bone, data.bones.size())) {
                        return invalid("IK link out of range");
                    }
                    for (std::size_t axis = 0; axis < AXIS_COUNT; ++axis) {
                        if (link.limited != 0 && link.lower[axis] > link.upper[axis]) {
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
            if (morph.type >
                (data.version == PMX_VERSION_2_1 ? MORPH_TYPE_IMPULSE : MORPH_TYPE_MATERIAL)) {
                return invalid("invalid morph type");
            }
            for (const auto& offset : morph.offsets) {
                std::size_t count = data.vertices.size();
                if (morph.type == MORPH_TYPE_GROUP || morph.type == MORPH_TYPE_FLIP) {
                    count = data.morphs.size();
                } else if (morph.type == MORPH_TYPE_BONE) {
                    count = data.bones.size();
                } else if (morph.type == MORPH_TYPE_MATERIAL) {
                    count = data.materials.size();
                } else if (morph.type == MORPH_TYPE_IMPULSE) {
                    count = data.rigid_bodies.size();
                }
                if (!reference(offset.index, count, morph.type == MORPH_TYPE_MATERIAL)) {
                    return invalid("morph reference out of range");
                }
                // Discrete selections are evaluated in source order and may refer to themselves.
                if (morph.type == MORPH_TYPE_GROUP) {
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
            if (body.shape > MAX_RIGID_BODY_SHAPE || body.mode > MAX_RIGID_BODY_MODE ||
                body.group > MAX_RIGID_BODY_GROUP ||
                !reference(body.bone, data.bones.size(), true)) {
                return invalid("rigid body bone out of range");
            }
        }
        for (const auto& joint : data.joints) {
            if (joint.type > (data.version == PMX_VERSION_2_1 ? MAX_PMX_2_1_JOINT_TYPE
                                                              : MAX_PMX_2_0_JOINT_TYPE) ||
                !reference(joint.body_a, data.rigid_bodies.size(), true) ||
                !reference(joint.body_b, data.rigid_bodies.size(), true)) {
                return invalid("joint body out of range");
            }
        }
        for (const auto& body : data.soft_bodies) {
            if (data.version != PMX_VERSION_2_1 || body.shape > MAX_SOFT_BODY_SHAPE ||
                body.group > MAX_RIGID_BODY_GROUP || body.aero_model < 0 ||
                body.aero_model > MAX_AERODYNAMIC_MODEL ||
                !reference(body.material, data.materials.size())) {
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
