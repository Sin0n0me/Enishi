#pragma once
#include <memory>
#include <renderer/common/render_handle_mapper.h>
#include <renderer/common/resource_accessor.h>
#include <renderer/vulkan/runtime/vulkan_renderer_state.h>
namespace enishi::renderer::vulkan {
    class IVulkanRenderResources {
      public:
        virtual ~IVulkanRenderResources() noexcept = default;
        virtual std::shared_ptr<VulkanDevice> get_context() noexcept = 0;
        virtual RenderHandleMapper& get_handle_mapper() noexcept = 0;
        virtual ResourceAccessor& get_resource_accessor() noexcept = 0;
        virtual VulkanRendererState& get_state() noexcept = 0;
    };
} // namespace enishi::renderer::vulkan
