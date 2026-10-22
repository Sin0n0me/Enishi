#pragma once

#include <renderer/vulkan/device/vulkan_device.h>
#include <renderer/vulkan/runtime/vulkan_renderer_state.h>
#include <renderer/common/resource_accessor.h>
#include <platform/renderer/interface_render_command_encoder.h>
#include <renderer/common/render_handle_mapper.h>

namespace enishi::renderer::vulkan {
    class VulkanRenderer final : public platform::IRenderer,
                                 public platform::IRenderCommandEncoder {
      private:
        std::shared_ptr<VulkanDevice> context;
        std::unique_ptr<RenderHandleMapper> handle_mapper;
        std::unique_ptr<ResourceAccessor> resource_accessor;
        std::unique_ptr<VulkanRendererState> state;

      public:
        explicit VulkanRenderer(std::shared_ptr<VulkanDevice> context);
        ~VulkanRenderer(void) noexcept override;

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
        platform::RenderResult<void> init_shadow_resources();
        platform::RenderResult<void> make_shadow_descriptors(
            VulkanShadowTarget& target, VkImageLayout layout);
        [[nodiscard]] types::RenderHandle make_handle(types::RenderHandleType type) noexcept;
        [[nodiscard]] platform::RenderResult<VkPipeline> active_pipeline() const;
        [[nodiscard]] platform::RenderResult<void> prepare_mesh_sampler(VulkanMesh& mesh) const;
        [[nodiscard]] platform::RenderResult<void> make_mesh_uniforms(
            types::MeshData::UniformMap& uniforms,
            types::MeshHandles::UniformBuffers& named,
            VulkanUniformMap& buffers);
        [[nodiscard]] platform::RenderResult<void> make_materials(
            types::MeshData& data, VulkanMesh& mesh, types::MeshHandles& handles);

      public:
        platform::RenderResult<void> init_resources();
        std::shared_ptr<VulkanDevice> get_native_context() const noexcept;
    };
} // namespace enishi::renderer::vulkan
