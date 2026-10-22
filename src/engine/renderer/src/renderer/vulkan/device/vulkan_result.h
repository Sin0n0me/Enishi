#pragma once
#include <platform/renderer/interface_renderer.h>
#include <vulkan/vulkan.h>
namespace enishi::renderer::vulkan {
    platform::RenderResult<void> check_vk(VkResult result, const char* operation);
} // namespace enishi::renderer::vulkan
