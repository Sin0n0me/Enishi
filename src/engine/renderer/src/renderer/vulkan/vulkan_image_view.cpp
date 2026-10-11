#include "vulkan_image_view.h"
namespace enishi::renderer::vulkan {

    VulkanRenderTargetView::VulkanRenderTargetView(
        types::RenderHandle handle, types::ImageViewDescription description)
        : handle(handle)
        , description(description)
        , color{} {
    }
    types::ImageViewDescription VulkanRenderTargetView::get_description(void) const noexcept {
        return this->description;
    }
    types::RenderHandle VulkanRenderTargetView::get_handle(void) const noexcept {
        return this->handle;
    }
    glm::vec4 VulkanRenderTargetView::get_clear_color(void) const noexcept {
        return this->color;
    }
    void VulkanRenderTargetView::set_clear_color(glm::vec4 color) noexcept {
        this->color = color;
    }
    VulkanDepthStencilView::VulkanDepthStencilView(
        types::RenderHandle handle, types::ImageViewDescription description)
        : handle(handle)
        , description(description)
        , depth(image_view_constants::DEFAULT_CLEAR_DEPTH)
        , stencil(image_view_constants::DEFAULT_CLEAR_STENCIL) {
    }
    types::ImageViewDescription VulkanDepthStencilView::get_description(void) const noexcept {
        return this->description;
    }
    types::RenderHandle VulkanDepthStencilView::get_handle(void) const noexcept {
        return this->handle;
    }
    float VulkanDepthStencilView::clear_depth(void) const noexcept {
        return this->depth;
    }
    std::uint8_t VulkanDepthStencilView::clear_stencil(void) const noexcept {
        return this->stencil;
    }
    void VulkanDepthStencilView::set_clear_depth(float value) noexcept {
        this->depth = value;
    }
    void VulkanDepthStencilView::set_clear_stencil(std::uint8_t value) noexcept {
        this->stencil = value;
    }
    VulkanShaderResourceView::VulkanShaderResourceView(
        types::RenderHandle handle, types::ImageViewDescription description)
        : handle(handle)
        , description(description) {
    }
    types::ImageViewDescription VulkanShaderResourceView::get_description(void) const noexcept {
        return this->description;
    }
    types::RenderHandle VulkanShaderResourceView::get_handle(void) const noexcept {
        return this->handle;
    }
    types::SamplerStateDescription VulkanShaderResourceView::sampler_description(
        void) const noexcept {
        return types::SamplerStateDescription::default_linear();
    }
    VulkanUnorderedAccessView::VulkanUnorderedAccessView(
        types::RenderHandle handle, types::ImageViewDescription description)
        : handle(handle)
        , description(description) {
    }
    types::ImageViewDescription VulkanUnorderedAccessView::get_description(void) const noexcept {
        return this->description;
    }
    types::RenderHandle VulkanUnorderedAccessView::get_handle(void) const noexcept {
        return this->handle;
    }
    std::uint32_t VulkanUnorderedAccessView::mip_level(void) const noexcept {
        return this->description.base_mip_level;
    }
} // namespace enishi::renderer::vulkan
