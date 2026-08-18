#include "native_gpu_resource.h"

namespace enishi::renderer::directx {
    IBufferAccessor* NativeGPUResource::get_buffer_accessor(void) noexcept {
        return this->buffer_pool.get();
    }
    const IBufferAccessor* NativeGPUResource::get_buffer_accessor(void) const noexcept {
        return this->buffer_pool.get();
    }
    ITextureAccessor* NativeGPUResource::get_texture_accessor(void) noexcept {
        return this->texture_pool.get();
    }
    const ITextureAccessor* NativeGPUResource::get_texture_accessor(void) const noexcept {
        return this->texture_pool.get();
    }
    IShaderAccessor* NativeGPUResource::get_shader_accessor(void) noexcept {
        return this->shader_pool.get();
    }
    const IShaderAccessor* NativeGPUResource::get_shader_accessor(void) const noexcept {
        return this->shader_pool.get();
    }
    IViewAccessor* NativeGPUResource::get_view_accessor(void) noexcept {
        return this->view_pool.get();
    }
    const IViewAccessor* NativeGPUResource::get_view_accessor(void) const noexcept {
        return this->view_pool.get();
    }
} // namespace enishi::renderer::directx