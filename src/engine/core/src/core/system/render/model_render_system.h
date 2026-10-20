#pragma once
#include "../interface_system.h"
#include <ecs/registry.h>
#include <memory>
#include <model_controller/model_controller.h>
#include <platform/asset/interface_asset_system.h>
#include <platform/renderer/interface_render_pass.h>
#include <platform/renderer/interface_renderer.h>
#include <sub_system/model/interface_model_selection.h>
#include <unordered_map>

namespace enishi::core {
    class ModelRenderSystem : public ISystem, public sub_system::IModelSelection {
      private:
        struct LoadedModel {
            types::HandleId entity;
            types::RenderHandle mesh;
            bool visible;
        };

      private:
        std::shared_ptr<platform::IRenderPass> model_render_pass;
        std::shared_ptr<platform::IRenderPass> shadow_render_pass;
        std::shared_ptr<ecs::Registry> registry;
        std::shared_ptr<model_controller::ModelController> model_controller;
        bool is_initial_models_registered;
        std::unordered_map<std::string, LoadedModel> loaded_models;

      public:
        ModelRenderSystem(std::shared_ptr<ecs::Registry> registry,
            std::shared_ptr<platform::IAssetSystem> asset_system,
            std::shared_ptr<platform::IRenderer> renderer,
            std::shared_ptr<platform::IRenderPass> model_render_pass,
            std::shared_ptr<platform::IRenderPass> shadow_render_pass = nullptr);

      public:
        std::vector<sub_system::ModelEntry> get_models(void) const override;
        foundation::VoidResult<sub_system::ModelSelectionError> show_model(
            const std::string& id) override;
        void hide_model(const std::string& id) override;

        bool should_close(void) override;
        void pre_update(void) override;
        void update(const types::DeltaTime& delta_time) override;
        void post_update(void) override;
        void render(void) const override;

      private:
        void remember_model(const std::string& id) const;
    };
} // namespace enishi::core
