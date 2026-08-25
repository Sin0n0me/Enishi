#include "render_pass.h"

namespace enishi::renderer {
    std::span<const types::DrawCommand> RenderPass::get_commands(void) const noexcept {
        return this->commands;
    }

    types::RenderHandle RenderPass::get_render_target(void) const noexcept {
        return this->render_target;
    }

    void RenderPass::update(void) {
        for (auto& updater : this->resource_updater) {
            updater->update();
        }
    }

    foundation::VoidResult<platform::RenderError> RenderPass::make_render_pass(
        const types::PipelineDescription& description) noexcept {
        // RTVの追加
        this->render_target = description.render_target_view;

        // DSVの追加
        this->add_command(description.depth_stencil_view);

        // トポロジの追加
        this->add_command(types::RenderHandle{
            types::HandleId{static_cast<types::HandleId::HandleType>(description.topology)},
            types::RenderHandleType::Topology,
        });

        // ラスタライザの追加
        this->add_command(description.rasterizer_state);

        // 頂点レイアウトの追加
        this->add_command(description.vertex_layout);

        // シェーダーの追加
        for (const auto& shader : description.shaders) {
            this->add_command(shader);
        }

        return {};
    }

    void RenderPass::add_mesh(const foundation::UTF8& mesh_name, const types::RenderHandle handle) {
        this->mesh_name_to_index[mesh_name] = this->commands.size();
        this->add_command(handle);
    }

    void RenderPass::add_command(const types::RenderHandle handle) {
        this->commands.emplace_back(types::DrawCommand{
            .handle = handle,
            .sub_command = types::SubCommand::Bind,
        });
    }
} // namespace enishi::renderer