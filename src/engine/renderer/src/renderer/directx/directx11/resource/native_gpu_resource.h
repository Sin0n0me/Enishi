#pragma once
#include "buffer/buffer_pool.h"
#include "input_layout/input_layout_pool.h"
#include "interface_native_resouce_accessor.h"
#include "rasterizer/rasterizer_pool.h"
#include "shader/shader_pool.h"
#include "texture/texture_pool.h"
#include "view/view_pool.h"
#include "viewport/viewport_pool.h"
#include <d3d11.h>
#include <engine_types/handle/handle_type.h>
#include <renderer/common/interface_gpu_resource_accessor.h>
#include <unordered_map>
#include <vector>
#include <wrl/client.h>

namespace enishi::renderer::directx {
    class NativeGPUResource : public INativeResourceAccessor, public IGPUResourceAccessor {
      private:
        std::unique_ptr<InputLayoutPool> input_layout_pool;
        std::unique_ptr<RasterizerPool> rasterizer_pool;
        std::unique_ptr<TexturePool> texture_pool;
        std::unique_ptr<BufferPool> buffer_pool;
        std::unique_ptr<ShaderPool> shader_pool;
        std::unique_ptr<ViewPool> view_pool;
        std::unique_ptr<ViewportPool> viewport_pool;

      public:
        INativeBufferAccessor* get_native_buffer_accessor(void) noexcept override;
        const INativeBufferAccessor* get_native_buffer_accessor(void) const noexcept override;
        INativeTextureAccessor* get_native_texture_accessor(void) noexcept override;
        const INativeTextureAccessor* get_native_texture_accessor(void) const noexcept override;
        INativeShaderAccessor* get_native_shader_accessor(void) noexcept override;
        const INativeShaderAccessor* get_native_shader_accessor(void) const noexcept override;
        INativeViewAccessor* get_native_view_accessor(void) noexcept override;
        const INativeViewAccessor* get_native_view_accessor(void) const noexcept override;
        INativeInputLayoutAccessor* get_native_input_layout_accessor(void) noexcept override;
        const INativeInputLayoutAccessor* get_native_input_layout_accessor(
            void) const noexcept override;
        INativeRasterizerAccessor* get_native_rasterizer_accessor(void) noexcept override;
        const INativeRasterizerAccessor* get_native_rasterizer_accessor(
            void) const noexcept override;
        INativeViewportAccessor* get_native_viewport_accessor(void) noexcept override;
        const INativeViewportAccessor* get_native_viewport_accessor(void) const noexcept override;

      public:
        IViewAccessor* get_view_accessor(void) noexcept override;
        const IViewAccessor* get_view_accessor(void) const noexcept override;
    };
} // namespace enishi::renderer::directx