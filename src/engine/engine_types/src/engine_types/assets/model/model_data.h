#pragma once
#include "../../renderer/mesh_data.h"
#include "../../renderer/render_data.h"
#include "../texture/texture_data.h"
#include "addons.h"
#include "materials.h"

#include <cstdint>
#include <glm/glm.hpp>
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
        std::vector<VertexVariants> vertices;
        IndicesVariant indices;
        std::vector<ModelAddon> addons;
        std::vector<Material> materials;

        [[nodiscard]] bool is_valid_data(void) const;

        // コピーが発生するので頻繁に呼ばないこと
        [[nodiscard]] MeshData to_mesh_data(void) const;

      private:
        [[nodiscard]] OwnedRenderData<std::byte> to_vertices(void) const;
        [[nodiscard]] OwnedRenderData<std::byte> to_indices(void) const;
        [[nodiscard]] std::vector<OwnedRenderData<std::byte>> to_uniforms(void) const;
    };
} // namespace enishi::types