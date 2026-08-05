#include "model_data.h"
#include <cstddef>
#include <span>
#include <type_traits>

namespace enishi::types {
    template <typename T>
        requires std::is_trivially_copyable_v<T>
    void append_bytes(std::vector<std::byte>& buffer, const T& value) {
        const auto bytes = std::as_bytes(std::span{&value, sizeof(std::byte)});
        buffer.insert(buffer.end(), bytes.begin(), bytes.end());
    }

    template <typename T>
        requires std::is_trivially_copyable_v<T>
    std::vector<std::byte> to_byte_vector(const std::vector<T>& src) {
        const auto byte_size = src.size() * sizeof(T);
        std::vector<std::byte> dest(byte_size);
        std::memcpy(dest.data(), src.data(), byte_size);
        return dest;
    }

    bool ModelData::is_valid_data(void) const {
        if (this->name.empty()) {
            return false;
        }
        if (this->vertices.empty()) {
            return false;
        }
        if (this->vertices[0].empty()) {
            return false;
        }

        // 最初の要素のサイズを基準とする
        const auto target_size = this->vertices.front().size();

        // すべての要素が基準サイズと同じかチェック
        if (!std::all_of(this->vertices.begin() + 1,
                this->vertices.end(),
                [target_size](const std::vector<VertexVariant>& variant) {
                    return variant.size() == target_size;
                })) {
            return false;
        }

        return false;
    }

    MeshData ModelData::to_mesh_data(void) const {
        return MeshData{
            .vertices = this->to_vertices(),
            .indices = this->to_indices(),
            .uniforms = this->to_uniforms(),
        };
    }

    OwnedRenderData<std::byte> ModelData::to_vertices(void) const {
        std::vector<std::byte> vertices;
        for (const auto& vertex : this->vertices) {
            for (const auto& variant : vertex) {
                if (auto data = std::get_if<Vertex>(&variant)) {
                    append_bytes(vertices, data);
                }
                if (auto data = std::get_if<Skinning>(&variant)) {
                    append_bytes(vertices, data);
                }
            }
        }

        return OwnedRenderData<std::byte>{std::move(vertices)};
    }

    OwnedRenderData<std::byte> ModelData::to_indices(void) const {
        std::vector<std::byte> byte_indices;
        if (auto indices = std::get_if<std::vector<std::uint8_t>>(&this->indices)) {
            byte_indices = to_byte_vector(*indices);
        }
        if (auto indices = std::get_if<std::vector<std::uint16_t>>(&this->indices)) {
            byte_indices = to_byte_vector(*indices);
        }
        if (auto indices = std::get_if<std::vector<std::uint32_t>>(&this->indices)) {
            byte_indices = to_byte_vector(*indices);
        }

        return OwnedRenderData<std::byte>{std::move(byte_indices)};
    }

    std::vector<OwnedRenderData<std::byte>> ModelData::to_uniforms(void) const {
        auto uniforms = std::vector<OwnedRenderData<std::byte>>{};

        for (const auto& material : this->materials) {
            std::vector<std::byte> uniform;

            for (const auto& variant : material.variants) {
                if (const auto& data = std::get_if<Ambient>(&variant)) {
                    append_bytes(uniform, data);
                }
                if (const auto& data = std::get_if<Specular>(&variant)) {
                    append_bytes(uniform, data);
                }
                if (const auto& data = std::get_if<Diffuse>(&variant)) {
                    append_bytes(uniform, data);
                }
            }

            uniforms.emplace_back(OwnedRenderData<std::byte>{std::move(uniform)});
        }

        return uniforms;
    }
} // namespace enishi::types