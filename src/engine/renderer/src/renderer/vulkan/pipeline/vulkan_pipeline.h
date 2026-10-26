#pragma once
#include <renderer/vulkan/runtime/vulkan_renderer_state.h>
namespace enishi::renderer::vulkan {
    class VulkanPipeline final {
      public:
        static platform::RenderResult<VkPipeline> active_pipeline(VulkanRendererState& state);

      private:
        static VkPipelineRasterizationStateCreateInfo make_rasterizer(
            const VulkanRendererState& state);
    };
} // namespace enishi::renderer::vulkan
