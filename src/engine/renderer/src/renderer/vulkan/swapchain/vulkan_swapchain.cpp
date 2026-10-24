#include "vulkan_swapchain.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <limits>

#define ENISHI_VK_TRY(call)                                                                        \
    do {                                                                                           \
        auto checked = check_vk((call), #call);                                                    \
        if (checked.is_err()) {                                                                    \
            return std::move(checked).take_err();                                                  \
        }                                                                                          \
    } while (false)

namespace enishi::renderer::vulkan {
    VulkanSwapchain::VulkanSwapchain(
        VulkanDevice& context, SDL_Window* window, VkSurfaceKHR surface)
        : context(context)
        , window(window)
        , surface(surface) {
    }
    VulkanSwapchain::~VulkanSwapchain() {
        this->destroy_swapchain();
        vkDestroyRenderPass(this->context.device, this->context.render_pass, nullptr);
        vkDestroySemaphore(this->context.device, this->acquired, nullptr);
        vkDestroySemaphore(this->context.device, this->finished, nullptr);
        vkDestroyFence(this->context.device, this->submitted, nullptr);
    }
    platform::RenderResult<void> VulkanSwapchain::init() {
        VkSemaphoreCreateInfo semaphore{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        ENISHI_VK_TRY(
            vkCreateSemaphore(this->context.device, &semaphore, nullptr, &this->acquired));
        ENISHI_VK_TRY(
            vkCreateSemaphore(this->context.device, &semaphore, nullptr, &this->finished));
        VkFenceCreateInfo fence{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        fence.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        ENISHI_VK_TRY(vkCreateFence(this->context.device, &fence, nullptr, &this->submitted));
        return this->make_swapchain();
    }
    platform::RenderResult<void> VulkanSwapchain::begin_frame(glm::vec4 clear_color) {
        if ((SDL_GetWindowFlags(this->window) & SDL_WINDOW_MINIMIZED) != 0) {
            return {};
        }
        if (this->swapchain == VK_NULL_HANDLE || this->context.extent.width == 0 ||
            this->context.extent.height == 0) {
            auto result = this->make_swapchain();
            if (result.is_err() || this->context.extent.width == 0 ||
                this->context.extent.height == 0) {
                return result;
            }
        }
        constexpr auto WAIT_FOREVER = (std::numeric_limits<std::uint64_t>::max)();
        ENISHI_VK_TRY(
            vkWaitForFences(this->context.device, 1, &this->submitted, VK_TRUE, WAIT_FOREVER));
        auto acquired_result = vkAcquireNextImageKHR(this->context.device,
            this->swapchain,
            WAIT_FOREVER,
            this->acquired,
            VK_NULL_HANDLE,
            &this->image_index);
        if (acquired_result == VK_ERROR_OUT_OF_DATE_KHR) {
            return this->make_swapchain();
        }
        if (acquired_result != VK_SUBOPTIMAL_KHR) {
            ENISHI_VK_TRY(acquired_result);
        }
        ENISHI_VK_TRY(vkResetCommandBuffer(this->context.command, 0));
        VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        ENISHI_VK_TRY(vkBeginCommandBuffer(this->context.command, &begin));
        this->frame_clear_color = clear_color;
        this->context.recording = true;
        this->main_recording = false;
        this->context.shadow_recording = false;
        return {};
    }
    void VulkanSwapchain::end_active_pass() {
        if (this->main_recording || this->context.shadow_recording) {
            vkCmdEndRenderPass(this->context.command);
        }
        this->main_recording = false;
        this->context.shadow_recording = false;
    }
    void VulkanSwapchain::begin_main_pass() {
        if (!this->context.recording || this->main_recording) {
            return;
        }
        this->end_active_pass();
        const auto clear_color = this->frame_clear_color;
        std::array<VkClearValue, 2> clear{};
        clear[0].color = {{clear_color.r, clear_color.g, clear_color.b, clear_color.a}};
        clear[1].depthStencil = {1.0f, 0};
        VkRenderPassBeginInfo pass{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        pass.renderPass = this->context.render_pass;
        pass.framebuffer = this->framebuffers[this->image_index];
        pass.renderArea.extent = this->context.extent;
        pass.clearValueCount = static_cast<std::uint32_t>(clear.size());
        pass.pClearValues = clear.data();
        vkCmdBeginRenderPass(this->context.command, &pass, VK_SUBPASS_CONTENTS_INLINE);
        const VkViewport viewport{0,
            0,
            static_cast<float>(this->context.extent.width),
            static_cast<float>(this->context.extent.height),
            0,
            1};
        const VkRect2D scissor{{0, 0}, this->context.extent};
        vkCmdSetViewport(this->context.command, 0, 1, &viewport);
        vkCmdSetScissor(this->context.command, 0, 1, &scissor);
        this->main_recording = true;
    }
    platform::RenderResult<void> VulkanSwapchain::end_frame(
        const std::shared_ptr<VulkanBuffer>& readback) {
        if (!this->context.recording) {
            return {};
        }
        this->begin_main_pass();
        this->end_active_pass();
        this->context.recording = false;
        if (readback != nullptr) {
            auto result = this->record_readback(*readback);
            if (result.is_err()) {
                return result;
            }
        }
        ENISHI_VK_TRY(vkEndCommandBuffer(this->context.command));
        ENISHI_VK_TRY(vkResetFences(this->context.device, 1, &this->submitted));
        const VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        submit.waitSemaphoreCount = 1;
        submit.pWaitSemaphores = &this->acquired;
        submit.pWaitDstStageMask = &wait_stage;
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &this->context.command;
        submit.signalSemaphoreCount = 1;
        submit.pSignalSemaphores = &this->finished;
        ENISHI_VK_TRY(vkQueueSubmit(this->context.queue, 1, &submit, this->submitted));
        VkPresentInfoKHR present{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
        present.waitSemaphoreCount = 1;
        present.pWaitSemaphores = &this->finished;
        present.swapchainCount = 1;
        present.pSwapchains = &this->swapchain;
        present.pImageIndices = &this->image_index;
        const auto result = vkQueuePresentKHR(this->context.queue, &present);
        ENISHI_VK_TRY(vkQueueWaitIdle(this->context.queue));
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
            return this->make_swapchain();
        }
        return check_vk(result, "vkQueuePresentKHR");
    }
    platform::RenderResult<void> VulkanSwapchain::begin_shadow_pass(
        const VulkanShadowTarget& target, float clear_depth) {
        if (!this->context.recording) {
            return {};
        }
        if (this->main_recording || this->context.shadow_recording) {
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
        vkCmdBeginRenderPass(this->context.command, &pass, VK_SUBPASS_CONTENTS_INLINE);
        const VkViewport viewport{0,
            0,
            static_cast<float>(target.extent.width),
            static_cast<float>(target.extent.height),
            0,
            1};
        const VkRect2D scissor{{0, 0}, target.extent};
        vkCmdSetViewport(this->context.command, 0, 1, &viewport);
        vkCmdSetScissor(this->context.command, 0, 1, &scissor);
        this->context.shadow_recording = true;
        return {};
    }
    void VulkanSwapchain::destroy_swapchain() {
        for (auto framebuffer : this->framebuffers) {
            vkDestroyFramebuffer(this->context.device, framebuffer, nullptr);
        }
        this->framebuffers.clear();
        vkDestroyImageView(this->context.device, this->depth_view, nullptr);
        vkDestroyImage(this->context.device, this->depth_image, nullptr);
        vkFreeMemory(this->context.device, this->depth_memory, nullptr);
        this->depth_view = VK_NULL_HANDLE;
        this->depth_image = VK_NULL_HANDLE;
        this->depth_memory = VK_NULL_HANDLE;
        for (auto view : this->views) {
            vkDestroyImageView(this->context.device, view, nullptr);
        }
        this->views.clear();
        this->swapchain_images.clear();
        vkDestroySwapchainKHR(this->context.device, this->swapchain, nullptr);
        this->swapchain = VK_NULL_HANDLE;
    }
    platform::RenderResult<void> VulkanSwapchain::make_depth() {
        VkFormatProperties properties{};
        vkGetPhysicalDeviceFormatProperties(
            this->context.physical, this->depth_format, &properties);
        if ((properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) ==
            0) {
            return foundation::Error(platform::RenderError::MakeError,
                "Vulkan device lacks D32 depth attachment support");
        }
        VkImageCreateInfo image{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        image.imageType = VK_IMAGE_TYPE_2D;
        image.format = this->depth_format;
        image.extent = {this->context.extent.width, this->context.extent.height, 1};
        image.mipLevels = 1;
        image.arrayLayers = 1;
        image.samples = VK_SAMPLE_COUNT_1_BIT;
        image.tiling = VK_IMAGE_TILING_OPTIMAL;
        image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        ENISHI_VK_TRY(vkCreateImage(this->context.device, &image, nullptr, &this->depth_image));
        auto result = this->context.allocate_image(this->depth_image, this->depth_memory);
        if (result.is_err()) {
            return result;
        }
        VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        view.image = this->depth_image;
        view.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view.format = this->depth_format;
        view.subresourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1};
        return check_vk(vkCreateImageView(this->context.device, &view, nullptr, &this->depth_view),
            "create depth view");
    }
    platform::RenderResult<void> VulkanSwapchain::make_render_pass() {
        std::array<VkAttachmentDescription, 2> attachments{};
        attachments[0].format = this->color_format;
        attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        attachments[1].format = this->depth_format;
        attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        VkAttachmentReference color{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        VkAttachmentReference depth{1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &color;
        subpass.pDepthStencilAttachment = &depth;
        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                                  VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstStageMask = dependency.srcStageMask;
        dependency.dstAccessMask =
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        VkRenderPassCreateInfo info{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
        info.attachmentCount = static_cast<std::uint32_t>(attachments.size());
        info.pAttachments = attachments.data();
        info.subpassCount = 1;
        info.pSubpasses = &subpass;
        info.dependencyCount = 1;
        info.pDependencies = &dependency;
        return check_vk(
            vkCreateRenderPass(this->context.device, &info, nullptr, &this->context.render_pass),
            "vkCreateRenderPass");
    }
    platform::RenderResult<void> VulkanSwapchain::make_swapchain() {
        ENISHI_VK_TRY(vkDeviceWaitIdle(this->context.device));
        VkSurfaceCapabilitiesKHR caps{};
        ENISHI_VK_TRY(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
            this->context.physical, this->surface, &caps));
        std::uint32_t count = 0;
        ENISHI_VK_TRY(vkGetPhysicalDeviceSurfaceFormatsKHR(
            this->context.physical, this->surface, &count, nullptr));
        std::vector<VkSurfaceFormatKHR> formats(count);
        ENISHI_VK_TRY(vkGetPhysicalDeviceSurfaceFormatsKHR(
            this->context.physical, this->surface, &count, formats.data()));
        const auto format = std::ranges::find_if(formats, [this](const auto& candidate) {
            return (this->color_format == VK_FORMAT_UNDEFINED
                           ? candidate.format == VK_FORMAT_B8G8R8A8_UNORM
                           : candidate.format == this->color_format) &&
                   candidate.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        });
        if (format == formats.end()) {
            return foundation::Error(platform::RenderError::MakeError,
                "Vulkan surface requires BGRA8_UNORM/SRGB_NONLINEAR");
        }
        this->color_format = format->format;
        this->context.extent = caps.currentExtent;
        if (this->context.extent.width == (std::numeric_limits<std::uint32_t>::max)()) {
            int width = 0;
            int height = 0;
            if (!SDL_GetWindowSizeInPixels(this->window, &width, &height)) {
                return foundation::Error(platform::RenderError::MakeError, SDL_GetError());
            }
            this->context.extent = {std::clamp(static_cast<std::uint32_t>((std::max)(width, 0)),
                                        caps.minImageExtent.width,
                                        caps.maxImageExtent.width),
                std::clamp(static_cast<std::uint32_t>((std::max)(height, 0)),
                    caps.minImageExtent.height,
                    caps.maxImageExtent.height)};
        }
        if (this->context.extent.width == 0 || this->context.extent.height == 0) {
            return {};
        }
        this->destroy_swapchain();
        constexpr std::uint32_t MINIMUM_IMAGE_COUNT = 2;
        auto image_count = (std::max)(MINIMUM_IMAGE_COUNT, caps.minImageCount);
        if (caps.maxImageCount != 0 && image_count > caps.maxImageCount) {
            return foundation::Error(platform::RenderError::MakeError,
                "Vulkan UI requires at least two swapchain images");
        }
        VkSwapchainCreateInfoKHR info{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
        info.surface = this->surface;
        info.minImageCount = image_count;
        info.imageFormat = this->color_format;
        info.imageColorSpace = format->colorSpace;
        info.imageExtent = this->context.extent;
        info.imageArrayLayers = 1;
        info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        this->can_readback = (caps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) != 0;
        if (this->can_readback) {
            info.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        }
        info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        info.preTransform = caps.currentTransform;
        info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        for (const auto alpha : {VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
                 VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
                 VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR}) {
            if ((caps.supportedCompositeAlpha & alpha) != 0) {
                info.compositeAlpha = alpha;
                break;
            }
        }
        if ((caps.supportedCompositeAlpha & info.compositeAlpha) == 0) {
            return foundation::Error(platform::RenderError::MakeError,
                "Vulkan surface does not support the renderer's composite alpha modes");
        }
        info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
        info.clipped = VK_TRUE;
        ENISHI_VK_TRY(vkCreateSwapchainKHR(this->context.device, &info, nullptr, &this->swapchain));
        ENISHI_VK_TRY(vkGetSwapchainImagesKHR(
            this->context.device, this->swapchain, &this->context.image_count, nullptr));
        std::vector<VkImage> images(this->context.image_count);
        ENISHI_VK_TRY(vkGetSwapchainImagesKHR(
            this->context.device, this->swapchain, &this->context.image_count, images.data()));
        this->swapchain_images = images;
        return this->make_framebuffers();
    }
    platform::RenderResult<void> VulkanSwapchain::make_framebuffers() {
        auto result = this->make_depth();
        if (result.is_err()) {
            return result;
        }
        if (this->context.render_pass == VK_NULL_HANDLE) {
            result = this->make_render_pass();
            if (result.is_err()) {
                return result;
            }
        }
        for (auto image : this->swapchain_images) {
            VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
            view.image = image;
            view.viewType = VK_IMAGE_VIEW_TYPE_2D;
            view.format = this->color_format;
            view.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
            this->views.emplace_back(VK_NULL_HANDLE);
            ENISHI_VK_TRY(
                vkCreateImageView(this->context.device, &view, nullptr, &this->views.back()));
            const std::array attachments{this->views.back(), this->depth_view};
            VkFramebufferCreateInfo framebuffer{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
            framebuffer.renderPass = this->context.render_pass;
            framebuffer.attachmentCount = static_cast<std::uint32_t>(attachments.size());
            framebuffer.pAttachments = attachments.data();
            framebuffer.width = this->context.extent.width;
            framebuffer.height = this->context.extent.height;
            framebuffer.layers = 1;
            this->framebuffers.emplace_back(VK_NULL_HANDLE);
            ENISHI_VK_TRY(vkCreateFramebuffer(
                this->context.device, &framebuffer, nullptr, &this->framebuffers.back()));
        }
        return {};
    }
    platform::RenderResult<void> VulkanSwapchain::record_readback(const VulkanBuffer& buffer) {
        constexpr VkDeviceSize BGRA_PIXEL_SIZE = 4;
        // Unpadded screenshot rows: byte count = width * height * four BGRA channels.
        const auto bytes = static_cast<VkDeviceSize>(this->context.extent.width) *
                           this->context.extent.height * BGRA_PIXEL_SIZE;
        if (!this->can_readback || buffer.size < bytes) {
            return foundation::Error(platform::RenderError::MakeError,
                "Swapchain readback unsupported or buffer too small");
        }
        VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        barrier.image = this->swapchain_images[this->image_index];
        barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        vkCmdPipelineBarrier(this->context.command,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &barrier);
        VkBufferImageCopy copy{};
        copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        copy.imageExtent = {this->context.extent.width, this->context.extent.height, 1};
        vkCmdCopyImageToBuffer(this->context.command,
            barrier.image,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            buffer.buffer,
            1,
            &copy);
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = 0;
        VkBufferMemoryBarrier host{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
        host.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        host.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
        host.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        host.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        host.buffer = buffer.buffer;
        host.size = bytes;
        vkCmdPipelineBarrier(this->context.command,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_HOST_BIT,
            0,
            0,
            nullptr,
            1,
            &host,
            1,
            &barrier);
        return {};
    }
} // namespace enishi::renderer::vulkan
