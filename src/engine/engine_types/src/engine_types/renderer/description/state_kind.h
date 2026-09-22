#pragma once
#include <cstdint>

namespace enishi::types {
    enum class StateKind {
        Unknown = 0,
        Blend,
        DepthStencil,
        Rasterizer,
        Sampler,
    };
} // namespace enishi::types