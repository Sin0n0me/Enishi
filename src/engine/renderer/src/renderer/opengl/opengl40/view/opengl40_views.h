#pragma once
#include <renderer/common/render_handle_mapper.h>
#include <renderer/common/resource_accessor.h>
#include <renderer/opengl/opengl40/runtime/opengl40_renderer_state.h>

namespace enishi::renderer::opengl {
    class OpenGL40Views final {
      public:
        static platform::RenderResult<std::shared_ptr<platform::IRenderTargetView>>
        create_render_target_view(OpenGL40RendererState& runtime,
            ResourceAccessor& resource_accessor,
            RenderHandleMapper& handle_mapper,
            types::RenderHandle image,
            const types::ImageViewDescription& description);
        static platform::RenderResult<std::shared_ptr<platform::IDepthStencilView>>
        create_depth_stencil_view(OpenGL40RendererState& runtime,
            ResourceAccessor& resource_accessor,
            RenderHandleMapper& handle_mapper,
            types::RenderHandle image,
            const types::ImageViewDescription& description);
        static platform::RenderResult<std::shared_ptr<platform::IShaderResourceView>>
        create_shader_resource_view(OpenGL40RendererState& runtime,
            ResourceAccessor& resource_accessor,
            RenderHandleMapper& handle_mapper,
            types::RenderHandle image,
            const types::ImageViewDescription& description);
    };
} // namespace enishi::renderer::opengl
