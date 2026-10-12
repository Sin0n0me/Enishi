#pragma once
#include "vulkan_renderer.h"
namespace enishi::renderer::vulkan {
    class VulkanRenderInitializer final {
      public:
        platform::RenderResult<std::shared_ptr<VulkanRenderer>> init(
            SDL_Window* window, bool validation = false);
    };
} // namespace enishi::renderer::vulkan
