#include "vulkan_renderer.h"
#include <array>

namespace enishi::renderer::vulkan {
    VulkanShadowTarget::~VulkanShadowTarget() {
        if (this->owner != nullptr) {
            vkDestroyDescriptorPool(this->owner->device, this->pool, nullptr);
            vkDestroyFramebuffer(this->owner->device, this->framebuffer, nullptr);
            vkDestroyRenderPass(this->owner->device, this->render_pass, nullptr);
        }
    }
    platform::RenderResult<void> VulkanDevice::make_shadow_render_pass(VulkanShadowTarget& target) {
        VkAttachmentDescription attachment{};
        attachment.format = VK_FORMAT_D32_SFLOAT;
        attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        VkAttachmentReference depth{0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.pDepthStencilAttachment = &depth;
        // Depth writes occur in both early/late tests; make them visible to the receiver fragment
        // shader.
        constexpr VkPipelineStageFlags DEPTH_STAGES =
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        std::array<VkSubpassDependency, 2> dependencies{};
        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[0].dstStageMask = DEPTH_STAGES;
        dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependencies[1].srcSubpass = 0;
        dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[1].srcStageMask = DEPTH_STAGES;
        dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        VkRenderPassCreateInfo pass{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
        pass.attachmentCount = 1;
        pass.pAttachments = &attachment;
        pass.subpassCount = 1;
        pass.pSubpasses = &subpass;
        pass.dependencyCount = static_cast<std::uint32_t>(dependencies.size());
        pass.pDependencies = dependencies.data();
        auto result =
            check_vk(vkCreateRenderPass(this->device, &pass, nullptr, &target.render_pass),
                "create shadow render pass");
        if (result.is_err()) {
            return result;
        }
        VkFramebufferCreateInfo framebuffer{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        framebuffer.renderPass = target.render_pass;
        framebuffer.attachmentCount = 1;
        framebuffer.pAttachments = &target.image->view;
        framebuffer.width = target.extent.width;
        framebuffer.height = target.extent.height;
        framebuffer.layers = 1;
        return check_vk(
            vkCreateFramebuffer(this->device, &framebuffer, nullptr, &target.framebuffer),
            "create shadow framebuffer");
    }
    platform::RenderResult<std::shared_ptr<VulkanShadowTarget>> VulkanDevice::make_shadow_target(
        VkExtent2D size) {
        VkFormatProperties properties{};
        vkGetPhysicalDeviceFormatProperties(this->physical, VK_FORMAT_D32_SFLOAT, &properties);
        constexpr VkFormatFeatureFlags REQUIRED =
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
        VkPhysicalDeviceProperties limits{};
        vkGetPhysicalDeviceProperties(this->physical, &limits);
        if ((properties.optimalTilingFeatures & REQUIRED) != REQUIRED || size.width == 0 ||
            size.height == 0 || size.width > limits.limits.maxFramebufferWidth ||
            size.height > limits.limits.maxFramebufferHeight ||
            size.width > limits.limits.maxImageDimension2D ||
            size.height > limits.limits.maxImageDimension2D) {
            return foundation::Error(
                platform::RenderError::MakeError, "Unsupported Vulkan sampled depth image");
        }
        auto target = std::make_shared<VulkanShadowTarget>();
        target->owner = this->shared_from_this();
        target->extent = size;
        target->image = std::make_shared<VulkanImage>();
        target->image->owner = this->shared_from_this();
        VkImageCreateInfo image{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        image.imageType = VK_IMAGE_TYPE_2D;
        image.format = VK_FORMAT_D32_SFLOAT;
        image.extent = {size.width, size.height, 1};
        image.mipLevels = 1;
        image.arrayLayers = 1;
        image.samples = VK_SAMPLE_COUNT_1_BIT;
        image.tiling = VK_IMAGE_TILING_OPTIMAL;
        image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        auto result = check_vk(vkCreateImage(this->device, &image, nullptr, &target->image->image),
            "create shadow image");
        if (result.is_err()) {
            return std::move(result).take_err();
        }
        result = this->allocate_image(target->image->image, target->image->memory);
        if (result.is_err()) {
            return std::move(result).take_err();
        }
        VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        view.image = target->image->image;
        view.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view.format = VK_FORMAT_D32_SFLOAT;
        view.subresourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1};
        result = check_vk(vkCreateImageView(this->device, &view, nullptr, &target->image->view),
            "create shadow image view");
        if (result.is_err()) {
            return std::move(result).take_err();
        }
        result = this->make_shadow_render_pass(*target);
        if (result.is_err()) {
            return std::move(result).take_err();
        }
        return target;
    }
    platform::RenderResult<void> VulkanDevice::begin_shadow_pass(
        const VulkanShadowTarget& target, float clear_depth) {
        if (!this->recording) {
            return {};
        }
        if (this->main_recording || this->shadow_recording) {
            return foundation::Error(platform::RenderError::MakeError,
                "Shadow depth must be rendered before the main pass");
        }
        VkClearValue clear{};
        clear.depthStencil = {clear_depth, 0};
        VkRenderPassBeginInfo pass{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        pass.renderPass = target.render_pass;
        pass.framebuffer = target.framebuffer;
        pass.renderArea.extent = target.extent;
        pass.clearValueCount = 1;
        pass.pClearValues = &clear;
        vkCmdBeginRenderPass(this->command, &pass, VK_SUBPASS_CONTENTS_INLINE);
        const VkViewport viewport{0,
            0,
            static_cast<float>(target.extent.width),
            static_cast<float>(target.extent.height),
            0,
            1};
        const VkRect2D scissor{{0, 0}, target.extent};
        vkCmdSetViewport(this->command, 0, 1, &viewport);
        vkCmdSetScissor(this->command, 0, 1, &scissor);
        this->shadow_recording = true;
        return {};
    }
    platform::RenderResult<void> VulkanRenderer::make_shadow_descriptors(
        VulkanShadowTarget& target, VkImageLayout layout) {
        const VkDescriptorPoolSize size{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1};
        VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        pool.maxSets = 1;
        pool.poolSizeCount = 1;
        pool.pPoolSizes = &size;
        auto result =
            check_vk(vkCreateDescriptorPool(this->context->device, &pool, nullptr, &target.pool),
                "create shadow descriptor pool");
        if (result.is_err()) {
            return result;
        }
        VkDescriptorSetAllocateInfo allocation{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        allocation.descriptorPool = target.pool;
        allocation.descriptorSetCount = 1;
        allocation.pSetLayouts = &this->state->shadow_descriptors;
        result = check_vk(
            vkAllocateDescriptorSets(this->context->device, &allocation, &target.descriptors),
            "allocate shadow descriptors");
        if (result.is_err()) {
            return result;
        }
        const VkDescriptorImageInfo image{this->state->shadow_sampler, target.image->view, layout};
        VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        write.dstSet = target.descriptors;
        write.dstBinding = 0;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.pImageInfo = &image;
        vkUpdateDescriptorSets(this->context->device, 1, &write, 0, nullptr);
        return {};
    }
    platform::RenderResult<void> VulkanRenderer::init_shadow_resources() {
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
                         this->context->device, &layout, nullptr, &this->state->shadow_descriptors),
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
            vkCreateSampler(this->context->device, &sampler, nullptr, &this->state->shadow_sampler),
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
        auto image = this->context->make_texture(texture);
        if (image.is_err()) {
            return std::move(image).take_err();
        }
        this->state->fallback_shadow = std::make_shared<VulkanShadowTarget>();
        auto& target = *this->state->fallback_shadow;
        target.owner = this->context;
        target.image = image.unwrap();
        return this->make_shadow_descriptors(target, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
} // namespace enishi::renderer::vulkan
