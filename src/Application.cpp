#include "application.h"
#include <script/script_engine.h>
#include <core/system/animation/animation_system.h>
#include <core/system/animation/cursor_pose_config.h>
#include <core/system/camera/camera_controller.h>
#include <core/system/camera/camera_zoom_system.h>
#include <core/system/animation/model_event_motion_handler.h>
#include <core/system/asset/asset_system.h>
#include <core/system/collision/model_collision_system.h>
#include <core/system/collision/model_click_system.h>
#include <core/system/physics/physics_system.h>
#include <core/system/render/model_render_system.h>
#include <core/system/skinning/skinning_system.h>
#include <core/system/window/window_animation_sync.h>
#include <core/system/ui/selection_ui.h>
#include <core/system/ui/ui_system.h>
#include <foundation/log/logger.h>
#include <foundation/str/string_builder.h>
#include <platform_impl/physics/physics_config.h>
#include <platform_impl/ui/ui_factory.h>
#include <render_pass/constructor/background/background_render_pass_constructor.h>
// #include <render_pass/constructor/debug/debug_render_pass_constructor.h>
#include <render_pass/constructor/model/model_render_pass_constructor.h>
#include <render_pass/constructor/shadow/shadow_map_render_pass_constructor.h>

#include <physics/bullet3/physics_engine.h>

#include <core/system/asset/shader/shader_data_provider.h>
#include <platform_impl/renderer/renderer_factory.h>
#include <platform_impl/window/sdl/sdl3_window.h>

