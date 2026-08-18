#pragma once
#include "interface_buffer_accessor.h"
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
    using BufferParameter =
        std::variant<std::monostate, VertexParameter, IndexParameter, UniformParameter>;

    class BufferPool : public IBufferAccessor {
      private:
        std::vector<Microsoft::WRL::ComPtr<ID3D11Buffer>> buffers;
        BufferParameter parameter;

      public:
    };
} // namespace enishi::renderer::directx