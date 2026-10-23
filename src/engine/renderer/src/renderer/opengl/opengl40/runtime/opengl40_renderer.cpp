#include "opengl40_renderer.h"
#include <renderer/opengl/opengl40/command/opengl40_render_command_encoder.h>
#include <renderer/opengl/opengl40/image/opengl40_images.h>
#include <renderer/opengl/opengl40/mesh/opengl40_meshes.h>
#include <renderer/opengl/opengl40/pipeline/opengl40_render_states.h>
#include <renderer/opengl/opengl40/shader/opengl40_shaders.h>
#include <renderer/opengl/opengl40/view/opengl40_viewports.h>
#include <renderer/opengl/opengl40/view/opengl40_views.h>
#include <utility>
namespace enishi::renderer::opengl {
    OpenGL40Renderer::OpenGL40Renderer(std::shared_ptr<platform::IOpenGLContext> context)
        : context(std::move(context))
        , handle_mapper(std::make_unique<RenderHandleMapper>())
        , resource_accessor(std::make_unique<ResourceAccessor>())
        , state(std::make_unique<OpenGL40RendererState>()) {
        this->context->make_current();
        this->state->initialize();
        this->encoder = std::make_unique<OpenGL40RenderCommandEncoder>(
            *this->state, *this->resource_accessor, *this->context);
    }
    OpenGL40Renderer::~OpenGL40Renderer() noexcept {
        if (this->context == nullptr || !this->context->make_current()) {
            return;
        }
        this->state->release();
    }
    platform::RenderResult<std::unique_ptr<platform::IPipelineLayout>>
    OpenGL40Renderer::create_vertex_layout(
        const types::VertexLayout&, const types::RenderHandle&, const types::RenderHandle&) {
        return foundation::Error(
            platform::RenderError::MakeError, "OpenGL 4.0 vertex layouts are not implemented");
    }
    platform::RenderResult<std::shared_ptr<platform::IUnorderedAccessView>>
    OpenGL40Renderer::create_unordered_access_view(
        types::RenderHandle image, const types::ImageViewDescription& description) {
        (void)image;
        (void)description;
        return foundation::Error(
            platform::RenderError::MakeError, "OpenGL 4.0 does not support unordered access views");
    }
    platform::IRenderResourceAccessor* OpenGL40Renderer::get_resource_accessor(void) noexcept {
        return this->resource_accessor.get();
    }
    platform::IRenderResourceAccessor* const OpenGL40Renderer::get_resource_accessor(
        void) const noexcept {
        return this->resource_accessor.get();
    }
    const platform::IRenderHandleMapper* OpenGL40Renderer::get_handle_mapper(void) const noexcept {
        return this->handle_mapper.get();
    }
    platform::RenderResult<types::RenderHandle> OpenGL40Renderer::create_viewport(
        const types::ViewportRect& viewport) {
        return OpenGL40Viewports::create_viewport(*this->state, *this->handle_mapper, viewport);
    }
    platform::RenderResult<types::RenderHandle> OpenGL40Renderer::create_shader_reflection(
        const types::ShaderData& data) {
        return OpenGL40Shaders::create_shader_reflection(
            *this->state, *this->resource_accessor, *this->handle_mapper, data);
    }
    platform::RenderResult<types::RenderHandle>
    OpenGL40Renderer::create_vertex_layout_from_shader_data(const types::ShaderData& data) {
        return OpenGL40Shaders::create_vertex_layout_from_shader_data(
            *this->state, *this->resource_accessor, *this->handle_mapper, data);
    }
    platform::RenderResult<types::RenderHandle> OpenGL40Renderer::create_shader(
        const types::ShaderKind kind, const types::ShaderData& data) {
        return OpenGL40Shaders::create_shader(*this->state, *this->handle_mapper, kind, data);
    }
    platform::RenderResult<types::RenderHandle> OpenGL40Renderer::create_rasterizer(
        const types::RasterizerStateDescription& state) {
        return OpenGL40RenderStates::create_rasterizer(
            *this->state, *this->resource_accessor, *this->handle_mapper, state);
    }
    platform::RenderResult<types::RenderHandle> OpenGL40Renderer::create_depth_stencil(
        const types::DepthStencilStateDescription& state) {
        return OpenGL40RenderStates::create_depth_stencil(
            *this->state, *this->resource_accessor, *this->handle_mapper, state);
    }
    platform::RenderResult<types::RenderHandle> OpenGL40Renderer::create_sampler(
        const types::SamplerStateDescription& state) {
        return OpenGL40RenderStates::create_sampler(
            *this->state, *this->resource_accessor, *this->handle_mapper, state);
    }
    platform::RenderResult<types::RenderHandle> OpenGL40Renderer::create_blend(
        const types::BlendStateDescription& state) {
        return OpenGL40RenderStates::create_blend(
            *this->state, *this->resource_accessor, *this->handle_mapper, state);
    }
    platform::RenderResult<types::RenderHandle> OpenGL40Renderer::create_image(
        const types::ImageDescription& description) {
        return OpenGL40Images::create_image(*this->state, *this->handle_mapper, description);
    }
    platform::RenderResult<types::RenderHandle> OpenGL40Renderer::create_texture(
        const types::TextureData& texture) {
        return OpenGL40Images::create_texture(*this->state, *this->handle_mapper, texture);
    }
    platform::RenderResult<std::shared_ptr<platform::IRenderTargetView>>
    OpenGL40Renderer::create_render_target_view(
        types::RenderHandle image, const types::ImageViewDescription& description) {
        return OpenGL40Views::create_render_target_view(
            *this->state, *this->resource_accessor, *this->handle_mapper, image, description);
    }
    platform::RenderResult<std::shared_ptr<platform::IDepthStencilView>>
    OpenGL40Renderer::create_depth_stencil_view(
        types::RenderHandle image, const types::ImageViewDescription& description) {
        return OpenGL40Views::create_depth_stencil_view(
            *this->state, *this->resource_accessor, *this->handle_mapper, image, description);
    }
    platform::RenderResult<std::shared_ptr<platform::IShaderResourceView>>
    OpenGL40Renderer::create_shader_resource_view(
        types::RenderHandle image, const types::ImageViewDescription& description) {
        return OpenGL40Views::create_shader_resource_view(
            *this->state, *this->resource_accessor, *this->handle_mapper, image, description);
    }
    platform::RenderResult<types::RenderHandle> OpenGL40Renderer::create_mesh(
        const types::ModelData& model, const std::vector<types::RenderHandle>& shader_reflections) {
        return OpenGL40Meshes::create_mesh(*this->state,
            *this->resource_accessor,
            *this->handle_mapper,
            model,
            shader_reflections);
    }

    platform::IRenderCommandEncoder& OpenGL40Renderer::get_command_encoder() const noexcept {
        return *this->encoder;
    }
} // namespace enishi::renderer::opengl
