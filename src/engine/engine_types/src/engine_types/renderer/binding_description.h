#pragma once
#include <cstdint>
#include <engine_types/assets/shader/shader_kind.h>
#include <engine_types/assets/texture/texture_data.h>
#include <variant>
#include <vector>

namespace enishi::types {
    enum class ResourceType : std::uint8_t {
        Buffer,
        Texture,
        Sampler,
        Shader,
        Pipeline,
    };

    enum class BindingType : std::uint8_t {
        UniformBuffer,
        StorageBuffer,
        Texture,
        StorageTexture,
        Sampler,
        CombinedTextureSampler,
    };

    struct VertexBufferParameter {
        std::uint32_t target;
        std::uint32_t stride;
        std::uint32_t offset;
    };

    struct IndexBufferParameter {
        std::uint32_t stride;
        std::uint32_t offset;
    };

    struct UniformBufferParameter {
        std::uint32_t target;
        types::ShaderKind target_shader;
    };

    struct BufferBinding {
        using BufferParameter =
            std::variant<VertexBufferParameter, IndexBufferParameter, UniformBufferParameter>;
        BufferParameter parameter;
    };

    struct TextureBinding {
        std::uint32_t mip_level;
        std::uint32_t mip_count;
        std::uint32_t array_slice;
        std::uint32_t array_count;
        // TextureViewType view_type;
    };

    struct BlendStateParameter {};

    struct DepthStencilStateParameter {};

    struct SamplerStateParameter {
        types::ShaderKind target_shader;
        std::uint32_t target;
    };

    struct RasterizerStateParameter {};

    struct StateBinding {
        using StateParameter = std::variant<BlendStateParameter,
            DepthStencilStateParameter,
            SamplerStateParameter,
            RasterizerStateParameter>;
        StateParameter parameter;
    };

    struct ShaderBinding {};

    struct DepthStencilParameter {};

    struct RenderTargetParameter {};

    struct ShaderResourceParameter {
        types::ShaderKind target_shader;
        std::uint32_t target;
    };
    struct UnorderedAccessParameter {
        std::uint32_t target;
        std::uint32_t stride;
        std::uint32_t offset;
    };

    struct ViewBinding {
        using ViewParameter = std::variant<DepthStencilParameter,
            RenderTargetParameter,
            ShaderResourceParameter,
            UnorderedAccessParameter>;
        ViewParameter parameter;
    };

    struct DrawParameter {
        std::uint32_t vertex_count;
        std::uint32_t instance_count;
        std::uint32_t first_vertex;
        std::uint32_t first_instance;
    };

    struct DrawIndexedParameter {
        std::uint32_t index_count;
        std::uint32_t instance_count;
        std::int32_t first_index;
        std::uint32_t vertex_offset;
        std::uint32_t first_instance;
    };

    struct DrawBinding {
        using DrawParameter = std::variant<DrawParameter, DrawIndexedParameter>;
        DrawParameter parameter;
    };

    struct BindingKey {
        std::uint32_t space;
        std::uint32_t binding;
    };
} // namespace enishi::types