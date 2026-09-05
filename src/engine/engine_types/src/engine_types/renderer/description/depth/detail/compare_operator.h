#pragma once
#include <cstdint>

namespace enishi::types {
    enum class CompareOperator : std::uint8_t {
        Never,
        Less,
        Equal,
        LessEqual,
        Greater,
        NotEqual,
        GreaterEqual,
        Always,
    };
} // namespace enishi::types