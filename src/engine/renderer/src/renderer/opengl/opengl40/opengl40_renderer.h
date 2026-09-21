#pragma once

#include <cstdint>
#include <memory>
#include <platform/renderer/interface_opengl_context.h>
#include <platform/renderer/interface_render_command_encoder.h>
#include <platform/renderer/interface_renderer.h>
#include <renderer/common/render_handle_mapper.h>
#include <renderer/opengl/common/glsl_shader_reflection.h>
#include <renderer/opengl/common/opengl_resource_accessor.h>
#include <renderer/opengl/common/opengl_uniform_updater.h>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace enishi::renderer::opengl {
    class OpenGL40Renderer final : public platform::IRenderer,
                                   public platform::IRenderCommandEncoder {
      private:
        class State {
          public:
            std::unordered_map<types::RenderHandle, std::uint32_t> objects;
            std::unordered_map<types::RenderHandle, types::ShaderKind> shader_kinds;
            std::unordered_map<types::RenderHandle, types::DrawBinding> draw_bindings;
            std::unordered_map<types::RenderHandle, std::uint32_t> index_types;
            std::unordered_map<types::RenderHandle, std::uint32_t> index_strides;
            std::unordered_map<types::RenderHandle, std::vector<types::DrawBinding>>
                mesh_draw_bindings;
            std::unordered_map<types::RenderHandle, types::ImageDescription> images;
            std::unordered_set<types::RenderHandle> back_buffer_images;
            std::unordered_map<types::RenderHandle, types::RasterizerStateDescription> rasterizers;
            std::unordered_map<types::RenderHandle, types::DepthStencilStateDescription>
                depth_stencils;
            std::unordered_map<types::RenderHandle, types::BlendStateDescription> blends;
            std::unordered_map<types::RenderHandle, types::SamplerStateDescription> samplers;
            std::unordered_map<types::RenderHandle, std::shared_ptr<GLSLShaderReflection>>
                reflections;
            std::unordered_map<std::string, std::uint32_t> uniform_block_bindings;
            std::vector<std::shared_ptr<OpenGLUniformUpdater>> uniform_updaters;
            std::unordered_map<types::RenderHandle, std::vector<types::HandleId>>
                mesh_uniform_buffers;
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

            State(void);
        };

        std::shared_ptr<platform::IOpenGLContext> context;
        std::unique_ptr<RenderHandleMapper> handle_mapper;
        std::unique_ptr<OpenGLResourceAccessor> resource_accessor;
        std::unique_ptr<State> state;

      public:
        explicit OpenGL40Renderer(std::shared_ptr<platform::IOpenGLContext> context);
        ~OpenGL40Renderer(void) noexcept override;

        platform::RenderResult<types::RenderHandle> create_viewport(
            const types::ViewportRect&) override;
        platform::RenderResult<types::RenderHandle> create_shader_reflection(
            const types::ShaderData&) override;
        platform::RenderResult<std::unique_ptr<platform::IPipelineLayout>> create_vertex_layout(
            const types::VertexLayout&,
            const types::RenderHandle&,
            const types::RenderHandle&) override;
        platform::RenderResult<types::RenderHandle> create_vertex_layout_from_shader_data(
            const types::ShaderData&) override;
        platform::RenderResult<types::RenderHandle> create_rasterizer(
            const types::RasterizerStateDescription&) override;
        platform::RenderResult<types::RenderHandle> create_depth_stencil(
            const types::DepthStencilStateDescription&) override;
        platform::RenderResult<types::RenderHandle> create_sampler(
            const types::SamplerStateDescription&) override;
        platform::RenderResult<types::RenderHandle> create_blend(
            const types::BlendStateDescription&) override;
        platform::RenderResult<types::RenderHandle> create_image(
            const types::ImageDescription&) override;
        platform::RenderResult<std::shared_ptr<platform::IRenderTargetView>>
        create_render_target_view(types::RenderHandle, const types::ImageViewDescription&) override;
        platform::RenderResult<std::shared_ptr<platform::IDepthStencilView>>
        create_depth_stencil_view(types::RenderHandle, const types::ImageViewDescription&) override;
        platform::RenderResult<std::shared_ptr<platform::IShaderResourceView>>
        create_shader_resource_view(
            types::RenderHandle, const types::ImageViewDescription&) override;
        platform::RenderResult<std::shared_ptr<platform::IUnorderedAccessView>>
        create_unordered_access_view(
            types::RenderHandle, const types::ImageViewDescription&) override;
        platform::RenderResult<types::RenderHandle> create_mesh(
            const types::ModelData&, const std::vector<types::RenderHandle>&) override;
        platform::RenderResult<types::RenderHandle> create_texture(
            const types::TextureData&) override;
        platform::RenderResult<types::RenderHandle> create_shader(
            types::ShaderKind, const types::ShaderData&) override;
        platform::IRenderResourceAccessor* get_resource_accessor(void) noexcept override;
        platform::IRenderResourceAccessor* const get_resource_accessor(
            void) const noexcept override;
        const platform::IRenderHandleMapper* get_handle_mapper(void) const noexcept override;

        void setup_viewports(void) const override;
        void setup_views(void) const override;
        void submit_command_buffer(const types::DrawCommand&) const override;
        void submit_command_shader(const types::DrawCommand&) const override;
        void submit_command_view(
            const types::DrawCommand&, const types::RenderHandle&) const override;
        void submit_command_viewport(const types::DrawCommand&) const override;
        void submit_command_mesh(
            const types::DrawCommand&, const types::RenderHandle&) const override;
        void submit_command_topology(const types::DrawCommand&) const override;
        void submit_command_vertex_layout(const types::DrawCommand&) const override;
        void submit_command_state(const types::DrawCommand&) const override;
        void submit_command_image(const types::DrawCommand&) const override;
        void draw(const types::RenderHandle&) const override;
        void present(void) const override;

      private:
        [[nodiscard]] types::RenderHandle make_handle(types::RenderHandleType type) noexcept;
        [[nodiscard]] platform::RenderResult<types::RenderHandle> make_buffer(
            const types::RenderData& data, std::uint32_t target);
        [[nodiscard]] bool use_active_program(void) const;
    };
} // namespace enishi::renderer::opengl
