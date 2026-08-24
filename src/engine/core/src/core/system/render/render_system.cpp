#include "render_system.h"
#include <component/animation_component.h>
#include <component/model_component.h>

namespace enishi::core {
    RenderSystem::RenderSystem(std::shared_ptr<ecs::Registory> registory,
        std::shared_ptr<platform::IRenderer> renderer,
        std::shared_ptr<platform::IRenderCommandEncoder> encoder)
        : registory(registory)
        , renderer(renderer)
        , encoder(encoder) {
    }

    foundation::VoidResult<SystemError> RenderSystem::add_render_pass_constructor(
        foundation::UTF8&& pass_name,
        std::shared_ptr<IRenderPassConstructor> render_pass_constructor) {
        this->name_to_constructor[pass_name] = render_pass_constructor;
        return {};
    }

    foundation::VoidResult<SystemError> RenderSystem::create_render_passes(
        assets_system::IAssetSystem* const asset_system) {
        for (auto& [name, constructor] : this->name_to_constructor) {
            auto result = constructor->make(this->renderer.get(), asset_system)
                              .add_message("レンダーパスの構築に失敗しました");
            if (result.is_err()) {
                return result.propagation(SystemError::ConstructRenderPassError);
            }

            // TODO: 順序を任意に
            auto& pass = this->render_passes.emplace_back(result.unwrap());
        }
        return {};
    }

    bool enishi::core::RenderSystem::should_close(void) {
        return false;
    }

    void enishi::core::RenderSystem::pre_update(void) {
    }

    void RenderSystem::update(const types::DeltaTime& delta_time) {
        auto view =
            this->registory->view<component::AnimationComponent, component::ModelComponent>();

        for (auto [entity, animation, model] : view) {
        }

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
            for (const auto& command : pass->get_commands()) {
                switch (command.handle.type) {
                    case types::RenderHandleType::Buffer: {
                        this->encoder->submit_command_buffer(command);
                    } break;
                    case types::RenderHandleType::Shader: {
                        this->encoder->submit_command_shader(command);
                    } break;
                    case types::RenderHandleType::Mesh: {
                        this->encoder->submit_command_mesh(command);
                    } break;
                    case types::RenderHandleType::Texture: {
                        this->encoder->submit_command_texture(command);
                    } break;
                    case types::RenderHandleType::View: {
                        this->encoder->submit_command_view(command, pass->get_render_target());
                    } break;
                    case types::RenderHandleType::Rasterizer: {
                        this->encoder->submit_command_rasterizer(command);
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