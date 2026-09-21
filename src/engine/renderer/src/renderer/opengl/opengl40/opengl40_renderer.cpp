#include "opengl40_renderer.h"
#include "opengl40_helpers.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <engine_types/assets/model/model_data.h>
#include <glad/gl.h>
#include <renderer/common/converter/model_to_mesh.h>
#include <renderer/opengl/common/opengl_image_view.h>

namespace enishi::renderer::opengl {
    constexpr std::uint32_t NO_GL_OBJECT = 0;
    constexpr std::uint32_t RESOURCE_COUNT = 1;
    constexpr std::uint32_t SHADER_SOURCE_COUNT = 1;
    constexpr std::int32_t MINIMUM_IMAGE_DIMENSION = 1;
    constexpr std::uint32_t SINGLE_IMAGE_LAYER = 1;
    constexpr std::uint32_t SINGLE_SAMPLE = 1;
    constexpr std::int32_t MIP_DIMENSION_DIVISOR = 2;
    constexpr std::int32_t TEXTURE_IMAGE_BORDER_WIDTH = 0;
    constexpr std::uint32_t DEFAULT_FRAMEBUFFER = 0;
    constexpr std::uint32_t NO_INDEX_STRIDE = 0;
    constexpr std::uint32_t POSITION_ATTRIBUTE = 0;
    constexpr std::uint32_t NORMAL_ATTRIBUTE = 1;
    constexpr std::uint32_t TEXTURE_COORDINATE_ATTRIBUTE = 2;
    constexpr std::uint32_t BLEND_INDEX_ATTRIBUTE = 3;
    constexpr std::uint32_t BLEND_WEIGHT_ATTRIBUTE = 4;
    constexpr std::int32_t POSITION_COMPONENT_COUNT = 3;
    constexpr std::int32_t NORMAL_COMPONENT_COUNT = 3;
    constexpr std::int32_t TEXTURE_COORDINATE_COMPONENT_COUNT = 2;
    constexpr std::int32_t BLEND_INDEX_COMPONENT_COUNT = 2;
    constexpr std::int32_t BLEND_WEIGHT_COMPONENT_COUNT = 2;
    constexpr std::size_t TEXTURE_COORDINATE_OFFSET_MULTIPLIER = 2;
    constexpr std::uint32_t BYTE_INDEX_STRIDE = 1;
    constexpr std::uint32_t SHORT_INDEX_STRIDE = 2;
    constexpr std::int32_t UNPACK_ALIGNMENT = 1;
    constexpr std::size_t MIPMAP_COUNT_THRESHOLD = 1;
    constexpr std::uint32_t SHADER_PROGRAM_KEY_SHIFT = 32u;
    constexpr std::uint32_t FIRST_RENDER_TARGET = 0;
    constexpr std::uint32_t MINIMUM_INSTANCE_COUNT = 1;
    constexpr std::uint8_t EMPTY_COLOR_WRITE_MASK = 0;
    constexpr float DEFAULT_CLEAR_COLOR = 0.25f;
    constexpr float OPAQUE_ALPHA = 1.0f;

    OpenGL40RendererState::OpenGL40RendererState(void)
        : topology(GL_TRIANGLES)
        , active_vertex_shader(NO_GL_OBJECT)
        , active_fragment_shader(NO_GL_OBJECT)
        , active_program(NO_GL_OBJECT)
        , active_framebuffer(NO_GL_OBJECT)
        , active_index_type(GL_UNSIGNED_INT) {
    }

    OpenGL40Renderer::OpenGL40Renderer(std::shared_ptr<platform::IOpenGLContext> context)
        : context(std::move(context))
        , handle_mapper(std::make_unique<RenderHandleMapper>())
        , resource_accessor(std::make_unique<OpenGLResourceAccessor>())
        , state(std::make_unique<OpenGL40RendererState>()) {
        this->context->make_current();
        glEnable(GL_DEPTH_TEST);
    }
    OpenGL40Renderer::~OpenGL40Renderer(void) noexcept {
        if (!this->context || !this->context->make_current()) {
            return;
        }
        glDeleteBuffers(
            static_cast<GLsizei>(this->state->buffers.size()), this->state->buffers.data());
        glDeleteTextures(
            static_cast<GLsizei>(this->state->textures.size()), this->state->textures.data());
        glDeleteVertexArrays(static_cast<GLsizei>(this->state->vertex_arrays.size()),
            this->state->vertex_arrays.data());
        for (const auto shader : this->state->shaders) {
            glDeleteShader(shader);
        }
        for (const auto& [_, program] : this->state->programs) {
            glDeleteProgram(program);
        }
        glDeleteFramebuffers(static_cast<GLsizei>(this->state->framebuffers.size()),
            this->state->framebuffers.data());
    }
    types::RenderHandle OpenGL40Renderer::make_handle(const types::RenderHandleType type) noexcept {
        auto handle = this->handle_mapper->make(type, {});
        (*this->handle_mapper)[handle].resource = handle.id;
        return handle;
    }

