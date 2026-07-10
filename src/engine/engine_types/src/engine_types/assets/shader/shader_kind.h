#pragma once
#include <cstdint>

namespace enishi::types {
    enum class ShaderKind : std::uint8_t {
        Vertex = 0,
        Fragment = 1,
        Pixel = 1,
        TessellationControl = 2,
        Hull = 2,
        Domain = 3,
        TessellationEvaluation = 3,
        Amplification = 4,
        Task = 4,
        Geometry,
        Compute,
        Mesh,
    };
} // namespace enishi::types