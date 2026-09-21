#include "opengl40_renderer.h"
#include <algorithm>
#include <engine_types/assets/model/model_data.h>
#include <glad/gl.h>
#include <renderer/common/converter/model_to_mesh.h>
#include <renderer/opengl/common/opengl_image_view.h>
#include <unordered_map>
#include <unordered_set>

namespace enishi::renderer::opengl {
    class OpenGL40Renderer::State {
      public:
        std::unordered_map<types::RenderHandle, std::uint32_t> objects;
        std::unordered_map<types::RenderHandle, types::ShaderKind> shader_kinds;
        std::unordered_map<types::RenderHandle, types::DrawBinding> draw_bindings;
        std::unordered_map<types::RenderHandle, std::uint32_t> index_types;
        std::unordered_map<types::RenderHandle, std::uint32_t> index_strides;
        std::unordered_map<types::RenderHandle, std::vector<types::DrawBinding>> mesh_draw_bindings;
        std::unordered_map<types::RenderHandle, types::ImageDescription> images;
        std::unordered_set<types::RenderHandle> back_buffer_images;
        std::unordered_map<types::RenderHandle, types::RasterizerStateDescription> rasterizers;
        std::unordered_map<types::RenderHandle, types::DepthStencilStateDescription> depth_stencils;
        std::unordered_map<types::RenderHandle, types::BlendStateDescription> blends;
        std::unordered_map<types::RenderHandle, types::SamplerStateDescription> samplers;
        std::unordered_map<types::RenderHandle, std::shared_ptr<GLSLShaderReflection>> reflections;
        std::unordered_map<std::string, std::uint32_t> uniform_block_bindings;
        std::vector<std::shared_ptr<OpenGLUniformUpdater>> uniform_updaters;
        std::unordered_map<types::RenderHandle, std::vector<types::HandleId>> mesh_uniform_buffers;
        std::vector<GLuint> buffers;
        std::vector<GLuint> textures;
        std::vector<GLuint> vertex_arrays;
        std::vector<GLuint> shaders;
        std::unordered_map<std::uint64_t, GLuint> programs;
        std::vector<GLuint> framebuffers;
        std::uint32_t topology;
        std::uint32_t active_vertex_shader;
        std::uint32_t active_fragment_shader;
        std::uint32_t active_program;
        std::uint32_t active_framebuffer;
        std::uint32_t active_index_type;

        State(void)
            : topology(GL_TRIANGLES)
            , active_vertex_shader(0)
            , active_fragment_shader(0)
            , active_program(0)
            , active_framebuffer(0)
            , active_index_type(GL_UNSIGNED_INT) {
        }
    };

    namespace {
        template <typename T> platform::RenderResult<T> unsupported(void) {
            return foundation::Error(platform::RenderError::MakeError,
                "This OpenGL 4.0 resource operation has not been implemented");
        }

        GLenum to_gl_compare(const types::CompareOperator value) {
            switch (value) {
                case types::CompareOperator::Never:
                    return GL_NEVER;
                case types::CompareOperator::Less:
                    return GL_LESS;
                case types::CompareOperator::Equal:
                    return GL_EQUAL;
                case types::CompareOperator::LessEqual:
                    return GL_LEQUAL;
                case types::CompareOperator::Greater:
                    return GL_GREATER;
                case types::CompareOperator::NotEqual:
                    return GL_NOTEQUAL;
                case types::CompareOperator::GreaterEqual:
                    return GL_GEQUAL;
                case types::CompareOperator::Always:
                    return GL_ALWAYS;
            }
            return GL_ALWAYS;
        }

