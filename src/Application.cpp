#include "application.h"
#include <core/system/animation/animation_system.h>
#include <core/system/asset/asset_system.h>
#include <core/system/physics/physics_system.h>
#include <core/system/render/model_render_system.h>
#include <foundation/log/logger.h>
#include <foundation/str/string_builder.h>
#include <platform_impl/physics/physics_config.h>
#include <render_pass/constructor/background/background_render_pass_constructor.h>
// #include <render_pass/constructor/debug/debug_render_pass_constructor.h>
#include <render_pass/constructor/model/model_render_pass_constructor.h>
#include <render_pass/constructor/shadow/shadow_map_render_pass_constructor.h>

#include <physics/bullet3/physics_engine.h>

#include <core/system/asset/shader/shader_data_provider.h>
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
        this->registry = std::make_shared<ecs::Registry>();

        // システムの追加
        auto asset_system = this->system_scheduler.register_system<core::AssetSystem>(50);
        auto animation_system =
            this->system_scheduler.register_system<core::AnimationSystem>(80, this->registry);

        auto physics_engine = std::make_shared<physics::bullet3::PhysicsEngine>(
            std::make_shared<platform_impl::PhysicsWorldConfig>());

        auto physics_system = this->system_scheduler.register_system<core::PhysicsSystem>(
            90, this->registry, physics_engine);

        // ウィンドウの初期化
        const auto root_window = this->init_window();
        if (!bool(root_window)) {
            return false;
        }
        foundation::Logger::info("ウィンドウの初期化に成功しました");

        // レンダラーの初期化
        const auto shared_asset_system = asset_system->get_asset_system();
        const auto renderer = this->init_renderer(root_window, shared_asset_system);
        if (!bool(renderer)) {
            return false;
        }
        foundation::Logger::info("レンダラーの初期化に成功しました");

        this->init_physics(shared_asset_system, physics_engine);

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

    std::shared_ptr<platform::IWindow> Application::init_window(void) {
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

    std::shared_ptr<platform::IRenderer> Application::init_renderer(
        std::shared_ptr<platform::IWindow> root_window,
        std::shared_ptr<platform::IAssetSystem> asset_system) {
        if (!bool(root_window)) {
            return {};
        }

        const auto opt_window_handle = root_window->get_handle();
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

        auto shader_data_provider = std::make_shared<core::ShaderDataProvider>(asset_system);
        const auto render_system = this->system_scheduler.register_system<core::RenderSystem>(
            100, this->registry, renderer, renderer);

        this->orchestra = std::make_unique<render_pass::RenderPassOrchestra>(
            render_system->get_renderer(), shader_data_provider);

        // レンダーパスの作成
        this->orchestra->add_constructor(
            std::make_shared<render_pass::ModelRenderPassConstructor>());
        this->orchestra->add_constructor(
            std::make_shared<render_pass::BackgroundRenderPassConstructor>());
        this->orchestra->add_constructor(
            std::make_shared<render_pass::ShadowMapRenderPassConstructor>());

        auto&& pass_result = this->orchestra->make_render_passes(root_window.get());
        if (pass_result.is_err()) {
            foundation::Logger::error(pass_result.unwrap_err().get_message());
            return {};
        }

        // レンダーパスのセット
        this->orchestra->set_render_passes({
            render_pass::ModelRenderPassConstructor::RENDER_PASS_NAME,
            render_pass::BackgroundRenderPassConstructor::RENDER_PASS_NAME,
            render_pass::ShadowMapRenderPassConstructor::RENDER_PASS_NAME,
        });
        render_system->set_render_passes(this->orchestra->get_passes());

        const auto model_render_pass = this->orchestra->get_render_pass(
            render_pass::ModelRenderPassConstructor::RENDER_PASS_NAME);
        if (model_render_pass.is_none()) {
            foundation::Logger::error("モデル用レンダーパスが見つかりません");
            return {};
        }
        this->system_scheduler.register_system<core::ModelRenderSystem>(
            95, asset_system, renderer, model_render_pass.unwrap());

        return renderer;
    }

    void Application::init_physics(std::shared_ptr<platform::IAssetSystem> asset_system,
        std::shared_ptr<platform::IPhysicsEngine> physics_engine) {
        physics_engine->init_world();
    }
} // namespace enishi