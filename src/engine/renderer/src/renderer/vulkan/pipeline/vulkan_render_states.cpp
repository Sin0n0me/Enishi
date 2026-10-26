#include "vulkan_render_states.h"
#include <algorithm>
#include <renderer/vulkan/runtime/vulkan_handles.h>
namespace enishi::renderer::vulkan {
    platform::RenderResult<types::RenderHandle> VulkanRenderStates::create_rasterizer(
        VulkanRendererState& state,
        ResourceAccessor& resource_accessor,
        RenderHandleMapper& handle_mapper,
        const types::RasterizerStateDescription& value) {
        if (value.fill_mode != types::FillMode::Solid || value.depth_clamp ||
            value.depth_bias.clamp != 0.0f || value.line_width > 1.0f) {
            return foundation::Error(platform::RenderError::MakeError,
                "Vulkan model rasterizer requires solid fill without depth clamp");
        }
        const auto handle = VulkanHandles::make(handle_mapper, types::RenderHandleType::State);
        state.rasterizers.emplace(handle, value);
        resource_accessor.add_state(handle.id, types::StateKind::Rasterizer);
        return handle;
    }
    platform::RenderResult<types::RenderHandle> VulkanRenderStates::create_depth_stencil(
        VulkanRendererState& state,
        ResourceAccessor& resource_accessor,
        RenderHandleMapper& handle_mapper,
        const types::DepthStencilStateDescription& value) {
        if (value.stencil.enabled) {
            return foundation::Error(platform::RenderError::MakeError,
                "Vulkan D32 model pass does not have a stencil attachment");
        }
        const auto handle = VulkanHandles::make(handle_mapper, types::RenderHandleType::State);
        state.depths.emplace(handle, value);
        resource_accessor.add_state(handle.id, types::StateKind::DepthStencil);
        return handle;
    }
    platform::RenderResult<types::RenderHandle> VulkanRenderStates::create_blend(
        VulkanRendererState& state,
        ResourceAccessor& resource_accessor,
        RenderHandleMapper& handle_mapper,
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
        const auto handle = VulkanHandles::make(handle_mapper, types::RenderHandleType::State);
        state.blends.emplace(handle, value);
        resource_accessor.add_state(handle.id, types::StateKind::Blend);
        return handle;
    }
    platform::RenderResult<types::RenderHandle> VulkanRenderStates::create_sampler(
        VulkanRendererState& state,
        ResourceAccessor& resource_accessor,
        RenderHandleMapper& handle_mapper,
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
        vkGetPhysicalDeviceFeatures(state.context->physical, &features);
        vkGetPhysicalDeviceProperties(state.context->physical, &properties);
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
            vkCreateSampler(state.context->device, &info, nullptr, &sampler), "vkCreateSampler");
        if (result.is_err()) {
            return std::move(result).take_err();
        }
        const auto handle = VulkanHandles::make(handle_mapper, types::RenderHandleType::State);
        state.samplers.emplace(handle, sampler);
        resource_accessor.add_state(handle.id, types::StateKind::Sampler);
        return handle;
    }
} // namespace enishi::renderer::vulkan
