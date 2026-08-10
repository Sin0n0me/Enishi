#include "render_pass.h"

namespace enishi::renderer {
    foundation::VoidResult<platform::RenderError> RenderPass::make_render_pass(
        const types::PipelineDescription& description) noexcept {
        // RTVの追加
        this->set_render_target(description.render_target);

        // トポロジの追加
        this->set_topology(description.topology);

        // ラスタライザの追加
        this->set_rasterizer(description.rasterizer);

        // 頂点レイアウトの追加
        this->set_vertex_layout(description.vertex_layout);

        // シェーダーの追加
        for (const auto& shader : description.shaders) {
            this->add_shader(shader);
        }

        return {};
    }

    foundation::VoidResult<platform::RenderError> RenderPass::set_topology(
        const types::PrimitiveTopology topology) noexcept {
        return this->add_command(types::RenderHandle{
            .id = static_cast<types::HandleId>(topology),
            .type = types::RenderHandleType::Topology,
        });
    }

    foundation::VoidResult<platform::RenderError> RenderPass::set_render_target(
        const types::RenderHandle handle) noexcept {
        if (!handle.is_valid() || handle.type != types::RenderHandleType::View) {
            return foundation::Error(platform::RenderError::MakeError, "無効なハンドルです");
        }

        return this->add_command(handle);
    }

    foundation::VoidResult<platform::RenderError> RenderPass::set_rasterizer(
        const types::RenderHandle handle) noexcept {
        if (!handle.is_valid() || handle.type != types::RenderHandleType::Rasterizer) {
            return foundation::Error(platform::RenderError::MakeError, "無効なハンドルです");
        }

        return this->add_command(handle);
    }

    foundation::VoidResult<platform::RenderError> RenderPass::set_vertex_layout(
        const types::RenderHandle handle) noexcept {
        if (!handle.is_valid() || handle.type != types::RenderHandleType::VertexLayout) {
            return foundation::Error(platform::RenderError::MakeError, "無効なハンドルです");
        }

        return this->add_command(handle);
    }

    foundation::VoidResult<platform::RenderError> RenderPass::enable_uniform_camera(void) noexcept {
        return {};
    }

    foundation::VoidResult<platform::RenderError> RenderPass::disable_uniform_camera(
        void) noexcept {
        return {};
    }

    foundation::VoidResult<platform::RenderError> RenderPass::add_shader(
        const types::RenderHandle handle) noexcept {
        if (!handle.is_valid() || handle.type != types::RenderHandleType::Shader) {
            return foundation::Error(platform::RenderError::MakeError, "無効なハンドルです");
        }

        return this->add_command(handle);
    }

    foundation::VoidResult<platform::RenderError> RenderPass::add_mesh(
        const types::RenderHandle handle,
        const std::vector<types::RenderHandle>& shaders) noexcept {
        if (this->handle_to_index.contains(handle)) {
            return foundation::Error(platform::RenderError::MakeError);
        }

        // シェーダーの追加
        for (const auto& shader : shaders) {
            auto&& result = this->add_shader(shader);
            if (result.is_err()) {
                return result;
            }
        }

        return this->add_command(handle);
    }

    foundation::VoidResult<platform::RenderError> RenderPass::remove(
        const types::RenderHandle handle) noexcept {
        auto opt_indices = this->get_indices(handle);
        if (opt_indices.is_none()) {
            return {};
        }

        auto& indices = opt_indices.unwrap_mut();
        indices.erase(indices.begin());

        return {};
    }

    foundation::VoidResult<platform::RenderError> RenderPass::remove_latest(
        const types::RenderHandle handle) noexcept {
        auto opt_indices = this->get_indices(handle);
        if (opt_indices.is_none()) {
            return {};
        }

        auto& indices = opt_indices.unwrap_mut();
        indices.erase(indices.end());

        return foundation::VoidResult<platform::RenderError>();
    }

    foundation::VoidResult<platform::RenderError> RenderPass::set_sub_command(
        const types::SubCommand sub_command, const types::RenderHandle handle) noexcept {
        auto opt_indices = this->get_indices(handle);
        if (opt_indices.is_none()) {
            return {};
        }

        auto& indices = opt_indices.unwrap_mut();

        for (const auto index : indices) {
            this->render_pass.commands[index].sub_command = sub_command;
        }

        return {};
    }

    const types::RenderPass& RenderPass::get_render_pass(void) const noexcept {
        return this->render_pass;
    }

    foundation::VoidResult<platform::RenderError> RenderPass::add_command(
        const types::RenderHandle handle) {
        const auto index = this->render_pass.commands.size();
        this->handle_to_index[handle].emplace_back(index);

        this->render_pass.commands.emplace_back(types::DrawCommand{
            .handle = handle,
            .sub_command = types::SubCommand::Bind,
        });

        return {};
    }

    foundation::Option<const RenderPass::Indices&> RenderPass::get_indices(
        const types::RenderHandle handle) const {
        const auto iter = this->handle_to_index.find(handle);
        if (iter == this->handle_to_index.end()) {
            return {};
        }

        return iter->second;
    }

    foundation::Option<RenderPass::Indices&> RenderPass::get_indices(
        const types::RenderHandle handle) {
        const auto iter = this->handle_to_index.find(handle);
        if (iter == this->handle_to_index.end()) {
            return {};
        }

        return iter->second;
    }
} // namespace enishi::renderer