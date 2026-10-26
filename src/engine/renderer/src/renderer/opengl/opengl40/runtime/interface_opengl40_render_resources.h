#pragma once
#include <memory>
#include <platform/renderer/interface_opengl_context.h>
#include <renderer/common/render_handle_mapper.h>
#include <renderer/common/resource_accessor.h>
#include <renderer/opengl/opengl40/runtime/opengl40_renderer_state.h>
namespace enishi::renderer::opengl {
    class IOpenGL40RenderResources {
      public:
        virtual ~IOpenGL40RenderResources() noexcept = default;
        virtual std::shared_ptr<platform::IOpenGLContext> get_context() noexcept = 0;
        virtual RenderHandleMapper& get_handle_mapper() noexcept = 0;
        virtual ResourceAccessor& get_resource_accessor() noexcept = 0;
        virtual OpenGL40RendererState& get_state() noexcept = 0;
    };
} // namespace enishi::renderer::opengl
