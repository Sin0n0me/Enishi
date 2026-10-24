#include "d3d11_render_resources.h"
#include <utility>
namespace enishi::renderer::directx {
    D3D11RenderResources::D3D11RenderResources(std::unique_ptr<D3D11> context)
        : context(std::move(context))
        , resource_manager(this->context) {
    }
    D3D11RenderResources::~D3D11RenderResources() noexcept {
    }
    std::shared_ptr<D3D11> D3D11RenderResources::get_context() noexcept {
        return this->context;
    }
    ResourceManager& D3D11RenderResources::get_resource_manager() noexcept {
        return this->resource_manager;
    }
} // namespace enishi::renderer::directx
