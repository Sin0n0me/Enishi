#include "model_to_mesh.h"
#include "skinned_vertices.h"
#include <algorithm>
#include <engine_types/renderer/texture/model_texture.h>
#include <engine_types/renderer/uniform_buffer/bones.h>
#include <engine_types/renderer/uniform_buffer/camera.h>
#include <engine_types/renderer/uniform_buffer/light.h>
#include <engine_types/renderer/uniform_buffer/material.h>
#include <foundation/str/string_builder.h>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

namespace enishi::renderer {
    namespace {
        constexpr std::uint8_t OPAQUE_WHITE = 255;
        constexpr std::uint8_t BLACK = 0;

        std::shared_ptr<types::TextureData> make_solid_texture(std::uint8_t color) {
            constexpr std::uint32_t SINGLE_PIXEL = 1;
            constexpr std::uint32_t RGBA_BYTES = 4;
            auto texture = std::make_shared<types::TextureData>();
            texture->format = types::TextureFormat::RGBA8_UNORM;
            texture->is_cubemap = false;
            texture->width = SINGLE_PIXEL;
            texture->height = SINGLE_PIXEL;
            texture->depth = SINGLE_PIXEL;
            texture->array_size = SINGLE_PIXEL;
            auto& mip = texture->mips.emplace_back();
            mip.width = SINGLE_PIXEL;
            mip.height = SINGLE_PIXEL;
            mip.row_pitch = RGBA_BYTES;
            mip.slice_pitch = RGBA_BYTES;
            mip.pixels = {color, color, color, OPAQUE_WHITE};
            return texture;
        }
    } // namespace

    template <typename T>
        requires std::is_trivially_copyable_v<T>
    void append_bytes(std::vector<std::byte>& buffer, const T& value) {
        const auto bytes = std::as_bytes(std::span{&value, sizeof(std::byte)});
        buffer.insert(buffer.end(), bytes.begin(), bytes.end());
    }

    // 大量のコピーが発生するので頻繁に呼ばないこと
    [[nodiscard]]
    foundation::Result<types::MeshData, RendererError> ModelToMesh::to_mesh_data(
        const types::ModelData& model_data, MeshConfig&& config) {
        auto&& vertices = ModelToMesh::to_vertices(model_data);
        if (vertices.is_err()) {
            return std::move(vertices).unwrap_err();
        }
        auto&& indices = ModelToMesh::to_indices(model_data);
        if (indices.is_err()) {
            return std::move(indices).unwrap_err();
        }
        auto&& uniforms = ModelToMesh::to_uniforms(model_data, config);
        if (uniforms.is_err()) {
            return std::move(uniforms).unwrap_err();
        }
        auto&& materials = ModelToMesh::to_mesh_material(model_data, config);
        if (materials.is_err()) {
            return std::move(materials).unwrap_err();
        }

        return types::MeshData{
            .vertices = std::move(vertices).unwrap_mut(),
            .indices = std::move(indices).unwrap_mut(),
            .uniforms = std::move(uniforms).unwrap_mut(),
            .materials = std::move(materials).unwrap_mut(),
        };
    }

    foundation::Result<types::OwnedRenderData, RendererError> ModelToMesh::to_vertices(
        const types::ModelData& model_data) {
        std::vector<std::byte> vertices;

        if (model_data.vertices.empty()) {
            return foundation::Error(RendererError::ConvertError, "Model has no vertices");
        }
        const auto& attributes = model_data.vertices.front();
        if (attributes.empty()) {
            return foundation::Error(RendererError::ConvertError, "Model vertex has no attributes");
        }
        if (std::ranges::any_of(attributes, [](const auto& attribute) {
                return std::holds_alternative<types::Skinning>(attribute) ||
                       std::holds_alternative<types::Skinning4>(attribute);
            })) {
            auto converted = make_skinned_vertices(model_data);
            if (converted.is_err()) {
                return std::move(converted).unwrap_err();
            }
            return types::OwnedRenderData{converted.unwrap()};
        }

        for (std::size_t index = 0; index < model_data.vertices.size(); ++index) {
            const auto& vertex = model_data.vertices[index];
            if (vertex.size() != attributes.size() ||
                !std::equal(vertex.begin(),
                    vertex.end(),
                    attributes.begin(),
                    [](const auto& value, const auto& expected) {
                        return value.index() == expected.index();
                    })) {
                return foundation::Error(RendererError::ConvertError,
                    std::format(
                        "Vertex {} has a different attribute layout from the first vertex", index));
            }
        }

        const auto append_vertex = [&vertices](const std::vector<types::VertexVariant>& vertex) {
            for (const auto& variant : vertex) {
                std::visit([&](const auto& data) { append_bytes(vertices, data); }, variant);
            }
        };

        // 先頭のみバイト幅の取得などで別途処理
        const auto& first_vertex = model_data.vertices.front();
        append_vertex(first_vertex);
        const auto stride = vertices.size();

        // 残り
        for (auto vertex = model_data.vertices.begin() + 1; vertex != model_data.vertices.end();
            vertex++) {
            append_vertex(*vertex);
        }

        return types::OwnedRenderData{std::move(vertices), static_cast<std::uint32_t>(stride)};
    }

