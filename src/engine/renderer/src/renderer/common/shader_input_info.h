#pragma once
#include <cstdint>
#include <string_view>

namespace enishi::renderer {
    enum class ShaderInputDataType {
        Unknown,

        Float,
        Float2,
        Float3,
        Float4,

        Int,
        Int2,
        Int3,
        Int4,

        UInt,
        UInt2,
        UInt3,
        UInt4,

        Double,
        Double2,
        Double3,
        Double4,

        Float2x2,
        Float3x3,
        Float4x4,
    };

    enum class ShaderInputComponentType {
        Unknown,
        Float,
        SignedInteger,
        UnsignedInteger,
        Double,
    };

    struct ShaderInputInfo {
        std::string_view name;
        std::uint32_t location; // DirectXならSlotに該当
        std::uint32_t component_count;
        std::uint32_t component_bit_width;
        std::uint32_t array_size;
        std::uint32_t format;
        ShaderInputDataType data_type;
        ShaderInputComponentType component_type;
    };
} // namespace enishi::renderer