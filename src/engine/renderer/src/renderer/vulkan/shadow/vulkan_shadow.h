#pragma once
#include <renderer/vulkan/device/vulkan_device.h>
namespace enishi::renderer::vulkan {
    class VulkanShadow final {
      public:
        static platform::RenderResult<std::shared_ptr<VulkanShadowTarget>> make_shadow_target(
            VulkanDevice& context, VkExtent2D size);

      private:
        static platform::RenderResult<void> make_shadow_render_pass(
            VulkanDevice& context, VulkanShadowTarget& target);
    };
} // namespace enishi::renderer::vulkan
