#include "vulkan_shadow_target.h"
#include <renderer/vulkan/device/vulkan_device.h>
namespace enishi::renderer::vulkan {
    VulkanShadowTarget::~VulkanShadowTarget() {
        if (this->owner != nullptr) {
            vkDestroyDescriptorPool(this->owner->device, this->pool, nullptr);
            vkDestroyFramebuffer(this->owner->device, this->framebuffer, nullptr);
            vkDestroyRenderPass(this->owner->device, this->render_pass, nullptr);
        }
    }
} // namespace enishi::renderer::vulkan
