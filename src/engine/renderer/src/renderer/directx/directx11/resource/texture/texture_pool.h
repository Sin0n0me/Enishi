#pragma once
#include "interface_texture_accessor.h"
#include <cstdint>
#include <d3d11.h>
#include <engine_types/assets/shader/shader_kind.h>
#include <variant>
#include <wrl/client.h>

namespace enishi::renderer::directx {
    struct SamplerParameter {
        TextureType texture_type;
        types::ShaderKind shader_kind;
        std::uint32_t target_slot;
    };

    class TexturePool : public ITextureAccessor {
      private:
        using Texture = std::variant<std::monostate, Texture1D, Texture2D, Texture3D>;

        std::vector<Texture> textures;

      public:
        std::tuple<std::size_t, Texture1D&> make_texture_1d(void) noexcept override;
        std::tuple<std::size_t, Texture2D&> make_texture_2d(void) noexcept override;
        std::tuple<std::size_t, Texture3D&> make_texture_3d(void) noexcept override;
        void remove_texture(
            const TextureType texture_kind, const std::size_t index) noexcept override;
        foundation::Option<Texture1D&> get_texture_1d(const std::size_t index) noexcept override;
        foundation::Option<const Texture1D&> get_texture_1d(
            const std::size_t index) const noexcept override;
        foundation::Option<Texture2D&> get_texture_2d(const std::size_t index) noexcept override;
        foundation::Option<const Texture2D&> get_texture_2d(
            const std::size_t index) const noexcept override;
        foundation::Option<Texture3D&> get_texture_3d(const std::size_t index) noexcept override;
        foundation::Option<const Texture3D&> get_texture_3d(
            const std::size_t index) const noexcept override;
    };
} // namespace enishi::renderer::directx