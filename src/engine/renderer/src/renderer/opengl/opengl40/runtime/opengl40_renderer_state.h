#pragma once

#include <cstdint>
#include <glad/gl.h>
#include <memory>
#include <platform/renderer/interface_renderer.h>
#include <renderer/opengl/common/glsl_shader_reflection.h>
#include <renderer/opengl/common/opengl_uniform_updater.h>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace enishi::renderer::opengl {
    class OpenGL40RendererState final {
      public:
        std::unordered_map<types::RenderHandle, GLuint> objects;
        std::unordered_map<types::RenderHandle, types::ViewportRect> viewports;
        std::unordered_map<types::RenderHandle, types::ShaderKind> shader_kinds;
        std::unordered_map<types::RenderHandle, types::DrawBinding> draw_bindings;
        std::unordered_map<types::RenderHandle, GLenum> index_types;
        std::unordered_map<types::RenderHandle, GLuint> index_strides;
        std::unordered_map<types::RenderHandle, std::vector<types::DrawBinding>> mesh_draw_bindings;
        std::unordered_map<types::RenderHandle, std::vector<std::unordered_map<GLuint, GLuint>>>
            mesh_texture_bindings;
        std::unordered_map<types::RenderHandle, types::ImageDescription> images;
        std::unordered_map<types::RenderHandle, GLuint> view_texture_units;
        std::unordered_map<types::RenderHandle, GLuint> depth_framebuffers;
        bool active_framebuffer_complete = true;
        bool back_buffer_cleared = false;
        std::unordered_set<types::RenderHandle> back_buffer_images;
        std::unordered_map<types::RenderHandle, types::RasterizerStateDescription> rasterizers;
        std::unordered_map<types::RenderHandle, types::DepthStencilStateDescription> depth_stencils;
        std::unordered_map<types::RenderHandle, types::BlendStateDescription> blends;
        std::unordered_map<types::RenderHandle, types::SamplerStateDescription> samplers;
        std::unordered_map<types::RenderHandle, GLuint> sampler_objects;
        std::unordered_map<types::RenderHandle, std::shared_ptr<GLSLShaderReflection>> reflections;
        std::unordered_map<std::string, GLuint> uniform_block_bindings;
        std::unordered_map<std::string, GLuint> sampler_bindings;
        std::vector<std::shared_ptr<OpenGLUniformUpdater>> uniform_updaters;
        std::unordered_map<types::RenderHandle, std::vector<types::HandleId>> mesh_uniform_buffers;
        std::unordered_map<types::RenderHandle, std::vector<std::vector<types::HandleId>>>
            material_uniform_buffers;
        std::vector<GLuint> buffers;
        std::vector<GLuint> textures;
        std::vector<GLuint> vertex_arrays;
        std::vector<GLuint> shaders;
        std::vector<GLuint> gl_samplers;
        std::unordered_map<std::uint64_t, GLuint> programs;
        std::vector<GLuint> framebuffers;
        GLenum topology;
        GLuint active_vertex_shader;
        GLuint active_fragment_shader;
        GLuint active_program;
        GLuint active_framebuffer;
        GLenum active_index_type;
        GLuint active_sampler;
        GLuint back_buffer_framebuffer;
        GLuint back_buffer_color;
        GLint back_buffer_width;
        GLint back_buffer_height;
        GLint uniform_buffer_binding_limit;
        GLint texture_unit_limit;
        bool is_back_buffer_framebuffer_complete;
        std::vector<types::RenderHandle> viewport_order;

        OpenGL40RendererState(void);
        // Both operations require the owning renderer's context to be current.
        void initialize();
        void release() noexcept;
    };
} // namespace enishi::renderer::opengl
