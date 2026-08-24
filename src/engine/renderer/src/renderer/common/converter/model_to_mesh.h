#pragma once
#include "../mesh/mesh_data.h"
#include <engine_types/assets/model/model_data.h>
#include <foundation/option/option.h>
#include <foundation/result/result.h>
#include <memory>
#include <platform/renderer/interface_image_view.h>
#include <renderer/errors/errors.h>
#include <vector>

namespace enishi::renderer {
    class ModelToMesh {
      public:
        struct MeshConfig {
            std::uint32_t uniform_separator = 16;
            bool use_camera = true;
        };

      public:
        // 大量のコピーが発生するので頻繁に呼ばないこと
        [[nodiscard]] static foundation::Result<MeshData, RendererError> to_mesh_data(
            const types::ModelData& model_data, MeshConfig&& config = MeshConfig{});

      private:
        using Uniforms = std::unordered_map<std::string, types::OwnedRenderData>;

        [[nodiscard]] static foundation::Result<types::OwnedRenderData, RendererError> to_vertices(
            const types::ModelData& model_data);
        [[nodiscard]] static foundation::Result<types::OwnedRenderData, RendererError> to_indices(
            const types::ModelData& model_data);
        [[nodiscard]] static foundation::Result<Uniforms, RendererError> to_uniforms(
            const types::ModelData& model_data, const MeshConfig& config);
        [[nodiscard]] static foundation::VoidResult<RendererError> to_uniforms_from_addon(
            const types::ModelData& model_data, Uniforms& uniforms);
        [[nodiscard]] static foundation::VoidResult<RendererError> to_uniforms_from_material(
            const types::ModelData& model_data, Uniforms& uniforms, const MeshConfig& config);
        [[nodiscard]] static foundation::Result<std::vector<MeshMaterial>, RendererError>
        to_mesh_material(const types::ModelData& model_data);
        [[nodiscard]] static foundation::Result<MeshMaterial, RendererError>
        make_mesh_material_from_empty_material(const types::ModelData& model_data);
        [[nodiscard]] static foundation::Result<DrawBinding, RendererError> to_draw_binding(
            const types::Material& material, const std::uint32_t offset, const bool has_indices);
        [[nodiscard]] static foundation::Result<MeshMaterial::BindTextureMap, RendererError>
        to_texture_map(const types::Material& material,
            const decltype(types::ModelData::textures)& texture_map);
    };
} // namespace enishi::renderer