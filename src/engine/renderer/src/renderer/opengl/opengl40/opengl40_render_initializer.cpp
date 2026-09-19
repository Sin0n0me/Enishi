#include "opengl40_render_initializer.h"
#include "opengl40_context.h"

namespace enishi::renderer::opengl {
    foundation::Result<std::shared_ptr<OpenGL40Renderer>, platform::RenderError>
    OpenGL40RenderInitializer::init(const platform::WindowHandle& window, const types::WindowSize&) {
        auto context = OpenGL40Context::make(window);
        if (context.is_err()) {
            return context.propagation(platform::RenderError::MakeError);
        }
        return std::make_shared<OpenGL40Renderer>(std::move(context.unwrap_mut()));
    }
} // namespace enishi::renderer::opengl
