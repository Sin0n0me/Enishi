#include "vulkan_render_resources.h"
#include <debug/log/logger.h>
#include <utility>
namespace enishi::renderer::vulkan {
    VulkanRenderResources::VulkanRenderResources(std::shared_ptr<VulkanDevice> context)
        : context(std::move(context))
        , state(this->context) {
    }
    VulkanRenderResources::~VulkanRenderResources() noexcept {
        auto result = this->context->wait_idle();
        if (result.is_err()) {
            debug::Logger::error(result.unwrap_err().get_message());
        }
    }
    std::shared_ptr<VulkanDevice> VulkanRenderResources::get_context() noexcept {
        return this->context;
    }
    RenderHandleMapper& VulkanRenderResources::get_handle_mapper() noexcept {
        return this->handle_mapper;
    }
    ResourceAccessor& VulkanRenderResources::get_resource_accessor() noexcept {
        return this->resource_accessor;
    }
    VulkanRendererState& VulkanRenderResources::get_state() noexcept {
        return this->state;
    }
} // namespace enishi::renderer::vulkan
