#pragma once
#include <d3d11.h>
#include <foundation/option/option.h>
#include <tuple>
#include <wrl/client.h>

namespace enishi::renderer::directx {
    enum class TextureType {
        Texture1D,
        Texture2D,
        Texture3D,
    };

    class ITextureAccessor {
      public:
        using Texture1D = Microsoft::WRL::ComPtr<ID3D11Texture1D>;
        using Texture2D = Microsoft::WRL::ComPtr<ID3D11Texture2D>;
        using Texture3D = Microsoft::WRL::ComPtr<ID3D11Texture3D>;

      public:
        virtual ~ITextureAccessor(void) noexcept = default;

        [[nodiscard]] virtual std::tuple<std::size_t, Texture1D&> make_texture_1d(
            void) noexcept = 0;
        [[nodiscard]] virtual std::tuple<std::size_t, Texture2D&> make_texture_2d(
            void) noexcept = 0;
        [[nodiscard]] virtual std::tuple<std::size_t, Texture3D&> make_texture_3d(
            void) noexcept = 0;
        [[nodiscard]] virtual void remove_texture(
            const TextureType texture_kind, const std::size_t index) noexcept = 0;

        [[nodiscard]] virtual foundation::Option<Texture1D&> get_texture_1d(
            const std::size_t index) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const Texture1D&> get_texture_1d(
            const std::size_t index) const noexcept = 0;
        [[nodiscard]] virtual foundation::Option<Texture2D&> get_texture_2d(
            const std::size_t index) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const Texture2D&> get_texture_2d(
            const std::size_t index) const noexcept = 0;
        [[nodiscard]] virtual foundation::Option<Texture3D&> get_texture_3d(
            const std::size_t index) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const Texture3D&> get_texture_3d(
            const std::size_t index) const noexcept = 0;
    };
} // namespace enishi::renderer::directx