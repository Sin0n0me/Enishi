#pragma once
#include <platform/renderer/interface_render_pass.h>
#include <unordered_map>

namespace enishi::renderer {
    class RendrPass : public platform::IRenderPass {
      private:
        types::RenderPass render_pass;
        std::unordered_map<types::RenderHandle, std::size_t> handle_to_index;

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
        foundation::VoidResult<platform::RenderError> remove_mesh(
            const types::RenderHandle handle) noexcept override;
        const types::RenderPass& get_render_pass(void) const noexcept override;
    };
} // namespace enishi::renderer