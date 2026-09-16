#include "model_render_system.h"
#include <foundation/log/logger.h>
#include <filesystem>

namespace enishi::core {
    namespace {
        const std::filesystem::path MODEL_SEARCH_PATH = "./assets/models";
    }

    ModelRenderSystem::ModelRenderSystem(std::shared_ptr<platform::IAssetSystem> asset_system,
        std::shared_ptr<platform::IRenderer> renderer,
        std::shared_ptr<platform::IRenderPass> model_render_pass)
        : model_render_pass(model_render_pass)
        , model_controller(std::make_shared<model_controller::ModelController>(asset_system,
              std::make_shared<model_controller::ModelRenderDataBuilder>(renderer, asset_system))) {
    }

    bool ModelRenderSystem::should_close(void) {
        return false;
    }

    void ModelRenderSystem::pre_update(void) {
        if (this->is_initial_model_selected) {
            return;
        }

        this->is_initial_model_selected = true;
        this->model_controller->find_model(MODEL_SEARCH_PATH);
        const auto model_names = this->model_controller->get_model_list();
        if (model_names.empty()) {
            foundation::Logger::warning("描画するモデルが見つかりません");
            return;
        }

        // TODO: モデル選択UIを実装するまで、名前順の先頭モデルを自動描画する。
        const auto shader_reflections = this->model_render_pass->get_shader_reflections();
        const auto result = this->model_controller->change_model(
            model_names.front(), {shader_reflections.begin(), shader_reflections.end()});
        if (result.is_err()) {
            foundation::Logger::error(result.unwrap_err().get_message());
            return;
        }

        this->model_render_pass->add_mesh(result.unwrap());
    }

    void ModelRenderSystem::update(const types::DeltaTime& delta_time) {
    }

    void ModelRenderSystem::post_update(void) {
    }

    void ModelRenderSystem::render(void) const {
    }
} // namespace enishi::core