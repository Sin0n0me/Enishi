#pragma once
#include "../../animation.h"
#include <cmath>

namespace enishi::animation {
    // glTF CubicSpline 用タンジェント
    template <typename T> struct HermiteTangents {
        T in_tangent;
        T out_tangent;
    };
} // namespace enishi::animation