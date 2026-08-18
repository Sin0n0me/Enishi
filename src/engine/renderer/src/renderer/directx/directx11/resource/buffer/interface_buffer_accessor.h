#pragma once
#include <d3d11.h>
#include <foundation/option/option.h>
#include <tuple>
#include <wrl/client.h>

namespace enishi::renderer::directx {
    class IBufferAccessor {
      public:
        using Buffer = Microsoft::WRL::ComPtr<ID3D11Buffer>;

      public:
        virtual ~IBufferAccessor(void) noexcept = default;

        [[nodiscard]] virtual std::tuple<std::size_t, Buffer&> make_buffer(void) noexcept = 0;
        [[nodiscard]] virtual void remove_buffer(const std::size_t index) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<Buffer&> get_buffer(
            const std::size_t index) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const Buffer&> get_buffer(
            const std::size_t index) const noexcept = 0;
    };
} // namespace enishi::renderer::directx