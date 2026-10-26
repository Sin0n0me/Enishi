#pragma once
#include <cstdint>
#include <vulkan/vulkan.h>

namespace enishi::debug {
    class VulkanDiagnostics {
      private:
        std::uint32_t validation_errors = 0;

      public:
        VkDebugUtilsMessengerCreateInfoEXT make_create_info() noexcept;
        std::uint32_t error_count() const noexcept;

      private:
        static VKAPI_ATTR VkBool32 VKAPI_CALL callback(
            VkDebugUtilsMessageSeverityFlagBitsEXT severity,
            VkDebugUtilsMessageTypeFlagsEXT type,
            const VkDebugUtilsMessengerCallbackDataEXT* data,
            void* user);
    };
} // namespace enishi::debug
