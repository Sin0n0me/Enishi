#include "opengl40_render_resources.h"
#include <utility>
namespace enishi::renderer::opengl {
    OpenGL40RenderResources::OpenGL40RenderResources(
        std::shared_ptr<platform::IOpenGLContext> context)
        : context(std::move(context)) {
        (void)this->context->make_current();
        this->state.initialize();
    }
    OpenGL40RenderResources::~OpenGL40RenderResources() noexcept {
        if (!this->context->make_current()) {
            return;
        }
        this->state.release();
    }
    std::shared_ptr<platform::IOpenGLContext> OpenGL40RenderResources::get_context() noexcept {
        return this->context;
    }
    RenderHandleMapper& OpenGL40RenderResources::get_handle_mapper() noexcept {
        return this->handle_mapper;
    }
    ResourceAccessor& OpenGL40RenderResources::get_resource_accessor() noexcept {
        return this->resource_accessor;
    }
    OpenGL40RendererState& OpenGL40RenderResources::get_state() noexcept {
        return this->state;
    }
} // namespace enishi::renderer::opengl
