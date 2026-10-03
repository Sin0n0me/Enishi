#pragma once
#include "d3d11.h"
#include "d3d11_renderer.h"
#include <platform/renderer/interface_render_initializer.h>

namespace enishi::renderer::directx {
    class D3D11RenderInitializer : public platform::IRendererInitializer<D3D11Renderer> {
      private:
        bool use_dcomp;

      public:
        explicit D3D11RenderInitializer(const bool use_dcomp) noexcept;

        foundation::Result<std::shared_ptr<D3D11Renderer>, platform::RenderError> init(
            const platform::WindowHandle& window_handle,
            const types::WindowSize& window_size) override;
    };
} // namespace enishi::renderer::directx
