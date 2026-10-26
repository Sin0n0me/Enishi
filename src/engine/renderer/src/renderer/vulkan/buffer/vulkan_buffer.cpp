#include "vulkan_buffer.h"
#include <renderer/vulkan/device/vulkan_device.h>
namespace enishi::renderer::vulkan {
    VulkanBuffer::~VulkanBuffer() {
        if (this->owner == nullptr) {
            return;
        }
        if (this->mapped != nullptr) {
            vkUnmapMemory(this->owner->device, this->memory);
        }
        vkDestroyBuffer(this->owner->device, this->buffer, nullptr);
        vkFreeMemory(this->owner->device, this->memory, nullptr);
    }
} // namespace enishi::renderer::vulkan
