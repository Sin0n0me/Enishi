#include "d3d11_converter.h"

namespace enishi::renderer::directx {
    D3D11_TEXTURE2D_DESC D3D11Converter::to_texture2d_desc(
        const types::ImageDescription& description) noexcept {
        const DXGI_SAMPLE_DESC sample{
            .Count = description.samples,
        };

        return D3D11_TEXTURE2D_DESC{
            .Width = static_cast<UINT>(description.size.x),
            .Height = static_cast<UINT>(description.size.y),
            .MipLevels = description.mip_levels,
            .ArraySize = description.array_layers,
            .Format = D3D11Converter::to_dxgi_format(description.format),
            .SampleDesc = sample,
            .Usage = D3D11Converter::to_usage(description.usage),
            .BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE, // TODO
        };
    }

    D3D11_RASTERIZER_DESC D3D11Converter::to_rasterizer_desc(
        const types::RasterizerDescription& description) noexcept {
        const bool is_clockwise = description.front_face == types::FrontFace::CounterClockwise;

        return D3D11_RASTERIZER_DESC{
            .FillMode = D3D11Converter::to_fill_mode(description.fill_mode),
            .CullMode = D3D11Converter::to_cull_mode(description.cull_mode),
            .FrontCounterClockwise = is_clockwise ? TRUE : FALSE,
            .DepthBias = static_cast<INT>(description.depth_bias.constant_factor),
            .SlopeScaledDepthBias = description.depth_bias.clamp,
            .DepthClipEnable = description.depth_bias.enable ? TRUE : FALSE,
        };
    }

    D3D11_FILL_MODE D3D11Converter::to_fill_mode(const types::FillMode& fill_mode) noexcept {
        switch (fill_mode) {
            case types::FillMode::Solid:
                return D3D11_FILL_MODE::D3D11_FILL_SOLID;
            case types::FillMode::Wireframe:
                return D3D11_FILL_MODE::D3D11_FILL_WIREFRAME;
            default:
                break;
        }

        return D3D11_FILL_MODE::D3D11_FILL_SOLID;
    }

    D3D11_CULL_MODE D3D11Converter::to_cull_mode(const types::CullMode& cull_mode) noexcept {
        switch (cull_mode) {
            case types::CullMode::Back:
                return D3D11_CULL_MODE::D3D11_CULL_BACK;
            case types::CullMode::Front:
                return D3D11_CULL_MODE::D3D11_CULL_FRONT;
            case types::CullMode::None:
                return D3D11_CULL_MODE::D3D11_CULL_NONE;
            default:
                break;
        }

        return D3D11_CULL_MODE::D3D11_CULL_FRONT;
    }

    DXGI_FORMAT D3D11Converter::to_dxgi_format(const types::ImageFormat& format) noexcept {
        switch (format) {
            case types::ImageFormat::BC3_UNORM:
                return DXGI_FORMAT::DXGI_FORMAT_BC3_UNORM;
            case types::ImageFormat::BC7_UNORM:
                return DXGI_FORMAT::DXGI_FORMAT_BC7_UNORM;
            case types::ImageFormat::BGRA8_UNORM:
                return DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM;
            case types::ImageFormat::D16_UNORM:
                return DXGI_FORMAT::DXGI_FORMAT_D16_UNORM;
            case types::ImageFormat::D24_UNORM_S8_UINT:
                return DXGI_FORMAT::DXGI_FORMAT_D24_UNORM_S8_UINT;
            case types::ImageFormat::D32_FLOAT:
                return DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;
            case types::ImageFormat::RGBA16_FLOAT:
                return DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
            case types::ImageFormat::RGBA8_UNORM:
                return DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
            default:
                break;
        }

        return DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
    }

    D3D11_USAGE D3D11Converter::to_usage(const types::ImageUsage& usage) noexcept {
        switch (usage) {
            case types::ImageUsage::None:
                return D3D11_USAGE::D3D11_USAGE_DEFAULT;
            case types::ImageUsage::DepthStencil:
                return D3D11_USAGE::D3D11_USAGE_DEFAULT;
            case types::ImageUsage::RenderTarget:
                return D3D11_USAGE::D3D11_USAGE_DEFAULT;
            case types::ImageUsage::ShaderResource:
                return D3D11_USAGE::D3D11_USAGE_DEFAULT;
            case types::ImageUsage::TransferDst:
                return D3D11_USAGE::D3D11_USAGE_DEFAULT;
            case types::ImageUsage::TransferSrc:
                return D3D11_USAGE::D3D11_USAGE_DEFAULT;
            case types::ImageUsage::UnorderedAccess:
                return D3D11_USAGE::D3D11_USAGE_DEFAULT;
            default:
                break;
        }

        return D3D11_USAGE::D3D11_USAGE_DEFAULT;
    }

    D3D11_PRIMITIVE_TOPOLOGY D3D11Converter::to_topology(
        const types::PrimitiveTopology& topology) noexcept {
        switch (topology) {
            case types::PrimitiveTopology::LineList:
                return D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
            case types::PrimitiveTopology::PointList:
                return D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
            case types::PrimitiveTopology::TriangleList:
                return D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
            default:
                break;
        }

        return D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
    }
} // namespace enishi::renderer::directx