#pragma once

#include <platform/renderer/view/interface_image_view.h>

namespace enishi::renderer::opengl {
    class OpenGLRenderTargetView final : public platform::IRenderTargetView {
      private:
        types::RenderHandle handle;
        types::ImageViewDescription description;
        glm::vec4 color{};

      public:
        OpenGLRenderTargetView(types::RenderHandle handle, types::ImageViewDescription description)
            : handle(handle)
            , description(description) {
        }
        types::ImageViewDescription get_description(void) const noexcept override {
            return this->description;
        }
        types::RenderHandle get_handle(void) const noexcept override {
            return this->handle;
        }
        glm::vec4 get_clear_color(void) const noexcept override {
            return this->color;
        }
        void set_clear_color(glm::vec4 color) noexcept override {
            this->color = color;
        }
    };
    class OpenGLDepthStencilView final : public platform::IDepthStencilView {
      private:
        types::RenderHandle handle;
        types::ImageViewDescription description;
        float depth = 1.0f;
        std::uint8_t stencil = 0;

      public:
        OpenGLDepthStencilView(types::RenderHandle handle, types::ImageViewDescription description)
            : handle(handle)
            , description(description) {
        }
        types::ImageViewDescription get_description(void) const noexcept override {
            return this->description;
        }
        types::RenderHandle get_handle(void) const noexcept override {
            return this->handle;
        }
        float clear_depth(void) const noexcept override {
            return this->depth;
        }
        std::uint8_t clear_stencil(void) const noexcept override {
            return this->stencil;
        }
        void set_clear_depth(float value) noexcept override {
            this->depth = value;
        }
        void set_clear_stencil(std::uint8_t value) noexcept override {
            this->stencil = value;
        }
    };
    class OpenGLShaderResourceView final : public platform::IShaderResourceView {
      private:
        types::RenderHandle handle;
        types::ImageViewDescription description;

      public:
        OpenGLShaderResourceView(
            types::RenderHandle handle, types::ImageViewDescription description)
            : handle(handle)
            , description(description) {
        }
        types::ImageViewDescription get_description(void) const noexcept override {
            return this->description;
        }
        types::RenderHandle get_handle(void) const noexcept override {
            return this->handle;
        }
        types::SamplerStateDescription sampler_description(void) const noexcept override {
            return types::SamplerStateDescription::default_linear();
        }
    };
    class OpenGLUnorderedAccessView final : public platform::IUnorderedAccessView {
      private:
        types::RenderHandle handle;
        types::ImageViewDescription description;

      public:
        OpenGLUnorderedAccessView(
            types::RenderHandle handle, types::ImageViewDescription description)
            : handle(handle)
            , description(description) {
        }
        types::ImageViewDescription get_description(void) const noexcept override {
            return this->description;
        }
        types::RenderHandle get_handle(void) const noexcept override {
            return this->handle;
        }
        std::uint32_t mip_level(void) const noexcept override {
            return this->description.base_mip_level;
        }
    };
} // namespace enishi::renderer::opengl
