#include "native_gpu_resource.h"

namespace enishi::renderer::directx {
    NativeGPUResource::NativeGPUResource(void)
        : input_layout_pool(std::make_unique<InputLayoutPool>())
        , rasterizer_pool(std::make_unique<RasterizerPool>())
        , texture_pool(std::make_unique<TexturePool>())
        , buffer_pool(std::make_unique<BufferPool>())
        , shader_pool(std::make_unique<ShaderPool>())
        , view_pool(std::make_unique<ViewPool>())
        , viewport_pool(std::make_unique<ViewportPool>())
        , mesh_pool(std::make_unique<MeshPool>()) {
    }

    INativeBufferAccessor* NativeGPUResource::get_native_buffer_accessor(void) noexcept {
        return this->buffer_pool.get();
    }
    const INativeBufferAccessor* NativeGPUResource::get_native_buffer_accessor(
        void) const noexcept {
        return this->buffer_pool.get();
    }
    INativeTextureAccessor* NativeGPUResource::get_native_texture_accessor(void) noexcept {
        return this->texture_pool.get();
    }
    const INativeTextureAccessor* NativeGPUResource::get_native_texture_accessor(
        void) const noexcept {
        return this->texture_pool.get();
    }
    INativeShaderAccessor* NativeGPUResource::get_native_shader_accessor(void) noexcept {
        return this->shader_pool.get();
    }
    const INativeShaderAccessor* NativeGPUResource::get_native_shader_accessor(
        void) const noexcept {
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
    const INativeViewportAccessor* NativeGPUResource::get_native_viewport_accessor(
        void) const noexcept {
        return this->viewport_pool.get();
    }

    IViewAccessor* NativeGPUResource::get_view_accessor(void) noexcept {
        return this->view_pool.get();
    }
    const IViewAccessor* NativeGPUResource::get_view_accessor(void) const noexcept {
        return this->view_pool.get();
    }
    IShaderAccessor* NativeGPUResource::get_shader_accessor(void) noexcept {
        return this->shader_pool.get();
    }
    const IShaderAccessor* NativeGPUResource::get_shader_accessor(void) const noexcept {
        return this->shader_pool.get();
    }
    IMeshAccessor* NativeGPUResource::get_mesh_accessor(void) noexcept {
        return this->mesh_pool.get();
    }
    const IMeshAccessor* NativeGPUResource::get_mesh_accessor(void) const noexcept {
        return this->mesh_pool.get();
    }
} // namespace enishi::renderer::directx