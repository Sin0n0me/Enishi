#pragma once
#include <cstdint>
#include <engine_types/renderer/vertex/vertex_layout.h>
#include <string_view>

namespace enishi::renderer {
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
        std::uint32_t array_size;
        types::VertexFormat format;
        std::uint32_t component_count;
        std::uint32_t component_bit_width;
        ShaderInputComponentType component_type;
        std::uint32_t offset;
    };
} // namespace enishi::renderer