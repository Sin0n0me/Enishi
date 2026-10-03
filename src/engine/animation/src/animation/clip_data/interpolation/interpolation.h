#pragma once
#include "hermite/hermite_data.h"
#include "vmd/vmd_interpolation_data.h"
#include <cstdint>
#include <variant>

namespace enishi::animation {
    // 補間データのバリアント
    // キーフレームごとにどちらかを持つ
    template <typename T>
    using InterpolationData = std::variant<std::monostate, // Linear, Stepなどの補間データなし
        HermiteTangents<T>,                                // glTF CubicSpline
        VMDAnimationBezier                                 // VMD ベジェ
        >;
} // namespace enishi::animation