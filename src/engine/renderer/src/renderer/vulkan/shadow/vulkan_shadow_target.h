#pragma once
#include <memory>
#include <renderer/vulkan/texture/vulkan_image.h>
#include <vulkan/vulkan.h>
namespace enishi::renderer::vulkan {
    class VulkanDevice;
    struct VulkanShadowTarget final {
        std::shared_ptr<VulkanDevice> owner;
        std::shared_ptr<VulkanImage> image;
        VkRenderPass render_pass = VK_NULL_HANDLE;
        VkFramebuffer framebuffer = VK_NULL_HANDLE;
        VkDescriptorPool pool = VK_NULL_HANDLE;
        VkDescriptorSet descriptors = VK_NULL_HANDLE;
        VkExtent2D extent{};
        ~VulkanShadowTarget();
    };
} // namespace enishi::renderer::vulkan
