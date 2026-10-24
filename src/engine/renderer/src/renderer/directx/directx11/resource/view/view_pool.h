#pragma once
#include "interface_native_view_accessor.h"
#include <engine_types/handle/handle_mapper.h>
#include <foundation/pool/resource_pool.h>
#include <renderer/common/resource_accessor.h>

namespace enishi::renderer::directx {
    class ViewPool : public INativeViewAccessor {
      private:
        struct ViewHandle {
            types::ImageViewType type;
            std::size_t resource_index;
        };

      private:
        ResourceAccessor& interfaces;
        types::ResourceMapper<ViewHandle> handle_mapper;
        foundation::ResourcePool<NativeDepthStencilView> native_depth_stencils;
        foundation::ResourcePool<NativeRenderTargetView> native_render_targets;
        foundation::ResourcePool<NativeShaderResourceView> native_shader_resources;
        foundation::ResourcePool<NativeUnorderedAccessView> native_unordered_accesses;

      public:
        explicit ViewPool(ResourceAccessor& interfaces);
        std::tuple<types::HandleId, NativeDepthStencilView&> make_native_depth_stencil_view(
            void) noexcept override;
        std::tuple<types::HandleId, NativeRenderTargetView&> make_native_render_target_view(
            void) noexcept override;
        std::tuple<types::HandleId, NativeShaderResourceView&> make_native_shader_resource_view(
            void) noexcept override;
        std::tuple<types::HandleId, NativeUnorderedAccessView&> make_native_unordered_access_view(
            void) noexcept override;
        void remove_native_view(const types::HandleId handle) noexcept override;
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
    };
} // namespace enishi::renderer::directx
