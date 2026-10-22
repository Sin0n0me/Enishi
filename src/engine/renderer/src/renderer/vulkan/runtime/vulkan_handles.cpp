#include "vulkan_handles.h"
namespace enishi::renderer::vulkan {
    types::RenderHandle VulkanHandles::make(
        RenderHandleMapper& mapper, types::RenderHandleType type) noexcept {
        auto handle = mapper.make(type, {});
        mapper[handle].resource = handle.id;
        return handle;
    }
} // namespace enishi::renderer::vulkan
