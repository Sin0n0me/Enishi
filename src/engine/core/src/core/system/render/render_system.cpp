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
                              .add_message("レンダーパスの作成に失敗しました");
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
        this->encoder->setup_render_targets();

        // 各パイプラインに応じた描画コマンド実行
        for (const auto& pass : this->render_passes) {
            for (const auto& command : pass->get_commands()) {
                this->execute(command);
            }
        }
    }

    void RenderSystem::present(void) const {
        this->encoder->present();
    }

    void RenderSystem::execute(const types::DrawCommand& command) const {
        switch (command.sub_command) {
            case types::SubCommand::Bind: {
                this->bind(command.handle);
            } break;
            case types::SubCommand::Clear: {
            } break;
            case types::SubCommand::Unbind: {
            } break;
            case types::SubCommand::Nop: {
                // 明示的に何もしない
            } break;
        }
    }

    void RenderSystem::bind(const types::RenderHandle& render_handle) const {
        switch (render_handle.type) {
            case types::RenderHandleType::Buffer: {
                this->encoder->bind_buffer(render_handle);
            } break;
            case types::RenderHandleType::Shader: {
                this->encoder->bind_shader(render_handle);
            } break;
            case types::RenderHandleType::Mesh: {
                this->encoder->bind_mesh(render_handle);
            } break;
            case types::RenderHandleType::Texture: {
                this->encoder->bind_texture(render_handle);
            } break;
            case types::RenderHandleType::View: {
                this->encoder->bind_view(render_handle);
            } break;
            case types::RenderHandleType::Rasterizer: {
                this->encoder->bind_rasterizer(render_handle);
            } break;
            case types::RenderHandleType::Topology: {
                this->encoder->bind_topology(render_handle);
            } break;
            case types::RenderHandleType::VertexLayout: {
                this->encoder->bind_input_layout(render_handle);
            } break;
            case types::RenderHandleType::Draw: {
                this->encoder->draw(render_handle);
            } break;
            default:
                break;
        }
    }

    std::shared_ptr<platform::IRenderer> RenderSystem::get_renderer(void) const {
        return this->renderer;
    }

    void core::RenderSystem::add_command() {
    }
} // namespace enishi::core