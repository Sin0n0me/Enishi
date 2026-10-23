#pragma once
#include <memory>
#include <renderer/directx/directx11/d3d11.h>
#include <renderer/directx/directx11/resource/resource_manager.h>
namespace enishi::renderer::directx {
    class ID3D11RenderResources {
      public:
        virtual ~ID3D11RenderResources() noexcept = default;
        virtual std::shared_ptr<D3D11> get_context() noexcept = 0;
        virtual ResourceManager& get_resource_manager() noexcept = 0;
    };
} // namespace enishi::renderer::directx
