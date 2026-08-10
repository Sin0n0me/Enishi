#pragma once
#include <foundation/option/option.h>
#include <platform/renderer/interface_render_pass.h>
#include <unordered_map>

namespace enishi::renderer {
    class RenderPass : public platform::IRenderPass {
      private:
        using Indices = std::vector<std::size_t>;

        types::RenderPass render_pass;
        std::unordered_map<types::RenderHandle, RenderPass::Indices> handle_to_index;

      public:
        foundation::VoidResult<platform::RenderError> make_render_pass(
            const types::PipelineDescription& description) noexcept override;
        foundation::VoidResult<platform::RenderError> set_topology(
            const types::PrimitiveTopology topology) noexcept override;
        foundation::VoidResult<platform::RenderError> set_render_target(
            const types::RenderHandle handle) noexcept override;
        foundation::VoidResult<platform::RenderError> set_rasterizer(
            const types::RenderHandle handle) noexcept override;
        foundation::VoidResult<platform::RenderError> set_vertex_layout(
            const types::RenderHandle handle) noexcept override;
        foundation::VoidResult<platform::RenderError> enable_uniform_camera(void) noexcept override;
        foundation::VoidResult<platform::RenderError> disable_uniform_camera(
            void) noexcept override;
        foundation::VoidResult<platform::RenderError> add_shader(
            const types::RenderHandle handle) noexcept override;
        foundation::VoidResult<platform::RenderError> add_mesh(const types::RenderHandle handle,
            const std::vector<types::RenderHandle>& shaders) noexcept override;
        foundation::VoidResult<platform::RenderError> remove(
            const types::RenderHandle handle) noexcept override;
        foundation::VoidResult<platform::RenderError> remove_latest(
            const types::RenderHandle handle) noexcept override;
        foundation::VoidResult<platform::RenderError> set_sub_command(
            const types::SubCommand sub_command,
            const types::RenderHandle handle) noexcept override;
        const types::RenderPass& get_render_pass(void) const noexcept override;

      private:
        foundation::VoidResult<platform::RenderError> add_command(const types::RenderHandle handle);
        foundation::Option<const RenderPass::Indices&> get_indices(
            const types::RenderHandle handle) const;
        foundation::Option<RenderPass::Indices&> get_indices(const types::RenderHandle handle);
    };
} // namespace enishi::renderer