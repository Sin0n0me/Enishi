#include <array>
#include <renderer/vulkan/shadow/vulkan_shadow_resources.h>
namespace enishi::renderer::vulkan {
    platform::RenderResult<void> VulkanShadowResources::make_shadow_descriptors(
        VulkanRendererState& state, VulkanShadowTarget& target, VkImageLayout layout) {
        const VkDescriptorPoolSize size{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1};
        VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        pool.maxSets = 1;
        pool.poolSizeCount = 1;
        pool.pPoolSizes = &size;
        auto result =
            check_vk(vkCreateDescriptorPool(state.context->device, &pool, nullptr, &target.pool),
                "create shadow descriptor pool");
        if (result.is_err()) {
            return result;
        }
        VkDescriptorSetAllocateInfo allocation{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        allocation.descriptorPool = target.pool;
        allocation.descriptorSetCount = 1;
        allocation.pSetLayouts = &state.shadow_descriptors;
        result = check_vk(
            vkAllocateDescriptorSets(state.context->device, &allocation, &target.descriptors),
            "allocate shadow descriptors");
        if (result.is_err()) {
            return result;
        }
        const VkDescriptorImageInfo image{state.shadow_sampler, target.image->view, layout};
        VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        write.dstSet = target.descriptors;
        write.dstBinding = 0;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.pImageInfo = &image;
        vkUpdateDescriptorSets(state.context->device, 1, &write, 0, nullptr);
        return {};
    }
    platform::RenderResult<void> VulkanShadowResources::init_shadow_resources(
        VulkanRendererState& state) {
        VkDescriptorSetLayoutBinding binding{};
        binding.binding = 0;
        binding.descriptorCount = 1;
        binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        VkDescriptorSetLayoutCreateInfo layout{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        layout.bindingCount = 1;
        layout.pBindings = &binding;
        auto result =
            check_vk(vkCreateDescriptorSetLayout(
                         state.context->device, &layout, nullptr, &state.shadow_descriptors),
                "create shadow descriptor layout");
        if (result.is_err()) {
            return result;
        }
        // Manual PCF uses texelFetch; nearest sampling avoids requiring D32 linear filtering.
        VkSamplerCreateInfo sampler{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        sampler.magFilter = VK_FILTER_NEAREST;
        sampler.minFilter = VK_FILTER_NEAREST;
        sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        result = check_vk(
            vkCreateSampler(state.context->device, &sampler, nullptr, &state.shadow_sampler),
            "create shadow sampler");
        if (result.is_err()) {
            return result;
        }
        // An all-lit fallback keeps meshes usable without a shadow pass and avoids attachment
        // feedback.
        constexpr std::uint8_t FULL_DEPTH = 255;
        types::TextureData texture{};
        texture.format = types::TextureFormat::R8_UNORM;
        texture.width = texture.height = texture.depth = texture.array_size = 1;
        auto& mip = texture.mips.emplace_back();
        mip.width = mip.height = mip.row_pitch = mip.slice_pitch = 1;
        mip.pixels = {FULL_DEPTH};
        auto image = state.context->make_texture(texture);
        if (image.is_err()) {
            return std::move(image).take_err();
        }
        state.fallback_shadow = std::make_shared<VulkanShadowTarget>();
        auto& target = *state.fallback_shadow;
        target.owner = state.context;
        target.image = image.unwrap();
        return VulkanShadowResources::make_shadow_descriptors(
            state, target, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
} // namespace enishi::renderer::vulkan
