#pragma once
#include <renderer/common/render_handle_mapper.h>
#include <renderer/common/resource_accessor.h>
#include <renderer/vulkan/runtime/vulkan_renderer_state.h>
namespace enishi::renderer::vulkan {
    class VulkanViews final {
      public:
        static platform::RenderResult<types::RenderHandle> create_image(VulkanRendererState& state,
            RenderHandleMapper& handle_mapper,
            const types::ImageDescription& description);
        static platform::RenderResult<std::shared_ptr<platform::IRenderTargetView>>
        create_render_target_view(VulkanRendererState& state,
            ResourceAccessor& resource_accessor,
            RenderHandleMapper& handle_mapper,
            types::RenderHandle image,
            const types::ImageViewDescription& description);
        static platform::RenderResult<std::shared_ptr<platform::IDepthStencilView>>
        create_depth_stencil_view(VulkanRendererState& state,
            ResourceAccessor& resource_accessor,
            RenderHandleMapper& handle_mapper,
            types::RenderHandle image,
            const types::ImageViewDescription& description);
        static platform::RenderResult<std::shared_ptr<platform::IShaderResourceView>>
        create_shader_resource_view(VulkanRendererState& state,
            ResourceAccessor& resource_accessor,
            RenderHandleMapper& handle_mapper,
            types::RenderHandle image,
            const types::ImageViewDescription& description);
        static platform::RenderResult<std::shared_ptr<platform::IUnorderedAccessView>>
        create_unordered_access_view(types::RenderHandle, const types::ImageViewDescription&);
    };
} // namespace enishi::renderer::vulkan
