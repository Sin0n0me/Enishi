#include "opengl40_renderer.h"
#include <renderer/opengl/common/opengl_image_view.h>
#include <renderer/common/converter/model_to_mesh.h>
#include <glad/gl.h>

namespace enishi::renderer::opengl {
    namespace {
        template <typename T> platform::RenderResult<T> unsupported(void) {
            return foundation::Error(platform::RenderError::MakeError,
                "This OpenGL 4.0 resource operation has not been implemented");
        }
    }

    OpenGL40Renderer::OpenGL40Renderer(std::shared_ptr<OpenGL40Context> context)
        : context(std::move(context)), handle_mapper(std::make_unique<RenderHandleMapper>()), topology(GL_TRIANGLES) {
        this->context->make_current();
        glEnable(GL_DEPTH_TEST);
    }
    types::RenderHandle OpenGL40Renderer::make_handle(const types::RenderHandleType type) noexcept {
        auto handle = this->handle_mapper->make(type, {});
        (*this->handle_mapper)[handle].resource = handle.id;
        return handle;
    }

    platform::RenderResult<types::RenderHandle> OpenGL40Renderer::create_viewport(const types::ViewportRect& viewport) {
        glViewport(static_cast<GLint>(viewport.left_top_x), static_cast<GLint>(viewport.left_top_y), static_cast<GLsizei>(viewport.width), static_cast<GLsizei>(viewport.height));
        return this->make_handle(types::RenderHandleType::ViewPort);
    }
    platform::RenderResult<types::RenderHandle> OpenGL40Renderer::create_shader_reflection(const types::ShaderData&) { return unsupported<types::RenderHandle>(); }
    platform::RenderResult<std::unique_ptr<platform::IPipelineLayout>> OpenGL40Renderer::create_vertex_layout(const types::VertexLayout&, const types::RenderHandle&, const types::RenderHandle&) { return unsupported<std::unique_ptr<platform::IPipelineLayout>>(); }
    platform::RenderResult<types::RenderHandle> OpenGL40Renderer::create_vertex_layout_from_shader_data(const types::ShaderData&) { return unsupported<types::RenderHandle>(); }
    platform::RenderResult<types::RenderHandle> OpenGL40Renderer::create_rasterizer(const types::RasterizerStateDescription&) { return unsupported<types::RenderHandle>(); }
    platform::RenderResult<types::RenderHandle> OpenGL40Renderer::create_depth_stencil(const types::DepthStencilStateDescription&) { return unsupported<types::RenderHandle>(); }
    platform::RenderResult<types::RenderHandle> OpenGL40Renderer::create_sampler(const types::SamplerStateDescription&) { return unsupported<types::RenderHandle>(); }
    platform::RenderResult<types::RenderHandle> OpenGL40Renderer::create_blend(const types::BlendStateDescription&) { return unsupported<types::RenderHandle>(); }
    platform::RenderResult<types::RenderHandle> OpenGL40Renderer::create_image(const types::ImageDescription& description) {
        const auto internal_format = description.format == types::ImageFormat::D32_FLOAT ? GL_DEPTH_COMPONENT32F : description.format == types::ImageFormat::D24_UNORM_S8_UINT ? GL_DEPTH24_STENCIL8 : description.format == types::ImageFormat::RGBA16_FLOAT ? GL_RGBA16F : GL_RGBA8;
        GLuint texture = 0; glGenTextures(1, &texture); glBindTexture(GL_TEXTURE_2D, texture);
        const auto format = description.format == types::ImageFormat::D32_FLOAT ? GL_DEPTH_COMPONENT : description.format == types::ImageFormat::D24_UNORM_S8_UINT ? GL_DEPTH_STENCIL : GL_RGBA;
        const auto type = description.format == types::ImageFormat::D32_FLOAT ? GL_FLOAT : description.format == types::ImageFormat::D24_UNORM_S8_UINT ? GL_UNSIGNED_INT_24_8 : GL_UNSIGNED_BYTE;
        glTexImage2D(GL_TEXTURE_2D, 0, internal_format, description.size.x, description.size.y, 0, format, type, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        const auto handle = this->make_handle(types::RenderHandleType::Image); this->objects.emplace(handle, texture); this->images.emplace(handle, description); return handle;
    }
    platform::RenderResult<std::shared_ptr<platform::IRenderTargetView>> OpenGL40Renderer::create_render_target_view(types::RenderHandle image, const types::ImageViewDescription& description) {
        if (!this->objects.contains(image)) return foundation::Error(platform::RenderError::MakeError, "Image handle is invalid");
        const auto handle = this->make_handle(types::RenderHandleType::View); this->objects.emplace(handle, this->objects.at(image)); return std::static_pointer_cast<platform::IRenderTargetView>(std::make_shared<OpenGLRenderTargetView>(handle, description));
    }
    platform::RenderResult<std::shared_ptr<platform::IDepthStencilView>> OpenGL40Renderer::create_depth_stencil_view(types::RenderHandle image, const types::ImageViewDescription& description) {
        if (!this->objects.contains(image)) return foundation::Error(platform::RenderError::MakeError, "Image handle is invalid");
        const auto handle = this->make_handle(types::RenderHandleType::View); this->objects.emplace(handle, this->objects.at(image)); return std::static_pointer_cast<platform::IDepthStencilView>(std::make_shared<OpenGLDepthStencilView>(handle, description));
    }
    platform::RenderResult<std::shared_ptr<platform::IShaderResourceView>> OpenGL40Renderer::create_shader_resource_view(types::RenderHandle image, const types::ImageViewDescription& description) {
        if (!this->objects.contains(image)) return foundation::Error(platform::RenderError::MakeError, "Image handle is invalid");
        const auto handle = this->make_handle(types::RenderHandleType::View); this->objects.emplace(handle, this->objects.at(image)); return std::static_pointer_cast<platform::IShaderResourceView>(std::make_shared<OpenGLShaderResourceView>(handle, description));
    }
    platform::RenderResult<std::shared_ptr<platform::IUnorderedAccessView>> OpenGL40Renderer::create_unordered_access_view(types::RenderHandle image, const types::ImageViewDescription& description) {
        if (!this->objects.contains(image)) return foundation::Error(platform::RenderError::MakeError, "Image handle is invalid");
        const auto handle = this->make_handle(types::RenderHandleType::View); this->objects.emplace(handle, this->objects.at(image)); return std::static_pointer_cast<platform::IUnorderedAccessView>(std::make_shared<OpenGLUnorderedAccessView>(handle, description));
    }
    platform::RenderResult<types::RenderHandle> OpenGL40Renderer::make_buffer(const types::RenderData& data, const std::uint32_t target) {
        GLuint object = 0; glGenBuffers(1, &object); glBindBuffer(target, object);
        glBufferData(target, static_cast<GLsizeiptr>(data.byte_width()), data.raw_data(), GL_DYNAMIC_DRAW);
        const auto handle = this->make_handle(types::RenderHandleType::Buffer);
        this->objects.emplace(handle, object);
        return handle;
    }

    platform::RenderResult<types::RenderHandle> OpenGL40Renderer::create_mesh(const types::ModelData& model, const std::vector<types::RenderHandle>&) {
        auto mesh = ModelToMesh::to_mesh_data(model);
        if (mesh.is_err()) return mesh.propagation(platform::RenderError::MakeError);
        auto data = std::move(mesh.unwrap_mut());
        const auto vertex = this->make_buffer(data.vertices.get_render_data(), GL_ARRAY_BUFFER);
        if (vertex.is_err()) return vertex;
        const auto index = this->make_buffer(data.indices.get_render_data(), GL_ELEMENT_ARRAY_BUFFER);
        if (index.is_err()) return index;
        GLuint vao = 0; glGenVertexArrays(1, &vao); glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, this->objects.at(vertex.unwrap()));
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->objects.at(index.unwrap()));
        const auto handle = this->make_handle(types::RenderHandleType::Mesh);
        this->objects.emplace(handle, vao);
        this->index_types.emplace(handle, data.indices.get_render_data().stride == 2 ? GL_UNSIGNED_SHORT : data.indices.get_render_data().stride == 1 ? GL_UNSIGNED_BYTE : GL_UNSIGNED_INT);
        return handle;
    }
    platform::RenderResult<types::RenderHandle> OpenGL40Renderer::create_texture(const types::TextureData&) { return unsupported<types::RenderHandle>(); }
    platform::RenderResult<types::RenderHandle> OpenGL40Renderer::create_shader(const types::ShaderKind kind, const types::ShaderData& data) {
        if (data.binary_type != types::ShaderBinaryType::SourceFileGLSL) return foundation::Error(platform::RenderError::MakeError, "OpenGL 4.0 shaders must be GLSL source");
        const GLenum stage = kind == types::ShaderKind::Vertex ? GL_VERTEX_SHADER : kind == types::ShaderKind::Pixel ? GL_FRAGMENT_SHADER : kind == types::ShaderKind::Compute ? GL_COMPUTE_SHADER : 0;
        if (!stage) return foundation::Error(platform::RenderError::MakeError, "Unsupported GLSL shader stage");
        const auto shader = glCreateShader(stage); const auto source = reinterpret_cast<const GLchar*>(data.code.data()); const auto length = static_cast<GLint>(data.code.size());
        glShaderSource(shader, 1, &source, &length); glCompileShader(shader);
        GLint compiled = GL_FALSE; glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (compiled != GL_TRUE) { glDeleteShader(shader); return foundation::Error(platform::RenderError::MakeError, "GLSL shader compilation failed"); }
        const auto handle = this->make_handle(types::RenderHandleType::Shader); this->objects.emplace(handle, shader); this->shader_kinds.emplace(handle, kind); return handle;
    }
    platform::IRenderResourceAccessor* OpenGL40Renderer::get_resource_accessor(void) noexcept { return nullptr; }
    platform::IRenderResourceAccessor* const OpenGL40Renderer::get_resource_accessor(void) const noexcept { return nullptr; }
    const platform::IRenderHandleMapper* OpenGL40Renderer::get_handle_mapper(void) const noexcept { return this->handle_mapper.get(); }
    void OpenGL40Renderer::setup_viewports(void) const {}
    void OpenGL40Renderer::setup_views(void) const { glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); }
    void OpenGL40Renderer::submit_command_buffer(const types::DrawCommand&) const {}
    bool OpenGL40Renderer::use_active_program(void) const {
        if (!this->active_vertex_shader || !this->active_fragment_shader) return false;
        if (this->active_program) { glUseProgram(this->active_program); return true; }
        this->active_program = glCreateProgram(); glAttachShader(this->active_program, this->active_vertex_shader); glAttachShader(this->active_program, this->active_fragment_shader); glLinkProgram(this->active_program);
        GLint linked = GL_FALSE; glGetProgramiv(this->active_program, GL_LINK_STATUS, &linked); if (linked != GL_TRUE) { glDeleteProgram(this->active_program); this->active_program = 0; return false; } glUseProgram(this->active_program); return true;
    }
    void OpenGL40Renderer::submit_command_shader(const types::DrawCommand& command) const {
        if (command.sub_command != types::SubCommand::Bind) return;
        const auto object = this->objects.find(command.handle); const auto kind = this->shader_kinds.find(command.handle); if (object == this->objects.end() || kind == this->shader_kinds.end()) return;
        if (kind->second == types::ShaderKind::Vertex) this->active_vertex_shader = object->second;
        if (kind->second == types::ShaderKind::Pixel) this->active_fragment_shader = object->second;
        this->active_program = 0; this->use_active_program();
    }
    void OpenGL40Renderer::submit_command_view(const types::DrawCommand&, const types::RenderHandle&) const {}
    void OpenGL40Renderer::submit_command_viewport(const types::DrawCommand&) const {}
    void OpenGL40Renderer::submit_command_mesh(const types::DrawCommand& command, const types::RenderHandle&) const { if (command.sub_command == types::SubCommand::Bind) { const auto object = this->objects.find(command.handle); if (object != this->objects.end()) glBindVertexArray(object->second); } }
    void OpenGL40Renderer::submit_command_topology(const types::DrawCommand&) const {}
    void OpenGL40Renderer::submit_command_vertex_layout(const types::DrawCommand&) const {}
    void OpenGL40Renderer::submit_command_state(const types::DrawCommand&) const {}
    void OpenGL40Renderer::submit_command_image(const types::DrawCommand&) const {}
    void OpenGL40Renderer::draw(const types::RenderHandle& handle) const { const auto binding = this->draw_bindings.find(handle); if (binding == this->draw_bindings.end()) return; this->use_active_program(); if (const auto indexed = std::get_if<types::DrawIndexedParameter>(&binding->second.parameter)) glDrawElementsInstanced(this->topology, indexed->index_count, GL_UNSIGNED_INT, nullptr, indexed->instance_count); else if (const auto plain = std::get_if<types::DrawParameter>(&binding->second.parameter)) glDrawArraysInstanced(this->topology, plain->first_vertex, plain->vertex_count, plain->instance_count); }
    void OpenGL40Renderer::present(void) const { this->context->present(); }
} // namespace enishi::renderer::opengl
