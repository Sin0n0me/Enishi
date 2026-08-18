#include "texture_pool.h"

namespace enishi::renderer::directx {
    std::tuple<std::size_t, TexturePool::Texture1D&> TexturePool::make_texture_1d(void) noexcept {
        const auto index = this->textures.size();
        auto texture = Texture1D{};
        this->textures.emplace_back(texture);
        return {index, texture};
    }

    std::tuple<std::size_t, TexturePool::Texture2D&> TexturePool::make_texture_2d(void) noexcept {
        const auto index = this->textures.size();
        auto texture = Texture2D{};
        this->textures.emplace_back(texture);
        return {index, texture};
    }

    std::tuple<std::size_t, TexturePool::Texture3D&> TexturePool::make_texture_3d(void) noexcept {
        const auto index = this->textures.size();
        auto texture = Texture3D{};
        this->textures.emplace_back(texture);
        return {index, texture};
    }

    void TexturePool::remove_texture(
        const TextureType texture_kind, const std::size_t index) noexcept {
    }

    foundation::Option<TexturePool::Texture1D&> TexturePool::get_texture_1d(
        const std::size_t index) noexcept {
        return foundation::Option<Texture1D&>();
    }

    foundation::Option<const TexturePool::Texture1D&> TexturePool::get_texture_1d(
        const std::size_t index) const noexcept {
        return foundation::Option<const Texture1D&>();
    }

    foundation::Option<TexturePool::Texture2D&> TexturePool::get_texture_2d(
        const std::size_t index) noexcept {
        return foundation::Option<Texture2D&>();
    }

    foundation::Option<const TexturePool::Texture2D&> TexturePool::get_texture_2d(
        const std::size_t index) const noexcept {
        return foundation::Option<const Texture2D&>();
    }

    foundation::Option<TexturePool::Texture3D&> TexturePool::get_texture_3d(
        const std::size_t index) noexcept {
        return foundation::Option<Texture3D&>();
    }
    foundation::Option<const TexturePool::Texture3D&> TexturePool::get_texture_3d(
        const std::size_t index) const noexcept {
        return foundation::Option<const Texture3D&>();
    }
} // namespace enishi::renderer::directx