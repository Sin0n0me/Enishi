#include "vulkan_diagnostics.h"
#include <debug/log/logger.h>

namespace enishi::debug {
    VkDebugUtilsMessengerCreateInfoEXT VulkanDiagnostics::make_create_info() noexcept {
        VkDebugUtilsMessengerCreateInfoEXT info{
            VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
        info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        info.pfnUserCallback = &VulkanDiagnostics::callback;
        info.pUserData = this;
        return info;
    }

    std::uint32_t VulkanDiagnostics::error_count() const noexcept {
        return this->validation_errors;
    }

    VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDiagnostics::callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT severity,
        VkDebugUtilsMessageTypeFlagsEXT,
        const VkDebugUtilsMessengerCallbackDataEXT* data,
        void* user) {
        if ((severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) != 0) {
            ++static_cast<VulkanDiagnostics*>(user)->validation_errors;
            Logger::error(data->pMessage);
        } else {
            Logger::warning(data->pMessage);
        }
        return VK_FALSE;
    }
} // namespace enishi::debug
