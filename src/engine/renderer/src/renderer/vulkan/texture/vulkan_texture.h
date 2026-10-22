#pragma once
#include <renderer/vulkan/device/vulkan_device.h>
namespace enishi::renderer::vulkan {
    class VulkanTexture final {
      public:
        static platform::RenderResult<std::shared_ptr<VulkanImage>> make_texture(
            VulkanDevice& context, const types::TextureData& texture);

      private:
        static platform::RenderResult<void> upload_texture(VulkanDevice& context,
            const VulkanImage& image,
            const VulkanBuffer& staging,
            std::span<const VkBufferImageCopy> copies,
            const VkImageSubresourceRange& range);
    };
} // namespace enishi::renderer::vulkan
