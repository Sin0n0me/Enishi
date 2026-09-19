#include "opengl40_renderer.h"
#include <glad/gl.h>

namespace enishi::renderer::opengl {
    namespace {
        template <typename T> platform::RenderResult<T> unsupported(void) {
            return foundation::Error(platform::RenderError::MakeError,
                "This OpenGL 4.0 resource operation has not been implemented");
        }
    }

    OpenGL40Renderer::OpenGL40Renderer(std::shared_ptr<OpenGL40Context> context)
        : context(std::move(context)) {
        this->context->make_current();
        glEnable(GL_DEPTH_TEST);
    }
    platform::RenderResult<types::RenderHandle> OpenGL40Renderer::create_viewport(const types::ViewportRect&) { return unsupported<types::RenderHandle>(); }
    platform::RenderResult<types::RenderHandle> OpenGL40Renderer::create_shader_reflection(const types::ShaderData&) { return unsupported<types::RenderHandle>(); }
    platform::RenderResult<std::unique_ptr<platform::IPipelineLayout>> OpenGL40Renderer::create_vertex_layout(const types::VertexLayout&, const types::RenderHandle&, const types::RenderHandle&) { return unsupported<std::unique_ptr<platform::IPipelineLayout>>(); }
    platform::RenderResult<types::RenderHandle> OpenGL40Renderer::create_vertex_layout_from_shader_data(const types::ShaderData&) { return unsupported<types::RenderHandle>(); }
    platform::RenderResult<types::RenderHandle> OpenGL40Renderer::create_rasterizer(const types::RasterizerStateDescription&) { return unsupported<types::RenderHandle>(); }
    platform::RenderResult<types::RenderHandle> OpenGL40Renderer::create_depth_stencil(const types::DepthStencilStateDescription&) { return unsupported<types::RenderHandle>(); }
    platform::RenderResult<types::RenderHandle> OpenGL40Renderer::create_sampler(const types::SamplerStateDescription&) { return unsupported<types::RenderHandle>(); }
    platform::RenderResult<types::RenderHandle> OpenGL40Renderer::create_blend(const types::BlendStateDescription&) { return unsupported<types::RenderHandle>(); }
    platform::RenderResult<types::RenderHandle> OpenGL40Renderer::create_image(const types::ImageDescription&) { return unsupported<types::RenderHandle>(); }
    platform::RenderResult<std::shared_ptr<platform::IRenderTargetView>> OpenGL40Renderer::create_render_target_view(types::RenderHandle, const types::ImageViewDescription&) { return unsupported<std::shared_ptr<platform::IRenderTargetView>>(); }
    platform::RenderResult<std::shared_ptr<platform::IDepthStencilView>> OpenGL40Renderer::create_depth_stencil_view(types::RenderHandle, const types::ImageViewDescription&) { return unsupported<std::shared_ptr<platform::IDepthStencilView>>(); }
    platform::RenderResult<std::shared_ptr<platform::IShaderResourceView>> OpenGL40Renderer::create_shader_resource_view(types::RenderHandle, const types::ImageViewDescription&) { return unsupported<std::shared_ptr<platform::IShaderResourceView>>(); }
    platform::RenderResult<std::shared_ptr<platform::IUnorderedAccessView>> OpenGL40Renderer::create_unordered_access_view(types::RenderHandle, const types::ImageViewDescription&) { return unsupported<std::shared_ptr<platform::IUnorderedAccessView>>(); }
    platform::RenderResult<types::RenderHandle> OpenGL40Renderer::create_mesh(const types::ModelData&, const std::vector<types::RenderHandle>&) { return unsupported<types::RenderHandle>(); }
    platform::RenderResult<types::RenderHandle> OpenGL40Renderer::create_texture(const types::TextureData&) { return unsupported<types::RenderHandle>(); }
    platform::RenderResult<types::RenderHandle> OpenGL40Renderer::create_shader(types::ShaderKind, const types::ShaderData&) { return unsupported<types::RenderHandle>(); }
    platform::IRenderResourceAccessor* OpenGL40Renderer::get_resource_accessor(void) noexcept { return nullptr; }
    platform::IRenderResourceAccessor* const OpenGL40Renderer::get_resource_accessor(void) const noexcept { return nullptr; }
    const platform::IRenderHandleMapper* OpenGL40Renderer::get_handle_mapper(void) const noexcept { return nullptr; }
    void OpenGL40Renderer::setup_viewports(void) const {}
    void OpenGL40Renderer::setup_views(void) const { glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); }
    void OpenGL40Renderer::submit_command_buffer(const types::DrawCommand&) const {}
    void OpenGL40Renderer::submit_command_shader(const types::DrawCommand&) const {}
    void OpenGL40Renderer::submit_command_view(const types::DrawCommand&, const types::RenderHandle&) const {}
    void OpenGL40Renderer::submit_command_viewport(const types::DrawCommand&) const {}
    void OpenGL40Renderer::submit_command_mesh(const types::DrawCommand&, const types::RenderHandle&) const {}
    void OpenGL40Renderer::submit_command_topology(const types::DrawCommand&) const {}
    void OpenGL40Renderer::submit_command_vertex_layout(const types::DrawCommand&) const {}
    void OpenGL40Renderer::submit_command_state(const types::DrawCommand&) const {}
    void OpenGL40Renderer::submit_command_image(const types::DrawCommand&) const {}
    void OpenGL40Renderer::draw(const types::RenderHandle&) const {}
    void OpenGL40Renderer::present(void) const { this->context->present(); }
} // namespace enishi::renderer::opengl
