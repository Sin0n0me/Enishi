#pragma once
#include <memory>
#include <vulkan/vulkan.h>
namespace enishi::renderer::vulkan {
    class VulkanDevice;
    struct VulkanBuffer final {
        std::shared_ptr<VulkanDevice> owner;
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        void* mapped = nullptr;
        VkDeviceSize size = 0;
        ~VulkanBuffer();
    };
} // namespace enishi::renderer::vulkan
