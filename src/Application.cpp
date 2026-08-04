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
    constexpr auto INIT_WINDOW_SIZE = types::WindowSize{
        .width = WINDOW_SIZE.x,
        .height = WINDOW_SIZE.y,
    };

    bool Application::init(void) {
        this->rsegistory = std::make_shared<ecs::Registory>();

        // システムの追加
        auto asset_manager = this->system_scheduler.register_system<core::AssetManager>(50);
        auto animation_system =
            this->system_scheduler.register_system<core::AnimationSystem>(80, this->rsegistory);

        // ウィンドウの初期化
        const auto root_window = this->init_window().lock();
        if (!bool(root_window)) {
            return false;
        }

        // レンダラーの初期化
        const auto renderer = this->init_renderer(root_window, asset_manager).lock();
        if (!bool(renderer)) {
            return false;
        }

        return true;
    }

    void Application::run(void) {
        const auto init_time = this->app_timer.tick_unclamp();
        foundation::Logger::info(std::format("初期化時間: {:%S}s", init_time.delta_time));

        for (; !this->system_scheduler.should_close();) {
            const auto dt = this->app_timer.tick();

            // 更新
            this->system_scheduler.pre_update();
            this->system_scheduler.update(dt);
            this->system_scheduler.post_update();

            // 描画
            this->system_scheduler.render();
        }
    }

    std::weak_ptr<platform::IWindow> Application::init_window(void) {
        const auto window_manager = this->system_scheduler.register_system<core::WindowManager>(80,
            std::make_shared<platform_impl::SDL3Window>(APPLICATION_NAME,
                INIT_WINDOW_SIZE,
                platform::WindowSystem::Windows,
                types::GraphicsAPI::DirectX11));

        auto root_window = window_manager->get_root_window().lock();
        if (!bool(root_window)) {
            return {};
        }

        if (root_window->init().is_err()) {
            return {};
        }

        return root_window;
    }

    std::weak_ptr<platform::IRenderer> Application::init_renderer(
        std::weak_ptr<platform::IWindow> root_window,
        std::weak_ptr<assets_system::IAssetSystem> asset_system) {
        auto window = root_window.lock();
        if (!bool(window)) {
            return {};
        }

        const auto opt_window_handle = window->get_handle();
        if (opt_window_handle.is_none()) {
            return {};
        }

        auto initializer = renderer::directx::D3D11RenderInitializer{};
        auto result_renderer = initializer.init(opt_window_handle.unwrap(), INIT_WINDOW_SIZE);
        if (result_renderer.is_err()) {
            return {};
        }

        auto&& renderer = result_renderer.unwrap();

        const auto rect = types::ViewportRect{
            .left_top_x = 0.0,
            .left_top_y = 0.0,
            .width = static_cast<float>(WINDOW_SIZE.x),
            .height = static_cast<float>(WINDOW_SIZE.y),
            .min_depth = 0.0,
            .max_depth = 1.0,
        };

        if (renderer->create_viewport(rect).is_err()) {
            return {};
        }

        const auto render_system = this->system_scheduler.register_system<core::RenderSystem>(
            100, this->rsegistory, renderer, renderer);

        if (!this->make_render_pass(render_system.get(), asset_system)) {
            return {};
        }

        return renderer;
    }

    bool Application::make_render_pass(core::RenderSystem* const render_system,
        std::weak_ptr<assets_system::IAssetSystem> asset_system) {
        auto result = RenderPassConstructor::make(render_system->get_renderer(), asset_system);
        if (result.is_err()) {
            return false;
        }
        auto&& constructor = result.unwrap_mut();

        // パスの作成
        auto&& pass = constructor.make_model_render_pass();
        if (pass.is_err()) {
            foundation::Logger::error(pass.unwrap_err().get_message());
            return false;
        }

        render_system->add_render_pass("Model", std::move(pass).unwrap_mut());

        return true;
    }
} // namespace enishi