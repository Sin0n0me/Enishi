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

    class INativeTextureAccessor {
      public:
        using NativeTexture1D = Microsoft::WRL::ComPtr<ID3D11Texture1D>;
        using NativeTexture2D = Microsoft::WRL::ComPtr<ID3D11Texture2D>;
        using NativeTexture3D = Microsoft::WRL::ComPtr<ID3D11Texture3D>;
        using NativeSampler = Microsoft::WRL::ComPtr<ID3D11SamplerState>;

      public:
        virtual ~INativeTextureAccessor(void) noexcept = default;

        [[nodiscard]] virtual std::tuple<std::size_t, NativeTexture1D&> make_native_texture_1d(
            void) noexcept = 0;
        [[nodiscard]] virtual std::tuple<std::size_t, NativeTexture2D&> make_native_texture_2d(
            void) noexcept = 0;
        [[nodiscard]] virtual std::tuple<std::size_t, NativeTexture3D&> make_native_texture_3d(
            void) noexcept = 0;
        [[nodiscard]] virtual std::tuple<std::size_t, NativeSampler&> make_native_sampler(
            void) noexcept = 0;
        [[nodiscard]] virtual void remove_native_texture(
            const TextureType texture_kind, const std::size_t index) noexcept = 0;
        [[nodiscard]] virtual void remove_native_sampler(const std::size_t index) noexcept = 0;

        [[nodiscard]] virtual foundation::Option<NativeTexture1D&> get_native_texture_1d(
            const std::size_t index) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const NativeTexture1D&> get_native_texture_1d(
            const std::size_t index) const noexcept = 0;
        [[nodiscard]] virtual foundation::Option<NativeTexture2D&> get_native_texture_2d(
            const std::size_t index) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const NativeTexture2D&> get_native_texture_2d(
            const std::size_t index) const noexcept = 0;
        [[nodiscard]] virtual foundation::Option<NativeTexture3D&> get_native_texture_3d(
            const std::size_t index) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const NativeTexture3D&> get_native_texture_3d(
            const std::size_t index) const noexcept = 0;
        [[nodiscard]] virtual foundation::Option<NativeSampler&> get_native_sampler(
            const std::size_t index) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const NativeSampler&> get_native_sampler(
            const std::size_t index) const noexcept = 0;
    };
} // namespace enishi::renderer::directx