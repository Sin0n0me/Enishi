#pragma once

#include <cstdint>
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
        std::unordered_map<types::RenderHandle, std::uint32_t> objects;
        std::unordered_map<types::RenderHandle, types::ViewportRect> viewports;
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
        std::vector<std::uint32_t> buffers;
        std::vector<std::uint32_t> textures;
        std::vector<std::uint32_t> vertex_arrays;
        std::vector<std::uint32_t> shaders;
        std::unordered_map<std::uint64_t, std::uint32_t> programs;
        std::vector<std::uint32_t> framebuffers;
        std::uint32_t topology;
        std::uint32_t active_vertex_shader;
        std::uint32_t active_fragment_shader;
        std::uint32_t active_program;
        std::uint32_t active_framebuffer;
        std::uint32_t active_index_type;

        OpenGL40RendererState(void);
    };
} // namespace enishi::renderer::opengl
