#pragma once
#include "interface_native_view_accessor.h"
#include <engine_types/handle/renderer/handles/resource_handles.h>
#include <platform/renderer/view/interface_view_accessor.h>
#include <renderer/common/resource_pool.h>

namespace enishi::renderer::directx {
    class ViewPool : public INativeViewAccessor, public platform::IViewAccessor {
      private:
        struct ViewHandle {
            types::ImageViewType type;
            std::size_t resource_index;
            std::size_t interface_index;
        };

      private:
        types::ResourceMapper<ViewHandle> handle_mapper;
        ResourcePool<NativeDepthStencilView> native_depth_stencils;
        ResourcePool<NativeRenderTargetView> native_render_targets;
        ResourcePool<NativeShaderResourceView> native_shader_resources;
        ResourcePool<NativeUnorderedAccessView> native_unordered_accesses;
        ResourcePool<RenderTargetView> render_targets;
        ResourcePool<ShaderResourceView> shader_resources;
        ResourcePool<DepthStencilView> depth_stencil;
        ResourcePool<UnorderedAccessView> unodered_access;

      public:
        std::tuple<types::HandleId, NativeDepthStencilView&> make_native_depth_stencil_view(
            void) noexcept override;
        std::tuple<types::HandleId, NativeRenderTargetView&> make_native_render_target_view(
            void) noexcept override;
        std::tuple<types::HandleId, NativeShaderResourceView&> make_native_shader_resource_view(
            void) noexcept override;
        std::tuple<types::HandleId, NativeUnorderedAccessView&> make_native_unordered_access_view(
            void) noexcept override;
        void remove_native_view(const types::HandleId handle) noexcept override;
        foundation::Option<types::ImageViewType> get_view_type(
            const types::HandleId& handle) const noexcept override;
        foundation::Option<const NativeDepthStencilView&> get_native_depth_stencil_view(
            const types::HandleId handle) const noexcept override;
        foundation::Option<NativeDepthStencilView&> get_native_depth_stencil_view(
            const types::HandleId handle) noexcept override;
        foundation::Option<const NativeRenderTargetView&> get_native_render_target_view(
            const types::HandleId handle) const noexcept override;
        foundation::Option<NativeRenderTargetView&> get_native_render_target_view(
            const types::HandleId handle) noexcept override;
        foundation::Option<const NativeShaderResourceView&> get_native_shader_resource_view(
            const types::HandleId handle) const noexcept override;
        foundation::Option<NativeShaderResourceView&> get_native_shader_resource_view(
            const types::HandleId handle) noexcept override;
        foundation::Option<const NativeUnorderedAccessView&> get_native_unordered_access_view(
            const types::HandleId handle) const noexcept override;
        foundation::Option<NativeUnorderedAccessView&> get_native_unordered_access_view(
            const types::HandleId handle) noexcept override;

      public:
        types::HandleId make_render_target_view(
            const types::HandleId& handle, RenderTargetView&& rtv) noexcept override;
        types::HandleId make_shader_resource_view(
            const types::HandleId& handle, ShaderResourceView&& srv) noexcept override;
        types::HandleId make_depth_stencil_view(
            const types::HandleId& handle, DepthStencilView&& dsv) noexcept override;
        types::HandleId make_unodered_access_view(
            const types::HandleId& handle, UnorderedAccessView&& uav) noexcept override;
        foundation::Option<RenderTargetView&> get_render_target_view(
            const types::HandleId handle) noexcept override;
        foundation::Option<const RenderTargetView&> get_render_target_view(
            const types::HandleId handle) const noexcept override;
        foundation::Option<ShaderResourceView&> get_shader_resource_view(
            const types::HandleId handle) noexcept override;
        foundation::Option<const ShaderResourceView&> get_shader_resource_view(
            const types::HandleId handle) const noexcept override;
        foundation::Option<DepthStencilView&> get_depth_stencil_view(
            const types::HandleId handle) noexcept override;
        foundation::Option<const DepthStencilView&> get_depth_stencil_view(
            const types::HandleId handle) const noexcept override;
        foundation::Option<UnorderedAccessView&> get_unodered_access_view(
            const types::HandleId handle) noexcept override;
        foundation::Option<const UnorderedAccessView&> get_unodered_access_view(
            const types::HandleId handle) const noexcept override;
        std::span<const RenderTargetView> get_render_target_views(void) const noexcept override;
        std::span<const ShaderResourceView> get_shader_resource_views(void) const noexcept override;
        std::span<const DepthStencilView> get_depth_stencil_views(void) const noexcept override;
        std::span<const UnorderedAccessView> get_unodered_access_views(
            void) const noexcept override;
    };
} // namespace enishi::renderer::directx