#pragma once
#include "vulkan_result.h"
#include <memory>
#include <platform/renderer/interface_renderer.h>
#include <renderer/vulkan/buffer/vulkan_buffer.h>
#include <renderer/vulkan/shadow/vulkan_shadow_target.h>
#include <renderer/vulkan/texture/vulkan_image.h>
#include <span>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

struct SDL_Window;

namespace enishi::renderer::vulkan {
    class VulkanSwapchain;

    // One frame in flight: present waits for the queue before CPU-side morph/bone updates.
    class VulkanDevice final : public std::enable_shared_from_this<VulkanDevice> {
      public:
        VkInstance instance = VK_NULL_HANDLE;
        VkPhysicalDevice physical = VK_NULL_HANDLE;
        VkDevice device = VK_NULL_HANDLE;
        VkQueue queue = VK_NULL_HANDLE;
        std::uint32_t queue_family = 0;
        VkRenderPass render_pass = VK_NULL_HANDLE;
        VkCommandBuffer command = VK_NULL_HANDLE;
        VkExtent2D extent{};
        std::uint32_t image_count = 0;
        bool recording = false;
        bool shadow_recording = false;

      private:
        std::unique_ptr<VulkanSwapchain> swapchain;
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        VkCommandPool command_pool = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT messenger = VK_NULL_HANDLE;
        std::uint32_t validation_errors = 0;

      public:
        VulkanDevice();
        ~VulkanDevice();
        VulkanDevice(const VulkanDevice&) = delete;
        VulkanDevice& operator=(const VulkanDevice&) = delete;
        platform::RenderResult<void> init(SDL_Window* window, bool validation);
        platform::RenderResult<std::shared_ptr<VulkanBuffer>> make_buffer(
            VkDeviceSize size, VkBufferUsageFlags usage);
        platform::RenderResult<std::shared_ptr<VulkanImage>> make_texture(
            const types::TextureData& texture);
        platform::RenderResult<std::shared_ptr<VulkanShadowTarget>> make_shadow_target(
            VkExtent2D size);
        platform::RenderResult<void> begin_frame(glm::vec4 clear_color);
        platform::RenderResult<void> begin_shadow_pass(
            const VulkanShadowTarget& target, float clear_depth);
        void begin_main_pass();
        void end_active_pass();
        platform::RenderResult<void> end_frame(const std::shared_ptr<VulkanBuffer>& readback = {});
        platform::RenderResult<void> wait_idle();
        std::uint32_t validation_error_count() const noexcept;
        platform::RenderResult<void> allocate_image(VkImage image, VkDeviceMemory& memory);

      private:
        static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
            VkDebugUtilsMessageSeverityFlagBitsEXT severity,
            VkDebugUtilsMessageTypeFlagsEXT type,
            const VkDebugUtilsMessengerCallbackDataEXT* data,
            void* user);
        platform::RenderResult<void> make_instance(bool validation);
        platform::RenderResult<void> select_device();
        platform::RenderResult<void> make_commands();
        platform::RenderResult<std::uint32_t> memory_type(
            std::uint32_t mask, VkMemoryPropertyFlags flags) const;
    };
} // namespace enishi::renderer::vulkan
