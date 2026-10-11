#pragma once
#include <d3d11.h>
#include <engine_types/handle/handle_type.h>
#include <foundation/option/option.h>
#include <span>
#include <tuple>

namespace enishi::renderer::directx {
    class INativeViewportAccessor {
      public:
        using NativeViewport = D3D11_VIEWPORT;

      public:
        virtual ~INativeViewportAccessor(void) noexcept = default;

        [[nodiscard]] virtual std::tuple<types::HandleId, NativeViewport&> make_native_viewport(
            void) noexcept = 0;
        [[nodiscard]] virtual void remove_native_viewport(
            const types::HandleId handle) noexcept = 0;

        [[nodiscard]] virtual foundation::Option<NativeViewport&> get_native_viewport(
            const types::HandleId handle) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const NativeViewport&> get_native_viewport(
            const types::HandleId handle) const noexcept = 0;
        [[nodiscard]] virtual std::span<const NativeViewport> get_native_viewports(
            void) const noexcept = 0;
    };
} // namespace enishi::renderer::directx