#pragma once
#include "interface_shader_input_reflection.h"
#include <foundation/option/option.h>
#include <memory>
#include <platform/renderer/interface_image_view.h>

namespace enishi::renderer {
    class IViewAccessor {
      public:
        using RenderTargetView = std::shared_ptr<platform::IRenderTargetView>;
        using ShaderResourceView = std::shared_ptr<platform::IShaderResourceView>;
        using DepthStencilView = std::shared_ptr<platform::IDepthStencilView>;
        using UnorderedAccessView = std::shared_ptr<platform::IUnorderedAccessView>;

      public:
        virtual ~IViewAccessor(void) noexcept = default;

        [[nodiscard]] virtual std::size_t make_render_target_view(
            RenderTargetView&& rtv) noexcept = 0;
        [[nodiscard]] virtual std::size_t make_shader_resource_view(
            ShaderResourceView&& srv) noexcept = 0;
        [[nodiscard]] virtual std::size_t make_depth_stencil_view(
            DepthStencilView&& dsv) noexcept = 0;
        [[nodiscard]] virtual std::size_t make_unodered_access_view(
            UnorderedAccessView&& uav) noexcept = 0;

        [[nodiscard]] virtual foundation::Option<RenderTargetView&> get_render_target_view(
            const std::size_t index) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const RenderTargetView&> get_render_target_view(
            const std::size_t index) const noexcept = 0;
        [[nodiscard]] virtual foundation::Option<ShaderResourceView&> get_shader_resource_view(
            const std::size_t index) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const ShaderResourceView&>
        get_shader_resource_view(const std::size_t index) const noexcept = 0;
        [[nodiscard]] virtual foundation::Option<DepthStencilView&> get_depth_stencil_view(
            const std::size_t index) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const DepthStencilView&> get_depth_stencil_view(
            const std::size_t index) const noexcept = 0;
        [[nodiscard]] virtual foundation::Option<UnorderedAccessView&> get_unodered_access_view(
            const std::size_t index) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const UnorderedAccessView&>
        get_unodered_access_view(const std::size_t index) const noexcept = 0;
    };
} // namespace enishi::renderer