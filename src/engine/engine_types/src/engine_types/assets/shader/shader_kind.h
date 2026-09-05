#pragma once
#include <cstdint>

namespace enishi::types {
    // 名称違いは内部的に同じにする
    enum class ShaderKind : std::uint8_t {
        Unknown = 0,

        Vertex = 1,

        Fragment = 2,
        Pixel = 2,

        TessellationControl = 3,
        Hull = 3,

        Domain = 4,
        TessellationEvaluation = 4,

        Amplification = 5,
        Task = 5,

        Geometry,
        Compute,
        Mesh,
    };
} // namespace enishi::types