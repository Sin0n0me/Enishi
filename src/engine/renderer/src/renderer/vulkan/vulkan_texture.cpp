#include "vulkan_device.h"
#include <algorithm>
#include <bit>
#include <cstring>
#include <limits>

#define ENISHI_VK_TRY(call)                                                                        \
    do {                                                                                           \
        auto checked = check_vk((call), #call);                                                    \
        if (checked.is_err()) {                                                                    \
            return std::move(checked).take_err();                                                  \
        }                                                                                          \
    } while (false)

namespace enishi::renderer::vulkan {
    namespace {
        VkFormat texture_format(types::TextureFormat format) {
            switch (format) {
                case types::TextureFormat::R8_UNORM:
                    return VK_FORMAT_R8_UNORM;
                case types::TextureFormat::RG8_UNORM:
                    return VK_FORMAT_R8G8_UNORM;
                case types::TextureFormat::RGBA8_UNORM:
                    return VK_FORMAT_R8G8B8A8_UNORM;
                case types::TextureFormat::RGBA8_SRGB:
                    return VK_FORMAT_R8G8B8A8_SRGB;
                case types::TextureFormat::RGBA16_FLOAT:
                    return VK_FORMAT_R16G16B16A16_SFLOAT;
                case types::TextureFormat::BC1_UNORM:
                    return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
                case types::TextureFormat::BC3_UNORM:
                    return VK_FORMAT_BC3_UNORM_BLOCK;
                case types::TextureFormat::BC4_UNORM:
                    return VK_FORMAT_BC4_UNORM_BLOCK;
                case types::TextureFormat::BC5_UNORM:
                    return VK_FORMAT_BC5_UNORM_BLOCK;
                case types::TextureFormat::BC7_UNORM:
                    return VK_FORMAT_BC7_UNORM_BLOCK;
                case types::TextureFormat::BC7_SRGB:
                    return VK_FORMAT_BC7_SRGB_BLOCK;
            }
            return VK_FORMAT_UNDEFINED;
        }
        platform::RenderResult<std::vector<std::byte>> pack_mips(
            const types::TextureData& texture, std::vector<VkBufferImageCopy>& copies) {
            std::vector<std::byte> bytes;
            constexpr std::uint32_t BC_BLOCK_DIMENSION = 4;
            constexpr std::size_t COPY_ALIGNMENT = 16;
            const bool compressed = types::TextureData::is_compressed(texture.format);
            const auto unit_bytes = compressed
                                        ? types::TextureData::block_size(texture.format)
                                        : types::TextureData::bytes_per_pixel(texture.format);
            const auto block = compressed ? BC_BLOCK_DIMENSION : 1u;
            auto expected_width = texture.width;
            auto expected_height = texture.height;
            for (std::uint32_t level = 0; level < texture.mips.size(); ++level) {
                const auto& mip = texture.mips[level];
                if (mip.width != expected_width || mip.height != expected_height ||
                    unit_bytes == 0) {
                    return foundation::Error(
                        platform::RenderError::MakeError, "Invalid Vulkan texture mip dimensions");
                }
                // Block rows = ceil(height / block); bytes per row = ceil(width / block) *
                // unit_bytes.
                const std::size_t rows = (static_cast<std::size_t>(mip.height) + block - 1) / block;
                const std::size_t row_bytes =
                    ((static_cast<std::size_t>(mip.width) + block - 1) / block) * unit_bytes;
                const auto pitch = mip.row_pitch == 0 ? row_bytes : mip.row_pitch;
                if (pitch < row_bytes || rows * pitch > mip.pixels.size()) {
                    return foundation::Error(platform::RenderError::MakeError,
                        "Invalid Vulkan texture mip row pitch/data size");
                }
                // Each copy offset must be a multiple of both four bytes and the compressed block
                // size.
                const auto offset =
                    (bytes.size() + COPY_ALIGNMENT - 1) / COPY_ALIGNMENT * COPY_ALIGNMENT;
                bytes.resize(offset + row_bytes * rows);
                for (std::size_t row = 0; row < rows; ++row) {
                    std::memcpy(bytes.data() + offset + row * row_bytes,
                        mip.pixels.data() + row * pitch,
                        row_bytes);
                }
                VkBufferImageCopy copy{};
                copy.bufferOffset = offset;
                copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, level, 0, 1};
                copy.imageExtent = {mip.width, mip.height, 1};
                copies.push_back(copy);
                expected_width = (std::max)(1u, expected_width / 2);
                expected_height = (std::max)(1u, expected_height / 2);
            }
            return bytes;
        }
    } // namespace
    platform::RenderResult<std::shared_ptr<VulkanImage>> VulkanDevice::make_texture(
        const types::TextureData& texture) {
        if (this->recording || texture.is_cubemap || texture.array_size != 1 || texture.depth > 1 ||
            texture.width == 0 || texture.height == 0 || texture.mips.empty()) {
            return foundation::Error(platform::RenderError::MakeError,
                "Vulkan textures must be 2D and uploaded outside a frame");
        }
        const auto format = texture_format(texture.format);
        VkPhysicalDeviceProperties device_properties{};
        vkGetPhysicalDeviceProperties(this->physical, &device_properties);
        if (texture.width > device_properties.limits.maxImageDimension2D ||
            texture.height > device_properties.limits.maxImageDimension2D ||
            texture.mips.size() > std::bit_width((std::max)(texture.width, texture.height))) {
            return foundation::Error(platform::RenderError::MakeError,
                "Vulkan texture dimensions or mip count exceed device limits");
        }
        if (types::TextureData::is_compressed(texture.format)) {
            VkPhysicalDeviceFeatures features{};
            vkGetPhysicalDeviceFeatures(this->physical, &features);
            if (features.textureCompressionBC != VK_TRUE) {
                return foundation::Error(platform::RenderError::MakeError,
                    "Vulkan device does not support BC texture compression");
            }
        }
        VkFormatProperties properties{};
        vkGetPhysicalDeviceFormatProperties(this->physical, format, &properties);
        constexpr auto REQUIRED =
            VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;
        if ((properties.optimalTilingFeatures & REQUIRED) != REQUIRED) {
            return foundation::Error(
                platform::RenderError::MakeError, "Unsupported Vulkan sampled texture format");
        }
        std::vector<VkBufferImageCopy> copies;
        auto packed = pack_mips(texture, copies);
        if (packed.is_err()) {
            return std::move(packed).take_err();
        }
        auto staging_result =
            this->make_buffer(packed.unwrap().size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
        if (staging_result.is_err()) {
            return std::move(staging_result).take_err();
        }
        auto staging = staging_result.unwrap();
        std::memcpy(staging->mapped, packed.unwrap().data(), packed.unwrap().size());
        auto image = std::make_shared<VulkanImage>();
        image->owner = this->shared_from_this();
        VkImageCreateInfo info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        info.imageType = VK_IMAGE_TYPE_2D;
        info.format = format;
        info.extent = {texture.width, texture.height, 1};
        info.mipLevels = static_cast<std::uint32_t>(texture.mips.size());
        info.arrayLayers = 1;
        info.samples = VK_SAMPLE_COUNT_1_BIT;
        info.tiling = VK_IMAGE_TILING_OPTIMAL;
        info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        ENISHI_VK_TRY(vkCreateImage(this->device, &info, nullptr, &image->image));
        auto allocated = this->allocate_image(image->image, image->memory);
        if (allocated.is_err()) {
            return std::move(allocated).take_err();
        }
        VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        view.image = image->image;
        view.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view.format = format;
        view.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, info.mipLevels, 0, 1};
        ENISHI_VK_TRY(vkCreateImageView(this->device, &view, nullptr, &image->view));
        auto uploaded = this->upload_texture(*image, *staging, copies, view.subresourceRange);
        if (uploaded.is_err()) {
            return std::move(uploaded).take_err();
        }
        return image;
    }
    platform::RenderResult<void> VulkanDevice::upload_texture(const VulkanImage& image,
        const VulkanBuffer& staging,
        std::span<const VkBufferImageCopy> copies,
        const VkImageSubresourceRange& range) {
        ENISHI_VK_TRY(vkQueueWaitIdle(this->queue));
        ENISHI_VK_TRY(vkResetCommandBuffer(this->command, 0));
        VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        ENISHI_VK_TRY(vkBeginCommandBuffer(this->command, &begin));
        VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.image = image.image;
        barrier.subresourceRange = range;
        vkCmdPipelineBarrier(this->command,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &barrier);
        vkCmdCopyBufferToImage(this->command,
            staging.buffer,
            image.image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            static_cast<std::uint32_t>(copies.size()),
            copies.data());
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(this->command,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &barrier);
        ENISHI_VK_TRY(vkEndCommandBuffer(this->command));
        VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &this->command;
        ENISHI_VK_TRY(vkQueueSubmit(this->queue, 1, &submit, VK_NULL_HANDLE));
        ENISHI_VK_TRY(vkQueueWaitIdle(this->queue));
        return {};
    }
} // namespace enishi::renderer::vulkan
