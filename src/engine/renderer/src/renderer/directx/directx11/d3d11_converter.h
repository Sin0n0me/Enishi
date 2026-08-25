#pragma once
#include <d3d11.h>
#include <dcomp.h>
#include <engine_types/assets/shader/shader_data.h>
#include <engine_types/assets/texture/texture_data.h>
#include <engine_types/renderer/description/pipeline_description.h>
#include <engine_types/renderer/description/rasterizer/rasterizer_description.h>
#include <engine_types/renderer/description/sampler/sampler_description.h>
#include <engine_types/renderer/description/view/image_description.h>
#include <engine_types/renderer/description/view/image_view_description.h>
#include <renderer/common/shader/shader_input_info.h>

namespace enishi::renderer::directx {
    class D3D11Converter {
      public:
        static D3D11_TEXTURE2D_DESC to_texture2d_desc(
            const types::ImageDescription& description) noexcept;
        static D3D11_TEXTURE2D_DESC to_texture2d_desc(
            const types::TextureData& texture_data) noexcept;

        static D3D11_RASTERIZER_DESC to_rasterizer_desc(
            const types::RasterizerStateDescription& description) noexcept;
        static D3D11_SAMPLER_DESC to_sampler_desc(
            const types::SamplerStateDescription& description) noexcept;
        static D3D11_BLEND_DESC to_blend_desc(
            const types::BlendStateDescription& description) noexcept;
        static D3D11_DEPTH_STENCIL_DESC to_depth_stencil_desc(
            const types::DepthStencilStateDescription& description) noexcept;

        static D3D11_FILL_MODE to_fill_mode(const types::FillMode& fill_mode) noexcept;
        static D3D11_CULL_MODE to_cull_mode(const types::CullMode& cull_mode) noexcept;
        static DXGI_FORMAT to_dxgi_format(const types::TextureFormat& format) noexcept;
        static DXGI_FORMAT to_dxgi_format(const types::ImageFormat& format) noexcept;
        static DXGI_FORMAT to_dxgi_format(
            const ShaderInputValueType& type, const std::uint32_t count) noexcept;
        static D3D11_USAGE to_usage(const types::ImageUsage& usage) noexcept;
        static D3D11_PRIMITIVE_TOPOLOGY to_topology(
            const types::PrimitiveTopology& topology) noexcept;
        static D3D11_INPUT_ELEMENT_DESC to_input_element_description(
            const ShaderInputLayout& info) noexcept;
        static D3D11_TEXTURE_ADDRESS_MODE to_texture_address_mode(
            const types::AddressMode& address_mode) noexcept;
        static D3D11_FILTER to_d3d11_filter(const types::FilterMode& min,
            const types::FilterMode& mag,
            const types::FilterMode& mip,
            const types::AnisotropyLevel& anisotropy) noexcept;
        static D3D11_BLEND to_d3d11_blend(const types::BlendFactor& factor) noexcept;
        static D3D11_BLEND_OP to_d3d11_blend_op(const types::BlendOperator& op) noexcept;
        static UINT8 to_d3d11_color_write_mask(const std::uint8_t mask) noexcept;
        static D3D11_COMPARISON_FUNC to_d3d11_comparison_func(
            const types::CompareOperator op) noexcept;
        static D3D11_STENCIL_OP to_d3d11_stencil_op(const types::StencilOpeartor op) noexcept;
        static D3D11_DEPTH_STENCILOP_DESC to_d3d11_depth_stencilop_desc(
            const types::StencilFaceState& face) noexcept;
    };
} // namespace enishi::renderer::directx