int main(void) {
    /*
#if defined(_DEBUG)
    constexpr DWORD kWaitIntervalMs = 100;
    while (!IsDebuggerPresent()) {
        Sleep(kWaitIntervalMs);
    }
    __debugbreak();
#endif
    */

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

    std::shared_ptr<sub_system::ICameraController> Application::get_camera_controller() const {
        return this->camera;
    }

    std::shared_ptr<sub_system::IWindowAnimationSync> Application::get_window_animation_sync() const {
        return this->window_animation_sync;
    }

    bool Application::init(void) {
        this->registry = std::make_shared<ecs::Registry>();
        this->click_settings = std::make_shared<core::ModelClickSettings>();
        this->camera = std::make_shared<core::CameraController>();

        // システムの追加
        auto asset_system = this->system_scheduler.register_system<core::AssetSystem>(50);
        auto animation_system =
            this->system_scheduler.register_system<core::AnimationSystem>(80, this->registry);

        auto physics_engine = std::make_shared<physics::bullet3::PhysicsEngine>(
            std::make_shared<platform_impl::PhysicsWorldConfig>());
        auto skinning_system = this->system_scheduler.register_system<core::SkinningSystem>(
            85, this->registry, physics_engine);

        auto physics_system = this->system_scheduler.register_system<core::PhysicsSystem>(
            90, this->registry, physics_engine, skinning_system);
        constexpr core::Priority COLLISION_PRIORITY = 96;
        auto collider = std::make_shared<collider::Collider>();
        this->system_scheduler.register_system<core::ModelCollisionSystem>(
            COLLISION_PRIORITY, this->registry, collider);

        // ウィンドウの初期化
        const auto root_window = this->init_window();
        if (!bool(root_window)) {
            return false;
        }
        foundation::Logger::info("ウィンドウの初期化に成功しました");
        skinning_system->set_cursor_window(root_window);
        // Final bone matrices are ready after physics (90); camera uploads occur at 100.
        constexpr core::Priority WINDOW_ANIMATION_PRIORITY = 95;
        const auto window_sync = this->system_scheduler.register_system<core::WindowAnimationSync>(
            WINDOW_ANIMATION_PRIORITY, this->registry, root_window, this->camera);
        this->window_animation_sync = window_sync;
        this->display_camera = window_sync;
        skinning_system->set_camera(this->display_camera);
        core::CameraZoomConfig zoom_config;
        auto zoom_settings = core::load_camera_zoom_config("config/camera.cfg");
        if (zoom_settings.is_err()) {
            foundation::Logger::warning(zoom_settings.unwrap_err().get_message());
        } else {
            zoom_config = std::move(zoom_settings).unwrap();
        }
        // Polling is priority 80; update the camera before click picking in pre_update (97).
        constexpr core::Priority CAMERA_ZOOM_PRIORITY = 81;
        this->system_scheduler.register_system<core::CameraZoomSystem>(
            CAMERA_ZOOM_PRIORITY, root_window, this->camera, zoom_config);
        auto cursor_config = core::load_cursor_pose_config("config/cursor_tracking.cfg");
        if (cursor_config.is_err()) {
            foundation::Logger::warning(cursor_config.unwrap_err().get_message());
        } else {
            skinning_system->set_cursor_config(std::move(cursor_config).unwrap());
        }

        constexpr core::Priority CLICK_PRIORITY = 97;
        const auto clicks = this->system_scheduler.register_system<core::ModelClickSystem>(
            CLICK_PRIORITY, collider, root_window, this->registry, this->click_settings,
            this->display_camera);
        auto motion_rules = core::load_model_event_motions(
            "config/model_event_motions.cfg", this->click_settings.get());
        if (motion_rules.is_err()) {
            foundation::Logger::warning(motion_rules.unwrap_err().get_message());
        } else {
            clicks->add_handler(std::make_shared<core::ModelEventMotionHandler>(
                this->registry, animation_system, std::move(motion_rules).unwrap(),
                this->click_settings, physics_engine->get_shared_world()));
        }
        root_window->set_model_drag_handler(
            [weak = std::weak_ptr<core::SkinningSystem>(skinning_system)](types::ClientMousePosition cursor) {
                const auto system = weak.lock();
                return system != nullptr && system->begin_model_drag(cursor);
            },
            [weak = std::weak_ptr<core::SkinningSystem>(skinning_system)]() {
                const auto system = weak.lock();
                if (system != nullptr) {
                    system->end_model_drag();
                }
            });
        root_window->set_interactive_hit_test(
            [weak = std::weak_ptr<core::ModelClickSystem>(clicks)](types::ClientMousePosition cursor) {
                const auto system = weak.lock();
                return system != nullptr && system->pick(cursor).has_value();
            });

        // レンダラーの初期化
        const auto shared_asset_system = asset_system->get_asset_system();
        const auto renderer = this->init_renderer(root_window, shared_asset_system);
        if (!bool(renderer)) {
            return false;
        }
        foundation::Logger::info("レンダラーの初期化に成功しました");

        if (!this->init_physics(physics_engine)) {
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

    std::shared_ptr<platform::IWindow> Application::init_window(void) {
        const auto window_manager = this->system_scheduler.register_system<core::WindowManager>(80,
            std::make_shared<platform_impl::SDL3Window>(APPLICATION_NAME,
                INIT_WINDOW_SIZE,
                platform::WindowSystem::Windows,
                platform_impl::render_backend_description().api));

        auto root_window = window_manager->get_root_window().lock();
        if (!bool(root_window)) {
            foundation::Logger::error("ルートウィンドウを取得できませんでした");
            return {};
        }

        auto init_result = root_window->init();
        if (init_result.is_err()) {
            foundation::Logger::error(init_result.unwrap_err().get_message());
            return {};
        }

        return root_window;
    }

    std::shared_ptr<platform::IRenderer> Application::init_renderer(
        std::shared_ptr<platform::IWindow> root_window,
        std::shared_ptr<platform::IAssetSystem> asset_system) {
        if (!bool(root_window)) {
            foundation::Logger::error("レンダラー初期化に必要なウィンドウがありません");
            return {};
        }

        auto result_renderer = platform_impl::create_renderer(root_window, INIT_WINDOW_SIZE);
        if (result_renderer.is_err()) {
            foundation::Logger::error(result_renderer.unwrap_err().get_message());
            return {};
        }

        const auto& instance = result_renderer.unwrap();
        const auto& renderer = instance.renderer;

        const auto rect = types::ViewportRect{
            .left_top_x = 0.0,
            .left_top_y = 0.0,
            .width = static_cast<float>(WINDOW_SIZE.x),
            .height = static_cast<float>(WINDOW_SIZE.y),
            .min_depth = 0.0,
            .max_depth = 1.0,
        };

        auto viewport_result = renderer->create_viewport(rect);
        if (viewport_result.is_err()) {
            foundation::Logger::error(viewport_result.unwrap_err().get_message());
            return {};
        }

        auto shader_data_provider = std::make_shared<core::ShaderDataProvider>(
            asset_system, platform_impl::shader_directory());
        const auto render_system = this->system_scheduler.register_system<core::RenderSystem>(
            100, this->registry, renderer, instance.encoder);

        this->orchestra = std::make_unique<render_pass::RenderPassOrchestra>(
            render_system->get_renderer(), shader_data_provider);

        // レンダーパスの作成
        const auto backend = platform_impl::render_backend_description();
        this->orchestra->add_constructor(
            std::make_shared<render_pass::ModelRenderPassConstructor>(backend));
        this->orchestra->add_constructor(
            std::make_shared<render_pass::BackgroundRenderPassConstructor>(backend));
        this->orchestra->add_constructor(
            std::make_shared<render_pass::ShadowMapRenderPassConstructor>(backend));

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
        render_system->set_camera(this->display_camera);

        const auto model_render_pass = this->orchestra->get_render_pass(
            render_pass::ModelRenderPassConstructor::RENDER_PASS_NAME);
        if (model_render_pass.is_none()) {
            foundation::Logger::error("モデル用レンダーパスが見つかりません");
            return {};
        }
        const auto models = this->system_scheduler.register_system<core::ModelRenderSystem>(
            95, this->registry, asset_system, renderer, model_render_pass.unwrap(),
            this->orchestra->get_render_pass(render_pass::ShadowMapRenderPassConstructor::RENDER_PASS_NAME)
                .unwrap_or(nullptr));

        if (!this->init_ui(root_window, render_system, models)) {
            return {};
        }

        return renderer;
    }

    bool Application::init_ui(std::shared_ptr<platform::IWindow> window,
        const std::shared_ptr<core::RenderSystem>& render_system,
        std::shared_ptr<sub_system::IModelSelection> models) {
        const auto result = platform_impl::create_ui(window, render_system->get_renderer());
        if (result.is_err()) {
            foundation::Logger::error(result.unwrap_err().get_message());
            return false;
        }
        const auto ui = result.unwrap();
        constexpr core::Priority UI_PRIORITY = 99;
        const auto scripts = std::make_shared<script::ScriptEngine>();
        const auto render_settings = std::make_shared<sub_system::RenderSettings>();
        render_system->set_settings(render_settings);
        const auto selection = std::make_shared<core::SelectionUI>(
            std::move(models), this->click_settings, scripts, render_settings);
        this->system_scheduler.register_system<core::UISystem>(
            UI_PRIORITY,
            ui,
            [selection](sub_system::IUI& ui) { selection->update(ui); },
            [selection]() { return selection->should_close(); });
        render_system->set_ui(ui);
        return true;
    }

    bool Application::init_physics(std::shared_ptr<sub_system::IPhysicsEngine> physics_engine) {
        auto init_result = physics_engine->init_world();
        if (init_result.is_err()) {
            foundation::Logger::error(init_result.unwrap_err().get_message());
            return false;
        }

        return true;
    }
} // namespace enishi
