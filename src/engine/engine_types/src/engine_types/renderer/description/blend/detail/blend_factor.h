#pragma once
#include <cstdint>

namespace enishi::types {
    enum class BlendFactor : std::uint8_t {
        Zero,
        One,

        SrcColor,
        OneMinusSrcColor,

        DstColor,
        OneMinusDstColor,

        SrcAlpha,
        OneMinusSrcAlpha,

        DstAlpha,
        OneMinusDstAlpha,

        ConstantColor,
        OneMinusConstantColor,

        ConstantAlpha,
        OneMinusConstantAlpha,

        SrcAlphaSaturate,

        Src1Color,
        OneMinusSrc1Color,

        Src1Alpha,
        OneMinusSrc1Alpha,
    };
} // namespace enishi::types