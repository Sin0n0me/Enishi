#pragma once
#include <renderer/common/render_handle_mapper.h>
#include <renderer/common/resource_accessor.h>
#include <renderer/vulkan/runtime/vulkan_renderer_state.h>
namespace enishi::renderer::vulkan {
    class VulkanRenderStates final {
      public:
        static platform::RenderResult<types::RenderHandle> create_rasterizer(
            VulkanRendererState& state,
            ResourceAccessor& resource_accessor,
            RenderHandleMapper& handle_mapper,
            const types::RasterizerStateDescription& value);
        static platform::RenderResult<types::RenderHandle> create_depth_stencil(
            VulkanRendererState& state,
            ResourceAccessor& resource_accessor,
            RenderHandleMapper& handle_mapper,
            const types::DepthStencilStateDescription& value);
        static platform::RenderResult<types::RenderHandle> create_blend(VulkanRendererState& state,
            ResourceAccessor& resource_accessor,
            RenderHandleMapper& handle_mapper,
            const types::BlendStateDescription& value);
        static platform::RenderResult<types::RenderHandle> create_sampler(
            VulkanRendererState& state,
            ResourceAccessor& resource_accessor,
            RenderHandleMapper& handle_mapper,
            const types::SamplerStateDescription& value);
    };
} // namespace enishi::renderer::vulkan
