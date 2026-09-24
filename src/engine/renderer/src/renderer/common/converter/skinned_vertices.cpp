#include "skinned_vertices.h"

namespace enishi::renderer {
    std::optional<std::uint32_t> skinned_vertex_offset(std::string_view semantic) {
        if (semantic == "POSITION") {
            return static_cast<std::uint32_t>(offsetof(SkinnedVertex, position));
        }
        if (semantic == "NORMAL") {
            return static_cast<std::uint32_t>(offsetof(SkinnedVertex, normal));
        }
        if (semantic == "TEXCOORD") {
            return static_cast<std::uint32_t>(offsetof(SkinnedVertex, uv));
        }
        if (semantic == "BONEINDICES") {
            return static_cast<std::uint32_t>(offsetof(SkinnedVertex, bones));
        }
        if (semantic == "BONEWEIGHTS") {
            return static_cast<std::uint32_t>(offsetof(SkinnedVertex, weights));
        }
        if (semantic == "EDGE_FLAG") {
            return static_cast<std::uint32_t>(offsetof(SkinnedVertex, edge));
        }
        if (semantic == "SKINNING_METHOD") {
            return static_cast<std::uint32_t>(offsetof(SkinnedVertex, method));
        }
        if (semantic == "BLEND_CENTER") {
            return static_cast<std::uint32_t>(offsetof(SkinnedVertex, center));
        }
        if (semantic == "BLEND_ANCHOR_A") {
            return static_cast<std::uint32_t>(offsetof(SkinnedVertex, anchor0));
        }
        if (semantic == "BLEND_ANCHOR_B") {
            return static_cast<std::uint32_t>(offsetof(SkinnedVertex, anchor1));
        }
        return std::nullopt;
    }
    namespace {
        foundation::Result<SkinnedVertex, RendererError> convert_vertex(
            const types::VertexVariants& source) {
            SkinnedVertex output;
            bool has_vertex = false;
            bool has_skin = false;
            for (const auto& attribute : source) {
                if (const auto* vertex = std::get_if<types::Vertex>(&attribute);
                    vertex != nullptr) {
                    output.position = vertex->position;
                    output.normal = vertex->normal;
                    output.uv = vertex->uv;
                    has_vertex = true;
                } else if (const auto* skin = std::get_if<types::Skinning>(&attribute);
                           skin != nullptr) {
                    output.bones = {skin->bone_index.x, skin->bone_index.y, 0, 0};
                    output.weights = {skin->bone_weight.x, skin->bone_weight.y, 0, 0};
                    has_skin = true;
                } else if (const auto* skin4 = std::get_if<types::Skinning4>(&attribute);
                           skin4 != nullptr) {
                    output.bones = skin4->bone_index;
                    output.weights = skin4->bone_weight;
                    has_skin = true;
                } else if (const auto* edge = std::get_if<types::EdgeFlag>(&attribute);
                           edge != nullptr) {
                    output.edge = edge->flag;
                }
            }
            if (!has_vertex || !has_skin) {
                return foundation::Error(RendererError::ConvertError,
                    "Skinned vertices require position, normal, UV and skinning attributes");
            }
            for (glm::length_t influence = 0; influence < output.bones.length(); ++influence) {
                // Even a zero-weight shader operand must have a valid matrix index.
                if (output.weights[influence] == 0.0f) {
                    output.bones[influence] = 0;
                }
            }
            return output;
        }
    } // namespace

    foundation::Result<std::vector<SkinnedVertex>, RendererError> make_skinned_vertices(
        const types::ModelData& model) {
        if ((!model.skinning_methods.empty() &&
                model.skinning_methods.size() != model.vertices.size()) ||
            (!model.spherical_blends.empty() &&
                model.spherical_blends.size() != model.vertices.size())) {
            return foundation::Error(RendererError::ConvertError, "Invalid skinning metadata size");
        }
        std::vector<SkinnedVertex> vertices;
        vertices.reserve(model.vertices.size());
        for (std::size_t index = 0; index < model.vertices.size(); ++index) {
            auto converted = convert_vertex(model.vertices[index]);
            if (converted.is_err()) {
                return std::move(converted).unwrap_err();
            }
            auto vertex = std::move(converted).unwrap_mut();
            if (!model.skinning_methods.empty()) {
                vertex.method = static_cast<std::uint32_t>(model.skinning_methods[index]);
            }
            if (vertex.method ==
                static_cast<std::uint32_t>(types::SkinningMethod::SphericalBlend)) {
                if (model.spherical_blends.empty()) {
                    return foundation::Error(RendererError::ConvertError,
                        "Spherical blending requires corrected anchors");
                }
                const auto& blend = model.spherical_blends[index];
                vertex.center = blend.center;
                vertex.anchor0 = blend.anchor0;
                vertex.anchor1 = blend.anchor1;
            }
            vertices.push_back(vertex);
        }
        return vertices;
    }
} // namespace enishi::renderer
