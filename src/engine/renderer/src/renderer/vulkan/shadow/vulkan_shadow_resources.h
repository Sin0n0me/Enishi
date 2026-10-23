#pragma once
#include <renderer/vulkan/runtime/vulkan_renderer_state.h>
namespace enishi::renderer::vulkan {
    class VulkanShadowResources final {
      public:
        static platform::RenderResult<void> make_shadow_descriptors(
            VulkanRendererState& state, VulkanShadowTarget& target, VkImageLayout layout);
        static platform::RenderResult<void> init_shadow_resources(VulkanRendererState& state);
    };
} // namespace enishi::renderer::vulkan
