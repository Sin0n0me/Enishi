#pragma once
#include <cstdint>

namespace enishi::types {
    struct WindowSize {
        std::int32_t width;
        std::int32_t height;

        constexpr float aspect_ratio(void) const noexcept {
            return static_cast<float>(this->width) / static_cast<float>(this->height);
        }
    };

    struct WindowPosition {
        std::int32_t x;
        std::int32_t y;
    };
} // namespace enishi::types