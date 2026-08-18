#pragma once
#include "../../renderer/mesh_data.h"
#include "../../renderer/render_data.h"
#include "../texture/texture_data.h"
#include "addons.h"
#include "materials.h"
#include <cstdint>
#include <glm/glm.hpp>
#include <memory>
#include <variant>
#include <vector>

namespace enishi::types {
    struct Vertex {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 uv;
    };

    struct Skinning {
        glm::u16vec2 bone_index; // ボーン番号(GPU上では1つのu32扱い)
        glm::vec2 bone_weight;   // 0-1
    };

    using VertexVariant = std::variant<Vertex, Skinning>;
    using VertexVariants = std::vector<VertexVariant>;
    using IndicesVariant = std::variant<std::monostate,
        std::vector<std::uint8_t>,
        std::vector<std::uint16_t>,
        std::vector<std::uint32_t>>;

    // このアプリケーション向けに設定されたモデルデータ
    // 基本的には不変
    struct ModelData {
        std::string name;
        std::filesystem::path path;
        std::vector<VertexVariants> vertices;
        IndicesVariant indices;
        std::vector<ModelAddon> addons;
        std::vector<Material> materials;
        std::unordered_map<std::filesystem::path, std::shared_ptr<TextureData>> textures;

        [[nodiscard]] bool is_valid_data(void) const;

        // 大量のコピーが発生するので頻繁に呼ばないこと
        [[nodiscard]] MeshData to_mesh_data(const std::uint32_t uniform_separator = 16) const;

      private:
        using Uniforms = std::unordered_map<std::string, OwnedRenderData>;

        [[nodiscard]] OwnedRenderData to_vertices(void) const;
        [[nodiscard]] OwnedRenderData to_indices(void) const;
        [[nodiscard]] Uniforms to_uniforms(const std::uint32_t separator) const;
        [[nodiscard]] void to_uniforms_from_addon(Uniforms& uniforms) const;
        [[nodiscard]] void to_uniforms_from_material(
            Uniforms& uniforms, const std::uint32_t separator) const;

        [[nodiscard]] std::vector<DrawArgs> to_draw_args(void) const;
    };
} // namespace enishi::types