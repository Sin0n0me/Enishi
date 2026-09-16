#pragma once
#include "../interface_system.h"
#include <memory>
#include <model_controller/model_controller.h>
#include <platform/asset/interface_asset_system.h>
#include <platform/renderer/interface_render_pass.h>
#include <platform/renderer/interface_renderer.h>

namespace enishi::core {
    class ModelRenderSystem : public ISystem {
      private:
        std::shared_ptr<platform::IRenderPass> model_render_pass;
        std::shared_ptr<model_controller::ModelController> model_controller;
        bool is_initial_model_selected;

      public:
        ModelRenderSystem(std::shared_ptr<platform::IAssetSystem> asset_system,
            std::shared_ptr<platform::IRenderer> renderer,
            std::shared_ptr<platform::IRenderPass> model_render_pass);

      public:
        bool should_close(void) override;
        void pre_update(void) override;
        void update(const types::DeltaTime& delta_time) override;
        void post_update(void) override;
        void render(void) const override;
    };
} // namespace enishi::core