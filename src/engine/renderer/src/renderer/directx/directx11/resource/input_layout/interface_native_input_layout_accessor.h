#pragma once
#include <d3d11.h>
#include <foundation/option/option.h>
#include <tuple>
#include <wrl/client.h>

namespace enishi::renderer::directx {
    class INativeInputLayoutAccessor {
      public:
        using NativeInputLayout = Microsoft::WRL::ComPtr<ID3D11InputLayout>;

      public:
        virtual ~INativeInputLayoutAccessor(void) noexcept = default;

        [[nodiscard]] virtual std::tuple<std::size_t, NativeInputLayout&> make_native_input_layout(
            void) noexcept = 0;
        [[nodiscard]] virtual void remove_native_input_layout(const std::size_t index) noexcept = 0;

        [[nodiscard]] virtual foundation::Option<NativeInputLayout&> get_native_input_layout(
            const std::size_t index) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const NativeInputLayout&> get_native_input_layout(
            const std::size_t index) const noexcept = 0;
    };
} // namespace enishi::renderer::directx