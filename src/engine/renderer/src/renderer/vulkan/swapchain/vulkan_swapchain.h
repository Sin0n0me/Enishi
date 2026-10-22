#pragma once
#include <renderer/vulkan/device/vulkan_device.h>
namespace enishi::renderer::vulkan {
    class VulkanSwapchain final {
      private:
        VulkanDevice& context;
        bool main_recording = false;
        glm::vec4 frame_clear_color{};
        SDL_Window* window; // Borrowed; the application owns the window.
        VkSurfaceKHR surface;
        VkSwapchainKHR swapchain = VK_NULL_HANDLE;
        VkFormat color_format = VK_FORMAT_UNDEFINED;
        VkFormat depth_format = VK_FORMAT_D32_SFLOAT;
        VkImage depth_image = VK_NULL_HANDLE;
        VkDeviceMemory depth_memory = VK_NULL_HANDLE;
        VkImageView depth_view = VK_NULL_HANDLE;
        VkSemaphore acquired = VK_NULL_HANDLE;
        VkSemaphore finished = VK_NULL_HANDLE;
        VkFence submitted = VK_NULL_HANDLE;
        std::uint32_t image_index = 0;
        std::vector<VkImageView> views;
        std::vector<VkImage> swapchain_images;
        bool can_readback = false;
        std::vector<VkFramebuffer> framebuffers;

      public:
        VulkanSwapchain(VulkanDevice& context, SDL_Window* window, VkSurfaceKHR surface);
        ~VulkanSwapchain();
        VulkanSwapchain(const VulkanSwapchain&) = delete;
        VulkanSwapchain& operator=(const VulkanSwapchain&) = delete;
        platform::RenderResult<void> init();
        platform::RenderResult<void> begin_frame(glm::vec4 clear_color);
        void end_active_pass();
        void begin_main_pass();
        platform::RenderResult<void> end_frame(const std::shared_ptr<VulkanBuffer>& readback);
        platform::RenderResult<void> begin_shadow_pass(
            const VulkanShadowTarget& target, float clear_depth);

      private:
        void destroy_swapchain();
        platform::RenderResult<void> make_depth();
        platform::RenderResult<void> make_render_pass();
        platform::RenderResult<void> make_swapchain();
        platform::RenderResult<void> make_framebuffers();
        platform::RenderResult<void> record_readback(const VulkanBuffer& buffer);
    };
} // namespace enishi::renderer::vulkan
