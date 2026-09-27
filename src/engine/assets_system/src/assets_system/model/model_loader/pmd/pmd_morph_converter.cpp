#include "pmd_morph_converter.h"
#include <algorithm>
#include <cmath>
#include <foundation/str/to_utf8.h>

namespace enishi::assets_system {
    namespace {
        constexpr std::uint8_t BASE_MORPH = 0;

        glm::vec3 position(const PMDMorphVertex& vertex) {
            return {vertex.position[0], vertex.position[1], vertex.position[2]};
        }

        bool finite_position(const PMDMorphVertex& vertex) {
            return std::isfinite(vertex.position[0]) && std::isfinite(vertex.position[1]) &&
                   std::isfinite(vertex.position[2]);
        }
    } // namespace

    foundation::Result<PMDMorphAddons, AssetError> convert_pmd_morphs(
        std::span<const PMDMorph> morphs, std::size_t vertex_count) {
        PMDMorphAddons result;
        if (morphs.empty()) {
            return result;
        }
        const auto& base = morphs.front();
        if (base.skin_type != BASE_MORPH) {
            return foundation::Error(AssetError::InvalidAssetData, "PMD base morph is missing");
        }
        for (const auto& vertex : base.vertices) {
            if (!(vertex.index < vertex_count) || !finite_position(vertex)) {
                return foundation::Error(
                    AssetError::InvalidAssetData, "Invalid PMD base morph vertex");
            }
            result.legacy.base_vertices.push_back({vertex.index, position(vertex)});
        }
        for (const auto& source : morphs.subspan(1)) {
            if (source.skin_type == BASE_MORPH) {
                return foundation::Error(AssetError::InvalidAssetData, "Duplicate PMD base morph");
            }
            const std::string name(std::begin(source.name),
                std::find(std::begin(source.name), std::end(source.name), '\0'));
            const auto decoded = foundation::sjis_to_utf8(name);
            if (decoded.is_err()) {
                return decoded.propagation(AssetError::InvalidAssetData);
            }
            types::MorphTarget target;
            target.name = decoded.unwrap();
            auto& legacy = result.legacy.vertices.emplace_back();
            for (const auto& vertex : source.vertices) {
                if (!(vertex.index < base.vertices.size()) || !finite_position(vertex)) {
                    return foundation::Error(
                        AssetError::InvalidAssetData, "Invalid PMD morph vertex");
                }
                // PMD offsets address the base-morph table, not the model's vertex array.
                const auto model_index = base.vertices[vertex.index].index;
                target.offsets.emplace_back(
                    types::VertexMorphOffset{model_index, position(vertex)});
                legacy.push_back({vertex.index, position(vertex)});
            }
            result.targets.targets.push_back(std::move(target));
        }
        return result;
    }
} // namespace enishi::assets_system
