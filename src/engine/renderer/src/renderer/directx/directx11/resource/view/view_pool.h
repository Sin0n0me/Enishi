#pragma once
#include "interface_native_view_accessor.h"
#include <renderer/common/interface_view_accessor.h>
#include <renderer/common/resource_pool.h>
#include <vector>

namespace enishi::renderer::directx {
    class ViewPool : public INativeViewAccessor, public IViewAccessor {
      private:
        ResourcePool<NativeDepthStencilView> native_depth_stencils;
        ResourcePool<NativeRenderTargetView> native_render_targets;
        ResourcePool<NativeShaderResourceView> native_shader_resources;
        ResourcePool<NativeUnorderedAccessView> native_unordered_accesses;
        ResourcePool<RenderTargetView> render_targets;
        ResourcePool<ShaderResourceView> shader_resources;

      public:
        std::tuple<std::size_t, NativeDepthStencilView&> make_native_depth_stencil_view(
            void) noexcept override;
        std::tuple<std::size_t, NativeRenderTargetView&> make_native_render_target_view(
            void) noexcept override;
        std::tuple<std::size_t, NativeShaderResourceView&> make_native_shader_resource_view(
            void) noexcept override;
        std::tuple<std::size_t, NativeUnorderedAccessView&> make_native_unordered_access_view(
            void) noexcept override;
        void remove_native_view(
            const types::ImageViewType view_kind, const std::size_t index) noexcept override;
        foundation::Option<const NativeDepthStencilView&> get_native_depth_stencil_view(
            const std::size_t index) const noexcept override;
        foundation::Option<NativeDepthStencilView> get_native_depth_stencil_view(
            const std::size_t index) noexcept override;
        foundation::Option<const NativeRenderTargetView&> get_native_render_target_view(
            const std::size_t index) const noexcept override;
        foundation::Option<NativeRenderTargetView> get_native_render_target_view(
            const std::size_t index) noexcept override;
        foundation::Option<const NativeShaderResourceView&> get_native_shader_resource_view(
            const std::size_t index) const noexcept override;
        foundation::Option<NativeShaderResourceView> get_native_shader_resource_view(
            const std::size_t index) noexcept override;
        foundation::Option<const NativeUnorderedAccessView&> get_native_unordered_access_view(
            const std::size_t index) const noexcept override;
        foundation::Option<NativeUnorderedAccessView&> get_native_unordered_access_view(
            const std::size_t index) noexcept override;

      public:
        // IViewAccessor を介して継承されました
        std::size_t make_render_target_view(RenderTargetView&& rtv) noexcept override;
        std::size_t make_shader_resource_view(ShaderResourceView&& srv) noexcept override;
        std::size_t make_depth_stencil_view(DepthStencilView&& dsv) noexcept override;
        std::size_t make_unodered_access_view(UnorderedAccessView&& uav) noexcept override;
        foundation::Option<RenderTargetView&> get_render_target_view(
            const std::size_t index) noexcept override;
        foundation::Option<const RenderTargetView&> get_render_target_view(
            const std::size_t index) const noexcept override;
        foundation::Option<ShaderResourceView&> get_shader_resource_view(
            const std::size_t index) noexcept override;
        foundation::Option<const ShaderResourceView&> get_shader_resource_view(
            const std::size_t index) const noexcept override;
        foundation::Option<DepthStencilView&> get_depth_stencil_view(
            const std::size_t index) noexcept override;
        foundation::Option<const DepthStencilView&> get_depth_stencil_view(
            const std::size_t index) const noexcept override;
        foundation::Option<UnorderedAccessView&> get_unodered_access_view(
            const std::size_t index) noexcept override;
        foundation::Option<const UnorderedAccessView&> get_unodered_access_view(
            const std::size_t index) const noexcept override;
    };
} // namespace enishi::renderer::directx