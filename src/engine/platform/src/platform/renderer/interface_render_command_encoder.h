#pragma once
#include <engine_types/handle/renderer/render_handle.h>

namespace enishi::platform {
    class IRenderCommandEncoder {
      public:
        virtual ~IRenderCommandEncoder(void) noexcept = default;

        virtual void setup_viewports(void) const = 0;
        virtual void setup_render_targets(void) const = 0;

        virtual void bind_buffer(const types::RenderHandle& handle) const = 0;
        virtual void bind_shader(const types::RenderHandle& handle) const = 0;
        virtual void bind_view(const types::RenderHandle& handle) const = 0;
        virtual void bind_rasterizer(const types::RenderHandle& handle) const = 0;
        virtual void bind_texture(const types::RenderHandle& handle) const = 0;
        virtual void bind_mesh(const types::RenderHandle& handle) const = 0;
        virtual void bind_topology(const types::RenderHandle& handle) const = 0;
        virtual void bind_input_layout(const types::RenderHandle& handle) const = 0;

        virtual void draw(const types::RenderHandle& handle) const = 0;

        virtual void present(void) const = 0;
    };
} // namespace enishi::platform