#include "render_system.h"
#include <component/animation_component.h>
#include <component/model_component.h>
#include <foundation/log/logger.h>

namespace enishi::core {
    RenderSystem::RenderSystem(std::shared_ptr<ecs::Registry> registry,
        std::shared_ptr<platform::IRenderer> renderer,
        std::shared_ptr<platform::IRenderCommandEncoder> encoder)
        : registry(registry)
        , renderer(renderer)
        , encoder(encoder) {
    }

    void RenderSystem::set_render_passes(
        std::vector<std::shared_ptr<platform::IRenderPass>>&& render_passes) {
        this->render_passes = std::move(render_passes);
    }

    bool enishi::core::RenderSystem::should_close(void) {
        return false;
    }

    void enishi::core::RenderSystem::pre_update(void) {
    }

    void RenderSystem::update(const types::DeltaTime& delta_time) {
        // 各パイプラインに応じた描画コマンド実行
        for (const auto& pass : this->render_passes) {
            pass->update();
        }
    }

    void enishi::core::RenderSystem::post_update(void) {
    }

    void enishi::core::RenderSystem::render(void) const {
        this->submit_render_graph();
        this->present();
    }

    void RenderSystem::submit_render_graph(void) const {
        // 描画前初期化
        this->encoder->setup_viewports();
        this->encoder->setup_views();

        // 各パイプラインに応じた描画コマンド実行
        for (const auto& pass : this->render_passes) {
            const auto render_target = pass->get_render_target();
            for (const auto& command : pass->get_commands()) {
                switch (command.handle.type) {
                    case types::RenderHandleType::Buffer: {
                        this->encoder->submit_command_buffer(command);
                    } break;
                    case types::RenderHandleType::Shader: {
                        this->encoder->submit_command_shader(command);
                    } break;
                    case types::RenderHandleType::Mesh: {
                        this->encoder->submit_command_mesh(command, render_target);
                    } break;
                    case types::RenderHandleType::View: {
                        this->encoder->submit_command_view(command, render_target);
                    } break;
                    case types::RenderHandleType::ViewPort: {
                        this->encoder->submit_command_viewport(command);
                    } break;
                    case types::RenderHandleType::State: {
                        this->encoder->submit_command_state(command);
                    } break;
                    case types::RenderHandleType::Image: {
                        this->encoder->submit_command_image(command);
                    } break;
                    case types::RenderHandleType::Topology: {
                        this->encoder->submit_command_topology(command);
                    } break;
                    case types::RenderHandleType::VertexLayout: {
                        this->encoder->submit_command_vertex_layout(command);
                    } break;
                    case types::RenderHandleType::Draw: {
                        this->encoder->draw(command.handle);
                    } break;
                    default:
                        break;
                }
            }
        }
    }

    void RenderSystem::present(void) const {
        this->encoder->present();
    }

    std::shared_ptr<platform::IRenderer> RenderSystem::get_renderer(void) const {
        return this->renderer;
    }
} // namespace enishi::core