    platform::RenderResult<types::RenderHandle> OpenGL40Renderer::create_viewport(
        const types::ViewportRect& viewport) {
        const auto handle = this->make_handle(types::RenderHandleType::ViewPort);
        this->state->viewports.emplace(handle, viewport);
        return handle;
    }
    platform::RenderResult<types::RenderHandle> OpenGL40Renderer::create_shader_reflection(
        const types::ShaderData& data) {
        auto reflection = std::make_shared<GLSLShaderReflection>();
        auto result = reflection->load(data);
        if (result.is_err()) {
            return result.propagation(platform::RenderError::MakeError);
        }
        for (const auto& resource :
            reflection->get_shader_input_reflection()->get_input_resources()) {
            if (resource.type == types::ShaderInputResourceType::UniformBuffer) {
                this->state->uniform_block_bindings.emplace(resource.name, resource.binding);
            }
        }
        const auto glsl_reflection = reflection;
        auto [resource_handle, _] =
            this->resource_accessor->make_shader_reflection(std::move(reflection));
        const auto handle = this->make_handle(types::RenderHandleType::ShaderReflection);
        (*this->handle_mapper)[handle].resource = resource_handle;
        this->state->reflections.emplace(handle, glsl_reflection);
        return handle;
    }
    platform::RenderResult<std::unique_ptr<platform::IPipelineLayout>>
    OpenGL40Renderer::create_vertex_layout(
        const types::VertexLayout&, const types::RenderHandle&, const types::RenderHandle&) {
        return foundation::Error(
            platform::RenderError::MakeError, "OpenGL 4.0 vertex layouts are not implemented");
    }
    platform::RenderResult<types::RenderHandle>
    OpenGL40Renderer::create_vertex_layout_from_shader_data(const types::ShaderData& data) {
        const auto reflection = this->create_shader_reflection(data);
        if (reflection.is_err()) {
            return reflection.propagation(platform::RenderError::MakeError);
        }
        return this->make_handle(types::RenderHandleType::VertexLayout);
    }
    platform::RenderResult<types::RenderHandle> OpenGL40Renderer::create_rasterizer(
        const types::RasterizerStateDescription& state) {
        const auto handle = this->make_handle(types::RenderHandleType::State);
        this->state->rasterizers.emplace(handle, state);
        this->resource_accessor->add_state(
            (*this->handle_mapper)[handle].resource, types::StateKind::Rasterizer);
        return handle;
    }
    platform::RenderResult<types::RenderHandle> OpenGL40Renderer::create_depth_stencil(
        const types::DepthStencilStateDescription& state) {
        const auto handle = this->make_handle(types::RenderHandleType::State);
        this->state->depth_stencils.emplace(handle, state);
        this->resource_accessor->add_state(
            (*this->handle_mapper)[handle].resource, types::StateKind::DepthStencil);
        return handle;
    }
    platform::RenderResult<types::RenderHandle> OpenGL40Renderer::create_sampler(
        const types::SamplerStateDescription& state) {
        const auto handle = this->make_handle(types::RenderHandleType::State);
        this->state->samplers.emplace(handle, state);
        this->resource_accessor->add_state(
            (*this->handle_mapper)[handle].resource, types::StateKind::Sampler);
        return handle;
    }
    platform::RenderResult<types::RenderHandle> OpenGL40Renderer::create_blend(
        const types::BlendStateDescription& state) {
        const auto handle = this->make_handle(types::RenderHandleType::State);
        this->state->blends.emplace(handle, state);
        this->resource_accessor->add_state(
            (*this->handle_mapper)[handle].resource, types::StateKind::Blend);
        return handle;
    }
    platform::RenderResult<types::RenderHandle> OpenGL40Renderer::create_image(
        const types::ImageDescription& description) {
        if (description.size.x < MINIMUM_IMAGE_DIMENSION ||
            description.size.y < MINIMUM_IMAGE_DIMENSION ||
            description.mip_levels == NO_GL_OBJECT ||
            description.array_layers != SINGLE_IMAGE_LAYER ||
            description.samples != SINGLE_SAMPLE) {
            return foundation::Error(platform::RenderError::MakeError,
                "OpenGL 4.0 supports only single-sample, single-layer 2D images");
        }
        if (description.format == types::ImageFormat::BC3_UNORM ||
            description.format == types::ImageFormat::BC7_UNORM) {
            return foundation::Error(platform::RenderError::MakeError,
                "OpenGL 4.0 image creation does not support compressed formats");
        }
        const auto handle = this->make_handle(types::RenderHandleType::Image);
        this->state->images.emplace(handle, description);
        if (description.contains(types::ImageUsage::BackBuffer)) {
            this->state->back_buffer_images.emplace(handle);

            this->state->objects.emplace(handle, NO_GL_OBJECT);
            return handle;
        }
        const auto internal_format = helpers::to_gl_image_internal_format(description.format);
        GLuint texture = NO_GL_OBJECT;
        glGenTextures(RESOURCE_COUNT, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        const auto format = helpers::to_gl_image_format(description.format);
        const auto type = helpers::to_gl_image_type(description.format);
        auto width = description.size.x;
        auto height = description.size.y;
        for (std::uint32_t level = 0; level < description.mip_levels; ++level) {
            glTexImage2D(GL_TEXTURE_2D,
                static_cast<GLint>(level),
                internal_format,
                width,
                height,
                TEXTURE_IMAGE_BORDER_WIDTH,
                format,
                type,
                nullptr);
            width = std::max(MINIMUM_IMAGE_DIMENSION, width / MIP_DIMENSION_DIVISOR);
            height = std::max(MINIMUM_IMAGE_DIMENSION, height / MIP_DIMENSION_DIVISOR);
        }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        this->state->textures.emplace_back(texture);
        this->state->objects.emplace(handle, texture);
        return handle;
    }
    platform::RenderResult<std::shared_ptr<platform::IRenderTargetView>>
    OpenGL40Renderer::create_render_target_view(
        types::RenderHandle image, const types::ImageViewDescription& description) {
        if (!this->state->objects.contains(image)) {
            return foundation::Error(platform::RenderError::MakeError, "Image handle is invalid");
        }
        const auto handle = this->make_handle(types::RenderHandleType::View);
        this->state->objects.emplace(handle, this->state->objects.at(image));
        if (this->state->back_buffer_images.contains(image)) {
            this->state->back_buffer_images.emplace(handle);
        }
        auto view = std::make_shared<OpenGLRenderTargetView>(handle, description);
        this->resource_accessor->make_render_target_view(handle.id, std::move(view));
        return this->resource_accessor->get_render_target_view(handle.id).unwrap();
    }
    platform::RenderResult<std::shared_ptr<platform::IDepthStencilView>>
    OpenGL40Renderer::create_depth_stencil_view(
        types::RenderHandle image, const types::ImageViewDescription& description) {
        if (!this->state->objects.contains(image)) {
            return foundation::Error(platform::RenderError::MakeError, "Image handle is invalid");
        }
        const auto handle = this->make_handle(types::RenderHandleType::View);
        this->state->objects.emplace(handle, this->state->objects.at(image));
        auto view = std::make_shared<OpenGLDepthStencilView>(handle, description);
        this->resource_accessor->make_depth_stencil_view(handle.id, std::move(view));
        return this->resource_accessor->get_depth_stencil_view(handle.id).unwrap();
    }
    platform::RenderResult<std::shared_ptr<platform::IShaderResourceView>>
    OpenGL40Renderer::create_shader_resource_view(
        types::RenderHandle image, const types::ImageViewDescription& description) {
        if (!this->state->objects.contains(image)) {
            return foundation::Error(platform::RenderError::MakeError, "Image handle is invalid");
        }
        const auto handle = this->make_handle(types::RenderHandleType::View);
        this->state->objects.emplace(handle, this->state->objects.at(image));
        auto view = std::make_shared<OpenGLShaderResourceView>(handle, description);
        this->resource_accessor->make_shader_resource_view(handle.id, std::move(view));
        return this->resource_accessor->get_shader_resource_view(handle.id).unwrap();
    }
    platform::RenderResult<std::shared_ptr<platform::IUnorderedAccessView>>
    OpenGL40Renderer::create_unordered_access_view(
        types::RenderHandle image, const types::ImageViewDescription& description) {
        (void)image;
        (void)description;
        return foundation::Error(
            platform::RenderError::MakeError, "OpenGL 4.0 does not support unordered access views");
    }
    platform::RenderResult<types::RenderHandle> OpenGL40Renderer::make_buffer(
        const types::RenderData& data, const std::uint32_t target) {
        GLuint object = NO_GL_OBJECT;
        glGenBuffers(RESOURCE_COUNT, &object);
        glBindBuffer(target, object);
        glBufferData(
            target, static_cast<GLsizeiptr>(data.byte_width()), data.raw_data(), GL_DYNAMIC_DRAW);
        this->state->buffers.emplace_back(object);
        const auto handle = this->make_handle(types::RenderHandleType::Buffer);
        this->state->objects.emplace(handle, object);
        return handle;
    }

    platform::RenderResult<types::RenderHandle> OpenGL40Renderer::create_mesh(
        const types::ModelData& model, const std::vector<types::RenderHandle>& shader_reflections) {
        auto mesh = ModelToMesh::to_mesh_data(model);
        if (mesh.is_err()) {
            return mesh.propagation(platform::RenderError::MakeError);
        }
        auto data = std::move(mesh.unwrap_mut());
        auto vertex = this->make_buffer(data.vertices.get_render_data(), GL_ARRAY_BUFFER);
        if (vertex.is_err()) {
            return std::move(vertex);
        }
        auto index = this->make_buffer(data.indices.get_render_data(), GL_ELEMENT_ARRAY_BUFFER);
        if (index.is_err()) {
            return std::move(index);
        }
        std::vector<types::HandleId> mesh_uniform_handles;
        for (const auto& reflection_handle : shader_reflections) {
            const auto reflection = this->state->reflections.find(reflection_handle);
            if (reflection == this->state->reflections.end()) {
                continue;
            }
            const auto* inputs = reflection->second->get_shader_input_reflection();
            for (const auto& resource : inputs->get_input_resources()) {
                if (resource.type != types::ShaderInputResourceType::UniformBuffer) {
                    continue;
                }
                const auto uniform = data.uniforms.find(resource.name);
                if (uniform == data.uniforms.end()) {
                    continue;
                }
                const auto render_data = uniform->second.get_render_data();
                GLuint buffer = NO_GL_OBJECT;
                glGenBuffers(RESOURCE_COUNT, &buffer);
                glBindBuffer(GL_UNIFORM_BUFFER, buffer);
                glBufferData(GL_UNIFORM_BUFFER,
                    static_cast<GLsizeiptr>(render_data.byte_width()),
                    render_data.raw_data(),
                    GL_DYNAMIC_DRAW);
                glBindBufferBase(GL_UNIFORM_BUFFER, resource.binding, buffer);
                this->state->buffers.emplace_back(buffer);
                auto updater = std::make_shared<OpenGLUniformUpdater>(
                    std::move(uniform->second), buffer, resource.binding);
                this->state->uniform_updaters.emplace_back(updater);
                const auto [buffer_handle, _] = this->resource_accessor->make_buffer();
                this->resource_accessor->add_interface(buffer_handle, updater);
                mesh_uniform_handles.emplace_back(buffer_handle);
            }
        }
        GLuint vao = NO_GL_OBJECT;
        glGenVertexArrays(RESOURCE_COUNT, &vao);
        this->state->vertex_arrays.emplace_back(vao);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, this->state->objects.at(vertex.unwrap()));
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->state->objects.at(index.unwrap()));
        const auto stride = static_cast<GLsizei>(data.vertices.get_render_data().stride);
        glEnableVertexAttribArray(POSITION_ATTRIBUTE);
        glVertexAttribPointer(
            POSITION_ATTRIBUTE, POSITION_COMPONENT_COUNT, GL_FLOAT, GL_FALSE, stride, nullptr);
        if (stride >= static_cast<GLsizei>(sizeof(types::Vertex))) {
            glEnableVertexAttribArray(NORMAL_ATTRIBUTE);

            glVertexAttribPointer(NORMAL_ATTRIBUTE,
                NORMAL_COMPONENT_COUNT,
                GL_FLOAT,
                GL_FALSE,
                stride,
                reinterpret_cast<const void*>(sizeof(glm::vec3)));
            glEnableVertexAttribArray(TEXTURE_COORDINATE_ATTRIBUTE);
            glVertexAttribPointer(TEXTURE_COORDINATE_ATTRIBUTE,
                TEXTURE_COORDINATE_COMPONENT_COUNT,
                GL_FLOAT,
                GL_FALSE,
                stride,
                reinterpret_cast<const void*>(
                    sizeof(glm::vec3) * TEXTURE_COORDINATE_OFFSET_MULTIPLIER));
        }
        if (stride >= static_cast<GLsizei>(sizeof(types::Vertex) + sizeof(types::Skinning))) {
            glEnableVertexAttribArray(BLEND_INDEX_ATTRIBUTE);

            glVertexAttribIPointer(BLEND_INDEX_ATTRIBUTE,
                BLEND_INDEX_COMPONENT_COUNT,
                GL_UNSIGNED_SHORT,
                stride,
                reinterpret_cast<const void*>(sizeof(types::Vertex)));
            glEnableVertexAttribArray(BLEND_WEIGHT_ATTRIBUTE);
            glVertexAttribPointer(BLEND_WEIGHT_ATTRIBUTE,
                BLEND_WEIGHT_COMPONENT_COUNT,
                GL_FLOAT,
                GL_FALSE,
                stride,
                reinterpret_cast<const void*>(sizeof(types::Vertex) + sizeof(glm::u16vec2)));
        }
        const auto handle = this->make_handle(types::RenderHandleType::Mesh);
        auto [mesh_resource, mesh_handles] = this->resource_accessor->make_mesh_handles();
        mesh_handles.mesh_handles.emplace_back(vertex.unwrap());
        mesh_handles.mesh_handles.emplace_back(index.unwrap());
        (*this->handle_mapper)[handle].resource = mesh_resource;
        this->state->mesh_uniform_buffers.emplace(handle, std::move(mesh_uniform_handles));
        this->state->objects.emplace(handle, vao);
        const auto index_stride = data.indices.get_render_data().stride;
        auto index_type = GL_UNSIGNED_INT;
        if (index_stride == SHORT_INDEX_STRIDE) {
            index_type = GL_UNSIGNED_SHORT;
        } else if (index_stride == BYTE_INDEX_STRIDE) {
            index_type = GL_UNSIGNED_BYTE;
        }
        this->state->index_types.emplace(handle, index_type);
        this->state->index_strides.emplace(handle, data.indices.get_render_data().stride);
        std::vector<types::DrawBinding> draw_bindings;
        draw_bindings.reserve(data.materials.size());
        for (const auto& material : data.materials) {
            draw_bindings.emplace_back(material.draw_binding);
        }
        this->state->mesh_draw_bindings.emplace(handle, std::move(draw_bindings));
        return handle;
    }
    platform::RenderResult<types::RenderHandle> OpenGL40Renderer::create_texture(
        const types::TextureData& texture) {
        if (texture.is_cubemap || texture.depth > SINGLE_IMAGE_LAYER || texture.mips.empty() ||
            types::TextureData::is_compressed(texture.format)) {
            return foundation::Error(
                platform::RenderError::MakeError, "Unsupported OpenGL 4.0 texture format");
        }
        const auto internal_format = helpers::to_gl_texture_internal_format(texture.format);
        const auto format = helpers::to_gl_texture_format(texture.format);
        const auto type = helpers::to_gl_texture_type(texture.format);
        GLuint object = NO_GL_OBJECT;
        glGenTextures(RESOURCE_COUNT, &object);
        glBindTexture(GL_TEXTURE_2D, object);
        glPixelStorei(GL_UNPACK_ALIGNMENT, UNPACK_ALIGNMENT);
        for (std::uint32_t level = 0; level < texture.mips.size(); ++level) {
            const auto& mip = texture.mips[level];
            glTexImage2D(GL_TEXTURE_2D,
                static_cast<GLint>(level),
                internal_format,
                mip.width,
                mip.height,
                TEXTURE_IMAGE_BORDER_WIDTH,
                format,
                type,
                mip.pixels.data());
        }
        glTexParameteri(GL_TEXTURE_2D,
            GL_TEXTURE_MIN_FILTER,
            texture.mips.size() > MIPMAP_COUNT_THRESHOLD ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        this->state->textures.emplace_back(object);
        const auto handle = this->make_handle(types::RenderHandleType::Image);
        this->state->objects.emplace(handle, object);
        return handle;
    }
    platform::RenderResult<types::RenderHandle> OpenGL40Renderer::create_shader(
        const types::ShaderKind kind, const types::ShaderData& data) {
        if (data.binary_type != types::ShaderBinaryType::SourceFileGLSL) {
            return foundation::Error(
                platform::RenderError::MakeError, "OpenGL 4.0 shaders must be GLSL source");
        }
        GLenum stage = NO_GL_OBJECT;
        if (kind == types::ShaderKind::Vertex) {
            stage = GL_VERTEX_SHADER;
        } else if (kind == types::ShaderKind::Pixel) {
            stage = GL_FRAGMENT_SHADER;
        }
        if (!stage) {
            return foundation::Error(
                platform::RenderError::MakeError, "Unsupported GLSL shader stage");
        }
        const auto shader = glCreateShader(stage);
        const auto source = reinterpret_cast<const GLchar*>(data.code.data());
        const auto length = static_cast<GLint>(data.code.size());
        glShaderSource(shader, SHADER_SOURCE_COUNT, &source, &length);
        glCompileShader(shader);
        GLint compiled = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (compiled != GL_TRUE) {
            glDeleteShader(shader);

            return foundation::Error(
                platform::RenderError::MakeError, "GLSL shader compilation failed");
        }
        const auto handle = this->make_handle(types::RenderHandleType::Shader);
        this->state->shaders.emplace_back(shader);
        this->state->objects.emplace(handle, shader);
        this->state->shader_kinds.emplace(handle, kind);
        return handle;
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
    void OpenGL40Renderer::setup_viewports(void) const {
    }
    void OpenGL40Renderer::setup_views(void) const {
        glBindFramebuffer(GL_FRAMEBUFFER, DEFAULT_FRAMEBUFFER);
    }
    void OpenGL40Renderer::submit_command_buffer(const types::DrawCommand&) const {
    }
    bool OpenGL40Renderer::use_active_program(void) const {
        if (!this->state->active_vertex_shader || !this->state->active_fragment_shader) {
            return false;
        }
        if (this->state->active_program) {
            glUseProgram(this->state->active_program);

            return true;
        }
        const auto program_key = (static_cast<std::uint64_t>(this->state->active_vertex_shader)
                                     << SHADER_PROGRAM_KEY_SHIFT) |
                                 this->state->active_fragment_shader;
        if (const auto program = this->state->programs.find(program_key);
            program != this->state->programs.end()) {
            this->state->active_program = program->second;

            glUseProgram(this->state->active_program);
            return true;
        }
        this->state->active_program = glCreateProgram();
        glAttachShader(this->state->active_program, this->state->active_vertex_shader);
        glAttachShader(this->state->active_program, this->state->active_fragment_shader);
        glLinkProgram(this->state->active_program);
        GLint linked = GL_FALSE;
        glGetProgramiv(this->state->active_program, GL_LINK_STATUS, &linked);
        if (linked != GL_TRUE) {
            glDeleteProgram(this->state->active_program);

            this->state->active_program = NO_GL_OBJECT;
            return false;
        }
        this->state->programs.emplace(program_key, this->state->active_program);
        for (const auto& [name, binding] : this->state->uniform_block_bindings) {
            const auto index = glGetUniformBlockIndex(this->state->active_program, name.c_str());
            if (index != GL_INVALID_INDEX) {
                glUniformBlockBinding(this->state->active_program, index, binding);
            }
        }
        glUseProgram(this->state->active_program);
        return true;
    }
    void OpenGL40Renderer::submit_command_shader(const types::DrawCommand& command) const {
        if (command.sub_command != types::SubCommand::Bind) {
            return;
        }
        const auto object = this->state->objects.find(command.handle);
        const auto kind = this->state->shader_kinds.find(command.handle);
        if (object == this->state->objects.end() || kind == this->state->shader_kinds.end()) {
            return;
        }
        if (kind->second == types::ShaderKind::Vertex) {
            this->state->active_vertex_shader = object->second;
        }
        if (kind->second == types::ShaderKind::Pixel) {
            this->state->active_fragment_shader = object->second;
        }
        this->state->active_program = NO_GL_OBJECT;
        this->use_active_program();
    }
    void OpenGL40Renderer::submit_command_view(
        const types::DrawCommand& command, const types::RenderHandle& render_target) const {
        if (command.sub_command != types::SubCommand::Bind) {
            return;
        }
        const auto view_type = this->resource_accessor->get_view_type(command.handle.id);
        if (view_type.is_none()) {
            return;
        }
        if (view_type.unwrap() == types::ImageViewType::ShaderResource) {
            const auto object = this->state->objects.find(command.handle);

            if (object != this->state->objects.end()) {
                glActiveTexture(GL_TEXTURE0);

                glBindTexture(GL_TEXTURE_2D, object->second);
            }
            return;
        }
        const auto object = this->state->objects.find(command.handle);
        if (object == this->state->objects.end()) {
            return;
        }
        const auto is_back_buffer =
            this->state->back_buffer_images.contains(command.handle) ||
            (render_target.is_valid() && this->state->back_buffer_images.contains(render_target));
        if (is_back_buffer) {
            glBindFramebuffer(GL_FRAMEBUFFER, DEFAULT_FRAMEBUFFER);

            glClearColor(
                DEFAULT_CLEAR_COLOR, DEFAULT_CLEAR_COLOR, DEFAULT_CLEAR_COLOR, OPAQUE_ALPHA);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
            return;
        }
        if (!this->state->active_framebuffer) {
            glGenFramebuffers(RESOURCE_COUNT, &this->state->active_framebuffer);

            this->state->framebuffers.emplace_back(this->state->active_framebuffer);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, this->state->active_framebuffer);
        if (view_type.unwrap() == types::ImageViewType::RenderTarget) {
            const auto render_target_view =
                this->resource_accessor->get_render_target_view(command.handle.id);
            if (render_target_view.is_none()) {
                return;
            }
            glFramebufferTexture2D(GL_FRAMEBUFFER,
                GL_COLOR_ATTACHMENT0,
                GL_TEXTURE_2D,
                object->second,
                static_cast<GLint>(render_target_view.unwrap()->get_description().base_mip_level));
        }
        if (view_type.unwrap() == types::ImageViewType::DepthStencil) {
            const auto depth_stencil_view =
                this->resource_accessor->get_depth_stencil_view(command.handle.id);
            if (depth_stencil_view.is_none()) {
                return;
            }
            glFramebufferTexture2D(GL_FRAMEBUFFER,
                GL_DEPTH_STENCIL_ATTACHMENT,
                GL_TEXTURE_2D,
                object->second,
                static_cast<GLint>(depth_stencil_view.unwrap()->get_description().base_mip_level));
        }
        if (render_target.is_valid() && view_type.unwrap() == types::ImageViewType::DepthStencil) {
            const auto target = this->state->objects.find(render_target);
            const auto render_target_view =
                this->resource_accessor->get_render_target_view(render_target.id);

            if (target != this->state->objects.end() && render_target_view.is_some()) {
                glFramebufferTexture2D(GL_FRAMEBUFFER,
                    GL_COLOR_ATTACHMENT0,
                    GL_TEXTURE_2D,
                    target->second,
                    static_cast<GLint>(
                        render_target_view.unwrap()->get_description().base_mip_level));
            }
        }
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            return;
        }
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    }
    void OpenGL40Renderer::submit_command_viewport(const types::DrawCommand& command) const {
        if (command.sub_command != types::SubCommand::Bind) {
            return;
        }
        const auto viewport = this->state->viewports.find(command.handle);
        if (viewport == this->state->viewports.end()) {
            return;
        }
        glViewport(static_cast<GLint>(viewport->second.left_top_x),
            static_cast<GLint>(viewport->second.left_top_y),
            static_cast<GLsizei>(viewport->second.width),
            static_cast<GLsizei>(viewport->second.height));
    }
    void OpenGL40Renderer::submit_command_mesh(
        const types::DrawCommand& command, const types::RenderHandle&) const {
        if (command.sub_command != types::SubCommand::Bind) {
            return;
        }
        const auto object = this->state->objects.find(command.handle);
        if (object == this->state->objects.end()) {
            return;
        }
        glBindVertexArray(object->second);
        const auto index_type = this->state->index_types.find(command.handle);
        this->state->active_index_type =
            index_type == this->state->index_types.end() ? GL_UNSIGNED_INT : index_type->second;
        const auto buffers = this->state->mesh_uniform_buffers.find(command.handle);
        if (buffers == this->state->mesh_uniform_buffers.end()) {
            return;
        }
        const auto accessor = this->resource_accessor->get_buffer_accessor();
        for (const auto& buffer_handle : buffers->second) {
            const auto updater = accessor->get_buffer(buffer_handle);
            if (updater.is_some() && updater.unwrap()) {
                updater.unwrap()->on_update();
            }
        }
        if (!this->use_active_program()) {
            return;
        }
        const auto bindings = this->state->mesh_draw_bindings.find(command.handle);
        if (bindings == this->state->mesh_draw_bindings.end()) {
            return;
        }
        const auto index_stride = this->state->index_strides.find(command.handle);
        const auto stride = index_stride == this->state->index_strides.end() ? NO_INDEX_STRIDE
                                                                             : index_stride->second;
        for (const auto& binding : bindings->second) {
            if (const auto indexed = std::get_if<types::DrawIndexedParameter>(&binding.parameter)) {
                glDrawElementsInstancedBaseVertex(this->state->topology,
                    indexed->index_count,
                    this->state->active_index_type,
                    reinterpret_cast<const void*>(
                        static_cast<std::uintptr_t>(indexed->first_index) * stride),
                    std::max(MINIMUM_INSTANCE_COUNT, indexed->instance_count),
                    static_cast<GLint>(indexed->vertex_offset));

            } else if (const auto plain = std::get_if<types::DrawParameter>(&binding.parameter)) {
                glDrawArraysInstanced(this->state->topology,
                    plain->first_vertex,
                    plain->vertex_count,
                    std::max(MINIMUM_INSTANCE_COUNT, plain->instance_count));
            }
        }
    }
    void OpenGL40Renderer::submit_command_topology(const types::DrawCommand& command) const {
        if (command.sub_command != types::SubCommand::Bind) {
            return;
        }
        const auto value = static_cast<types::PrimitiveTopology>(command.handle.id.handle_id);
        if (value == types::PrimitiveTopology::LineList) {
            this->state->topology = GL_LINES;
        } else if (value == types::PrimitiveTopology::PointList) {
            this->state->topology = GL_POINTS;
        } else {
            this->state->topology = GL_TRIANGLES;
        }
    }
    void OpenGL40Renderer::submit_command_vertex_layout(const types::DrawCommand&) const {
    }
    void OpenGL40Renderer::submit_command_state(const types::DrawCommand& command) const {
        if (command.sub_command != types::SubCommand::Bind) {
            return;
        }
        if (const auto it = this->state->rasterizers.find(command.handle);
            it != this->state->rasterizers.end()) {
            it->second.cull_mode == types::CullMode::None ? glDisable(GL_CULL_FACE)
                                                          : glEnable(GL_CULL_FACE);

            if (it->second.cull_mode == types::CullMode::Front) {
                glCullFace(GL_FRONT);
            } else {
                glCullFace(GL_BACK);
            }
            glPolygonMode(GL_FRONT_AND_BACK,
                it->second.fill_mode == types::FillMode::Wireframe ? GL_LINE : GL_FILL);
            glFrontFace(it->second.front_face == types::FrontFace::Clockwise ? GL_CW : GL_CCW);
            glLineWidth(it->second.line_width);
            return;
        }
        if (const auto it = this->state->depth_stencils.find(command.handle);
            it != this->state->depth_stencils.end()) {
            it->second.depth.enabled ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);

            glDepthMask(it->second.depth.write_enabled ? GL_TRUE : GL_FALSE);
            glDepthFunc(helpers::to_gl_compare(it->second.depth.compare_operator));
            return;
        }
        if (const auto it = this->state->blends.find(command.handle);
            it != this->state->blends.end()) {
            const auto& target = it->second.render_targets[FIRST_RENDER_TARGET];

            target.enabled ? glEnable(GL_BLEND) : glDisable(GL_BLEND);
            glBlendFuncSeparate(helpers::to_gl_blend_factor(target.src_color),
                helpers::to_gl_blend_factor(target.dst_color),
                helpers::to_gl_blend_factor(target.src_alpha),
                helpers::to_gl_blend_factor(target.dst_alpha));
            glBlendEquationSeparate(helpers::to_gl_blend_operator(target.color_operator),
                helpers::to_gl_blend_operator(target.alpha_operator));
            glColorMask((target.write_mask &
                            static_cast<std::uint8_t>(types::ColorWriteMask::ColorWriteR)) !=
                            EMPTY_COLOR_WRITE_MASK,
                (target.write_mask &
                    static_cast<std::uint8_t>(types::ColorWriteMask::ColorWriteG)) !=
                    EMPTY_COLOR_WRITE_MASK,
                (target.write_mask &
                    static_cast<std::uint8_t>(types::ColorWriteMask::ColorWriteB)) !=
                    EMPTY_COLOR_WRITE_MASK,
                (target.write_mask &
                    static_cast<std::uint8_t>(types::ColorWriteMask::ColorWriteA)) !=
                    EMPTY_COLOR_WRITE_MASK);
            return;
        }
        if (const auto it = this->state->samplers.find(command.handle);
            it != this->state->samplers.end()) {
            glTexParameteri(GL_TEXTURE_2D,
                GL_TEXTURE_MIN_FILTER,
                it->second.min_filter == types::FilterMode::Nearest ? GL_NEAREST : GL_LINEAR);

            glTexParameteri(GL_TEXTURE_2D,
                GL_TEXTURE_MAG_FILTER,
                it->second.mag_filter == types::FilterMode::Nearest ? GL_NEAREST : GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D,
                GL_TEXTURE_WRAP_S,
                helpers::to_gl_address_mode(it->second.address_u));
            glTexParameteri(GL_TEXTURE_2D,
                GL_TEXTURE_WRAP_T,
                helpers::to_gl_address_mode(it->second.address_v));
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, it->second.min_lod);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, it->second.max_lod);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, it->second.mip_lod_bias);
        }
    }
    void OpenGL40Renderer::submit_command_image(const types::DrawCommand& command) const {
        if (command.sub_command != types::SubCommand::Bind) {
            return;
        }
        const auto object = this->state->objects.find(command.handle);
        if (object == this->state->objects.end()) {
            return;
        }
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, object->second);
    }
    void OpenGL40Renderer::draw(const types::RenderHandle& handle) const {
        const auto binding = this->state->draw_bindings.find(handle);
        if (binding == this->state->draw_bindings.end() || !this->use_active_program()) {
            return;
        }
        if (const auto indexed =
                std::get_if<types::DrawIndexedParameter>(&binding->second.parameter)) {
            glDrawElementsInstancedBaseVertex(this->state->topology,
                indexed->index_count,
                this->state->active_index_type,
                reinterpret_cast<const void*>(static_cast<std::uintptr_t>(indexed->first_index)),
                indexed->instance_count,
                static_cast<GLint>(indexed->vertex_offset));
        } else if (const auto plain =
                       std::get_if<types::DrawParameter>(&binding->second.parameter)) {
            glDrawArraysInstanced(this->state->topology,
                plain->first_vertex,
                plain->vertex_count,
                plain->instance_count);
        }
    }
    void OpenGL40Renderer::present(void) const {
        this->context->present();
    }
} // namespace enishi::renderer::opengl
