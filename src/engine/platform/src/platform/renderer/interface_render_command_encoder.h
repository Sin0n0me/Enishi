#pragma once
#include <engine_types/handle/renderer/render_handle.h>
#include <engine_types/renderer/command/draw_command.h>

namespace enishi::platform {
    class IRenderCommandEncoder {
      public:
        virtual ~IRenderCommandEncoder(void) noexcept = default;

        virtual void setup_viewports(void) const = 0;
        virtual void setup_views(void) const = 0;

        virtual void submit_command_buffer(const types::DrawCommand& command) const = 0;
        virtual void submit_command_shader(const types::DrawCommand& command) const = 0;
        virtual void submit_command_view(
            const types::DrawCommand& command, const types::RenderHandle& handle) const = 0;
        virtual void submit_command_rasterizer(const types::DrawCommand& command) const = 0;
        virtual void submit_command_texture(const types::DrawCommand& command) const = 0;
        virtual void submit_command_mesh(const types::DrawCommand& command) const = 0;
        virtual void submit_command_topology(const types::DrawCommand& command) const = 0;
        virtual void submit_command_vertex_layout(const types::DrawCommand& command) const = 0;

        virtual void draw(const types::RenderHandle& handle) const = 0;

        virtual void present(void) const = 0;
    };
} // namespace enishi::platform