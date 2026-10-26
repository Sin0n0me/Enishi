#pragma once
#include <renderer/common/render_handle_mapper.h>

namespace enishi::renderer::opengl {
    class OpenGL40Handles final {
      public:
        static types::RenderHandle make_handle(
            RenderHandleMapper& handle_mapper, const types::RenderHandleType type) noexcept;
    };
} // namespace enishi::renderer::opengl
