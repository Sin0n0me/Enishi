#include "texture_pool.h"

namespace enishi::renderer::directx {
    std::tuple<types::HandleId, TexturePool::NativeTexture1D&> TexturePool::make_native_texture_1d(
        void) noexcept {
        return this->handle_mapper.make_from(this->textures_1d.make(), [](const std::size_t index) {
            return decltype(handle_mapper)::ValueType{
                .type = TextureType::Texture1D,
                .resource_index = index,
            };
        });
    }

    std::tuple<types::HandleId, TexturePool::NativeTexture2D&> TexturePool::make_native_texture_2d(
        void) noexcept {
        return this->handle_mapper.make_from(this->textures_2d.make(), [](const std::size_t index) {
            return decltype(handle_mapper)::ValueType{
                .type = TextureType::Texture2D,
                .resource_index = index,
            };
        });
    }

    std::tuple<types::HandleId, TexturePool::NativeTexture3D&> TexturePool::make_native_texture_3d(
        void) noexcept {
        return this->handle_mapper.make_from(this->textures_3d.make(), [](const std::size_t index) {
            return decltype(handle_mapper)::ValueType{
                .type = TextureType::Texture3D,
                .resource_index = index,
            };
        });
    }

    std::tuple<types::HandleId, TexturePool::NativeSampler&> TexturePool::make_native_sampler(
        void) noexcept {
        return this->handle_mapper.make_from(this->samplers.make(), [](const std::size_t index) {
            return decltype(handle_mapper)::ValueType{
                .type = TextureType::Texture2D,
                .resource_index = index,
            };
        });
    }

    void TexturePool::remove_native_texture(
        const TextureType texture_kind, const types::HandleId handle) noexcept {
    }

    void TexturePool::remove_native_sampler(const types::HandleId handle) noexcept {
    }

    foundation::Option<TexturePool::NativeTexture1D&> TexturePool::get_native_texture_1d(
        const types::HandleId handle) noexcept {
        return this->handle_mapper.get(handle).and_then([this](const TextureHandle& texture) {
            return this->textures_1d.get(texture.resource_index);
        });
    }

    foundation::Option<const TexturePool::NativeTexture1D&> TexturePool::get_native_texture_1d(
        const types::HandleId handle) const noexcept {
        return this->handle_mapper.get(handle).and_then([this](const TextureHandle& texture) {
            return this->textures_1d.get(texture.resource_index);
        });
    }

    foundation::Option<TexturePool::NativeTexture2D&> TexturePool::get_native_texture_2d(
        const types::HandleId handle) noexcept {
        return this->handle_mapper.get(handle).and_then([this](const TextureHandle& texture) {
            return this->textures_2d.get(texture.resource_index);
        });
    }

    foundation::Option<const TexturePool::NativeTexture2D&> TexturePool::get_native_texture_2d(
        const types::HandleId handle) const noexcept {
        return this->handle_mapper.get(handle).and_then([this](const TextureHandle& texture) {
            return this->textures_2d.get(texture.resource_index);
        });
    }

    foundation::Option<TexturePool::NativeTexture3D&> TexturePool::get_native_texture_3d(
        const types::HandleId handle) noexcept {
        return this->handle_mapper.get(handle).and_then([this](const TextureHandle& texture) {
            return this->textures_3d.get(texture.resource_index);
        });
    }

    foundation::Option<const TexturePool::NativeTexture3D&> TexturePool::get_native_texture_3d(
        const types::HandleId handle) const noexcept {
        return this->handle_mapper.get(handle).and_then([this](const TextureHandle& texture) {
            return this->textures_3d.get(texture.resource_index);
        });
    }

    foundation::Option<TexturePool::NativeSampler&> TexturePool::get_native_sampler(
        const types::HandleId handle) noexcept {
        return this->handle_mapper.get(handle).and_then([this](const TextureHandle& texture) {
            return this->samplers.get(texture.resource_index);
        });
    }

    foundation::Option<const TexturePool::NativeSampler&> TexturePool::get_native_sampler(
        const types::HandleId handle) const noexcept {
        return this->handle_mapper.get(handle).and_then([this](const TextureHandle& texture) {
            return this->samplers.get(texture.resource_index);
        });
    }
} // namespace enishi::renderer::directx