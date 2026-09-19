#include "opengl40_renderer.h"
#include <renderer/opengl/common/opengl_image_view.h>
#include <engine_types/assets/model/model_data.h>
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
        : context(std::move(context)), handle_mapper(std::make_unique<RenderHandleMapper>()), resource_accessor(std::make_unique<OpenGLResourceAccessor>()), topology(GL_TRIANGLES) {
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
    platform::RenderResult<types::RenderHandle> OpenGL40Renderer::create_shader_reflection(const types::ShaderData& data) {
        auto reflection=std::make_shared<GLSLShaderReflection>(); auto result=reflection->load(data);
        if (result.is_err()) return result.propagation(platform::RenderError::MakeError);
        const auto handle=this->make_handle(types::RenderHandleType::ShaderReflection); this->reflections.emplace(handle, std::move(reflection)); return handle;
    }
    platform::RenderResult<std::unique_ptr<platform::IPipelineLayout>> OpenGL40Renderer::create_vertex_layout(const types::VertexLayout&, const types::RenderHandle&, const types::RenderHandle&) { return unsupported<std::unique_ptr<platform::IPipelineLayout>>(); }
    platform::RenderResult<types::RenderHandle> OpenGL40Renderer::create_vertex_layout_from_shader_data(const types::ShaderData& data) { const auto reflection=this->create_shader_reflection(data); if (reflection.is_err()) return reflection; return this->make_handle(types::RenderHandleType::VertexLayout); }
    platform::RenderResult<types::RenderHandle> OpenGL40Renderer::create_rasterizer(const types::RasterizerStateDescription& state) { const auto handle=this->make_handle(types::RenderHandleType::State); this->rasterizers.emplace(handle, state); return handle; }
    platform::RenderResult<types::RenderHandle> OpenGL40Renderer::create_depth_stencil(const types::DepthStencilStateDescription& state) { const auto handle=this->make_handle(types::RenderHandleType::State); this->depth_stencils.emplace(handle, state); return handle; }
    platform::RenderResult<types::RenderHandle> OpenGL40Renderer::create_sampler(const types::SamplerStateDescription& state) { const auto handle=this->make_handle(types::RenderHandleType::State); this->samplers.emplace(handle, state); return handle; }
    platform::RenderResult<types::RenderHandle> OpenGL40Renderer::create_blend(const types::BlendStateDescription& state) { const auto handle=this->make_handle(types::RenderHandleType::State); this->blends.emplace(handle, state); return handle; }
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
        const auto handle = this->make_handle(types::RenderHandleType::View); this->objects.emplace(handle, this->objects.at(image)); auto view=std::make_shared<OpenGLRenderTargetView>(handle, description); this->resource_accessor->make_render_target_view(handle.id, std::move(view)); return this->resource_accessor->get_render_target_view(handle.id).unwrap();
    }
    platform::RenderResult<std::shared_ptr<platform::IDepthStencilView>> OpenGL40Renderer::create_depth_stencil_view(types::RenderHandle image, const types::ImageViewDescription& description) {
        if (!this->objects.contains(image)) return foundation::Error(platform::RenderError::MakeError, "Image handle is invalid");
        const auto handle = this->make_handle(types::RenderHandleType::View); this->objects.emplace(handle, this->objects.at(image)); auto view=std::make_shared<OpenGLDepthStencilView>(handle, description); this->resource_accessor->make_depth_stencil_view(handle.id, std::move(view)); return this->resource_accessor->get_depth_stencil_view(handle.id).unwrap();
    }
    platform::RenderResult<std::shared_ptr<platform::IShaderResourceView>> OpenGL40Renderer::create_shader_resource_view(types::RenderHandle image, const types::ImageViewDescription& description) {
        if (!this->objects.contains(image)) return foundation::Error(platform::RenderError::MakeError, "Image handle is invalid");
        const auto handle = this->make_handle(types::RenderHandleType::View); this->objects.emplace(handle, this->objects.at(image)); auto view=std::make_shared<OpenGLShaderResourceView>(handle, description); this->resource_accessor->make_shader_resource_view(handle.id, std::move(view)); return this->resource_accessor->get_shader_resource_view(handle.id).unwrap();
    }
    platform::RenderResult<std::shared_ptr<platform::IUnorderedAccessView>> OpenGL40Renderer::create_unordered_access_view(types::RenderHandle image, const types::ImageViewDescription& description) {
        if (!this->objects.contains(image)) return foundation::Error(platform::RenderError::MakeError, "Image handle is invalid");
        const auto handle = this->make_handle(types::RenderHandleType::View); this->objects.emplace(handle, this->objects.at(image)); auto view=std::make_shared<OpenGLUnorderedAccessView>(handle, description); this->resource_accessor->make_unordered_access_view(handle.id, std::move(view)); return this->resource_accessor->get_unordered_access_view(handle.id).unwrap();
    }
    platform::RenderResult<types::RenderHandle> OpenGL40Renderer::make_buffer(const types::RenderData& data, const std::uint32_t target) {
        GLuint object = 0; glGenBuffers(1, &object); glBindBuffer(target, object);
        glBufferData(target, static_cast<GLsizeiptr>(data.byte_width()), data.raw_data(), GL_DYNAMIC_DRAW);
        const auto handle = this->make_handle(types::RenderHandleType::Buffer);
        this->objects.emplace(handle, object);
        return handle;
    }

    platform::RenderResult<types::RenderHandle> OpenGL40Renderer::create_mesh(const types::ModelData& model, const std::vector<types::RenderHandle>& shader_reflections) {
        auto mesh = ModelToMesh::to_mesh_data(model);
        if (mesh.is_err()) return mesh.propagation(platform::RenderError::MakeError);
        auto data = std::move(mesh.unwrap_mut());
        const auto vertex = this->make_buffer(data.vertices.get_render_data(), GL_ARRAY_BUFFER);
        if (vertex.is_err()) return vertex;
        const auto index = this->make_buffer(data.indices.get_render_data(), GL_ELEMENT_ARRAY_BUFFER);
        if (index.is_err()) return index;
        std::vector<types::HandleId> mesh_uniform_handles;
        for (const auto& reflection_handle : shader_reflections) {
            const auto reflection = this->reflections.find(reflection_handle);
            if (reflection == this->reflections.end()) continue;
            const auto* inputs = reflection->second->get_shader_input_reflection();
            for (const auto& resource : inputs->get_input_resources()) {
                if (resource.type != types::ShaderInputResourceType::UniformBuffer) continue;
                const auto uniform = data.uniforms.find(resource.name);
                if (uniform == data.uniforms.end()) continue;
                const auto render_data = uniform->second.get_render_data();
                GLuint buffer = 0; glGenBuffers(1, &buffer); glBindBuffer(GL_UNIFORM_BUFFER, buffer);
                glBufferData(GL_UNIFORM_BUFFER, static_cast<GLsizeiptr>(render_data.byte_width()), render_data.raw_data(), GL_DYNAMIC_DRAW);
                glBindBufferBase(GL_UNIFORM_BUFFER, resource.binding, buffer);
                auto updater = std::make_shared<OpenGLUniformUpdater>(std::move(uniform->second), buffer, resource.binding);
                this->uniform_updaters.emplace_back(updater);
                const auto [buffer_handle, _] = this->resource_accessor->make_buffer();
                this->resource_accessor->add_interface(buffer_handle, updater);
                mesh_uniform_handles.emplace_back(buffer_handle);
            }
        }
        GLuint vao = 0; glGenVertexArrays(1, &vao); glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, this->objects.at(vertex.unwrap()));
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->objects.at(index.unwrap()));
        const auto stride = static_cast<GLsizei>(data.vertices.get_render_data().stride);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, nullptr);
        if (stride >= static_cast<GLsizei>(sizeof(types::Vertex))) {
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(sizeof(glm::vec3)));
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(sizeof(glm::vec3) * 2));
        }
        const auto handle = this->make_handle(types::RenderHandleType::Mesh);
        this->mesh_uniform_buffers.emplace(handle, std::move(mesh_uniform_handles));
        this->objects.emplace(handle, vao);
        this->index_types.emplace(handle, data.indices.get_render_data().stride == 2 ? GL_UNSIGNED_SHORT : data.indices.get_render_data().stride == 1 ? GL_UNSIGNED_BYTE : GL_UNSIGNED_INT);
        return handle;
    }
    platform::RenderResult<types::RenderHandle> OpenGL40Renderer::create_texture(const types::TextureData& texture) {
        if (texture.is_cubemap || texture.depth > 1 || texture.mips.empty() || types::TextureData::is_compressed(texture.format)) {
            return foundation::Error(platform::RenderError::MakeError, "Unsupported OpenGL 4.0 texture format");
        }
        const auto internal_format = texture.format == types::TextureFormat::R8_UNORM ? GL_R8 : texture.format == types::TextureFormat::RG8_UNORM ? GL_RG8 : texture.format == types::TextureFormat::RGBA16_FLOAT ? GL_RGBA16F : texture.format == types::TextureFormat::RGBA8_SRGB ? GL_SRGB8_ALPHA8 : GL_RGBA8;
        const auto format = texture.format == types::TextureFormat::R8_UNORM ? GL_RED : texture.format == types::TextureFormat::RG8_UNORM ? GL_RG : GL_RGBA;
        const auto type = texture.format == types::TextureFormat::RGBA16_FLOAT ? GL_HALF_FLOAT : GL_UNSIGNED_BYTE;
        GLuint object = 0; glGenTextures(1, &object); glBindTexture(GL_TEXTURE_2D, object); glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        for (std::uint32_t level = 0; level < texture.mips.size(); ++level) { const auto& mip = texture.mips[level]; glTexImage2D(GL_TEXTURE_2D, static_cast<GLint>(level), internal_format, mip.width, mip.height, 0, format, type, mip.pixels.data()); }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, texture.mips.size() > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        const auto handle = this->make_handle(types::RenderHandleType::Image); this->objects.emplace(handle, object); return handle;
    }
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
    platform::IRenderResourceAccessor* OpenGL40Renderer::get_resource_accessor(void) noexcept { return this->resource_accessor.get(); }
    platform::IRenderResourceAccessor* const OpenGL40Renderer::get_resource_accessor(void) const noexcept { return this->resource_accessor.get(); }
    const platform::IRenderHandleMapper* OpenGL40Renderer::get_handle_mapper(void) const noexcept { return this->handle_mapper.get(); }
    void OpenGL40Renderer::setup_viewports(void) const {}
    void OpenGL40Renderer::setup_views(void) const { glBindFramebuffer(GL_FRAMEBUFFER, 0); glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); }
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
    void OpenGL40Renderer::submit_command_view(const types::DrawCommand& command, const types::RenderHandle& render_target) const {
        if (command.sub_command != types::SubCommand::Bind) return;
        const auto view_type = this->resource_accessor->get_view_type(command.handle.id);
        if (view_type.is_none()) return;
        if (view_type.unwrap() == types::ImageViewType::ShaderResource) {
            const auto object = this->objects.find(command.handle);
            if (object != this->objects.end()) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, object->second);
            }
            return;
        }
        if (!this->active_framebuffer) glGenFramebuffers(1, &this->active_framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, this->active_framebuffer);
        const auto object = this->objects.find(command.handle); if (object == this->objects.end()) return;
        if (view_type.unwrap() == types::ImageViewType::RenderTarget) glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, object->second, 0);
        if (view_type.unwrap() == types::ImageViewType::DepthStencil) glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, object->second, 0);
        if (render_target.is_valid() && view_type.unwrap() == types::ImageViewType::DepthStencil) { const auto target=this->objects.find(render_target); if (target != this->objects.end()) glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, target->second, 0); }
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    }
    void OpenGL40Renderer::submit_command_viewport(const types::DrawCommand&) const {}
    void OpenGL40Renderer::submit_command_mesh(const types::DrawCommand& command, const types::RenderHandle&) const {
        if (command.sub_command != types::SubCommand::Bind) return;
        const auto object = this->objects.find(command.handle);
        if (object == this->objects.end()) return;
        glBindVertexArray(object->second);
        const auto buffers = this->mesh_uniform_buffers.find(command.handle);
        if (buffers == this->mesh_uniform_buffers.end()) return;
        const auto accessor = this->resource_accessor->get_buffer_accessor();
        for (const auto& buffer_handle : buffers->second) {
            const auto updater = accessor->get_buffer(buffer_handle);
            if (updater.is_some() && updater.unwrap()) updater.unwrap()->on_update();
        }
    }
    void OpenGL40Renderer::submit_command_topology(const types::DrawCommand&) const {}
    void OpenGL40Renderer::submit_command_vertex_layout(const types::DrawCommand&) const {}
    void OpenGL40Renderer::submit_command_state(const types::DrawCommand& command) const {
        if (command.sub_command != types::SubCommand::Bind) return;
        if (const auto it=this->rasterizers.find(command.handle); it != this->rasterizers.end()) { it->second.cull_mode == types::CullMode::None ? glDisable(GL_CULL_FACE) : glEnable(GL_CULL_FACE); if (it->second.cull_mode == types::CullMode::Front) glCullFace(GL_FRONT); else glCullFace(GL_BACK); glPolygonMode(GL_FRONT_AND_BACK, it->second.fill_mode == types::FillMode::Wireframe ? GL_LINE : GL_FILL); glFrontFace(it->second.front_face == types::FrontFace::Clockwise ? GL_CW : GL_CCW); glLineWidth(it->second.line_width); return; }
        if (const auto it=this->depth_stencils.find(command.handle); it != this->depth_stencils.end()) { it->second.depth.enabled ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST); glDepthMask(it->second.depth.write_enabled ? GL_TRUE : GL_FALSE); return; }
        if (const auto it=this->blends.find(command.handle); it != this->blends.end()) { it->second.render_targets[0].enabled ? glEnable(GL_BLEND) : glDisable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); return; }
        if (const auto it=this->samplers.find(command.handle); it != this->samplers.end()) { glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, it->second.min_filter == types::FilterMode::Nearest ? GL_NEAREST : GL_LINEAR); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, it->second.mag_filter == types::FilterMode::Nearest ? GL_NEAREST : GL_LINEAR); }
    }
    void OpenGL40Renderer::submit_command_image(const types::DrawCommand& command) const { if (command.sub_command != types::SubCommand::Bind) return; const auto object=this->objects.find(command.handle); if (object == this->objects.end()) return; glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, object->second); }
    void OpenGL40Renderer::draw(const types::RenderHandle& handle) const { const auto binding = this->draw_bindings.find(handle); if (binding == this->draw_bindings.end()) return; this->use_active_program(); if (const auto indexed = std::get_if<types::DrawIndexedParameter>(&binding->second.parameter)) glDrawElementsInstanced(this->topology, indexed->index_count, GL_UNSIGNED_INT, nullptr, indexed->instance_count); else if (const auto plain = std::get_if<types::DrawParameter>(&binding->second.parameter)) glDrawArraysInstanced(this->topology, plain->first_vertex, plain->vertex_count, plain->instance_count); }
    void OpenGL40Renderer::present(void) const { this->context->present(); }
} // namespace enishi::renderer::opengl
