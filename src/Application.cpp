#include "application.h"
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

        if (!this->load()) {
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

        this->renderer = std::move(renderer.value());

        // this->renderer->create_pipeline();
        types::PipelineDescription{};

        const auto rect = types::ViewportRect{
            .left_top_x = 0.0,
            .left_top_y = 0.0,
            .width = static_cast<float>(WINDOW_SIZE.x),
            .height = static_cast<float>(WINDOW_SIZE.y),
            .min_depth = 0.0,
            .max_depth = 1.0,
        };
        this->renderer->create_viewport(rect);

        //
        {
            const auto rtv_image_desc = types::ImageDescription::make_render_target(
                {window_size.width, window_size.height});
            auto result_image = this->renderer->create_image(rtv_image_desc);
            if (result_image.is_err()) {
                return false;
            }

            const auto image_view_desc = types::ImageViewDescription{};
            auto result_rtv =
                this->renderer->create_render_target_view(result_image.value(), image_view_desc);
            if (result_rtv.is_err()) {
                return false;
            }

            if (const auto& rtv = result_rtv.value().lock()) {
                rtv->set_clear_color(CLEAR_COLOR);
            }
        }

        {
            types::RasterizerDescription description =
                types::RasterizerDescription::default_solid();
            auto result = this->renderer->create_rasterizer(description);
            if (result.is_err()) {
                return false;
            }
        }

        return true;
    }

    bool Application::make_render_pass(void) {
        if (!this->make_model_render_pass()) {
            return false;
        }

        return true;
    }

    bool Application::load(void) {
        if (!this->load_models()) {
            return false;
        }
        if (!this->load_shader()) {
            return false;
        }

        return true;
    }

    bool Application::load_models(void) {
        const auto asset_paths = this->asset_manager->find_models("./assets/models/");
        if (asset_paths.empty()) {
            foundation::Logger::warning("モデルファイルが見つかりません");
        }

        for (const auto& path : asset_paths) {
            const auto result = this->asset_manager->load_asset(path);
            if (result.is_err()) {
                foundation::Logger::error(std::format("load error path: {}, {}",
                    path.string<char>(),
                    result.error().get_message("\n")));
                continue;
            }

            foundation::Logger::info(std::format("loaded path: {}", path.string<char>()));
            const auto handle = result.value();

            const auto opt_model_data = this->asset_manager->get_model_data(handle);
            if (opt_model_data.is_none()) {
                continue;
            }
            const auto& model_data = opt_model_data.unwrap();

            const auto result_mesh = this->renderer->create_mesh(model_data.to_mesh_data());
            if (result_mesh.is_err()) {
                foundation::Logger::error("メッシュの作成に失敗しました");
                continue;
            }
            const auto mesh_handle = result_mesh.value();
        }

        return true;
    }

    bool Application::load_shader(void) {
        const auto asset_paths = this->asset_manager->find_shaders("./assets/shader/");
        if (asset_paths.empty()) {
            foundation::Logger::warning("シェーダーファイルが見つかりません");
        }

        for (const auto& path : asset_paths) {
            const auto result = this->asset_manager->load_asset(path);
            if (result.is_err()) {
                foundation::Logger::error(std::format("load error path: {}, {}",
                    path.string<char>(),
                    result.error().get_message("\n")));
                continue;
            }

            foundation::Logger::info(std::format("loaded path: {}", path.string<char>()));
            const auto handle = result.value();

            const auto opt_model_data = this->asset_manager->get_shader_data(handle);
            if (opt_model_data.is_none()) {
                continue;
            }
            const auto& shader_data = opt_model_data.unwrap();

            const auto result_shader = this->renderer->create_shader(shader_data);
            if (result_shader.is_err()) {
                foundation::Logger::error("シェーダーの作成に失敗しました");
                continue;
            }
            const auto shader_handle = result_shader.value();
        }

        return true;
    }
} // namespace enishi