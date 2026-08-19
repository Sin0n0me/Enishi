#pragma once
#include "../errors/renderer_errors.h"
#include <engine_types/handle/renderer/render_handle.h>
#include <engine_types/renderer/description/image_view_description.h>
#include <engine_types/renderer/description/sampler_description.h>
#include <foundation/result/result.h>
#include <glm/glm.hpp>

namespace enishi::platform {
    class IImageView {
      public:
        virtual ~IImageView(void) noexcept = default;

        [[nodiscard]]
        virtual types::ImageViewDescription get_description(void) const noexcept = 0;

        [[nodiscard]]
        virtual types::RenderHandle get_handle(void) const noexcept = 0;
    };

    // RTV
    class IRenderTargetView : public IImageView {
      public:
        [[nodiscard]]
        virtual glm::vec4 get_clear_color(void) const noexcept = 0;

        virtual void set_clear_color(glm::vec4 color) noexcept = 0;
    };

    // DSV
    class IDepthStencilView : public IImageView {
      public:
        [[nodiscard]]
        virtual float clear_depth(void) const noexcept = 0;

        [[nodiscard]]
        virtual std::uint8_t clear_stencil(void) const noexcept = 0;

        virtual void set_clear_depth(const float depth) noexcept = 0;

        virtual void set_clear_stencil(const std::uint8_t stencil) noexcept = 0;
    };

    // SRV
    class IShaderResourceView : public IImageView {
      public:
        [[nodiscard]]
        virtual types::SamplerDescription sampler_description(void) const noexcept = 0;
    };

    // UAV
    class IUnorderedAccessView : public IImageView {
      public:
        [[nodiscard]]
        virtual std::uint32_t mip_level(void) const noexcept = 0;
    };
} // namespace enishi::platform