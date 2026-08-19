#include "native_gpu_resource.h"

namespace enishi::renderer::directx {
    INativeBufferAccessor* NativeGPUResource::get_native_buffer_accessor(void) noexcept {
        return this->buffer_pool.get();
    }
    const INativeBufferAccessor* NativeGPUResource::get_native_buffer_accessor(void) const noexcept {
        return this->buffer_pool.get();
    }
    INativeTextureAccessor* NativeGPUResource::get_native_texture_accessor(void) noexcept {
        return this->texture_pool.get();
    }
    const INativeTextureAccessor* NativeGPUResource::get_native_texture_accessor(void) const noexcept {
        return this->texture_pool.get();
    }
    INativeShaderAccessor* NativeGPUResource::get_native_shader_accessor(void) noexcept {
        return this->shader_pool.get();
    }
    const INativeShaderAccessor* NativeGPUResource::get_native_shader_accessor(void) const noexcept {
        return this->shader_pool.get();
    }
    INativeViewAccessor* NativeGPUResource::get_native_view_accessor(void) noexcept {
        return this->view_pool.get();
    }
    const INativeViewAccessor* NativeGPUResource::get_native_view_accessor(void) const noexcept {
        return this->view_pool.get();
    }
    INativeInputLayoutAccessor* NativeGPUResource::get_native_input_layout_accessor(void) noexcept {
        return this->input_layout_pool.get();
    }
    const INativeInputLayoutAccessor* NativeGPUResource::get_native_input_layout_accessor(
        void) const noexcept {
        return this->input_layout_pool.get();
    }
    INativeRasterizerAccessor* NativeGPUResource::get_native_rasterizer_accessor(void) noexcept {
        return this->rasterizer_pool.get();
    }
    const INativeRasterizerAccessor* NativeGPUResource::get_native_rasterizer_accessor(
        void) const noexcept {
        return this->rasterizer_pool.get();
    }
    INativeViewportAccessor* NativeGPUResource::get_native_viewport_accessor(void) noexcept {
        return this->viewport_pool.get();
    }
    const INativeViewportAccessor* NativeGPUResource::get_native_viewport_accessor(void) const noexcept {
        return this->viewport_pool.get();
    }

    IViewAccessor* NativeGPUResource::get_view_accessor(const types::HandleId handle) noexcept {
        return this->view_pool.get();
    }
    const IViewAccessor* NativeGPUResource::get_view_accessor(
        const types::HandleId handle) const noexcept {
        return this->view_pool.get();
    }
} // namespace enishi::renderer::directx