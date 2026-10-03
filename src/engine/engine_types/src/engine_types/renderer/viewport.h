#pragma once
#include <cstdint>

namespace enishi::types {
    struct ViewportRect {
        float left_top_x;
        float left_top_y;
        float width;
        float height;
        float min_depth;
        float max_depth;
    };
} // namespace enishi::types