    foundation::Result<types::OwnedRenderData, RendererError> ModelToMesh::to_indices(
        const types::ModelData& model_data) {
        if (auto indices = std::get_if<std::vector<std::uint8_t>>(&model_data.indices)) {
            return types::OwnedRenderData{*indices};
        }
        if (auto indices = std::get_if<std::vector<std::uint16_t>>(&model_data.indices)) {
            return types::OwnedRenderData{*indices};
        }
        if (auto indices = std::get_if<std::vector<std::uint32_t>>(&model_data.indices)) {
            return types::OwnedRenderData{*indices};
        }

        return foundation::Error(
            RendererError::ConvertError, "インデックスの数が多すぎます. 非対応です");
    }

    foundation::Result<ModelToMesh::Uniforms, RendererError> ModelToMesh::to_uniforms(
        const types::ModelData& model_data, const MeshConfig& config) {
        auto uniforms = Uniforms{};

        if (config.use_camera) {
            // 仮
            // TODO:
            std::vector<std::byte> uniform;
            uniform.reserve(sizeof(types::UniformCamera));
            constexpr float eye_position = 11.0f;
            constexpr float distance = 25.0f;
            constexpr float fov = 45.0f * (3.1415926535f / 180.0f);
            const glm::vec3 eye{
                0,
                eye_position + 7.5f,
                -distance,
            };
            const glm::vec3 target{
                0.0f,
                eye_position,
                0.0f,
            };
            const glm::vec3 up{
                0.0f,
                1.0f,
                0.0f,
            };

            types::UniformCamera camera{
                .world = glm::mat4(1.0f),
                .view = glm::lookAtLH(eye, target, up),
                .projection = glm::perspectiveFovLH(fov, 600.0f, 800.0f, 0.1f, 200.0f),
            };
            camera.mvp = camera.projection * camera.view * camera.world;

            append_bytes(uniform, camera);

            uniforms.emplace(types::UniformCamera::UNIFORM_NAME,
                types::OwnedRenderData{
                    std::move(uniform),
                    sizeof(types::UniformCamera),
                });
        }

        auto&& addon_result = ModelToMesh::to_uniforms_from_addon(model_data, uniforms);
        if (addon_result.is_err()) {
            return std::move(addon_result).unwrap_err();
        }

        return uniforms;
    }

    foundation::VoidResult<RendererError> ModelToMesh::to_uniforms_from_addon(
        const types::ModelData& model_data, Uniforms& uniforms) {
        for (const auto& addon : model_data.addons) {
            if (auto data = std::get_if<types::AddonBones>(&addon)) {
                using LightModel = types::LightModelBones;
                using MediumModel = types::MediumModelBones;
                using HeavyModel = types::HeavyModelBones;

                // ボーンの数に応じて使用するボーン行列を決める
                const auto size = data->size();

                // シェーダー側が対応できていないので
                // 今は仮で中規模モデルを使用する
                if (size < LightModel::CAPACITY && false) {
                    constexpr auto STRIDE = sizeof(LightModel::MatrixType);
                    auto [iter, is_override] = uniforms.emplace(LightModel::UNIFORM_NAME,
                        types::OwnedRenderData{
                            std::vector<std::byte>(sizeof(LightModel)),
                            STRIDE,
                        });
                    auto& [name, bones] = *iter;
                    for (auto i = 0; i < LightModel::CAPACITY; ++i) {
                        bones.update(LightModel::MatrixType{1.0}, i);
                    }
                } else if (size < MediumModel::CAPACITY) {
                    constexpr auto STRIDE = sizeof(MediumModel::MatrixType);
                    auto [iter, is_override] = uniforms.emplace(MediumModel::UNIFORM_NAME,
                        types::OwnedRenderData{
                            std::vector<std::byte>(sizeof(MediumModel)),
                            STRIDE,
                        });
                    auto& [name, bones] = *iter;
                    for (auto i = 0; i < MediumModel::CAPACITY; ++i) {
                        bones.update(MediumModel::MatrixType{1.0}, i);
                    }
                } else if (size < HeavyModel::CAPACITY) {
                    constexpr auto STRIDE = sizeof(HeavyModel::MatrixType);
                    auto [iter, is_override] = uniforms.emplace(HeavyModel::UNIFORM_NAME,
                        types::OwnedRenderData{
                            std::vector<std::byte>(sizeof(HeavyModel)),
                            STRIDE,
                        });
                    auto& [name, bones] = *iter;
                    for (auto i = 0; i < HeavyModel::CAPACITY; ++i) {
                        bones.update(HeavyModel::MatrixType{1.0}, i);
                    }
                } else {
                    foundation::StringBuilder strings;
                    strings.push_back("ボーンの数が多すぎます. 非対応です.");
                    strings.push_back(
                        std::format("読み込んだボーンサイズ: {}, 対応可能な最大ボーンサイズ: {}",
                            size,
                            HeavyModel::CAPACITY));

                    return foundation::Error(RendererError::ConvertError, strings.join("\n"));
                }
            }
        }

        return {};
    }

