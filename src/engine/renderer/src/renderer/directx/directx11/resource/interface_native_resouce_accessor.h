#pragma once
#include "buffer/interface_buffer_accessor.h"
#include "shader/interface_shader_accessor.h"
#include "texture/interface_texture_accessor.h"
#include "view/interface_view_accessor.h"

namespace enishi::renderer::directx {
    class INativeResourceAccessor {
      public:
        virtual ~INativeResourceAccessor(void) noexcept = default;

        [[nodiscard]] virtual IBufferAccessor* get_buffer_accessor(void) noexcept = 0;
        [[nodiscard]] virtual const IBufferAccessor* get_buffer_accessor(void) const noexcept = 0;
        [[nodiscard]] virtual ITextureAccessor* get_texture_accessor(void) noexcept = 0;
        [[nodiscard]] virtual const ITextureAccessor* get_texture_accessor(void) const noexcept = 0;
        [[nodiscard]] virtual IShaderAccessor* get_shader_accessor(void) noexcept = 0;
        [[nodiscard]] virtual const IShaderAccessor* get_shader_accessor(void) const noexcept = 0;
        [[nodiscard]] virtual IViewAccessor* get_view_accessor(void) noexcept = 0;
        [[nodiscard]] virtual const IViewAccessor* get_view_accessor(void) const noexcept = 0;
    };
} // namespace enishi::renderer::directx