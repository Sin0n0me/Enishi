#include "render_system.h"
#include "bone_matrix_upload.h"
#include "camera_upload.h"
#include "material_morph_upload.h"
#include "shadow_upload.h"
#include "vertex_morph_upload.h"
#include <algorithm>
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

    void RenderSystem::set_camera(std::shared_ptr<sub_system::ICameraController> camera) {
        this->camera = std::move(camera);
    }

    void RenderSystem::set_ui(std::shared_ptr<sub_system::IUI> ui) {
        this->ui = std::move(ui);
    }

    bool enishi::core::RenderSystem::should_close(void) {
        return false;
    }

    void enishi::core::RenderSystem::pre_update(void) {
    }

    void RenderSystem::update(const types::DeltaTime& delta_time) {
        if (this->camera != nullptr) {
            const auto snapshot = this->camera->get_camera();
            const bool has_shadow = std::ranges::any_of(this->render_passes,
                [](const auto& pass) { return pass->get_sampled_depth().is_valid(); });
            if (has_shadow) {
                std::vector<types::RenderHandle> receivers;
                for (const auto& pass : this->render_passes) {
                    if (pass->get_shadow_receiver().is_valid()) {
                        receivers.push_back(pass->get_shadow_receiver());
                    }
                }
                const auto result =
                    upload_shadow(*this->renderer, *this->registry, snapshot, receivers);
                if (result.is_err()) {
                    foundation::Logger::warning(result.unwrap_err().get_message());
                }
            }
            for (auto [entity, model] : this->registry->view<component::ModelComponent>()) {
                if (!model.visible) {
                    continue;
                }
                // Upload every visible mesh, including newly loaded or re-shown models.
                const auto result = upload_camera(*this->renderer, model.render_handle, snapshot);
                if (result.is_err()) {
                    foundation::Logger::warning(result.unwrap_err().get_message());
                }
            }
        }
        for (auto [entity, model, morph] :
            this->registry->view<component::ModelComponent, component::MorphComponent>()) {
            const auto result = upload_vertex_morphs(*this->renderer, model, morph);
            if (result.is_err()) {
                foundation::Logger::warning(result.unwrap_err().get_message());
            }
            const auto material_result = upload_material_morphs(*this->renderer, model, morph);
            if (material_result.is_err()) {
                foundation::Logger::warning(material_result.unwrap_err().get_message());
            }
        }
        for (auto [entity, model, skinning] :
            this->registry->view<component::ModelComponent, component::SkinningComponent>()) {
            const auto result = upload_bone_matrices(*this->renderer, model, skinning);
            if (result.is_err()) {
                foundation::Logger::warning(result.unwrap_err().get_message());
            }
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
        if (this->ui != nullptr) {
            this->ui->render();
        }
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
