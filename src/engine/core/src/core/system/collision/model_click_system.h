#pragma once
#include "model_click_settings.h"
#include <collider/collider.h>
#include <core/system/interface_system.h>
#include <ecs/registry.h>
#include <platform/window/interface_window.h>
#include <sub_system/camera/interface_camera_controller.h>
#include <sub_system/collision/interface_model_click_handler.h>

namespace enishi::core {
    class ModelClickSystem : public ISystem {
      private:
        std::shared_ptr<collider::Collider> collider;
        std::shared_ptr<platform::IWindow> window;
        std::shared_ptr<ecs::Registry> registry;
        std::shared_ptr<ModelClickSettings> settings;
        std::shared_ptr<sub_system::ICameraController> camera;
        std::vector<std::shared_ptr<sub_system::IModelClickHandler>> handlers;
        std::optional<types::ModelClick> pending_click;

      public:
        ModelClickSystem(std::shared_ptr<collider::Collider> collider,
            std::shared_ptr<platform::IWindow> window,
            std::shared_ptr<ecs::Registry> registry,
            std::shared_ptr<ModelClickSettings> settings,
            std::shared_ptr<sub_system::ICameraController> camera);
        void add_handler(std::shared_ptr<sub_system::IModelClickHandler> handler);
        [[nodiscard]] std::optional<types::ModelHit> pick(
            types::ClientMousePosition cursor, bool apply_click_mode = false) const;
        bool should_close(void) override;
        void pre_update(void) override;
        void update(const types::DeltaTime& delta_time) override;
        void post_update(void) override;
        void render(void) const override;
    };
} // namespace enishi::core