        GLenum to_gl_blend_factor(const types::BlendFactor value) {
            switch (value) {
                case types::BlendFactor::Zero:
                    return GL_ZERO;
                case types::BlendFactor::One:
                    return GL_ONE;
                case types::BlendFactor::SrcColor:
                    return GL_SRC_COLOR;
                case types::BlendFactor::OneMinusSrcColor:
                    return GL_ONE_MINUS_SRC_COLOR;
                case types::BlendFactor::DstColor:
                    return GL_DST_COLOR;
                case types::BlendFactor::OneMinusDstColor:
                    return GL_ONE_MINUS_DST_COLOR;
                case types::BlendFactor::SrcAlpha:
                    return GL_SRC_ALPHA;
                case types::BlendFactor::OneMinusSrcAlpha:
                    return GL_ONE_MINUS_SRC_ALPHA;
                case types::BlendFactor::DstAlpha:
                    return GL_DST_ALPHA;
                case types::BlendFactor::OneMinusDstAlpha:
                    return GL_ONE_MINUS_DST_ALPHA;
                case types::BlendFactor::ConstantColor:
                    return GL_CONSTANT_COLOR;
                case types::BlendFactor::OneMinusConstantColor:
                    return GL_ONE_MINUS_CONSTANT_COLOR;
                case types::BlendFactor::ConstantAlpha:
                    return GL_CONSTANT_ALPHA;
                case types::BlendFactor::OneMinusConstantAlpha:
                    return GL_ONE_MINUS_CONSTANT_ALPHA;
                case types::BlendFactor::SrcAlphaSaturate:
                    return GL_SRC_ALPHA_SATURATE;
                case types::BlendFactor::Src1Color:
                    return GL_SRC1_COLOR;
                case types::BlendFactor::OneMinusSrc1Color:
                    return GL_ONE_MINUS_SRC1_COLOR;
                case types::BlendFactor::Src1Alpha:
                    return GL_SRC1_ALPHA;
                case types::BlendFactor::OneMinusSrc1Alpha:
                    return GL_ONE_MINUS_SRC1_ALPHA;
            }
            return GL_ONE;
        }

        GLenum to_gl_blend_operator(const types::BlendOperator value) {
            switch (value) {
                case types::BlendOperator::Add:
                    return GL_FUNC_ADD;
                case types::BlendOperator::Subtract:
                    return GL_FUNC_SUBTRACT;
                case types::BlendOperator::ReverseSubtract:
                    return GL_FUNC_REVERSE_SUBTRACT;
                case types::BlendOperator::Min:
                    return GL_MIN;
                case types::BlendOperator::Max:
                    return GL_MAX;
            }
            return GL_FUNC_ADD;
        }

        GLenum to_gl_address_mode(const types::AddressMode value) {
            switch (value) {
                case types::AddressMode::Repeat:
                    return GL_REPEAT;
                case types::AddressMode::Clamp:
                    return GL_CLAMP_TO_EDGE;
                case types::AddressMode::Mirror:
                    return GL_MIRRORED_REPEAT;
            }
            return GL_REPEAT;
        }

    } // namespace

