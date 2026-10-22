#include <algorithm>
#include <renderer/vulkan/runtime/vulkan_handles.h>
#include <renderer/vulkan/shadow/vulkan_shadow_resources.h>
#include <renderer/vulkan/view/vulkan_image_view.h>
#include <renderer/vulkan/view/vulkan_views.h>

namespace enishi::renderer::vulkan {
    platform::RenderResult<types::RenderHandle> VulkanViews::create_image(
        VulkanRendererState& state,
        RenderHandleMapper& handle_mapper,
        const types::ImageDescription& description) {
        if ((!description.contains(types::ImageUsage::BackBuffer) &&
                !description.contains(types::ImageUsage::DepthStencil)) ||
            description.samples != 1 || description.mip_levels != 1 ||
            description.array_layers != 1) {
            return foundation::Error(platform::RenderError::MakeError,
                "Vulkan render passes support swapchain color and depth images");
        }
        if (description.contains(types::ImageUsage::DepthStencil) &&
            description.format != types::ImageFormat::D32_FLOAT) {
            return foundation::Error(
                platform::RenderError::MakeError, "Vulkan swapchain depth format is D32_FLOAT");
        }
        const auto handle = VulkanHandles::make(handle_mapper, types::RenderHandleType::Image);
        if (description.contains(types::ImageUsage::DepthStencil) &&
            description.contains(types::ImageUsage::ShaderResource)) {
            if (description.size.x < 1 || description.size.y < 1) {
                return foundation::Error(platform::RenderError::MakeError,
                    "Sampled render images require positive depth dimensions");
            }
            auto target =
                state.context->make_shadow_target({static_cast<std::uint32_t>(description.size.x),
                    static_cast<std::uint32_t>(description.size.y)});
            if (target.is_err()) {
                return std::move(target).take_err();
            }
            state.shadow_targets.emplace(handle, target.unwrap());
        }
        state.images.emplace(handle, description);
        return handle;
    }
    platform::RenderResult<std::shared_ptr<platform::IRenderTargetView>>
    VulkanViews::create_render_target_view(VulkanRendererState& state,
        ResourceAccessor& resource_accessor,
        RenderHandleMapper& handle_mapper,
        types::RenderHandle image,
        const types::ImageViewDescription& description) {
        const auto found = state.images.find(image);
        if (found == state.images.end() || !found->second.contains(types::ImageUsage::BackBuffer)) {
            return foundation::Error(platform::RenderError::MakeError,
                "Vulkan render target must reference the swapchain");
        }
        const auto handle = VulkanHandles::make(handle_mapper, types::RenderHandleType::View);
        auto view = std::make_shared<VulkanRenderTargetView>(handle, description);
        resource_accessor.make_render_target_view(handle.id, view);
        return view;
    }
    platform::RenderResult<std::shared_ptr<platform::IDepthStencilView>>
    VulkanViews::create_depth_stencil_view(VulkanRendererState& state,
        ResourceAccessor& resource_accessor,
        RenderHandleMapper& handle_mapper,
        types::RenderHandle image,
        const types::ImageViewDescription& description) {
        const auto found = state.images.find(image);
        if (found == state.images.end() ||
            !found->second.contains(types::ImageUsage::DepthStencil)) {
            return foundation::Error(
                platform::RenderError::MakeError, "Vulkan depth view requires a depth image");
        }
        const auto handle = VulkanHandles::make(handle_mapper, types::RenderHandleType::View);
        if (state.shadow_targets.contains(image)) {
            if (description.format != types::ImageFormat::D32_FLOAT ||
                description.base_mip_level != 0 || description.base_array_layer != 0 ||
                description.mip_Level_count != 1 || description.array_layer_count != 1) {
                return foundation::Error(
                    platform::RenderError::MakeError, "Invalid sampled depth view");
            }
            state.shadow_targets.emplace(handle, state.shadow_targets.at(image));
        }
        auto view = std::make_shared<VulkanDepthStencilView>(handle, description);
        resource_accessor.make_depth_stencil_view(handle.id, view);
        return view;
    }
    platform::RenderResult<std::shared_ptr<platform::IShaderResourceView>>
    VulkanViews::create_shader_resource_view(VulkanRendererState& state,
        ResourceAccessor& resource_accessor,
        RenderHandleMapper& handle_mapper,
        types::RenderHandle image,
        const types::ImageViewDescription& description) {
        constexpr std::uint32_t SHADOW_TEXTURE_SLOT = 3;
        const auto found = state.shadow_targets.find(image);
        if (found == state.shadow_targets.end() ||
            description.shader_resource_slot != SHADOW_TEXTURE_SLOT ||
            description.format != types::ImageFormat::D32_FLOAT ||
            description.aspect != types::ImageAspect::Depth || description.base_mip_level != 0 ||
            description.base_array_layer != 0 || description.mip_Level_count != 1 ||
            description.array_layer_count != 1) {
            return foundation::Error(
                platform::RenderError::MakeError, "Invalid Vulkan sampled depth view");
        }
        if (found->second->descriptors == VK_NULL_HANDLE) {
            auto result = VulkanShadowResources::make_shadow_descriptors(
                state, *found->second, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
            if (result.is_err()) {
                return std::move(result).take_err();
            }
        }
        const auto handle = VulkanHandles::make(handle_mapper, types::RenderHandleType::View);
        state.shadow_targets.emplace(handle, found->second);
        auto view = std::make_shared<VulkanShaderResourceView>(handle, description);
        resource_accessor.make_shader_resource_view(handle.id, view);
        return view;
    }
    platform::RenderResult<std::shared_ptr<platform::IUnorderedAccessView>>
    VulkanViews::create_unordered_access_view(
        types::RenderHandle, const types::ImageViewDescription&) {
        return foundation::Error(
            platform::RenderError::MakeError, "Vulkan storage image views are not supported");
    }
} // namespace enishi::renderer::vulkan
