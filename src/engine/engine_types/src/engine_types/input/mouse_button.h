#pragma once
#include <cstdint>

namespace enishi::types {
    enum class MouseButton : std::uint8_t {
        None = 0,

        Left,
        Right,
        Middle,

        Button4,
        Button5,
        Button6,
        Button7,
        Button8,
    };
}