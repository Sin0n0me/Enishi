#include "texture_pool.h"

namespace enishi::renderer::directx {
    std::tuple<std::size_t, TexturePool::NativeTexture1D&> TexturePool::make_native_texture_1d(
        void) noexcept {
        auto texture = NativeTexture1D{};
        const auto [index, tex] = this->textures.emplace(texture);
        return {index, texture};
    }

    std::tuple<std::size_t, TexturePool::NativeTexture2D&> TexturePool::make_native_texture_2d(
        void) noexcept {
        auto texture = NativeTexture2D{};
        const auto [index, tex] = this->textures.emplace(texture);
        return {index, texture};
    }

    std::tuple<std::size_t, TexturePool::NativeTexture3D&> TexturePool::make_native_texture_3d(
        void) noexcept {
        auto texture = NativeTexture3D{};
        const auto [index, tex] = this->textures.emplace(texture);
        return {index, texture};
    }

    std::tuple<std::size_t, TexturePool::NativeSampler&> TexturePool::make_native_sampler(
        void) noexcept {
        return this->samplers.make();
    }

    void TexturePool::remove_native_texture(
        const TextureType texture_kind, const std::size_t index) noexcept {
    }

    void TexturePool::remove_native_sampler(const std::size_t index) noexcept {
    }

    foundation::Option<TexturePool::NativeTexture1D&> TexturePool::get_native_texture_1d(
        const std::size_t index) noexcept {
        auto texture = this->textures.get(index);
        if (texture.is_none()) {
            return {};
        }
        if (auto tex = std::get_if<NativeTexture1D>(&texture.unwrap_mut())) {
            return *tex;
        }
        return {};
    }

    foundation::Option<const TexturePool::NativeTexture1D&> TexturePool::get_native_texture_1d(
        const std::size_t index) const noexcept {
        const auto texture = this->textures.get(index);
        if (texture.is_none()) {
            return {};
        }
        if (auto tex = std::get_if<NativeTexture1D>(&texture.unwrap())) {
            return *tex;
        }
        return {};
    }

    foundation::Option<TexturePool::NativeTexture2D&> TexturePool::get_native_texture_2d(
        const std::size_t index) noexcept {
        auto texture = this->textures.get(index);
        if (texture.is_none()) {
            return {};
        }
        if (auto tex = std::get_if<NativeTexture2D>(&texture.unwrap_mut())) {
            return *tex;
        }
        return {};
    }

    foundation::Option<const TexturePool::NativeTexture2D&> TexturePool::get_native_texture_2d(
        const std::size_t index) const noexcept {
        const auto texture = this->textures.get(index);
        if (texture.is_none()) {
            return {};
        }
        if (auto tex = std::get_if<NativeTexture2D>(&texture.unwrap())) {
            return *tex;
        }
        return {};
    }

    foundation::Option<TexturePool::NativeTexture3D&> TexturePool::get_native_texture_3d(
        const std::size_t index) noexcept {
        auto texture = this->textures.get(index);
        if (texture.is_none()) {
            return {};
        }
        if (auto tex = std::get_if<NativeTexture3D>(&texture.unwrap_mut())) {
            return *tex;
        }
        return {};
    }

    foundation::Option<const TexturePool::NativeTexture3D&> TexturePool::get_native_texture_3d(
        const std::size_t index) const noexcept {
        const auto texture = this->textures.get(index);
        if (texture.is_none()) {
            return {};
        }
        if (auto tex = std::get_if<NativeTexture3D>(&texture.unwrap())) {
            return *tex;
        }
        return {};
    }

    foundation::Option<TexturePool::NativeSampler&> TexturePool::get_native_sampler(
        const size_t index) noexcept {
        const auto sampler = this->samplers.get(index);
        if (sampler.is_none()) {
            return {};
        }
        return sampler;
    }

    foundation::Option<const TexturePool::NativeSampler&> TexturePool::get_native_sampler(
        const size_t index) const noexcept {
        const auto sampler = this->samplers.get(index);
        if (sampler.is_none()) {
            return {};
        }
        return sampler;
    }
} // namespace enishi::renderer::directx