    foundation::VoidResult<RendererError> ModelToMesh::to_uniforms_from_material(
        const types::Material& material, Uniforms& uniforms, const MeshConfig& config) {
        if (config.uniform_separator == 0) {
            return foundation::Error(
                RendererError::ConvertError, "Uniform alignment must be positive");
        }
        std::vector<std::byte> uniform;
        if (material.name == types::UniformMaterial::UNIFORM_NAME) {
            append_bytes(uniform, types::make_uniform_material(material));
        } else {
            for (const auto& variant : material.variants) {
                std::visit([&](const auto& data) { append_bytes(uniform, data); }, variant);
            }
        }

        // 指定のByte区切りにする
        // 基本16ByteでDirectX12なら256バイト区切り
        const auto stride = uniform.size();
        const auto padding = (config.uniform_separator - stride % config.uniform_separator) %
                             config.uniform_separator;
        const auto buffer_size = stride + padding;
        if (padding != 0) {
            uniform.resize(buffer_size);
        }

        uniforms.emplace(material.name,
            types::OwnedRenderData{
                std::move(uniform),
                static_cast<std::uint32_t>(buffer_size),
            });

        return {};
    }

    foundation::Result<std::vector<types::MeshMaterial>, RendererError>
    ModelToMesh::to_mesh_material(const types::ModelData& model_data, const MeshConfig& config) {
        std::vector<types::MeshMaterial> mesh_materials;

        if (model_data.materials.empty()) {
            auto&& result = ModelToMesh::make_mesh_material_from_empty_material(model_data);
            if (result.is_err()) {
                return std::move(result).unwrap_err();
            }

            mesh_materials.emplace_back(std::move(result).unwrap_mut());
            return mesh_materials;
        }

        std::uint32_t offset = 0;
        const auto has_indices = !std::holds_alternative<std::monostate>(model_data.indices);
        for (const auto& material : model_data.materials) {
            auto&& draw_binding = ModelToMesh::to_draw_binding(material, offset, has_indices);
            if (draw_binding.is_err()) {
                return std::move(draw_binding).unwrap_err();
            }
            offset += material.count;

            auto&& textures = ModelToMesh::to_texture_map(material, model_data.textures);
            if (textures.is_err()) {
                return std::move(textures).unwrap_err();
            }

            Uniforms uniforms;
            auto material_uniforms =
                ModelToMesh::to_uniforms_from_material(material, uniforms, config);
            if (material_uniforms.is_err()) {
                return std::move(material_uniforms).unwrap_err();
            }
            mesh_materials.emplace_back(types::MeshMaterial{
                .draw_binding = std::move(draw_binding).unwrap_mut(),
                .textures = std::move(textures).unwrap_mut(),
                .uniforms = std::make_unique<Uniforms>(std::move(uniforms)),
            });
        }

        return mesh_materials;
    }

