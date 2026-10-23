#pragma once
#include "interface_opengl40_render_resources.h"
namespace enishi::renderer::opengl {
    class OpenGL40RenderResources final : public IOpenGL40RenderResources {
      private:
        std::shared_ptr<platform::IOpenGLContext> context;
        RenderHandleMapper handle_mapper;
        ResourceAccessor resource_accessor;
        OpenGL40RendererState state;

      public:
        explicit OpenGL40RenderResources(std::shared_ptr<platform::IOpenGLContext> context);
        OpenGL40RenderResources(const OpenGL40RenderResources&) = delete;
        ~OpenGL40RenderResources() noexcept override;
        OpenGL40RenderResources& operator=(const OpenGL40RenderResources&) = delete;
        std::shared_ptr<platform::IOpenGLContext> get_context() noexcept override;
        RenderHandleMapper& get_handle_mapper() noexcept override;
        ResourceAccessor& get_resource_accessor() noexcept override;
        OpenGL40RendererState& get_state() noexcept override;
    };
} // namespace enishi::renderer::opengl
