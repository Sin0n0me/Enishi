#pragma once
#include <memory>
#include <vulkan/vulkan.h>
namespace enishi::renderer::vulkan {
    class VulkanDevice;
    struct VulkanImage final {
        std::shared_ptr<VulkanDevice> owner;
        VkImage image = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
        ~VulkanImage();
    };
} // namespace enishi::renderer::vulkan
