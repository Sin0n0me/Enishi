#pragma once
#include "interface_d3d11_render_resources.h"
namespace enishi::renderer::directx {
    class D3D11RenderResources final : public ID3D11RenderResources {
      private:
        std::shared_ptr<D3D11> context;
        ResourceManager resource_manager;

      public:
        explicit D3D11RenderResources(std::unique_ptr<D3D11> context);
        D3D11RenderResources(const D3D11RenderResources&) = delete;
        ~D3D11RenderResources() noexcept override;
        D3D11RenderResources& operator=(const D3D11RenderResources&) = delete;
        std::shared_ptr<D3D11> get_context() noexcept override;
        ResourceManager& get_resource_manager() noexcept override;
    };
} // namespace enishi::renderer::directx
