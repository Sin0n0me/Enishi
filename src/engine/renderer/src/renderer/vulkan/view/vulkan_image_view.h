#pragma once

#include <cstdint>
#include <platform/renderer/view/interface_image_view.h>

namespace enishi::renderer::vulkan {
    namespace image_view_constants {
        inline constexpr float DEFAULT_CLEAR_DEPTH = 1.0f;
        inline constexpr std::uint8_t DEFAULT_CLEAR_STENCIL = 0;
    } // namespace image_view_constants

    class VulkanRenderTargetView final : public platform::IRenderTargetView {
      private:
        types::RenderHandle handle;
        types::ImageViewDescription description;
        glm::vec4 color;

      public:
        VulkanRenderTargetView(types::RenderHandle handle, types::ImageViewDescription description);
        types::ImageViewDescription get_description(void) const noexcept override;
        types::RenderHandle get_handle(void) const noexcept override;
        glm::vec4 get_clear_color(void) const noexcept override;
        void set_clear_color(glm::vec4 color) noexcept override;
    };
    class VulkanDepthStencilView final : public platform::IDepthStencilView {
      private:
        types::RenderHandle handle;
        types::ImageViewDescription description;
        float depth;
        std::uint8_t stencil;

      public:
        VulkanDepthStencilView(types::RenderHandle handle, types::ImageViewDescription description);
        types::ImageViewDescription get_description(void) const noexcept override;
        types::RenderHandle get_handle(void) const noexcept override;
        float clear_depth(void) const noexcept override;
        std::uint8_t clear_stencil(void) const noexcept override;
        void set_clear_depth(float value) noexcept override;
        void set_clear_stencil(std::uint8_t value) noexcept override;
    };
    class VulkanShaderResourceView final : public platform::IShaderResourceView {
      private:
        types::RenderHandle handle;
        types::ImageViewDescription description;

      public:
        VulkanShaderResourceView(
            types::RenderHandle handle, types::ImageViewDescription description);
        types::ImageViewDescription get_description(void) const noexcept override;
        types::RenderHandle get_handle(void) const noexcept override;
        types::SamplerStateDescription sampler_description(void) const noexcept override;
    };
    class VulkanUnorderedAccessView final : public platform::IUnorderedAccessView {
      private:
        types::RenderHandle handle;
        types::ImageViewDescription description;

      public:
        VulkanUnorderedAccessView(
            types::RenderHandle handle, types::ImageViewDescription description);
        types::ImageViewDescription get_description(void) const noexcept override;
        types::RenderHandle get_handle(void) const noexcept override;
        std::uint32_t mip_level(void) const noexcept override;
    };
} // namespace enishi::renderer::vulkan
