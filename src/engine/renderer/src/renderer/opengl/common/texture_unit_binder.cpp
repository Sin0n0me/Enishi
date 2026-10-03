#include "texture_unit_binder.h"

namespace enishi::renderer::opengl {
    constexpr std::uint32_t UNBOUND_TEXTURE = 0;

    void TextureUnitBinder::bind(
        const std::uint32_t binding, const std::uint32_t texture) noexcept {
        this->textures.insert_or_assign(binding, texture);
    }
    std::uint32_t TextureUnitBinder::get(const std::uint32_t binding) const noexcept {
        const auto iter = this->textures.find(binding);
        return iter == this->textures.end() ? UNBOUND_TEXTURE : iter->second;
    }
} // namespace enishi::renderer::opengl
