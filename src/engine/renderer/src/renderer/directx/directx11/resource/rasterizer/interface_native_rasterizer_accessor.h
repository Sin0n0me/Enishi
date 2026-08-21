#pragma once
#include <d3d11.h>
#include <engine_types/handle/handle_type.h>
#include <foundation/option/option.h>
#include <tuple>
#include <wrl/client.h>

namespace enishi::renderer::directx {
    class INativeRasterizerAccessor {
      public:
        using NativeRasterizer = Microsoft::WRL::ComPtr<ID3D11RasterizerState>;

      public:
        virtual ~INativeRasterizerAccessor(void) noexcept = default;

        [[nodiscard]] virtual std::tuple<types::HandleId, NativeRasterizer&> make_native_rasterizer(
            void) noexcept = 0;
        [[nodiscard]] virtual void remove_native_rasterizer(
            const types::HandleId handle) noexcept = 0;

        [[nodiscard]] virtual foundation::Option<NativeRasterizer&> get_native_rasterizer(
            const types::HandleId handle) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const NativeRasterizer&> get_native_rasterizer(
            const types::HandleId handle) const noexcept = 0;
    };
} // namespace enishi::renderer::directx