#pragma once
#include <cstdint>

namespace enishi::types {
    enum class PrimitiveTopology : std::uint8_t {
        TriangleList, // 三角形リスト
        LineList,     // 線分リスト
        PointList,    // 点リスト
    };
} // namespace enishi::types