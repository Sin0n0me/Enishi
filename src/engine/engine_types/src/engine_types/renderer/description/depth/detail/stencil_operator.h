#pragma once
#include <cstdint>

namespace enishi::types {
    enum class StencilOpeartor : std::uint8_t {
        Keep,
        Zero,
        Replace,
        IncrementClamp,
        DecrementClamp,
        Invert,
        IncrementWrap,
        DecrementWrap
    };
} // namespace enishi::types