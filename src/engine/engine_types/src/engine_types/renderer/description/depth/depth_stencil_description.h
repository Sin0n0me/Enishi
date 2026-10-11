#pragma once
#include "detail/depth_state.h"
#include "detail/stencil_state.h"
#include <cstdint>

namespace enishi::types {
    struct DepthStencilStateDescription {
        DepthState depth;
        StencilState stencil;
    };
} // namespace enishi::types