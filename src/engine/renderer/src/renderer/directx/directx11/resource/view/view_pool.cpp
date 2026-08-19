#include "view_pool.h"

namespace enishi::renderer::directx {
    std::tuple<std::size_t, ViewPool::NativeDepthStencilView&>
    ViewPool::make_native_depth_stencil_view(void) noexcept {
        return this->native_depth_stencils.make();
    }

    std::tuple<std::size_t, ViewPool::NativeRenderTargetView&>
    ViewPool::make_native_render_target_view(void) noexcept {
        return this->native_render_targets.make();
    }

    std::tuple<std::size_t, ViewPool::NativeShaderResourceView&>
    ViewPool::make_native_shader_resource_view(void) noexcept {
        return this->native_shader_resources.make();
    }

    std::tuple<std::size_t, ViewPool::NativeUnorderedAccessView&>
    ViewPool::make_native_unordered_access_view(void) noexcept {
        return this->native_unordered_accesses.make();
    }

    void ViewPool::remove_native_view(
        const types::ImageViewType view_kind, const std::size_t index) noexcept {
    }

    foundation::Option<const ViewPool::NativeDepthStencilView&>
    ViewPool::get_native_depth_stencil_view(const std::size_t index) const noexcept {
        return this->native_depth_stencils.get(index);
    }

    foundation::Option<ViewPool::NativeDepthStencilView> ViewPool::get_native_depth_stencil_view(
        const std::size_t index) noexcept {
        return this->native_depth_stencils.get(index);
    }

    foundation::Option<const ViewPool::NativeRenderTargetView&>
    ViewPool::get_native_render_target_view(const std::size_t index) const noexcept {
        return this->native_render_targets.get(index);
    }

    foundation::Option<ViewPool::NativeRenderTargetView> ViewPool::get_native_render_target_view(
        const std::size_t index) noexcept {
        return this->native_render_targets.get(index);
    }

    foundation::Option<const ViewPool::NativeShaderResourceView&>
    ViewPool::get_native_shader_resource_view(const std::size_t index) const noexcept {
        return this->native_shader_resources.get(index);
    }

    foundation::Option<ViewPool::NativeShaderResourceView>
    ViewPool::get_native_shader_resource_view(const std::size_t index) noexcept {
        return this->native_shader_resources.get(index);
    }

    foundation::Option<const ViewPool::NativeUnorderedAccessView&>
    ViewPool::get_native_unordered_access_view(const std::size_t index) const noexcept {
        return this->native_unordered_accesses.get(index);
    }

    foundation::Option<ViewPool::NativeUnorderedAccessView&>
    ViewPool::get_native_unordered_access_view(const std::size_t index) noexcept {
        return this->native_unordered_accesses.get(index);
    }

    std::size_t ViewPool::make_render_target_view(RenderTargetView&& rtv) noexcept {
        return std::size_t();
    }
    std::size_t ViewPool::make_shader_resource_view(ShaderResourceView&& srv) noexcept {
        return std::size_t();
    }
    std::size_t ViewPool::make_depth_stencil_view(DepthStencilView&& dsv) noexcept {
        return std::size_t();
    }
    std::size_t ViewPool::make_unodered_access_view(UnorderedAccessView&& uav) noexcept {
        return std::size_t();
    }
    foundation::Option<RenderTargetView&> ViewPool::get_render_target_view(
        const std::size_t index) noexcept {
        return foundation::Option<RenderTargetView&>();
    }
    foundation::Option<const RenderTargetView&> ViewPool::get_render_target_view(
        const std::size_t index) const noexcept {
        return foundation::Option<const RenderTargetView&>();
    }
    foundation::Option<ShaderResourceView&> ViewPool::get_shader_resource_view(
        const std::size_t index) noexcept {
        return foundation::Option<ShaderResourceView&>();
    }
    foundation::Option<const ShaderResourceView&> ViewPool::get_shader_resource_view(
        const std::size_t index) const noexcept {
        return foundation::Option<const ShaderResourceView&>();
    }
    foundation::Option<DepthStencilView&> ViewPool::get_depth_stencil_view(
        const std::size_t index) noexcept {
        return foundation::Option<DepthStencilView&>();
    }
    foundation::Option<const DepthStencilView&> ViewPool::get_depth_stencil_view(
        const std::size_t index) const noexcept {
        return foundation::Option<const DepthStencilView&>();
    }
    foundation::Option<UnorderedAccessView&> ViewPool::get_unodered_access_view(
        const std::size_t index) noexcept {
        return foundation::Option<UnorderedAccessView&>();
    }
    foundation::Option<const UnorderedAccessView&> ViewPool::get_unodered_access_view(
        const std::size_t index) const noexcept {
        return foundation::Option<const UnorderedAccessView&>();
    }
} // namespace enishi::renderer::directx