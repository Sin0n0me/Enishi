#pragma once
#include <renderer/common/render_handle_mapper.h>
namespace enishi::renderer::vulkan {
    class VulkanHandles final {
      public:
        static types::RenderHandle make(
            RenderHandleMapper& mapper, types::RenderHandleType type) noexcept;
    };
} // namespace enishi::renderer::vulkan
