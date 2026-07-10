#pragma once
#include <core/system/asset/asset_manager.h>
#include <core/system/render/render_system.h>
#include <core/system/system_scheduler.h>
#include <core/timer/application_timer.h>
#include <platform/renderer/interface_renderer.h>
#include <platform/window/interface_window.h>
#include <ui/ui.h>

namespace enishi {
    constexpr char APPLICATION_NAME[] = "enishi";
    constexpr glm::vec4 CLEAR_COLOR = glm::vec4{0.0f, 0.0f, 0.0f, 0.0f};
    constexpr glm::ivec2 WINDOW_SIZE = glm::ivec2{200, 400};

    class Application {
      private:
        std::unique_ptr<platform::IWindow> root_window;
        std::shared_ptr<platform::IRenderer> renderer;
        std::shared_ptr<ecs::Registory> rsegistory;
        std::shared_ptr<core::RenderSystem> render_system;
        std::shared_ptr<core::AssetManager> asset_manager;
        core::SystemScheduler system_scheduler;
        core::ApplicationTimer app_timer;

      public:
        bool init(void);
        void run(void);

      private:
        bool init_system(void);
        bool init_window(void);
        bool init_renderer(void);

        bool make_render_pass(void);

        bool load(void);
        bool load_models(void);
        bool load_shader(void);
    };
} // namespace enishi
