#pragma once
#include <memory>
#include <platform/renderer/interface_renderer.h>
#include <span>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

struct SDL_Window;

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
    struct VulkanImage final {
        std::shared_ptr<VulkanDevice> owner;
        VkImage image = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
        ~VulkanImage();
    };

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

      private:
        bool main_recording = false;
        glm::vec4 frame_clear_color{};
        platform::RenderResult<void> make_shadow_render_pass(VulkanShadowTarget& target);
        SDL_Window* window = nullptr; // Borrowed; the application owns the window.
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        VkSwapchainKHR swapchain = VK_NULL_HANDLE;
        VkFormat color_format = VK_FORMAT_UNDEFINED;
        VkFormat depth_format = VK_FORMAT_D32_SFLOAT;
        VkImage depth_image = VK_NULL_HANDLE;
        VkDeviceMemory depth_memory = VK_NULL_HANDLE;
        VkImageView depth_view = VK_NULL_HANDLE;
        VkCommandPool command_pool = VK_NULL_HANDLE;
        VkSemaphore acquired = VK_NULL_HANDLE;
        VkSemaphore finished = VK_NULL_HANDLE;
        VkFence submitted = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT messenger = VK_NULL_HANDLE;
        std::uint32_t validation_errors = 0;
        std::uint32_t image_index = 0;
        std::vector<VkImageView> views;
        std::vector<VkImage> swapchain_images;
        bool can_readback = false;
        std::vector<VkFramebuffer> framebuffers;
        platform::RenderResult<void> make_instance(bool validation);
        platform::RenderResult<void> select_device();
        platform::RenderResult<void> make_commands();
        platform::RenderResult<void> make_swapchain();
        platform::RenderResult<void> make_framebuffers();
        platform::RenderResult<void> upload_texture(const VulkanImage& image,
            const VulkanBuffer& staging,
            std::span<const VkBufferImageCopy> copies,
            const VkImageSubresourceRange& range);
        platform::RenderResult<void> make_render_pass();
        platform::RenderResult<void> make_depth();
        void destroy_swapchain();
        platform::RenderResult<void> record_readback(const VulkanBuffer& buffer);
        platform::RenderResult<std::uint32_t> memory_type(
            std::uint32_t mask, VkMemoryPropertyFlags flags) const;
        platform::RenderResult<void> allocate_image(VkImage image, VkDeviceMemory& memory);
        static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
            VkDebugUtilsMessageSeverityFlagBitsEXT severity,
            VkDebugUtilsMessageTypeFlagsEXT type,
            const VkDebugUtilsMessengerCallbackDataEXT* data,
            void* user);
    };
    platform::RenderResult<void> check_vk(VkResult result, const char* operation);
} // namespace enishi::renderer::vulkan
