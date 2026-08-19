#pragma once
#include "buffer/interface_native_buffer_accessor.h"
#include "input_layout/interface_native_input_layout_accessor.h"
#include "rasterizer/interface_native_rasterizer_accessor.h"
#include "shader/interface_native_shader_accessor.h"
#include "texture/interface_native_texture_accessor.h"
#include "view/interface_native_view_accessor.h"
#include "viewport/interface_native_viewport_accessor.h"

namespace enishi::renderer::directx {
    class INativeResourceAccessor {
      public:
        virtual ~INativeResourceAccessor(void) noexcept = default;

        [[nodiscard]] virtual INativeBufferAccessor* get_native_buffer_accessor(void) noexcept = 0;
        [[nodiscard]] virtual const INativeBufferAccessor* get_native_buffer_accessor(void) const noexcept = 0;
        [[nodiscard]] virtual INativeInputLayoutAccessor* get_native_input_layout_accessor(void) noexcept = 0;
        [[nodiscard]] virtual const INativeInputLayoutAccessor* get_native_input_layout_accessor(
            void) const noexcept = 0;
        [[nodiscard]] virtual INativeRasterizerAccessor* get_native_rasterizer_accessor(void) noexcept = 0;
        [[nodiscard]] virtual const INativeRasterizerAccessor* get_native_rasterizer_accessor(
            void) const noexcept = 0;
        [[nodiscard]] virtual INativeTextureAccessor* get_native_texture_accessor(void) noexcept = 0;
        [[nodiscard]] virtual const INativeTextureAccessor* get_native_texture_accessor(void) const noexcept = 0;
        [[nodiscard]] virtual INativeShaderAccessor* get_native_shader_accessor(void) noexcept = 0;
        [[nodiscard]] virtual const INativeShaderAccessor* get_native_shader_accessor(void) const noexcept = 0;
        [[nodiscard]] virtual INativeViewAccessor* get_native_view_accessor(void) noexcept = 0;
        [[nodiscard]] virtual const INativeViewAccessor* get_native_view_accessor(void) const noexcept = 0;
        [[nodiscard]] virtual INativeViewportAccessor* get_native_viewport_accessor(void) noexcept = 0;
        [[nodiscard]] virtual const INativeViewportAccessor* get_native_viewport_accessor(
            void) const noexcept = 0;
    };
} // namespace enishi::renderer::directx