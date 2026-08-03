#pragma once
#include <engine_types/renderer/render_handle.h>

namespace enishi::platform {
    class IRenderCommandEncoder {
      public:
        virtual ~IRenderCommandEncoder(void) noexcept = default;

        virtual void setup_viewports(void) const = 0;
        virtual void setup_render_targets(void) const = 0;

        virtual void bind_buffer(const types::HandleId id) const = 0;
        virtual void bind_shader(const types::HandleId id) const = 0;
        virtual void bind_view(const types::HandleId id) const = 0;
        virtual void bind_rasterizer(const types::HandleId id) const = 0;
        virtual void bind_texture(const types::HandleId id) const = 0;
        virtual void bind_mesh(const types::HandleId id) const = 0;
        virtual void bind_topology(const types::HandleId id) const = 0;
        virtual void bind_input_layout(const types::HandleId id) const = 0;

        virtual void present(void) const = 0;
    };
} // namespace enishi::platform