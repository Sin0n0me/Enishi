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

    MeshData ModelData::to_mesh_data(const std::uint32_t uniform_separator) const {
        return MeshData{
            .vertices = this->to_vertices(),
            .indices = this->to_indices(),
            .uniforms = this->to_uniforms(uniform_separator),
            .draw_args = this->to_draw_args(),
        };
    }

    OwnedRenderData ModelData::to_vertices(void) const {
        std::vector<std::byte> vertices;

        const auto append_vertex = [&vertices](const std::vector<types::VertexVariant>& vertex) {
            for (const auto& variant : vertex) {
                if (auto data = std::get_if<Vertex>(&variant)) {
                    append_bytes(vertices, *data);
                }
                if (auto data = std::get_if<Skinning>(&variant)) {
                    append_bytes(vertices, *data);
                }
            }
        };

        // 先頭のみバイト幅の取得などで別途処理
        const auto& first_vertex = this->vertices.front();
        append_vertex(first_vertex);
        const auto stride = vertices.size();

        // 残り
        for (auto vertex = this->vertices.begin() + 1; vertex != this->vertices.end(); vertex++) {
            append_vertex(*vertex);
        }

        return OwnedRenderData{std::move(vertices), static_cast<std::uint32_t>(stride)};
    }

    OwnedRenderData ModelData::to_indices(void) const {
        if (auto indices = std::get_if<std::vector<std::uint8_t>>(&this->indices)) {
            return OwnedRenderData{*indices};
        }
        if (auto indices = std::get_if<std::vector<std::uint16_t>>(&this->indices)) {
            return OwnedRenderData{*indices};
        }
        if (auto indices = std::get_if<std::vector<std::uint32_t>>(&this->indices)) {
            return OwnedRenderData{*indices};
        }

        return OwnedRenderData{OwnedRenderData::RawDataType{}};
    }

    std::unordered_map<std::string, OwnedRenderData> ModelData::to_uniforms(
        const std::uint32_t separator) const {
        auto uniforms = std::unordered_map<std::string, OwnedRenderData>{};

        for (const auto& material : this->materials) {
            std::vector<std::byte> uniform;

            for (const auto& variant : material.variants) {
                if (auto data = std::get_if<Ambient>(&variant)) {
                    append_bytes(uniform, data);
                }
                if (auto data = std::get_if<Specular>(&variant)) {
                    append_bytes(uniform, data);
                }
                if (auto data = std::get_if<Diffuse>(&variant)) {
                    append_bytes(uniform, data);
                }
            }

            // 16Byte区切りにする(DirectX12だと256バイト区切りだからどうしようかな)
            const auto stride = uniform.size();
            constexpr auto SEPARATOR = 16;
            const auto padding = separator - stride % separator;
            if (padding != 0) {
                uniform.resize(stride + padding);
            }

            uniforms.emplace(material.name,
                OwnedRenderData{std::move(uniform), static_cast<std::uint32_t>(stride + padding)});
        }

        return uniforms;
    }

    std::vector<DrawArgs> ModelData::to_draw_args(void) const {
        std::vector<DrawArgs> draw_args;

        if (this->materials.empty()) {
            if (std::holds_alternative<std::monostate>(this->indices)) {
                draw_args.emplace_back(Draw{
                    .vertex_count = static_cast<std::uint32_t>(this->vertices.size()),
                    .instance_count = 0,
                    .first_vertex = 0,
                    .first_instance = 0,
                });
            } else {
                auto index_count = 0u;
                if (auto indices = std::get_if<std::vector<std::uint8_t>>(&this->indices)) {
                    index_count = static_cast<std::uint32_t>(indices->size());
                }
                if (auto indices = std::get_if<std::vector<std::uint16_t>>(&this->indices)) {
                    index_count = static_cast<std::uint32_t>(indices->size());
                }
                if (auto indices = std::get_if<std::vector<std::uint32_t>>(&this->indices)) {
                    index_count = static_cast<std::uint32_t>(indices->size());
                }

                draw_args.emplace_back(DrawIndexed{
                    .index_count = index_count,
                    .instance_count = 0,
                    .first_index = 0,
                    .vertex_offset = 0,
                    .first_instance = 0,
                });
            }
        } else {
            draw_args.reserve(this->materials.size());
            auto offset = 0;
            for (const auto& material : this->materials) {
                if (std::holds_alternative<std::monostate>(this->indices)) {
                    draw_args.emplace_back(Draw{
                        .vertex_count = material.count,
                        .instance_count = material.instance_count,
                        .first_vertex = material.first_offset,
                        .first_instance = material.first_instance_offset,
                    });
                } else {
                    draw_args.emplace_back(DrawIndexed{
                        .index_count = material.count,
                        .instance_count = material.instance_count,
                        .first_index = material.first_offset,
                        .vertex_offset = offset,
                        .first_instance = material.first_instance_offset,
                    });
                    offset += material.count;
                }
            }
        }

        return draw_args;
    }
} // namespace enishi::types