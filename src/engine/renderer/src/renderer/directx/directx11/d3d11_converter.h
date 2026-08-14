#pragma once
#include <d3d11.h>
#include <dcomp.h>
#include <engine_types/assets/shader/shader_data.h>
#include <engine_types/assets/texture/texture_data.h>
#include <engine_types/renderer/description/image_description.h>
#include <engine_types/renderer/description/image_view_description.h>
#include <engine_types/renderer/description/pipeline_description.h>
#include <engine_types/renderer/description/rasterizer_description.h>
#include <engine_types/renderer/description/sampler_description.h>
#include <renderer/common/shader_input_info.h>

namespace enishi::renderer::directx {
    class D3D11Converter {
      public:
        static D3D11_TEXTURE2D_DESC to_texture2d_desc(
            const types::ImageDescription& description) noexcept;
        static D3D11_RASTERIZER_DESC to_rasterizer_desc(
            const types::RasterizerDescription& description) noexcept;

        static D3D11_FILL_MODE to_fill_mode(const types::FillMode& fill_mode) noexcept;
        static D3D11_CULL_MODE to_cull_mode(const types::CullMode& cull_mode) noexcept;
        static DXGI_FORMAT to_dxgi_format(const types::ImageFormat& format) noexcept;
        static DXGI_FORMAT to_dxgi_format(const types::VertexFormat& format) noexcept;
        static D3D11_USAGE to_usage(const types::ImageUsage& usage) noexcept;
        static D3D11_PRIMITIVE_TOPOLOGY to_topology(
            const types::PrimitiveTopology& topology) noexcept;
        static D3D11_INPUT_ELEMENT_DESC to_input_element_description(
            const ShaderInputInfo& info) noexcept;
    };
} // namespace enishi::renderer::directx