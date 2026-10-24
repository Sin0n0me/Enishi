#pragma once
#include "settings.h"
#include <collider/collider.h>
#include <core/system/asset/asset_manager.h>
#include <engine_types/input/model_click_settings.h>
#include <core/system/render/render_system.h>
#include <core/system/system_scheduler.h>
#include <core/system/window/window_manager.h>
#include <core/timer/application_timer.h>

#include <sub_system/model/interface_model_selection.h>
#include <sub_system/camera/interface_camera_controller.h>
#include <sub_system/window/interface_window_animation_sync.h>
#include <sub_system/physics/interface_physics_engine.h>
#include <platform/renderer/interface_renderer.h>
#include <platform/window/interface_window.h>
#include <render_pass/render_pass_orchestra.h>

namespace enishi {
    class Application {
      private:
        std::shared_ptr<ecs::Registry> registry;
        std::shared_ptr<collider::Collider> collider;
        std::shared_ptr<types::ModelClickSettings> click_settings;
        std::shared_ptr<sub_system::ICameraController> camera;
        std::shared_ptr<sub_system::ICameraController> display_camera;
        std::shared_ptr<sub_system::IWindowAnimationSync> window_animation_sync;
        std::shared_ptr<platform::IWindow> root_window;
        core::SystemScheduler system_scheduler;
        core::ApplicationTimer app_timer;
        std::unique_ptr<render_pass::RenderPassOrchestra> orchestra;

      public:
        bool init(void);
        void run(void);
        [[nodiscard]] std::shared_ptr<sub_system::ICameraController> get_camera_controller() const;
        [[nodiscard]] std::shared_ptr<sub_system::IWindowAnimationSync>
        get_window_animation_sync() const;

      private:
        std::shared_ptr<platform::IWindow> init_window(void);
        std::shared_ptr<platform::IRenderer> init_renderer(
            std::shared_ptr<platform::IWindow> root_window,
            std::shared_ptr<platform::IAssetSystem> asset_system);
        bool init_physics(std::shared_ptr<sub_system::IPhysicsEngine> physics_engine);
        bool init_ui(std::shared_ptr<platform::IWindow> window,
            const std::shared_ptr<core::RenderSystem>& render_system,
            std::shared_ptr<sub_system::IModelSelection> models);
    };
} // namespace enishi
