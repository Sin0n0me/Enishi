#include "model_render_data_builder.h"
#include <foundation/log/logger.h>

namespace enishi::model_controller {
    ModelRenderDataBuilder::ModelRenderDataBuilder(std::shared_ptr<platform::IRenderer> renderer,
        std::shared_ptr<platform::IAssetSystem> asset_system) noexcept
        : renderer(renderer)
        , asset_system(asset_system) {
    }

    foundation::Result<types::RenderHandle, BuildError> ModelRenderDataBuilder::build(
        const types::AssetHandle model_handle,
        const std::vector<types::RenderHandle>& shader_reflections) noexcept {
        // 読み込み中であれば内部で待機し、完了したデータを返す(asset_systemの実装依存)
        const auto opt_model_data = this->asset_system->get_asset<types::AssetModelData>(model_handle);
        if (opt_model_data.is_none()) {
            return foundation::Error(BuildError::AssetNotReady);
        }
        const auto& model_data = opt_model_data.unwrap();

        // モデルが参照するテクスチャを先に読み込み要求しておく
        // (テクスチャ自体の完了待ちはマテリアル参照時、実際に必要になったタイミングで行われる想定)
        this->request_textures(model_data);

        auto mesh_handle = this->renderer->create_mesh(*model_data, shader_reflections);
        if (mesh_handle.is_err()) {
            foundation::Logger::warning(mesh_handle.unwrap_err().get_message());
            return foundation::Error(BuildError::CreateMeshFailed);
        }

        return mesh_handle.unwrap();
    }

    void ModelRenderDataBuilder::request_textures(
        const types::AssetModelData& model_data) noexcept {
        for (const auto& material : model_data->materials) {
            for (const auto& material_texture : material.textures) {
                const auto texture_handle = this->asset_system->load_asset(material_texture.path);
                if (texture_handle.is_err()) {
                    foundation::Logger::warning(texture_handle.unwrap_err().get_message());
                }
            }
        }
    }
} // namespace enishi::model_controller
