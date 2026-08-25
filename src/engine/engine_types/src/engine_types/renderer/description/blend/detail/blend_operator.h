#pragma once
#include <cstdint>

namespace enishi::types {
    enum class BlendOperator : std::uint8_t {
        Add,
        Subtract,
        ReverseSubtract,
        Min,
        Max,
    };
} // namespace enishi::types