    OpenGL40Renderer::OpenGL40Renderer(std::shared_ptr<platform::IOpenGLContext> context)
        : context(std::move(context))
        , handle_mapper(std::make_unique<RenderHandleMapper>())
        , resource_accessor(std::make_unique<OpenGLResourceAccessor>())
        , state(std::make_unique<State>()) {
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
        for (const auto shader : this->state->shaders)
            glDeleteShader(shader);
        for (const auto& [_, program] : this->state->programs)
            glDeleteProgram(program);
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
        glViewport(static_cast<GLint>(viewport.left_top_x),
            static_cast<GLint>(viewport.left_top_y),
            static_cast<GLsizei>(viewport.width),
            static_cast<GLsizei>(viewport.height));
        return this->make_handle(types::RenderHandleType::ViewPort);
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
        return unsupported<std::unique_ptr<platform::IPipelineLayout>>();
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
        if (description.size.x <= 0 || description.size.y <= 0 || description.mip_levels == 0 ||
            description.array_layers != 1 || description.samples != 1) {
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

            this->state->objects.emplace(handle, 0);
            return handle;
        }
        const auto internal_format =
            description.format == types::ImageFormat::D32_FLOAT           ? GL_DEPTH_COMPONENT32F
            : description.format == types::ImageFormat::D24_UNORM_S8_UINT ? GL_DEPTH24_STENCIL8
            : description.format == types::ImageFormat::D16_UNORM         ? GL_DEPTH_COMPONENT16
            : description.format == types::ImageFormat::RGBA16_FLOAT      ? GL_RGBA16F
                                                                          : GL_RGBA8;
        GLuint texture = 0;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        const auto format = description.format == types::ImageFormat::D32_FLOAT ||
                                    description.format == types::ImageFormat::D16_UNORM
                                ? GL_DEPTH_COMPONENT
                            : description.format == types::ImageFormat::D24_UNORM_S8_UINT
                                ? GL_DEPTH_STENCIL
                            : description.format == types::ImageFormat::BGRA8_UNORM ? GL_BGRA
                                                                                    : GL_RGBA;
        const auto type = description.format == types::ImageFormat::D32_FLOAT   ? GL_FLOAT
                          : description.format == types::ImageFormat::D16_UNORM ? GL_UNSIGNED_SHORT
                          : description.format == types::ImageFormat::D24_UNORM_S8_UINT
                              ? GL_UNSIGNED_INT_24_8
                              : GL_UNSIGNED_BYTE;
        auto width = description.size.x;
        auto height = description.size.y;
        for (std::uint32_t level = 0; level < description.mip_levels; ++level) {
            glTexImage2D(GL_TEXTURE_2D,
                static_cast<GLint>(level),
                internal_format,
                width,
                height,
                0,
                format,
                type,
                nullptr);
            width = std::max(1, width / 2);
            height = std::max(1, height / 2);
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
        GLuint object = 0;
        glGenBuffers(1, &object);
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
                GLuint buffer = 0;
                glGenBuffers(1, &buffer);
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
        GLuint vao = 0;
        glGenVertexArrays(1, &vao);
        this->state->vertex_arrays.emplace_back(vao);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, this->state->objects.at(vertex.unwrap()));
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->state->objects.at(index.unwrap()));
        const auto stride = static_cast<GLsizei>(data.vertices.get_render_data().stride);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, nullptr);
        if (stride >= static_cast<GLsizei>(sizeof(types::Vertex))) {
            glEnableVertexAttribArray(1);

            glVertexAttribPointer(
                1, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(sizeof(glm::vec3)));
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2,
                2,
                GL_FLOAT,
                GL_FALSE,
                stride,
                reinterpret_cast<const void*>(sizeof(glm::vec3) * 2));
        }
        if (stride >= static_cast<GLsizei>(sizeof(types::Vertex) + sizeof(types::Skinning))) {
            glEnableVertexAttribArray(3);

            glVertexAttribIPointer(3,
                2,
                GL_UNSIGNED_SHORT,
                stride,
                reinterpret_cast<const void*>(sizeof(types::Vertex)));
            glEnableVertexAttribArray(4);
            glVertexAttribPointer(4,
                2,
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
        this->state->index_types.emplace(handle,
            data.indices.get_render_data().stride == 2   ? GL_UNSIGNED_SHORT
            : data.indices.get_render_data().stride == 1 ? GL_UNSIGNED_BYTE
                                                         : GL_UNSIGNED_INT);
        this->state->index_strides.emplace(handle, data.indices.get_render_data().stride);
        std::vector<types::DrawBinding> draw_bindings;
        draw_bindings.reserve(data.materials.size());
        for (const auto& material : data.materials)
            draw_bindings.emplace_back(material.draw_binding);
        this->state->mesh_draw_bindings.emplace(handle, std::move(draw_bindings));
        return handle;
    }
    platform::RenderResult<types::RenderHandle> OpenGL40Renderer::create_texture(
        const types::TextureData& texture) {
        if (texture.is_cubemap || texture.depth > 1 || texture.mips.empty() ||
            types::TextureData::is_compressed(texture.format)) {
            return foundation::Error(
                platform::RenderError::MakeError, "Unsupported OpenGL 4.0 texture format");
        }
        const auto internal_format =
            texture.format == types::TextureFormat::R8_UNORM       ? GL_R8
            : texture.format == types::TextureFormat::RG8_UNORM    ? GL_RG8
            : texture.format == types::TextureFormat::RGBA16_FLOAT ? GL_RGBA16F
            : texture.format == types::TextureFormat::RGBA8_SRGB   ? GL_SRGB8_ALPHA8
                                                                   : GL_RGBA8;
        const auto format = texture.format == types::TextureFormat::R8_UNORM    ? GL_RED
                            : texture.format == types::TextureFormat::RG8_UNORM ? GL_RG
                                                                                : GL_RGBA;
        const auto type =
            texture.format == types::TextureFormat::RGBA16_FLOAT ? GL_HALF_FLOAT : GL_UNSIGNED_BYTE;
        GLuint object = 0;
        glGenTextures(1, &object);
        glBindTexture(GL_TEXTURE_2D, object);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        for (std::uint32_t level = 0; level < texture.mips.size(); ++level) {
            const auto& mip = texture.mips[level];
            glTexImage2D(GL_TEXTURE_2D,
                static_cast<GLint>(level),
                internal_format,
                mip.width,
                mip.height,
                0,
                format,
                type,
                mip.pixels.data());
        }
        glTexParameteri(GL_TEXTURE_2D,
            GL_TEXTURE_MIN_FILTER,
            texture.mips.size() > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
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
        const GLenum stage = kind == types::ShaderKind::Vertex  ? GL_VERTEX_SHADER
                             : kind == types::ShaderKind::Pixel ? GL_FRAGMENT_SHADER
                                                                : 0;
        if (!stage) {
            return foundation::Error(
                platform::RenderError::MakeError, "Unsupported GLSL shader stage");
        }
        const auto shader = glCreateShader(stage);
        const auto source = reinterpret_cast<const GLchar*>(data.code.data());
        const auto length = static_cast<GLint>(data.code.size());
        glShaderSource(shader, 1, &source, &length);
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
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
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
        const auto program_key =
            (static_cast<std::uint64_t>(this->state->active_vertex_shader) << 32u) |
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

            this->state->active_program = 0;
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
        this->state->active_program = 0;
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
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            glClearColor(0.25f, 0.25f, 0.25f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
            return;
        }
        if (!this->state->active_framebuffer) {
            glGenFramebuffers(1, &this->state->active_framebuffer);

            this->state->framebuffers.emplace_back(this->state->active_framebuffer);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, this->state->active_framebuffer);
        if (view_type.unwrap() == types::ImageViewType::RenderTarget) {
            glFramebufferTexture2D(
                GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, object->second, 0);
        }
        if (view_type.unwrap() == types::ImageViewType::DepthStencil) {
            glFramebufferTexture2D(
                GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, object->second, 0);
        }
        if (render_target.is_valid() && view_type.unwrap() == types::ImageViewType::DepthStencil) {
            const auto target = this->state->objects.find(render_target);

            if (target != this->state->objects.end()) {
                glFramebufferTexture2D(
                    GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, target->second, 0);
            }
        }
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            return;
        }
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    }
    void OpenGL40Renderer::submit_command_viewport(const types::DrawCommand&) const {
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
        const auto stride =
            index_stride == this->state->index_strides.end() ? 0 : index_stride->second;
        for (const auto& binding : bindings->second) {
            if (const auto indexed = std::get_if<types::DrawIndexedParameter>(&binding.parameter)) {
                glDrawElementsInstanced(this->state->topology,
                    indexed->index_count,
                    this->state->active_index_type,
                    reinterpret_cast<const void*>(
                        static_cast<std::uintptr_t>(indexed->first_index) * stride),
                    std::max(1u, indexed->instance_count));

            } else if (const auto plain = std::get_if<types::DrawParameter>(&binding.parameter)) {
                glDrawArraysInstanced(this->state->topology,
                    plain->first_vertex,
                    plain->vertex_count,
                    std::max(1u, plain->instance_count));
            }
        }
    }
    void OpenGL40Renderer::submit_command_topology(const types::DrawCommand& command) const {
        if (command.sub_command != types::SubCommand::Bind) {
            return;
        }
        const auto value = static_cast<types::PrimitiveTopology>(command.handle.id.handle_id);
        this->state->topology = value == types::PrimitiveTopology::LineList    ? GL_LINES
                                : value == types::PrimitiveTopology::PointList ? GL_POINTS
                                                                               : GL_TRIANGLES;
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
            glDepthFunc(to_gl_compare(it->second.depth.compare_operator));
            return;
        }
        if (const auto it = this->state->blends.find(command.handle);
            it != this->state->blends.end()) {
            const auto& target = it->second.render_targets[0];

            target.enabled ? glEnable(GL_BLEND) : glDisable(GL_BLEND);
            glBlendFuncSeparate(to_gl_blend_factor(target.src_color),
                to_gl_blend_factor(target.dst_color),
                to_gl_blend_factor(target.src_alpha),
                to_gl_blend_factor(target.dst_alpha));
            glBlendEquationSeparate(to_gl_blend_operator(target.color_operator),
                to_gl_blend_operator(target.alpha_operator));
            glColorMask((target.write_mask &
                            static_cast<std::uint8_t>(types::ColorWriteMask::ColorWriteR)) != 0,
                (target.write_mask &
                    static_cast<std::uint8_t>(types::ColorWriteMask::ColorWriteG)) != 0,
                (target.write_mask &
                    static_cast<std::uint8_t>(types::ColorWriteMask::ColorWriteB)) != 0,
                (target.write_mask &
                    static_cast<std::uint8_t>(types::ColorWriteMask::ColorWriteA)) != 0);
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
            glTexParameteri(
                GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, to_gl_address_mode(it->second.address_u));
            glTexParameteri(
                GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, to_gl_address_mode(it->second.address_v));
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
            glDrawElementsInstanced(this->state->topology,
                indexed->index_count,
                this->state->active_index_type,
                reinterpret_cast<const void*>(static_cast<std::uintptr_t>(indexed->first_index)),
                indexed->instance_count);
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
