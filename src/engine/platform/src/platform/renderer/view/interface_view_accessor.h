#pragma once
#include <engine_types/renderer/description/view/image_view_description.h>
#include <foundation/option/option.h>
#include <memory>
#include <platform/renderer/view/interface_image_view.h>

namespace enishi::platform {
    class IViewAccessor {
      public:
        using RenderTargetView = std::shared_ptr<platform::IRenderTargetView>;
        using ShaderResourceView = std::shared_ptr<platform::IShaderResourceView>;
        using DepthStencilView = std::shared_ptr<platform::IDepthStencilView>;
        using UnorderedAccessView = std::shared_ptr<platform::IUnorderedAccessView>;

      public:
        virtual ~IViewAccessor(void) noexcept = default;

        [[nodiscard]] virtual foundation::Option<types::ImageViewType> get_view_type(
            const types::HandleId& handle) const noexcept = 0;

        /**
         * @param handle ネイティブなリソースへのハンドル
         * @param rtv ネイティブなリソースを外部から扱うためのインターフェイス
         */
        [[nodiscard]] virtual types::HandleId make_render_target_view(
            const types::HandleId& handle, RenderTargetView&& rtv) noexcept = 0;
        [[nodiscard]] virtual types::HandleId make_shader_resource_view(
            const types::HandleId& handle, ShaderResourceView&& srv) noexcept = 0;
        [[nodiscard]] virtual types::HandleId make_depth_stencil_view(
            const types::HandleId& handle, DepthStencilView&& dsv) noexcept = 0;
        [[nodiscard]] virtual types::HandleId make_unodered_access_view(
            const types::HandleId& handle, UnorderedAccessView&& uav) noexcept = 0;

        [[nodiscard]] virtual foundation::Option<RenderTargetView&> get_render_target_view(
            const types::HandleId handle) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const RenderTargetView&> get_render_target_view(
            const types::HandleId handle) const noexcept = 0;
        [[nodiscard]] virtual foundation::Option<ShaderResourceView&> get_shader_resource_view(
            const types::HandleId handle) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const ShaderResourceView&>
        get_shader_resource_view(const types::HandleId handle) const noexcept = 0;
        [[nodiscard]] virtual foundation::Option<DepthStencilView&> get_depth_stencil_view(
            const types::HandleId handle) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const DepthStencilView&> get_depth_stencil_view(
            const types::HandleId handle) const noexcept = 0;
        [[nodiscard]] virtual foundation::Option<UnorderedAccessView&> get_unodered_access_view(
            const types::HandleId handle) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const UnorderedAccessView&>
        get_unodered_access_view(const types::HandleId handle) const noexcept = 0;

        [[nodiscard]] virtual std::span<const RenderTargetView> get_render_target_views(
            void) const noexcept = 0;
        [[nodiscard]] virtual std::span<const ShaderResourceView> get_shader_resource_views(
            void) const noexcept = 0;
        [[nodiscard]] virtual std::span<const DepthStencilView> get_depth_stencil_views(
            void) const noexcept = 0;
        [[nodiscard]] virtual std::span<const UnorderedAccessView> get_unodered_access_views(
            void) const noexcept = 0;
    };
} // namespace enishi::platform