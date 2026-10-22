#include "vulkan_result.h"
namespace enishi::renderer::vulkan {
    platform::RenderResult<void> check_vk(VkResult result, const char* operation) {
        if (result != VK_SUCCESS) {
            return foundation::Error(platform::RenderError::MakeError,
                std::format("{} failed (VkResult {})", operation, static_cast<int>(result)));
        }
        return {};
    }
} // namespace enishi::renderer::vulkan
