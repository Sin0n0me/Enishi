#pragma once
#include <d3d11.h>
#include <foundation/option/option.h>
#include <span>
#include <tuple>

namespace enishi::renderer::directx {
    class INativeViewportAccessor {
      public:
        using Viewport = D3D11_VIEWPORT;

      public:
        virtual ~INativeViewportAccessor(void) noexcept = default;

        [[nodiscard]] virtual std::tuple<std::size_t, Viewport&> make_native_viewport(
            void) noexcept = 0;
        [[nodiscard]] virtual void remove_native_viewport(const std::size_t index) noexcept = 0;

        [[nodiscard]] virtual foundation::Option<Viewport&> get_native_viewport(
            const std::size_t index) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const Viewport&> get_native_viewport(
            const std::size_t index) const noexcept = 0;
        [[nodiscard]] virtual std::span<const Viewport> get_native_viewports(
            void) const noexcept = 0;
    };
} // namespace enishi::renderer::directx