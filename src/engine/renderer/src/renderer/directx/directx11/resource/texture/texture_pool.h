#pragma once
#include "interface_native_texture_accessor.h"
#include <cstdint>
#include <d3d11.h>
#include <engine_types/assets/shader/shader_kind.h>
#include <renderer/common/resource_pool.h>
#include <variant>
#include <wrl/client.h>

namespace enishi::renderer::directx {
    struct SamplerParameter {
        TextureType texture_type;
        types::ShaderKind shader_kind;
        std::uint32_t target_slot;
    };

    class TexturePool : public INativeTextureAccessor {
      private:
        using NativeTexture =
            std::variant<std::monostate, NativeTexture1D, NativeTexture2D, NativeTexture3D>;

        ResourcePool<NativeTexture> textures;
        ResourcePool<NativeSampler> samplers;

      public:
        std::tuple<std::size_t, NativeTexture1D&> make_native_texture_1d(void) noexcept override;
        std::tuple<std::size_t, NativeTexture2D&> make_native_texture_2d(void) noexcept override;
        std::tuple<std::size_t, NativeTexture3D&> make_native_texture_3d(void) noexcept override;
        std::tuple<std::size_t, NativeSampler&> make_native_sampler(void) noexcept override;
        void remove_native_texture(
            const TextureType texture_kind, const std::size_t index) noexcept override;
        void remove_native_sampler(const std::size_t index) noexcept override;
        foundation::Option<NativeTexture1D&> get_native_texture_1d(
            const std::size_t index) noexcept override;
        foundation::Option<const NativeTexture1D&> get_native_texture_1d(
            const std::size_t index) const noexcept override;
        foundation::Option<NativeTexture2D&> get_native_texture_2d(
            const std::size_t index) noexcept override;
        foundation::Option<const NativeTexture2D&> get_native_texture_2d(
            const std::size_t index) const noexcept override;
        foundation::Option<NativeTexture3D&> get_native_texture_3d(
            const std::size_t index) noexcept override;
        foundation::Option<const NativeTexture3D&> get_native_texture_3d(
            const std::size_t index) const noexcept override;
        foundation::Option<NativeSampler&> get_native_sampler(const size_t index) noexcept override;
        foundation::Option<const NativeSampler&> get_native_sampler(
            const size_t index) const noexcept override;
    };
} // namespace enishi::renderer::directx