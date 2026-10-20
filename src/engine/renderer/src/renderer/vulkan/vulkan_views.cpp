#include "vulkan_image_view.h"
#include "vulkan_renderer.h"
#include <algorithm>

namespace enishi::renderer::vulkan {
    platform::RenderResult<types::RenderHandle> VulkanRenderer::create_image(
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
        const auto handle = this->make_handle(types::RenderHandleType::Image);
        if (description.contains(types::ImageUsage::DepthStencil) &&
            description.contains(types::ImageUsage::ShaderResource)) {
            if (description.size.x < 1 || description.size.y < 1) {
                return foundation::Error(platform::RenderError::MakeError,
                    "Sampled render images require positive depth dimensions");
            }
            auto target =
                this->context->make_shadow_target({static_cast<std::uint32_t>(description.size.x),
                    static_cast<std::uint32_t>(description.size.y)});
            if (target.is_err()) {
                return std::move(target).take_err();
            }
            this->state->shadow_targets.emplace(handle, target.unwrap());
        }
        this->state->images.emplace(handle, description);
        return handle;
    }
    platform::RenderResult<std::shared_ptr<platform::IRenderTargetView>>
    VulkanRenderer::create_render_target_view(
        types::RenderHandle image, const types::ImageViewDescription& description) {
        const auto found = this->state->images.find(image);
        if (found == this->state->images.end() ||
            !found->second.contains(types::ImageUsage::BackBuffer)) {
            return foundation::Error(platform::RenderError::MakeError,
                "Vulkan render target must reference the swapchain");
        }
        const auto handle = this->make_handle(types::RenderHandleType::View);
        auto view = std::make_shared<VulkanRenderTargetView>(handle, description);
        this->resource_accessor->make_render_target_view(handle.id, view);
        return view;
    }
    platform::RenderResult<std::shared_ptr<platform::IDepthStencilView>>
    VulkanRenderer::create_depth_stencil_view(
        types::RenderHandle image, const types::ImageViewDescription& description) {
        const auto found = this->state->images.find(image);
        if (found == this->state->images.end() ||
            !found->second.contains(types::ImageUsage::DepthStencil)) {
            return foundation::Error(
                platform::RenderError::MakeError, "Vulkan depth view requires a depth image");
        }
        const auto handle = this->make_handle(types::RenderHandleType::View);
        if (this->state->shadow_targets.contains(image)) {
            if (description.format != types::ImageFormat::D32_FLOAT ||
                description.base_mip_level != 0 || description.base_array_layer != 0 ||
                description.mip_Level_count != 1 || description.array_layer_count != 1) {
                return foundation::Error(
                    platform::RenderError::MakeError, "Invalid sampled depth view");
            }
            this->state->shadow_targets.emplace(handle, this->state->shadow_targets.at(image));
        }
        auto view = std::make_shared<VulkanDepthStencilView>(handle, description);
        this->resource_accessor->make_depth_stencil_view(handle.id, view);
        return view;
    }
    platform::RenderResult<std::shared_ptr<platform::IShaderResourceView>>
    VulkanRenderer::create_shader_resource_view(
        types::RenderHandle image, const types::ImageViewDescription& description) {
        constexpr std::uint32_t SHADOW_TEXTURE_SLOT = 3;
        const auto found = this->state->shadow_targets.find(image);
        if (found == this->state->shadow_targets.end() ||
            description.shader_resource_slot != SHADOW_TEXTURE_SLOT ||
            description.format != types::ImageFormat::D32_FLOAT ||
            description.aspect != types::ImageAspect::Depth || description.base_mip_level != 0 ||
            description.base_array_layer != 0 || description.mip_Level_count != 1 ||
            description.array_layer_count != 1) {
            return foundation::Error(
                platform::RenderError::MakeError, "Invalid Vulkan sampled depth view");
        }
        if (found->second->descriptors == VK_NULL_HANDLE) {
            auto result = this->make_shadow_descriptors(
                *found->second, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
            if (result.is_err()) {
                return std::move(result).take_err();
            }
        }
        const auto handle = this->make_handle(types::RenderHandleType::View);
        this->state->shadow_targets.emplace(handle, found->second);
        auto view = std::make_shared<VulkanShaderResourceView>(handle, description);
        this->resource_accessor->make_shader_resource_view(handle.id, view);
        return view;
    }
    platform::RenderResult<std::shared_ptr<platform::IUnorderedAccessView>>
    VulkanRenderer::create_unordered_access_view(
        types::RenderHandle, const types::ImageViewDescription&) {
        return foundation::Error(
            platform::RenderError::MakeError, "Vulkan storage image views are not supported");
    }
    platform::RenderResult<types::RenderHandle> VulkanRenderer::create_rasterizer(
        const types::RasterizerStateDescription& value) {
        if (value.fill_mode != types::FillMode::Solid || value.depth_clamp ||
            value.depth_bias.clamp != 0.0f || value.line_width > 1.0f) {
            return foundation::Error(platform::RenderError::MakeError,
                "Vulkan model rasterizer requires solid fill without depth clamp");
        }
        const auto handle = this->make_handle(types::RenderHandleType::State);
        this->state->rasterizers.emplace(handle, value);
        this->resource_accessor->add_state(handle.id, types::StateKind::Rasterizer);
        return handle;
    }
    platform::RenderResult<types::RenderHandle> VulkanRenderer::create_depth_stencil(
        const types::DepthStencilStateDescription& value) {
        if (value.stencil.enabled) {
            return foundation::Error(platform::RenderError::MakeError,
                "Vulkan D32 model pass does not have a stencil attachment");
        }
        const auto handle = this->make_handle(types::RenderHandleType::State);
        this->state->depths.emplace(handle, value);
        this->resource_accessor->add_state(handle.id, types::StateKind::DepthStencil);
        return handle;
    }
    platform::RenderResult<types::RenderHandle> VulkanRenderer::create_blend(
        const types::BlendStateDescription& value) {
        if (value.alpha_to_coverage || value.independent_blend) {
            return foundation::Error(platform::RenderError::MakeError,
                "Vulkan model pass uses one single-sample color attachment");
        }
        const auto& target = value.render_targets[0];
        if (target.src_color > types::BlendFactor::SrcAlphaSaturate ||
            target.dst_color > types::BlendFactor::SrcAlphaSaturate ||
            target.src_alpha > types::BlendFactor::SrcAlphaSaturate ||
            target.dst_alpha > types::BlendFactor::SrcAlphaSaturate) {
            return foundation::Error(platform::RenderError::MakeError,
                "Vulkan model shaders do not support dual-source blending");
        }
        const auto handle = this->make_handle(types::RenderHandleType::State);
        this->state->blends.emplace(handle, value);
        this->resource_accessor->add_state(handle.id, types::StateKind::Blend);
        return handle;
    }
    platform::RenderResult<types::RenderHandle> VulkanRenderer::create_sampler(
        const types::SamplerStateDescription& value) {
        const auto address = [](types::AddressMode mode) {
            switch (mode) {
                case types::AddressMode::Repeat:
                    return VK_SAMPLER_ADDRESS_MODE_REPEAT;
                case types::AddressMode::Clamp:
                    return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                case types::AddressMode::Mirror:
                    return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
            }
            return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        };
        VkPhysicalDeviceFeatures features{};
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceFeatures(this->context->physical, &features);
        vkGetPhysicalDeviceProperties(this->context->physical, &properties);
        VkSamplerCreateInfo info{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        info.magFilter =
            value.mag_filter == types::FilterMode::Linear ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
        info.minFilter =
            value.min_filter == types::FilterMode::Linear ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
        info.mipmapMode = value.mip_filter == types::FilterMode::Linear
                              ? VK_SAMPLER_MIPMAP_MODE_LINEAR
                              : VK_SAMPLER_MIPMAP_MODE_NEAREST;
        info.addressModeU = address(value.address_u);
        info.addressModeV = address(value.address_v);
        info.addressModeW = address(value.address_w);
        info.minLod = value.min_lod;
        info.maxLod = value.max_lod;
        info.mipLodBias = std::clamp(value.mip_lod_bias,
            -properties.limits.maxSamplerLodBias,
            properties.limits.maxSamplerLodBias);
        info.anisotropyEnable = features.samplerAnisotropy == VK_TRUE &&
                                value.anisotropy != types::AnisotropyLevel::None;
        info.maxAnisotropy = (std::min)(
            static_cast<float>(value.anisotropy), properties.limits.maxSamplerAnisotropy);
        VkSampler sampler = VK_NULL_HANDLE;
        auto result = check_vk(
            vkCreateSampler(this->context->device, &info, nullptr, &sampler), "vkCreateSampler");
        if (result.is_err()) {
            return std::move(result).take_err();
        }
        const auto handle = this->make_handle(types::RenderHandleType::State);
        this->state->samplers.emplace(handle, sampler);
        this->resource_accessor->add_state(handle.id, types::StateKind::Sampler);
        return handle;
    }
} // namespace enishi::renderer::vulkan
