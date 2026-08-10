#include "render_pass.h"

namespace enishi::renderer {
    foundation::VoidResult<platform::RenderError> RendrPass::make_render_pass(
        const types::PipelineDescription& description) noexcept {
        // RTVの追加
        this->set_render_target(description.render_target);

        // トポロジの追加
        this->set_topology(description.topology);

        // ラスタライザの追加
        this->set_rasterizer(description.rasterizer);

        // 頂点レイアウトの追加
        this->render_pass.commands.emplace_back(description.vertex_layout);

        // シェーダーの追加
        for (const auto& shader : description.shaders) {
            this->add_shader(shader);
        }

        return {};
    }

    foundation::VoidResult<platform::RenderError> RendrPass::set_topology(
        const types::PrimitiveTopology topology) noexcept {
        this->render_pass.commands.emplace_back(types::DrawCommand{
            .handle =
                types::RenderHandle{
                    .id = static_cast<types::HandleId>(topology),
                    .type = types::RenderHandleType::Topology,
                },
            .sub_command = types::SubCommand::Bind,
        });

        return {};
    }

    foundation::VoidResult<platform::RenderError> RendrPass::set_render_target(
        const types::RenderHandle handle) noexcept {
        if (!handle.is_valid() || handle.type != types::RenderHandleType::View) {
            return;
        }

        this->render_pass.commands.emplace_back(types::DrawCommand{
            .handle = handle,
            .sub_command = types::SubCommand::Bind,
        });

        return {};
    }

    foundation::VoidResult<platform::RenderError> RendrPass::set_rasterizer(
        const types::RenderHandle handle) noexcept {
        if (!handle.is_valid() || handle.type != types::RenderHandleType::Rasterizer) {
            return;
        }

        this->render_pass.commands.emplace_back(types::DrawCommand{
            .handle = handle,
            .sub_command = types::SubCommand::Bind,
        });

        return {};
    }

    foundation::VoidResult<platform::RenderError> RendrPass::set_vertex_layout(
        const types::RenderHandle handle) noexcept {
        if (!handle.is_valid() || handle.type != types::RenderHandleType::VertexLayout) {
            return;
        }

        this->render_pass.commands.emplace_back(types::DrawCommand{
            .handle = handle,
            .sub_command = types::SubCommand::Bind,
        });

        return {};
    }

    foundation::VoidResult<platform::RenderError> RendrPass::enable_uniform_camera(void) noexcept {
        return {};
    }

    foundation::VoidResult<platform::RenderError> RendrPass::disable_uniform_camera(void) noexcept {
        return {};
    }

    foundation::VoidResult<platform::RenderError> RendrPass::add_shader(
        const types::RenderHandle handle) noexcept {
        if (!handle.is_valid() || handle.type != types::RenderHandleType::Shader) {
            return;
        }

        this->render_pass.commands.emplace_back(types::DrawCommand{
            .handle = handle,
            .sub_command = types::SubCommand::Bind,
        });

        return {};
    }

    foundation::VoidResult<platform::RenderError> RendrPass::add_mesh(
        const types::RenderHandle handle,
        const std::vector<types::RenderHandle>& shaders) noexcept {
        if (this->handle_to_index.contains(handle)) {
            return foundation::Error(platform::RenderError::MakeError);
        }
        const auto index = this->render_pass.commands.size();
        this->handle_to_index[handle] = index;

        this->render_pass.commands.emplace_back(types::DrawCommand{
            .handle = handle,
            .sub_command = types::SubCommand::Bind,
        });

        return {};
    }

    foundation::VoidResult<platform::RenderError> RendrPass::remove_mesh(
        const types::RenderHandle handle) noexcept {
        return {};
    }

    const types::RenderPass& RendrPass::get_render_pass(void) const noexcept {
        return this->render_pass;
    }
} // namespace enishi::renderer