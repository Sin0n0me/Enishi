#pragma once
#include <cstdint>
#include <d3d11.h>
#include <engine_types/assets/shader/shader_kind.h>
#include <variant>
#include <wrl/client.h>

namespace enishi::renderer::directx {
    struct VertexParameter {
        std::uint32_t stride;
        std::uint32_t offset;
        std::uint32_t target_slot;
    };

    struct IndexParameter {
        DXGI_FORMAT format;
        std::uint32_t offset;
    };

    struct UniformParameter {
        types::ShaderKind target_shader;
        std::uint32_t target_slot;
    };

    // parameter
    using BufferParameter = std::variant<VertexParameter, IndexParameter, UniformParameter>;

    struct alignas(8) Buffer {
        Microsoft::WRL::ComPtr<ID3D11Buffer> buffer;
        BufferParameter parameter;

        explicit Buffer(const VertexParameter& parameter);
        explicit Buffer(const IndexParameter& parameter);
        explicit Buffer(const UniformParameter& parameter);
    };
} // namespace enishi::renderer::directx