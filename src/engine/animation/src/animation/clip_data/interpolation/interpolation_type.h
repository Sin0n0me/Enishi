#pragma once
#include <cstdint>

namespace enishi::animation {
    // 補間方式
    enum class InterpolationType : std::uint8_t {
        Linear,      // 線形補間
        Step,        // ステップ(即時切り替え)
        CubicSpline, // 三次スプライン
        VmdBezier,   // VMD専用
    };
} // namespace enishi::animation