#include "application.h"
#include "render_pass_constructor.h"
#include <core/system/animation/animation_system.h>
#include <foundation/log/logger.h>

#include <platform_impl/window/sdl/sdl3_window.h>

#include <renderer/directx/directx11/d3d11_render_initializer.h>
#include <renderer/directx/directx11/d3d11_renderer.h>

int main(void) {
    enishi::Application app;

    if (!app.init()) {
        return -1;
    }

    app.run();

    return 0;
}

namespace enishi {
    bool Application::init(void) {
        if (!this->init_system()) {
            return false;
        }

        if (!this->init_window()) {
            return false;
        }
        if (!this->init_renderer()) {
            return false;
        }

        if (!this->make_render_pass()) {
            return false;
        }

        return true;
    }

    void Application::run(void) {
        const auto init_time = this->app_timer.tick_unclamp();
        foundation::Logger::info(std::format("初期化時間: {:%S}s", init_time.delta_time));

        for (; this->root_window->should_close();) {
            this->root_window->poll_events();

            const auto dt = this->app_timer.tick();

            // 更新
            this->system_scheduler.update(dt);

            // 描画
            const auto& render_graph = this->render_system->get_render_graph();
            this->renderer->submit_render_graph(render_graph);
            this->renderer->present();
        }
    }

    bool Application::init_system(void) {
        this->rsegistory = std::make_shared<ecs::Registory>();

        // システムの追加
        this->asset_manager = this->system_scheduler.register_system<core::AssetManager>(50);
        this->system_scheduler.register_system<core::AnimationSystem>(80, this->rsegistory);
        this->render_system =
            this->system_scheduler.register_system<core::RenderSystem>(100, this->rsegistory);

        return true;
    }

    bool Application::init_window(void) {
        const auto window_size = types::WindowSize{
            .width = WINDOW_SIZE.x,
            .height = WINDOW_SIZE.y,
        };
        this->root_window = std::make_unique<platform_impl::SDL3Window>(APPLICATION_NAME,
            window_size,
            platform::WindowSystem::Windows,
            types::GraphicsAPI::DirectX11);
        if (!bool(this->root_window)) {
            return false;
        }

        if (this->root_window->init().is_err()) {
            return false;
        }

        return true;
    }

    bool Application::init_renderer(void) {
        const auto opt_window_size = this->root_window->get_size();
        if (opt_window_size.is_none()) {
            return false;
        }
        const auto& window_size = opt_window_size.value();

        const auto opt_handle = this->root_window->get_handle();
        if (opt_handle.is_none()) {
            return false;
        }
        const auto& window_handle = opt_handle.value();

        auto initializer = renderer::directx::D3D11RenderInitializer{};
        auto renderer = initializer.init(window_handle, window_size);
        if (renderer.is_err()) {
            return false;
        }

        this->renderer = renderer.value();

        const auto rect = types::ViewportRect{
            .left_top_x = 0.0,
            .left_top_y = 0.0,
            .width = static_cast<float>(WINDOW_SIZE.x),
            .height = static_cast<float>(WINDOW_SIZE.y),
            .min_depth = 0.0,
            .max_depth = 1.0,
        };
        this->renderer->create_viewport(rect);

        return true;
    }

    bool Application::make_render_pass(void) {
        auto result = RenderPassConstructor::make(this->renderer, this->asset_manager);
        if (result.is_err()) {
            return false;
        }
        auto&& constructor = result.value();
        auto pass = constructor.make_model_render_pass();
        if (pass.is_err()) {
            foundation::Logger::error(pass.error().get_message());
            return false;
        }

        this->render_system->add_render_pass("Model", pass.value());

        return true;
    }
} // namespace enishi