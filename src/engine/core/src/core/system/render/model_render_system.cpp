#include "model_render_system.h"
#include <filesystem>
#include <foundation/log/logger.h>
#include <utility>
#include <vector>

namespace enishi::core {
    namespace {
        const std::filesystem::path MODEL_SEARCH_PATH = "./assets/models";
    }

    ModelRenderSystem::ModelRenderSystem(std::shared_ptr<ecs::Registry> registry,
        std::shared_ptr<platform::IAssetSystem> asset_system,
        std::shared_ptr<platform::IRenderer> renderer,
        std::shared_ptr<platform::IRenderPass> model_render_pass)
        : model_render_pass(model_render_pass)
        , registry(std::move(registry))
        , model_controller(std::make_shared<model_controller::ModelController>(asset_system,
              std::make_shared<model_controller::ModelRenderDataBuilder>(renderer, asset_system)))
        , is_initial_models_registered(false) {
    }

    bool ModelRenderSystem::should_close(void) {
        return false;
    }

    void ModelRenderSystem::pre_update(void) {
        if (this->is_initial_models_registered) {
            return;
        }

        this->is_initial_models_registered = true;
        this->model_controller->find_model(MODEL_SEARCH_PATH);
        const auto model_names = this->model_controller->get_model_list();
        if (model_names.empty()) {
            foundation::Logger::warning("描画するモデルが見つかりません");
            return;
        }

        // TODO: モデル選択UIを実装するまで、名前順の先頭モデルを自動描画する。
        const auto shader_reflections = this->model_render_pass->get_shader_reflections();
        const auto shader_reflection_list =
            std::vector<types::RenderHandle>{shader_reflections.begin(), shader_reflections.end()};
        for (const auto& model_name : model_names) {
            const auto result = this->model_controller->make_model(model_name, shader_reflection_list);
            if (result.is_err()) {
                foundation::Logger::error(result.unwrap_err().get_message());
                continue;
            }

            const auto entity = this->registry->create();
            auto model_component = result.unwrap();
            auto insert_result = this->registry->insert(entity, std::move(model_component));
            if (insert_result.is_err()) {
                foundation::Logger::error(insert_result.unwrap_err().get_message());
                this->registry->destroy(entity);
                continue;
            }

            this->model_render_pass->add_mesh(insert_result.unwrap_mut().render_handle);
        }
    }

    void ModelRenderSystem::update(const types::DeltaTime& delta_time) {
    }

    void ModelRenderSystem::post_update(void) {
    }

    void ModelRenderSystem::render(void) const {
    }
} // namespace enishi::core
