#pragma once
#include "settings.h"
#include <core/system/asset/asset_manager.h>
#include <core/system/render/render_system.h>
#include <core/system/system_scheduler.h>
#include <core/system/window/window_manager.h>
#include <core/timer/application_timer.h>
#include <platform/renderer/interface_renderer.h>
#include <platform/window/interface_window.h>
#include <ui/ui.h>

namespace enishi {
    class Application {
      private:
        std::shared_ptr<ecs::Registory> rsegistory;
        core::SystemScheduler system_scheduler;
        core::ApplicationTimer app_timer;

      public:
        bool init(void);
        void run(void);

      private:
        std::shared_ptr<platform::IWindow> init_window(void);
        std::shared_ptr<platform::IRenderer> init_renderer(
            std::shared_ptr<platform::IWindow> root_window,
            std::shared_ptr<assets_system::IAssetSystem> asset_system);
    };
} // namespace enishi
