#include "Application.h"
#include <algorithm>
#include <array>
#include <limits>
#include <effect/crack/crack_effect.h>
#include <platform_impl/effect/crack_surface.h>
#include <core/config/application_settings.h>
#include <core/system/animation/animation_system.h>
#include <core/system/animation/bone_name_config.h>
#include <core/system/animation/model_event_motion_handler.h>
#include <core/system/animation/morph_name_config.h>
#include <core/system/asset/asset_system.h>
#include <core/system/camera/camera_controller.h>
#include <core/system/camera/camera_zoom_system.h>
#include <core/system/collision/model_click_system.h>
#include <core/system/collision/model_collision_system.h>
#include <core/system/debug/obb_debug_system.h>
#include <core/system/physics/physics_system.h>
#include <core/system/render/model_render_system.h>
#include <core/system/skinning/skinning_system.h>
#include <core/system/ui/ui_system.h>
#include <core/system/window/window_animation_sync.h>
#include <debug/log/logger.h>
#include <foundation/str/string_builder.h>
#include <physics/common/physics_config.h>
#include <platform_impl/ui/ui_factory.h>
#include <render_pass/constructor/background/background_render_pass_constructor.h>
#include <script/script_engine.h>
#include <ui/selection/selection_ui.h>
#include <utility>
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
    Application::~Application() {
        if (this->effects != nullptr) {
            static_cast<void>(this->effects->stop_all());
        }
    }
    foundation::Result<effect::EffectId, effect::EffectError> Application::play_screen_crack(
        const effect::CrackSettings& settings) {
        if (this->effects == nullptr || this->effect_shaders == nullptr) {
            return foundation::Error(
                effect::EffectError::StartFailed, "Application renderer is not initialized");
        }
        return this->effects->play(std::make_unique<effect::CrackEffect>(
            settings, platform_impl::create_crack_surface(this->effect_shaders)));
    }
    foundation::Result<effect::EffectId, effect::EffectError> Application::play_screen_crack() {
        if (this->root_window == nullptr) {
            return foundation::Error(
                effect::EffectError::StartFailed, "Model window is unavailable");
        }
        const auto position = this->root_window->get_position();
        const auto size = this->root_window->get_size();
        if (position.is_none() || size.is_none()) {
            return foundation::Error(
                effect::EffectError::StartFailed, "Model window bounds are unavailable");
        }
        constexpr int CENTER_DIVISOR = 2;
        const effect::CrackImpact center{position.unwrap().x + size.unwrap().width / CENTER_DIVISOR,
            position.unwrap().y + size.unwrap().height / CENTER_DIVISOR};
        return this->play_screen_crack_at(
            std::nullopt, effect::EffectTarget::display_at(center.x, center.y));
    }
    foundation::Result<effect::EffectId, effect::EffectError> Application::play_screen_crack_at(
        effect::CrackImpact impact) {
        return this->play_screen_crack_at(
            impact, effect::EffectTarget::display_at(impact.x, impact.y));
    }
    foundation::Result<effect::EffectId, effect::EffectError> Application::play_screen_crack_at(
        std::optional<effect::CrackImpact> impact, const effect::EffectTarget& target) {
        if (this->prepared_crack == nullptr || this->effects == nullptr) {
            return foundation::Error(effect::EffectError::StartFailed,
                "Default crack effect could not be prepared during initialization");
        }
        if (this->effects->is_playing(this->screen_crack_id)) {
            auto stopped = this->effects->stop(this->screen_crack_id);
            if (stopped.is_err()) {
                return std::move(stopped).take_err();
            }
        }
        this->prepared_crack->set_impact(impact);
        this->prepared_crack->set_target(target);
        auto started = this->effects->play_shared(this->prepared_crack);
        if (started.is_ok()) {
            this->screen_crack_id = started.unwrap();
        }
        return started;
    }
    std::shared_ptr<effect::IEffectPlayer> Application::get_effect_player() const {
        return this->effects;
    }

    std::shared_ptr<bypass_shell::ShellMonitor> Application::get_shell_monitor() const {
        return this->shell_monitor;
    }
    constexpr auto INIT_WINDOW_SIZE = types::WindowSize{
        .width = WINDOW_SIZE.x,
        .height = WINDOW_SIZE.y,
    };

    std::shared_ptr<sub_system::ICameraController> Application::get_camera_controller() const {
        return this->camera;
    }

    std::shared_ptr<sub_system::IWindowAnimationSync>
    Application::get_window_animation_sync() const {
        return this->window_animation_sync;
    }

    bool Application::init(void) {
        this->effects = std::make_shared<effect::EffectManager>();
        this->shell_monitor = std::make_shared<bypass_shell::ShellMonitor>();
        this->registry = std::make_shared<ecs::Registry>();
        this->debug_playback = std::make_shared<debug::PlaybackControl>();
        this->click_settings = std::make_shared<types::ModelClickSettings>();
        this->camera = std::make_shared<core::CameraController>();

        // システムの追加
        auto asset_system = this->system_scheduler.register_system<core::AssetSystem>(50);
        auto animation_system =
            this->system_scheduler.register_system<core::AnimationSystem>(80, this->registry);

        auto physics_engine = std::make_shared<physics::bullet3::PhysicsEngine>(
            std::make_shared<physics::PhysicsWorldConfig>());
        auto skinning_system = this->system_scheduler.register_system<core::SkinningSystem>(
            85, this->registry, physics_engine);

        auto physics_system = this->system_scheduler.register_system<core::PhysicsSystem>(
            90, this->registry, physics_engine, skinning_system);
        animation_system->set_debug_playback(this->debug_playback);
        skinning_system->set_debug_playback(this->debug_playback);
        physics_system->set_debug_playback(this->debug_playback);
        constexpr core::Priority COLLISION_PRIORITY = 96;
        this->collider = std::make_shared<collider::Collider>();
        this->system_scheduler.register_system<core::ModelCollisionSystem>(
            COLLISION_PRIORITY, this->registry, this->collider);

        // ウィンドウの初期化
        this->root_window = this->init_window();
        const auto& root_window = this->root_window;
        if (!bool(root_window)) {
            return false;
        }
        debug::Logger::info("ウィンドウの初期化に成功しました");
        skinning_system->set_cursor_window(root_window);
        // Final bone matrices are ready after physics (90); camera uploads occur at 100.
        constexpr core::Priority WINDOW_ANIMATION_PRIORITY = 95;
        const auto window_sync = this->system_scheduler.register_system<core::WindowAnimationSync>(
            WINDOW_ANIMATION_PRIORITY, this->registry, root_window, this->camera);
        this->window_animation_sync = window_sync;
        this->display_camera = window_sync;
        skinning_system->set_camera(this->display_camera);
        core::ApplicationSettings settings;
        auto loaded_settings = core::load_application_settings("config/settings.cfg");
        if (loaded_settings.is_err()) {
            debug::Logger::warning(loaded_settings.unwrap_err().get_message());
        } else {
            settings = std::move(loaded_settings).unwrap();
        }
        // Polling is priority 80; update the camera before click picking in pre_update (97).
        constexpr core::Priority CAMERA_ZOOM_PRIORITY = 81;
        this->system_scheduler.register_system<core::CameraZoomSystem>(
            CAMERA_ZOOM_PRIORITY, root_window, this->camera, settings.camera);
        skinning_system->set_cursor_config(settings.cursor_tracking);
        this->click_settings->push_states = std::move(settings.click.push_states);

        auto bone_names = core::load_bone_name_config("config/bone_names.toml");
        if (bone_names.is_err()) {
            debug::Logger::warning(bone_names.unwrap_err().get_message());
        } else {
            this->click_bone_names = bone_names.unwrap();
            skinning_system->set_bone_names(std::move(bone_names).unwrap());
        }

        auto morph_names = core::load_morph_name_config("config/morph_names.toml");
        if (morph_names.is_err()) {
            debug::Logger::warning(morph_names.unwrap_err().get_message());
        } else {
            animation_system->set_morph_names(std::move(morph_names).unwrap());
        }

        constexpr core::Priority CLICK_PRIORITY = 97;
        const auto clicks =
            this->system_scheduler.register_system<core::ModelClickSystem>(CLICK_PRIORITY,
                this->collider,
                root_window,
                this->registry,
                this->click_settings,
                this->display_camera);
        this->model_clicks = clicks;
        clicks->add_handler(std::make_shared<core::ModelEventMotionHandler>(this->registry,
            animation_system,
            std::move(settings.model_events),
            this->click_settings,
            physics_engine->get_shared_world()));
        root_window->set_model_drag_handler(
            [weak = std::weak_ptr<core::SkinningSystem>(skinning_system)](
                types::ClientMousePosition cursor) {
                const auto system = weak.lock();
                return system != nullptr && system->begin_model_drag(cursor);
            },
            [weak = std::weak_ptr<core::SkinningSystem>(skinning_system)]() {
                const auto system = weak.lock();
                if (system != nullptr) {
                    system->end_model_drag();
                }
            });
        root_window->set_interactive_hit_test([weak = std::weak_ptr<core::ModelClickSystem>(
                                                   clicks)](types::ClientMousePosition cursor) {
            const auto system = weak.lock();
            return system != nullptr && system->pick(cursor).has_value();
        });

        // レンダラーの初期化
        const auto shared_asset_system = asset_system->get_asset_system();
        const auto renderer = this->init_renderer(root_window, shared_asset_system);
        if (!bool(renderer)) {
            return false;
        }
        debug::Logger::info("レンダラーの初期化に成功しました");

        if (!this->init_physics(physics_engine)) {
            return false;
        }

        this->prepared_crack = std::make_shared<effect::CrackEffect>(
            effect::CrackSettings{}, platform_impl::create_crack_surface(this->effect_shaders));
        auto prepared = this->prepared_crack->prepare();
        if (prepared.is_err()) {
            debug::Logger::warning(prepared.unwrap_err().get_message());
            this->prepared_crack.reset();
        }
        auto shell_started = this->shell_monitor->start();
        if (!shell_started.has_value()) {
            debug::Logger::warning(std::move(shell_started.error()));
        }

        return true;
    }

    void Application::run(void) {
        const auto init_time = this->app_timer.tick_unclamp();
        debug::Logger::info(std::format("初期化時間: {:%S}s", init_time.delta_time));

        for (; !this->root_window->should_close();) {
            const auto dt = this->app_timer.tick();
            this->shell_monitor->poll();

            // 更新
            this->system_scheduler.pre_update();
            this->update_screen_crack_click();
            this->system_scheduler.update(dt);
            this->system_scheduler.post_update();
            auto updated_effects = this->effects->update(dt);
            if (updated_effects.is_err()) {
                debug::Logger::warning(updated_effects.unwrap_err().get_message());
            } else {
                for (const auto& failure : updated_effects.unwrap()) {
                    debug::Logger::warning(std::format(
                        "Effect {} failed: {}", failure.id.value, failure.error.get_message()));
                }
            }

            // 描画
            this->system_scheduler.render();
        }
        auto stopped = this->effects->stop_all();
        if (stopped.is_err()) {
            debug::Logger::warning(stopped.unwrap_err().get_message());
        }
    }

    void Application::update_screen_crack_click() {
        const auto input = this->root_window->get_input();
        const auto position = this->root_window->get_position();
        if (this->model_clicks == nullptr || input.is_none() || position.is_none() ||
            !input.unwrap()->is_mouse_button_pressed(types::MouseButton::Left)) {
            return;
        }
        const auto cursor =
            input.unwrap()->get_mouse_press_position(types::MouseButton::Left, position.unwrap());
        // This preview trigger uses the displayed pose, independently of physics push filtering.
        const auto hit = this->model_clicks->pick(cursor, false);
        if (!hit.has_value() ||
            hit->model > (std::numeric_limits<types::HandleId::HandleType>::max)()) {
            return;
        }
        const auto model = this->registry->get<component::ModelComponent>(
            types::HandleId{static_cast<types::HandleId::HandleType>(hit->model)});
        if (model.is_none() || !model.unwrap().visible) {
            return;
        }
        const auto& data = model.unwrap();
        constexpr std::array LOWER_BODY_ROLES{animation::BoneRole::Hips,
            animation::BoneRole::LeftUpperLeg,
            animation::BoneRole::LeftLowerLeg,
            animation::BoneRole::LeftFoot,
            animation::BoneRole::RightUpperLeg,
            animation::BoneRole::RightLowerLeg,
            animation::BoneRole::RightFoot};
        std::array<types::BoneIndex, LOWER_BODY_ROLES.size()> lower_body_bones{};
        for (std::size_t index = 0; index < LOWER_BODY_ROLES.size(); ++index) {
            lower_body_bones[index] =
                this->click_bone_names.find(data.bone_names, LOWER_BODY_ROLES[index]);
        }
        const auto spine = this->click_bone_names.find(data.bone_names, animation::BoneRole::Spine);
        auto bone = hit->bone;
        // Spine descendants must not match hips; bound traversal also rejects cyclic hierarchies.
        for (std::size_t visited = 0; visited < data.bone_node.size(); ++visited) {
            if (!(bone < data.bone_node.size()) || bone == spine) {
                return;
            }
            if (std::find(lower_body_bones.begin(), lower_body_bones.end(), bone) !=
                lower_body_bones.end()) {
                const auto point = types::to_screen(cursor, position.unwrap());
                auto started = this->play_screen_crack_at(effect::CrackImpact{point.x, point.y});
                if (started.is_err()) {
                    debug::Logger::warning(started.unwrap_err().get_message());
                }
                return;
            }
            bone = data.bone_node[bone].parent;
        }
    }

    std::shared_ptr<platform::IWindow> Application::init_window(void) {
        const auto window = std::make_shared<platform_impl::SDL3Window>(APPLICATION_NAME,
            INIT_WINDOW_SIZE,
            platform::WindowSystem::Windows,
            platform_impl::render_backend_description().api);
        window->set_debug_playback(this->debug_playback);
        const auto window_manager =
            this->system_scheduler.register_system<core::WindowManager>(80, window);

        auto root_window = window_manager->get_root_window().lock();
        if (!bool(root_window)) {
            debug::Logger::error("ルートウィンドウを取得できませんでした");
            return {};
        }

        auto init_result = root_window->init();
        if (init_result.is_err()) {
            debug::Logger::error(init_result.unwrap_err().get_message());
            return {};
        }

        return root_window;
    }

    std::shared_ptr<platform::IRenderer> Application::init_renderer(
        std::shared_ptr<platform::IWindow> root_window,
        std::shared_ptr<platform::IAssetSystem> asset_system) {
        if (!bool(root_window)) {
            debug::Logger::error("レンダラー初期化に必要なウィンドウがありません");
            return {};
        }

        auto result_renderer = platform_impl::create_renderer(root_window, INIT_WINDOW_SIZE);
        if (result_renderer.is_err()) {
            debug::Logger::error(result_renderer.unwrap_err().get_message());
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
            debug::Logger::error(viewport_result.unwrap_err().get_message());
            return {};
        }

        auto shader_data_provider = std::make_shared<core::ShaderDataProvider>(
            asset_system, platform_impl::shader_directory());
        this->effect_shaders = shader_data_provider;
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
            debug::Logger::error(pass_result.unwrap_err().get_message());
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
            debug::Logger::error("モデル用レンダーパスが見つかりません");
            return {};
        }
        const auto models = this->system_scheduler.register_system<core::ModelRenderSystem>(95,
            this->registry,
            asset_system,
            renderer,
            model_render_pass.unwrap(),
            this->orchestra
                ->get_render_pass(render_pass::ShadowMapRenderPassConstructor::RENDER_PASS_NAME)
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
            debug::Logger::error(result.unwrap_err().get_message());
            return false;
        }
        const auto ui = result.unwrap();
        const auto overlay = std::dynamic_pointer_cast<debug::IDebugOverlay>(ui);
        if (overlay == nullptr) {
            debug::Logger::error("UI does not support debug overlays");
            return false;
        }
        // Collision bounds are finalized at 96; the UI frame is finalized at 99.
        constexpr core::Priority OBB_DEBUG_PRIORITY = 98;
        this->system_scheduler.register_system<core::OBBDebugSystem>(OBB_DEBUG_PRIORITY,
            this->registry,
            this->collider,
            window,
            this->display_camera,
            overlay,
            this->debug_playback);
        constexpr core::Priority UI_PRIORITY = 99;
        const auto scripts = std::make_shared<script::ScriptEngine>();
        const auto render_settings = std::make_shared<sub_system::RenderSettings>();
        render_system->set_settings(render_settings);
        const auto selection = std::make_shared<ui::SelectionUI>(
            std::move(models), this->click_settings, scripts, render_settings, this->shell_monitor);
        this->system_scheduler.register_system<core::UISystem>(
            UI_PRIORITY, ui, [selection, window](sub_system::IUI& ui) {
                selection->update(ui);
                if (selection->is_exit_requested()) {
                    window->close();
                }
            });
        render_system->set_ui(ui);
        return true;
    }

    bool Application::init_physics(std::shared_ptr<sub_system::IPhysicsEngine> physics_engine) {
        auto init_result = physics_engine->init_world();
        if (init_result.is_err()) {
            debug::Logger::error(init_result.unwrap_err().get_message());
            return false;
        }

        return true;
    }
} // namespace enishi
