#pragma once
#include <platform/renderer/interface_render_command_encoder.h>
#include <renderer/directx/directx11/d3d11.h>
#include <renderer/directx/directx11/resource/resource_manager.h>

namespace enishi::renderer::directx {
    class D3D11RenderCommandEncoder final : public platform::IRenderCommandEncoder {
      private:
        D3D11& d3d11;
        ResourceManager& resource_manager;

      public:
        D3D11RenderCommandEncoder(D3D11& d3d11, ResourceManager& resource_manager);
        void setup_viewports(void) const override;
        void setup_views(void) const override;
        void submit_command_buffer(const types::DrawCommand& command) const override;
        void submit_command_shader(const types::DrawCommand& command) const override;
        void submit_command_view(
            const types::DrawCommand& command, const types::RenderHandle& handle) const override;
        void submit_command_mesh(
            const types::DrawCommand& command, const types::RenderHandle& handle) const override;
        void submit_command_topology(const types::DrawCommand& command) const override;
        void submit_command_vertex_layout(const types::DrawCommand& command) const override;
        void submit_command_viewport(const types::DrawCommand& command) const override;
        void submit_command_state(const types::DrawCommand& command) const override;
        void submit_command_image(const types::DrawCommand& command) const override;
        void draw(const types::RenderHandle& handle) const override;
        void present(void) const override;

      private:
        void bind_buffer(const types::RenderHandle& handle) const;
        void bind_shader(const types::RenderHandle& handle) const;
        void bind_view(const types::RenderHandle& bind_handle,
            const types::RenderHandle& render_target_handle) const;
        void bind_state(const types::RenderHandle& handle) const;
        void bind_image(const types::RenderHandle& handle) const;
        void bind_mesh(const types::RenderHandle& handle,
            const types::RenderHandle& render_target_handle) const;
        void bind_topology(const types::RenderHandle& handle) const;
        void bind_input_layout(const types::RenderHandle& handle) const;
    };
} // namespace enishi::renderer::directx
