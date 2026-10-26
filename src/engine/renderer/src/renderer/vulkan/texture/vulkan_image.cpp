#include "vulkan_image.h"
#include <renderer/vulkan/device/vulkan_device.h>
namespace enishi::renderer::vulkan {
    VulkanImage::~VulkanImage() {
        if (this->owner == nullptr) {
            return;
        }
        vkDestroyImageView(this->owner->device, this->view, nullptr);
        vkDestroyImage(this->owner->device, this->image, nullptr);
        vkFreeMemory(this->owner->device, this->memory, nullptr);
    }
} // namespace enishi::renderer::vulkan
