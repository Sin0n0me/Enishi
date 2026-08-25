#pragma once
#include "interface_native_texture_accessor.h"
#include <engine_types/assets/shader/shader_kind.h>
#include <engine_types/handle/handle_type.h>
#include <renderer/common/handle_mapper.h>
#include <renderer/common/resource_pool.h>

namespace enishi::renderer::directx {
    class TexturePool : public INativeTextureAccessor {
      private:
        struct TextureHandle {
            TextureType type;
            std::size_t resource_index;
        };

      private:
        HandleMapper<TextureHandle> handle_mapper;
        ResourcePool<NativeTexture1D> textures_1d;
        ResourcePool<NativeTexture2D> textures_2d;
        ResourcePool<NativeTexture3D> textures_3d;

      public:
        std::tuple<types::HandleId, NativeTexture1D&> make_native_texture_1d(
            void) noexcept override;
        std::tuple<types::HandleId, NativeTexture2D&> make_native_texture_2d(
            void) noexcept override;
        std::tuple<types::HandleId, NativeTexture3D&> make_native_texture_3d(
            void) noexcept override;
        void remove_native_texture(const types::HandleId handle) noexcept override;
        foundation::Option<NativeTexture1D&> get_native_texture_1d(
            const types::HandleId handle) noexcept override;
        foundation::Option<const NativeTexture1D&> get_native_texture_1d(
            const types::HandleId handle) const noexcept override;
        foundation::Option<NativeTexture2D&> get_native_texture_2d(
            const types::HandleId handle) noexcept override;
        foundation::Option<const NativeTexture2D&> get_native_texture_2d(
            const types::HandleId handle) const noexcept override;
        foundation::Option<NativeTexture3D&> get_native_texture_3d(
            const types::HandleId handle) noexcept override;
        foundation::Option<const NativeTexture3D&> get_native_texture_3d(
            const types::HandleId handle) const noexcept override;
    };
} // namespace enishi::renderer::directx