    foundation::Result<types::MeshMaterial, RendererError>
    ModelToMesh::make_mesh_material_from_empty_material(const types::ModelData& model_data) {
        if (std::holds_alternative<std::monostate>(model_data.indices)) {
            return types::MeshMaterial{
                .draw_binding = types::DrawBinding{
                    .parameter = types::DrawParameter{
                        .vertex_count = static_cast<std::uint32_t>(model_data.vertices.size()),
                        .instance_count = 1,
                        .first_vertex = 0,
                        .first_instance = 0,
                    }}};
        }

        auto index_count = 0u;
        if (auto indices = std::get_if<std::vector<std::uint8_t>>(&model_data.indices)) {
            index_count = static_cast<std::uint32_t>(indices->size());
        } else if (auto indices = std::get_if<std::vector<std::uint16_t>>(&model_data.indices)) {
            index_count = static_cast<std::uint32_t>(indices->size());
        } else if (auto indices = std::get_if<std::vector<std::uint32_t>>(&model_data.indices)) {
            index_count = static_cast<std::uint32_t>(indices->size());
        } else {
            return foundation::Error(
                RendererError::ConvertError, "インデックスの数が多すぎます. 非対応です");
        }

        return types::MeshMaterial{
            .draw_binding = types::DrawBinding{.parameter = types::DrawIndexedParameter{
                                                   .index_count = index_count,
                                                   .instance_count = 0,
                                                   .first_index = 0,
                                                   .vertex_offset = 0,
                                                   .first_instance = 0,
                                               }}};
    }

    foundation::Result<types::DrawBinding, RendererError> ModelToMesh::to_draw_binding(
        const types::Material& material, const std::uint32_t offset, const bool has_indices) {
        if (!has_indices) {
            return types::DrawBinding{.parameter = types::DrawParameter{
                                          .vertex_count = material.count,
                                          .instance_count = material.instance_count,
                                          .first_vertex = material.first_offset,
                                          .first_instance = material.first_instance_offset,
                                      }};
        }

        return types::DrawBinding{
            .parameter = types::DrawIndexedParameter{
                .index_count = material.count,
                .instance_count = material.instance_count,
                // Material ranges partition the index buffer; indices remain model-relative.
                .first_index = static_cast<std::int32_t>(offset),
                .vertex_offset = 0,
                .first_instance = material.first_instance_offset,
            }};
    }

    foundation::Result<types::MeshMaterial::BindTextureMap, RendererError>
    ModelToMesh::to_texture_map(
        const types::Material& material, const decltype(types::ModelData::textures)& texture_map) {
        types::MeshMaterial::BindTextureMap bind_texture_map;

        for (const auto& texture : material.textures) {
            const auto iter = texture_map.find(texture.path);
            if (iter == texture_map.end()) {
                return foundation::Error(RendererError::ConvertError,
                    std::format("テクスチャデータが見つかりませんでした. path: {}",
                        texture.path.string<char>()));
            }

            if (iter->second == nullptr) {
                return foundation::Error(RendererError::ConvertError,
                    std::format("Texture data is null: {}", texture.path.string()));
            }

            // https://cpprefjp.github.io/reference/unordered_map/unordered_map/emplace.html
            if (!bind_texture_map.emplace(texture.texture_target_name, iter->second).second) {
                return foundation::Error(RendererError::ConvertError,
                    std::format("バインド先のテクスチャは既に存在します. name: {}, path: {}",
                        texture.texture_target_name,
                        texture.path.string<char>()));
            }
            if (!bind_texture_map.emplace(texture.sampler_target_name, iter->second).second) {
                return foundation::Error(RendererError::ConvertError,
                    std::format("バインド先のサンプラーは既に存在します. name: {}, path: {}",
                        texture.sampler_target_name,
                        texture.path.string<char>()));
            }
        }

        if (material.name == types::UniformMaterial::UNIFORM_NAME) {
            const auto bind_default =
                [&](const char* texture_name, const char* sampler_name, std::uint8_t color) {
                    if (!bind_texture_map.contains(texture_name)) {
                        auto texture = make_solid_texture(color);
                        bind_texture_map.emplace(texture_name, texture);
                        bind_texture_map.try_emplace(sampler_name, std::move(texture));
                    }
                };
            // Model shaders sample every slot. Neutral defaults also prevent a material
            // without a texture from inheriting the preceding draw's texture binding.
            bind_default(types::ModelTexture::MODEL_TEXTURE_NAME,
                types::ModelTexture::MODEL_SAMPLER_NAME,
                OPAQUE_WHITE);
            bind_default(types::ModelTexture::TOON_TEXTURE_NAME,
                types::ModelTexture::TOON_SAMPLER_NAME,
                OPAQUE_WHITE);
            const auto uniform = types::make_uniform_material(material);
            bind_default(types::ModelTexture::SPHERE_TEXTURE_NAME,
                types::ModelTexture::SPHERE_SAMPLER_NAME,
                uniform.sphere_add.x != 0.0f ? BLACK : OPAQUE_WHITE);
        }
        return bind_texture_map;
    }
} // namespace enishi::renderer
