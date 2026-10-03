#pragma once
#include "blend_factor.h"
#include "blend_operator.h"
#include <cstdint>

namespace enishi::types {
    enum class ColorWriteMask : std::uint8_t {
        ColorWriteNone = 0,
        ColorWriteR = 1u << 0,
        ColorWriteG = 1u << 1,
        ColorWriteB = 1u << 2,
        ColorWriteA = 1u << 3,

        ColorWriteRGBA = ColorWriteR | ColorWriteG | ColorWriteB | ColorWriteA
    };
} // namespace enishi::types