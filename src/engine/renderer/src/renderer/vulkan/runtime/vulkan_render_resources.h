#pragma once
#include "interface_vulkan_render_resources.h"
namespace enishi::renderer::vulkan {
    class VulkanRenderResources final : public IVulkanRenderResources {
      private:
        std::shared_ptr<VulkanDevice> context;
        RenderHandleMapper handle_mapper;
        ResourceAccessor resource_accessor;
        VulkanRendererState state;

      public:
        explicit VulkanRenderResources(std::shared_ptr<VulkanDevice> context);
        VulkanRenderResources(const VulkanRenderResources&) = delete;
        ~VulkanRenderResources() noexcept override;
        VulkanRenderResources& operator=(const VulkanRenderResources&) = delete;
        std::shared_ptr<VulkanDevice> get_context() noexcept override;
        RenderHandleMapper& get_handle_mapper() noexcept override;
        ResourceAccessor& get_resource_accessor() noexcept override;
        VulkanRendererState& get_state() noexcept override;
    };
} // namespace enishi::renderer::vulkan
