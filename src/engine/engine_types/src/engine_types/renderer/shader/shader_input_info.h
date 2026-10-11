#pragma once
#include <cstdint>
#include <engine_types/renderer/vertex/vertex_layout.h>
#include <string>

namespace enishi::types {
    enum class ShaderInputValueType {
        Unknown,
        Float,
        SignedInteger,
        UnsignedInteger,
        Double,
    };

    enum class ShaderInputResourceType : std::uint8_t {
        Unknown,
        UniformBuffer,
        StorageBuffer,
        Texture,
        StorageTexture,
        Sampler,
    };

    enum class ShaderInputResourceDimension : std::uint8_t {
        Unknown,
        Buffer,
        Texture1D,
        Texture2D,
        Texture3D,
        TextureCube,
        Texture1DArray,
        Texture2DArray,
        TextureCubeArray,
        Texture2DMS,
        Texture2DMSArray,
    };

    enum class ShaderInputResourceAccess : std::uint8_t {
        ReadOnly,
        ReadWrite,
    };

    struct ShaderInputLayout {
        std::string name;
        ShaderInputValueType value_type;
        std::uint32_t location; // DirectXならSlot
        std::uint32_t array_size;
        std::uint32_t component; //
        std::uint32_t component_count;
    };

    struct ShaderInputResource {
        std::string name;
        ShaderInputResourceType type;
        ShaderInputResourceDimension dimension;
        ShaderInputResourceAccess read_only;
        std::uint32_t set;
        std::uint32_t binding;
        std::uint32_t array_size;
    };
} // namespace enishi::types