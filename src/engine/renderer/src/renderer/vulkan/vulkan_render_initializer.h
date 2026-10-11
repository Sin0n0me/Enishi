#pragma once
#include "vulkan_renderer.h"
#include <platform/renderer/interface_render_initializer.h>

namespace enishi::renderer::vulkan {
    class VullkanRenderInitializer : public platform::IRendererInitializer<VulkanRenderer> {
      public:
        foundation::Result<std::shared_ptr<VulkanRenderer>, platform::RenderError> init(
            const platform::WindowHandle& window_handle,
            const types::WindowSize& window_size) override;
    };
} // namespace enishi::renderer::vulkan
